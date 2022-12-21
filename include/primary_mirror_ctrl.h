/*******************************************************************************
Copyright 2022
Steward Observatory Engineering & Technical Services, University of Arizona
This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*******************************************************************************/

/**
@brief LFAST prototype Primary Mirror Control Interface function definitions
@author Nestor Garcia
@date October 17, 2022
@file primary_mirror_ctrl.h

Definitions of Primary Mirror Control functions
*/

/*
MoveAbsolute(V, X,Y) – Move each axis with velocity V to an absolute X,Y position with respect to “home”
MoveRelative(V, X,Y) – Move each axis with velocity V X,Y units from the current position In the above commands,
                       V, X and Y are vectors of length 3. Velocity is in units of radians per second, X,Y are milliradians.
MoveRawAbsolute(V, X,Y) – Move each axis with velocity V to an absolute X,Y position with respect to “home”
MoveRawRelative(V, X,Y) – Move each axis with velocity V X,Y units from the current position In the above commands,
                          V, X and Y are vectors of length 3. Velocity is in units of steps per second, X,Y are steps.
Home(V) – Move all actuators to home positions at velocity V
FanSpeed(S) – Set the fan speed to a percentage S of full scale
GetStatus() – Returns the status bits for each axis of motion. Bits are Faulted, Home and Moving
GetPositions() – Returns 3 step counts
Stop() – Immediately stops all motion
*/

#ifndef PRIMARY_MIRROR_CONTROL_H
#define PRIMARY_MIRROR_CONTROL_H

#include <Arduino.h>
#include <iostream>
#include <LFAST_Device.h>
#include <TerminalInterface.h>
#include <cmath>

#include <MultiStepper.h>
// Setup functions

#define ENABLE_STEPPER LOW
#define DISABLE_STEPPER HIGH
// void connectTerminalInterface(TerminalInterface* _cli);
constexpr double MICROSTEP_DIVIDER = 16;
constexpr double MICROSTEP_RATIO = 1.0/MICROSTEP_DIVIDER;
// PMC Command Processing functions
#define MIRROR_RADIUS_MICRONS 281880 // Radius of mirror actuator positions in um
constexpr double MICRON_PER_STEP = 3 * MICROSTEP_RATIO;            // conversion factor of stepper motor steps to vertical movement in um
constexpr double STEPS_PER_MICRON = 1.0/MICRON_PER_STEP;
// constexpr double MM_PER_STEP = (MICRON_PER_STEP/1000);
// constexpr double STEP_PER_MM = 1.0 / MM_PER_STEP;

#define MIRROR_MATH_COEFFS   \
    {                        \
        281.3, -140.6, 243.6 \
    }

// PM Control functions
enum PRIMARY_MIRROR_ROWS
{
    BLANK_ROW_0,
    CMD_MODE_ROW,
    TIP_ROW,
    TILT_ROW,
    FOCUS_ROW,
    BLANK_ROW_1,
    STEPPERS_ENABLED,
    STEPPER_A_FB,
    STEPPER_B_FB,
    STEPPER_C_FB,
    MOVE_SM_STATE_ROW,
};

namespace LFAST
{
    namespace PMC
    {
        enum ControlMode
        {
            STOP = 0,
            RELATIVE = 1,
            ABSOLUTE = 2,
        };

        enum UNIT_TYPES
        {
            ENGINEERING = 0,
            STEPS_PER_SEC = 1
        };

        enum AXIS
        {
            TIP = 0,
            TILT = 1,
            FOCUS = 2
        };

        enum DIRECTION
        {
            REVERSE = -1,
            FORWARD = 1
        };

        enum MOTOR_ID
        {
            MOTOR_A = 0,
            MOTOR_B = 1,
            MOTOR_C = 2
        };

    }
};

class MirrorStates
{
private:
    const double c[3] = MIRROR_MATH_COEFFS; // = ; // Coefficients calculated based on  motor positions

public:
    MirrorStates &operator=(MirrorStates const &other)
    {
        noInterrupts();
        TIP_POS_ENG = other.TIP_POS_ENG;
        TILT_POS_ENG = other.TILT_POS_ENG;
        FOCUS_POS_ENG = other.FOCUS_POS_ENG;
        interrupts();
        return *this;
    }

    volatile double TIP_POS_ENG;
    volatile double TILT_POS_ENG;
    volatile double FOCUS_POS_ENG;

    template <typename T>
    void getMotorPosnCommands(T *a_steps, T *b_steps, T *c_steps) const
    {
        double tanAlpha = std::tan(TIP_POS_ENG);
        double cosAlpha = std::cos(TIP_POS_ENG);
        double tanBeta = std::tan(TILT_POS_ENG);
        double gamma = this->FOCUS_POS_ENG;

        double a_distance = gamma + (c[0] * tanAlpha);
        double b_distance = gamma + (c[1] * tanAlpha + c[2] * tanBeta / cosAlpha);
        double c_distance = gamma + (c[1] * tanAlpha - c[2] * tanBeta / cosAlpha);


        *a_steps = (T)(a_distance * STEPS_PER_MICRON);
        *b_steps = (T)(b_distance * STEPS_PER_MICRON);
        *c_steps = (T)(c_distance * STEPS_PER_MICRON);
    }
    void reset()
    {
        TIP_POS_ENG = 0;
        TILT_POS_ENG = 0;
        FOCUS_POS_ENG = 0;
    }
};

// void updateControlLoop_ISR();

class PrimaryMirrorControl : public LFAST_Device
{
public:
    static PrimaryMirrorControl &getMirrorController();

    virtual ~PrimaryMirrorControl() {}
    void setupPersistentFields() override;
    void updateStatusFields();
    void updateCommandFields();
    void updateFeedbackFields();
    void pingMirrorControlStateMachine();
    void copyShadowToActive();
    void setControlMode(uint8_t moveType);
    void setFanSpeed(unsigned int PWR);
    void setTipTarget(double tgt);
    void setTiltTarget(double tgt);
    void setFocusTarget(double tgt);
    void goHome(volatile double homeSpeed);
    void stopNow();
    bool getStatus(uint8_t motor);
    double getStepperPosition(uint8_t motor);

    void saveStepperPositionsToEeprom();
    void resetPositionsInEeprom();
    void loadCurrentPositionsFromEeprom();
    void enableControlInterrupt();
    void setMoveNotifierFlag(volatile bool *flagPtr);
    void setHomingCompleteNotifierFlag(volatile bool *flagPtr);
    bool checkForNewCommand();
    bool isHomingInProgress();

    void limitSwitchHandler(uint16_t axis);
    void enableSteppers(bool doEnable);
    bool isEnabled() {return steppersEnabled;}
private:
    PrimaryMirrorControl();
    void hardware_setup();
    void enableLimitSwitchInterrupts();
    void updateStepperCommands();
    bool pingSteppers();
    bool pingHomingRoutine();
    MultiStepper *stepperControl;
    MirrorStates CommandStates_Eng;
    MirrorStates ShadowCommandStates_Eng;
    uint8_t controlMode;

    bool steppersEnabled;
    bool focusUpdated;
    bool tipUpdated;
    bool tiltUpdated;
    int32_t A_cmdSteps;
    int32_t B_cmdSteps;
    int32_t C_cmdSteps;
    bool limitFound_A;
    bool limitFound_B;
    bool limitFound_C;
    double homingSpeedStepsPerSec;

    static void limitSwitch_A_ISR();
    static void limitSwitch_B_ISR();
    static void limitSwitch_C_ISR();

    typedef enum
    {
        IDLE = 0,
        NEW_MOVE_CMD = 1,
        MOVE_IN_PROGRESS = 2,
        MOVE_COMPLETE = 3,
        LIMIT_SW_DETECT = 4,
        HOMING_IS_ACTIVE = 5,
    } MOVE_STATE;
    MOVE_STATE currentMoveState;

    typedef enum
    {
        INITIALIZE,
        HOMING_STEP_1,
        HOMING_STEP_2,
        HOMING_STEP_3,
        HOMING_STEP_4,
        HOMING_STEP_5
    } HOMING_STATE;
    HOMING_STATE currentHomingState;

    volatile bool *moveNotifierFlagPtr;
    volatile bool *homeNotifierFlagPtr;
};

#endif
