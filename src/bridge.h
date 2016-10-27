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

class Bridge {
    public:
        enum StatusBarState {
            SBS_ERROR          = 0,   // rot blitz
            SBS_INIT           = 1,   // rot
            SBS_POWER_OFF      = 2,   // rot blink

            SBS_STANDBY        = 3,   // grün blitz
            SBS_READY          = 4,   // grün
            SBS_AUTOMATIC      = 5    // grün blink
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

        Bridge();
        virtual ~Bridge();

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
        void setAmbientLightRed(int ratio, int ticks = 200);
        void setAmbientLightBlue(int ratio, int ticks = 200);
        void setAmbientLightWhite(int ratio, int ticks = 200);

        SwitchState checkSwitchState();

    private:
        Bridge(const Bridge& orig);
};

