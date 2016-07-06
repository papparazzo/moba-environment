/*
 * File:   messageLoop.h
 * Author: stefan
 *
 * Created on December 12, 2015, 10:53 PM
 */

#ifndef MESSAGELOOP_H
#define	MESSAGELOOP_H

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

#include "bridge.h"

#include <moba/msghandler.h>
#include <moba/log.h>

class MessageLoop : private boost::noncopyable {
    public:
        MessageLoop(
            const std::string &appName,
            const Version &version,
            boost::shared_ptr<Bridge> bridge
        );

        void run();
        void init();
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
        boost::shared_ptr<Bridge> bridge;

        Bridge::StatusBarState statusbarState;
};

#endif	/* MESSAGELOOP_H */

