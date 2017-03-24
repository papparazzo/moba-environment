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

#include "bridge.h"
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

#include <moba/log.h>
#include <moba/version.h>
#include <moba/helper.h>
#include <moba/ipc.h>

#include <config.h>

#include "msgloop.h"

namespace {
    moba::AppData appData = {
        PACKAGE_NAME,
        moba::Version(PACKAGE_VERSION),
        __DATE__,
        __TIME__,
        "localhost",
        7000
    };

    std::string pidfile = "/var/run/environment.pid";
}

int main(int argc, char* argv[]) {
    int key = moba::IPC::DEFAULT_KEY;

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
    moba::printAppData(appData);

    if(!moba::setCoreFileSizeToULimit()) {
        LOG(moba::WARNING) << "Could not set corefile-size to unlimited" << std::endl;
    }

    if(geteuid() != 0) {
        LOG(moba::ERROR) << "This daemon can only be run by root user, exiting" << std::endl;
	    exit(EXIT_FAILURE);
	}
/*
    int fh = open(pidfile, O_RDWR | O_CREAT, 0644);

    if(fh == -1) {
        LOG(tlog::Error) << "Could not open PID lock file <" << pidfile << ">, exiting" << std::endl;
        exit(EXIT_FAILURE);
    }

    if(lockf(fh, F_TLOCK, 0) == -1) {
        LOG(tlog::Error) << "Could not lock PID lock file <" << pidfile << ">, exiting" << std::endl;
        exit(EXIT_FAILURE);
    }
*/
    boost::shared_ptr<moba::IPC> ipc(new moba::IPC(key, moba::IPC::TYPE_CLIENT));
    boost::shared_ptr<Bridge> bridge(new Bridge(ipc));

    while(true) {
        try {
            MessageLoop loop(appData.appName, appData.version, bridge);
            loop.connect(appData.host, appData.port);
            loop.init();
            loop.run();
            exit(EXIT_SUCCESS);
        } catch(std::exception &e) {
            LOG(moba::WARNING) << e.what() << std::endl;
            sleep(4);
        }
    }
}
