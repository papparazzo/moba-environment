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

#include "msgloop.h"
#include "moba/registry.h"
#include "moba/timermessages.h"
#include "moba/environmentmessages.h"

#include <moba-common/ipc.h>
#include <thread>
#include <syslog.h>

MessageLoop::MessageLoop(EndpointPtr endpoint, StatusControlPtr status, EclipseControlPtr eclctr, BridgePtr bridge) :
endpoint{endpoint}, status{status}, eclctr{eclctr}, bridge{bridge} {
}

void MessageLoop::run() {

    while(!closing) {
        try {
            endpoint->connect();
            Registry registry;
            registry.registerHandler<SystemHardwareStateChanged>(std::bind(&MessageLoop::setHardwareState, this, std::placeholders::_1));
            registry.registerHandler<ClientShutdown>([this]{shutdown();});
            registry.registerHandler<ClientReset>([this]{reboot();});
            //registry.registerHandler<ClientSelfTesting>([this]{bridge->selftesting();});
            registry.registerHandler<ClientError>(std::bind(&MessageLoop::setError, this, std::placeholders::_1));
            registry.registerHandler<EnvSetAmbience>(std::bind(&MessageLoop::setAmbience, this, std::placeholders::_1));

            endpoint->sendMsg(SystemGetHardwareState{});
            endpoint->sendMsg(TimerGetGlobalTimer{});
/*
            if(bridge->getDebounced(Bridge::LIGHT_STATE)) {
                endpoint->sendMsg(EnvSetAmbience{ToggleState::UNSET, ToggleState::ON});
            } else {
                endpoint->sendMsg(EnvSetAmbience{ToggleState::UNSET, ToggleState::OFF});
            }
*/
            while(!closing) {
                registry.handleMsg(endpoint->waitForNewMsg());
            }
        } catch(const std::exception &e) {
            syslog(LOG_CRIT, "exception occured! <%s> started", e.what());
            status->setStatusBar(StatusControl::StatusBarState::ERROR);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{500});
    }
}

void MessageLoop::setHardwareState(const SystemHardwareStateChanged &data) {

    switch(data.hardwareState) {
        case SystemHardwareStateChanged::HardwareState::ERROR:
            syslog(LOG_INFO, "setHardwareState <ERROR>");
            status->setStatusBar(StatusControl::StatusBarState::ERROR);
            break;

        case SystemHardwareStateChanged::HardwareState::STANDBY:
            syslog(LOG_INFO, "setHardwareState <STANDBY>");
            status->setStatusBar(StatusControl::StatusBarState::STANDBY);
            break;

        case SystemHardwareStateChanged::HardwareState::EMERGENCY_STOP:
            syslog(LOG_INFO, "setHardwareState <EMERGENCY_STOP>");
            status->setStatusBar(StatusControl::StatusBarState::EMERGENCY_STOP);
            break;

        case SystemHardwareStateChanged::HardwareState::MANUEL:
            syslog(LOG_INFO, "setHardwareState <MANUEL>");
            status->setStatusBar(StatusControl::StatusBarState::MANUEL);
            eclctr->stopEclipse();
//            setAmbientLight();
            break;

        case SystemHardwareStateChanged::HardwareState::AUTOMATIC:
            syslog(LOG_INFO, "setHardwareState <AUTOMATIC>");
            status->setStatusBar(StatusControl::StatusBarState::AUTOMATIC);
            eclctr->startEclipse();
//            setAmbientLight();
            break;
    }
}

void MessageLoop::setError(const ClientError &data) {
    syslog(LOG_INFO, "ErrorId <%s> %s", data.errorId.c_str(), data.additionalMsg.c_str());
}

void MessageLoop::setAmbience(const EnvSetAmbience &data) {
    if(automatic) {
        syslog(LOG_WARNING, "setAmbience: automatic is on!");
        return;
    }

    if(data.curtainUp == ToggleState::ON) {
//        eclctr->curtainUp();
    } else if(data.curtainUp == ToggleState::OFF) {
//        eclctr->curtainDown();
    }

    if(data.mainLightOn == ToggleState::ON) {
        eclctr->mainLightOn();
    } else if(data.mainLightOn == ToggleState::OFF) {
        eclctr->mainLightOff();
    }
}

void MessageLoop::shutdown() {
    syslog(LOG_INFO, "shutdown");
    execl("/usr/local/bin/moba-shutdown", "moba-shutdown", (char *)NULL);
}

void MessageLoop::reboot() {
    syslog(LOG_INFO, "reboot");
    execl("/usr/local/bin/moba-shutdown", "moba-shutdown", "-r", (char *)NULL);
}






















/*
void MessageLoop::setAmbientLight(const Envi) {
    std::cerr << "setAmbientLight" << std::endl;

    AmbientLightData ambientLightData;

    ambientLightData.blue = boost::dynamic_pointer_cast<moba::JsonNumber<long int> >(o->at("blue"))->getVal();
    std::cerr << "blue <" << ambientLightData.blue << ">" << std::endl;
    ambientLightData.green = boost::dynamic_pointer_cast<moba::JsonNumber<long int> >(o->at("green"))->getVal();
    std::cerr << "blue <" << ambientLightData.green << ">" << std::endl;
    ambientLightData.red = boost::dynamic_pointer_cast<moba::JsonNumber<long int> >(o->at("red"))->getVal();
    std::cerr << "red <" << ambientLightData.red << ">" << std::endl;
    ambientLightData.white = boost::dynamic_pointer_cast<moba::JsonNumber<long int> >(o->at("white"))->getVal();
    std::cerr << "white <" << ambientLightData.white << ">" << std::endl;
    if(!automatic) {
        setAmbientLight();
    }

}
 */

/*
void MessageLoop::setAmbientLight() {
    bridge->setAmbientLight(
        ambientLightData.blue,
        ambientLightData.green,
        ambientLightData.red,
        ambientLightData.white,
        20
    );
}
 */






    /*
    while(true) {
        switch(msg->getMsgType()) {
            case moba::Message::MT_GLOBAL_TIMER_EVENT:
                globalTimerEvent(msg->getData());
                break;

            case moba::Message::MT_SET_ENVIRONMENT:
                setEnvironment(msg->getData());
                break;


            case moba::Message::MT_SET_AMBIENT_LIGHT:
                setAmbientLight(msg->getData());
                break;

            }
        }
    }
}

void MessageLoop::setEnvironment(moba::JsonItemPtr ptr) {
    moba::JsonObjectPtr o = boost::dynamic_pointer_cast<moba::JsonObject>(ptr);
    moba::JsonSwitch::Switch a[3];
    moba::JsonSwitch::Switch thst = moba::castToSwitch(o->at("thunderStorm"));
    moba::JsonSwitch::Switch wind = moba::castToSwitch(o->at("wind"));
    moba::JsonSwitch::Switch rain = moba::castToSwitch(o->at("rain"));
    moba::JsonSwitch::Switch enso = moba::castToSwitch(o->at("environmentSound"));
    a[0] = moba::castToSwitch(o->at("aux01"));
    a[1] = moba::castToSwitch(o->at("aux02"));
    a[2] = moba::castToSwitch(o->at("aux03"));

    LOG(moba::NOTICE) << "thunderStorm     <" << thst << ">" << std::endl;
    LOG(moba::NOTICE) << "wind             <" << wind << ">" << std::endl;
    LOG(moba::NOTICE) << "environmentSound <" << enso << ">" << std::endl;
    LOG(moba::NOTICE) << "rain             <" << rain << ">" << std::endl;
    LOG(moba::NOTICE) << "aux01            <" << a[0] << ">" << std::endl;
    LOG(moba::NOTICE) << "aux02            <" << a[1] << ">" << std::endl;
    LOG(moba::NOTICE) << "aux03            <" << a[2] << ">" << std::endl;

    switch(thst) {
        case moba::JsonSwitch::AUTO:
        case moba::JsonSwitch::ON:
            bridge->thunderstormOn();
            break;

        case moba::JsonSwitch::OFF:
            bridge->thunderstormOff();
            break;

        case moba::JsonSwitch::TRIGGER:
            bridge->thunderstormTrigger();
            break;
    }

    for(int i = 0; i < 3; ++i) {
        switch(a[i]) {
            case moba::JsonSwitch::AUTO:
            case moba::JsonSwitch::ON:
                bridge->auxOn((Bridge::AuxPin)i);
                break;

            case moba::JsonSwitch::OFF:
                bridge->auxOff((Bridge::AuxPin)i);
                break;

            case moba::JsonSwitch::TRIGGER:
                bridge->auxTrigger((Bridge::AuxPin)i);
                break;
        }
    }
}


void MessageLoop::globalTimerEvent(moba::JsonItemPtr ptr) {
    moba::JsonObjectPtr o = boost::dynamic_pointer_cast<moba::JsonObject>(ptr);
    o->at("curModelTime");
    o->at("multiplicator");

    //boost::shared_ptr<moba::JsonNumber<long int> > i =
    //boost::dynamic_pointer_cast<moba::JsonNumber<long int> >(o->at("errorId"));
    //moba::JsonStringPtr s = boost::dynamic_pointer_cast<moba::JsonString>(o->at("additonalMsg"));

    int mz;
    int dur;
    //Sonnenaufgang   04:00; dauer 2h
    if(4 * 60 + 30 == mz) {
        //blue, green, red, white, duration
        //bridge->setAmbientLight(500, 250, 700, dur);
    }

    //Sonnenuntergang 22:00; dauer 2h
    if(21 * 60 + 30 == mz) {
        //bridge->setAmbientLight(500, 250, 700, dur);
    }
}


 * */
