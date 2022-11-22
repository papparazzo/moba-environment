/*
 *  Project:    moba-environment
 *
 *  Copyright (C) 2016 Stefan Paproth <pappi-@gmx.de>
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

#pragma once

#include "moba/endpoint.h"
#include "bridge.h"

#include <string>

class MessageLoop : private boost::noncopyable {
    public:
        MessageLoop(
            BridgePtr bridge,
            EndpointPtr endpoint
        );

        MessageLoop(const MessageLoop&) = delete;
        MessageLoop& operator=(const MessageLoop&) = delete;

        void run();
        void connect();

    protected:
        struct AmbientLightData {
            int red;
            int green;
            int blue;
            int white;
        };
/*
        void printError(moba::JsonItemPtr ptr);
        void checkSwitchState();
        void setAutoMode(bool on);
        void setEnvironment(moba::JsonItemPtr ptr);
        void globalTimerEvent(moba::JsonItemPtr ptr);
        void setHardwareState(const std::string &s);
        void setAmbience(moba::JsonItemPtr ptr);
        void setAmbientLight(moba::JsonItemPtr ptr);
        void setAmbientLight();
*/
        bool automatic;

        EndpointPtr endpoint;

        BridgePtr bridge;

        Bridge::StatusBarState statusbarState;
        AmbientLightData ambientLightData;
};
