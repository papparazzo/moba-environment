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

#include "bridge.h"
#include <moba-common/ini.h>
#include <atomic>
#include <thread>

class EclipseControl final {
public:
    EclipseControl(BridgePtr bridge, moba::IniPtr ini);

    EclipseControl(const EclipseControl&) = delete;
    EclipseControl& operator=(const EclipseControl&) = delete;

    ~EclipseControl();

    void startEclipse();
    void stopEclipse();

    void mainLightOn();
    void mainLightOff();

    void curtainRunningUp();
    void curtainRunningDown();

private:
    enum class CurtainState {
        STOP         = 0,
        POS_UP       = 1,
        POS_DOWN     = 2,
        RUNNING_UP   = 3,
        RUNNING_DOWN = 4,
    };

    enum class MainLightState {
        ON   = 0,
        OFF  = 1,
        IDLE = 2,
    };

    void curtainControl();
    void mainLightControl();

    BridgePtr bridge;
    moba::IniPtr ini;

    std::thread curtainThread;
    std::thread mainLightThread;

    std::atomic<bool> running{true};
    std::atomic<int> curtainPos;
    std::atomic<CurtainState> curtainState{CurtainState::STOP};
    std::atomic<MainLightState> mainLightState{MainLightState::IDLE};

    bool eclipsed{false};
    bool mainLightWasOn{false};
};

