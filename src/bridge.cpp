/*
 * File:   bridge.cpp
 * Author: stefan
 *
 * Created on December 20, 2015, 10:34 PM
 */

#include <wiringPi.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <string>
#include <pthread.h>

#include <moba/log.h>

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

    enum PinOutputMapping {
        FLASH_3      = 29,       // PIN 40
        FLASH_2      = 28,       // PIN 38
        FLASH_1      = 25,       // PIN 37
        STATUS_RED   = 27,       // PIN 36
        STATUS_GREEN = 24,       // PIN 35
        MAIN_LIGHT   = 23,       // PIN 33
        CURTAIN_ON   = 26,       // PIN 32
        CURTAIN_DIR  = 22,       // PIN 31
        SHUTDOWN     = 21,       // PIN 29

        AUX_3        = 4,        // PIN 16
        AUX_2        = 3,        // PIN 15
        AUX_1        = 2,        // PIN 13
    };

    enum PinInputMapping {
        PUSH_BUTTON_STATE = 0,   // PIN 12
        LIGHT_STATE       = 1,  // PIN 11
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

    int auxPins[] = {AUX_1, AUX_2, AUX_3};
    int flashPins[] = {FLASH_1, FLASH_2, FLASH_3};

    const char *thunder[] = {
        "thunder0.mp3",
        "thunder1.mp3",
        "thunder2.mp3",
        "thunder3.mp3",
        "thunder4.mp3",
    };

    Bridge::StatusBarState statusBarState_ = Bridge::SBS_INIT;
    Bridge::SwitchState switchState_;
    CurtainState curtainState_     = CURTAIN_STOP;
    MainLightState mainLightState_ = MAINLIGHT_IDLE;
    AuxState auxPinState_[]        = {AUX_OFF, AUX_OFF, AUX_OFF};
    bool auxPinTrigger_[]          = {false, false, false};
    bool thunderStormState_        = false;
    bool thunderStormTrigger_      = false;

    enum ThreadIdentifier {
        TH_STATUS_BAR = 0,
        TH_CURTAIN,
        TH_MAINLIGHT,
        TH_THUNDERSTORM,
        TH_AUX,
        TH_SWITCH_STATE,
        TH_LAST
    };

    pthread_t       th[TH_LAST];
    pthread_mutex_t mutexs[TH_LAST];

    void *statusBar_(void *) {
        while(true) {
            pthread_mutex_lock(&mutexs[TH_STATUS_BAR]);
            Bridge::StatusBarState sbs = statusBarState_;
            pthread_mutex_unlock(&mutexs[TH_STATUS_BAR]);

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
        return NULL;
    }

    void *curtain_(void *) {
        CurtainState currentCurtainState = CURTAIN_STOP;

        while(true) {
            sleep(1);
            pthread_mutex_lock(&mutexs[TH_CURTAIN]);
            if(curtainState_ == CURTAIN_STOP) {
                pthread_mutex_unlock(&mutexs[TH_CURTAIN]);
                continue;
            }

            if(currentCurtainState == curtainState_) {
                pthread_mutex_unlock(&mutexs[TH_CURTAIN]);
                continue;
            }
            currentCurtainState = curtainState_;
            pthread_mutex_unlock(&mutexs[TH_CURTAIN]);

            int dir = LOW;
            if(currentCurtainState == CURTAIN_UP) {
                dir = HIGH;
            }
            digitalWrite(CURTAIN_ON, HIGH);
            digitalWrite(CURTAIN_DIR, dir);

            for(int i = 0; i < 30; ++i) {
                pthread_mutex_lock(&mutexs[TH_CURTAIN]);
                if(currentCurtainState != curtainState_) {
                    pthread_mutex_unlock(&mutexs[TH_CURTAIN]);
                    break;
                }
                pthread_mutex_unlock(&mutexs[TH_CURTAIN]);
                sleep(1);
            }
            digitalWrite(CURTAIN_ON, LOW);
            digitalWrite(CURTAIN_DIR, LOW);
        }
        return NULL;
    }

    void *mainLight_(void *) {
        while(true) {
            sleep(1);
            pthread_mutex_lock(&mutexs[TH_MAINLIGHT]);
            MainLightState mal = mainLightState_;
            pthread_mutex_unlock(&mutexs[TH_MAINLIGHT]);

            if(mal == MAINLIGHT_IDLE) {
                continue;
            }
            if(digitalRead(LIGHT_STATE) && mal == MAINLIGHT_ON) {
                continue;
            }
            if(!digitalRead(LIGHT_STATE) && mal == MAINLIGHT_OFF) {
                continue;
            }
            digitalWrite(MAIN_LIGHT, HIGH);
            delay(500);
            digitalWrite(MAIN_LIGHT, LOW);
            pthread_mutex_lock(&mutexs[TH_MAINLIGHT]);
            mainLightState_ = MAINLIGHT_IDLE;
            pthread_mutex_unlock(&mutexs[TH_MAINLIGHT]);
        }
        return NULL;
    }

    void *thunderStorm_(void *) {
        while(true) {
            pthread_mutex_lock(&mutexs[TH_THUNDERSTORM]);
            if(!thunderStormState_ && !thunderStormTrigger_) {
                pthread_mutex_unlock(&mutexs[TH_THUNDERSTORM]);
                sleep(1);
                continue;
            }
            thunderStormTrigger_ = false;
            pthread_mutex_unlock(&mutexs[TH_THUNDERSTORM]);
            int f = flashPins[rand() % 3];
            for(int i = 0; i < 33; ++i) {
                digitalWrite(f, HIGH);
                delay(rand() % 15);
                digitalWrite(f, LOW);
                delay(rand() % 15);
            }
            delay(rand() % 800 + 200);
            system(std::string(std::string("mpg123 ../../cpp/Environment/data/") + thunder[rand() % 5]).c_str());
            sleep(rand() % 25);
        }
        return NULL;
    }

    void *aux_(void *) {
        while(true) {
            sleep(1);
            for(int i = 0; i < 3; ++i) {
                pthread_mutex_lock(&mutexs[TH_AUX]);
                AuxState tmpAuxPinState = auxPinState_[i];
                bool tmpAuxPinTrigger = auxPinTrigger_[i];
                pthread_mutex_unlock(&mutexs[TH_AUX]);

                switch(tmpAuxPinState) {
                    case AUX_ON_IDLE:
                        break;

                    case AUX_OFF_IDLE:
                        if(tmpAuxPinTrigger) {
                            digitalWrite(auxPins[i], HIGH);
                            delay(500);
                            digitalWrite(auxPins[i], LOW);
                            pthread_mutex_lock(&mutexs[TH_AUX]);
                            auxPinTrigger_[i] = false;
                            pthread_mutex_unlock(&mutexs[TH_AUX]);
                        }
                        break;

                    case AUX_OFF:
                        digitalWrite(auxPins[i], LOW);
                        pthread_mutex_lock(&mutexs[TH_AUX]);
                        auxPinState_[i] = AUX_OFF_IDLE;
                        pthread_mutex_unlock(&mutexs[TH_AUX]);
                        break;

                    case AUX_ON:
                        digitalWrite(auxPins[i], HIGH);
                        pthread_mutex_lock(&mutexs[TH_AUX]);
                        auxPinState_[i] = AUX_ON_IDLE;
                        pthread_mutex_unlock(&mutexs[TH_AUX]);
                        break;
                }
            }
        }
    }

    void *switchStateChecker_(void *) {
        while(true) {
            pthread_mutex_lock(&mutexs[TH_SWITCH_STATE]);
            if(switchState_ != Bridge::SS_UNSET) {
                pthread_mutex_unlock(&mutexs[TH_SWITCH_STATE]);
                sleep(1);
                continue;
            }
            pthread_mutex_unlock(&mutexs[TH_SWITCH_STATE]);

            int cntOn = 0;
            int cntOff = 0;
            while(digitalRead(PUSH_BUTTON_STATE)) {
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
                pthread_mutex_lock(&mutexs[TH_SWITCH_STATE]);
                switchState_ = Bridge::SS_LONG_ONCE;
                pthread_mutex_unlock(&mutexs[TH_SWITCH_STATE]);
                continue;
            }

            while(!digitalRead(PUSH_BUTTON_STATE)) {
                cntOff++;
                if(cntOff > 500) {
                    break;
                }
                usleep(1000);
            }

            if(cntOff > 500) {
                pthread_mutex_lock(&mutexs[TH_SWITCH_STATE]);
                switchState_ = Bridge::SS_SHORT_ONCE;
                pthread_mutex_unlock(&mutexs[TH_SWITCH_STATE]);
                continue;
            }
            pthread_mutex_lock(&mutexs[TH_SWITCH_STATE]);
            switchState_ = Bridge::SS_SHORT_TWICE;
            pthread_mutex_unlock(&mutexs[TH_SWITCH_STATE]);
        }
    }
}

Bridge::Bridge() {
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

    for(int i = 0; i < TH_LAST; ++i) {
        pthread_mutex_init(&mutexs[i], NULL);
    }

    pthread_create(&th[TH_CURTAIN], NULL, curtain_, NULL);
    pthread_create(&th[TH_STATUS_BAR], NULL, statusBar_, NULL);
    pthread_create(&th[TH_MAINLIGHT], NULL, mainLight_, NULL);
    pthread_create(&th[TH_THUNDERSTORM], NULL, thunderStorm_, NULL);
    pthread_create(&th[TH_AUX], NULL, aux_, NULL);
    pthread_create(&th[TH_SWITCH_STATE], NULL, switchStateChecker_, NULL);

    for(int i = 0; i < TH_LAST; ++i) {
        pthread_detach(th[i]);
    }
}

Bridge::~Bridge() {
    for(int i = 0; i < TH_LAST; ++i) {
        pthread_cancel(th[i]);
    }
    digitalWrite(CURTAIN_DIR,  LOW);
    digitalWrite(CURTAIN_ON,   LOW);
    digitalWrite(MAIN_LIGHT,   LOW);
    digitalWrite(SHUTDOWN,     LOW);
    digitalWrite(AUX_1,        LOW);
    digitalWrite(AUX_2,        LOW);
    digitalWrite(AUX_3,        LOW);
    digitalWrite(FLASH_1,      LOW);
    digitalWrite(FLASH_2,      LOW);
    digitalWrite(FLASH_3,      LOW);
    digitalWrite(STATUS_RED,   LOW);
    digitalWrite(STATUS_GREEN, LOW);
}

void Bridge::selftesting() {
    LOG(tlog::Info) << "selftesting";
    // FIXME
    return;
}

void Bridge::shutdown() {
    LOG(tlog::Info) << "shutdown";
    digitalWrite(SHUTDOWN, HIGH);
    sleep(2);
    digitalWrite(SHUTDOWN, LOW);
    execl("/sbin/shutdown", "shutdown", "-h", "now", NULL);
}

void Bridge::reboot() {
    LOG(tlog::Info) << "reboot";
    execl("/sbin/shutdown", "shutdown", "-r", "now", NULL);
}

void Bridge::auxOn(AuxPin nb) {
    LOG(tlog::Info) << "auxOn <" << nb << ">";
    pthread_mutex_lock(&mutexs[TH_AUX]);
    auxPinState_[nb] = AUX_ON;
    pthread_mutex_unlock(&mutexs[TH_AUX]);
}

void Bridge::auxOff(AuxPin nb) {
    LOG(tlog::Info) << "auxOff <" << nb << ">";
    pthread_mutex_lock(&mutexs[TH_AUX]);
    auxPinState_[nb] = AUX_OFF;
    pthread_mutex_unlock(&mutexs[TH_AUX]);
}

void Bridge::auxTrigger(AuxPin nb) {
    LOG(tlog::Info) << "auxTrigger <" << nb << ">";
    pthread_mutex_lock(&mutexs[TH_AUX]);
    auxPinTrigger_[nb] = true;
    pthread_mutex_unlock(&mutexs[TH_AUX]);
}

void Bridge::curtainUp() {
    LOG(tlog::Info) << "curtainUp";
    pthread_mutex_lock(&mutexs[TH_CURTAIN]);
    curtainState_ = CURTAIN_UP;
    pthread_mutex_unlock(&mutexs[TH_CURTAIN]);
}

void Bridge::curtainDown() {
    LOG(tlog::Info) << "curtainDown";
    pthread_mutex_lock(&mutexs[TH_CURTAIN]);
    curtainState_ = CURTAIN_DOWN;
    pthread_mutex_unlock(&mutexs[TH_CURTAIN]);
}

void Bridge::setStatusBar(Bridge::StatusBarState sbstate) {
    LOG(tlog::Info) << "setStatusBar <" << sbstate << ">";
    pthread_mutex_lock(&mutexs[TH_STATUS_BAR]);
    statusBarState_ = sbstate;
    pthread_mutex_unlock(&mutexs[TH_STATUS_BAR]);
}

void Bridge::mainLightOn() {
    LOG(tlog::Info) << "mainLightOn";
    pthread_mutex_lock(&mutexs[TH_MAINLIGHT]);
    mainLightState_ = MAINLIGHT_ON;
    pthread_mutex_unlock(&mutexs[TH_MAINLIGHT]);
}

void Bridge::mainLightOff() {
    LOG(tlog::Info) << "mainLightOff";
    pthread_mutex_lock(&mutexs[TH_MAINLIGHT]);
    mainLightState_ = MAINLIGHT_OFF;
    pthread_mutex_unlock(&mutexs[TH_MAINLIGHT]);
}

void Bridge::thunderstormOn() {
    LOG(tlog::Info) << "thunderstormOn";
    pthread_mutex_lock(&mutexs[TH_THUNDERSTORM]);
    thunderStormState_ = true;
    pthread_mutex_unlock(&mutexs[TH_THUNDERSTORM]);
}

void Bridge::thunderstormOff() {
    LOG(tlog::Info) << "thunderstormOff";
    pthread_mutex_lock(&mutexs[TH_THUNDERSTORM]);
    thunderStormState_ = false;
    pthread_mutex_unlock(&mutexs[TH_THUNDERSTORM]);
}

void Bridge::thunderstormTrigger() {
    LOG(tlog::Info) << "thunderstormTrigger";
    pthread_mutex_lock(&mutexs[TH_THUNDERSTORM]);
    thunderStormTrigger_ = true;
    pthread_mutex_unlock(&mutexs[TH_THUNDERSTORM]);
}

Bridge::SwitchState Bridge::checkSwitchState() {
    Bridge::SwitchState tmp;
    pthread_mutex_lock(&mutexs[TH_SWITCH_STATE]);
    tmp = switchState_;
    switchState_ = Bridge::SS_UNSET;
    pthread_mutex_unlock(&mutexs[TH_SWITCH_STATE]);
    return tmp;
}
