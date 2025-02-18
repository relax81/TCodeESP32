
// OSR-Alpha3_ESP32
// by TempestMAx 9-7-21
// Please copy, share, learn, innovate, give attribution.
// Decodes T-code commands and uses them to control servos and vibration motors
// It can handle:
//   10x linear channels (L0, L1, L2... L9)
//   10x rotation channels (R0, R1, R2... L9) 
//   10x vibration channels (V0, V1, V2... V9)
//   10x auxilliary channels (A0, A1, A2... A9)
// This code is for the ESP32 DevKit v1 and is designed to drive the SR6 stroker robot, 
// but is also able to drive the OSR2. See below for servo pin assignments
// Have fun, play safe!
// History:
// Alpha3 - First ESP32 release, 9-7-2021


// ----------------------------
//   Settings
// ----------------------------

#pragma once

#include "TCode0_3.h"
#include "../Global.h"
#include "../ServoHandler.h"

class ServoHandler0_3 : public ServoHandler {

public:
    // Setup function
    // This is run once, when the arduino starts
    void setup(int servoFrequency, int pitchFrequency, int valveFrequency, int twistFrequency, int msPerRad) override {
        ms_per_rad = msPerRad;
        MainServo_Freq = servoFrequency;
        PitchServo_Freq = pitchFrequency;
        TwistServo_Freq = twistFrequency;
        ValveServo_Freq = valveFrequency;
        SqueezeServo_Freq = SettingsHandler::squeezeFrequency;
// Servo Pulse intervals
        MainServo_Int = 1000000/MainServo_Freq;
        PitchServo_Int = 1000000/PitchServo_Freq;
        TwistServo_Int = 1000000/TwistServo_Freq;
        ValveServo_Int = 1000000/ValveServo_Freq;
        SqueezeServo_Int = 1000000/SqueezeServo_Freq;

        tcode.setup(SettingsHandler::ESP32Version, SettingsHandler::TCodeVersionName.c_str());
        // report status
        tcode.StringInput("D0");
        tcode.StringInput("D1");

        // Set SR6 arms to startup positions
        if (SettingsHandler::sr6Mode) { tcode.StringInput("R2750"); }

        // Register device axes
        tcode.RegisterAxis("L0", "Up");
        if (SettingsHandler::sr6Mode) {
            tcode.RegisterAxis("L1", "Forward");
            tcode.RegisterAxis("L2", "Left");
        }
        tcode.RegisterAxis("R0", "Twist");
        tcode.RegisterAxis("R1", "Roll");
        tcode.RegisterAxis("R2", "Pitch");
        tcode.RegisterAxis("V0", "Vibe1");
        if (!SettingsHandler::lubeEnabled) { tcode.RegisterAxis("V1", "Vibe2"); }
        tcode.RegisterAxis("V2", "Vibe3");
        tcode.RegisterAxis("V3", "Vibe4");
        tcode.RegisterAxis("A0", "Valve");
        tcode.RegisterAxis("A1", "Suck");
        tcode.AxisInput("A1",VALVE_DEFAULT,'I',3000);
        if (SettingsHandler::lubeEnabled) {
            tcode.RegisterAxis("A2", "Lube");
            tcode.AxisInput("A2",0,' ',0);
            pinMode(SettingsHandler::LubeButton_PIN,INPUT);
        }
        tcode.RegisterAxis("A3", "Squeeze");
        // Setup Servo PWM channels
        // Lower Left Servo
        if(DEBUG_BUILD == 0) {
            LogHandler::verbose(_TAG, "Connecting left servo to pin: %u", SettingsHandler::LeftServo_PIN);
            ledcSetup(LowerLeftServo_PWM,MainServo_Freq,16);
            ledcAttachPin(SettingsHandler::LeftServo_PIN,LowerLeftServo_PWM);
            // Lower Right Servo
            LogHandler::verbose(_TAG, "Connecting right servo to pin: %u", SettingsHandler::RightServo_PIN);
            ledcSetup(LowerRightServo_PWM,MainServo_Freq,16);
            ledcAttachPin(SettingsHandler::RightServo_PIN,LowerRightServo_PWM);
        }
        if(SettingsHandler::sr6Mode)
        {
            // Upper Left Servo
            LogHandler::verbose(_TAG, "Connecting left upper servo to pin: %u", SettingsHandler::LeftUpperServo_PIN);
            ledcSetup(UpperLeftServo_PWM,MainServo_Freq,16);
            ledcAttachPin(SettingsHandler::LeftUpperServo_PIN,UpperLeftServo_PWM);
            if(DEBUG_BUILD == 0) {
                // Upper Right Servo
            LogHandler::verbose(_TAG, "Connecting right upper servo to pin: %u", SettingsHandler::RightUpperServo_PIN);
                ledcSetup(UpperRightServo_PWM,MainServo_Freq,16);
                ledcAttachPin(SettingsHandler::RightUpperServo_PIN,UpperRightServo_PWM);
                // Right Pitch Servo
                LogHandler::verbose(_TAG, "Connecting right pitch servo to pin: %u", SettingsHandler::PitchRightServo_PIN);
                ledcSetup(RightPitchServo_PWM,PitchServo_Freq,16);
                ledcAttachPin(SettingsHandler::PitchRightServo_PIN,RightPitchServo_PWM);
            }
        }
        // Left Pitch Servo
        LogHandler::verbose(_TAG, "Connecting pitch servo to pin: %u", SettingsHandler::PitchLeftServo_PIN);
        ledcSetup(LeftPitchServo_PWM,PitchServo_Freq,16);
        ledcAttachPin(SettingsHandler::PitchLeftServo_PIN,LeftPitchServo_PWM);
        // Twist Servo
        LogHandler::verbose(_TAG, "Connecting twist servo to pin: %u", SettingsHandler::TwistServo_PIN);
        ledcSetup(TwistServo_PWM,TwistServo_Freq,16);
        ledcAttachPin(SettingsHandler::TwistServo_PIN,TwistServo_PWM);
        // Valve Servo
        LogHandler::verbose(_TAG, "Connecting valve servo to pin: %u", SettingsHandler::ValveServo_PIN);
        ledcSetup(ValveServo_PWM,ValveServo_Freq,16);
        ledcAttachPin(SettingsHandler::ValveServo_PIN,ValveServo_PWM);

        LogHandler::verbose(_TAG, "Connecting squeeze servo to pin: %u", SettingsHandler::Squeeze_PIN);
        ledcSetup(SqueezeServo_PWM,SqueezeServo_Freq,16);
        ledcAttachPin(SettingsHandler::Squeeze_PIN,SqueezeServo_PWM);

        // Set vibration PWM pins
        // Vibe0 Pin
        LogHandler::verbose(_TAG, "Connecting vib 1 to pin: %u", SettingsHandler::Vibe0_PIN);
        ledcSetup(Vibe0_PWM,VibePWM_Freq,8);
        ledcAttachPin(SettingsHandler::Vibe0_PIN,Vibe0_PWM);
        // Vibe1 Pin
        LogHandler::verbose(_TAG, "Connecting lube/vib 2 to pin: %u", SettingsHandler::Vibe1_PIN);
        ledcSetup(Vibe1_PWM,VibePWM_Freq,8);
        ledcAttachPin(SettingsHandler::Vibe1_PIN,Vibe1_PWM); 

        LogHandler::verbose(_TAG, "Connecting vib 3 to pin: %u", SettingsHandler::Vibe3_PIN);
        ledcSetup(Vibe2_PWM,VibePWM_Freq,8);
        ledcAttachPin(SettingsHandler::Vibe3_PIN,Vibe2_PWM); 

        if(SettingsHandler::feedbackTwist)
        {
            // Initiate position tracking for twist
            pinMode(SettingsHandler::TwistFeedBack_PIN,INPUT);
            if(!SettingsHandler::analogTwist) 
            {
                LogHandler::verbose(_TAG, "Attaching interrupt for twist feedback to pin: %u", SettingsHandler::TwistFeedBack_PIN);
                attachInterrupt(SettingsHandler::TwistFeedBack_PIN, twistChange, CHANGE);
                //Serial.print("Setting digital twist "); 
                //Serial.println(SettingsHandler::TwistFeedBack_PIN);
            } 
            else
            {
                //Serial.print("Setting analog twist "); 
                //Serial.println(SettingsHandler::TwistFeedBack_PIN);
    /*             adcAttachPin(SettingsHandler::TwistFeedBack_PIN);
                analogReadResolution(11);
                analogSetAttenuation(ADC_6db); */
            }
        } else {
            LogHandler::verbose(_TAG, "Connecting vib 4 to pin: %u", SettingsHandler::Vibe4_PIN);
            ledcSetup(Vibe3_PWM,VibePWM_Freq,8);
            ledcAttachPin(SettingsHandler::Vibe4_PIN,Vibe3_PWM); 
        }
        
        // Signal done
        tcode.sendMessage("Ready!");
    }

    void setMessageCallback(TCODE_FUNCTION_PTR_T function) override {
        tcode.setMessageCallback(function);
    }

    void read(String input) override
    {
        tcode.StringInput(input);
    }

    void read(byte input) override 
    {
        tcode.ByteInput(input);
    }

    // String getDeviceSettings() {
    //     return tcode.getDeviceSettings();
    // }

// int testVar = -1;
// int testVar2 = -1;
    void execute() override {
        // Collect inputs
        // These functions query the t-code object for the position/level at a specified time
        // Number recieved will be an integer, 0-9999
        xLin = tcode.AxisRead("L0");
        if (SettingsHandler::sr6Mode) {
            yLin = tcode.AxisRead("L1");
            zLin = tcode.AxisRead("L2");
        }
        xRot = tcode.AxisRead("R0");
        yRot = tcode.AxisRead("R1");
        zRot = tcode.AxisRead("R2");
        vibe0 = tcode.AxisRead("V0");
        if (!SettingsHandler::lubeEnabled) { vibe1 = tcode.AxisRead("V1"); }
        vibe2 = tcode.AxisRead("V2");
        vibe3 = tcode.AxisRead("V3");
        valveCmd = tcode.AxisRead("A0");
        suckCmd = tcode.AxisRead("A1");
        if (SettingsHandler::lubeEnabled) { lube = tcode.AxisRead("A2"); }
        squeezeCmd = tcode.AxisRead("A3");

        // If you want to mix your servos differently, enter your code below:

        if (SettingsHandler::feedbackTwist && !SettingsHandler::continuousTwist) 
        {
            float angPos;
            // Calculate twist position
            if (!SettingsHandler::analogTwist)
            {
                noInterrupts();
                float dutyCycle = twistPulseLength;
                dutyCycle = dutyCycle/lastTwistPulseCycle;
                interrupts();
                angPos = (dutyCycle - 0.029)/0.942;
                    //  Serial.print("angPos "); 
                    //  Serial.println(angPos);
            }
            else 
            {
                int feedBackValue = analogRead(SettingsHandler::TwistFeedBack_PIN);
                angPos = feedBackValue / 675.0;
                // if(feedBackValue != testVar) {
                //     testVar = feedBackValue;
                //     Serial.print("feedBackValue: ");
                //     Serial.println(feedBackValue);
                //     Serial.print("angPos: ");
                //     Serial.println(angPos);
                // }
            }
            angPos = constrain(angPos,0,1) - 0.5;
            if (angPos - twistServoAngPos < - 0.8) { twistTurns += 1; }
            if (angPos - twistServoAngPos > 0.8) { twistTurns -= 1; }
            twistServoAngPos = angPos;
            twistPos = 1000*(angPos + twistTurns);
        }

        // Calculate valve position
        // Track receiver velocity
        unsigned long t = millis();
        float upVelNow;
        if (t > tLast) {
            upVelNow = xLin - xLast;
            upVelNow /= t - tLast;
            upVel = (upVelNow + 9*upVel)/10;
        }
        tLast = t;
        xLast = xLin;
        // Use suck command if most recent
        bool suck;
        if (tcode.AxisLast("A1") >= tcode.AxisLast("A0")) {
            suck = true;
            valveCmd = suckCmd;
        } else {
            suck = false;
        }
        // Set valve position
        if (suck) {
            if (upVel < -5) {
                valveCmd = 0;  
            } else if ( upVel < 0 ) {
                valveCmd = map(100*upVel, 0, -500, suckCmd, 0);
            }
        }
        valvePos = (9*valvePos + map(valveCmd, 0, 9999, 0, 1000))/10;

        // OSR2 Kinematics
        if (!SettingsHandler::sr6Mode) {
            // Calculate arm angles
            // Linear scale inputs to servo appropriate numbers
            int stroke,roll,pitch;
            stroke = map(xLin,0,9999,-350,350);
            roll   = map(yRot,0,9999,-180,180);
            pitch  = map(zRot,0,9999,-350,350);
            if(SettingsHandler::inverseStroke) 
            {
                ledcWrite(LowerLeftServo_PWM, map(SettingsHandler::LeftServo_ZERO - stroke + roll,0,MainServo_Int,0,65535));
                ledcWrite(LowerRightServo_PWM, map(SettingsHandler::RightServo_ZERO + stroke + roll,0,MainServo_Int,0,65535));
            }
            else
            {
                ledcWrite(LowerLeftServo_PWM, map(SettingsHandler::LeftServo_ZERO + stroke + roll,0,MainServo_Int,0,65535));
                ledcWrite(LowerRightServo_PWM, map(SettingsHandler::RightServo_ZERO - stroke + roll,0,MainServo_Int,0,65535));
            }
            
            if(SettingsHandler::inversePitch) 
            {
                ledcWrite(LeftPitchServo_PWM, map(SettingsHandler::PitchLeftServo_ZERO + pitch,0,PitchServo_Int,0,65535));

            }
            else
            {
                ledcWrite(LeftPitchServo_PWM, map(SettingsHandler::PitchLeftServo_ZERO - pitch,0,PitchServo_Int,0,65535));
            }
        }
        else 
        {
            // SR6 Kinematics
            // Calculate arm angles
            int roll,pitch,fwd,thrust,side;
            roll = map(yRot,0,9999,-3000,3000);
            pitch = map(zRot,0,9999,-2500,2500);
            fwd = map(yLin,0,9999,-3000,3000);
            thrust = map(xLin,0,9999,-6000,6000);
            side = map(zLin,0,9999,-3000,3000);

            // Main arms
            int lowerLeftValue,upperLeftValue,pitchLeftValue,pitchRightValue,upperRightValue,lowerRightValue;
            if(SettingsHandler::inverseStroke) 
            {
                lowerLeftValue = SetMainServo(16248 - fwd, 1500 - thrust - roll); // Lower left servo
                upperLeftValue = SetMainServo(16248 - fwd, 1500 + thrust + roll); // Upper left servo
                upperRightValue = SetMainServo(16248 - fwd, 1500 + thrust - roll); // Upper right servo
                lowerRightValue = SetMainServo(16248 - fwd, 1500 - thrust + roll); // Lower right servo
                pitchLeftValue = SetPitchServo(16248 - fwd, 4500 + thrust, -side + 1.5*roll, -pitch);
                pitchRightValue = SetPitchServo(16248 - fwd, 4500 + thrust, side - 1.5*roll, -pitch);
            } 
            else 
            {
                lowerLeftValue = SetMainServo(16248 - fwd, 1500 + thrust + roll); // Lower left servo
                upperLeftValue = SetMainServo(16248 - fwd, 1500 - thrust - roll); // Upper left servo
                upperRightValue = SetMainServo(16248 - fwd, 1500 - thrust + roll); // Upper right servo
                lowerRightValue = SetMainServo(16248 - fwd, 1500 + thrust - roll); // Lower right servo
                pitchLeftValue = SetPitchServo(16248 - fwd, 4500 - thrust, side - 1.5*roll, -pitch);
                pitchRightValue = SetPitchServo(16248 - fwd, 4500 - thrust, -side + 1.5*roll, -pitch);
            }
            // Set Servos
            ledcWrite(LowerLeftServo_PWM, map(SettingsHandler::LeftServo_ZERO - lowerLeftValue,0,MainServo_Int,0,65535));
            ledcWrite(UpperLeftServo_PWM, map(SettingsHandler::LeftUpperServo_ZERO + upperLeftValue,0,MainServo_Int,0,65535));
            ledcWrite(UpperRightServo_PWM, map(SettingsHandler::RightUpperServo_ZERO - upperRightValue,0,MainServo_Int,0,65535));
            ledcWrite(LowerRightServo_PWM, map(SettingsHandler::RightServo_ZERO + lowerRightValue,0,MainServo_Int,0,65535));
            ledcWrite(LeftPitchServo_PWM, map(constrain(SettingsHandler::PitchLeftServo_ZERO - pitchLeftValue, SettingsHandler::PitchLeftServo_ZERO - 600, SettingsHandler::PitchLeftServo_ZERO + 1000), 0, PitchServo_Int, 0, 65535));
            ledcWrite(RightPitchServo_PWM, map(constrain(SettingsHandler::PitchRightServo_ZERO + pitchRightValue, SettingsHandler::PitchRightServo_ZERO - 1000, SettingsHandler::PitchRightServo_ZERO + 600), 0, PitchServo_Int, 0, 65535));
        }

        // Twist and valve
        int twist,valve,squeeze;
        if (SettingsHandler::feedbackTwist && !SettingsHandler::continuousTwist) 
        {
            twist  = (xRot - map(twistPos,-1500,1500,9999,0))/5;
            if(!SettingsHandler::analogTwist) 
            { 
                twist  = constrain(twist, -750, 750);
            }
            else 
            {
                int jitter = 1;
                twist += jitter;
                jitter *= -1;
                twist = -constrain(twist, -500, 500);
                // if(twist != testVar2) {
                //     testVar2 = twist;
                //     Serial.print("twist: ");
                //     Serial.println(1500 + twist);
                //     Serial.print("map(twistPos,-1500,1500,9999,0) "); 
                //     Serial.println(map(twistPos,-1500,1500,9999,0));
                    // Serial.print("map "); 
                    // Serial.println(map(SettingsHandler::TwistServo_ZERO + twist,0,TwistServo_Int,0,65535));
                //}
            }
        } 
        else 
        {
            twist  = map(xRot,0,9999,1000,-1000);
        }
        squeeze = map(squeezeCmd,0,9999,1000,-1000);
        valve  = valvePos - 500;
        valve  = constrain(valve, -500, 500);
        if (SettingsHandler::inverseValve) { valve = -valve; }
        if(SettingsHandler::valveServo90Degrees)
        {
            if (SettingsHandler::inverseValve) { 
                valve = map(valve,0,500,-500,500);
            } 
            else
            {
                valve = map(valve,-500,0,-500,500);
            }
        }
        // Set Servos
        ledcWrite(TwistServo_PWM, map(SettingsHandler::TwistServo_ZERO + twist,0,TwistServo_Int,0,65535));
        ledcWrite(ValveServo_PWM, map(SettingsHandler::ValveServo_ZERO + valve,0,ValveServo_Int,0,65535));
        ledcWrite(SqueezeServo_PWM, map(SettingsHandler::SqueezeServo_ZERO + squeeze,0,SqueezeServo_Int,0,65535));

        // Done with servo channels

        // Output vibration channels
        // These should drive PWM pins connected to vibration motors via MOSFETs or H-bridges.
        if (vibe0 > 0 && vibe0 <= 9999) {
            ledcWrite(Vibe0_PWM, map(vibe0,1,9999,31,255));
        } else {
            ledcWrite(Vibe0_PWM, 0);
        }
        if (!SettingsHandler::lubeEnabled && vibe1 > 0 && vibe1 <= 9999) {
            ledcWrite(Vibe1_PWM, map(vibe1,1,9999,31,255));
        } else {
            ledcWrite(Vibe1_PWM, 0);
        }
        // Vibe timeout functions - shuts the vibne channels down if not commanded for a specified interval
        if (millis() - tcode.AxisLast("V0") > VIBE_TIMEOUT) { tcode.AxisInput("V0",0,'I',500); }
        if (!SettingsHandler::lubeEnabled && millis() - tcode.AxisLast("V1") > VIBE_TIMEOUT) { tcode.AxisInput("V1",0,'I',500); }
        
        // Done with vibration channels

        // Lube functions
        if (SettingsHandler::lubeEnabled) {
            if (lube > 0 && lube <= 9999) {
                ledcWrite(Vibe1_PWM, map(lube,1,9999,127,255));
            } else if (digitalRead(SettingsHandler::LubeButton_PIN) == HIGH) {
                ledcWrite(Vibe1_PWM,SettingsHandler::lubeAmount);
            } else { 
                ledcWrite(Vibe1_PWM,0);
            }
            if (millis() - tcode.AxisLast("A2") > 500) { tcode.AxisInput("A2",0,' ',0); } // Auto cutoff
        }
        // Done with lube
    }

private:
	const char* _TAG = "ServoHandler0_3";
    int MainServo_Int;
    int PitchServo_Int;
    int TwistServo_Int;
    int ValveServo_Int;
    int SqueezeServo_Int;

    int MainServo_Freq;
    int PitchServo_Freq;
    int TwistServo_Freq;
    int ValveServo_Freq;
    int SqueezeServo_Freq;

    // Declare classes
    // This uses the t-code object above
    TCode0_3 tcode;
    // Declare operating variables
    // Position variables
    int xLin,yLin,zLin;
    // Rotation variables
    int xRot,yRot,zRot;
    // Vibration variables
    int vibe0,vibe1,vibe2,vibe3;
    // Lube variables
    int lube;
    // Valve variables
    int valveCmd,suckCmd,squeezeCmd;
    // Velocity tracker variables, for valve
    int xLast;
    unsigned long tLast;
    float upVel,valvePos;
    float twistServoAngPos = 0.5;
    int twistTurns = 0;
    float twistPos;

    // Function to calculate the angle for the main arm servos
    // Inputs are target x,y coords of receiver pivot in 1/100 of a mm
    int SetMainServo(float x, float y) {
        x /= 100; y /= 100;          // Convert to mm
        float gamma = atan2(x,y);    // Angle of line from servo pivot to receiver pivot
        float csq = sq(x) + sq(y);   // Square of distance between servo pivot and receiver pivot
        float c = sqrt(csq);         // Distance between servo pivot and receiver pivot
        float beta = acos((csq - 28125)/(100*c));  // Angle between c-line and servo arm
        int out = ms_per_rad*(gamma + beta - 3.14159); // Servo signal output, from neutral
        return out;
    }


    // Function to calculate the angle for the pitcher arm servos
    // Inputs are target x,y,z coords of receiver upper pivot in 1/100 of a mm
    // Also pitch in 1/100 of a degree
    int SetPitchServo(float x, float y, float z, float pitch) {
        pitch *= 0.0001745; // Convert to radians
        x += 5500*sin(0.2618 + pitch);
        y -= 5500*cos(0.2618 + pitch);
        x /= 100; y /= 100; z /= 100;   // Convert to mm
        float bsq = 36250 - sq(75 + z); // Equivalent arm length
        float gamma = atan2(x,y);       // Angle of line from servo pivot to receiver pivot
        float csq = sq(x) + sq(y);      // Square of distance between servo pivot and receiver pivot
        float c = sqrt(csq);            // Distance between servo pivot and receiver pivot
        float beta = acos((csq + 5625 - bsq)/(150*c)); // Angle between c-line and servo arm
        int out = ms_per_rad*(gamma + beta - 3.14159); // Servo signal output, from neutral
        return out;
    }
};
