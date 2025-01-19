## BACCAble 

## Scope
This project uses the famous CANABLE (the cheapest can bus device on the market) in order to:
- sniff on the can bus (useful for debug and exploit purposes)
- decode and store some parameters sniffed on the bus (like motor rpm, accelerator pedal position and gear selection)
- control a WS281x leds strip by means of the decoded can bus data, then lighting the leds strip according to accelerator pedal position and gear selection.
- automatically disable start&stop car functionality
- act as Immobilizer, by injecting can bus messages when required.
- show SHIFT warning indicator on dashboard when configurable motor rpm speed is overcomed
- enable and disable ESC and TC with left stalk button press
- add a menu to dashboard in order to show additional parameters like dpf occlusion percentage (under development) 

BACCABLE overview (click on the following image to see the video) (note: video not updated. do not includes all the functionalities added to the device last months)
[![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/HStaXDe9asQ/0.jpg)](https://www.youtube.com/watch?v=HStaXDe9asQ)

Link to youtube videos playlist:
https://www.youtube.com/playlist?list=PLBaS0780TbwKpBBER44QJkiz-0hAlga8X

## General Description
I started the development from the famous SLCAN firmware (https://github.com/normaldotcom/canable-fw), by porting it inside stm32Cube environment (I updated usb interface), then I added: 
- message decoding (watch the following video)

[![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/v0ZhjYjx-OM/0.jpg)](https://www.youtube.com/watch?v=v0ZhjYjx-OM)

- leds Controlling function  (watch the following video)
  NOTE: video was not updated after firmware optimization. Now you have to uncomment the define LED_STRIP_CONTROLLER_ENABLED in order to use the led controlling functionality
  
[![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/QDK8YCOsdVM/0.jpg)](https://www.youtube.com/watch?v=QDK8YCOsdVM)

- start&stop car function disabler (more detailes in the dedicated subparagraph),
- immobilizer (more details in the dedicated subparagraph)
- SHIFT warning indicator on dashboard (more detailes in the dedicated subparagraph),
- ESC and TC controller (more detailes in the dedicated subparagraph)

## Folders content
- Subfolder firmware contains the firmware
- Subfolder hardware/canable contains canable board layout and pcb wiring diagram. It comes from https://github.com/makerbase-mks/CANable-MKS. There are different designs of canable, but theay are all similar.
- Subfolder hardware/box contains the 3d model of the case to accomodate required components.
- Subfolder hardware/system interconnection contains interconnection diagram to connect required components
- Subfolder tools contains the famous savvyCan sniffer tool for windows (portable) and excel sheet used to calculate pwm and clocks settings.
## Start&Stop car functionality Disabler
The functionality "car start&stop disabler" is implemented by simply shorting a gpio to ground trough a resistor (if the motor is rotating), in order to simulate Start&Stop button press on the car panel, with a delay after the device was switched on. I avoid to do anything if a specific can bus message tells me that the the Start&Stop was still manually disabled by the pilot. The used resistor is suitable for my car. 
This projet was tested on alfaromeo Giulia. Each one of you, if dealing with other car, different than Alfaromeo Giulia/Stelvio,  should:
- perform some checks on the panel with a multimeter, in order to find the proper resistor value for the start&stop button. 
## Immobilizer functionality
The functionality IMMOBILIZER performs the following:
1. Detects if the thief is trying to connect to to RFHUB (they do it to add a key to the car)
2. Starts the Panic Alarm after one second
3. Continuously Resets the RFHUB in order to reset the thief connection, with this message for 10 seconds
4. after 10 seconds stops to send messages and stops alarm, and return listening for thief messages

Note1: Panic alarm will start only if you previusly enabled panic alarm in your ECU, with the MES proxy alignment procedure shown in this video: [![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/dHC6A2Jsalo/0.jpg)](https://www.youtube.com/watch?v=dHC6A2Jsalo)
Note2: The Immobilizer functionality will not detect the thief if you power the BACCAble with a voltage available only when the panel is switched on. Therefore, if you use immobilizer function, you shall remove the voltage regulator that I use to convert the 12V to 5V and directly plug the canable to the 5V usb voltage taken from the connector of the USB interface in the central area, close to cigarette lighter socket. In fact, usb voltage is switched on as soon as the thief wakes up the rfhub. 

Note3: Once we start to send the rfhub reset message, neither the injition button will work. the car will appear as dead..

## Leds Strip controller
The leds strip is lighted accordingly to the movement of the accelerator pedal and the gear selection. 

This projet was tested on alfaromeo Giulia. Each one of you, if dealing with other car, should:
- identify proprietary can bus messages, by sniffing data with savvycan.

## Shift Warning Indicator
The SHIFT warning indicator function allows you to show on dashboard (only if you are in race mode) the SHIFT warning label when configurable motor rpm speed is overcomed (3 levels of warning).
The following video explains the behavious and the code description: [![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/JYUBBTW4WRo/0.jpg)](https://www.youtube.com/watch?v=JYUBBTW4WRo)

## ESC & TC enabler/disabler functionality
By pressing left stalk button (LANE indicator) for 2 seconds, in D,N,A modes the ESC and TC will be disabled. Changing DNA mode or pressing again the same button, it is possible to revert the change. In Race mode, where ESC and TC ar tipicalliy disabled, this functionality allows to enable ESC and TC.
The following video explains the behavious and the code description: [![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/I1GHFjOmpOs/0.jpg)](https://www.youtube.com/watch?v=I1GHFjOmpOs)

The following video shows tests performed on this functionality on the road: [![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/hMO_yby04wI/0.jpg)](https://www.youtube.com/watch?v=hMO_yby04wI)

## DASHBOARD MENU functionality
Adds a menu to the dashboard allowing the user to show additional parameters. the menu pages are changed with buttons on the steering wheel.
See following video for a preview of the functionality, realized during first development phase.
This first implementation is on BH can bus both for write on IPC and to detect wheel button press.
[![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/-_93Q_ZlYxc/0.jpg)](https://www.youtube.com/watch?v=-_93Q_ZlYxc)


## Usage Instructions
You should perform some preliminary settings inside firmware:
- If you want to use the device as usb can bus sniffer you shall uncomment #define ACT_AS_CANABLE in main.h (better if you comment the other functionalities defines to reduce computational charge)
- If you want to use the device as leds strip controller you shall comment the line " #define ACT_AS_CANABLE " and uncomment the line " #define LED_STRIP_CONTROLLER_ENABLED " in main.h (this was tested only connected to C1 can bus)
- If you don't want to use the piece of code that disables the car start&stop at the power on, you shall comment #define DISABLE_START_STOP in main.h (this works only if connected to C1 can bus because I check motor speed. If someone wants it on another can bus, he shall remove the motor speed check)
- If you want to disable IMMOBILIZER functionality, you shall comment #define IMMOBILIZER_ENABLED in main.h (this works only if connected to C1 can bus)
- In vumeter.c you shall set the number of leds in your leds strip, by modifing the following line: #define MAX_LED 46
- If you want to use SHIFT WARNING INDICATOR functionality, you shall uncomment #define SHIFT_INDICATOR_ENABLED in main.h and set the define SHIFT_THRESHOLD to the rpm speed at which the indicator will start to be shown (2000rpm by default) (works only in race mode, and was tested only connected to C1 can bus)
- If  you want the capability to enable and disable TC with left stalk button, you shall uncomment #define ESC_TC_CUSTOMIZATOR_ENABLED in main.h and connect the baccable to C2 can bus (pin 12 and 13 of the OBD port)
- If you want to add menu on the dashboard to display additional parameters, you shall uncomment #define SHOW_PARAMS_ON_DASHBOARD in main.h, and connect Baccable to BH can bus (in example on OBD port, pins 3 and 11)
![Interconnections](https://github.com/gaucho1978/CANableAndLedsStripController/blob/master/hardware/system_interconnection/IMG_3511.jpeg)

Note: compile the code in Release version and not debug since Release version is much more light (smaller elf file size). Some of BACCABLE videos shows how to compile. 

Software to use:
- use stm32CubeIde to compile on windows
- use stm32CubeProgrammer to flash the firmware elf file contained in subfolder firmware\ledsStripController\Release 

Note: i downloaded previous version of the programmer (v.2.15.0) since last available revision had some bug that won't allow me to flash canable. Edit: now also last version can be used.

Flash procedure:
- press reset button on the canable, then connect usb to pc (the canable will be detected as serial device named "stm32 bootloader"
- use stm32CubeProgrammer to flash the device

## The hardware
click on the following image to see the full hardware and interconnections video:
[![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/-PcnPGzh-L0/0.jpg)](https://www.youtube.com/watch?v=-PcnPGzh-L0)

Used hardware:

Canable: https://a.aliexpress.com/_Ev1yBz1
Leds Strip ws2811 ip65: https://www.ebay.it/itm/325563557492?mkcid=16&mkevt=1&mkrid=711-127632-2357-0&ssspo=wTLp3UyoQGK&sssrc=4429486&ssuid=zXyeQJ2cSnu&var=514593107226&widget_ver=artemis&media=COPY
Amazon alternatives:
Canable: https://amzn.to/3zzeNMq
Leds strip: https://amzn.to/3W3TifJ

Note: use recommended canable links cause some of them uses different st chip and I'm not sure if other chips are supported.

## The interconnections
Note: In "Usage Instructions" section it is defined when you need to connect to a different can bus. The folliwing diagram shows the connection to C1 can bus (pin 6 and 14 of the OBD port), commonly used for immobilizer and leds strip controller functionalitites, but there are also C2 can bus (pin 12 and 13 of the OBD port) required in example for ESC&TC disabler functionality and BH can bus (pin 3 and 11 of the OBD port) for the future functionality to add parameters on the dashboard. 
![Interconnections](https://github.com/gaucho1978/CANableAndLedsStripController/blob/master/hardware/system_interconnection/SCHEMA_DI_INTERCONNESSIONE.png)
Note: if you use immobilizer function, you shall remove the voltage regulator that I use to convert the 12V to 5V and directly plug the CANABLE to the  5V usb voltage, taken from the connector of the USB interface in the central area, close to cigarette lighter socket.
## The Box
![Box](https://github.com/gaucho1978/CANableAndLedsStripController/blob/master/hardware/box/box.png)
![Cap](https://github.com/gaucho1978/CANableAndLedsStripController/blob/master/hardware/box/cap.png)

## Installation 
watch the following video to see installation procedure.
note: the video doesn't show the connection from usb +5V required to use immobilizer function.

[![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/aylwa35GtuU/0.jpg)](https://www.youtube.com/watch?v=aylwa35GtuU)

## Firmware description
The following video will show the structure of the firmware.
[![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/zmMgXUu2TZM/0.jpg)](https://www.youtube.com/watch?v=zmMgXUu2TZM)
Note: the video was made before to update the method to send rfhub reset message for 10 seconds, and before of the code optimization in the main loop, and before latest functionalities were added.

## Usage when configured to act as Canable
when configured as canable the firmware acts as the classic SLCAN firmware. it means that you can use it with a pc equipped with savvycan tool, in order to sniff packets in the canbus. 
With such configuration the device is seen by the pc as a virtual serial port implementing the following serial commands:

- O - Open channel
- C - Close channel
- S0 - Set bitrate to 10k
- S1 - Set bitrate to 20k
- S2 - Set bitrate to 50k
- S3 - Set bitrate to 100k
- S4 - Set bitrate to 125k
- S5 - Set bitrate to 250k
- S6 - Set bitrate to 500k
- S7 - Set bitrate to 750k
- S8 - Set bitrate to 1M
- M0 - Set mode to normal mode (default)
- M1 - Set mode to silent mode
- A0 - Disable automatic retransmission
- A1 - Enable automatic retransmission (default)
- TIIIIIIIILDD... - Transmit data frame (Extended ID) [ID, length, data]
- tIIILDD... - Transmit data frame (Standard ID) [ID, length, data]
- RIIIIIIIIL - Transmit remote frame (Extended ID) [ID, length]
- rIIIL - Transmit remote frame (Standard ID) [ID, length]
- V - Returns firmware version and remote path as a string
Note: Channel configuration commands must be sent before opening the channel. The channel must be opened before transmitting frames.

This firmware currently does not provide any ACK/NACK feedback for serial commands.

## Understanding LED protocol

The approach used to control WS281x leds strip controller was derived from this: https://github.com/MaJerle/stm32-ws2811-ws2812-ws2812b-ws281x-tim-pwm-dma-timer where it is used a timer to start a pwm, then DMA allows a fast change of the duty cycle of the pwm.

Summarizing, the ws281x uses a control signal where each bit is transmitted as 1 or 0 with a pwm signal (with 2 different duty cycle for 0 and for 1 logic levels).
The ws281x protocol expects a 24 bits sequence (3x8) for each led, where each 8 bits defines a color (red, green and blue). 
First led will get the first 24 bits, then it sends the rest to the next led. each led does the same.
A pause in the transmission determines the end of the frame, then a new frame can be sent.
The protocol and the timings are described in the ws281x datasheet

WS2811 and WS2812 protocol is specific one and has defined values:

- Transfer rate is `800 kHz`, or `1.25us` pulse length for each bit
- Transfer length is `24` pulses for each led, that's `30us` for one LED
- Each logical bit (`1` or `0`) consists of high and low part, with different length
- Reset pulse is needed prior updating led strip, to synchronize sequence

![WS2811 & WS2812 & WS2812B LED protocol](https://raw.githubusercontent.com/MaJerle/stm32-ws2812b-tim-pwm-dma/master/docs/ws-protocol.svg?sanitize=true)

> Minimum reset pulse length depends on WS281x device. Check datasheet for your particular unit. WS2812B says `> 50us`, while WS2811 says `> 280us`.

## STM32 DMA

DMA controllers in STM32s support various operations, one of them being super handy for our WS LED driver, called *circular operation mode*.
*Circular mode* will continuously transmit data from memory to peripheral (or, in general, can also go opposite direction) and periodically send *transfer-complete* or *half-transfer-complete* interrupts to the application.

![STM32 DMA circular mode](https://raw.githubusercontent.com/MaJerle/stm32-ws2812b-tim-pwm-dma/master/docs/stm32-dma-circular.svg?sanitize=true)

We will use *HT* and *TC* events extensively, as they will be use to *prepare data* for next operations to transfer all bits for all leds.

## Alfa Romeo Giulia Protocol Reverse Engineering 

These areinformation that I found. Use everything this at your own risk.
      
1. msg id 0x90  prints text on dashboard (on BH can bus) and contains:
    - total frame number is on byte 0 from bit 7 to 3
    - frame number is on byte 0 from bit 2 to 0 and byte1 from bit7 to 6
    - infoCode is on byte1 from bit 5 to 0 (0x12=phone connected, 0x13=phone disconnected, 0x15=call in progress, 0x17=call in wait, 0x18=call terminated, 0x11=clear display, ...)
    - /UTF text 1 is on byte 2 and byte 3
    - UTF text 2 is on byte 4 and byte 5
    - UTF text 3 is on byte 6 and byte 7

2. msg id 0x0ed  (Thanks to SniZ - https://alfatuning.app ) contains: 
   - shift warning lamp directed to dashboard in byte6 , bit 1 and 0(lsb) -  Value 0= no indicator, 1=urgency level1, 2=urgency level2, 3=urgency level3 (the one with shift label)
   - EngineWaterTemperature is on byte0
   - fuel consumption is on byte 4 bit 0 to 3, byte 5, and byte 6 from bit 7 to 3
   
3. msg id 0xFC (Thanks to SniZ - a famous guru - https://alfatuning.app ) contains: 
   - motor rpm speed is in byte 0 and 1 (the least significant 2 bits of byte 1 are not related to rpm speed, and should not be used)
   - engine speed fail is on byte1 bit 1
   - engine StopStart Status is on byte 1 bit 0 and byte 2 bit 7
   - engine Status is on byte 2 bit 6 and bit 5.
   - gas pedal position is on byte 2 from bit 4 to 0 and byte 3 from bit 7 to 5.
   - gas pedal position fail is on byte3 bit 4.
   - .....
   - alternator fail is on byte 3, bit1.
   - stopStart status is on byte3 bit 0 and byte4 bit7.
   - CC brake intervention request is on byte 4, bit5
   - bank deactivation status is on byte5, bit 7 and 6
   - CC brake intervention is on byte 5 from bit 5 to 0 and byte 6 from bit 7 to 4.

4. msg id 0x1F0 contains:
    - clutch interlock is on byte 0 bit 7
    - clutch upstop is on byte0 bit 6
    - actual pedal position is on byte0 from bit 4 to 0 and byte 1 from bit7 to 5
    - analog cluch is on byte 1 from bit 4 to 0 and byte 2 from bit 7 to 5.

5. msg id 0x1FC is received on C2 can bus and it contains:
    - Rear Diff. Warning La. is on byte0 bit7
    - Rear Diff, Control Status is on byte0 bit6 (it is 0 with car engine off and 1 with car engine on)
    - Active Dumping Control Status (the suspensions) is on byte0 from bit 5 to 4 (0x0=Mid, 0x1=Soft, 0x2=Firm [only on QV])
    - Rd. Asp. Ind. is on byte 0 from bit3 to 0 and byte 1 from bit 7 to 4
    - Active Dumping Control Fail status is on byte 1 bit3
    - Aero. Fail Status is on byte 1 bit2
    - Front Aero. status is on byte 1 bit1 to bit0
    - CDCM warning lamp is on byte 2 bit5

6. msg id 0x226:
    - status of start & stop lamp (or function) is on byte 2 (0xF1= S&S lamp off, 0x05= S&S lamp on ). Lamp on means Start&Stop disabled
   
7. msg id 0x2EE contains the following radio buttons on the steering wheel - only available on BH can bus at 125kbps
    - radio right button is on byte 3 bit6 (1=button pressed)
    - radio left button on the steering wheel is on byte 3 bit4 (1=button pressed)
    - radio Voice command button is on byte 3 bit2 (1= button pressed)
    - phone call button is on byte3 bit0(1=button pressed)
    - volume  is on byte 4 (volume up increases the value, volume down reduces the value. once arrived to 255 restarts from 0 and under 0 goes to 255)
    - volume change is on byte5 bit 7 and bit6 (1=volume was increased rotation, 2=volume decreased rotation, 3=volume mute button press) (then reading the entire byte we will see respectively, 0x40, 0x80,  0xC0)

8. msg id 0x2EF contains:
   - actual gear status is on byte 0 from bit 7 to 4 (0x0=neutral, 0x1 to 0x6=gear 1 to 6, 0x07=reverse gear, 0x8 to 0xA=gear 7 to 9, 0xF=SNA)
   - suggested gear status is on byte 0 from bit 3 to 0 (decoded as actual gear status field)
   - DPF Regeneration mode is on byte 1 bit 7
   - SAM info is on byte 1 from bit 3 to 0
   - stop start fault status is on byte 2 bit 7
   - ..
   - boost pressure indication is on byte 3 bit from 6 to 0 and byte 4  bit 7

9. msg id 0x2FA contains, in byte0, the Button pressed on left area of the wheel - These Buttons are detected only if the main panel of the car is on. (WARNING1: when you press cruise control strong up, before and after it, also cruise control gently up message is fired) Possible values are:
   - 0x90=RES,
   - 0x10=Buttons released
   - 0x12=Cruise control on/off,
   - 0x08=Cruise control speed gently up,
   - 0x00=Cruise control speed strong up,
   - 0x18=Cruise control speed gently down
   - 0x20=Cruise control speed strong down

   
10. msg id 0x384 (periodically sent on the bus when ecu is on - example of this message with dna selection dynamic: t38480809DA080004XXYY where XX= counter from 00 to 0F and YY=checksum):
   - Command Ignition Status is on byte0 from bit 3 to 1.
   - Command Ignition Fail Status is on byte 0 bit0 and in byte1 bit7.
   - Drive Style Status (RDNA mode) is on byte 1 from bit 6 to bit 2 (0x0=Natural, 0x2=dynamic, 0x4=AllWeather, 0xC=race)
   - External temperature is on byte 1 from bit 1 to 0 and on byte 2 from bit 7 to bit 1.
   - External temperature fail is on byte2 bit0
   - Low Beam Status is on byte3 bit7
   - Lane Indicator button status (left stalk button) is on byte 3 bit6. (populated only on C2 can bus) (1 when pressed)
   - Power Mode Status is on byte 3 from bit 5 to 4.
   - Park Brake Status is on byte 3 bit 3.
   - Int. Relay Fail Status is on byte 4 from bit 7 to 6
   - SuspensionLevel is on byte 5 bit0 and byte 6 bit7.

11. msg id 0x4B2 contains:
    - engine oil level is in byte 0 from bit 7 to 3.
    - engine oil over fill status is on byte 0, bit 2.
    - engine oil min. is on byte 0 bit 1
    - engine oil pressure is on byte 0, bit 0 and on byte 1 from bit 7 to 1.
    - power mode status is on byte 1 bit 0 and on byte 2 bit 7.
    - engine water level is on byte 2 bit 6.
    - engine oil temperature is on byte 2 from bit 5 to 0 and on byte 3 from bit 7 to 6.
    - engine oil temperature warning light is on byte 3 bit 5.
     
12. Thanks to SniZ ( https://alfatuning.app ) , and to alfaobd developer, this is RFHUB Reset message. To make it work, this message shall be periodically sent (each 200msec should be ok, but i decided to send it each 10msec):
   - T18DAC7F180211010000000000 

13. These messages changes when you move accelerator pedal:
   - message id 0ff, second and third byte, changes from 1D33 to 39f3
   - message id 1f0, first 3 nibble changes from 000 to 1f2
   - message id 412 , fourth byte, changes from 33 to E6. I use this one!!
   - message id 736, second and third byte, changes from 3319 to E772

14. The following message sequence starts (and stops) car Alarm, but it works only if the bus is not flooden with other messages:
   - T1E340041488201500  //this message it is like a wake up sequence
   - T1E340041488201500
   - T1E340041488201500
   - T1E340041488201500
   - T1E340041488201500
   - T1E340041488201500
   - T1E340041488201500
   - T1E340041488201500
   - T1E340041488201500
   - T1E340041488201500
   - T1E340041488201500
   - T1E340041488201500
   - T1E340041488201500
   - T1E340041488201500
   - T1E340041488201500
   - t1EF84202E20000000156 At this point of the sequence, on my car,  the main panel temporary resets, if it is on, and starts the panic alarm. If the alarm was on, it goes off.

15. Experiments: 
   - t2EE47FE00000 This on my car, if the panel is on, temporary resets main panel ad you can ear relays switch sound
   - t0FA8A0200000200400F1 this on my car, if the panel is on, generates animation on the panel like switch off and on
