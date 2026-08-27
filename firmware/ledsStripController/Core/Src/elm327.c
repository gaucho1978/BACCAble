/*
 * elm327.c
 *
 *  Emulazione ELM327 (ISO 15765-4) sulla porta USB CDC della BACCAble.
 *  La velocita' del bus e' scelta in fase di compilazione (ELM327_BITRATE_KBPS).
 *  Vedi elm327.h per le note di porting dalla versione ESP32-C3.
 *
 *  Flusso:
 *    cdc_process()   -> elm327_rx_byte()  (solo accodamento, gira con irq disabilitati)
 *    main loop       -> elm327_process()  (interpreta ed esegue un comando per volta)
 */

#include "elm327.h"

#ifdef ACT_AS_ELM327

#include <string.h>
#include "globalVariables.h"
#include "usbd_cdc_if.h"
#include "onboardLed.h"
#if defined(C1baccable)
	#include "usb_device.h"		//la porta USB si accende solo quando serve
#endif
#if defined(C1baccable)
	#include "elmlink.h"	//ponte verso i chip dei bus C2 e BH
#endif

// ------------------------------------------------------------------ stato ELM327
#ifdef ELM327_STRICT_ELM_DEFAULTS
	#define ELM_DEF_ECHO	1	//eco attivo, come l'ELM327 originale dopo il reset
	#define ELM_DEF_LF		0	//line feed spenti, come l'ELM327 originale dopo il reset
	#ifndef ELM327_EXTRA_CR_BEFORE_PROMPT
		#define ELM327_EXTRA_CR_BEFORE_PROMPT	//riga vuota prima del prompt
	#endif
#else
	#define ELM_DEF_ECHO	0
	#define ELM_DEF_LF		1
#endif

static uint8_t  echoOn			= ELM_DEF_ECHO;
static uint8_t  headersOn		= 0;
static uint8_t  linefeedOn		= ELM_DEF_LF;
static uint8_t  spacesOn		= 1;
static uint8_t  cafOn			= 1;			//ATCAF0/1: 0 = modalita' raw, i frame passano cosi' come sono (AlfaOBD)
static uint8_t  cfcOn			= 1;			//se 0 non inviamo il Flow Control all'ecu
static uint8_t  adaptTiming		= 1;			//solo memorizzato (ATAT0/1/2)
static uint32_t canFilterValue	= 0;			//filtro software impostato con ATCRA / ATCF
static uint32_t canFilterMask	= 0;			//0 = accetta tutto (ATCM)
static uint32_t canSendHeader	= 0x7DF;		//header di trasmissione (ATSH)
static uint8_t  extendedHeader	= 0;			//1 = id a 29 bit
static uint8_t  canPriority		= 0x18;			//ATCP, usato per completare gli id a 29 bit
static uint16_t cmdTimeout		= ELM327_DEFAULT_TIMEOUT_MS;
static uint16_t rspTimeout		= ELM327_DEFAULT_TIMEOUT_MS;	//timeout del tentativo in corso

//parametri programmabili usati da MultiECUScan per scegliere la velocita' del bus:
//  PP 2C = opzioni del protocollo B (USER1)   PP 2D = divisore di baud rate del protocollo B
//  PP 2E = opzioni del protocollo C (USER2)   PP 2F = divisore di baud rate del protocollo C
//il bitrate vale 500 kbit/s diviso il divisore (01 = 500k, 02 = 250k, 04 = 125k, 0A = 50k).
static uint8_t  pp2C			= 0x00;
static uint8_t  pp2D			= 0x01;
static uint8_t  pp2E			= 0x00;
static uint8_t  pp2F			= 0x01;
static uint8_t  variableDlc		= 0;			//ATV1 = DLC pari ai byte utili, ATV0 = riempimento a 8
static uint8_t  busOpen			= 0;			//il bus CAN si apre solo quando serve davvero
static uint8_t  busDivisor		= 0;			//divisore richiesto (0 = quello della build)

#if defined(C1baccable)
	// Modalita' accesa dal menu del quadro. Da spenta l'interprete e' inerte: nessun byte
	// letto dall'USB, nessun frame sul bus, nessun messaggio sulla linea fra i chip.
	static uint8_t  elmModeOn   = 0;
	static uint32_t elmLastCmd  = 0;	//quando e' arrivato l'ultimo comando dal PC
#endif

#if defined(C1baccable)
// --- gateway verso gli altri due chip (vedi elmlink.h) ---
// Il bus attivo per la richiesta in corso e la memoria di quale centralina sta su quale
// rete: la prima volta si prova, poi la risposta arriva sempre dal bus giusto.
static uint8_t  activeBus = ELMLINK_BUS_LOCAL;

// La linea seriale e' un filo solo: chi trasmette non sente. La regola che elimina le
// collisioni e' che IL MASTER NON TRASMETTE MAI finche' lo slave non ha detto "ho finito"
// (blocco END) o non e' scaduto un margine piu' lungo del timeout dello slave. Prima i due
// timeout erano identici: lo slave mandava END nell'istante esatto in cui il master
// trasmetteva il comando successivo, un byte andava perso e il framing restava sfasato
// per sempre (tutti i bus remoti morti dopo la prima sessione di scrittura).
static uint8_t  remoteEnded = 1;		//1 = lo slave ha chiuso, la linea e' libera

//l'ultima configurazione mandata allo slave: si rimanda solo se cambia (meno traffico,
//meno finestre di collisione, comandi piu' rapidi)
static uint8_t  cfgSentBus = 0xFF;
static uint32_t cfgSentFilter, cfgSentMask; static uint16_t cfgSentTimeout; static uint8_t cfgSentFc;

typedef struct { uint16_t addr; uint8_t bus; } elm_route_t;
static elm_route_t routeCache[ELM327_ROUTE_CACHE_LEN];
static uint8_t     routeCount = 0;

//frame ricevuti dal bus remoto, in attesa di essere letti come se fossero locali
typedef struct { uint32_t id; uint8_t ext; uint8_t dlc; uint8_t data[8]; } elm_remote_frame_t;
static elm_remote_frame_t remoteFifo[ELM327_REMOTE_FIFO_LEN];
static uint8_t remoteHead = 0, remoteTail = 0;

static void elm_remote_push(uint32_t id, uint8_t ext, const uint8_t *d, uint8_t dlc){
	uint8_t next = (uint8_t)((remoteHead + 1) % ELM327_REMOTE_FIFO_LEN);
	if(next == remoteTail) return;	//coda piena: si scarta
	remoteFifo[remoteHead].id  = id;
	remoteFifo[remoteHead].ext = ext;
	remoteFifo[remoteHead].dlc = (dlc > 8) ? 8 : dlc;
	memcpy(remoteFifo[remoteHead].data, d, 8);
	remoteHead = next;
}

static void elm_remote_clear(void){ remoteHead = remoteTail = 0; }

//indirizzo della centralina interrogata: ultimo byte utile dell'header di trasmissione
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

// Ordine in cui provare i bus. Se la centralina e' gia' nota si va diretti; altrimenti si
// parte dal bus locale, a meno che il programma non abbia chiesto una velocita' diversa da
// 500k: in quel caso la rete e' quella carrozzeria (BH), che gira a 125 kbit/s.
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

//protocollo dichiarato: l'ELM327 vero risponde ad ATDP/ATDPN con quello selezionato con ATSP.
//Rispondere sempre "AB" (protocollo utente) manda in confusione i tool tipo AlfaOBD.
static uint8_t  protoNum		= 6;			//6 = ISO 15765-4 (CAN 11/500)
static uint8_t  protoAuto		= 1;			//1 = ricerca automatica ("A" davanti al numero)
static uint32_t lastRxByteTime	= 0;			//per scartare un comando rimasto a meta'

//flow control configurabile (ATFCSH / ATFCSD / ATFCSM), usato da AlfaOBD
static uint32_t fcHeader		= 0;			//0 = usa l'header di trasmissione corrente
static uint8_t  fcHeaderExt		= 0;
static uint8_t  fcData[8]		= {0x30, 0x00, 0x00, 0, 0, 0, 0, 0};
static uint8_t  fcDataLen		= 3;
static uint8_t  fcMode			= 0;			//0 = automatico, 1/2 = usa header e dati impostati

// ------------------------------------------------------------------ buffer ingresso usb
static volatile uint8_t  rxRing[ELM327_RX_RING_LEN];
static volatile uint16_t rxHead = 0;
static volatile uint16_t rxTail = 0;

static char    cmdBuf[ELM327_CMD_BUF_LEN];
static uint8_t cmdLen = 0;

// ------------------------------------------------------------------ buffer uscita usb
// CDC_Transmit_FS() scarta i pacchetti piu' lunghi di TX_BUF_SIZE, quindi l'output
// viene accumulato in un blocco piccolo e spedito appena si riempie.
static uint8_t txChunk[ELM327_TX_CHUNK_LEN];
static uint8_t txChunkLen = 0;

// ------------------------------------------------------------------ registratore (ATLOG)
#ifndef ELM327_TRACE_DISABLE
static char     traceBuf[ELM327_TRACE_LEN];
static uint16_t traceHead    = 0;
static uint8_t  traceWrapped = 0;
static uint8_t  traceDumping = 0;	//mentre stampiamo il registro non registriamo l'uscita

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
	// CDC_Transmit_FS aspetta al massimo 10ms e poi restituisce USBD_BUSY scartando i dati:
	// succede quando l'host ha appena aperto la porta e non sta ancora leggendo (e' il motivo
	// per cui i primi comandi dopo la connessione potevano andare persi). Qui ritentiamo.
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

//fine riga secondo l'impostazione ATL0/ATL1
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

// ------------------------------------------------------------------ helper CAN
// can_tx() di BACCAble accoda soltanto: qui accodiamo e poi "pompiamo" la coda
// finche' il frame non e' realmente uscito (o scade il timeout).
static void elm_bus_open(void);	//dichiarazione anticipata: il bus si apre alla prima richiesta

static uint8_t elm_can_send_frame_id(const uint8_t *bytes, uint8_t len, uint32_t id, uint8_t ext){
	CAN_TxHeaderTypeDef h;
	uint8_t data[8];

	elm_bus_open();	//prima trasmissione: entriamo sul bus adesso, non all'accensione

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

	//svuota la coda di trasmissione (can_process invia un frame per chiamata)
	uint32_t t0 = currentTime;
	while(currentTime - t0 < 20){
		can_process();
		if(HAL_CAN_GetTxMailboxesFreeLevel(can_gethandle()) == 3) return 1;
	}
	return 1; //passati i 20ms lo consideriamo comunque partito: il timeout risposta fara' il resto
}

//invia un frame sul bus attivo: quello locale oppure, tramite l'altro chip, C2 o BH
static uint8_t elm_bus_send(const uint8_t *bytes, uint8_t len, uint32_t id, uint8_t ext){
	#if defined(C1baccable)
		if(activeBus != ELMLINK_BUS_LOCAL){
			//non si aspetta qui: le risposte arrivano nel ciclo di attesa, una per una
			remoteEnded = 0;
			return elmlink_send_request(activeBus, id, ext, bytes, len);
		}
	#endif
	return elm_can_send_frame_id(bytes, len, id, ext);
}

// Fa girare il trasporto durante le attese: sul bus locale svuota la coda di trasmissione,
// su bus remoto raccoglie i frame che lo slave manda indietro. In entrambi i casi la
// risposta viene poi stampata al PC frame per frame, appena disponibile: e' il motivo per
// cui le scritture lunghe non vanno piu' in timeout sul programma.
static void elm_bus_pump(void){
	#if defined(C1baccable)
		if(activeBus != ELMLINK_BUS_LOCAL){
			if(elmlink_poll(elm_remote_push)) remoteEnded = 1;	//lo slave ha chiuso
			return;
		}
	#endif
	can_process();
}

// Timeout di attesa effettivo: sul bus remoto si aggiunge il margine del trasporto
// seriale, cosi' il master aspetta SEMPRE piu' a lungo dello slave e l'END arriva
// quando la linea e' libera (normalmente l'attesa finisce molto prima, proprio all'END).
static uint16_t elm_wait_budget(void){
	#if defined(C1baccable)
		if(activeBus != ELMLINK_BUS_LOCAL) return (uint16_t)(rspTimeout + ELMLINK_DEFAULT_TIMEOUT_MS);
	#endif
	return rspTimeout;
}

//1 = lo slave ha detto END: non arrivera' piu' niente, inutile aspettare
static uint8_t elm_remote_finished(void){
	#if defined(C1baccable)
		return (activeBus != ELMLINK_BUS_LOCAL && remoteEnded) ? 1 : 0;
	#endif
	return 0;
}

#if defined(C1baccable)
// Prima di restituire il controllo (e quindi prima che MES mandi il comando successivo)
// si aspetta l'END dello slave: e' il "via libera" che rende la linea half-duplex sicura.
static void elm_remote_drain(void){
	if(activeBus == ELMLINK_BUS_LOCAL || remoteEnded) return;
	// Lo slave puo' legittimamente restare armato fino al timeout che gli abbiamo
	// comunicato nella CFG (rspTimeout: con ATSTFE sono ~1016 ms): il drain deve coprire
	// TUTTO quel tempo, non un margine fisso. Un margine piu' corto significava tornare a
	// trasmettere con lo slave ancora armato: la collisione che uccideva la linea.
	uint32_t t0 = currentTime;
	while(currentTime - t0 < (uint32_t)rspTimeout + ELMLINK_DEFAULT_TIMEOUT_MS){
		if(elmlink_poll(elm_remote_push)){ remoteEnded = 1; break; }
	}
	remoteEnded = 1;	//se l'END si e' perso, il margine e' comunque passato: linea libera
}
#else
	#define elm_remote_drain()	do{}while(0)
#endif

//invia un frame da 8 byte con l'header di trasmissione corrente (ATSH)
static uint8_t elm_can_send_frame(const uint8_t *data8){
	return elm_bus_send(data8, 8, canSendHeader, extendedHeader);
}

// Il bus resta chiuso finche' un programma non chiede davvero qualcosa: alimentare la
// scheda non deve mai disturbare le linee CAN dell'auto (un nodo alla velocita' sbagliata
// le riempie di error frame). E' anche il comportamento dell'ELM327 originale.
// Con la modalita' selezionabile dal quadro il bus del C1 NON e' nostro: lo tiene aperto la
// BACCAble per il suo lavoro normale (500 kbit/s, la velocita' giusta per questa rete), e
// chiuderlo qui vorrebbe dire spegnere il cruscotto ad ogni ATZ. Teniamo solo il conto.
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

// Cambia la velocita' del bus seguendo il divisore dei parametri programmabili:
// bitrate = 500 kbit/s / divisore, cioe' prescaler = 12 * divisore (48MHz / (presc * 8)).
// Se il bus e' gia' aperto viene chiuso e riaperto, altrimenti si prepara e basta.
static void elm_apply_bitrate_divisor(uint8_t divisor){
	if(divisor == 0) divisor = 1;
	busDivisor = divisor;	//serve comunque a capire quale bus vuole il programma

	#if defined(C1baccable)
		// CON IL GATEWAY IL BUS LOCALE NON SI TOCCA.
		// Ogni chip sta su una rete con la sua velocita' fissa (C1 e C2 a 500 kbit/s, BH a
		// 125): qui il divisore chiesto dal programma dice QUALE rete vuole, non a che
		// velocita' riconfigurare la nostra. Riconfigurando il bus locale a 125 kbit/s
		// mentre la rete C1 gira a 500, il nostro nodo comincia a sparare error frame sulla
		// rete dell'auto e va in bus-off: da quel momento non risponde piu' niente, nemmeno
		// le centraline sui bus C. Era il motivo per cui dopo la prima centralina di
		// carrozzeria falliva tutto il resto della sessione.
		return;
	#else
		//senza gateway un solo chip serve qualsiasi velocita': qui il cambio si fa davvero
		uint8_t wasOpen = busOpen;
		elm_bus_close();
		can_set_prescaler((uint32_t)12 * divisor);
		if(wasOpen) elm_bus_open();
	#endif
}

//true se l'id ricevuto passa il filtro impostato con ATCRA
static uint8_t elm_filter_pass(const CAN_RxHeaderTypeDef *h){
	if(canFilterMask == 0) return 1;
	uint32_t id = (h->IDE == CAN_ID_EXT) ? h->ExtId : h->StdId;
	return ((id & canFilterMask) == (canFilterValue & canFilterMask)) ? 1 : 0;
}

//riceve un frame rispettando il filtro. 1 = frame disponibile.
//Se la richiesta e' partita verso un bus remoto, i frame arrivano dalla coda riempita
//dalle risposte dell'altro chip, e tutto il resto del codice non se ne accorge.
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

//stampa l'header della risposta. Con ATS1 i byte sono separati da spazio (e c'e' uno
//spazio anche prima dei dati), con ATS0 non c'e' nessun separatore, come l'ELM327 vero.
static void elm_print_header(const CAN_RxHeaderTypeDef *h){
	if(!headersOn) return;

	#if defined(ELM327_HEADER_ALWAYS_SPACED)
		const uint8_t sep = 1;	//comportamento della prima versione (spazi anche con ATS0)
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

//invia il Flow Control all'ecu: di default ContinueToSend / BlockSize 0 / STmin 0,
//oppure header e dati impostati con ATFCSH / ATFCSD (ATFCSM diverso da 0).
static void elm_send_flow_control(void){
	#if defined(C1baccable)
		//su bus remoto ci pensa lo slave, appena vede passare il primo frame
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
	elm_bus_close();	//ATZ = reset: si esce dal bus, si rientra alla prima richiesta
	#if defined(C1baccable)
		cfgSentBus = 0xFF;		//dopo un reset la configurazione va rimandata
		//la mappa centralina->bus NON si azzera: e' il cablaggio dell'auto, non cambia con
		//un reset. MultiECUScan manda un ATZ prima di ogni centralina, azzerarla voleva
		//dire risondare tutti i bus ad ogni collegamento.
		activeBus  = ELMLINK_BUS_LOCAL;
		rspTimeout = cmdTimeout;
		elm_remote_clear();
	#endif
	//i parametri programmabili non si azzerano: sul chip originale stanno in memoria non
	//volatile e sopravvivono ad ATZ (MultiECUScan li imposta proprio prima di un ATZ).
	cmdLen			= 0;
}

// Chiamata quando l'host apre o chiude la porta seriale (CDC_SET_CONTROL_LINE_STATE):
// butta via i byte rimasti dalla sessione precedente, che altrimenti si mescolerebbero
// ai primi comandi del nuovo collegamento.
void elm327_port_reset(void){
	#if defined(C1baccable)
		if(!elmModeOn) return;		//modalita' spenta: non c'e' niente da azzerare
	#endif
	trace_str("\r\n== porta aperta/chiusa ==");
	elm_bus_close();	//nessun programma collegato: usciamo dal bus e non disturbiamo l'auto
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
// ------------------------------------------------- accensione dal menu del quadro
// Da spento non viene chiamato niente di tutto questo: e' come se l'ELM327 non ci fosse.
// Da acceso la BACCAble sospende il suo lavoro normale (main.c) e il chip si comporta
// esattamente come il firmware ELM327 dedicato, gateway verso C2 e BH compreso.
void elm327_set_enabled(uint8_t on){
	on = on ? 1 : 0;
	if(on == elmModeOn) return;

	elmModeOn  = on;
	elmLastCmd = currentTime;

	elm327_init();			//stato dell'interprete pulito ad ogni accensione
	rxHead = rxTail = 0;	//e via i byte rimasti sulla porta

	// LA PORTA USB SI ACCENDE SOLO ADESSO.
	// Finche' la modalita' e' spenta il connettore resta muto e il computer non vede
	// nessun dispositivo: la BACCAble non deve farsi riconoscere come interfaccia ELM327
	// se non gliel'ha chiesto nessuno dal quadro. Ogni accensione rifa' l'inizializzazione
	// da capo (vedi usb_device.c: dopo uno spegnimento il blocco USB resta in power-down).
	if(on)	usb_device_attach();
	else	usb_device_detach();

	#if defined(C1baccable)
		//accende (o spegne) il ponte qui e sugli altri due chip
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

// ------------------------------------------------------------------ ingresso da usb
void elm327_rx_byte(uint8_t c){
	#if defined(C1baccable)
		if(!elmModeOn) return;		//modalita' spenta: la porta USB non ci interessa
	#endif
	uint16_t next = (uint16_t)((rxHead + 1) % ELM327_RX_RING_LEN);
	if(next == rxTail) return; //buffer pieno: scarta (il tool ritenta)
	rxRing[rxHead] = c;
	rxHead = next;
}

static int16_t elm_ring_get(void){
	if(rxTail == rxHead) return -1;
	uint8_t c = rxRing[rxTail];
	rxTail = (uint16_t)((rxTail + 1) % ELM327_RX_RING_LEN);
	return (int16_t)c;
}

// ------------------------------------------------------------------ comandi AT
static void elm_handle_at(const char *at){

	if(!strcmp(at, "Z")){					//reset completo
		elm_reset_defaults();
		HAL_Delay(50);
		elm_puts(ELM327_ID_STRING); elm_eol();
		return;
	}
	if(!strcmp(at, "WS")){ HAL_Delay(50); elm_line(ELM327_ID_STRING);	return; }
	if(!strcmp(at, "I"))  { elm_line(ELM327_ID_STRING);					return; }
	if(!strcmp(at, "@1")) { elm_line(ELM327_DESCR_STRING);				return; }
	if(!strcmp(at, "@2")) { elm_line("?");								return; }	//identificatore non programmato, come un chip originale
	if(!strcmp(at, "D"))  { elm_reset_defaults(); elm_line("OK"); return; }

	if(!strcmp(at, "E0")) { echoOn = 0;		elm_line("OK"); return; }
	if(!strcmp(at, "E1")) { echoOn = 1;		elm_line("OK"); return; }
	if(!strcmp(at, "L0")) { linefeedOn = 0;	elm_line("OK"); return; }
	if(!strcmp(at, "L1")) { linefeedOn = 1;	elm_line("OK"); return; }
	if(!strcmp(at, "H0")) { headersOn = 0;	elm_line("OK"); return; }
	if(!strcmp(at, "H1")) { headersOn = 1;	elm_line("OK"); return; }
	if(!strcmp(at, "S0")) { spacesOn = 0;	elm_line("OK"); return; }
	if(!strcmp(at, "S1")) { spacesOn = 1;	elm_line("OK"); return; }

	if(!strcmp(at, "V0")) { variableDlc = 0; elm_line("OK"); return; }	//frame riempiti fino a 8 byte
	if(!strcmp(at, "V1")) { variableDlc = 1; elm_line("OK"); return; }	//DLC pari ai soli byte utili

	//comandi accettati senza effetto reale su questo hardware
	if(!strcmp(at, "AR") || !strcmp(at, "AL") || !strcmp(at, "NL") ||
	   !strcmp(at, "BI") || !strcmp(at, "PC") || !strcmp(at, "MA")){
		elm_line("OK"); return;
	}

	//ATDP / ATDPN: riportano il protocollo selezionato con ATSP (non un valore fisso)
	if(!strcmp(at, "DP")){
		switch(protoNum){
			case 6:  elm_line("ISO 15765-4 (CAN 11/500)"); break;
			case 7:  elm_line("ISO 15765-4 (CAN 29/500)"); break;
			case 8:  elm_line("ISO 15765-4 (CAN 11/250)"); break;
			case 9:  elm_line("ISO 15765-4 (CAN 29/250)"); break;
			case 0xA:elm_line("SAE J1939 (CAN 29/250)");   break;
			case 0xB: case 0xC: {	//protocolli utente: velocita' = 500 / divisore
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
	if(!strcmp(at, "RV"))  { elm_line("12.3V");						return; } //nessun partitore sul 12V: valore fisso

	if(!strcmp(at, "CAF0")){ cafOn = 0; elm_line("OK"); return; }
	if(!strcmp(at, "CAF1")){ cafOn = 1; elm_line("OK"); return; }
	if(!strcmp(at, "CFC0")){ cfcOn = 0; elm_line("OK"); return; }
	if(!strcmp(at, "CFC1")){ cfcOn = 1; elm_line("OK"); return; }

	//ATAT0/1/2 (adaptive timing): il comando completo e' "ATAT<n>", qui arriva "AT<n>"
	if(strlen(at) == 3 && at[0] == 'A' && at[1] == 'T'){
		uint8_t v = (uint8_t)(at[2] - '0');
		if(v <= 2){ adaptTiming = v; elm_line("OK"); }
		else       { elm_line("?"); }
		return;
	}

	if(!strncmp(at, "ST", 2)){					//timeout risposta, passo 4ms
		uint32_t v = elm_parse_hex(at + 2);
		cmdTimeout = (v == 0) ? ELM327_DEFAULT_TIMEOUT_MS : (uint16_t)(v * 4);
		elm_line("OK"); return;
	}
	if(!strncmp(at, "CP", 2)){					//priorita' per gli id a 29 bit
		canPriority = (uint8_t)elm_parse_hex(at + 2);
		elm_line("OK"); return;
	}
	if(!strncmp(at, "CRA", 3)){					//filtro di ricezione software
		const char *a = at + 3;
		uint8_t n = (uint8_t)strlen(a);
		if(n == 0){
			canFilterValue = 0;
			canFilterMask  = 0;					//accetta tutto
		}else{
			uint32_t val = 0, msk = 0;
			for(uint8_t i = 0; i < n; i++){
				uint8_t d = elm_hexval(a[i]);
				val <<= 4; msk <<= 4;
				if(d != 0xFF){ val |= d; msk |= 0x0F; }	//le 'X' restano wildcard
			}
			canFilterValue = val;
			canFilterMask  = msk;
		}
		elm_line("OK"); return;
	}
	if(!strncmp(at, "SH", 2)){					//header di trasmissione
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

	//ATFCSH xxx / xxxxxxxx : header usato per i frame di Flow Control (usato da AlfaOBD)
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
	//ATFCSD hh hh hh... : dati del frame di Flow Control (max 5 byte, di solito 30 00 00)
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
	//ATFCSM n : 0 = automatico, 1/2 = usa header e dati impostati sopra
	if(!strncmp(at, "FCSM", 4)){
		uint8_t v = (uint8_t)(at[4] - '0');
		fcMode = (v <= 2) ? v : 0;
		elm_line("OK"); return;
	}

	//ATCF xxx / xxxxxxxx : valore del filtro di ricezione (usato insieme ad ATCM)
	if(!strncmp(at, "CF", 2)){
		const char *a = at + 2;
		if(strlen(a) == 0){ canFilterValue = 0; canFilterMask = 0; }
		else{
			canFilterValue = elm_parse_hex(a);
			if(canFilterMask == 0) canFilterMask = (strlen(a) > 3) ? 0x1FFFFFFF : 0x7FF;
		}
		elm_line("OK"); return;
	}
	//ATCM xxx / xxxxxxxx : maschera del filtro di ricezione
	if(!strncmp(at, "CM", 2)){
		const char *a = at + 2;
		canFilterMask = (strlen(a) == 0) ? 0 : elm_parse_hex(a);
		elm_line("OK"); return;
	}

	//ATSPx / ATSPAx / ATTPx: il bus resta quello compilato, ma memorizziamo il protocollo
	//dichiarato in modo che ATDP e ATDPN rispondano in modo coerente.
	//ATPPxxSVyy / ATPPxxON / ATPPxxOFF: parametri programmabili.
	//Ci interessano 2D e 2F, i divisori di baud rate dei protocolli utente B e C: e' con
	//questi che MultiECUScan chiede una velocita' del bus diversa, prima di fare ATSPB.
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
			//ON / OFF: l'attivazione la consideriamo implicita, rispondiamo OK
		}
		elm_line("OK"); return;
	}

	if(!strncmp(at, "SP", 2) || !strncmp(at, "TP", 2)){
		const char *a = at + 2;
		if(*a == 'A'){ protoAuto = 1; a++; }
		else           protoAuto = 0;
		uint8_t v = elm_hexval(*a);
		if(v == 0xFF){ elm_line("OK"); return; }	//nessuna cifra: lasciamo com'e'
		if(v == 0){ protoAuto = 1; }				//0 = ricerca automatica
		else       { protoNum = v; }

		//i protocolli utente B e C usano il divisore dei parametri programmabili, gli altri
		//le velocita' standard: e' qui che la velocita' del bus cambia davvero.
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

	//ATIB/ATFC/ATBRT: accettati senza effetto
	if(!strncmp(at, "IB", 2) || !strncmp(at, "FC", 2) || !strncmp(at, "BRT", 3)){
		elm_line("OK"); return;
	}

	if(!strncmp(at, "BRD", 3)){
		// Su USB CDC il baud rate e' virtuale: eseguiamo solo l'handshake previsto
		// dall'ELM327 (OK -> stringa id -> attesa CR dal tool -> OK).
		elm_puts("OK"); elm_eol();
		elm_flush();
		HAL_Delay(15);
		elm_puts(ELM327_ID_STRING); elm_eol();
		elm_flush();

		uint32_t t0 = currentTime;
		while(currentTime - t0 < 2000){
			cdc_process();						//continua a raccogliere i byte dall'usb
			int16_t c = elm_ring_get();
			if(c < 0) continue;
			if(c == '\r' || c == '\n') break;
		}
		while(elm_ring_get() >= 0);				//scarta il resto
		cmdLen = 0;
		elm_puts("OK"); elm_eol();
		return;
	}

	#ifndef ELM327_TRACE_DISABLE
	//ATLOG: stampa il dialogo registrato (non e' un comando ELM327, serve per la diagnosi)
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

	// Comando AT non riconosciuto. L'ELM327 originale risponde "?", ma diversi tool
	// interrompono la connessione appena lo ricevono: di default rispondiamo "OK".
	#ifdef ELM327_UNKNOWN_AT_IS_ERROR
		elm_line("?");
	#else
		elm_line("OK");
	#endif
}

// ---- modalita' raw (ATCAF0): usata da AlfaOBD -------------------------------
// Con l'auto formatting disattivato l'ELM327 non aggiunge e non toglie niente:
// trasmette i byte esattamente come li riceve (compreso il byte PCI) e stampa i
// frame ricevuti uno per riga, cosi' come sono. La gestione ISO-TP (frammentazione
// e flow control) la fa il programma sul PC.
static uint8_t elm_handle_obd_raw(const uint8_t *payload, uint8_t payloadLen, uint8_t expectedResponses){
	if(!elm_bus_send(payload, payloadLen, canSendHeader, extendedHeader)){
		return 0;
	}

	CAN_RxHeaderTypeDef resp;
	uint8_t  rd[8];
	uint8_t  printed = 0;
	uint8_t  sawPending = 0;	//1 se la centralina ha gia' risposto "sto lavorando"
	uint32_t t0    = currentTime;
	uint32_t start = currentTime;

	//limite assoluto: su un bus molto trafficato e senza filtro non si esce piu'
	uint16_t waitMs = elm_wait_budget();
	while((currentTime - t0 < waitMs) &&
	      (currentTime - start < (uint32_t)waitMs * 4 + 100) &&
	      (printed < ELM327_MAX_RAW_FRAMES)){
		elm_bus_pump();
		if(elm_remote_finished()) break;	//lo slave ha chiuso: la linea e' gia' libera
		if(!elm_can_get_frame(&resp, rd)) continue;

		onboardLed_blue_on();
		elm_print_header(&resp);
		for(uint8_t i = 0; i < resp.DLC && i < 8; i++){
			if(spacesOn && i > 0) elm_putc(' ');
			elm_puthex(rd[i]);
		}
		elm_eol();
		// Spedita subito, senza aspettare il prompt: e' cosi' che si comporta l'ELM327 vero
		// e i programmi contano su questo per capire che la centralina sta rispondendo.
		// Tenere la riga in buffer faceva scadere il timeout del programma sulle scritture
		// lunghe, dove fra "sto lavorando" e conferma passa quasi un secondo.
		elm_flush();
		printed++;

		//se l'ecu manda un First Frame e il flow control automatico e' attivo, rispondiamo noi
		if((rd[0] & 0xF0) == 0x10 && cfcOn) elm_send_flow_control();

		//se il tool ha indicato quante risposte aspettarsi, ci fermiamo appena le abbiamo
		if(expectedResponses && printed >= expectedResponses){ elm_remote_drain(); return 1; }

		// Coppia tipica delle scritture (allineamento proxi): prima "sto lavorando"
		// (7F xx 78), poi la risposta vera. Appena arriva la seconda si chiude subito
		// invece di restare in ascolto fino al timeout: e' quel ritardo che faceva
		// scadere l'attesa del programma e mandava le risposte fuori sincrono.
		if((rd[0] & 0xF0) == 0x00){
			if(rd[1] == 0x7F && rd[3] == 0x78) sawPending = 1;
			else if(sawPending){ elm_remote_drain(); return 1; }
		}

		t0 = currentTime;	//ci sono altri frame in arrivo: riparte l'attesa
	}

	elm_remote_drain();		//linea libera prima di ridare il prompt al programma
	return printed ? 1 : 0;
}

// ---- modalita' automatica (ATCAF1): PCI aggiunto e risposta ISO-TP ricomposta -----
static uint8_t elm_handle_obd_caf(const uint8_t *payload, uint16_t payloadLen, uint8_t expectedResponses){
	(void)expectedResponses;
	uint8_t frame[8];

	if(payloadLen <= 7){
		// ---- Single Frame ----
		memset(frame, ELM327_PAD_BYTE, sizeof(frame));
		frame[0] = (uint8_t)payloadLen;
		memcpy(&frame[1], payload, payloadLen);
		//ATV1: DLC pari ai soli byte utili; ATV0 (default): riempimento fino a 8
		uint8_t dlc = variableDlc ? (uint8_t)(payloadLen + 1) : 8;
		if(!elm_bus_send(frame, dlc, canSendHeader, extendedHeader)){
			return 0;
		}
	}else{
		// ---- First Frame + Consecutive Frames (richieste piu' lunghe di 7 byte) ----
		memset(frame, ELM327_PAD_BYTE, sizeof(frame));
		frame[0] = (uint8_t)(0x10 | ((payloadLen >> 8) & 0x0F));
		frame[1] = (uint8_t)(payloadLen & 0xFF);
		memcpy(&frame[2], payload, 6);
		if(!elm_can_send_frame(frame)){ return 0; }

		//attesa del Flow Control dell'ecu
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
		if(stMin > 127) stMin = 1;	//valori 0xF1..0xF9 sono microsecondi: arrotondiamo a 1ms

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

			//se l'ecu ha chiesto blocchi limitati, attendiamo il prossimo Flow Control
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

	// ------------------ attesa e ricomposizione della risposta ------------------
	static uint8_t isoData[ELM327_ISOTP_MAX_LEN];
	uint16_t isoTotal    = 0;
	uint16_t isoReceived = 0;
	uint8_t  isoStarted  = 0;
	uint8_t  isoSN       = 1;

	CAN_RxHeaderTypeDef resp;
	uint8_t  rd[8];
	uint8_t  answered = 0;		//1 se qualcosa e' gia' stato passato al programma
	uint32_t t0 = currentTime;

	uint16_t waitMs = elm_wait_budget();
	while(currentTime - t0 < waitMs){
		elm_bus_pump();
		if(elm_remote_finished()) break;	//lo slave ha chiuso: non arrivera' altro								//coda di trasmissione locale o frame dal bus remoto
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
			elm_flush();	//spedito subito, come fa l'ELM327 vero

			// "Richiesta ricevuta, sto lavorando" (7F xx 78): non e' la risposta finale.
			// La si passa subito al programma (cosi' sa che la centralina c'e') e si
			// continua ad aspettare quella vera, che nelle scritture lunghe come
			// l'allineamento proxi arriva anche mezzo secondo dopo.
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
		//i frame di Flow Control (0x30) in ingresso vengono ignorati
	}

	elm_remote_drain();		//linea libera prima di ridare il prompt al programma
	//se e' passato almeno un "sto lavorando" il programma una risposta l'ha avuta
	return answered;
}

// ------------------------------------------------------------------ comandi OBD (hex)
// Sceglie il bus e passa la richiesta al gestore giusto. Con il gateway attivo, se sul bus
// locale non risponde nessuno la stessa richiesta viene inoltrata agli altri due chip; il
// bus che risponde viene ricordato, cosi' le richieste successive vanno dirette.
static void elm_handle_obd(const char *hexCmd){
	uint8_t  payload[32];
	uint16_t payloadLen = 0;
	uint16_t l = (uint16_t)strlen(hexCmd);
	uint8_t  expectedResponses = 0;

	// Una cifra finale in piu' indica il numero di risposte attese (es. "0100 1"), ma vale
	// solo con la formattazione automatica attiva: con ATCAF0 ogni byte e' dato grezzo e
	// l'ELM327 originale rifiuta il comando con "?". Verificato sul log di un ELM327 vero:
	// MultiECUScan riceve il "?" e rimanda subito il comando senza la cifra.
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

	//se il bus e' gia' noto si va diretti con il timeout pieno, altrimenti si sondano
	//i bus con un timeout ridotto per non pagare 612 ms per ogni tentativo a vuoto
	uint8_t probing = (candidates > 1);

	for(uint8_t k = 0; k < candidates; k++){
		activeBus  = order[k];
		rspTimeout = probing ? ELM327_PROBE_TIMEOUT_MS : cmdTimeout;
		if(rspTimeout > cmdTimeout) rspTimeout = cmdTimeout;
		trace_str(activeBus == ELMLINK_BUS_LOCAL ? "{C1}" :
		          (activeBus == ELMLINK_BUS_C2   ? "{C2}" : "{BH}"));
		if(activeBus != ELMLINK_BUS_LOCAL){
			elm_remote_clear();
			//la configurazione si manda solo quando cambia: meno blocchi sulla linea,
			//meno finestre di collisione e comandi piu' rapidi
			if(cfgSentBus != activeBus || cfgSentFilter != canFilterValue ||
			   cfgSentMask != canFilterMask || cfgSentTimeout != rspTimeout || cfgSentFc != cfcOn){
				elmlink_send_config(activeBus, canFilterValue, canFilterMask, rspTimeout, cfcOn);
				cfgSentBus = activeBus;       cfgSentFilter  = canFilterValue;
				cfgSentMask = canFilterMask;  cfgSentTimeout = rspTimeout; cfgSentFc = cfcOn;
			}
			//se il programma ha impostato un flow control personalizzato, lo slave deve saperlo
			if(fcMode && fcDataLen)
				elmlink_send_fc_config(activeBus, fcHeader ? fcHeader : canSendHeader,
				                       fcHeader ? fcHeaderExt : extendedHeader, fcData, fcDataLen);
		}

		uint8_t answered = cafOn
			? elm_handle_obd_caf(payload, payloadLen, expectedResponses)
			: elm_handle_obd_raw(payload, (uint8_t)payloadLen, expectedResponses);

		if(answered){
			elm_route_store(elm_target_addr(), activeBus);	//da ora si va diretti
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
	//registra il comando ricevuto (l'eco dell'uscita viene registrata da elm_putc)
	trace_str("\r\n<");
	trace_str(cmd);
	trace_str(">");

	if(echoOn){					//eco del comando ricevuto, come fa l'ELM327 reale
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
		elm_putc('\r');		//riga vuota prima del prompt, come l'ELM327 originale
	#endif
	elm_putc('>');				//prompt
	elm_flush();
}

// Interpreta al massimo un comando per chiamata, cosi' il main loop resta reattivo.
void elm327_process(void){
	int16_t c;

	#if defined(C1baccable)
		if(!elmModeOn) return;					//modalita' spenta: la BACCAble lavora normalmente

		// Via d'uscita automatica: con l'ELM327 acceso il quadro non risponde ai pulsanti,
		// quindi dal menu non si potrebbe piu' spegnere. Se per un po' non arriva niente dal
		// PC (cavo staccato, programma chiuso) si torna da soli al funzionamento normale.
		if((currentTime - elmLastCmd) > ELM327_IDLE_EXIT_MS){
			elm327_set_enabled(0);
			return;
		}
	#endif

	//se un comando e' rimasto incompleto (nessun CR) lo scartiamo dopo un secondo:
	//evita che un frammento di una sessione precedente rovini il comando successivo.
	if(cmdLen && (currentTime - lastRxByteTime) > 1000) cmdLen = 0;

	while((c = elm_ring_get()) >= 0){
		lastRxByteTime = currentTime;
		#if defined(C1baccable)
			elmLastCmd = currentTime;			//c'e' vita sulla porta: la modalita' resta accesa
		#endif
		if(c == '\r' || c == '\n'){
			if(cmdLen == 0) continue;			//riga vuota: ELM ripeterebbe l'ultimo comando, qui la ignoriamo
			cmdBuf[cmdLen] = '\0';
			cmdLen = 0;
			elm_execute(cmdBuf);
			return;								//un comando per giro di loop
		}
		if(c == ' ') continue;					//l'ELM327 ignora gli spazi nei comandi
		if(c < 0x20 || c >= 0x7F) continue;		//scarta i caratteri non stampabili
		if(cmdLen >= (ELM327_CMD_BUF_LEN - 1)){	//comando troppo lungo: lo si scarta
			cmdLen = 0;
			continue;
		}
		if(c >= 'a' && c <= 'z') c = (int16_t)(c - 'a' + 'A');
		cmdBuf[cmdLen++] = (char)c;
	}
}

#endif //ACT_AS_ELM327
