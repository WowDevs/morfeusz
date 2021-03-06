/* -*- C++ -*-
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

#include "ObjectMgr.h"
#include "Proxy_Service.h"

namespace Morpheus
{

namespace Managers
{

void ObjectMgr::player_create_info_loaded(uint8 loaded)
{
    printf("Loaded %u character creation information sets.\n", loaded);
}

void ObjectMgr::init_max_guids()
{
    // Characters
    SqlOperationRequest* op = new SqlOperationRequest(PROXYD_DB_GET_CHAR_MAX_GUID);
    Morpheus::SQL::ResultSet* res = sProxy->get_db()->enqueue_synch_query(op); // TODO: This can be made asynchroneously to speed up startup

    res->next();
    max_guids[GUID_CHAR] = res->getUint32(1);
    max_guids[GUID_CHAR]++;
}

};
};
