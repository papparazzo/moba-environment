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

#include "statuscontrol.h"

#include <thread>
#include <syslog.h>
#include "moba/systemmessages.h"

StatusControl::StatusControl(BridgePtr bridge, EndpointPtr endpoint): bridge{bridge}, endpoint{endpoint} {
    switchStateThread  = std::thread{&StatusControl::switchStateControl, this};
    statusBarThread = std::thread{&StatusControl::statusBarControl, this};
}

StatusControl::~StatusControl() {
    running = false;
    switchStateThread.join();
    statusBarThread.join();
}

void StatusControl::setStatusBar(StatusBarState sbstate) {
    statusBarState = sbstate;
    switch(statusBarState) {
        case StatusBarState::ERROR:
            syslog(LOG_INFO, "set statusbar to <ERROR>");
            break;

        case StatusBarState::INIT:
            syslog(LOG_INFO, "set statusbar to <INIT>");
            break;

        case StatusBarState::EMERGENCY_STOP:
            syslog(LOG_INFO, "set statusbar to <EMERGENCY_STOP>");
            break;

        case StatusBarState::STANDBY:
            syslog(LOG_INFO, "set statusbar to <STANDBY>");
            break;

        case StatusBarState::MANUEL:
            syslog(LOG_INFO, "set statusbar to <MANUEL>");
            break;

        case StatusBarState::AUTOMATIC:
            syslog(LOG_INFO, "set statusbar to <AUTOMATIC>");
            break;
    }
}

void StatusControl::switchStateControl() {
    // Taster: 1x lang -> Anlage aus
    //         1x kurz -> Ruhemodus an / aus

    while(running) {

        while(bridge->getDebounced(Bridge::PUSH_BUTTON_STATE)) {
            std::this_thread::sleep_for(std::chrono::milliseconds{50});
        }

        int cntOn = 0;

        while(!bridge->getDebounced(Bridge::PUSH_BUTTON_STATE)) {
            cntOn++;
            std::this_thread::sleep_for(std::chrono::milliseconds{5});
        }

        if(cntOn < 300) { // 300 * 5ms -> 1.5 seconds
            syslog(LOG_INFO, "SHORT_ONCE (< 1.5s pressed) [standby]");
            endpoint->sendMsg(SystemToggleStandbyMode{});
            continue;
        }

        syslog(LOG_INFO, "LONG_ONCE (> 1.5s pressed) [shutdown]");
        endpoint->sendMsg(SystemHardwareShutdown{});
    }
}

void StatusControl::statusBarControl() {
    while(running) {
        StatusBarState sbs = statusBarState;

        switch(sbs) {
            case StatusBarState::ERROR:
            case StatusBarState::INIT:
            case StatusBarState::EMERGENCY_STOP:
                bridge->setHigh(Bridge::STATUS_RED);
                bridge->setLow(Bridge::STATUS_GREEN);
                break;

            case StatusBarState::STANDBY:
            case StatusBarState::MANUEL:
            case StatusBarState::AUTOMATIC:
                bridge->setLow(Bridge::STATUS_RED);
                bridge->setHigh(Bridge::STATUS_GREEN);
                break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        switch(sbs) {
            case StatusBarState::EMERGENCY_STOP:
                bridge->setLow(Bridge::STATUS_RED);
                break;

            case StatusBarState::STANDBY:
                bridge->setLow(Bridge::STATUS_GREEN);
                break;

            default:
                break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(700));
        switch(sbs) {
            case StatusBarState::INIT:
                bridge->setLow(Bridge::STATUS_RED);
                break;

            case StatusBarState::MANUEL:
                bridge->setLow(Bridge::STATUS_GREEN);
                break;

            default:
                break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(750));
    }
    bridge->setLow(Bridge::STATUS_RED);
    bridge->setLow(Bridge::STATUS_GREEN);
}
