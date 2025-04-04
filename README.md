# Interface-board-for-remote-control
The interface board allows the buttons of a remote control to be pressed by a microcontroller

I apologize in advance.  I am not a markdown language expert.  I will try to put libreoffice doc in the documents section

Blinds Controller
Abstract
A significant investment was made for automated cell blinds.  The provided automation hardware was inadequate to the task. The investment would be diminished if the blinds could not be automated.  This is a is a design that achieve adequacy.

Challenges

Tuya
Unfortunately, the supplied WiFi bridge (AC801 433MHz) was not compatible with my home controller, Home Assistant (HA).  The issues were:
Cloud based
The cloud company, Tuya, could easily control the blinds.
The WiFi bridge was not supported by Tuya
In the HA Community Store (HACS) is LocalTuya, I was never successful in controlling the blinds.  It required a connection to the Tuya cloud, but most communication is local.
The documentation is poor and poorly translated.
Radio reverse engineering
Various HA Forum members suggested using a Software Defined Radio(SDR).  This was not pursued because the lead time on the SDR was 1-2 months (RTL-SDR.com cautioned against knock-offs)

Instructables has a project Decoding and Sending 433MHz RF Codes With Arduino and Rc-switch.  Pressing a button on the remote control and looking at the signal out with an oscilloscope did not yield meaningful results.

Electronically pushing the buttons on the remote.
Not a satisfying solution but it was do-able.

The technique is to use a microcontroller, such as an Arduino Nano, to press the buttons of the remote control (AC2001-15A).  There are two challenges
The buttons are not simply connect an input to ground
There are 15 channels (word used in documentation) on the remote control.  The Arduino and remote control need to be synchronized.

Theory of Operation

Pushing a button

A multiplexing technique is used by the remote to determine which button is pressed.  In using this technique, neither side of the button is connected to + nor - of the battery.  A 4N25 opto-isolator is used to push a button of the remote control.  Since the output of the opto-isolator is polarized, essentially an NPN transistor (collect to emitter), one needs to know which side of the button to connect the collector.  This was easy.  I put a voltmeter across the button and was able to determine this; the collector goes to the positive voltage side.  Applying a current pulse to the LED of the opto-isolator causes the transistor of the opto-isolator to conduct which “presses” the button.

Synchronization

This got a little tricky.  There are 15 channels and the 00 channel (the function of which I don’t know).  So 00, 01, 02 03 04 05 06 07 08 09 10 11 12 13 14 15.  This is two 7 segment display
 _     a
| |   f b
 _     g
| |   e c
 -     d
Note that segments a, d, e, and f are off for 11 through 15 and on for 00 through 09.  This will be used in software to determine a transition, 0 to 1 or 1 to 0, while pressing the channel + button.  If the former transition, the channel is 00, if the latter, 10.

The display is made up of LEDs.  That’s exactly what the input to an opto-isolator is.  So I removed the segment e of the tens digit and I connected a regular LED to the two sides, not knowing which was which.   In the first way I connected the LED, I found that when the channel was 10-15, the LED was off and when 00-09 the LED was on.  Connecting the LED the other way was indeterminate.  The first way is the way to connect to the LED of the opto-isolator.  I determined which lead of my LED was the anode and that side of segment e would go to the anode of the opto-isolator (pin 1); the other side of segment e would connect to the cathode, (pin 2).  Ref multiplexing 7 segment display.

Hardware

Schematic
Material list

The only thing not explained above is the resistors.  By experimentation, it was found that a 47Ω resistor was needed to reduce the current output from the Arduino through the LED of the opto-isolator.  The Arduino specs say that the maximum output current of a pin is 40mA.  Without the resistor it would be 80.  Unfortunately it was found and confirmed by the specs, that to operate reliably, the opto–isolator LED requires 50mA, (specs imply 10mA, though tested with 50mA).  A compromise was found with 47Ω. (fingers crossed)  I used a 2X4 RA male connector plus a 1 male pin for the remote connector and a 2x4 female header for the Arduino

Remote disassembly and connection points

I found that wirewrap wire works very well and is secured with strategic drops of superglue.  A 2x4 female IDC connector was found to be effective. The 3V3 connection would be a wire with a female pin used to connect to the male pin on the adapter board.

Software

Setting up the Home Automation

The software which will run on an Arduino Nano (Nano) is in the MySensors environment.  The Nano will connect to a USB port of a PC. The PC OS will be Ubuntu linux.  Running under this OS will be Virtual Box with the extension pack.  Running in Virtual box will be HAOS.  

Once up and running, the Nano must be made available to the virtual machine, HAOs.  In the Oracle VirtualBox Manager, ensure that the virtual machine, HAOS is not running.  Click on USB.  On the right click the icon to add a USB device.  Select the device that is the Nano that is plugged into the machine. Star the virtual machine.

Install the MySensors integration.  The serial port will probably be /dev/ttyUSB0.  The MySensors version should be 2.3.  The persistence file can be left blank but I prefer to give it a name: mysensors_blind_controller.json.

Arduino Nano code

MySensors
MySensors provides all the communication between the Nano and HA. MySensors is sufficiently supported and those portions of the code will not be documented here.  Similarly, Arduino is very well supported.  Please review the volatile qualifier.

The process

Synchronize the channels

As noted above, the channel of the Nano and the remote needs to be synchronized.
The current channel is given the arbitrary value of zero
The remote must be awake to read the state of the segment e LED of the 10’s digit  of the remote. The STOP button is pushed to ensure that the remote is awake.
Pushing a button
The I/O line for the button to be pushed is raised which will, through the opto-isolator, cause the contacts of the button to close.
The DWELL time is waited (simulating human response times)
The I/O line for the button to be pushed is lowered, which opens the contacts
Note time at which the button is pressed (this is to predict the timeout of the remote display and  will be used in conjunction with changing the channel.)
The DWELL time is waited (simulating human response times)
The current segment e state is read
This is a multiplexed output, that is, it turns on and off faster than the eye can see.  If at any time in a 12 millisecond period, checking every 3 milliseconds, the input goes low (which means the segment turned on), the channel is set to 10, 11, 12, 13, 14, or 15.  That is, the channel is not less than 10 (the channel is greater than 9)
Segment e is read again and compared to the “current state”.  If they are the same, the channel + button is pushed.  This is repeated until the just-read-state is different from the “current state”.
If the “current state” was less than 10, the channel is 10; if it was not, then channel is 0.

Send HA the initial values to HA

This is required by HA.  This is how HA knows that these entities (Channel+, DOWN, STOP, and UP exists (“entitiy” is an HA term)

Wait for a command from HA

Each sensor has a state of on or off.  In the quiescent state they are all off.  HA will change this state and the state change is sent to the Nano.  The following will be continually executed:
Step through each sensor, UP, STOP, DOWN, CHANNEL+
If the sensor being checked’s state is on:
Set the sensor state back to off
Push the button associated with the sensor
Send HA an acknowledgement by sending the state (off) of the sensor.
Channel+ is a little different.  It also has a state which is data-received (on) and data sent (off).  The data it actually received is the channel to which the remote is to be set.  Instead of “Push the button” the channel is changed to that value.  Also,if any button has been pushed within the last 4 seconds, the remote is awake
If the remote is not awake, press the channel up button.  This wakes up the remote but does not change its channel.
Repeat the following steps until the channel is equal to the requested channel
Push the channel plus button
Increment the channel
If the channel is greater than 15 set the channel to 0

Receiving data from HA

When a message arrives:
If the message has come from HA (this is probably not necessary)
Get the sensor for which the data is destined
If the sensor is not for channel
Set the sensor received flat to the value sent
If the sensor is for channel
Set the incoming channel to the value coming in
If the value is different from the current channel and valid, set the sensor received flag to true

