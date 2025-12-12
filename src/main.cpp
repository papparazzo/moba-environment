/*
 *  Project:    moba-environment
 *
 *  Copyright (C) 2025 Stefan Paproth <pappi-@gmx.de>
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
 *  along with this program. If not, see <https://www.gnu.org/licenses/agpl.txt>.
 *
 */

#include <config.h>

#include <memory>

#include <moba-common/daemon.h>
#include <moba-common/ini.h>
#include <moba-common/version.h>
#include <moba-common/helper.h>
#include <moba-common/ipc.h>

#include "bridge.h"
#include "eclipsecontrol.h"
#include "statuscontrol.h"
#include "msgloop.h"
#include "moba/endpoint.h"
#include "moba/socket.h"

namespace {
    moba::AppData appData = {
        PACKAGE_NAME,
        moba::Version{PACKAGE_VERSION},
        __DATE__,
        __TIME__,
        "::1",
        7000
    };
}

int main(const int argc, char *argv[]) {
    if(argc == 2) {
        appData.host = std::string(argv[1]);
    }

	unsigned int deviceId = 0;

    moba::Daemon daemon{appData.appName};

    daemon.daemonize();

    auto ini = std::make_shared<moba::Ini>(std::string{SYSCONFIR} + "/" + PACKAGE_NAME + ".conf");

    int key = ini->getInt("settings", "ipc_key", moba::IPC::DEFAULT_KEY);
    appData.port = ini->getInt("settings", "port", appData.port);
    appData.host = ini->getString("settings", "host", appData.host);

    auto socket = std::make_shared<Socket>(appData.host, appData.port);
    auto endpoint = EndpointPtr{new Endpoint{
        socket,
        appData.appName,
        appData.version,
        {Message::SYSTEM, Message::TIMER, Message::ENVIRONMENT}
    }};

    auto bridge = std::make_shared<Bridge>();
    auto status = std::make_shared<StatusControl>(bridge, endpoint);
    auto eclctr = std::make_shared<EclipseControl>(bridge, ini);




    //auto ipc = std::make_shared<moba::IPC>(key, moba::IPC::TYPE_CLIENT);

    MessageLoop loop{endpoint, status, eclctr, bridge};
    loop.run();
    exit(EXIT_SUCCESS);
}
