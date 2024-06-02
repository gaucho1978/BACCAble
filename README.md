## General description

This project uses the famous CANABLE in order to:
- sniff on the can bus (useful for debug and exploit purposes)
- decode some parameters on the bus (like motor rpm, accelerator pedal position and gear selection)
- control a WS281x leds strip controlled by means of the decoded can bus data
- automatically disable start&stop car functionality

Subfolder firmware contains the firmware

Subfolder hardware/canable contains canable board layout and pcb wiring diagram. It comes from https://github.com/makerbase-mks/CANable-MKS. There are different designs of canable, but theay are all similar.

Subfolder hardware/box contains the 3d model of the case to accomodate required components.

Subfolder hardware/system interconnection contains interconnection diagram to connect required components

Subfolder tools contains the famous savvyCan sniffer tool for windows (portable) and excel sheet used to calculate pwm and clocks settings.

In the firmware I used the famous SLCAN firmware (https://github.com/normaldotcom/canable-fw), then I added message decoding, leds Controlling functions and start&stop car function disabler.

the can start&stop disabler function is done by simply shorting a gpio to ground to simulate button press on the car panel, with a delay after the device was switched on. The used resistor is suitable for my car. Each one of you shoud perform some checks on the panel with a multimeter to find the proper resistor value. Same approach shall be used on the can bus protocol, even if the accelerator pedal position should be the same on each car.

## Usage Instructions
use stm32CubeIde to compile on windows
use stm32CubeProgrammer to flash the firmware elf file contained in subfolder firmware\ledsStripController\Release 
Note: i downloaded previous version of the programmer (v.2.15.0) since last available revision had some bug that won't allow me to flash canable.

You should perform some settings inside firmware:
If you want to use the device as usb can bus sniffer you shall uncomment #define ACT_AS_CANABLE in main.h
If you want to use the device as leds strip controller you shall comment #define ACT_AS_CANABLE in main.h
If you don't want to use the piece of code that disables the car start&stop at the power on, you shall comment #define DISABLE_START_STOP in main.h
In vumeter.c you shall set the number of leds in your leds strip, by modifing the following line: #define MAX_LED 46

flash procedure:
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
More explained in the later sections.


## The interconnections
![Interconnections](https://raw.githubusercontent.com/gaucho1978/ledsstripcontroller/hardware/system_interconnection/SCHEMA_DI_INTERCONNESSIONE.png?sanitize=true)
## The Box
![Box](https://raw.githubusercontent.com/gaucho1978/ledsstripcontroller/hardware/box/box.png?sanitize=true)
![Cap](https://raw.githubusercontent.com/gaucho1978/ledsstripcontroller/hardware/box/cap.png?sanitize=true)
