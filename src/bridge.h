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

#include <memory>
#include <moba-common/ipc.h>

class Bridge {
    public:
        enum StatusBarState {
            SBS_ERROR          = 0,   // rot blitz
            SBS_INIT           = 1,   // rot
            SBS_POWER_OFF      = 2,   // rot blink

            SBS_STANDBY        = 3,   // gruen blitz
            SBS_READY          = 4,   // gruen
            SBS_AUTOMATIC      = 5    // gruen blink
        };

        enum AuxPin {
            AUX1,
            AUX2,
            AUX3
        };

        enum SwitchState {
            SS_UNSET,
            SS_SHORT_ONCE,
            SS_SHORT_TWICE,
            SS_LONG_ONCE
        };

        Bridge(std::shared_ptr<moba::common::IPC> ipc);
        virtual ~Bridge();

        Bridge(const Bridge&) = delete;
        Bridge& operator=(const Bridge&) = delete;

        void curtainUp();
        void curtainDown();
        void mainLightOn();
        void mainLightOff();

        void shutdown();
        void reboot();

        void auxOn(AuxPin nb);
        void auxOff(AuxPin nb);
        void auxTrigger(AuxPin nb);

        void thunderstormOn();
        void thunderstormOff();
        void thunderstormTrigger();

        void setStatusBar(StatusBarState sbstate);

        void selftesting();
        void setAmbientLight(int blue, int green, int red, int white, int ratio);

        void setEmergencyStop();
        void setEmergencyStopClearing();

        SwitchState checkSwitchState();

    protected:
        std::shared_ptr<moba::common::IPC> ipc;
        std::string getStatusbarClearText(StatusBarState statusbar);
};

using BridgePtr = std::shared_ptr<Bridge>;
