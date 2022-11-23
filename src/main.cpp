/*
 *  Project:    moba-environment
 *
 *  Copyright (C) 2015 Stefan Paproth <pappi-@gmx.de>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/agpl.txt>.
 *
 */

#include <config.h>

#include <iostream>
#include <memory>

#include <moba-common/version.h>
#include <moba-common/helper.h>
#include <moba-common/ipc.h>

#include "bridge.h"
#include "msgloop.h"
#include "moba/endpoint.h"
#include "moba/socket.h"


/*
#include <cstdio>
#include <cstdlib>
#include <libgen.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <iomanip>
#include <errno.h>

#include <sys/time.h>
#include <sys/resource.h>
#include <stdlib.h>

*/

namespace {
    moba::common::AppData appData = {
        PACKAGE_NAME,
        moba::common::Version{PACKAGE_VERSION},
        __DATE__,
        __TIME__,
        "::1",
        7000
    };

    std::string pidfile = "/run/environment.pid";
}

int main(int argc, char* argv[]) {
    int key = moba::common::IPC::DEFAULT_KEY;

    switch(argc) {
        case 4:
            key = atoi(argv[3]);

        case 3:
            appData.port = atoi(argv[2]);

        case 2:
            appData.host = std::string(argv[1]);

        default:
            break;
    }

    if(geteuid() != 0) {
        std::cerr << "This daemon can only be run by root user, exiting" << std::endl;
        exit(EXIT_FAILURE);
    }
/*
    int fh = open(pidfile, O_RDWR | O_CREAT, 0644);

    if(fh == -1) {
        std::cout << "Could not open PID lock file <" << pidfile << ">, exiting" << std::endl;
        exit(EXIT_FAILURE);
    }

    if(lockf(fh, F_TLOCK, 0) == -1) {
        std::cout << "Could not lock PID lock file <" << pidfile << ">, exiting" << std::endl;
        exit(EXIT_FAILURE);
    }
*/

    //auto ipc = std::make_shared<moba::common::IPC>(key, moba::common::IPC::TYPE_CLIENT);
    auto bridge = std::make_shared<Bridge>(/*ipc*/);
    auto socket = std::make_shared<Socket>(appData.host, appData.port);
    auto endpoint = EndpointPtr{new Endpoint{
        socket,
        appData.appName,
        appData.version,
        {Message::SYSTEM, Message::TIMER, Message::ENVIRONMENT}
    }};

    MessageLoop loop{bridge, endpoint};
    loop.run();
    exit(EXIT_SUCCESS);
}
