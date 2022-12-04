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

#include <memory>
#include <mutex>

class Bridge {
public:

    enum PinInputMapping {
        LIGHT_STATE       = 29,  // PIN 40
        PUSH_BUTTON_STATE = 28,  // PIN 38
    };

    enum PinOutputMapping {
        STATUS_GREEN = 24,       // PIN 35
        STATUS_RED   = 27,       // PIN 36

        SHUTDOWN     = 23,       // PIN 33
        MAIN_LIGHT   = 26,       // PIN 32

        CURTAIN_DIR  = 22,       // PIN 31
        CURTAIN_ON   = 21,       // PIN 29

        AUX_1        = 5,        // PIN 18
        AUX_2        = 4,        // PIN 16
        AUX_3        = 3,        // PIN 15

        FLASH_1      = 2,        // PIN 13
        FLASH_2      = 1,        // PIN 12
        FLASH_3      = 0,        // PIN 11
    };

    Bridge();
    virtual ~Bridge();

    Bridge(const Bridge&) = delete;
    Bridge& operator=(const Bridge&) = delete;

    void setHight(PinOutputMapping pin);
    void setLow(PinOutputMapping pin);
    bool getDebounced(PinInputMapping pin);

private:
    std::mutex m;
};

using BridgePtr = std::shared_ptr<Bridge>;
