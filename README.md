# Requirements and Implementation Notes for the Primary Mirror Control System Microcontroller

## Introduction
The primary mirror control system consists of 3 mirror support actuators, and 3 home switches. The 3 motors are used to change the tip, tilt, and focus of the primary mirror. There is also a fan controller that controls the fans that blow on the back of the mirror to dissipate heat from the TECs. The Teensy 4.1 microcontroller is used to receive commands from the host computer, translate them into stepper motor commands, and step the motors to achieve the commanded positions. The controller will then remember the current position of each of the motors, to allow the system to rapidly recover after a power outage or ungraceful shutdown. The communications with the host computer are over the Ethernet. The hardware, command and data interfaces are described below.

## Hardware Interfaces:
### Power System:
The input power for the system comes from a fused 120V input port with a DIN C13 connector on it. This connector is compatible with standard computer and instrument power cables. The power is then fed into an EMI filter, and to the 12V power supply that supplies power for the 3 motors, the fans, and the computer module.

### Microcontroller:
The Teensy microcontroller is connected to a Teensy4.1 to Arduino Mega pinout adapter. This allows us to use an Arduino CNC V3.0 shield directly, without needing to design any hardware or circuit boards. The Arduino Mega pinout is backward compatible with the Uno’s pinout that the shield is based on. The adapter also contains CAN bus transceivers and a voltage regulator to supply power to the Teensy.

### Motor Drivers:
The 3 stepper motors are interfaced to the Teensy via a CNC V3.0 Arduino -compatible shield. The shield contains spots for 4 stepper driver modules, 3 of which are populated with Pololu DRV8825 stepper sticks. These are interfaced to the Arduino’s GPIO pins. Inputs for the limit switches, and terminals for connecting I2C and SPI peripherals are also provided by the CNC shield. There is an enable pin that can be used to turn the drivers off and on to minimize heat generation in the motors.

### Fans:
There is a PWM output from the Teensy that is routed to the fans’ PWM input. A status bit that comes back from the fans (which is open-drain) is routed to the Teensy for monitoring. It is currently envisioned that the fans will run at full speed all the time. So the only control function would be to monitor the fans to be sure they are running, and to command full PWM duty cycle upon startup. Or possibly on command from the host.

### CAN bus:
A CANbus interface is provided to connect all the subsystems up when the demo telescope turns into the full-up 20 mirror telescope. The CANbus is much cheaper and less power hungry than an Ethernet port. But for now, we don’t intend to use the CANbus ports for the system.

### Ethernet:
The Ethernet port will be used to command and monitor the system. The protocol for communications will be developed with the software providers of the host software that will control this module, but in the beginning a Python GUI based system will be used for debugging and testing. This can be similar to that in place for the thermometry multiplexer and the test system for the TEC control.

## Command and Control Interfaces
A series of requirements for the command and control interfaces has been developed verbally over the months. It is desired that manual control in the tip/tilt (X/Y) coordinate system be available, rather than trying to move the 3 motors in a coordinated fashion by hand to accomplish that. It is further desired that a “Joystick” mode be available, either by using the arrow keys on the keyboard, or an actual joystick controller. In addition to this manual mode, there should be automated routines that will send the mirror to its home position to find home position for each actuator, and a routine to send it to its last known correct position.

### Command Interface (TODO)
The command interface is implemented using the LFAST_Device comms library (github: TODO), and consists of the following commands:
| | |
| --- | --- |
| | |
| | |
| | |

### Stepper Motor Control

The CNC shield provides the hardware needed to control the stepper motors. The drivers are the step/direction type, with microstepping built in. The shield has jumpers to allow the microstepping level to be set. The DRV8825 has up to 1/32 microstepping built in. The step and direction pins for each axis are as follows:

| **Function** | **Pin** | |**Function** | **Pin** |
| --- | --- | ---  | --- | --- |
| X-Direction | 5 || Y-Step | 3 |
| X-Step | 2 || Y-Limit | 10 |
| X-Limit | 9 || Z-Direction | 7 |
| Y-Direction | 6 || Z-Step | 4 |
| Z-Limit | 11 || Enable | 8 |


Each motor should be managed using the AccelStepper() library. This library has the ability to perform coordinated motion, and can manage multiple motors simultaneously. See the MultiStepper example code. As the name implies, Acceleration limits can be used to manage the acceleration and eliminate missed steps due to too-fast acceleration towards high speeds. Another alternate library is the TeensyStep, ( https://luni64.github.io/TeensyStep/ )which is also an advanced library for controlling multiple motors simultaneously. However, it may not be available for the Teensy 4.1.

The code should support microstepping the motors. The microstepping should be able to handle all of the microstepping modes of the drivers.

The limit switches are used in this software to set the zero point of the stepper. This will need some testing to ensure repeatability of the switches. The idea is to drive the stepper slowly into the limit switch, then slowly back out of the limit until the switch releases, then record this as zero.

Derivations of the equations for determining the stepper positions from tip/tilt/focus commands are documented in docs/PrimaryMirrorMath.pdf in the github repository.

### Limit Switches
At this time it has been decided that adding limit switches to the primary mirror steppers is not necessary, as nothing can be damaged by bottoming out the motors. The homing routine drives the motors downwards and the operator presses a button on the GUI once all three have been bottomed out.

### Fan Control (In Work)
The fans are PC fans with a PWM input. The PWM frequency should be set to 25 kHz. The duty cycle will control the fan speed. The PWM output from the Arduino is connected to the SpinEnable pin on the Shield. The fan status bit is connected to SpinDir.
**Note:** Fan Control is in work because the fans have not been integrated in to the system yet.

### Software Structure
The software is interrupt-driven, to prevent the control and communication functionalities from interfering with eachother. The communication functions are executed from the primary application loop, and the stepper control functions occur in a timer interrupt at regular intervals. The interrupt checks if new instructions have been received and applies the appropriate commands to the motor accordingly.

### Test control GUI current capabilities:
1.  Collect the arguments for and send the commands defined above. 
2.  Display the current step positions
3.  Ability to nudge the mirror in any direction in both absolute and relative coordinates
4.  Have a STOP button that is prominent. Don’t call it E-STOP.
5.  Store the current stepper positions in FRAM

### Test control GUI planned capabilities:
1. Request and display current tip/tilt/focus positions calculated on the microcontroller from the current stepper positions. 
2.  Display the commanded fan speed and status
