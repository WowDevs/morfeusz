/* -*- C++ -*-
 * Copyright (C) 2009 Trinity Core <http://www.trinitycore.org>
 * Copyright (C) 2012 Morpheus
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 *  @file
 *  @brief   Proxy_Service implementation
 *  @author  raczman <raczman@gmail.com>
 *  @date    2009-11-13
 *
 */

#include <ace/Thread_Manager.h>
#include <ace/Dev_Poll_Reactor.h>
#include <ace/ARGV.h>
#include "Configuration.h"
#include "Proxy_Service.h"
#include "Proxyd_EC_Communicator.h"
#include "Proxy_Timer.h"
#include "dbc/DBC_Store.h"
#include "dbc/DBC_File.h"
#include "Log.h"

#include "ObjectMgr.h"

namespace Morpheus
{

namespace Proxyd
{

class Proxy_Signal_Handler : public ACE_Event_Handler
{
public:
    int handle_signal(int signum, siginfo_t*, ucontext_t*)
    {
        PROXY_LOG("[SIGNAL] %s (%d) occured\n", ACE_OS::strsignal(signum), signum);
        sProxy->stop();
        return 0;
    }
};

void Proxy_Service::start()
{
    PROXY_TRACE;
    //Boilerplate code goes in here.
    PROXY_LOG("Starting proxyd\n");

    sLog->initialize(LOG_TYPE_PROXYD);
    sLog->outDetail(LOG_FILTER_SYSTEM, "Log system initialized.");

    this->load = 0;
    this->connection_limit = sConfig->getInt("proxyd","ConnectionLimit");
    this->realm_id = sConfig->getInt("proxyd","RealmID");
    this->current_connections = 0;
    this->announce_timer_id = -1;

#if defined (ACE_HAS_EVENT_POLL) || defined (ACE_HAS_DEV_POLL)
    this->reactor = new ACE_Reactor(new ACE_Dev_Poll_Reactor());
    this->reactor->restart(1);
#else
    this->reactor = new ACE_Reactor(new ACE_TP_Reactor(), 1);
#endif
    this->acceptor = new ProxydAcceptor();

    try
    {
        sDBC->open();
        sDBC->load_item_dbc();
        sDBC->load_spell_item_enchantments_dbc();
        sDBC->load_chr_races_dbc();
        sDBC->load_chr_classes_dbc();
    }
    catch (DBC::DBC_DIR_Read_Exception& e)
    {
        ACE_DEBUG((LM_ERROR, "Couldn't open DBC directory: %s\n", e.What().c_str()));
        return;
    }
    catch (DBC::DBC_Read_Exception& e)
    {
        ACE_DEBUG((LM_ERROR, "[%s] - %s\n", e.File().c_str(), e.What().c_str()));
        return;
    }

    if (this->acceptor->
            open(ACE_INET_Addr(sConfig->getString("proxyd","BindAddr").c_str()),
            this->reactor) == -1)
    {
        PROXY_LOG("Couldn't bind to interface!\n");
        delete this->reactor;
        delete this->acceptor;
        return;
    }

    ACE_ARGV* orb_args = new ACE_ARGV;
    ACE_Time_Value tm(sConfig->getInt("proxyd", "AnnounceInterval"));

    try {
        orb_args->add("");
        orb_args->add("-ORBInitRef");
        std::string str = "NameService=";
        str += sConfig->getString("corba","NSLocation");
        str += "/NameService";
        orb_args->add(str.c_str());
        int argc = orb_args->argc();
        PROXY_LOG("Using %s\n", str.c_str());
        this->orb = CORBA::ORB_init(argc, orb_args->argv(), NULL);
        //Although EC_Communicator catches exceptions, we dont wanna call
        //its functions in case of a CORBA exception.
        this->event_channel = new EC_Communicator(this->orb.in());
        this->event_channel->connect();
    }
    catch (CORBA::Exception &e) {
        ACE_DEBUG((LM_ERROR, "CORBA exception!\n"));
        delete orb_args;
        return;
    }

    delete orb_args;

    this->is_running = true;
    this->database = new ProxyDB(sConfig->getInt("proxyd","DBThreads"));
    this->database->open(sConfig->getString("proxyd","DBengine"),
                        sConfig->getString("proxyd","DBUrl"));

    this->activate(THR_NEW_LWP | THR_JOINABLE,
                sConfig->getInt("proxyd","NetThreads"));

    this->database->load_player_createinfo();
    sObjectMgr->init_max_guids();

    this->event_channel->announce();
    this->announce_timer_id = this->reactor->schedule_timer(
                                    new Proxy_Announce_Timer(),
                                    0,
                                    tm,
                                    tm);

    // register the signal handler
    Proxy_Signal_Handler proxy_sig_handler;
    ACE_Sig_Handler sig_handler;
    sig_handler.register_handler(SIGINT, &proxy_sig_handler);

    ACE_Thread_Manager::instance()->wait();
    return;
}

void Proxy_Service::stop()
{
    PROXY_LOG("Proxy shutting down...\n");
    this->event_channel->shutdowned();
    this->is_running = false;
    this->database->close();
}

int Proxy_Service::svc()
{
    PROXY_TRACE;
    ACE_Time_Value tm;
    tm.msec(100);
    while (this->is_running) {
        tm.msec(100);
        this->reactor->handle_events(tm);
        tm.msec(100);
        if (this->orb->work_pending())
            this->orb->run(tm);
    }

    return 0;
}

void Proxy_Service::update_connections(bool up)
{
    PROXY_TRACE;
    if (up)
        this->current_connections++;
    else
        this->current_connections--;

    this->load = this->current_connections.value() / this->connection_limit;
}

};
};
