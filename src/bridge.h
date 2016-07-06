/*
 * File:   bridge.h
 * Author: stefan
 *
 * Created on December 20, 2015, 10:34 PM
 */

#ifndef BRIDGE_H
#define	BRIDGE_H

class Bridge {
    public:
        enum StatusBarState {
            SBS_ERROR          = 0,   // rot blitz
            SBS_INIT           = 1,   // rot
            SBS_POWER_OFF      = 2,   // rot blink

            SBS_STANDBY        = 3,   // grün blitz
            SBS_READY          = 4,   // grün
            SBS_AUTOMATIC      = 5    // grün blink
        };

        enum AuxPin {
            AUX1,
            AUX2,
            AUX3
        };

        enum SwitchState {
            SS_UNSET,
            SS_SHORT_ONCE,
            SS_SHORT_TWICE,
            SS_LONG_ONCE
        };

        Bridge();
        virtual ~Bridge();

        void curtainUp();
        void curtainDown();
        void mainLightOn();
        void mainLightOff();

        void shutdown();
        void reboot();

        void auxOn(AuxPin nb);
        void auxOff(AuxPin nb);
        void auxTrigger(AuxPin nb);

        void thunderstormOn();
        void thunderstormOff();
        void thunderstormTrigger();

        void setStatusBar(StatusBarState sbstate);

        void selftesting();

        SwitchState checkSwitchState();

    private:
        Bridge(const Bridge& orig);
};

#endif	// BRIDGE_H

