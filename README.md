## BACCAble 

## Scope
This project uses the famous CANABLE (the cheapest can bus device on the market) in order to:
- sniff on the can bus (useful for debug and exploit purposes)
- decode and store some parameters sniffed on the bus (like motor rpm, accelerator pedal position and gear selection)
- control a WS281x leds strip by means of the decoded can bus data, then lighting the leds strip according to accelerator pedal position and gear selection.
- automatically disable start&stop car functionality
- act as a can bus Immobilizer.
## General Description
I started the development from the famous SLCAN firmware (https://github.com/normaldotcom/canable-fw), by porting it inside stm32Cube environment (I updated usb interface), then I added: 
- message decoding, 
- leds Controlling functions, 
- start&stop car function disabler (more detailes in the dedicated subparagraph),
- immobilizer (more details in the dedicated subparagraph)
## Folders content
- Subfolder firmware contains the firmware
- Subfolder hardware/canable contains canable board layout and pcb wiring diagram. It comes from https://github.com/makerbase-mks/CANable-MKS. There are different designs of canable, but theay are all similar.
- Subfolder hardware/box contains the 3d model of the case to accomodate required components.
- Subfolder hardware/system interconnection contains interconnection diagram to connect required components
- Subfolder tools contains the famous savvyCan sniffer tool for windows (portable) and excel sheet used to calculate pwm and clocks settings.
## Start&Stop car functionality Disabler
The functionality "car start&stop disabler" is implemented by simply shorting a gpio to ground trough a resistor, in order to simulate button press on the car panel, with a delay after the device was switched on. The used resistor is suitable for my car. 
This projet was tested on alfaromeo Giulia. Each one of you, if dealing with other car, different than Alfaromeo Giulia/Stelvio,  should:
- perform some checks on the panel with a multimeter, in order to find the proper resistor value for the start&stop button. 
## Immobilizer functionality
The functionality IMMOBILIZER performs the following:
1. Detects if the thief is trying to connect to to RFHUB (they do it to add a key to the car)
2. Resets the RFHUB in order to reset the thief connection
3. Starts the Panic Alarm

Note1: Panic alarm will start only if you previusly enabled panic alarm in your ECU, with the MES proxy alignment procedure shown in this video: https://youtu.be/dHC6A2Jsalo
Note2: The Immobilizer functionality will not detect the thief if you power the BACCAble with a voltage available only when the panel is switched on. I'm still working on this, in order to understand if Could I use another approach.
## Leds Strip controller
The leds strip is lighted accordingly to the movement of the accelerator pedal and the gear selection. 

This projet was tested on alfaromeo Giulia. Each one of you, if dealing with other car, should:
- identify proprietary can bus messages, by sniffing data with savvycan.
## Usage Instructions
You should perform some preliminary settings inside firmware:
- If you want to use the device as usb can bus sniffer you shall uncomment #define ACT_AS_CANABLE in main.h
- If you want to use the device as leds strip controller you shall comment #define ACT_AS_CANABLE in main.h
- If you don't want to use the piece of code that disables the car start&stop at the power on, you shall comment #define DISABLE_START_STOP in main.h
- If you want to disable IMMOBILIZER functionality, you shall comment #define IMMOBILIZER_ENABLED in main.h
- In vumeter.c you shall set the number of leds in your leds strip, by modifing the following line: #define MAX_LED 46

Software to use:
- use stm32CubeIde to compile on windows
- use stm32CubeProgrammer to flash the firmware elf file contained in subfolder firmware\ledsStripController\Release 

Note: i downloaded previous version of the programmer (v.2.15.0) since last available revision had some bug that won't allow me to flash canable.

Flash procedure:
- press reset button on the canable, then connect usb to pc (the canable will be detected as serial device named "stm32 bootloader"
- use stm32CubeProgrammer to flash the device


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

## The interconnections
![Interconnections](https://github.com/gaucho1978/CANableAndLedsStripController/blob/master/hardware/system_interconnection/SCHEMA_DI_INTERCONNESSIONE.png)

## The Box
![Box](https://github.com/gaucho1978/CANableAndLedsStripController/blob/master/hardware/box/box.png)
![Cap](https://github.com/gaucho1978/CANableAndLedsStripController/blob/master/hardware/box/cap.png)

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

## Alfa Romeo Giulia Protocol Reverse Engineering 

These are information that I found and that I can share. Use everything this at your own risk.

1. These messages changes when you move accelerator pedal:
   - message id 0ff, second and third byte, changes from 1D33 to 39f3
   - message id 1f0, first 3 nibble changes from 000 to 1f2
   - message id 412 , fourth byte, changes from 33 to E6. I use this one!!
   - message id 736, second and third byte, changes from 3319 to E772

1. The following message identifies gear selection (I use this too):
   - Message id 2ef, first byte: 0x70=Reverse , 0x00=Neutral, 0xf0=Undefined (in example pressed clutch), 0x10=first gear, 0x20=second gear ...and so on up to sixt gear

2. According to Sniz (a famous guru), this is RFHUB Reset (But I didn't tested it):
   - T18DAC7F180211010000000000 

3. According to Sniz this starts car Alarm, but on my car it doesn't work:
   - T1E340041488201500
   - t1EF84202E20000000156 This, on my car, temporary resets the main panel (if it is on) and starts the panic alarm
   - t2EC80000000000000000
   - T1E340041488201500

4. This message is periodically sent when panel is on:
   - t38480809DA080004XXYY (XX= counter from 00 to 0F , YY=checksum) DNA status - Dynamic
   - t38480801DA080004XXYY (XX= counter from 00 to 0F , YY=checksum) DNA status - Natural
   - t38480811DA080004XXYY (XX= counter from 00 to 0F , YY=checksum) DNA status - AllWeather
   - t38480831DA080004XXYY (XX= counter from 00 to 0F , YY=checksum) DNA status - Race (on my car this is not available)

5. You can Send these messages to emulate the following key Press (or you can detect when they have been pressed, by filtering received messages):
   - t2FA390XXYY (XX= counter from 00 to 0F , YY=checksum) Steering wheel button - RES
   - t2FA312XXYY (XX= counter from 00 to 0F , YY=checksum) Steering wheel button - Cruise control on/off
   - t2FA308XXYY (XX= counter from 00 to 0F , YY=checksum) Steering wheel button - Cruise control speed gently up
   - t2FA300XXYY (XX= counter from 00 to 0F , YY=checksum) Steering wheel button - Cruise control speed strong up
   - t2FA318XXYY (XX= counter from 00 to 0F , YY=checksum) Steering wheel button - Cruise control speed gently down
   - t2FA320XXYY (XX= counter from 00 to 0F , YY=checksum) Steering wheel button - Cruise control speed strong down
   
6. Experiments: 
   - t2EE47FE00000 This on my car, if the panel is on, temporary resets main panel ad you can ear relays switch sound
   - t0FA8A0200000200400F1 this on my car, if the panel is on, generates animation on the panel like switch off and on
