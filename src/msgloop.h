/*
 * File:   messageLoop.h
 * Author: stefan
 *
 * Created on December 12, 2015, 10:53 PM
 */

#ifndef MESSAGELOOP_H
#define	MESSAGELOOP_H

#include <string>

#include "bridge.h"

#include <moba/msghandler.h>
#include <moba/log.h>

class MessageLoop {
    public:
        MessageLoop(const std::string &appName, const Version &version);
        MessageLoop(const MessageLoop& orig) {
        }
        virtual ~MessageLoop() {
        }

        void run();
        void connect(const std::string &host, int port);

    protected:
        void printError(JsonItemPtr ptr);
        void checkSwitchState();
        void setAutoMode(bool on);
        void setEnvironment(JsonItemPtr ptr);
        void setAmbience(JsonItemPtr ptr);
        void setGlobalTimer(JsonItemPtr ptr);
        void setHardwareState(const std::string &s);

        bool automatic;

        MsgHandler msgHandler;
        long appId;
        std::string appName;
        Version version;
        Bridge bridge;
};

#endif	/* MESSAGELOOP_H */

