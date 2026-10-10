#ifndef _UART_H
	#define _UART_H

	#include "stm32f0xx_hal.h"
	#include "globalVariables.h"

	#include "onboardLed.h"
	#include "string.h"
	//#include "error.h"
		
	
	#define C1BusID							0x01 //first byte sent over uart to identify destinator baccable connected to C1 Can Bus
	#define C2BusID							0x02 //first byte sent over uart to identify destinator baccable connected to C2 Can Bus
	#define BhBusIDparamString				0x03 //first byte sent over uart to identify destinator baccable connected to BH Can Bus to transfer parameter string
	#define AllSleep						0x04 //first byte sent over uart to tell anyone connecter to go to sleep (or low consumption).
	#define C2BusIDAllSleepAck				0x05 //first byte sent over uart by C2 baccable in order to communicate that the sleep was received and executed
	#define BHBusIDAllSleepAck				0x06 //first byte sent over uart by BH baccable in order to communicate that the sleep was received and executed
	#define AllResetFaults					0x07 //first byte sent over uart by C1 baccable in order to communicate that the Reset Faults is required
	#define BhBusIDgetStatus				0x08 //first byte sent over uart to identify destinator baccable connected to BH Can Bus to request its status
	#define BhBusChimeRequest				0x09 //first byte sent over uart to indentify message to BH to play sound
	#define BhBusID							0x0A //first byte sent over uart to indentify destinator baccable connected to BH Can Bus
	#define C2_Bh_BusID						0x0B //first byte sent over uart to indentify destinator baccable connected to C2 and BH Can Bus
	#define C1_Bh_BusID						0x0C //first byte sent over uart to indentify destinator baccable connected to C1 and BH Can Bus
	#define C1_C2_BusID						0x0D //first byte sent over uart to indentify destinator baccable connected to C1 and C2 Can Bus

	#define C1cmdLaneSingleTap					0x1F  //second byte of the message to C1 bus, notifies Lane button single tap
	#define C1cmdLaneDoubleTap					0x20  //second byte of the message to C1 bus, notifies Lane button double tap
	#define C1cmdNormalFrontBrake				0x21 //second byte of the message to C1 bus, identifies the request to set front Brake to normal
	#define C1cmdForceFrontBrake				0x22 //second byte of the message to C1 bus, identifies the request to force Front Brake ON
	//sniffer function 24/08/2026 - one command per sender, so C1 can track the two slaves independently: the
	//sniffer enable command is broadcast to C2 and BH together, so the slave with no cable attached always times
	//out, and with a single shared command its "disconnected" would clear the state the other slave had just
	//legitimately reported (and C1 would sleep with a session still in use).
	#define C1usbConnectedFromC2				0x23 //second byte of the message to C1 bus, identifies the connection to usb connector of C2
	#define C1usbDisconnectedFromC2				0x24 //second byte of the message to C1 bus, identifies that the usb connector of C2 is no longer configured
	#define C1cmdDynoActive						0x25 //second byte of the message to C2 bus, identifies the status dyno Active
	#define C1cmdDynoNotActive					0x26 //second byte of the message to C2 bus, identifies the status dyno Not Active
	#define C1usbConnectedFromBH				0x27 //second byte of the message to C1 bus, identifies the connection to usb connector of BH //sniffer function 24/08/2026
	#define C1usbDisconnectedFromBH				0x28 //second byte of the message to C1 bus, identifies that the usb connector of BH is no longer configured //sniffer function 24/08/2026
	#define C1cmdAbsFaultsReply				0x29 //second byte of the message to C1 bus: ABS faults read by C2 (see FAULTS ABS PROTOCOL below) //readFaults ABS 10/10/2026

	#define C2cmdtoggleDyno						0x20 //second byte of the message to C2 bus, identifies the request to toggle dyno
	#define C2cmdNormalFrontBrake				0x21 //second byte of the message to C2 bus, identifies the request to set front Brake to normal
	#define C2cmdForceFrontBrake				0x22 //second byte of the message to C2 bus, identifies the request to force Front Brake ON
	#define C2cmdGetStatus						0x23 //second byte of the message to C2 bus, identifies the request to getStatus
	#define C2cmdtoggleEscTc					0x24 //second byte of the message to C2 bus, identifies the request to toggle ESC/TC
	#define C2cmdRaceMaskDefault				0x27 //second byte of the message to C2 bus, identifies the status race mask not requested
	#define C2cmdShowRaceMask					0x28 //second byte of the message to C2 bus, identifies the status race mask requested
	#define C2cmdToggleHas						0x29 //second byte of the message to C2 bus, identifies the request to press HAS button for some consecutive messages
	//#define C2cmdLaneLongPress				0x2A //second byte of the message to C2 bus, identifies the notification of long press of LANE button received on BH bus
	#define C2cmdFunctParkSensorsMuteDisabled	0x2B //second byte of the message to C2 bus: park sensors mute function disabled
	#define C2cmdFunctParkSensorsMuteEnabled	0x2C //second byte of the message to C2 bus: park sensors mute function enabled
	#define C2cmdParkMuteBrakeReleased			0x2D //second byte of the message to C2 bus: brake released (from 0x107, only on the C1 bus) //park mute brake from C1 10/10/2026
	#define C2cmdParkMuteBrakePressed			0x2E //second byte of the message to C2 bus: brake pressed (from 0x107, only on the C1 bus) //park mute brake from C1 10/10/2026
	#define C2cmdAbsFaultsStart				0x2F //second byte of the message to C2 bus: read the ABS faults (see FAULTS ABS PROTOCOL below) //readFaults ABS 10/10/2026
	#define C2cmdAbsFaultsGet					0x30 //second byte of the message to C2 bus: send the ABS faults from index [2]-'A' (see FAULTS ABS PROTOCOL below) //readFaults ABS 10/10/2026

	//readFaults ABS 10/10/2026 - FAULTS ABS PROTOCOL. The ABS is on the C2 bus: C2 reads its faults by itself (10 03, 19 02 09,
	//multiframe towards 0x18DA28F1, reply 0x18DAF128) and C1 collects them with the normal master/slave messages, never with
	//the elm327 bridge (it stops the normal work of C2 and BH and transmits asynchronously on the single wire line).
	//  C1 -> C2  [C2BusID][C2cmdAbsFaultsStart]                 start a new reading
	//  C1 -> C2  [C2BusID][C2cmdAbsFaultsGet]['A'+i]            send the faults from index i. It replaces C2cmdGetStatus while
	//                                                           C1 is collecting: it also opens the reply window of C2
	//  C2 -> C1  [C1BusID][C1cmdAbsFaultsReply][status][total hi][total lo][kept][index][DTC i: 6 chars][DTC i+1: 6 chars]
	//            status 'B' reading in progress, 'E' failed (refused, timeout, ABS busy with dyno/front brake), 'R' ready
	//            (the other fields only with 'R'); total = valid DTC in the ABS answer, 2 hex chars; kept = 'A'+n DTC kept by
	//            C2; index = 'A'+i of the first DTC of the message; DTC = 3 bytes as 6 hex chars, spaces if not present.
	//            Up to ABS_FAULTS_REPLY_MSGS messages per request.
	//  From byte 2 on every byte is a letter, a digit or a space (>= 0x20): even if the single wire line lost the
	//  synchronisation, no byte of the payload can be taken for the first byte of a message (0x01..0x10) by BH or C2.
	//  ABS_FAULTS_DTC_MAX and ABS_FAULTS_REPLY_MSGS are in globalVariables.h

	#define BHcmdOdometerBlinkDisable			0x20 //second byte of the message to BH bus, identifies the request to disable odometer blink
	#define BHcmdOdometerBlinkDefault			0x21 //second byte of the message to BH bus, identifies the request to restore normal odometer blink status
	#define BHcmdFunctParkMirrorDisabled		0x22 //second byte of the message to BH bus, identifies the request to disable park mirror
	#define BHcmdFunctParkMirrorEnabled			0x23 //second byte of the message to BH bus, identifies the request to enable park mirror
	#define BHcmdFunctParkMirrorStoreCurPos		0x24 //second byte of the message to BH bus, identifies the request to enable park mirror and to store current mirror position

	#define C2_Bh_cmdSetPedalBoostStatus		0x39 //second byte of the message to C2 and BH bus, identifies the pedal booster function status. Third byte of the message will contain its status
	#define	C2_Bh_cmdFunctHAS_Disabled			0x3A //second byte of the message to C2 and BH bus, identifies the request to disable HAS function.
	#define	C2_Bh_cmdFunctHAS_Enabled			0x3B //second byte of the message to C2 and BH bus, identifies the request to enable HAS function.
	#define	C2_Bh_cmdFunction_ESC_TC_Disabled	0x3C //second byte of the message to C2 and BH bus, identifies the request to disable ESC/TC function.
	#define	C2_Bh_cmdFunction_ESC_TC_Enabled	0x3D //second byte of the message to C2 and BH bus, identifies the request to disable ESC/TC function.
	#define	C2_Bh_cmdFunction_Save_Log_to_File	0x3E //second byte of the message to C2 and BH bus, identifies the request to save log to file.
	#define	C2_Bh_cmdSnifferDisabled			0x3F //second byte of the message to C2 and BH bus: sniffer function disabled //sniffer function 24/08/2026
	#define	C2_Bh_cmdSnifferEnabled				0x40 //second byte of the message to C2 and BH bus: sniffer function enabled //sniffer function 24/08/2026

	#define C1BHcmdShowRaceScreen				0x40 //second byte of the message to C1 and BH bus, identifies the request to show Race Screen on dashboard
	#define C1BHcmdStopShowRaceScreen			0x41 //second byte of the message to C1 and BH bus, identifies the request to STOP to show Race Screen on dashboard

	#define C1_C2_cmdLaneDoubleTap				0x50 //second byte of the message to C1 and C2 bus, notifies LANE button double tap




	
	void uart_init();
	void pauseUart(UART_HandleTypeDef *huart);
	void restartUart(UART_HandleTypeDef *huart);

	//void uart_transmit_data(char*  message);
	//void process_received_data();
	//void enter_standby_mode();
	//void resetOtherProcessorsSleepStatus();
	//uint8_t getOtherProcessorsSleepingStatus();
	void addToUARTSendQueue(const uint8_t *data, size_t length);
	void addToUARTSendQueueDuringInterrupt(const uint8_t *data, size_t length);

	//elm327 function 26/08/2026 - the diagnostic bridge bypasses the queue: see uart.c for why
	#include "elmlink.h"
	#if defined(C1baccable) || defined(C2baccable) || defined(BHbaccable)
		void uart_link_send(const uint8_t *frame, uint8_t len);
	#endif

	#if(defined(C1baccable) || defined (ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER))
		void addToUART1SendQueue(const uint8_t *data, size_t length);
		void addToUART1SendQueueDuringInterrupt(const uint8_t *data, size_t length);
		void processUART1(void);
	#endif
	void processUART();
#endif
