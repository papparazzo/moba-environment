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

#include "msgloop.h"
#include "moba/registry.h"
#include "moba/timermessages.h"

#include <moba-common/ipc.h>
#include <thread>

MessageLoop::MessageLoop(BridgePtr bridge, EndpointPtr endpoint) :
closing{false}, bridge{bridge}, endpoint{endpoint} {
    bridge->setStatusBar(Bridge::StatusBarState::INIT);
}

void MessageLoop::run() {

    while(!closing) {
        try {
            endpoint->connect();
            Registry registry;
            registry.registerHandler<SystemHardwareStateChanged>(std::bind(&MessageLoop::setHardwareState, this, std::placeholders::_1));
            registry.registerHandler<ClientShutdown>([this]{bridge->shutdown();});
            registry.registerHandler<ClientReset>([this]{bridge->reboot();});
            registry.registerHandler<ClientSelfTesting>([this]{bridge->selftesting();});
/*
            registry.registerHandler<InterfaceSetBrakeVector>(std::bind(&JsonReader::setBrakeVector, this, std::placeholders::_1));
            registry.registerHandler<InterfaceSetLocoDirection>([this](const InterfaceSetLocoDirection &d){cs2writer->send(setLocDirection(d.localId, static_cast<std::uint8_t>(d.direction)));});
            registry.registerHandler<InterfaceSetLocoSpeed>([this](const InterfaceSetLocoSpeed &d){cs2writer->send(setLocSpeed(d.localId, d.speed));});
*/
            endpoint->sendMsg(SystemGetHardwareState{});
            endpoint->sendMsg(TimerGetGlobalTimer{});

            while(!closing) {
                checkSwitchState();

                auto msg = endpoint->recieveMsg();
                if(!msg) {
                    std::this_thread::sleep_for(std::chrono::milliseconds{50});
                    continue;
                }
                registry.handleMsg(msg);
            }
        } catch(const std::exception &e) {
            //LOG(moba::common::LogLevel::ERROR) << "exception occured! <" << e.what() << ">" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{500});
    }
}

void MessageLoop::checkSwitchState() {
    Bridge::SwitchState ss = bridge->checkSwitchState();

    // Taster: 1x kurz -> Ruhemodus an / aus
    //         2x kurz -> Automatik an / aus
    //         1x lang -> Anlage aus

    switch(ss) {
        case Bridge::SwitchState::SHORT_ONCE:
            std::cerr << "SHORT_ONCE" << std::endl;
            // TODO: standby = !standby; ??
            endpoint->sendMsg(SystemSetStandbyMode{standby});
            break;

        case Bridge::SwitchState::SHORT_TWICE:
            std::cerr << "SHORT_TWICE" << std::endl;
            // TODO: automatic = !automatic; ??
            endpoint->sendMsg(SystemSetAutomaticMode{automatic});
            break;

        case Bridge::SwitchState::LONG_ONCE:
            std::cerr << "LONG_ONCE" << std::endl;
            endpoint->sendMsg(SystemHardwareShutdown{});
            break;

        case Bridge::SwitchState::UNSET:
            break;
    }
}

void MessageLoop::setHardwareState(const SystemHardwareStateChanged &data) {

    switch(data.hardwareState) {
        case SystemHardwareStateChanged::HardwareState::ERROR:
            std::cerr << "setHardwareState <ERROR>" << std::endl;
            statusbarState = Bridge::StatusBarState::ERROR;
            break;

        case SystemHardwareStateChanged::HardwareState::STANDBY:
            std::cerr << "setHardwareState <STANDBY>" << std::endl;
            statusbarState = Bridge::StatusBarState::STANDBY;
            break;

        case SystemHardwareStateChanged::HardwareState::EMERGENCY_STOP:
            /* // FIXME: Was machen wir hier?
            case moba::Message::MT_EMERGENCY_STOP_CLEARING:
                bridge->setEmergencyStopClearing();
                bridge->setStatusBar(statusbarState);
                break;
             */

            std::cerr << "setHardwareState <EMERGENCY_STOP>" << std::endl;
            statusbarState = Bridge::StatusBarState::EMERGENCY_STOP;
            bridge->setEmergencyStop();
            if(automatic) {
                bridge->mainLightOn();
            }
            break;

        case SystemHardwareStateChanged::HardwareState::MANUEL:
            std::cerr << "setHardwareState <MANUEL>" << std::endl;
            automatic = false;
            bridge->curtainUp();
            setAmbientLight();
            statusbarState = Bridge::StatusBarState::READY;
            break;

        case SystemHardwareStateChanged::HardwareState::AUTOMATIC:
            std::cerr << "setHardwareState <AUTOMATIC>" << std::endl;
            automatic = true;
            bridge->curtainDown();
            bridge->mainLightOff();
            statusbarState = Bridge::StatusBarState::AUTOMATIC;
            break;
    }
    bridge->setStatusBar(statusbarState);
}

void MessageLoop::setError(const ClientError &data) {
    std::cerr << "ErrorId <" << data.errorId << "> " << data.additionalMsg << std::endl;
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


void MessageLoop::setAmbientLight() {
    bridge->setAmbientLight(
        ambientLightData.blue,
        ambientLightData.green,
        ambientLightData.red,
        ambientLightData.white,
        20
    );
}






    /*
    while(true) {
        switch(msg->getMsgType()) {
            case moba::Message::MT_GLOBAL_TIMER_EVENT:
                globalTimerEvent(msg->getData());
                break;

            case moba::Message::MT_SET_ENVIRONMENT:
                setEnvironment(msg->getData());
                break;

            case moba::Message::MT_SET_AMBIENCE:
                setAmbience(msg->getData());
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

void MessageLoop::setAmbience(moba::JsonItemPtr ptr) {
    moba::JsonObjectPtr o = boost::dynamic_pointer_cast<moba::JsonObject>(ptr);
    moba::JsonToggleState::ToggleState cuup = moba::castToToggleState(o->at("curtainUp"));
    moba::JsonToggleState::ToggleState mlon = moba::castToToggleState(o->at("mainLightOn"));

    LOG(moba::NOTICE) << "curtainUp    <" << cuup << ">" << std::endl;
    LOG(moba::NOTICE) << "mainLightOn  <" << mlon << ">" << std::endl;

    if(automatic) {
        LOG(moba::WARNING) << "automatic is on!" << std::endl;
        return;
    }

    if(mlon == moba::JsonToggleState::ON) {
        bridge->mainLightOn();
    } else if(mlon == moba::JsonToggleState::OFF) {
        bridge->mainLightOff();
    }
    if(cuup == moba::JsonToggleState::ON) {
        bridge->curtainUp();
    } else if(cuup == moba::JsonToggleState::OFF) {
        bridge->curtainDown();
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
