/*
 * elmlink.h
 *
 *  Ponte diagnostico fra i tre chip della BACCAble.
 *
 *  Il chip collegato al bus C1 fa da gateway: riceve i comandi ELM327 dalla porta USB e,
 *  quando la centralina cercata non sta sul suo bus, inoltra il frame CAN al chip del bus
 *  C2 o BH attraverso la linea seriale che gia' collega i tre processori (USART2,
 *  half-duplex a filo singolo, 38400 baud). Lo slave trasmette il frame sul proprio bus,
 *  raccoglie le risposte e le rimanda indietro. Cosi' con una sola porta USB si raggiungono
 *  tutte e tre le reti, senza spostare l'adattatore sull'OBD.
 *
 *  Il modulo e' scritto per poter essere acceso a runtime (elmlink_set_enabled): oggi viene
 *  compilato nei flavor dedicati, domani potra' stare dentro il firmware normale e venire
 *  attivato dal menu del quadro strumenti.
 *
 *  Formato dei messaggi: si riusa l'incapsulamento gia' presente sulla linea (blocchi di
 *  UART_BUFFER_SIZE byte, primo byte = destinatario), aggiungendo tre nuovi destinatari.
 *  In questo modo il ponte convive con i messaggi normali fra i chip.
 *
 *      [0]     destinatario   0x0E = chip C2, 0x0F = chip BH, 0x10 = master (C1)
 *      [1]     tipo           1 = REQ, 2 = CFG, 3 = RSP, 4 = END
 *      [2]     flag           bit0 = id a 29 bit, bit1 = nessuna risposta (su END)
 *      [3..6]  id CAN (big endian)      | CFG: valore del filtro
 *      [7]     dlc                      | CFG: non usato
 *      [8..15] dati (8 byte)            | CFG: [8..11] maschera, [12..13] timeout ms
 *      [16]    numero di sequenza
 *      [17]    checksum (xor dei byte 0..16)
 *      [18]    riempimento
 */

#ifndef INC_ELMLINK_H_
#define INC_ELMLINK_H_

	#include "compile_time_defines.h"

	//destinatari dei messaggi del ponte (continuano la numerazione di uart.h)
	#define ELMLINK_TO_C2			0x0E
	#define ELMLINK_TO_BH			0x0F
	#define ELMLINK_TO_MASTER		0x10

	//tipi di messaggio
	#define ELMLINK_TYPE_REQ		0x01	//master -> slave: trasmetti questo frame
	#define ELMLINK_TYPE_CFG		0x02	//master -> slave: filtro di ricezione e timeout
	#define ELMLINK_TYPE_RSP		0x03	//slave -> master: frame ricevuto
	#define ELMLINK_TYPE_END		0x04	//slave -> master: risposte finite
	#define ELMLINK_TYPE_FCCFG		0x05	//master -> slave: flow control personalizzato (ATFCSH/ATFCSD)
	#define ELMLINK_TYPE_ARM		0x06	//master -> slave: accendi/spegni il ponte (menu del quadro)

	#define ELMLINK_FLAG_EXTID		0x01
	#define ELMLINK_FLAG_NODATA		0x02
	#define ELMLINK_FLAG_ARM_ON		0x01	//in un messaggio ARM: 1 = accendi, 0 = spegni
	#define ELMLINK_CFG_AUTOFC		0x01	//flag in CFG[14]: lo slave manda da solo il flow control

	//identificatori di bus usati dal gateway
	#define ELMLINK_BUS_LOCAL		0		//il bus del chip stesso (C1)
	#define ELMLINK_BUS_C2			1
	#define ELMLINK_BUS_BH			2
	#define ELMLINK_BUS_COUNT		3

	#define ELMLINK_DEFAULT_TIMEOUT_MS	300	//attesa massima della PRIMA risposta sullo slave

	// Dopo che almeno un frame e' stato inoltrato non ha senso aspettare il timeout pieno:
	// si aspetta solo il tempo in cui potrebbe arrivare il frame successivo. Quanto, dipende
	// da cosa e' appena passato (byte PCI dell'ISO-TP):
	#define ELMLINK_GAP_AFTER_SF_MS		40	//frame singolo: al massimo risponde un'altra centralina
	#define ELMLINK_GAP_AFTER_FF_MS		250	//primo frame: l'ecu attende il flow control, che fa il giro della seriale
	#define ELMLINK_GAP_AFTER_CF_MS		80	//frame consecutivi: arrivano uno dietro l'altro

	// Eccezione: se la centralina risponde "richiesta ricevuta, sto lavorando" (UDS 7F xx 78,
	// tipico delle scritture come l'allineamento proxi) la risposta vera puo' arrivare anche
	// dopo qualche secondo. In quel caso si aspetta tutto il tempo che il programma ha chiesto,
	// altrimenti la scrittura risulta andata in timeout anche se la centralina la conclude bene.
	#define ELMLINK_UART_TX_TIMEOUT_MS	30	//attesa massima per mettere un blocco sulla linea

#if defined(C1baccable) || defined(C2baccable) || defined(BHbaccable)

	#include "stm32f0xx_hal.h"
	#include "can.h"

	// --- comune -------------------------------------------------------------------
	void elmlink_init(void);
	void elmlink_set_enabled(uint8_t on);	//accensione/spegnimento a runtime (menu del quadro)
	uint8_t elmlink_is_enabled(void);
	//da chiamare dentro HAL_UART_RxCpltCallback quando arriva un blocco del ponte:
	//restituisce 1 se il messaggio e' stato preso in carico
	uint8_t elmlink_on_uart_frame(const uint8_t *frame);
	void elmlink_process(void);				//da chiamare nel main loop

	// --- lato slave (chip C2 / BH) ------------------------------------------------
	#if defined(C2baccable) || defined(BHbaccable)
		//nessuna API pubblica: lo slave lavora dentro elmlink_process()
	#endif

	// --- lato master (chip C1, gateway) -------------------------------------------
	#if defined(C1baccable)
		// Accende (o spegne) il ponte sui due chip remoti. Lo chiama il C1 quando si sceglie
		// "ELM327 Diag" nel menu del quadro: prima di quel momento C2 e BH lavorano normalmente
		// e ignorano i messaggi del ponte.
		void elmlink_send_arm(uint8_t on);

		//comunica allo slave il filtro di ricezione e il timeout da usare
		void elmlink_send_config(uint8_t bus, uint32_t filterValue, uint32_t filterMask,
		                         uint16_t timeoutMs, uint8_t autoFlowControl);

		//flow control personalizzato (ATFCSH / ATFCSD): serve solo se il programma lo imposta
		void elmlink_send_fc_config(uint8_t bus, uint32_t fcHeader, uint8_t fcExt,
		                            const uint8_t *fcData, uint8_t fcLen);

		// Invia la richiesta su un bus remoto e torna SUBITO: le risposte si raccolgono
		// con elmlink_poll() dentro il normale ciclo di attesa, cosi' ogni frame puo'
		// essere mandato al PC appena arriva, esattamente come sul bus locale.
		uint8_t elmlink_send_request(uint8_t bus, uint32_t canId, uint8_t ext,
		                             const uint8_t *data, uint8_t dlc);

		// Da chiamare nei cicli di attesa: consegna i frame arrivati dallo slave.
		// Restituisce 1 quando lo slave ha comunicato che non arrivera' altro.
		uint8_t elmlink_poll(void (*onFrame)(uint32_t id, uint8_t ext, const uint8_t *d, uint8_t dlc));
	#endif

#endif //C1baccable || C2baccable || BHbaccable

#endif /* INC_ELMLINK_H_ */
