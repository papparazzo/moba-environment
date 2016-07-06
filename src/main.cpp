/*
 *  Project:    Environment-Application
 *
 *  Version:    1.0.0
 *  History:    V1.0    15/10/2013  SP - created
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


///////
//

#include <iostream>
#include "bridge.h"
//
//////

#include <cstdio>
#include <cstdlib>
#include <libgen.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <iomanip>

//#include <sys/types.h>
//#include <sys/stat.h>
//#include <fcntl.h>

#include <moba/log.h>
#include <moba/version.h>
#include <moba/helper.h>

#include "msgloop.h"

namespace {
    AppData appData = {
        "",
        "1.0.0",
        __DATE__,
        __TIME__,
        "localhost",
        7000
    };

    std::string pidfile = "/var/run/environment.pid";
}

int main(int argc, char* argv[]) {
////////////////////////////////////////////////////////////////////////////////
//
/** /
    Bridge bridge;
    int i;

    while(true) {
        //system("clear");

        std::cout << "1:  mainLightOn" << std::endl;
        std::cout << "2:  mainLightOff" << std::endl;
        std::cout << "3:  curtainUp" << std::endl;
        std::cout << "4:  curtainDown" << std::endl;
        std::cout << "5:  thunderstormOn" << std::endl;
        std::cout << "6:  thunderstormOff" << std::endl;
        std::cout << "7:  statusbar" << std::endl;


        std::cout << "99: Programmende" << std::endl;
        std::cout << "?";

        int cmd = 0;
        std::cin >> cmd;

        switch(cmd) {
            case 1:
                bridge.mainLightOn();
                break;

            case 2:
                bridge.mainLightOff();
                break;

            case 3:
                bridge.curtainUp();
                break;

            case 4:
                bridge.curtainDown();
                break;

            case 5:
                bridge.thunderstormOn();
                break;

            case 6:
                bridge.thunderstormOff();
                break;

            case 7:
                i = ++i % 6;
                bridge.setStatusBar((Bridge::StatusBarState)i);
                break;

            case 99:
                return 0;

        }

    }
//*/
////////////////////////////////////////////////////////////////////////////////










    switch(argc) {
        case 3:
            appData.port = atoi(argv[2]);

        case 2:
            appData.host = std::string(argv[1]);

        case 1:
            appData.appName = basename(argv[0]);

        default:
            break;
    }
    printAppData(appData);

    if(!setCoreFileSizeToULimit()) {
        LOG(tlog::Warning) << "Could not set corefile-size to unlimited";
    }

    if(geteuid() != 0) {
        LOG(tlog::Error) << "This daemon can only be run by root user, exiting";
	    exit(EXIT_FAILURE);
	}
/*
    int fh = open(pidfile, O_RDWR | O_CREAT, 0644);

    if(fh == -1) {
        LOG(tlog::Error) << "Could not open PID lock file <" << pidfile << ">, exiting";
        exit(EXIT_FAILURE);
    }

    if(lockf(fh, F_TLOCK, 0) == -1) {
        LOG(tlog::Error) << "Could not lock PID lock file <" << pidfile << ">, exiting";
        exit(EXIT_FAILURE);
    }
*/
    boost::shared_ptr<Bridge> bridge(new Bridge());

    while(true) {
        try {
            MessageLoop loop(appData.appName, Version(appData.version), bridge);
            loop.connect(appData.host, appData.port);
            loop.init();
            loop.run();
            exit(EXIT_SUCCESS);
        } catch(std::exception &e) {
            LOG(tlog::Notice) << e.what();
            sleep(4);
        }
    }
}

