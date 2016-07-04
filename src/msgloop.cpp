/*
 * File:   messageLoop.cpp
 * Author: stefan
 *
 * Created on December 12, 2015, 10:53 PM
 */

#include <boost/algorithm/string.hpp>

#include "msgloop.h"

MessageLoop::MessageLoop(const std::string &appName, const Version &version) :
appName(appName), version(version) {
}

void MessageLoop::run() {
    while(true) {
        Bridge::SwitchState ss = this->bridge.checkSwitchState();

        // Taster: 1x kurz -> Ruhemodus an / aus
        //         2x kurz -> Automatik an / aus
        //         1x lang -> Anlage aus

        switch(ss) {
            case Bridge::SS_SHORT_ONCE:
                LOG(tlog::Info) << "SHORT_ONCE";
                this->msgHandler.sendHardwareSwitchStandby();
                break;

            case Bridge::SS_SHORT_TWICE:
                LOG(tlog::Info) << "SHORT_TWICE";
                this->msgHandler.sendSetAutoMode(true);
                break;

            case Bridge::SS_LONG_ONCE:
                LOG(tlog::Info) << "LONG_ONCE";
                this->msgHandler.sendHardwareShutdown();
                break;

            case Bridge::SS_UNSET:
                break;
        }

        MessagePtr msg = this->msgHandler.recieveMsg();
        if(!msg) {
            continue;
        }
        LOG(tlog::Notice) << "New Message <" << *msg << ">";
        switch(msg->getMsgType()) {
            case Message::MT_GLOBAL_TIMER_EVENT:
                break;

            case Message::MT_SET_GLOBAL_TIMER:
                break;

            case Message::MT_SET_ENVIRONMENT:
                this->setEnvironment(msg->getData());
                break;

            case Message::MT_SET_AMBIENCE:
                this->setAmbience(msg->getData());
                break;

            case Message::MT_HARDWARE_STATE_CHANGED: {
                boost::shared_ptr<JsonString> s =
                boost::dynamic_pointer_cast<JsonString>(msg->getData());
                this->setHardwareState(*s);
                break;
            }

            case Message::MT_SET_AUTO_MODE: {
                boost::shared_ptr<JsonBool> o =
                boost::dynamic_pointer_cast<JsonBool>(msg->getData());
                this->setAutoMode(o->getVal());
                break;
            }

            case Message::MT_CLIENT_SHUTDOWN:
                this->bridge.shutdown();
                return;

            case Message::MT_ERROR:
                this->printError(msg->getData());
                break;

            case Message::MT_CLIENT_RESET:
                this->bridge.reboot();
                return;

            case Message::MT_CLIENT_SELF_TESTING:
                this->bridge.selftesting();
                break;

            default:
                break;
        }
    }
}

void MessageLoop::connect(const std::string &host, int port) {
    LOG(tlog::Info) << "Try to connect (" << host << ":" << port << ")...";
    this->msgHandler.connect(host, port);
    JsonArrayPtr groups(new JsonArray());
    groups->push_back(toJsonStringPtr("BASE"));
    groups->push_back(toJsonStringPtr("ENV"));
    groups->push_back(toJsonStringPtr("SYSTEM"));

    this->appId = this->msgHandler.registerApp(
        this->appName,
        this->version,
        groups
    );
    LOG(tlog::Notice) << "AppId <" << this->appId << ">";
}

void MessageLoop::printError(JsonItemPtr ptr) {
    JsonObjectPtr o = boost::dynamic_pointer_cast<JsonObject>(ptr);

    boost::shared_ptr<JsonNumber<int> > i =
    boost::dynamic_pointer_cast<JsonNumber<int> >(o->at("errorId"));
    JsonStringPtr s = boost::dynamic_pointer_cast<JsonString>(o->at("additonalMsg"));
    LOG(tlog::Warning) << "ErrorId <" << i << "> " << s;
}

void MessageLoop::setEnvironment(JsonItemPtr ptr) {
    JsonObjectPtr o = boost::dynamic_pointer_cast<JsonObject>(ptr);
    JsonSwitch::Switch a[3];
    JsonSwitch::Switch thst = castToSwitch(o->at("thunderStorm"));
    JsonSwitch::Switch wind = castToSwitch(o->at("wind"));
    JsonSwitch::Switch rain = castToSwitch(o->at("rain"));
    JsonSwitch::Switch enso = castToSwitch(o->at("environmentSound"));
    a[0] = castToSwitch(o->at("aux01"));
    a[1] = castToSwitch(o->at("aux02"));
    a[2] = castToSwitch(o->at("aux03"));

    LOG(tlog::Notice) << "thunderStorm     <" << thst << ">";
    LOG(tlog::Notice) << "wind             <" << wind << ">";
    LOG(tlog::Notice) << "environmentSound <" << enso << ">";
    LOG(tlog::Notice) << "rain             <" << rain << ">";
    LOG(tlog::Notice) << "aux01            <" << a[0] << ">";
    LOG(tlog::Notice) << "aux02            <" << a[1] << ">";
    LOG(tlog::Notice) << "aux03            <" << a[2] << ">";

    switch(thst) {
        case JsonSwitch::AUTO:
        case JsonSwitch::ON:
            this->bridge.thunderstormOn();
            break;

        case JsonSwitch::OFF:
            this->bridge.thunderstormOff();
            break;

        case JsonSwitch::TRIGGER:
            this->bridge.thunderstormTrigger();
            break;
    }

    for(int i = 0; i < 3; ++i) {
        switch(a[i]) {
            case JsonSwitch::AUTO:
            case JsonSwitch::ON:
                this->bridge.auxOn((Bridge::AuxPin)i);
                break;

            case JsonSwitch::OFF:
                this->bridge.auxOff((Bridge::AuxPin)i);
                break;

            case JsonSwitch::TRIGGER:
                this->bridge.auxTrigger((Bridge::AuxPin)i);
                break;
        }
    }
}

void MessageLoop::setAmbience(JsonItemPtr ptr) {
    JsonObjectPtr o = boost::dynamic_pointer_cast<JsonObject>(ptr);
    JsonThreeState::ThreeState cuup = castToThreeState(o->at("curtainUp"));
    JsonThreeState::ThreeState mlon = castToThreeState(o->at("mainLightOn"));

    LOG(tlog::Notice) << "curtainUp    <" << cuup << ">";
    LOG(tlog::Notice) << "mainLightOn  <" << mlon << ">";

    if(mlon == JsonThreeState::ON) {
        this->bridge.mainLightOn();
    } else if(mlon == JsonThreeState::OFF) {
        this->bridge.mainLightOff();
    }
    if(cuup == JsonThreeState::ON) {
        this->bridge.curtainUp();
    } else if(cuup == JsonThreeState::OFF) {
        this->bridge.curtainDown();
    }
}

void MessageLoop::setGlobalTimer(JsonItemPtr ptr) {
    JsonObjectPtr o = boost::dynamic_pointer_cast<JsonObject>(ptr);
}

void MessageLoop::setAutoMode(bool on) {
    if(on) {
        LOG(tlog::Notice) << "setAutoMode <on>";
        this->automatic = true;
        this->bridge.curtainDown();
        this->bridge.mainLightOff();
        // FIXME: Achtung prüfen, ob Notaus...
        this->bridge.setStatusBar(Bridge::SBS_AUTOMATIC);
        return;
    }
    LOG(tlog::Notice) << "setAutoMode <off>";
    this->automatic = false;
    this->bridge.curtainDown();
    this->bridge.mainLightOff();
    // FIXME: Achtung prüfen, ob Notaus...
    this->bridge.setStatusBar(Bridge::SBS_READY);
    return;
}

void MessageLoop::setHardwareState(const std::string &s) {
    if(boost::iequals(s, "STANDBY")) {
        this->bridge.setStatusBar(Bridge::SBS_STANDBY);
    } else if(boost::iequals(s, "POWER_OFF")) {
        this->bridge.setStatusBar(Bridge::SBS_POWER_OFF);
    } else if(boost::iequals(s, "READY") && this->automatic) {
        this->bridge.setStatusBar(Bridge::SBS_AUTOMATIC);
    } else if(boost::iequals(s, "READY")) {
        this->bridge.setStatusBar(Bridge::SBS_READY);
    } else {

    }
/*
            SBS_ERROR          = 0,   // rot blitz
            SBS_STANDBY        = 1,   // grün blitz
            SBS_POWER_OFF      = 2,   // rot
            SBS_EMERGENCY_STOP = 3,   // rot blink
            SBS_READY          = 4,   // grün
            SBS_AUTOMATIC      = 5    // grün blink
 */

}



//msgHandler.sendGetGlobalTimer();
//msgHandler.sendGetEnvironment();
//msgHandler.sendHardwareStateRequest();
//msgHandler.sendGetServerState();





