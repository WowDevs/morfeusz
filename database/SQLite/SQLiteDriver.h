/*
 * Copyright (C) 2009 Dawn Of Reckoning
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

/*
 *  @file    SQLiteDriver.h
 *  @brief
 *  @author  Albator
 *  @date    2009-07-22
 *
 */
#ifndef SQLITEDRIVER_H_
#define SQLITEDRIVER_H_

#include "../Driver.h"

namespace Morpheus
{
namespace SQL
{

class Connection;

class SQLiteDriver : public Driver
{
public:

    Connection* connect(const std::string& url);

    const uint32 getMajorVersion() const;

    const uint32 getMinorVersion() const;

private:

    static const uint32 majorVersion;
    static const uint32 minorVersion;
};

};
};

#endif /* SQLITEDRIVER_H_ */
