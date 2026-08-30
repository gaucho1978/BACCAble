/*
 * elmlink.h
 *
 *  Diagnostic bridge between the three chips of the BACCAble.
 *
 *  The chip connected to the C1 bus acts as gateway: it receives the ELM327 commands from the
 *  USB port and, when the ecu being looked for is not on its own bus, it forwards the CAN frame
 *  to the chip of the C2 or BH bus through the serial line that already connects the three
 *  processors (USART2, single wire half-duplex, 38400 baud). The slave transmits the frame on
 *  its own bus, collects the answers and sends them back. This way a single USB port reaches
 *  all three networks, without moving the adapter on the OBD.
 *
 *  The module is written so that it can be switched on at runtime (elmlink_set_enabled): today it
 *  is compiled into the dedicated flavors, tomorrow it will be able to live inside the normal
 *  firmware and be activated from the instrument cluster menu.
 *
 *  Message format: the encapsulation already present on the line is reused (blocks of
 *  UART_BUFFER_SIZE bytes, first byte = recipient), adding three new recipients.
 *  This way the bridge coexists with the normal messages between the chips.
 *
 *      [0]     recipient      0x0E = chip C2, 0x0F = chip BH, 0x10 = master (C1)
 *      [1]     type           1 = REQ, 2 = CFG, 3 = RSP, 4 = END
 *      [2]     flags          bit0 = 29 bit id, bit1 = no answer (on END)
 *      [3..6]  CAN id (big endian)      | CFG: filter value
 *      [7]     dlc                      | CFG: not used
 *      [8..15] data (8 bytes)           | CFG: [8..11] mask, [12..13] timeout ms
 *      [16]    sequence number
 *      [17]    checksum (xor of bytes 0..16)
 *      [18]    padding
 */

#ifndef INC_ELMLINK_H_
#define INC_ELMLINK_H_

	#include "compile_time_defines.h"

	//recipients of the bridge messages (they continue the numbering of uart.h)
	#define ELMLINK_TO_C2			0x0E
	#define ELMLINK_TO_BH			0x0F
	#define ELMLINK_TO_MASTER		0x10

	//message types
	#define ELMLINK_TYPE_REQ		0x01	//master -> slave: transmit this frame
	#define ELMLINK_TYPE_CFG		0x02	//master -> slave: reception filter and timeout
	#define ELMLINK_TYPE_RSP		0x03	//slave -> master: frame received
	#define ELMLINK_TYPE_END		0x04	//slave -> master: answers finished
	#define ELMLINK_TYPE_FCCFG		0x05	//master -> slave: custom flow control (ATFCSH/ATFCSD)
	#define ELMLINK_TYPE_ARM		0x06	//master -> slave: switch the bridge on/off (cluster menu)

	#define ELMLINK_FLAG_EXTID		0x01
	#define ELMLINK_FLAG_NODATA		0x02
	#define ELMLINK_FLAG_ARM_ON		0x01	//in an ARM message: 1 = switch on, 0 = switch off
	#define ELMLINK_CFG_AUTOFC		0x01	//flag in CFG[14]: the slave sends the flow control by itself

	//bus identifiers used by the gateway
	#define ELMLINK_BUS_LOCAL		0		//the bus of the chip itself (C1)
	#define ELMLINK_BUS_C2			1
	#define ELMLINK_BUS_BH			2
	#define ELMLINK_BUS_COUNT		3

	#define ELMLINK_DEFAULT_TIMEOUT_MS	300	//maximum wait for the FIRST answer on the slave

	// Once at least one frame has been forwarded there is no point in waiting the full timeout:
	// only the time in which the next frame could arrive is waited for. How much, depends
	// on what has just gone through (PCI byte of the ISO-TP):
	#define ELMLINK_GAP_AFTER_SF_MS		40	//single frame: at most another ecu answers
	#define ELMLINK_GAP_AFTER_FF_MS		250	//first frame: the ecu waits for the flow control, which goes around the serial line
	#define ELMLINK_GAP_AFTER_CF_MS		80	//consecutive frames: they arrive one after the other

	// Exception: if the ecu answers "request received, I am working on it" (UDS 7F xx 78,
	// typical of writes such as the proxi alignment) the real answer can arrive even
	// a few seconds later. In that case all the time the program asked for is waited,
	// otherwise the write looks timed out even if the ecu completes it correctly.
	#define ELMLINK_UART_TX_TIMEOUT_MS	30	//maximum wait to put a block on the line

#if defined(C1baccable) || defined(C2baccable) || defined(BHbaccable)

	#include "stm32f0xx_hal.h"
	#include "can.h"

	// --- common -------------------------------------------------------------------
	void elmlink_init(void);
	void elmlink_set_enabled(uint8_t on);	//switching on/off at runtime (cluster menu)
	uint8_t elmlink_is_enabled(void);
	//to be called inside HAL_UART_RxCpltCallback when a block of the bridge arrives:
	//returns 1 if the message has been taken care of
	uint8_t elmlink_on_uart_frame(const uint8_t *frame);
	void elmlink_process(void);				//to be called in the main loop

	// --- slave side (chip C2 / BH) ------------------------------------------------
	#if defined(C2baccable) || defined(BHbaccable)
		//no public API: the slave works inside elmlink_process()
	#endif

	// --- master side (chip C1, gateway) -------------------------------------------
	#if defined(C1baccable)
		// Switches the bridge on (or off) on the two remote chips. C1 calls it when
		// "ELM327 Diag" is chosen in the cluster menu: before that moment C2 and BH work normally
		// and ignore the bridge messages.
		void elmlink_send_arm(uint8_t on);

		//tells the slave the reception filter and the timeout to use
		void elmlink_send_config(uint8_t bus, uint32_t filterValue, uint32_t filterMask,
		                         uint16_t timeoutMs, uint8_t autoFlowControl);

		//custom flow control (ATFCSH / ATFCSD): only needed if the program sets it
		void elmlink_send_fc_config(uint8_t bus, uint32_t fcHeader, uint8_t fcExt,
		                            const uint8_t *fcData, uint8_t fcLen);

		// Sends the request on a remote bus and returns IMMEDIATELY: the answers are collected
		// with elmlink_poll() inside the normal waiting loop, so that every frame can
		// be sent to the PC as soon as it arrives, exactly as on the local bus.
		uint8_t elmlink_send_request(uint8_t bus, uint32_t canId, uint8_t ext,
		                             const uint8_t *data, uint8_t dlc);

		// To be called in the waiting loops: delivers the frames arrived from the slave.
		// Returns 1 when the slave has reported that nothing else will arrive.
		uint8_t elmlink_poll(void (*onFrame)(uint32_t id, uint8_t ext, const uint8_t *d, uint8_t dlc));
	#endif

#endif //C1baccable || C2baccable || BHbaccable

#endif /* INC_ELMLINK_H_ */
