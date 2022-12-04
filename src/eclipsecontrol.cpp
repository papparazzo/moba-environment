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

#include "eclipsecontrol.h"

#include <syslog.h>
#include <thread>

EclipseControl::EclipseControl(BridgePtr bridge, moba::IniPtr ini): bridge{bridge}, ini{ini} {
    curtainPos = ini->getInt("curtain", "pos", 0);
    curtainThread = std::thread{&EclipseControl::curtainControl, this};
    mainLightThread = std::thread{&EclipseControl::mainLightControl, this};
}

EclipseControl::~EclipseControl() {
    ini->setInt("curtain", "pos", curtainPos);
    running = false;
    curtainThread.join();
    mainLightThread.join();
}

void EclipseControl::startEclipse() {
    if(eclipsed) {
        syslog(LOG_WARNING, "startEclipse: allready eclipsed!");
        return;
    }
    eclipsed = true;
    mainLightWasOn = !getDebounced(Bridge::LIGHT_STATE);
    if(!mainLightWasOn) {
        mainLightOff();
    }
    curtainState = CurtainState::POS_DOWN;
}

void EclipseControl::stopEclipse() {
    if(!eclipsed) {
        syslog(LOG_WARNING, "stopEclipse: allready stopped!");
        return;
    }
    eclipsed = false;
    if(mainLightWasOn) {
        mainLightOn();
    }
    curtainState = CurtainState::POS_UP;
}

void EclipseControl::mainLightOn() {
    syslog(LOG_INFO, "mainLightOn");
    mainLightState = MainLightState::ON;
}

void EclipseControl::mainLightOff() {
    syslog(LOG_INFO, "mainLightOff");
    mainLightState = MainLightState::OFF;
}

void EclipseControl::curtainRunningUp() {
    syslog(LOG_INFO, "curtainRunningUp");
    if(eclipsed) {
        syslog(LOG_WARNING, "curtainRunningUp: eclipse!");
        return;
    }

    if(curtainState == CurtainState::STOP) {
        curtainState = CurtainState::RUNNING_UP;
    } else {
        curtainState = CurtainState::STOP;
    }
}

void EclipseControl::curtainRunningDown() {
    syslog(LOG_INFO, "curtainRunningDown");
    if(eclipsed) {
        syslog(LOG_WARNING, "curtainRunningDown: eclipse!");
        return;
    }

    if(curtainState == CurtainState::STOP) {
        curtainState = CurtainState::RUNNING_DOWN;
    } else {
        curtainState = CurtainState::STOP;
    }
}

void EclipseControl::curtainControl() {

    while(running) {
        std::this_thread::sleep_for(std::chrono::milliseconds{500});

        int x;

        switch(curtainState) {
            case CurtainState::STOP:
                continue;

            case CurtainState::POS_UP:
                x = 120 - curtainPos;
                break;

            case CurtainState::RUNNING_UP:
                x = curtainPos;
                break;

            case CurtainState::POS_DOWN:
                x = 130 - curtainPos;
                bridge->setHight(Bridge::CURTAIN_DIR);
                break;

            case CurtainState::RUNNING_DOWN:
                x = curtainPos
                bridge->setHight(Bridge::CURTAIN_DIR);
                break;
        }

        bridge->setHight(Bridge::CURTAIN_ON);

        do {
            if(curtainState == CurtainState::STOP) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds{250});
        } while(true);






        for(int i = 0; i < 120; ++i) {
        }

    auto currentCurtainState = ;


          /*
        STOP         = 0,
        POS_UP       = 1,
        POS_DOWN     = 2,
        RUNNING_UP   = 3,
        RUNNING_DOWN = 4,
     */





        bridge->setLow(Bridge::CURTAIN_ON);
        bridge->setLow(Bridge::CURTAIN_DIR);
    }
}

void EclipseControl::mainLightControl() {
    while(running) {
        std::this_thread::sleep_for(std::chrono::milliseconds{500});
        MainLightState mal = mainLightState;

        if(mal == MainLightState::IDLE) {
            continue;
        }
        if(!bridge->getDebounced(Bridge::LIGHT_STATE) && mal == MainLightState::ON) {
            continue;
        }
        if(bridge->getDebounced(Bridge::LIGHT_STATE) && mal == MainLightState::OFF) {
            continue;
        }
        bridge->setHight(Bridge::MAIN_LIGHT);
        std::this_thread::sleep_for(std::chrono::milliseconds{500});
        bridge->setLow(Bridge::MAIN_LIGHT);
        mainLightState = MainLightState::IDLE;
    }
}
