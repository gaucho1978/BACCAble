/*
 * elmlink.c — diagnostic bridge between the chips of the BACCAble (see elmlink.h)
 */

#include "elmlink.h"

#if defined(C1baccable) || defined(C2baccable) || defined(BHbaccable)

#include <string.h>
#include "globalVariables.h"
#include "uart.h"
#include "onboardLed.h"

extern UART_HandleTypeDef huart2;

// ------------------------------------------------------------------ common state
// elm327 function 27/08/2026 - the bridge lives inside the normal firmware of all three chips,
// so it ALWAYS starts off: it is switched on only when "ELM327 Diag" is chosen in the cluster
// menu. The case of the flavors dedicated to the bridge (module on from the start) does not exist here.
static uint8_t linkEnabled = 0;
static uint8_t seqCounter  = 0;

// Blocks received from the interrupt, waiting to be processed in the main loop.
// It is a small queue (not a single slot): the gateway sends CFG and REQ one after the other,
// and with a single slot the second one could arrive before the first had been consumed and
// be discarded. With the queue the two blocks always coexist.
#define ELMLINK_INBOX_LEN 4
static volatile uint8_t  inbox[ELMLINK_INBOX_LEN][UART_BUFFER_SIZE];
static volatile uint8_t  inboxHead = 0, inboxTail = 0;

static uint8_t inbox_push(const uint8_t *f){
	uint8_t next = (uint8_t)((inboxHead + 1) % ELMLINK_INBOX_LEN);
	if(next == inboxTail) return 0;			//queue full: discard (should not happen)
	memcpy((void *)inbox[inboxHead], f, UART_BUFFER_SIZE);
	inboxHead = next;
	return 1;
}
static uint8_t inbox_pop(uint8_t *out){
	if(inboxTail == inboxHead) return 0;
	memcpy(out, (const void *)inbox[inboxTail], UART_BUFFER_SIZE);
	inboxTail = (uint8_t)((inboxTail + 1) % ELMLINK_INBOX_LEN);
	return 1;
}
static void inbox_clear(void){ inboxTail = inboxHead; }

void elmlink_set_enabled(uint8_t on){ linkEnabled = on ? 1 : 0; }
uint8_t elmlink_is_enabled(void){ return linkEnabled; }

static uint8_t elmlink_checksum(const uint8_t *f){
	uint8_t x = 0;
	for(uint8_t i = 0; i < 17; i++) x = (uint8_t)(x ^ f[i]);
	return x;
}

static void elmlink_build(uint8_t *f, uint8_t dest, uint8_t type, uint8_t flags,
                          uint32_t id, const uint8_t *data, uint8_t dlc, uint8_t seq){
	memset(f, 0x20, UART_BUFFER_SIZE);
	f[0] = dest;
	f[1] = type;
	f[2] = flags;
	f[3] = (uint8_t)((id >> 24) & 0xFF);
	f[4] = (uint8_t)((id >> 16) & 0xFF);
	f[5] = (uint8_t)((id >> 8)  & 0xFF);
	f[6] = (uint8_t)( id        & 0xFF);
	f[7] = dlc;
	memset(&f[8], 0, 8);
	if(data && dlc) memcpy(&f[8], data, dlc > 8 ? 8 : dlc);
	f[16] = seq;
	f[17] = elmlink_checksum(f);
}

static uint32_t elmlink_id_of(const uint8_t *f){
	return ((uint32_t)f[3] << 24) | ((uint32_t)f[4] << 16) |
	       ((uint32_t)f[5] << 8)  |  (uint32_t)f[6];
}

// Sends a block on the line (see uart_link_send in uart.c: it writes to the registers and keeps
// the reception off while sending, otherwise the interrupt never re-arms).
static void elmlink_send(const uint8_t *f){
	uart_link_send(f, UART_BUFFER_SIZE);
}

// Called from the reception interrupt: it just copies the block.
uint8_t elmlink_on_uart_frame(const uint8_t *frame){
	uint8_t dest = frame[0];

	if(dest != ELMLINK_TO_C2 && dest != ELMLINK_TO_BH && dest != ELMLINK_TO_MASTER) return 0;

	// Corrupted block: almost always it means that the 19 byte framing has slipped
	// (a byte lost during a collision on the single wire line). Returning 2
	// asks uart.c to throw the synchronisation away and catch it again byte by byte:
	// without this the link stayed out of phase FOREVER and all the remote buses
	// died after the first session with heavy traffic (log of 24/08, 21:27).
	if(elmlink_checksum(frame) != frame[17]) return 2;

	#if defined(C2baccable) || defined(BHbaccable)
		// The switch-on order must get through EVEN with the bridge off: it is precisely the
		// message that switches it on. It comes from C1 when "ELM327 Diag" is chosen in the cluster menu.
		if(dest == ELMLINK_SLAVE_ID && frame[1] == ELMLINK_TYPE_ARM){
			linkEnabled = (frame[2] & ELMLINK_FLAG_ARM_ON) ? 1 : 0;
			if(!linkEnabled) inbox_clear();
			return 1;
		}
	#endif

	if(!linkEnabled) return 1;			//message of ours but the bridge is off: we consume it

	#if defined(C1baccable)
		//the master only listens to the answers; its own messages come back as an echo
		if(dest != ELMLINK_TO_MASTER) return 1;
	#endif
	#if defined(C2baccable) || defined(BHbaccable)
		if(dest != ELMLINK_SLAVE_ID) return 1;
	#endif

	inbox_push(frame);					//queued: it will be processed in the main loop
	return 1;
}


// =====================================================================================
//  SLAVE SIDE (chip C2 / BH): receives a frame, transmits it on its own bus and sends
//  the answers back. It does not block the main loop: the collection is a state machine.
// =====================================================================================
#if defined(C2baccable) || defined(BHbaccable)

static uint8_t  slaveBusOpen	= 0;
static uint8_t  slaveCollecting	= 0;
static uint32_t slaveDeadline	= 0;
static uint8_t  slaveSeq		= 0;
static uint8_t  slaveGotFrames	= 0;
static uint8_t  slaveLastPci	= 0xFF;	//PCI byte of the last frame forwarded
static uint32_t slaveReqId		= 0;	//id of the last request (used for the flow control)
static uint8_t  slaveReqExt		= 0;
static uint8_t  slaveAutoFc		= 1;	//the slave answers the first frames by itself
static uint32_t slaveFcHeader	= 0;	//0 = use the id of the request
static uint8_t  slaveFcExt		= 0;
static uint8_t  slaveFcData[8]	= {0x30, 0x00, 0x00, 0, 0, 0, 0, 0};
static uint8_t  slaveFcLen		= 3;
static uint32_t slaveFilterVal	= 0;
static uint32_t slaveFilterMask	= 0;
static uint16_t slaveTimeout	= ELMLINK_DEFAULT_TIMEOUT_MS;

//the bus is opened only at the first request: powering the board must not disturb the car
static void slave_bus_open(void){
	if(slaveBusOpen) return;
	// elm327 function 27/08/2026 - the bus is already open at the right speed, it is the one the
	// BACCAble works on every day: we do not touch it. The branch that reconfigured it (flavors dedicated
	// to the bridge, where the bus belonged to the slave) does not exist here any more: on an installed
	// baccable it would mean switching off the network the chip is working on.
	can_enable();	//no-op if it is already open: it only serves the case of the BH chip from idle
	slaveBusOpen = 1;
}

static void slave_send_end(uint8_t noData){
	uint8_t f[UART_BUFFER_SIZE];
	elmlink_build(f, ELMLINK_TO_MASTER, ELMLINK_TYPE_END,
	              noData ? ELMLINK_FLAG_NODATA : 0, 0, NULL, 0, slaveSeq);
	elmlink_send(f);
}

static void slave_handle_frame(const uint8_t *f){
	switch(f[1]){
		case ELMLINK_TYPE_FCCFG:	//custom flow control set by the program
			slaveFcHeader = elmlink_id_of(f);
			slaveFcExt    = (f[2] & ELMLINK_FLAG_EXTID) ? 1 : 0;
			slaveFcLen    = (f[7] > 8) ? 8 : f[7];
			if(slaveFcLen == 0){
				slaveFcLen = 3;
				slaveFcData[0] = 0x30; slaveFcData[1] = 0x00; slaveFcData[2] = 0x00;
			}else{
				memcpy(slaveFcData, &f[8], slaveFcLen);
			}
			break;

		case ELMLINK_TYPE_CFG:
			slaveAutoFc     = (f[14] & ELMLINK_CFG_AUTOFC) ? 1 : 0;
			slaveFilterVal  = elmlink_id_of(f);
			slaveFilterMask = ((uint32_t)f[8] << 24) | ((uint32_t)f[9] << 16) |
			                  ((uint32_t)f[10] << 8) |  (uint32_t)f[11];
			slaveTimeout    = (uint16_t)(((uint16_t)f[12] << 8) | f[13]);
			if(slaveTimeout == 0) slaveTimeout = ELMLINK_DEFAULT_TIMEOUT_MS;
			break;

		case ELMLINK_TYPE_REQ: {
			CAN_TxHeaderTypeDef h;
			uint8_t data[8];
			uint8_t dlc = f[7];
			if(dlc > 8) dlc = 8;
			memcpy(data, &f[8], 8);

			slave_bus_open();

			h.RTR = CAN_RTR_DATA;
			h.DLC = dlc;
			h.TransmitGlobalTime = DISABLE;
			if(f[2] & ELMLINK_FLAG_EXTID){
				h.IDE = CAN_ID_EXT; h.ExtId = elmlink_id_of(f) & 0x1FFFFFFF; h.StdId = 0;
			}else{
				h.IDE = CAN_ID_STD; h.StdId = elmlink_id_of(f) & 0x7FF;      h.ExtId = 0;
			}

			slaveSeq    = f[16];
			slaveReqId  = elmlink_id_of(f);
			slaveReqExt = (f[2] & ELMLINK_FLAG_EXTID) ? 1 : 0;
			can_tx(&h, data);
			//can_tx only queues: let us give the queue a push
			for(uint8_t i = 0; i < 8; i++) can_process();

			slaveGotFrames = 0;
			slaveLastPci = 0xFF;
			slaveCollecting = 1;
			slaveDeadline = currentTime + slaveTimeout;
			onboardLed_blue_on();
			break; }

		default:
			break;
	}
}

// Recognises the UDS negative answer "request received, response pending" (7F xx 78):
// the ecu is still working and the real answer will arrive later.
static uint8_t elmlink_is_response_pending(const uint8_t *d){
	if((d[0] & 0xF0) != 0x00) return 0;			//single frames only
	return (d[1] == 0x7F && d[3] == 0x78) ? 1 : 0;
}

// Offers the bridge a frame received from the bus. Returns 1 if it has been forwarded to the master.
static uint8_t slave_offer_rx(const CAN_RxHeaderTypeDef *h, const uint8_t *d){
	if(!slaveCollecting) return 0;

	uint32_t id = (h->IDE == CAN_ID_EXT) ? h->ExtId : h->StdId;
	if(slaveFilterMask && ((id & slaveFilterMask) != (slaveFilterVal & slaveFilterMask))) return 0;

	uint8_t f[UART_BUFFER_SIZE];
	elmlink_build(f, ELMLINK_TO_MASTER, ELMLINK_TYPE_RSP,
	              (h->IDE == CAN_ID_EXT) ? ELMLINK_FLAG_EXTID : 0,
	              id, d, (uint8_t)h->DLC, slaveSeq);
	elmlink_send(f);
	slaveGotFrames++;

	// Wait for the next frame calibrated on what has just gone through: without this the
	// full timeout was always waited for (with ATST99 that is 612 ms) even when the
	// ecu had already answered in 5 ms, and it is the reason why the remote buses
	// turned out to be much slower than the local bus.
	slaveLastPci = d[0];
	uint16_t gap = ELMLINK_GAP_AFTER_SF_MS;

	if((slaveLastPci & 0xF0) == 0x10){
		// First frame of a long answer: the ecu waits for the "go on" (flow control).
		// The slave sends it, on its own bus, in a few microseconds. Having the master send it
		// meant a full round trip of the serial line and, above all, opening a new request
		// in the middle of the collection: the ecu timed out and the answer ended up queued to the
		// next command (the answers shifted by one).
		if(slaveAutoFc){
			CAN_TxHeaderTypeDef fh;
			uint8_t fd[8];
			memset(fd, 0, sizeof(fd));
			memcpy(fd, slaveFcData, slaveFcLen);
			fh.RTR = CAN_RTR_DATA;
			fh.DLC = slaveFcLen;
			fh.TransmitGlobalTime = DISABLE;
			uint32_t fid = slaveFcHeader ? slaveFcHeader : slaveReqId;
			uint8_t  fex = slaveFcHeader ? slaveFcExt    : slaveReqExt;
			if(fex){ fh.IDE = CAN_ID_EXT; fh.ExtId = fid & 0x1FFFFFFF; fh.StdId = 0; }
			else   { fh.IDE = CAN_ID_STD; fh.StdId = fid & 0x7FF;      fh.ExtId = 0; }
			can_tx(&fh, fd);
			for(uint8_t i = 0; i < 8; i++) can_process();
			gap = ELMLINK_GAP_AFTER_CF_MS;	//the consecutive frames now arrive straight away
		}else{
			gap = ELMLINK_GAP_AFTER_FF_MS;	//flow control disabled: the program sends it
		}
	}
	else if((slaveLastPci & 0xF0) == 0x20) gap = ELMLINK_GAP_AFTER_CF_MS;

	if(gap > slaveTimeout) gap = slaveTimeout;

	// "I am working on it": all the time granted by the program is waited for, not the short gap.
	// Defining ELMLINK_NO_PENDING_WAIT goes back to the previous behaviour (quick close
	// even after an "I am working on it").
	// Without this, in the long writes (proxi alignment) the frame with the real confirmation
	// arrived when the collection was already closed and was lost: the program saw a timeout
	// even though the ecu had completed correctly.
	#ifndef ELMLINK_NO_PENDING_WAIT
		if(elmlink_is_response_pending(d)) gap = slaveTimeout;
	#endif

	slaveDeadline = currentTime + gap;
	return 1;
}

#endif //C2baccable || BHbaccable


// =====================================================================================
//  MASTER SIDE (chip C1, gateway)
// =====================================================================================
#if defined(C1baccable)

static volatile uint8_t masterWaiting = 0;

static uint8_t bus_to_dest(uint8_t bus){
	return (bus == ELMLINK_BUS_BH) ? ELMLINK_TO_BH : ELMLINK_TO_C2;
}

// Switching the bridge on/off on the C2 and BH chips. It has to be sent with the bridge already on
// on this side (it is the master that decides), and the two messages are spaced out: the line is a
// single wire and the two chips must have the time to read their own.
void elmlink_send_arm(uint8_t on){
	uint8_t f[UART_BUFFER_SIZE];
	uint8_t flags = on ? ELMLINK_FLAG_ARM_ON : 0;

	elmlink_build(f, ELMLINK_TO_C2, ELMLINK_TYPE_ARM, flags, 0, NULL, 0, ++seqCounter);
	elmlink_send(f);
	HAL_Delay(10);
	elmlink_build(f, ELMLINK_TO_BH, ELMLINK_TYPE_ARM, flags, 0, NULL, 0, ++seqCounter);
	elmlink_send(f);
	HAL_Delay(10);
}

void elmlink_send_config(uint8_t bus, uint32_t filterValue, uint32_t filterMask,
                         uint16_t timeoutMs, uint8_t autoFlowControl){
	if(!linkEnabled) return;
	uint8_t f[UART_BUFFER_SIZE];
	elmlink_build(f, bus_to_dest(bus), ELMLINK_TYPE_CFG, 0, filterValue, NULL, 0, seqCounter);
	f[14] = autoFlowControl ? ELMLINK_CFG_AUTOFC : 0;
	f[8]  = (uint8_t)((filterMask >> 24) & 0xFF);
	f[9]  = (uint8_t)((filterMask >> 16) & 0xFF);
	f[10] = (uint8_t)((filterMask >> 8)  & 0xFF);
	f[11] = (uint8_t)( filterMask        & 0xFF);
	f[12] = (uint8_t)((timeoutMs >> 8) & 0xFF);
	f[13] = (uint8_t)( timeoutMs       & 0xFF);
	f[17] = elmlink_checksum(f);
	elmlink_send(f);
}

void elmlink_send_fc_config(uint8_t bus, uint32_t fcHeader, uint8_t fcExt,
                            const uint8_t *fcData, uint8_t fcLen){
	if(!linkEnabled) return;
	uint8_t f[UART_BUFFER_SIZE];
	elmlink_build(f, bus_to_dest(bus), ELMLINK_TYPE_FCCFG,
	              fcExt ? ELMLINK_FLAG_EXTID : 0, fcHeader, fcData, fcLen, seqCounter);
	elmlink_send(f);
}

uint8_t elmlink_send_request(uint8_t bus, uint32_t canId, uint8_t ext,
                             const uint8_t *data, uint8_t dlc){
	if(!linkEnabled) return 0;

	uint8_t f[UART_BUFFER_SIZE];
	seqCounter++;
	inbox_clear();			//discards any leftovers of the previous request
	elmlink_build(f, bus_to_dest(bus), ELMLINK_TYPE_REQ,
	              ext ? ELMLINK_FLAG_EXTID : 0, canId, data, dlc, seqCounter);
	elmlink_send(f);
	return 1;
}

uint8_t elmlink_poll(void (*onFrame)(uint32_t id, uint8_t ext, const uint8_t *d, uint8_t dlc)){
	uint8_t local[UART_BUFFER_SIZE];
	if(!inbox_pop(local)) return 0;

	if(local[16] != seqCounter) return 0;	//answer of a previous request: it is discarded

	if(local[1] == ELMLINK_TYPE_RSP){
		if(onFrame) onFrame(elmlink_id_of(local), (local[2] & ELMLINK_FLAG_EXTID) ? 1 : 0,
		                    &local[8], local[7]);
		return 0;
	}
	if(local[1] == ELMLINK_TYPE_END) return 1;
	return 0;
}

#endif //C1baccable


// =====================================================================================
//  main loop
// =====================================================================================
void elmlink_init(void){
	inbox_clear();
	seqCounter     = 0;
}

void elmlink_process(void){
	#if defined(C2baccable) || defined(BHbaccable)
		// Bridge just switched off by C1: everything is left as it was before. The CAN bus is not closed,
		// it is the one the normal firmware keeps working on.
		if(!linkEnabled){
			slaveCollecting = 0;
			slaveBusOpen    = 0;
			return;
		}
	#endif
	if(!linkEnabled) return;

	#if defined(C2baccable) || defined(BHbaccable)
		{
			uint8_t local[UART_BUFFER_SIZE];
			while(inbox_pop(local)) slave_handle_frame(local);	//all the blocks in the queue
		}

		// The CAN reception queue must ALWAYS be emptied, even when we are not collecting.
		// The body bus (BH) has continuous traffic even with the dashboard off: leaving the
		// queue full between one request and the next, at the beginning of the collection there were
		// old frames of that traffic inside it and the real answer risked arriving into
		// an already saturated queue. Outside the collection the frames are read and thrown away.
		CAN_RxHeaderTypeDef h;
		uint8_t d[8];
		while(is_can_msg_pending(CAN_RX_FIFO0)){
			if(can_rx(&h, d) != HAL_OK) break;
			if(h.RTR == CAN_RTR_DATA && slaveCollecting) slave_offer_rx(&h, d);
		}
		can_process();

		if(slaveCollecting && currentTime >= slaveDeadline){
			slaveCollecting = 0;
			slave_send_end(slaveGotFrames ? 0 : 1);
		}
	#endif
}

#endif //C1baccable || C2baccable || BHbaccable
