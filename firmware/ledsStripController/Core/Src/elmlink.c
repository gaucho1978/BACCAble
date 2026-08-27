/*
 * elmlink.c — ponte diagnostico fra i chip della BACCAble (vedi elmlink.h)
 */

#include "elmlink.h"

#if defined(C1baccable) || defined(C2baccable) || defined(BHbaccable)

#include <string.h>
#include "globalVariables.h"
#include "uart.h"
#include "onboardLed.h"

extern UART_HandleTypeDef huart2;

// ------------------------------------------------------------------ stato comune
// elm327 function 27/08/2026 - il ponte vive dentro il firmware normale di tutti e tre i chip,
// quindi parte SEMPRE spento: si accende solo quando si sceglie "ELM327 Diag" nel menu del
// quadro. Il caso dei flavor dedicati al ponte (modulo acceso da subito) qui non esiste.
	static uint8_t linkEnabled = 0;
static uint8_t seqCounter  = 0;

// Blocchi ricevuti dall'interrupt, in attesa di essere elaborati nel main loop.
// E' una piccola coda (non un solo slot): il gateway invia CFG e REQ uno dietro l'altro,
// e con un solo slot il secondo poteva arrivare prima che il primo fosse consumato e
// venire scartato. Con la coda i due blocchi convivono sempre.
#define ELMLINK_INBOX_LEN 4
static volatile uint8_t  inbox[ELMLINK_INBOX_LEN][UART_BUFFER_SIZE];
static volatile uint8_t  inboxHead = 0, inboxTail = 0;

static uint8_t inbox_push(const uint8_t *f){
	uint8_t next = (uint8_t)((inboxHead + 1) % ELMLINK_INBOX_LEN);
	if(next == inboxTail) return 0;			//coda piena: si scarta (non dovrebbe succedere)
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

// Invia un blocco sulla linea (vedi uart_link_send in uart.c: scrive sui registri e tiene
// spenta la ricezione durante l'invio, altrimenti l'interrupt non si riarma piu').
static void elmlink_send(const uint8_t *f){
	uart_link_send(f, UART_BUFFER_SIZE);
}

// Chiamata dall'interrupt di ricezione: si limita a copiare il blocco.
uint8_t elmlink_on_uart_frame(const uint8_t *frame){
	uint8_t dest = frame[0];

	if(dest != ELMLINK_TO_C2 && dest != ELMLINK_TO_BH && dest != ELMLINK_TO_MASTER) return 0;

	// Blocco corrotto: quasi sempre vuol dire che il framing a 19 byte si e' sfasato
	// (un byte perso durante una collisione sulla linea a filo singolo). Restituendo 2
	// si chiede a uart.c di buttare la sincronizzazione e riagganciarla byte per byte:
	// senza questo il collegamento restava sfasato PER SEMPRE e tutti i bus remoti
	// morivano dopo la prima sessione con traffico fitto (log del 24/08, ore 21:27).
	if(elmlink_checksum(frame) != frame[17]) return 2;

	#if defined(C2baccable) || defined(BHbaccable)
		// L'ordine di accensione deve passare ANCHE a ponte spento: e' proprio il messaggio
		// che lo accende. Arriva dal C1 quando si sceglie "ELM327 Diag" nel menu del quadro.
		if(dest == ELMLINK_SLAVE_ID && frame[1] == ELMLINK_TYPE_ARM){
			linkEnabled = (frame[2] & ELMLINK_FLAG_ARM_ON) ? 1 : 0;
			if(!linkEnabled) inbox_clear();
			return 1;
		}
	#endif

	if(!linkEnabled) return 1;			//messaggio nostro ma il ponte e' spento: lo consumiamo

	#if defined(C1baccable)
		//il master ascolta solo le risposte; i propri messaggi tornano indietro per eco
		if(dest != ELMLINK_TO_MASTER) return 1;
	#endif
	#if defined(C2baccable) || defined(BHbaccable)
		if(dest != ELMLINK_SLAVE_ID) return 1;
	#endif

	inbox_push(frame);					//accodato: verra' elaborato nel main loop
	return 1;
}


// =====================================================================================
//  LATO SLAVE (chip C2 / BH): riceve un frame, lo trasmette sul proprio bus e rimanda
//  indietro le risposte. Non blocca il main loop: la raccolta e' una macchina a stati.
// =====================================================================================
#if defined(C2baccable) || defined(BHbaccable)

static uint8_t  slaveBusOpen	= 0;
static uint8_t  slaveCollecting	= 0;
static uint32_t slaveDeadline	= 0;
static uint8_t  slaveSeq		= 0;
static uint8_t  slaveGotFrames	= 0;
static uint8_t  slaveLastPci	= 0xFF;	//byte PCI dell'ultimo frame inoltrato
static uint32_t slaveReqId		= 0;	//id dell'ultima richiesta (usato per il flow control)
static uint8_t  slaveReqExt		= 0;
static uint8_t  slaveAutoFc		= 1;	//lo slave risponde da solo ai primi frame
static uint32_t slaveFcHeader	= 0;	//0 = usa l'id della richiesta
static uint8_t  slaveFcExt		= 0;
static uint8_t  slaveFcData[8]	= {0x30, 0x00, 0x00, 0, 0, 0, 0, 0};
static uint8_t  slaveFcLen		= 3;
static uint32_t slaveFilterVal	= 0;
static uint32_t slaveFilterMask	= 0;
static uint16_t slaveTimeout	= ELMLINK_DEFAULT_TIMEOUT_MS;

//il bus si apre solo alla prima richiesta: alimentare la scheda non deve disturbare l'auto
static void slave_bus_open(void){
	if(slaveBusOpen) return;
	// elm327 function 27/08/2026 - il bus e' gia' aperto alla velocita' giusta, e' quello su cui la
	// BACCAble lavora tutti i giorni: non lo tocchiamo. Il ramo che lo riconfigurava (flavor dedicati
	// al ponte, dove il bus apparteneva allo slave) qui non esiste piu': su un baccable installato
	// vorrebbe dire spegnere la rete su cui il chip sta lavorando.
		can_enable();	//no-op se e' gia' aperto: serve solo al caso del chip BH da fermo
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
		case ELMLINK_TYPE_FCCFG:	//flow control personalizzato impostato dal programma
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
			//can_tx accoda soltanto: diamo una spinta alla coda
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

// Riconosce la risposta negativa UDS "richiesta ricevuta, risposta in arrivo" (7F xx 78):
// la centralina sta ancora lavorando e la risposta vera arrivera' piu' tardi.
static uint8_t elmlink_is_response_pending(const uint8_t *d){
	if((d[0] & 0xF0) != 0x00) return 0;			//solo i frame singoli
	return (d[1] == 0x7F && d[3] == 0x78) ? 1 : 0;
}

// Offre al ponte un frame ricevuto dal bus. Restituisce 1 se e' stato inoltrato al master.
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

	// Attesa del prossimo frame calibrata su cosa e' appena passato: senza questo si
	// aspettava sempre il timeout pieno (con ATST99 sono 612 ms) anche quando la
	// centralina aveva gia' risposto in 5 ms, ed e' il motivo per cui i bus remoti
	// risultavano molto piu' lenti del bus locale.
	slaveLastPci = d[0];
	uint16_t gap = ELMLINK_GAP_AFTER_SF_MS;

	if((slaveLastPci & 0xF0) == 0x10){
		// Primo frame di una risposta lunga: la centralina aspetta il "continua" (flow control).
		// Lo manda lo slave, sul suo bus, in pochi microsecondi. Farlo mandare al master
		// significava un giro completo di seriale e, soprattutto, aprire una nuova richiesta
		// nel mezzo della raccolta: la centralina scadeva e la risposta finiva accodata al
		// comando successivo (le risposte sfasate di uno).
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
			gap = ELMLINK_GAP_AFTER_CF_MS;	//i frame consecutivi ora arrivano subito
		}else{
			gap = ELMLINK_GAP_AFTER_FF_MS;	//flow control disattivato: lo manda il programma
		}
	}
	else if((slaveLastPci & 0xF0) == 0x20) gap = ELMLINK_GAP_AFTER_CF_MS;

	if(gap > slaveTimeout) gap = slaveTimeout;

	// "Sto lavorando": si aspetta tutto il tempo concesso dal programma, non il gap breve.
	// Definendo ELMLINK_NO_PENDING_WAIT si torna al comportamento precedente (chiusura
	// rapida anche dopo un "sto lavorando").
	// Senza questo, nelle scritture lunghe (allineamento proxi) il frame con la conferma
	// vera arrivava a raccolta gia' chiusa e veniva perso: il programma vedeva un timeout
	// anche se la centralina aveva concluso correttamente.
	#ifndef ELMLINK_NO_PENDING_WAIT
		if(elmlink_is_response_pending(d)) gap = slaveTimeout;
	#endif

	slaveDeadline = currentTime + gap;
	return 1;
}

#endif //C2baccable || BHbaccable


// =====================================================================================
//  LATO MASTER (chip C1, gateway)
// =====================================================================================
#if defined(C1baccable)

static volatile uint8_t masterWaiting = 0;

static uint8_t bus_to_dest(uint8_t bus){
	return (bus == ELMLINK_BUS_BH) ? ELMLINK_TO_BH : ELMLINK_TO_C2;
}

// Accensione/spegnimento del ponte sui chip C2 e BH. Va mandato a ponte gia' acceso da
// questa parte (e' il master che decide), e i due messaggi sono distanziati: la linea e' a
// filo singolo e i due chip devono avere il tempo di leggere il proprio.
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
	inbox_clear();			//scarta eventuali resti della richiesta precedente
	elmlink_build(f, bus_to_dest(bus), ELMLINK_TYPE_REQ,
	              ext ? ELMLINK_FLAG_EXTID : 0, canId, data, dlc, seqCounter);
	elmlink_send(f);
	return 1;
}

uint8_t elmlink_poll(void (*onFrame)(uint32_t id, uint8_t ext, const uint8_t *d, uint8_t dlc)){
	uint8_t local[UART_BUFFER_SIZE];
	if(!inbox_pop(local)) return 0;

	if(local[16] != seqCounter) return 0;	//risposta di una richiesta precedente: si scarta

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
		// Ponte appena spento dal C1: si lascia tutto com'era prima. Il bus CAN non si chiude,
		// e' quello su cui il firmware normale continua a lavorare.
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
			while(inbox_pop(local)) slave_handle_frame(local);	//tutti i blocchi in coda
		}

		// La coda di ricezione CAN va SEMPRE svuotata, anche quando non stiamo raccogliendo.
		// Il bus carrozzeria (BH) ha traffico continuo anche a quadro spento: lasciando la
		// coda piena fra una richiesta e l'altra, all'inizio della raccolta si trovavano
		// dentro frame vecchi di quel traffico e la risposta vera rischiava di arrivare in
		// una coda gia' satura. Fuori dalla raccolta i frame si leggono e si buttano.
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
