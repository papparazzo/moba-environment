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

#include <boost/algorithm/string.hpp>

#include "msgloop.h"
#include <moba-common/ipc.h>

MessageLoop::MessageLoop(
    const std::string &appName, const moba::Version &version,
    boost::shared_ptr<Bridge> bridge, moba::MsgEndpointPtr endpoint
) : appName(appName), version(version), bridge(bridge), msgEndpoint(endpoint) {
    bridge->setStatusBar(Bridge::SBS_INIT);
}

void MessageLoop::run() {
    while(true) {
        Bridge::SwitchState ss = bridge->checkSwitchState();

        // Taster: 1x kurz -> Ruhemodus an / aus
        //         2x kurz -> Automatik an / aus
        //         1x lang -> Anlage aus

        switch(ss) {
            case Bridge::SS_SHORT_ONCE:
                LOG(moba::INFO) << "SHORT_ONCE" << std::endl;
                msgEndpoint->sendMsg(moba::Message::MT_HARDWARE_SWITCH_STANDBY);
                break;

            case Bridge::SS_SHORT_TWICE:
                LOG(moba::INFO) << "SHORT_TWICE" << std::endl;
                msgEndpoint->sendMsg(moba::Message::MT_SET_AUTO_MODE, moba::toJsonBoolPtr(true));
                break;

            case Bridge::SS_LONG_ONCE:
                LOG(moba::INFO) << "LONG_ONCE" << std::endl;
                msgEndpoint->sendMsg(moba::Message::MT_HARDWARE_SHUTDOWN);
                break;

            case Bridge::SS_UNSET:
                break;
        }

        moba::MessagePtr msg = msgEndpoint->recieveMsg();
        if(!msg) {
            usleep(50000);
            continue;
        }
        LOG(moba::NOTICE) << "New Message <" << *msg << ">" << std::endl;
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

            case moba::Message::MT_SET_HARDWARE_STATE:
            case moba::Message::MT_HARDWARE_STATE_CHANGED: {
                boost::shared_ptr<moba::JsonString> s =
                boost::dynamic_pointer_cast<moba::JsonString>(msg->getData());
                setHardwareState(*s);
                break;
            }

            case moba::Message::MT_SET_AUTO_MODE: {
                boost::shared_ptr<moba::JsonBool> o =
                boost::dynamic_pointer_cast<moba::JsonBool>(msg->getData());
                setAutoMode(o->getVal());
                break;
            }

            case moba::Message::MT_CLIENT_SHUTDOWN:
                bridge->shutdown();
                return;

            case moba::Message::MT_ERROR:
                printError(msg->getData());
                break;

            case moba::Message::MT_CLIENT_RESET:
                bridge->reboot();
                return;

            case moba::Message::MT_CLIENT_SELF_TESTING:
                bridge->selftesting();
                break;

            case moba::Message::MT_EMERGENCY_STOP:
                bridge->setEmergencyStop();
                bridge->setStatusBar(Bridge::SBS_ERROR);
                break;

            case moba::Message::MT_EMERGENCY_STOP_CLEARING:
                bridge->setEmergencyStopClearing();
                bridge->setStatusBar(statusbarState);
                break;

            case moba::Message::MT_SYSTEM_NOTICE: {
                moba::JsonObjectPtr o = boost::dynamic_pointer_cast<moba::JsonObject>(msg->getData());
                LOG(moba::INFO) << moba::castToString(o->at("type")) << ": [" <<
                castToString(o->at("caption")) << "] " <<
                castToString(o->at("text")) << std::endl;
                break;
            }

            default:
                break;
        }
    }
}

void MessageLoop::connect() {
    moba::JsonArrayPtr groups(new moba::JsonArray());
    groups->push_back(moba::toJsonStringPtr("BASE"));
    groups->push_back(moba::toJsonStringPtr("ENV"));
    groups->push_back(moba::toJsonStringPtr("SYSTEM"));

    appId = msgEndpoint->connect(appName, version, groups);
    LOG(moba::NOTICE) << "AppId <" << appId << ">" << std::endl;

    msgEndpoint->sendMsg(moba::Message::MT_GET_HARDWARE_STATE);
    msgEndpoint->sendMsg(moba::Message::MT_GET_AUTO_MODE);
    msgEndpoint->sendMsg(moba::Message::MT_GET_AMBIENT_LIGHT);
}

void MessageLoop::printError(moba::JsonItemPtr ptr) {
    moba::JsonObjectPtr o = boost::dynamic_pointer_cast<moba::JsonObject>(ptr);

    boost::shared_ptr<moba::JsonNumber<long int> > i =
    boost::dynamic_pointer_cast<moba::JsonNumber<long int> >(o->at("errorId"));
    moba::JsonStringPtr s = boost::dynamic_pointer_cast<moba::JsonString>(o->at("additonalMsg"));
    LOG(moba::WARNING) << "ErrorId <" << i << "> " << s << std::endl;
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

void MessageLoop::setAmbientLight(moba::JsonItemPtr ptr) {
    LOG(moba::NOTICE) << "setAmbientLight" << std::endl;
    moba::JsonObjectPtr o = boost::dynamic_pointer_cast<moba::JsonObject>(ptr);
    ambientLightData.blue = boost::dynamic_pointer_cast<moba::JsonNumber<long int> >(o->at("blue"))->getVal();
    LOG(moba::NOTICE) << "blue <" << ambientLightData.blue << ">" << std::endl;
    ambientLightData.green = boost::dynamic_pointer_cast<moba::JsonNumber<long int> >(o->at("green"))->getVal();
    LOG(moba::NOTICE) << "blue <" << ambientLightData.green << ">" << std::endl;
    ambientLightData.red = boost::dynamic_pointer_cast<moba::JsonNumber<long int> >(o->at("red"))->getVal();
    LOG(moba::NOTICE) << "red <" << ambientLightData.red << ">" << std::endl;
    ambientLightData.white = boost::dynamic_pointer_cast<moba::JsonNumber<long int> >(o->at("white"))->getVal();
    LOG(moba::NOTICE) << "white <" << ambientLightData.white << ">" << std::endl;
    if(!automatic) {
        setAmbientLight();
    }
}

void MessageLoop::setAmbientLight() {
    bridge->setAmbientLight(
        ambientLightData.blue,
        ambientLightData.green,
        ambientLightData.red,
        ambientLightData.white,
        20
    );
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

void MessageLoop::setAutoMode(bool on) {
    if(on) {
        LOG(moba::NOTICE) << "setAutoMode <on>" << std::endl;
        automatic = true;
        bridge->curtainDown();
        if(statusbarState == Bridge::SBS_READY) {
            bridge->mainLightOff();
            statusbarState == Bridge::SBS_AUTOMATIC;
            bridge->setStatusBar(statusbarState);
        }
        return;
    }
    LOG(moba::NOTICE) << "setAutoMode <off>" << std::endl;
    automatic = false;
    bridge->curtainUp();
    setAmbientLight();
    if(statusbarState == Bridge::SBS_AUTOMATIC) {
        statusbarState == Bridge::SBS_READY;
        bridge->setStatusBar(statusbarState);
    }
    return;
}

void MessageLoop::setHardwareState(const std::string &s) {
    if(boost::iequals(s, "STANDBY")) {
        LOG(moba::NOTICE) << "setHardwareState <STANDBY>" << std::endl;
        statusbarState = Bridge::SBS_STANDBY;
    } else if(boost::iequals(s, "POWER_OFF")) {
        LOG(moba::NOTICE) << "setHardwareState <POWER_OFF>" << std::endl;
        statusbarState = Bridge::SBS_POWER_OFF;
        if(automatic) {
            bridge->mainLightOn();
        }
    } else if(boost::iequals(s, "READY") && automatic) {
        LOG(moba::NOTICE) << "setHardwareState <READY>" << std::endl;
        statusbarState = Bridge::SBS_AUTOMATIC;
        bridge->mainLightOff();
    } else if(boost::iequals(s, "READY")) {
        LOG(moba::NOTICE) << "setHardwareState <READY>" << std::endl;
        statusbarState = Bridge::SBS_READY;
    } else {
        LOG(moba::NOTICE) << "setHardwareState <ERROR>" << std::endl;
        statusbarState = Bridge::SBS_ERROR;
    }
    bridge->setStatusBar(statusbarState);
}
