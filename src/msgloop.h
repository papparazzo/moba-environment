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

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

#include "bridge.h"

#include <moba/msghandler.h>
#include <moba/log.h>

class MessageLoop : private boost::noncopyable {
    public:
        MessageLoop(
            const std::string &appName,
            const moba::Version &version,
            boost::shared_ptr<Bridge> bridge
        );

        void run();
        void init();
        void connect(const std::string &host, int port);

    protected:
        struct AmbientLightData {
            int red;
            int blue;
            int white;
        };

        void printError(moba::JsonItemPtr ptr);
        void checkSwitchState();
        void setAutoMode(bool on);
        void setEnvironment(moba::JsonItemPtr ptr);
        void globalTimerEvent(moba::JsonItemPtr ptr);
        void setHardwareState(const std::string &s);
        void setAmbience(moba::JsonItemPtr ptr);
        void setAmbientLight(moba::JsonItemPtr ptr);
        void setAmbientLight();

        bool automatic;

        moba::MsgHandler msgHandler;
        long appId;
        std::string appName;
        moba::Version version;
        boost::shared_ptr<Bridge> bridge;

        Bridge::StatusBarState statusbarState;
        AmbientLightData ambientLightData;
};
