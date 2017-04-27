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
#include <moba/ipc.h>

MessageLoop::MessageLoop(
    const std::string &appName, const moba::Version &version, boost::shared_ptr<Bridge> bridge
) : appName(appName), version(version), bridge(bridge) {
    this->bridge->setStatusBar(Bridge::SBS_INIT);
}

void MessageLoop::run() {
    while(true) {
        Bridge::SwitchState ss = this->bridge->checkSwitchState();

        // Taster: 1x kurz -> Ruhemodus an / aus
        //         2x kurz -> Automatik an / aus
        //         1x lang -> Anlage aus

        switch(ss) {
            case Bridge::SS_SHORT_ONCE:
                LOG(moba::INFO) << "SHORT_ONCE" << std::endl;
                this->msgHandler.sendHardwareSwitchStandby();
                break;

            case Bridge::SS_SHORT_TWICE:
                LOG(moba::INFO) << "SHORT_TWICE" << std::endl;
                this->msgHandler.sendSetAutoMode(true);
                break;

            case Bridge::SS_LONG_ONCE:
                LOG(moba::INFO) << "LONG_ONCE" << std::endl;
                this->msgHandler.sendHardwareShutdown();
                break;

            case Bridge::SS_UNSET:
                break;
        }

        moba::MessagePtr msg = this->msgHandler.recieveMsg();
        if(!msg) {
            usleep(50000);
            continue;
        }
        LOG(moba::NOTICE) << "New Message <" << *msg << ">" << std::endl;
        switch(msg->getMsgType()) {
            case moba::Message::MT_GLOBAL_TIMER_EVENT:
                this->globalTimerEvent(msg->getData());
                break;

            case moba::Message::MT_SET_ENVIRONMENT:
                this->setEnvironment(msg->getData());
                break;

            case moba::Message::MT_SET_AMBIENCE:
                this->setAmbience(msg->getData());
                break;

            case moba::Message::MT_SET_AMBIENT_LIGHT:
                this->setAmbientLight(msg->getData());
                break;

            case moba::Message::MT_SET_HARDWARE_STATE:
            case moba::Message::MT_HARDWARE_STATE_CHANGED: {
                boost::shared_ptr<moba::JsonString> s =
                boost::dynamic_pointer_cast<moba::JsonString>(msg->getData());
                this->setHardwareState(*s);
                break;
            }

            case moba::Message::MT_SET_AUTO_MODE: {
                boost::shared_ptr<moba::JsonBool> o =
                boost::dynamic_pointer_cast<moba::JsonBool>(msg->getData());
                this->setAutoMode(o->getVal());
                break;
            }

            case moba::Message::MT_CLIENT_SHUTDOWN:
                this->bridge->shutdown();
                return;

            case moba::Message::MT_ERROR:
                this->printError(msg->getData());
                break;

            case moba::Message::MT_CLIENT_RESET:
                this->bridge->reboot();
                return;

            case moba::Message::MT_CLIENT_SELF_TESTING:
                this->bridge->selftesting();
                break;

            case moba::Message::MT_EMERGENCY_STOP:
                this->bridge->setEmergencyStop();
                this->bridge->setStatusBar(Bridge::SBS_ERROR);
                break;

            case moba::Message::MT_EMERGENCY_STOP_CLEARING:
                this->bridge->setEmergencyStopClearing();
                this->bridge->setStatusBar(this->statusbarState);
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

void MessageLoop::connect(const std::string &host, int port) {
    LOG(moba::INFO) << "Try to connect (" << host << ":" << port << ")..." << std::endl;
    this->msgHandler.connect(host, port);
    moba::JsonArrayPtr groups(new moba::JsonArray());
    groups->push_back(moba::toJsonStringPtr("BASE"));
    groups->push_back(moba::toJsonStringPtr("ENV"));
    groups->push_back(moba::toJsonStringPtr("SYSTEM"));

    this->appId = this->msgHandler.registerApp(
        this->appName,
        this->version,
        groups
    );
    LOG(moba::NOTICE) << "AppId <" << this->appId << ">" << std::endl;
}

void MessageLoop::init() {
    this->msgHandler.sendGetHardwareState();
    this->msgHandler.sendGetAutoMode();
    this->msgHandler.sendGetAmbientLight();
}

void MessageLoop::printError(moba::JsonItemPtr ptr) {
    moba::JsonObjectPtr o = boost::dynamic_pointer_cast<moba::JsonObject>(ptr);

    boost::shared_ptr<moba::JsonNumber<int> > i =
    boost::dynamic_pointer_cast<moba::JsonNumber<int> >(o->at("errorId"));
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
            this->bridge->thunderstormOn();
            break;

        case moba::JsonSwitch::OFF:
            this->bridge->thunderstormOff();
            break;

        case moba::JsonSwitch::TRIGGER:
            this->bridge->thunderstormTrigger();
            break;
    }

    for(int i = 0; i < 3; ++i) {
        switch(a[i]) {
            case moba::JsonSwitch::AUTO:
            case moba::JsonSwitch::ON:
                this->bridge->auxOn((Bridge::AuxPin)i);
                break;

            case moba::JsonSwitch::OFF:
                this->bridge->auxOff((Bridge::AuxPin)i);
                break;

            case moba::JsonSwitch::TRIGGER:
                this->bridge->auxTrigger((Bridge::AuxPin)i);
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

    if(this->automatic) {
        LOG(moba::WARNING) << "automatic is on!" << std::endl;
        return;
    }

    if(mlon == moba::JsonToggleState::ON) {
        this->bridge->mainLightOn();
    } else if(mlon == moba::JsonToggleState::OFF) {
        this->bridge->mainLightOff();
    }
    if(cuup == moba::JsonToggleState::ON) {
        this->bridge->curtainUp();
    } else if(cuup == moba::JsonToggleState::OFF) {
        this->bridge->curtainDown();
    }
}

void MessageLoop::setAmbientLight(moba::JsonItemPtr ptr) {
    LOG(moba::NOTICE) << "setAmbientLight" << std::endl;
    boost::shared_ptr<moba::JsonNumber<int> > d;
    moba::JsonObjectPtr o = boost::dynamic_pointer_cast<moba::JsonObject>(ptr);
    this->ambientLightData.red = boost::dynamic_pointer_cast<moba::JsonNumber<int> >(o->at("red"));
    LOG(moba::NOTICE) << "red <" << this->ambientLightData.red << ">" << std::endl;
    this->ambientLightData.blue = boost::dynamic_pointer_cast<moba::JsonNumber<int> >(o->at("blue"));
    LOG(moba::NOTICE) << "blue <" << this->ambientLightData.blue << ">" << std::endl;
    this->ambientLightData.white = boost::dynamic_pointer_cast<moba::JsonNumber<int> >(o->at("white"));
    LOG(moba::NOTICE) << "white <" << this->ambientLightData.white << ">" << std::endl;
    if(!this->automatic) {
        this->setAmbientLight();
    }
}

void MessageLoop::setAmbientLight() {
    this->bridge->setAmbientLight(
        this->ambientLightData.blue,
        this->ambientLightData.green,
        this->ambientLightData.red,
        this->ambientLightData.white,
        20
    );
}

void MessageLoop::globalTimerEvent(moba::JsonItemPtr ptr) {
    moba::JsonObjectPtr o = boost::dynamic_pointer_cast<moba::JsonObject>(ptr);
    o->at("curModelTime");
    o->at("multiplicator");

    boost::shared_ptr<moba::JsonNumber<int> > i =
    boost::dynamic_pointer_cast<moba::JsonNumber<int> >(o->at("errorId"));
    moba::JsonStringPtr s = boost::dynamic_pointer_cast<moba::JsonString>(o->at("additonalMsg"));

    int mz;
    int dur;
    //Sonnenaufgang   04:00; dauer 2h
    if(4 * 60 + 30 == mz) {
        //blue, green, red, white, duration
        this->bridge->setAmbientLight(500, 250, 300, dur);
        this->bridge->setAmbientLight(500, 250, 700, dur);
    }

    //Sonnenuntergang 22:00; dauer 2h
    if(21 * 60 + 30 == mz) {
        this->bridge->setAmbientLight();
    }
}

void MessageLoop::setAutoMode(bool on) {
    if(on) {
        LOG(moba::NOTICE) << "setAutoMode <on>" << std::endl;
        this->automatic = true;
        this->bridge->curtainDown();
        if(this->statusbarState == Bridge::SBS_READY) {
            this->bridge->mainLightOff();
            this->statusbarState == Bridge::SBS_AUTOMATIC;
            this->bridge->setStatusBar(this->statusbarState);
        }
        return;
    }
    LOG(moba::NOTICE) << "setAutoMode <off>" << std::endl;
    this->automatic = false;
    this->bridge->curtainUp();
    this->setAmbientLight();
    if(this->statusbarState == Bridge::SBS_AUTOMATIC) {
        this->statusbarState == Bridge::SBS_READY;
        this->bridge->setStatusBar(this->statusbarState);
    }
    return;
}

void MessageLoop::setHardwareState(const std::string &s) {
    if(boost::iequals(s, "STANDBY")) {
        LOG(moba::NOTICE) << "setHardwareState <STANDBY>" << std::endl;
        this->statusbarState = Bridge::SBS_STANDBY;
    } else if(boost::iequals(s, "POWER_OFF")) {
        LOG(moba::NOTICE) << "setHardwareState <POWER_OFF>" << std::endl;
        this->statusbarState = Bridge::SBS_POWER_OFF;
        if(this->automatic) {
            this->bridge->mainLightOn();
        }
    } else if(boost::iequals(s, "READY") && this->automatic) {
        LOG(moba::NOTICE) << "setHardwareState <READY>" << std::endl;
        this->statusbarState = Bridge::SBS_AUTOMATIC;
        this->bridge->mainLightOff();
    } else if(boost::iequals(s, "READY")) {
        LOG(moba::NOTICE) << "setHardwareState <READY>" << std::endl;
        this->statusbarState = Bridge::SBS_READY;
    } else {
        LOG(moba::NOTICE) << "setHardwareState <ERROR>" << std::endl;
        this->statusbarState = Bridge::SBS_ERROR;
    }
    this->bridge->setStatusBar(this->statusbarState);
}
