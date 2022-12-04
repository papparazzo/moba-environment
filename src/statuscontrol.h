/*
 *  Project:    moba-environment
 *
 *  Copyright (C) 2022 Stefan Paproth <pappi-@gmx.de>
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

#include <atomic>
#include <thread>
#include <memory>

#include "moba/endpoint.h"

#include "bridge.h"

class StatusControl {
public:
   enum class StatusBarState {
        INIT           = 0,   // rot blink
        ERROR          = 1,   // rot
        EMERGENCY_STOP = 2,   // rot blitz

        STANDBY        = 3,   // gruen blitz
        MANUEL         = 4,   // gruen blink
        AUTOMATIC      = 5    // gruen
    };

    StatusControl(BridgePtr bridge, EndpointPtr endpoint);
    virtual ~StatusControl();

    StatusControl(const StatusControl&) = delete;
    StatusControl& operator=(const StatusControl&) = delete;

    void setStatusBar(StatusBarState sbstate);

private:
    void switchStateControl();
    void statusBarControl();

    BridgePtr bridge;
    EndpointPtr endpoint;

    std::thread statusBarThread;
    std::thread switchStateThread;

    std::atomic<bool> running{true};
    std::atomic<StatusBarState> statusBarState{StatusBarState::INIT};
};

using StatusControlPtr = std::shared_ptr<StatusControl>;
