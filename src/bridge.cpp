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

#include <wiringPi.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <string>
#include <sstream>
#include <pthread.h>

#include <moba/log.h>
#include <moba/atomic.h>

#include "bridge.h"

/*
 +-----+-----+---------+------+---+---Pi 2---+---+------+---------+-----+-----+
 | BCM | wPi |   Name  | Mode | V | Physical | V | Mode | Name    | wPi | BCM |
 +-----+-----+---------+------+---+----++----+---+------+---------+-----+-----+
 |     |     |    3.3v |      |   |  1 || 2  |   |      | 5v      |     |     |
 |   2 |   8 |   SDA.1 |   IN | 1 |  3 || 4  |   |      | 5V      |     |     |
 |   3 |   9 |   SCL.1 |   IN | 1 |  5 || 6  |   |      | 0v      |     |     |
 |   4 |   7 | GPIO. 7 |   IN | 1 |  7 || 8  | 1 | ALT0 | TxD     | 15  | 14  |
 |     |     |      0v |      |   |  9 || 10 | 1 | ALT0 | RxD     | 16  | 15  |
 |  17 |   0 | GPIO. 0 |   IN | 0 | 11 || 12 | 0 | IN   | GPIO. 1 | 1   | 18  |
 |  27 |   2 | GPIO. 2 |   IN | 0 | 13 || 14 |   |      | 0v      |     |     |
 |  22 |   3 | GPIO. 3 |   IN | 0 | 15 || 16 | 0 | IN   | GPIO. 4 | 4   | 23  |
 |     |     |    3.3v |      |   | 17 || 18 | 0 | IN   | GPIO. 5 | 5   | 24  |
 |  10 |  12 |    MOSI |   IN | 0 | 19 || 20 |   |      | 0v      |     |     |
 |   9 |  13 |    MISO |   IN | 0 | 21 || 22 | 0 | IN   | GPIO. 6 | 6   | 25  |
 |  11 |  14 |    SCLK |   IN | 0 | 23 || 24 | 1 | IN   | CE0     | 10  | 8   |
 |     |     |      0v |      |   | 25 || 26 | 1 | IN   | CE1     | 11  | 7   |
 |   0 |  30 |   SDA.0 |   IN | 1 | 27 || 28 | 1 | IN   | SCL.0   | 31  | 1   |
 |   5 |  21 | GPIO.21 |   IN | 1 | 29 || 30 |   |      | 0v      |     |     |
 |   6 |  22 | GPIO.22 |   IN | 1 | 31 || 32 | 1 | IN   | GPIO.26 | 26  | 12  |
 |  13 |  23 | GPIO.23 |   IN | 1 | 33 || 34 |   |      | 0v      |     |     |
 |  19 |  24 | GPIO.24 |   IN | 1 | 35 || 36 | 1 | IN   | GPIO.27 | 27  | 16  |
 |  26 |  25 | GPIO.25 |   IN | 1 | 37 || 38 | 1 | IN   | GPIO.28 | 28  | 20  |
 |     |     |      0v |      |   | 39 || 40 | 1 | IN   | GPIO.29 | 29  | 21  |
 +-----+-----+---------+------+---+----++----+---+------+---------+-----+-----+
 | BCM | wPi |   Name  | Mode | V | Physical | V | Mode | Name    | wPi | BCM |
 +-----+-----+---------+------+---+---Pi 2---+---+------+---------+-----+-----+
 */

namespace {

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

    enum CurtainState {
        CURTAIN_UP   = 0,
        CURTAIN_DOWN = 1,
        CURTAIN_STOP = 2,
    };

    enum MainLightState {
        MAINLIGHT_ON   = 0,
        MAINLIGHT_OFF  = 1,
        MAINLIGHT_IDLE = 2,
    };

    enum AuxState {
        AUX_ON       = 0,
        AUX_OFF      = 1,
        AUX_ON_IDLE  = 2,
        AUX_OFF_IDLE = 3,
    };

    int auxPins[]   = {AUX_1, AUX_2, AUX_3};
    int flashPins[] = {FLASH_1, FLASH_2, FLASH_3};

    const char *thunder[] = {
        "thunder0.mp3",
        "thunder1.mp3",
        "thunder2.mp3",
        "thunder3.mp3",
        "thunder4.mp3",
    };

    moba::Atomic<Bridge::StatusBarState> statusBarState_ = Bridge::SBS_INIT;
    moba::Atomic<Bridge::SwitchState> switchState_       = Bridge::SS_UNSET;
    moba::Atomic<CurtainState> curtainState_             = CURTAIN_STOP;
    moba::Atomic<MainLightState> mainLightState_         = MAINLIGHT_IDLE;
    moba::Atomic<AuxState> auxPinState_[]                = {AUX_OFF, AUX_OFF, AUX_OFF};
    moba::Atomic<bool> auxPinTrigger_[]                  = {false, false, false};
    moba::Atomic<bool> thunderStormState_                = false;
    moba::Atomic<bool> thunderStormTrigger_              = false;
    moba::Atomic<bool> selftestRunning_                  = false;

    enum ThreadIdentifier {
        TH_STATUS_BAR = 0,
        TH_CURTAIN,
        TH_MAINLIGHT,
        TH_THUNDERSTORM,
        TH_AUX,
        TH_SWITCH_STATE,
        TH_SELF_TESTING,
        TH_LAST
    };

    pthread_t th[TH_LAST];

    moba::Atomic<bool> running = true;

    void *statusBar_(void *) {
        while(running) {
            if(selftestRunning_) {
                delay(500);
                continue;
            }

            Bridge::StatusBarState sbs = statusBarState_;

            switch(sbs) {
                case Bridge::SBS_ERROR:
                case Bridge::SBS_INIT:
                case Bridge::SBS_POWER_OFF:
                    digitalWrite(STATUS_RED, HIGH);
                    digitalWrite(STATUS_GREEN, LOW);
                    break;

                case Bridge::SBS_STANDBY:
                case Bridge::SBS_READY:
                case Bridge::SBS_AUTOMATIC:
                    digitalWrite(STATUS_GREEN, HIGH);
                    digitalWrite(STATUS_RED, LOW);
                    break;
            }
            delay(25);
            switch(sbs) {
                case Bridge::SBS_ERROR:
                    digitalWrite(STATUS_RED, LOW);
                    break;

                case Bridge::SBS_STANDBY:
                    digitalWrite(STATUS_GREEN, LOW);
                    break;

                default:
                    break;
            }
            delay(700);
            switch(sbs) {
                case Bridge::SBS_POWER_OFF:
                    digitalWrite(STATUS_RED, LOW);
                    break;

                case Bridge::SBS_AUTOMATIC:
                    digitalWrite(STATUS_GREEN, LOW);
                    break;

                default:
                    break;
            }
            delay(750);
        }
        digitalWrite(STATUS_RED, LOW);
        digitalWrite(STATUS_GREEN, LOW);
        return NULL;
    }

    void *curtain_(void *) {
        CurtainState currentCurtainState = CURTAIN_STOP;

        while(running) {
            if(selftestRunning_) {
                delay(500);
                continue;
            }

            sleep(1);
            if(curtainState_ == CURTAIN_STOP) {
                continue;
            }

            if(currentCurtainState == curtainState_) {
                continue;
            }
            currentCurtainState = curtainState_;

            int dir = LOW;
            if(currentCurtainState == CURTAIN_UP) {
                dir = HIGH;
            }
            digitalWrite(CURTAIN_ON, HIGH);
            digitalWrite(CURTAIN_DIR, dir);

            for(int i = 0; i < 30; ++i) {
                if(currentCurtainState != curtainState_) {
                    break;
                }
                sleep(1);
            }
            digitalWrite(CURTAIN_ON, LOW);
            digitalWrite(CURTAIN_DIR, LOW);
        }
        return NULL;
    }

    void *mainLight_(void *) {
        while(running) {
            if(selftestRunning_) {
                delay(500);
                continue;
            }

            sleep(1);
            MainLightState mal = mainLightState_;

            if(mal == MAINLIGHT_IDLE) {
                continue;
            }
            if(!digitalRead(LIGHT_STATE) && mal == MAINLIGHT_ON) {
                continue;
            }
            if(digitalRead(LIGHT_STATE) && mal == MAINLIGHT_OFF) {
                continue;
            }
            digitalWrite(MAIN_LIGHT, HIGH);
            delay(500);
            digitalWrite(MAIN_LIGHT, LOW);
            mainLightState_ = MAINLIGHT_IDLE;
        }
        return NULL;
    }

    void *thunderStorm_(void *) {
        while(running) {
            if(selftestRunning_) {
                delay(500);
                continue;
            }
            if(!thunderStormState_ && !thunderStormTrigger_) {
                sleep(1);
                continue;
            }
            thunderStormTrigger_ = false;

            int f = flashPins[rand() % 3];
            digitalWrite(f, HIGH);
            delay(500);
            digitalWrite(f, LOW);
            delay(rand() % 800 + 200);
            // FIXME no hard-coded path
            system(std::string(std::string("mpg123 ../../cpp/Environment/data/") + thunder[rand() % 5]).c_str());
            sleep(rand() % 25);
        }
        return NULL;
    }

    void *aux_(void *) {
        while(running) {
            if(selftestRunning_) {
                delay(500);
                continue;
            }
            sleep(1);
            for(int i = 0; i < 3; ++i) {
                AuxState tmpAuxPinState = auxPinState_[i];
                if(!running) {
                    tmpAuxPinState = AUX_OFF;
                }
                bool tmpAuxPinTrigger = auxPinTrigger_[i];

                switch(tmpAuxPinState) {
                    case AUX_ON_IDLE:
                        break;

                    case AUX_OFF_IDLE:
                        if(tmpAuxPinTrigger) {
                            digitalWrite(auxPins[i], HIGH);
                            delay(500);
                            digitalWrite(auxPins[i], LOW);
                            auxPinTrigger_[i] = false;
                        }
                        break;

                    case AUX_OFF:
                        digitalWrite(auxPins[i], LOW);
                        auxPinState_[i] = AUX_OFF_IDLE;
                        break;

                    case AUX_ON:
                        digitalWrite(auxPins[i], HIGH);
                        auxPinState_[i] = AUX_ON_IDLE;
                        break;
                }
            }
        }
    }

    void *switchStateChecker_(void *) {
        while(running) {
            if(switchState_ != Bridge::SS_UNSET) {
                sleep(1);
                continue;
            }

            int cntOn = 0;
            int cntOff = 0;
            while(!digitalRead(PUSH_BUTTON_STATE)) {
                cntOn++;
                if(cntOn > 2000) {
                    break;
                }
                usleep(1000);
            }

            if(!cntOn) {
                continue;
            }

            if(cntOn > 2000) {
                switchState_ = Bridge::SS_LONG_ONCE;
                continue;
            }

            while(digitalRead(PUSH_BUTTON_STATE)) {
                cntOff++;
                if(cntOff > 500) {
                    break;
                }
                usleep(1000);
            }

            if(cntOff > 500) {
                switchState_ = Bridge::SS_SHORT_ONCE;
                continue;
            }
            switchState_ = Bridge::SS_SHORT_TWICE;
        }
    }

    void *selftesting_(void *) {
        while(running) {
            if(!selftestRunning_) {
                delay(500);
                continue;
            }
            sleep(1);
            digitalWrite(STATUS_RED,   HIGH);
            digitalWrite(STATUS_GREEN, HIGH);

            for(int i = 0; i < 3; ++i) {
                digitalWrite(FLASH_1, HIGH);
                digitalWrite(AUX_1,   HIGH);
                delay(500);
                digitalWrite(FLASH_1, LOW);
                digitalWrite(AUX_1,   LOW);
                delay(500);
            }
            sleep(3);
            for(int i = 0; i < 3; ++i) {
                digitalWrite(FLASH_2, HIGH);
                digitalWrite(AUX_2,   HIGH);
                delay(500);
                digitalWrite(FLASH_2, LOW);
                digitalWrite(AUX_2,   LOW);
                delay(500);
            }
            sleep(3);
            for(int i = 0; i < 3; ++i) {
                digitalWrite(FLASH_3, HIGH);
                digitalWrite(AUX_3,   HIGH);
                delay(500);
                digitalWrite(FLASH_3, LOW);
                digitalWrite(AUX_3,   LOW);
                delay(500);
            }
            sleep(3);

            digitalWrite(MAIN_LIGHT,   HIGH);
            delay(500);
            digitalWrite(MAIN_LIGHT,   LOW);
            delay(1500);
            digitalWrite(MAIN_LIGHT,   HIGH);
            delay(500);
            digitalWrite(MAIN_LIGHT,   LOW);

            for(int i = 0; i < 2; ++i) {
                if(i) {
                    digitalWrite(CURTAIN_DIR, HIGH);
                } else {
                    digitalWrite(CURTAIN_DIR, LOW);
                }
                digitalWrite(CURTAIN_ON, HIGH);
                sleep(3);
                digitalWrite(CURTAIN_ON, LOW);
                digitalWrite(CURTAIN_DIR, LOW);

            }
            digitalWrite(STATUS_RED,   LOW);
            digitalWrite(STATUS_GREEN, LOW);
            selftestRunning_ = false;
        }
    }
}

Bridge::Bridge( boost::shared_ptr<moba::IPC> ipc) : ipc(ipc) {
    srand(time(NULL));
    wiringPiSetup();
    pinMode(CURTAIN_DIR,       OUTPUT);
    pinMode(CURTAIN_ON,        OUTPUT);
    pinMode(MAIN_LIGHT,        OUTPUT);
    pinMode(SHUTDOWN,          OUTPUT);
    pinMode(AUX_1,             OUTPUT);
    pinMode(AUX_2,             OUTPUT);
    pinMode(AUX_3,             OUTPUT);
    pinMode(FLASH_1,           OUTPUT);
    pinMode(FLASH_2,           OUTPUT);
    pinMode(FLASH_3,           OUTPUT);
    pinMode(STATUS_RED,        OUTPUT);
    pinMode(STATUS_GREEN,      OUTPUT);

    pinMode(LIGHT_STATE,       INPUT);
    pinMode(PUSH_BUTTON_STATE, INPUT);

    pthread_create(&th[TH_CURTAIN     ], NULL, curtain_,            NULL);
    pthread_create(&th[TH_STATUS_BAR  ], NULL, statusBar_,          NULL);
    pthread_create(&th[TH_MAINLIGHT   ], NULL, mainLight_,          NULL);
    pthread_create(&th[TH_THUNDERSTORM], NULL, thunderStorm_,       NULL);
    pthread_create(&th[TH_AUX         ], NULL, aux_,                NULL);
    pthread_create(&th[TH_SWITCH_STATE], NULL, switchStateChecker_, NULL);
    pthread_create(&th[TH_SELF_TESTING], NULL, selftesting_,        NULL);

    for(int i = 0; i < TH_LAST; ++i) {
        pthread_detach(th[i]);
    }
}

Bridge::~Bridge() {
/*
    for(int i = 0; i < TH_LAST; ++i) {
        pthread_cancel(th[i]);
    }
 */
}

void Bridge::selftesting() {
    LOG(moba::INFO) << "selftesting" << std::endl;
    ipc->send("", moba::IPC::CMD_TEST);
    selftestRunning_ = true;
}

void Bridge::shutdown() {
    LOG(moba::INFO) << "shutdown" << std::endl;
    digitalWrite(SHUTDOWN, HIGH);
    sleep(2);
    digitalWrite(SHUTDOWN, LOW);
    execl("/sbin/shutdown", "shutdown", "-h", "now", NULL);
}

void Bridge::reboot() {
    LOG(moba::INFO) << "reboot" << std::endl;
    execl("/sbin/shutdown", "shutdown", "-r", "now", NULL);
}

void Bridge::auxOn(AuxPin nb) {
    LOG(moba::INFO) << "auxOn <" << nb << ">" << std::endl;
    auxPinState_[nb] = AUX_ON;
}

void Bridge::auxOff(AuxPin nb) {
    LOG(moba::INFO) << "auxOff <" << nb << ">" << std::endl;
    auxPinState_[nb] = AUX_OFF;
}

void Bridge::auxTrigger(AuxPin nb) {
    LOG(moba::INFO) << "auxTrigger <" << nb << ">" << std::endl;
    auxPinTrigger_[nb] = true;
}

void Bridge::curtainUp() {
    LOG(moba::INFO) << "curtainUp" << std::endl;
    curtainState_ = CURTAIN_UP;
}

void Bridge::curtainDown() {
    LOG(moba::INFO) << "curtainDown" << std::endl;
    curtainState_ = CURTAIN_DOWN;
}

void Bridge::setStatusBar(Bridge::StatusBarState sbstate) {
    LOG(moba::INFO) << "setStatusBar <" << sbstate << ">" << std::endl;
    statusBarState_ = sbstate;
}

void Bridge::mainLightOn() {
    LOG(moba::INFO) << "mainLightOn" << std::endl;
    mainLightState_ = MAINLIGHT_ON;
}

void Bridge::mainLightOff() {
    LOG(moba::INFO) << "mainLightOff" << std::endl;
    mainLightState_ = MAINLIGHT_OFF;
}

void Bridge::thunderstormOn() {
    LOG(moba::INFO) << "thunderstormOn" << std::endl;
    // FIXME ipc->send("", moba::IPC::CMD_EMERGENCY_STOP);
    thunderStormState_ = true;
}

void Bridge::thunderstormOff() {
    LOG(moba::INFO) << "thunderstormOff" << std::endl;
    // FIXME ipc->send("", moba::IPC::CMD_EMERGENCY_STOP);
    thunderStormState_ = false;
}

void Bridge::thunderstormTrigger() {
    LOG(moba::INFO) << "thunderstormTrigger" << std::endl;
    thunderStormTrigger_ = true;
}

Bridge::SwitchState Bridge::checkSwitchState() {
    Bridge::SwitchState tmp = switchState_;
    switchState_ = Bridge::SS_UNSET;
    return tmp;
}

void Bridge::setEmergencyStop() {
    ipc->send("", moba::IPC::CMD_EMERGENCY_STOP);
}

void Bridge::setEmergencyStopClearing() {
    ipc->send("", moba::IPC::CMD_EMERGENCY_RELEASE);
}

void Bridge::setAmbientLight(int blue, int green, int red, int white, int duration) {
    LOG(moba::INFO) << "setAmbientLight <" << blue << "><" << green << "><" << red << "><" << white << "><" << duration << ">" << std::endl;
    std::stringstream ss;
    ss << blue << ";" << green << ";" << red << ";" << white << ";" << duration;
    ipc->send(ss.str(), moba::IPC::CMD_RUN);
}
