/*
 * elm327.h
 *
 *  Emulation of an ELM327 interface (ISO 15765-4 / CAN) on the USB CDC port
 *  of the BACCAble, meant for MultiECUScan ("ELM327 High Speed").
 *
 *  Ported from the ESP32-C3 version (TWAI) to the HAL/bxCAN API of the STM32F072.
 *
 *  Main differences from the ESP32 version:
 *   - the transport is USB CDC: the baud rate is virtual, so ATBRD only performs
 *     the formal handshake without changing anything real;
 *   - can_tx() of BACCAble only queues: during the waits can_process() has to be
 *     called, otherwise the frame never leaves;
 *   - CDC_Transmit_FS() drops packets longer than TX_BUF_SIZE (64B), so the
 *     output is transmitted in chunks;
 *   - multi-frame ISO-TP sending (requests > 7 bytes) is supported as well.
 */

#ifndef INC_ELM327_H_
#define INC_ELM327_H_

	#include "compile_time_defines.h"

	#ifdef ACT_AS_ELM327

		#include "stm32f0xx_hal.h"
		#include "can.h"

		//identification string returned by ATZ / ATI / ATWS
		#define ELM327_ID_STRING		"ELM327 v1.4"
		//answer to AT@1: it is the exact one of the original chip (no dash in "OBDII"),
		//tools compare it to recognise the interface
		#define ELM327_DESCR_STRING		"OBDII to RS232 Interpreter"

		// After the reset the original ELM327 has ECHO ON, LINE FEEDS OFF and puts an
		// empty line before the prompt. Many programs (AlfaOBD among them) discard the first
		// line received because they expect the echo of the command: with the echo off they
		// threw away the real answer and did not recognise the interface.
		// Commenting this line out goes back to the previous behaviour (echo off, line feeds on).
		#define ELM327_STRICT_ELM_DEFAULTS

		#define ELM327_RX_RING_LEN		256		//circular buffer of the bytes coming in from usb
		#define ELM327_CMD_BUF_LEN		80		//maximum length of a command
		#define ELM327_ISOTP_MAX_LEN	255		//maximum length of an ISO-TP answer
		#define ELM327_TX_CHUNK_LEN		60		//usb transmission chunk (< TX_BUF_SIZE)
		#define ELM327_PAD_BYTE			0xAA	//padding of the can frames (as in the esp32 firmware)

		// Format of the header printed with ATH1.
		// By default the bytes are printed separated (e.g. "07 E8 ..."), as in the esp32
		// firmware already tested with MultiECUScan. Defining ELM327_HEADER_3DIGITS gives
		// the format of the original ELM327 instead (e.g. "7E8 ..."): worth trying if the
		// tool were to refuse the 11 bit answers.
		//#define ELM327_HEADER_3DIGITS

		// Unrecognised AT command: by default we answer "OK" because several tools
		// (AlfaOBD among them) close the connection as soon as they receive a "?".
		// Defining ELM327_UNKNOWN_AT_IS_ERROR goes back to the "?" answer of the original ELM327.
		//#define ELM327_UNKNOWN_AT_IS_ERROR

		// With ATS0 the header is printed without spaces, as on the original ELM327.
		// Defining ELM327_HEADER_ALWAYS_SPACED goes back to the behaviour of the first
		// version (header bytes always separated by a space, even with ATS0).
		//#define ELM327_HEADER_ALWAYS_SPACED

		#define ELM327_MAX_RAW_FRAMES		64	//max frames printed per command in ATCAF0 mode

		// When an ecu has answered "I am working on it" (7F xx 78), the answer that
		// follows is the definitive one: we close immediately instead of listening until
		// the timeout, otherwise the prompt reaches the program one second late.
		#define ELM327_ROUTE_CACHE_LEN		16	//how many ecus to remember (address -> bus)
		#define ELM327_REMOTE_FIFO_LEN		12	//frames coming in from a remote bus, waiting to be read

		// While it is not known yet which bus an ecu is on, the buses are tried with
		// this reduced timeout instead of the one asked for by the program (which with ATST99
		// is 612 ms per attempt). An ecu in a diagnostic session answers within tens of
		// milliseconds: as soon as the bus is known the full timeout is used again.
		#define ELM327_PROBE_TIMEOUT_MS		200

		#define ELM327_DEFAULT_TIMEOUT_MS	200	//default timeout (ATST not set)
		#define ELM327_FC_TIMEOUT_MS		250	//maximum wait for the Flow Control of the ecu

		// Some tools expect the "empty" CR that the original ELM327 sends before the
		// ">" prompt. Enable it if a program does not recognise the answers.
		//#define ELM327_EXTRA_CR_BEFORE_PROMPT

		// Recorder of the dialog with the PC: keeps in memory the last commands received and
		// the answers sent. It serves to understand what a program that does not connect is
		// sending: after the failed attempt just open a serial terminal and type ATLOG
		// (ATLOGC clears the log). To remove it completely: ELM327_TRACE_DISABLE.
		//#define ELM327_TRACE_DISABLE
		#define ELM327_TRACE_LEN		2048	//bytes of memory dedicated to the log

		void elm327_init(void);					//to be called after the init of can and usb
		void elm327_port_reset(void);			//host opening/closing the port: discards the old data
		void elm327_rx_byte(uint8_t c);			//called by cdc_process (context with irq disabled): only queues
		void elm327_process(void);				//called from the main loop: interprets and executes one command at a time

		#if defined(C1baccable)
			// Switching on and off from the dashboard menu ("ELM327 Diag" entry).
			// While off the interpreter touches nothing: it does not read the USB, does not touch
			// the CAN bus, does not talk on the serial line between the chips. The BACCAble works normally.
			void elm327_set_enabled(uint8_t on);
			uint8_t elm327_is_enabled(void);

			// Safety: if no command arrives from the PC for this long the mode switches itself
			// off and the BACCAble goes back to working normally. It is needed because with the
			// ELM327 on the dashboard no longer answers the buttons, so without an automatic way
			// out one would stay stuck until the battery is disconnected.
			#ifndef ELM327_IDLE_EXIT_MS
				#define ELM327_IDLE_EXIT_MS		120000	//two minutes
			#endif
		#endif

	#endif //ACT_AS_ELM327

#endif /* INC_ELM327_H_ */
