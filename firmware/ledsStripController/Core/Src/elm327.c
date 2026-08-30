/*
 * elm327.c
 *
 *  ELM327 emulation (ISO 15765-4) on the USB CDC port of the BACCAble.
 *  The bus speed is chosen at compile time (ELM327_BITRATE_KBPS).
 *  See elm327.h for the porting notes from the ESP32-C3 version.
 *
 *  Flow:
 *    cdc_process()   -> elm327_rx_byte()  (queueing only, runs with irq disabled)
 *    main loop       -> elm327_process()  (interprets and executes one command at a time)
 */

#include "elm327.h"

#ifdef ACT_AS_ELM327

#include <string.h>
#include "globalVariables.h"
#include "usbd_cdc_if.h"
#include "onboardLed.h"
#if defined(C1baccable)
	#include "usb_device.h"		//the USB port is switched on only when needed
#endif
#if defined(C1baccable)
	#include "elmlink.h"	//bridge towards the chips of the C2 and BH buses
#endif

// ------------------------------------------------------------------ ELM327 state
#ifdef ELM327_STRICT_ELM_DEFAULTS
	#define ELM_DEF_ECHO	1	//echo on, like the original ELM327 after the reset
	#define ELM_DEF_LF		0	//line feeds off, like the original ELM327 after the reset
	#ifndef ELM327_EXTRA_CR_BEFORE_PROMPT
		#define ELM327_EXTRA_CR_BEFORE_PROMPT	//empty line before the prompt
	#endif
#else
	#define ELM_DEF_ECHO	0
	#define ELM_DEF_LF		1
#endif

static uint8_t  echoOn			= ELM_DEF_ECHO;
static uint8_t  headersOn		= 0;
static uint8_t  linefeedOn		= ELM_DEF_LF;
static uint8_t  spacesOn		= 1;
static uint8_t  cafOn			= 1;			//ATCAF0/1: 0 = raw mode, the frames go through exactly as they are (AlfaOBD)
static uint8_t  cfcOn			= 1;			//if 0 we do not send the Flow Control to the ecu
static uint8_t  adaptTiming		= 1;			//only stored (ATAT0/1/2)
static uint32_t canFilterValue	= 0;			//software filter set with ATCRA / ATCF
static uint32_t canFilterMask	= 0;			//0 = accept everything (ATCM)
static uint32_t canSendHeader	= 0x7DF;		//transmission header (ATSH)
static uint8_t  extendedHeader	= 0;			//1 = 29 bit id
static uint8_t  canPriority		= 0x18;			//ATCP, used to complete the 29 bit ids
static uint16_t cmdTimeout		= ELM327_DEFAULT_TIMEOUT_MS;
static uint16_t rspTimeout		= ELM327_DEFAULT_TIMEOUT_MS;	//timeout of the attempt in progress

//programmable parameters used by MultiECUScan to choose the bus speed:
//  PP 2C = options of protocol B (USER1)   PP 2D = baud rate divisor of protocol B
//  PP 2E = options of protocol C (USER2)   PP 2F = baud rate divisor of protocol C
//the bitrate is 500 kbit/s divided by the divisor (01 = 500k, 02 = 250k, 04 = 125k, 0A = 50k).
static uint8_t  pp2C			= 0x00;
static uint8_t  pp2D			= 0x01;
static uint8_t  pp2E			= 0x00;
static uint8_t  pp2F			= 0x01;
static uint8_t  variableDlc		= 0;			//ATV1 = DLC equal to the useful bytes, ATV0 = padding to 8
static uint8_t  busOpen			= 0;			//the CAN bus is opened only when it is really needed
static uint8_t  busDivisor		= 0;			//divisor requested (0 = the one of the build)

#if defined(C1baccable)
	// Mode switched on from the cluster menu. While off the interpreter is inert: no byte
	// read from the USB, no frame on the bus, no message on the line between the chips.
	static uint8_t  elmModeOn   = 0;
	static uint32_t elmLastCmd  = 0;	//when the last command from the PC arrived
#endif

#if defined(C1baccable)
// --- gateway towards the other two chips (see elmlink.h) ---
// The bus active for the request in progress and the memory of which ecu is on which
// network: the first time it is tried, then the answer always comes from the right bus.
static uint8_t  activeBus = ELMLINK_BUS_LOCAL;

// The serial line is a single wire: whoever transmits does not hear. The rule that eliminates
// the collisions is that THE MASTER NEVER TRANSMITS until the slave has said "I am done"
// (END block) or a margin longer than the slave timeout has expired. Before, the two
// timeouts were identical: the slave sent END at the exact instant the master
// was transmitting the next command, a byte was lost and the framing stayed out of phase
// forever (all the remote buses dead after the first writing session).
static uint8_t  remoteEnded = 1;		//1 = the slave has closed, the line is free

//the last configuration sent to the slave: it is sent again only if it changes (less traffic,
//fewer collision windows, faster commands)
static uint8_t  cfgSentBus = 0xFF;
static uint32_t cfgSentFilter, cfgSentMask; static uint16_t cfgSentTimeout; static uint8_t cfgSentFc;

typedef struct { uint16_t addr; uint8_t bus; } elm_route_t;
static elm_route_t routeCache[ELM327_ROUTE_CACHE_LEN];
static uint8_t     routeCount = 0;

//frames received from the remote bus, waiting to be read as if they were local
typedef struct { uint32_t id; uint8_t ext; uint8_t dlc; uint8_t data[8]; } elm_remote_frame_t;
static elm_remote_frame_t remoteFifo[ELM327_REMOTE_FIFO_LEN];
static uint8_t remoteHead = 0, remoteTail = 0;

static void elm_remote_push(uint32_t id, uint8_t ext, const uint8_t *d, uint8_t dlc){
	uint8_t next = (uint8_t)((remoteHead + 1) % ELM327_REMOTE_FIFO_LEN);
	if(next == remoteTail) return;	//queue full: it is discarded
	remoteFifo[remoteHead].id  = id;
	remoteFifo[remoteHead].ext = ext;
	remoteFifo[remoteHead].dlc = (dlc > 8) ? 8 : dlc;
	memcpy(remoteFifo[remoteHead].data, d, 8);
	remoteHead = next;
}

static void elm_remote_clear(void){ remoteHead = remoteTail = 0; }

//address of the ecu being queried: last useful byte of the transmission header
static uint16_t elm_target_addr(void){
	if(extendedHeader) return (uint16_t)((canSendHeader >> 8) & 0xFF);
	return (uint16_t)(canSendHeader & 0x7FF);
}

static uint8_t elm_route_lookup(uint16_t addr, uint8_t *bus){
	for(uint8_t i = 0; i < routeCount; i++){
		if(routeCache[i].addr == addr){ *bus = routeCache[i].bus; return 1; }
	}
	return 0;
}

static void elm_route_store(uint16_t addr, uint8_t bus){
	for(uint8_t i = 0; i < routeCount; i++){
		if(routeCache[i].addr == addr){ routeCache[i].bus = bus; return; }
	}
	if(routeCount < ELM327_ROUTE_CACHE_LEN){
		routeCache[routeCount].addr = addr;
		routeCache[routeCount].bus  = bus;
		routeCount++;
	}
}

// Order in which to try the buses. If the ecu is already known we go straight to it; otherwise
// we start from the local bus, unless the program has asked for a speed different from
// 500k: in that case the network is the body one (BH), which runs at 125 kbit/s.
static uint8_t elm_bus_order(uint8_t *order){
	uint8_t known;
	if(elm_route_lookup(elm_target_addr(), &known)){
		order[0] = known;
		return 1;
	}
	if(busDivisor > 1){
		order[0] = ELMLINK_BUS_BH; order[1] = ELMLINK_BUS_C2; order[2] = ELMLINK_BUS_LOCAL;
	}else{
		order[0] = ELMLINK_BUS_LOCAL; order[1] = ELMLINK_BUS_C2; order[2] = ELMLINK_BUS_BH;
	}
	return 3;
}
#endif //C1baccable

//declared protocol: the real ELM327 answers ATDP/ATDPN with the one selected with ATSP.
//Always answering "AB" (user protocol) confuses tools like AlfaOBD.
static uint8_t  protoNum		= 6;			//6 = ISO 15765-4 (CAN 11/500)
static uint8_t  protoAuto		= 1;			//1 = automatic search ("A" in front of the number)
static uint32_t lastRxByteTime	= 0;			//to discard a command left half way

//configurable flow control (ATFCSH / ATFCSD / ATFCSM), used by AlfaOBD
static uint32_t fcHeader		= 0;			//0 = use the current transmission header
static uint8_t  fcHeaderExt		= 0;
static uint8_t  fcData[8]		= {0x30, 0x00, 0x00, 0, 0, 0, 0, 0};
static uint8_t  fcDataLen		= 3;
static uint8_t  fcMode			= 0;			//0 = automatic, 1/2 = use the header and data set above

// ------------------------------------------------------------------ usb input buffer
static volatile uint8_t  rxRing[ELM327_RX_RING_LEN];
static volatile uint16_t rxHead = 0;
static volatile uint16_t rxTail = 0;

static char    cmdBuf[ELM327_CMD_BUF_LEN];
static uint8_t cmdLen = 0;

// ------------------------------------------------------------------ usb output buffer
// CDC_Transmit_FS() drops packets longer than TX_BUF_SIZE, so the output
// is accumulated in a small chunk and sent as soon as it fills up.
static uint8_t txChunk[ELM327_TX_CHUNK_LEN];
static uint8_t txChunkLen = 0;

// ------------------------------------------------------------------ recorder (ATLOG)
#ifndef ELM327_TRACE_DISABLE
static char     traceBuf[ELM327_TRACE_LEN];
static uint16_t traceHead    = 0;
static uint8_t  traceWrapped = 0;
static uint8_t  traceDumping = 0;	//while we print the log we do not record the output

static void trace_char(char c){
	if(traceDumping) return;
	traceBuf[traceHead++] = c;
	if(traceHead >= ELM327_TRACE_LEN){ traceHead = 0; traceWrapped = 1; }
}
static void trace_str(const char *s){
	while(*s) trace_char(*s++);
}
#else
	#define trace_char(c)	do{}while(0)
	#define trace_str(s)	do{}while(0)
#endif

static void elm_flush(void){
	if(!txChunkLen) return;
	// CDC_Transmit_FS waits at most 10ms and then returns USBD_BUSY discarding the data:
	// it happens when the host has just opened the port and is not reading yet (it is the reason
	// why the first commands after the connection could be lost). Here we retry.
	for(uint8_t attempt = 0; attempt < 5; attempt++){
		if(CDC_Transmit_FS(txChunk, txChunkLen) == USBD_OK) break;
	}
	txChunkLen = 0;
}

static void elm_putc(char c){
	trace_char(c);
	txChunk[txChunkLen++] = (uint8_t)c;
	if(txChunkLen >= ELM327_TX_CHUNK_LEN) elm_flush();
}

static void elm_puts(const char *s){
	while(*s) elm_putc(*s++);
}

static void elm_puthex(uint8_t v){
	const char *h = "0123456789ABCDEF";
	elm_putc(h[(v >> 4) & 0x0F]);
	elm_putc(h[v & 0x0F]);
}

//end of line according to the ATL0/ATL1 setting
static void elm_eol(void){
	elm_putc('\r');
	if(linefeedOn) elm_putc('\n');
}

static void elm_line(const char *s){
	elm_puts(s);
	elm_eol();
}

// ------------------------------------------------------------------ utility hex
static uint8_t elm_hexval(char c){
	if(c >= '0' && c <= '9') return (uint8_t)(c - '0');
	if(c >= 'A' && c <= 'F') return (uint8_t)(c - 'A' + 10);
	return 0xFF;
}

static uint8_t elm_is_hex_string(const char *s){
	if(strlen(s) < 2) return 0;
	while(*s){
		if(elm_hexval(*s) == 0xFF) return 0;
		s++;
	}
	return 1;
}

static uint32_t elm_parse_hex(const char *s){
	uint32_t v = 0;
	while(*s){
		uint8_t n = elm_hexval(*s);
		if(n == 0xFF) break;
		v = (v << 4) | n;
		s++;
	}
	return v;
}

// ------------------------------------------------------------------ CAN helper
// can_tx() of BACCAble only queues: here we queue and then "pump" the queue
// until the frame has really gone out (or the timeout expires).
static void elm_bus_open(void);	//forward declaration: the bus is opened at the first request

static uint8_t elm_can_send_frame_id(const uint8_t *bytes, uint8_t len, uint32_t id, uint8_t ext){
	CAN_TxHeaderTypeDef h;
	uint8_t data[8];

	elm_bus_open();	//first transmission: we join the bus now, not at power on

	if(len > 8) len = 8;
	memset(data, 0, sizeof(data));
	memcpy(data, bytes, len);

	h.RTR = CAN_RTR_DATA;
	h.DLC = len;
	h.TransmitGlobalTime = DISABLE;
	if(ext){
		h.IDE   = CAN_ID_EXT;
		h.ExtId = id & 0x1FFFFFFF;
		h.StdId = 0;
	}else{
		h.IDE   = CAN_ID_STD;
		h.StdId = id & 0x7FF;
		h.ExtId = 0;
	}

	if(can_tx(&h, data) != HAL_OK) return 0;

	//empties the transmission queue (can_process sends one frame per call)
	uint32_t t0 = currentTime;
	while(currentTime - t0 < 20){
		can_process();
		if(HAL_CAN_GetTxMailboxesFreeLevel(can_gethandle()) == 3) return 1;
	}
	return 1; //after the 20ms we consider it gone anyway: the response timeout will do the rest
}

//sends a frame on the active bus: the local one or, through the other chip, C2 or BH
static uint8_t elm_bus_send(const uint8_t *bytes, uint8_t len, uint32_t id, uint8_t ext){
	#if defined(C1baccable)
		if(activeBus != ELMLINK_BUS_LOCAL){
			//we do not wait here: the answers arrive in the waiting loop, one by one
			remoteEnded = 0;
			return elmlink_send_request(activeBus, id, ext, bytes, len);
		}
	#endif
	return elm_can_send_frame_id(bytes, len, id, ext);
}

// Keeps the transport running during the waits: on the local bus it empties the transmission
// queue, on a remote bus it collects the frames the slave sends back. In both cases the
// answer is then printed to the PC frame by frame, as soon as it is available: it is the reason
// why the long writes no longer time out on the program.
static void elm_bus_pump(void){
	#if defined(C1baccable)
		if(activeBus != ELMLINK_BUS_LOCAL){
			if(elmlink_poll(elm_remote_push)) remoteEnded = 1;	//the slave has closed
			return;
		}
	#endif
	can_process();
}

// Effective waiting timeout: on a remote bus the margin of the serial transport is added,
// so the master ALWAYS waits longer than the slave and the END arrives
// when the line is free (normally the wait ends much earlier, right at the END).
static uint16_t elm_wait_budget(void){
	#if defined(C1baccable)
		if(activeBus != ELMLINK_BUS_LOCAL) return (uint16_t)(rspTimeout + ELMLINK_DEFAULT_TIMEOUT_MS);
	#endif
	return rspTimeout;
}

//1 = the slave has said END: nothing else will arrive, no point in waiting
static uint8_t elm_remote_finished(void){
	#if defined(C1baccable)
		return (activeBus != ELMLINK_BUS_LOCAL && remoteEnded) ? 1 : 0;
	#endif
	return 0;
}

#if defined(C1baccable)
// Before giving control back (and therefore before MES sends the next command)
// the END of the slave is waited for: it is the "all clear" that makes the half-duplex line safe.
static void elm_remote_drain(void){
	if(activeBus == ELMLINK_BUS_LOCAL || remoteEnded) return;
	// The slave can legitimately stay armed until the timeout we told it
	// in the CFG (rspTimeout: with ATSTFE that is ~1016 ms): the drain must cover
	// ALL that time, not a fixed margin. A shorter margin meant going back to
	// transmitting with the slave still armed: the collision that killed the line.
	uint32_t t0 = currentTime;
	while(currentTime - t0 < (uint32_t)rspTimeout + ELMLINK_DEFAULT_TIMEOUT_MS){
		if(elmlink_poll(elm_remote_push)){ remoteEnded = 1; break; }
	}
	remoteEnded = 1;	//if the END was lost, the margin has passed anyway: line free
}
#else
	#define elm_remote_drain()	do{}while(0)
#endif

//sends an 8 byte frame with the current transmission header (ATSH)
static uint8_t elm_can_send_frame(const uint8_t *data8){
	return elm_bus_send(data8, 8, canSendHeader, extendedHeader);
}

// The bus stays closed until a program really asks for something: powering the
// board must never disturb the CAN lines of the car (a node at the wrong speed
// fills them with error frames). It is also the behaviour of the original ELM327.
// With the mode selectable from the cluster the C1 bus is NOT ours: the BACCAble keeps it
// open for its normal work (500 kbit/s, the right speed for this network), and
// closing it here would mean switching the dashboard off at every ATZ. We only keep count.
static void elm_bus_open(void){
	if(busOpen) return;
	#if !defined(C1baccable)
		can_enable();
	#endif
	busOpen = 1;
}

static void elm_bus_close(void){
	if(!busOpen) return;
	#if !defined(C1baccable)
		can_disable();
	#endif
	busOpen = 0;
}

// Changes the bus speed following the divisor of the programmable parameters:
// bitrate = 500 kbit/s / divisor, that is prescaler = 12 * divisor (48MHz / (presc * 8)).
// If the bus is already open it is closed and reopened, otherwise it is just prepared.
static void elm_apply_bitrate_divisor(uint8_t divisor){
	if(divisor == 0) divisor = 1;
	busDivisor = divisor;	//it is needed anyway to understand which bus the program wants

	#if defined(C1baccable)
		// WITH THE GATEWAY THE LOCAL BUS IS NOT TOUCHED.
		// Each chip is on a network with its own fixed speed (C1 and C2 at 500 kbit/s, BH at
		// 125): here the divisor asked for by the program says WHICH network it wants, not at what
		// speed to reconfigure ours. Reconfiguring the local bus at 125 kbit/s
		// while the C1 network runs at 500, our node starts firing error frames on the
		// network of the car and goes bus-off: from that moment nothing answers any more, not even
		// the ecus on the C buses. It was the reason why after the first body
		// ecu all the rest of the session failed.
		return;
	#else
		//without the gateway a single chip serves any speed: here the change is really done
		uint8_t wasOpen = busOpen;
		elm_bus_close();
		can_set_prescaler((uint32_t)12 * divisor);
		if(wasOpen) elm_bus_open();
	#endif
}

//true if the received id passes the filter set with ATCRA
static uint8_t elm_filter_pass(const CAN_RxHeaderTypeDef *h){
	if(canFilterMask == 0) return 1;
	uint32_t id = (h->IDE == CAN_ID_EXT) ? h->ExtId : h->StdId;
	return ((id & canFilterMask) == (canFilterValue & canFilterMask)) ? 1 : 0;
}

//receives a frame respecting the filter. 1 = frame available.
//If the request went out towards a remote bus, the frames come from the queue filled
//by the answers of the other chip, and all the rest of the code does not notice.
static uint8_t elm_can_get_frame(CAN_RxHeaderTypeDef *h, uint8_t *d){
	#if defined(C1baccable)
		if(activeBus != ELMLINK_BUS_LOCAL){
			while(remoteTail != remoteHead){
				elm_remote_frame_t *f = &remoteFifo[remoteTail];
				remoteTail = (uint8_t)((remoteTail + 1) % ELM327_REMOTE_FIFO_LEN);
				memset(h, 0, sizeof(*h));
				h->RTR = CAN_RTR_DATA;
				h->DLC = f->dlc;
				if(f->ext){ h->IDE = CAN_ID_EXT; h->ExtId = f->id; }
				else      { h->IDE = CAN_ID_STD; h->StdId = f->id; }
				memcpy(d, f->data, 8);
				if(!elm_filter_pass(h)) continue;
				return 1;
			}
			return 0;
		}
	#endif
	while(is_can_msg_pending(CAN_RX_FIFO0)){
		if(can_rx(h, d) != HAL_OK) continue;
		if(h->RTR != CAN_RTR_DATA) continue;
		if(!elm_filter_pass(h)) continue;
		return 1;
	}
	return 0;
}

//prints the header of the answer. With ATS1 the bytes are separated by a space (and there is a
//space before the data too), with ATS0 there is no separator, like the real ELM327.
static void elm_print_header(const CAN_RxHeaderTypeDef *h){
	if(!headersOn) return;

	#if defined(ELM327_HEADER_ALWAYS_SPACED)
		const uint8_t sep = 1;	//behaviour of the first version (spaces even with ATS0)
	#else
		const uint8_t sep = spacesOn;
	#endif

	if(h->IDE == CAN_ID_EXT){
		elm_puthex((h->ExtId >> 24) & 0xFF); if(sep) elm_putc(' ');
		elm_puthex((h->ExtId >> 16) & 0xFF); if(sep) elm_putc(' ');
		elm_puthex((h->ExtId >> 8)  & 0xFF); if(sep) elm_putc(' ');
		elm_puthex(h->ExtId & 0xFF);         if(sep) elm_putc(' ');
	}else{
		#ifdef ELM327_HEADER_3DIGITS
			const char *hx = "0123456789ABCDEF";
			elm_putc(hx[(h->StdId >> 8) & 0x0F]);
			elm_putc(hx[(h->StdId >> 4) & 0x0F]);
			elm_putc(hx[h->StdId & 0x0F]);
			if(sep) elm_putc(' ');
		#else
			elm_puthex((h->StdId >> 8) & 0xFF); if(sep) elm_putc(' ');
			elm_puthex(h->StdId & 0xFF);        if(sep) elm_putc(' ');
		#endif
	}
}

//sends the Flow Control to the ecu: by default ContinueToSend / BlockSize 0 / STmin 0,
//or the header and data set with ATFCSH / ATFCSD (ATFCSM different from 0).
static void elm_send_flow_control(void){
	#if defined(C1baccable)
		//on a remote bus the slave takes care of it, as soon as it sees the first frame go by
		if(activeBus != ELMLINK_BUS_LOCAL) return;
	#endif
	uint8_t fc[8];
	uint8_t len = 8;
	memset(fc, ELM327_PAD_BYTE, sizeof(fc));

	if(fcMode && fcDataLen){
		memcpy(fc, fcData, fcDataLen);
	}else{
		fc[0] = 0x30;
		fc[1] = 0x00;
		fc[2] = 0x00;
	}

	if(fcMode && fcHeader) elm_bus_send(fc, len, fcHeader, fcHeaderExt);
	else                   elm_bus_send(fc, len, canSendHeader, extendedHeader);
}

// ------------------------------------------------------------------ default
static void elm_reset_defaults(void){
	echoOn			= ELM_DEF_ECHO;
	headersOn		= 0;
	linefeedOn		= ELM_DEF_LF;
	spacesOn		= 1;
	cafOn			= 1;
	cfcOn			= 1;
	adaptTiming		= 1;
	canSendHeader	= 0x7DF;
	extendedHeader	= 0;
	canFilterValue	= 0;
	canFilterMask	= 0;
	cmdTimeout		= ELM327_DEFAULT_TIMEOUT_MS;
	canPriority		= 0x18;
	fcHeader		= 0;
	fcHeaderExt		= 0;
	fcData[0]		= 0x30; fcData[1] = 0x00; fcData[2] = 0x00;
	fcDataLen		= 3;
	fcMode			= 0;
	protoNum		= 6;
	protoAuto		= 1;
	variableDlc		= 0;
	elm_bus_close();	//ATZ = reset: we leave the bus, we join it again at the first request
	#if defined(C1baccable)
		cfgSentBus = 0xFF;		//after a reset the configuration has to be sent again
		//the ecu->bus map is NOT cleared: it is the wiring of the car, it does not change with
		//a reset. MultiECUScan sends an ATZ before every ecu, clearing it would have
		//meant probing all the buses again at every connection.
		activeBus  = ELMLINK_BUS_LOCAL;
		rspTimeout = cmdTimeout;
		elm_remote_clear();
	#endif
	//the programmable parameters are not cleared: on the original chip they live in non
	//volatile memory and survive an ATZ (MultiECUScan sets them right before an ATZ).
	cmdLen			= 0;
}

// Called when the host opens or closes the serial port (CDC_SET_CONTROL_LINE_STATE):
// it throws away the bytes left over from the previous session, which would otherwise mix
// with the first commands of the new connection.
void elm327_port_reset(void){
	#if defined(C1baccable)
		if(!elmModeOn) return;		//mode off: there is nothing to clear
	#endif
	trace_str("\r\n== porta aperta/chiusa ==");
	elm_bus_close();	//no program connected: we leave the bus and do not disturb the car
	rxHead     = rxTail;
	cmdLen     = 0;
	txChunkLen = 0;
}

void elm327_init(void){
	elm_reset_defaults();
	rxHead = 0;
	rxTail = 0;
	txChunkLen = 0;
}

#if defined(C1baccable)
// ------------------------------------------------- switching on from the cluster menu
// While off none of this is called at all: it is as if the ELM327 were not there.
// While on the BACCAble suspends its normal work (main.c) and the chip behaves
// exactly like the dedicated ELM327 firmware, gateway towards C2 and BH included.
void elm327_set_enabled(uint8_t on){
	on = on ? 1 : 0;
	if(on == elmModeOn) return;

	elmModeOn  = on;
	elmLastCmd = currentTime;

	elm327_init();			//clean interpreter state at every switch on
	rxHead = rxTail = 0;	//and away with the bytes left on the port

	// THE USB PORT IS SWITCHED ON ONLY NOW.
	// While the mode is off the connector stays silent and the computer sees
	// no device: the BACCAble must not let itself be recognised as an ELM327 interface
	// if nobody asked for it from the cluster. Every switch on redoes the initialisation
	// from scratch (see usb_device.c: after a switch off the USB block stays in power-down).
	if(on)	usb_device_attach();
	else	usb_device_detach();

	#if defined(C1baccable)
		//switches the bridge on (or off) here and on the other two chips
		elmlink_set_enabled(on);
		if(on){
			elmlink_send_arm(1);
		}else{
			elmlink_send_arm(0);
			elmlink_set_enabled(0);
		}
	#endif
}

uint8_t elm327_is_enabled(void){ return elmModeOn; }
#endif //C1baccable

// ------------------------------------------------------------------ input from usb
void elm327_rx_byte(uint8_t c){
	#if defined(C1baccable)
		if(!elmModeOn) return;		//mode off: the USB port does not interest us
	#endif
	uint16_t next = (uint16_t)((rxHead + 1) % ELM327_RX_RING_LEN);
	if(next == rxTail) return; //buffer full: discard (the tool retries)
	rxRing[rxHead] = c;
	rxHead = next;
}

static int16_t elm_ring_get(void){
	if(rxTail == rxHead) return -1;
	uint8_t c = rxRing[rxTail];
	rxTail = (uint16_t)((rxTail + 1) % ELM327_RX_RING_LEN);
	return (int16_t)c;
}

// ------------------------------------------------------------------ AT commands
static void elm_handle_at(const char *at){

	if(!strcmp(at, "Z")){					//full reset
		elm_reset_defaults();
		HAL_Delay(50);
		elm_puts(ELM327_ID_STRING); elm_eol();
		return;
	}
	if(!strcmp(at, "WS")){ HAL_Delay(50); elm_line(ELM327_ID_STRING);	return; }
	if(!strcmp(at, "I"))  { elm_line(ELM327_ID_STRING);					return; }
	if(!strcmp(at, "@1")) { elm_line(ELM327_DESCR_STRING);				return; }
	if(!strcmp(at, "@2")) { elm_line("?");								return; }	//identifier not programmed, like an original chip
	if(!strcmp(at, "D"))  { elm_reset_defaults(); elm_line("OK"); return; }

	if(!strcmp(at, "E0")) { echoOn = 0;		elm_line("OK"); return; }
	if(!strcmp(at, "E1")) { echoOn = 1;		elm_line("OK"); return; }
	if(!strcmp(at, "L0")) { linefeedOn = 0;	elm_line("OK"); return; }
	if(!strcmp(at, "L1")) { linefeedOn = 1;	elm_line("OK"); return; }
	if(!strcmp(at, "H0")) { headersOn = 0;	elm_line("OK"); return; }
	if(!strcmp(at, "H1")) { headersOn = 1;	elm_line("OK"); return; }
	if(!strcmp(at, "S0")) { spacesOn = 0;	elm_line("OK"); return; }
	if(!strcmp(at, "S1")) { spacesOn = 1;	elm_line("OK"); return; }

	if(!strcmp(at, "V0")) { variableDlc = 0; elm_line("OK"); return; }	//frames padded up to 8 bytes
	if(!strcmp(at, "V1")) { variableDlc = 1; elm_line("OK"); return; }	//DLC equal to the useful bytes only

	//commands accepted with no real effect on this hardware
	if(!strcmp(at, "AR") || !strcmp(at, "AL") || !strcmp(at, "NL") ||
	   !strcmp(at, "BI") || !strcmp(at, "PC") || !strcmp(at, "MA")){
		elm_line("OK"); return;
	}

	//ATDP / ATDPN: they report the protocol selected with ATSP (not a fixed value)
	if(!strcmp(at, "DP")){
		switch(protoNum){
			case 6:  elm_line("ISO 15765-4 (CAN 11/500)"); break;
			case 7:  elm_line("ISO 15765-4 (CAN 29/500)"); break;
			case 8:  elm_line("ISO 15765-4 (CAN 11/250)"); break;
			case 9:  elm_line("ISO 15765-4 (CAN 29/250)"); break;
			case 0xA:elm_line("SAE J1939 (CAN 29/250)");   break;
			case 0xB: case 0xC: {	//user protocols: speed = 500 / divisor
				uint8_t  div = (protoNum == 0xB) ? pp2D : pp2F;
				uint16_t kb  = (uint16_t)(500 / (div ? div : 1));
				elm_puts((protoNum == 0xB) ? "USER1 (CAN " : "USER2 (CAN ");
				if(kb >= 100) elm_putc((char)('0' + (kb / 100)));
				if(kb >= 10)  elm_putc((char)('0' + ((kb / 10) % 10)));
				elm_putc((char)('0' + (kb % 10)));
				elm_line(")");
				break; }
			default: elm_line("ISO 15765-4 (CAN 29/" ELM327_BITRATE_STR ")"); break;
		}
		return;
	}
	if(!strcmp(at, "DPN")){
		const char *hx = "0123456789ABCDEF";
		if(protoAuto) elm_putc('A');
		elm_putc(hx[protoNum & 0x0F]);
		elm_eol();
		return;
	}
	if(!strcmp(at, "RV"))  { elm_line("12.3V");						return; } //no divider on the 12V: fixed value

	if(!strcmp(at, "CAF0")){ cafOn = 0; elm_line("OK"); return; }
	if(!strcmp(at, "CAF1")){ cafOn = 1; elm_line("OK"); return; }
	if(!strcmp(at, "CFC0")){ cfcOn = 0; elm_line("OK"); return; }
	if(!strcmp(at, "CFC1")){ cfcOn = 1; elm_line("OK"); return; }

	//ATAT0/1/2 (adaptive timing): the full command is "ATAT<n>", here "AT<n>" arrives
	if(strlen(at) == 3 && at[0] == 'A' && at[1] == 'T'){
		uint8_t v = (uint8_t)(at[2] - '0');
		if(v <= 2){ adaptTiming = v; elm_line("OK"); }
		else       { elm_line("?"); }
		return;
	}

	if(!strncmp(at, "ST", 2)){					//response timeout, 4ms step
		uint32_t v = elm_parse_hex(at + 2);
		cmdTimeout = (v == 0) ? ELM327_DEFAULT_TIMEOUT_MS : (uint16_t)(v * 4);
		elm_line("OK"); return;
	}
	if(!strncmp(at, "CP", 2)){					//priority for the 29 bit ids
		canPriority = (uint8_t)elm_parse_hex(at + 2);
		elm_line("OK"); return;
	}
	if(!strncmp(at, "CRA", 3)){					//software reception filter
		const char *a = at + 3;
		uint8_t n = (uint8_t)strlen(a);
		if(n == 0){
			canFilterValue = 0;
			canFilterMask  = 0;					//accept everything
		}else{
			uint32_t val = 0, msk = 0;
			for(uint8_t i = 0; i < n; i++){
				uint8_t d = elm_hexval(a[i]);
				val <<= 4; msk <<= 4;
				if(d != 0xFF){ val |= d; msk |= 0x0F; }	//the 'X' stay wildcards
			}
			canFilterValue = val;
			canFilterMask  = msk;
		}
		elm_line("OK"); return;
	}
	if(!strncmp(at, "SH", 2)){					//transmission header
		const char *a = at + 2;
		uint8_t n = (uint8_t)strlen(a);
		uint32_t addr = elm_parse_hex(a);
		if(n > 3){
			canSendHeader  = (n == 8) ? addr : (((uint32_t)canPriority << 24) | (addr & 0x00FFFFFF));
			extendedHeader = 1;
		}else{
			canSendHeader  = addr;
			extendedHeader = 0;
		}
		elm_line("OK"); return;
	}

	//ATFCSH xxx / xxxxxxxx : header used for the Flow Control frames (used by AlfaOBD)
	if(!strncmp(at, "FCSH", 4)){
		const char *a = at + 4;
		uint8_t n = (uint8_t)strlen(a);
		if(n == 0){ fcHeader = 0; fcHeaderExt = 0; }
		else{
			uint32_t addr = elm_parse_hex(a);
			if(n > 3){
				fcHeader	= (n == 8) ? addr : (((uint32_t)canPriority << 24) | (addr & 0x00FFFFFF));
				fcHeaderExt	= 1;
			}else{
				fcHeader	= addr;
				fcHeaderExt	= 0;
			}
		}
		elm_line("OK"); return;
	}
	//ATFCSD hh hh hh... : data of the Flow Control frame (max 5 bytes, usually 30 00 00)
	if(!strncmp(at, "FCSD", 4)){
		const char *a = at + 4;
		uint8_t n = (uint8_t)strlen(a);
		uint8_t k = 0;
		for(uint8_t i = 0; (i + 1) < n && k < 5; i += 2)
			fcData[k++] = (uint8_t)((elm_hexval(a[i]) << 4) | elm_hexval(a[i + 1]));
		if(k == 0){ fcData[0] = 0x30; fcData[1] = 0x00; fcData[2] = 0x00; k = 3; }
		fcDataLen = k;
		elm_line("OK"); return;
	}
	//ATFCSM n : 0 = automatic, 1/2 = use the header and data set above
	if(!strncmp(at, "FCSM", 4)){
		uint8_t v = (uint8_t)(at[4] - '0');
		fcMode = (v <= 2) ? v : 0;
		elm_line("OK"); return;
	}

	//ATCF xxx / xxxxxxxx : value of the reception filter (used together with ATCM)
	if(!strncmp(at, "CF", 2)){
		const char *a = at + 2;
		if(strlen(a) == 0){ canFilterValue = 0; canFilterMask = 0; }
		else{
			canFilterValue = elm_parse_hex(a);
			if(canFilterMask == 0) canFilterMask = (strlen(a) > 3) ? 0x1FFFFFFF : 0x7FF;
		}
		elm_line("OK"); return;
	}
	//ATCM xxx / xxxxxxxx : mask of the reception filter
	if(!strncmp(at, "CM", 2)){
		const char *a = at + 2;
		canFilterMask = (strlen(a) == 0) ? 0 : elm_parse_hex(a);
		elm_line("OK"); return;
	}

	//ATSPx / ATSPAx / ATTPx: the bus stays the compiled one, but we store the declared
	//protocol so that ATDP and ATDPN answer consistently.
	//ATPPxxSVyy / ATPPxxON / ATPPxxOFF: programmable parameters.
	//We care about 2D and 2F, the baud rate divisors of the user protocols B and C: it is with
	//these that MultiECUScan asks for a different bus speed, before doing ATSPB.
	if(!strncmp(at, "PP", 2)){
		const char *a = at + 2;
		if(elm_hexval(a[0]) != 0xFF && elm_hexval(a[1]) != 0xFF){
			uint8_t     pn   = (uint8_t)((elm_hexval(a[0]) << 4) | elm_hexval(a[1]));
			const char *rest = a + 2;
			if(!strncmp(rest, "SV", 2)){
				uint8_t v = (uint8_t)((elm_hexval(rest[2]) << 4) | elm_hexval(rest[3]));
				switch(pn){
					case 0x2C: pp2C = v; break;
					case 0x2D: pp2D = v; break;
					case 0x2E: pp2E = v; break;
					case 0x2F: pp2F = v; break;
					default: break;
				}
			}
			//ON / OFF: we consider the activation implicit, we answer OK
		}
		elm_line("OK"); return;
	}

	if(!strncmp(at, "SP", 2) || !strncmp(at, "TP", 2)){
		const char *a = at + 2;
		if(*a == 'A'){ protoAuto = 1; a++; }
		else           protoAuto = 0;
		uint8_t v = elm_hexval(*a);
		if(v == 0xFF){ elm_line("OK"); return; }	//no digit: we leave it as it is
		if(v == 0){ protoAuto = 1; }				//0 = automatic search
		else       { protoNum = v; }

		//the user protocols B and C use the divisor of the programmable parameters, the others
		//the standard speeds: it is here that the bus speed really changes.
		switch(protoNum){
			case 6: case 7:  elm_apply_bitrate_divisor(1);    break;	//CAN 500 kbit/s
			case 8: case 9:  elm_apply_bitrate_divisor(2);    break;	//CAN 250 kbit/s
			case 0xA:        elm_apply_bitrate_divisor(2);    break;	//J1939, 250 kbit/s
			case 0xB:        elm_apply_bitrate_divisor(pp2D); break;	//USER1
			case 0xC:        elm_apply_bitrate_divisor(pp2F); break;	//USER2
			default: break;
		}
		elm_line("OK"); return;
	}

	//ATIB/ATFC/ATBRT: accepted with no effect
	if(!strncmp(at, "IB", 2) || !strncmp(at, "FC", 2) || !strncmp(at, "BRT", 3)){
		elm_line("OK"); return;
	}

	if(!strncmp(at, "BRD", 3)){
		// On USB CDC the baud rate is virtual: we only perform the handshake foreseen
		// by the ELM327 (OK -> id string -> wait for CR from the tool -> OK).
		elm_puts("OK"); elm_eol();
		elm_flush();
		HAL_Delay(15);
		elm_puts(ELM327_ID_STRING); elm_eol();
		elm_flush();

		uint32_t t0 = currentTime;
		while(currentTime - t0 < 2000){
			cdc_process();						//keeps collecting the bytes from the usb
			int16_t c = elm_ring_get();
			if(c < 0) continue;
			if(c == '\r' || c == '\n') break;
		}
		while(elm_ring_get() >= 0);				//discards the rest
		cmdLen = 0;
		elm_puts("OK"); elm_eol();
		return;
	}

	#ifndef ELM327_TRACE_DISABLE
	//ATLOG: prints the recorded dialog (it is not an ELM327 command, it serves for diagnosis)
	if(!strcmp(at, "LOG")){
		traceDumping = 1;
		elm_line("---- registro ----");
		if(traceWrapped){
			for(uint16_t i = traceHead; i < ELM327_TRACE_LEN; i++) elm_putc(traceBuf[i]);
		}
		for(uint16_t i = 0; i < traceHead; i++) elm_putc(traceBuf[i]);
		elm_eol();
		elm_line("---- fine registro ----");
		traceDumping = 0;
		return;
	}
	if(!strcmp(at, "LOGC")){
		traceHead = 0; traceWrapped = 0;
		elm_line("OK");
		return;
	}
	#endif

	// Unrecognised AT command. The original ELM327 answers "?", but several tools
	// break off the connection as soon as they receive it: by default we answer "OK".
	#ifdef ELM327_UNKNOWN_AT_IS_ERROR
		elm_line("?");
	#else
		elm_line("OK");
	#endif
}

// ---- raw mode (ATCAF0): used by AlfaOBD --------------------------------------
// With the auto formatting off the ELM327 adds nothing and removes nothing:
// it transmits the bytes exactly as it receives them (PCI byte included) and prints the
// received frames one per line, just as they are. The ISO-TP handling (fragmentation
// and flow control) is done by the program on the PC.
static uint8_t elm_handle_obd_raw(const uint8_t *payload, uint8_t payloadLen, uint8_t expectedResponses){
	if(!elm_bus_send(payload, payloadLen, canSendHeader, extendedHeader)){
		return 0;
	}

	CAN_RxHeaderTypeDef resp;
	uint8_t  rd[8];
	uint8_t  printed = 0;
	uint8_t  sawPending = 0;	//1 if the ecu has already answered "I am working on it"
	uint32_t t0    = currentTime;
	uint32_t start = currentTime;

	//absolute limit: on a very busy bus and without a filter one never gets out
	uint16_t waitMs = elm_wait_budget();
	while((currentTime - t0 < waitMs) &&
	      (currentTime - start < (uint32_t)waitMs * 4 + 100) &&
	      (printed < ELM327_MAX_RAW_FRAMES)){
		elm_bus_pump();
		if(elm_remote_finished()) break;	//the slave has closed: the line is already free
		if(!elm_can_get_frame(&resp, rd)) continue;

		onboardLed_blue_on();
		elm_print_header(&resp);
		for(uint8_t i = 0; i < resp.DLC && i < 8; i++){
			if(spacesOn && i > 0) elm_putc(' ');
			elm_puthex(rd[i]);
		}
		elm_eol();
		// Sent straight away, without waiting for the prompt: this is how the real ELM327 behaves
		// and the programs count on it to understand that the ecu is answering.
		// Keeping the line in the buffer made the timeout of the program expire on the long
		// writes, where almost a second passes between "I am working on it" and the confirmation.
		elm_flush();
		printed++;

		//if the ecu sends a First Frame and the automatic flow control is on, we answer it
		if((rd[0] & 0xF0) == 0x10 && cfcOn) elm_send_flow_control();

		//if the tool has said how many answers to expect, we stop as soon as we have them
		if(expectedResponses && printed >= expectedResponses){ elm_remote_drain(); return 1; }

		// Typical pair of the writes (proxi alignment): first "I am working on it"
		// (7F xx 78), then the real answer. As soon as the second one arrives we close immediately
		// instead of listening until the timeout: it is that delay that made
		// the wait of the program expire and sent the answers out of sync.
		if((rd[0] & 0xF0) == 0x00){
			if(rd[1] == 0x7F && rd[3] == 0x78) sawPending = 1;
			else if(sawPending){ elm_remote_drain(); return 1; }
		}

		t0 = currentTime;	//there are more frames coming: the wait restarts
	}

	elm_remote_drain();		//line free before giving the prompt back to the program
	return printed ? 1 : 0;
}

// ---- automatic mode (ATCAF1): PCI added and ISO-TP answer reassembled -------------
static uint8_t elm_handle_obd_caf(const uint8_t *payload, uint16_t payloadLen, uint8_t expectedResponses){
	(void)expectedResponses;
	uint8_t frame[8];

	if(payloadLen <= 7){
		// ---- Single Frame ----
		memset(frame, ELM327_PAD_BYTE, sizeof(frame));
		frame[0] = (uint8_t)payloadLen;
		memcpy(&frame[1], payload, payloadLen);
		//ATV1: DLC equal to the useful bytes only; ATV0 (default): padding up to 8
		uint8_t dlc = variableDlc ? (uint8_t)(payloadLen + 1) : 8;
		if(!elm_bus_send(frame, dlc, canSendHeader, extendedHeader)){
			return 0;
		}
	}else{
		// ---- First Frame + Consecutive Frames (requests longer than 7 bytes) ----
		memset(frame, ELM327_PAD_BYTE, sizeof(frame));
		frame[0] = (uint8_t)(0x10 | ((payloadLen >> 8) & 0x0F));
		frame[1] = (uint8_t)(payloadLen & 0xFF);
		memcpy(&frame[2], payload, 6);
		if(!elm_can_send_frame(frame)){ return 0; }

		//wait for the Flow Control of the ecu
		uint8_t  blockSize = 0, stMin = 0;
		uint8_t  gotFC = 0;
		CAN_RxHeaderTypeDef h;
		uint8_t  d[8];
		uint32_t t0 = currentTime;
		while(currentTime - t0 < ELM327_FC_TIMEOUT_MS){
			can_process();
			if(!elm_can_get_frame(&h, d)) continue;
			if((d[0] & 0xF0) != 0x30) continue;
			if((d[0] & 0x0F) == 1){ t0 = currentTime; continue; }	//FC.WAIT
			if((d[0] & 0x0F) == 2){ return 0; }	//FC.OVFLW
			blockSize = d[1];
			stMin     = d[2];
			gotFC     = 1;
			break;
		}
		if(!gotFC){ return 0; }
		if(stMin > 127) stMin = 1;	//values 0xF1..0xF9 are microseconds: we round up to 1ms

		uint16_t sent = 6;
		uint8_t  sn   = 1;
		uint8_t  inBlock = 0;
		while(sent < payloadLen){
			uint8_t toCopy = (uint8_t)((payloadLen - sent) > 7 ? 7 : (payloadLen - sent));
			memset(frame, ELM327_PAD_BYTE, sizeof(frame));
			frame[0] = (uint8_t)(0x20 | (sn & 0x0F));
			memcpy(&frame[1], &payload[sent], toCopy);
			if(!elm_can_send_frame(frame)){ return 0; }
			sent += toCopy;
			sn++;
			if(stMin) HAL_Delay(stMin);

			//if the ecu asked for limited blocks, we wait for the next Flow Control
			if(blockSize){
				inBlock++;
				if(inBlock >= blockSize && sent < payloadLen){
					inBlock = 0;
					uint8_t again = 0;
					uint32_t tf = currentTime;
					while(currentTime - tf < ELM327_FC_TIMEOUT_MS){
						can_process();
						if(!elm_can_get_frame(&h, d)) continue;
						if((d[0] & 0xF0) != 0x30) continue;
						if((d[0] & 0x0F) == 1){ tf = currentTime; continue; }
						if((d[0] & 0x0F) == 2){ return 0; }
						blockSize = d[1];
						stMin     = (d[2] > 127) ? 1 : d[2];
						again     = 1;
						break;
					}
					if(!again){ return 0; }
				}
			}
		}
	}

	// ------------------ waiting for and reassembling the answer ------------------
	static uint8_t isoData[ELM327_ISOTP_MAX_LEN];
	uint16_t isoTotal    = 0;
	uint16_t isoReceived = 0;
	uint8_t  isoStarted  = 0;
	uint8_t  isoSN       = 1;

	CAN_RxHeaderTypeDef resp;
	uint8_t  rd[8];
	uint8_t  answered = 0;		//1 if something has already been passed to the program
	uint32_t t0 = currentTime;

	uint16_t waitMs = elm_wait_budget();
	while(currentTime - t0 < waitMs){
		elm_bus_pump();
		if(elm_remote_finished()) break;	//the slave has closed: nothing else will arrive								//local transmission queue or frame from the remote bus
		if(!elm_can_get_frame(&resp, rd)) continue;

		uint8_t pci = rd[0];

		if((pci & 0xF0) == 0x00){
			// ---- Single Frame ----
			uint8_t len = pci & 0x0F;
			if(len > 7) len = 7;
			onboardLed_blue_on();
			elm_print_header(&resp);
			for(uint8_t i = 1; i <= len; i++){
				if(spacesOn && i > 1) elm_putc(' ');
				elm_puthex(rd[i]);
			}
			elm_eol();
			elm_flush();	//sent straight away, as the real ELM327 does

			// "Request received, I am working on it" (7F xx 78): it is not the final answer.
			// It is passed to the program straight away (so it knows the ecu is there) and we
			// keep waiting for the real one, which in the long writes such as
			// the proxi alignment arrives even half a second later.
			if(rd[1] == 0x7F && rd[3] == 0x78){
				answered = 1;
				t0 = currentTime;
				continue;
			}
			elm_remote_drain();
			return 1;

		}else if((pci & 0xF0) == 0x10){
			// ---- First Frame ----
			isoTotal    = (uint16_t)(((uint16_t)(pci & 0x0F) << 8) | rd[1]);
			if(isoTotal > ELM327_ISOTP_MAX_LEN) isoTotal = ELM327_ISOTP_MAX_LEN;
			isoReceived = 0;
			isoStarted  = 1;
			isoSN       = 1;
			uint8_t toCopy = (uint8_t)(isoTotal < 6 ? isoTotal : 6);
			for(uint8_t i = 0; i < toCopy; i++) isoData[isoReceived++] = rd[2 + i];
			if(cfcOn) elm_send_flow_control();
			t0 = currentTime;

		}else if((pci & 0xF0) == 0x20 && isoStarted){
			// ---- Consecutive Frame ----
			if((pci & 0x0F) != (isoSN & 0x0F)){ return 0; }
			isoSN++;
			uint16_t remaining = (uint16_t)(isoTotal - isoReceived);
			uint8_t  toCopy    = (uint8_t)(remaining > 7 ? 7 : remaining);
			for(uint8_t i = 0; i < toCopy && isoReceived < ELM327_ISOTP_MAX_LEN; i++)
				isoData[isoReceived++] = rd[1 + i];
			t0 = currentTime;

			if(isoReceived >= isoTotal){
				onboardLed_blue_on();
				elm_print_header(&resp);
				for(uint16_t i = 0; i < isoReceived; i++){
					if(spacesOn && i > 0) elm_putc(' ');
					elm_puthex(isoData[i]);
				}
				elm_eol();
				elm_flush();
				elm_remote_drain();
				return 1;
			}
		}
		//the incoming Flow Control frames (0x30) are ignored
	}

	elm_remote_drain();		//line free before giving the prompt back to the program
	//if at least one "I am working on it" went through, the program did get an answer
	return answered;
}

// ------------------------------------------------------------------ OBD commands (hex)
// Chooses the bus and passes the request to the right handler. With the gateway active, if
// nobody answers on the local bus the same request is forwarded to the other two chips; the
// bus that answers is remembered, so that the following requests go straight there.
static void elm_handle_obd(const char *hexCmd){
	uint8_t  payload[32];
	uint16_t payloadLen = 0;
	uint16_t l = (uint16_t)strlen(hexCmd);
	uint8_t  expectedResponses = 0;

	// An extra final digit indicates the number of expected answers (e.g. "0100 1"), but it only
	// applies with the automatic formatting on: with ATCAF0 every byte is raw data and
	// the original ELM327 refuses the command with "?". Verified on the log of a real ELM327:
	// MultiECUScan receives the "?" and immediately sends the command again without the digit.
	if(l & 1){
		if(!cafOn){ elm_line("?"); return; }
		expectedResponses = elm_hexval(hexCmd[l - 1]);
		if(expectedResponses == 0xFF) expectedResponses = 0;
		l--;
	}

	for(uint16_t i = 0; (i + 1) < l && payloadLen < sizeof(payload); i += 2){
		payload[payloadLen++] = (uint8_t)((elm_hexval(hexCmd[i]) << 4) | elm_hexval(hexCmd[i + 1]));
	}
	if(payloadLen == 0){ elm_line("?"); return; }

#if defined(C1baccable)
	uint8_t order[ELMLINK_BUS_COUNT];
	uint8_t candidates = elm_bus_order(order);

	//if the bus is already known we go straight there with the full timeout, otherwise
	//the buses are probed with a reduced timeout so as not to pay 612 ms for every empty attempt
	uint8_t probing = (candidates > 1);

	for(uint8_t k = 0; k < candidates; k++){
		activeBus  = order[k];
		rspTimeout = probing ? ELM327_PROBE_TIMEOUT_MS : cmdTimeout;
		if(rspTimeout > cmdTimeout) rspTimeout = cmdTimeout;
		trace_str(activeBus == ELMLINK_BUS_LOCAL ? "{C1}" :
		          (activeBus == ELMLINK_BUS_C2   ? "{C2}" : "{BH}"));
		if(activeBus != ELMLINK_BUS_LOCAL){
			elm_remote_clear();
			//the configuration is sent only when it changes: fewer blocks on the line,
			//fewer collision windows and faster commands
			if(cfgSentBus != activeBus || cfgSentFilter != canFilterValue ||
			   cfgSentMask != canFilterMask || cfgSentTimeout != rspTimeout || cfgSentFc != cfcOn){
				elmlink_send_config(activeBus, canFilterValue, canFilterMask, rspTimeout, cfcOn);
				cfgSentBus = activeBus;       cfgSentFilter  = canFilterValue;
				cfgSentMask = canFilterMask;  cfgSentTimeout = rspTimeout; cfgSentFc = cfcOn;
			}
			//if the program has set a custom flow control, the slave has to know about it
			if(fcMode && fcDataLen)
				elmlink_send_fc_config(activeBus, fcHeader ? fcHeader : canSendHeader,
				                       fcHeader ? fcHeaderExt : extendedHeader, fcData, fcDataLen);
		}

		uint8_t answered = cafOn
			? elm_handle_obd_caf(payload, payloadLen, expectedResponses)
			: elm_handle_obd_raw(payload, (uint8_t)payloadLen, expectedResponses);

		if(answered){
			elm_route_store(elm_target_addr(), activeBus);	//from now on we go straight there
			activeBus  = ELMLINK_BUS_LOCAL;
			rspTimeout = cmdTimeout;
			return;
		}
	}
	activeBus  = ELMLINK_BUS_LOCAL;
	rspTimeout = cmdTimeout;
	elm_line("NO DATA");
#else
	rspTimeout = cmdTimeout;
	uint8_t answered = cafOn
		? elm_handle_obd_caf(payload, payloadLen, expectedResponses)
		: elm_handle_obd_raw(payload, (uint8_t)payloadLen, expectedResponses);
	if(!answered) elm_line("NO DATA");
#endif
}

// ------------------------------------------------------------------ dispatcher
static void elm_execute(char *cmd){
	//records the received command (the echo of the output is recorded by elm_putc)
	trace_str("\r\n<");
	trace_str(cmd);
	trace_str(">");

	if(echoOn){					//echo of the received command, as the real ELM327 does
		elm_puts(cmd);
		elm_eol();
	}

	if(cmd[0] == 'A' && cmd[1] == 'T'){
		elm_handle_at(cmd + 2);
	}else if(elm_is_hex_string(cmd)){
		elm_handle_obd(cmd);
	}else{
		elm_line("?");
	}

	#ifdef ELM327_EXTRA_CR_BEFORE_PROMPT
		elm_putc('\r');		//empty line before the prompt, like the original ELM327
	#endif
	elm_putc('>');				//prompt
	elm_flush();
}

// Interprets at most one command per call, so that the main loop stays responsive.
void elm327_process(void){
	int16_t c;

	#if defined(C1baccable)
		if(!elmModeOn) return;					//mode off: the BACCAble works normally

		// Automatic way out: with the ELM327 on the cluster does not answer the buttons,
		// so it could no longer be switched off from the menu. If nothing arrives from the
		// PC for a while (cable unplugged, program closed) we go back to normal operation by ourselves.
		if((currentTime - elmLastCmd) > ELM327_IDLE_EXIT_MS){
			elm327_set_enabled(0);
			return;
		}
	#endif

	//if a command has been left incomplete (no CR) we discard it after one second:
	//it prevents a fragment of a previous session from ruining the next command.
	if(cmdLen && (currentTime - lastRxByteTime) > 1000) cmdLen = 0;

	while((c = elm_ring_get()) >= 0){
		lastRxByteTime = currentTime;
		#if defined(C1baccable)
			elmLastCmd = currentTime;			//there is life on the port: the mode stays on
		#endif
		if(c == '\r' || c == '\n'){
			if(cmdLen == 0) continue;			//empty line: ELM would repeat the last command, here we ignore it
			cmdBuf[cmdLen] = '\0';
			cmdLen = 0;
			elm_execute(cmdBuf);
			return;								//one command per loop iteration
		}
		if(c == ' ') continue;					//the ELM327 ignores the spaces in the commands
		if(c < 0x20 || c >= 0x7F) continue;		//discards the non printable characters
		if(cmdLen >= (ELM327_CMD_BUF_LEN - 1)){	//command too long: it is discarded
			cmdLen = 0;
			continue;
		}
		if(c >= 'a' && c <= 'z') c = (int16_t)(c - 'a' + 'A');
		cmdBuf[cmdLen++] = (char)c;
	}
}

#endif //ACT_AS_ELM327
