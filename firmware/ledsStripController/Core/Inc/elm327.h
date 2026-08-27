/*
 * elm327.h
 *
 *  Emulazione di un'interfaccia ELM327 (ISO 15765-4 / CAN) sulla porta USB CDC
 *  della BACCAble, pensata per MultiECUScan ("ELM327 High Speed").
 *
 *  Portato dalla versione ESP32-C3 (TWAI) alle API HAL/bxCAN dell'STM32F072.
 *
 *  Differenze principali rispetto alla versione ESP32:
 *   - il trasporto e' USB CDC: il baud rate e' virtuale, quindi ATBRD esegue
 *     solo l'handshake formale senza cambiare nulla di reale;
 *   - can_tx() di BACCAble accoda soltanto: durante le attese va chiamata
 *     can_process(), altrimenti il frame non parte mai;
 *   - CDC_Transmit_FS() scarta i pacchetti piu' lunghi di TX_BUF_SIZE (64B),
 *     quindi l'output viene trasmesso a blocchi;
 *   - e' supportato anche l'invio ISO-TP multi-frame (richieste > 7 byte).
 */

#ifndef INC_ELM327_H_
#define INC_ELM327_H_

	#include "compile_time_defines.h"

	#ifdef ACT_AS_ELM327

		#include "stm32f0xx_hal.h"
		#include "can.h"

		//stringa di identificazione restituita da ATZ / ATI / ATWS
		#define ELM327_ID_STRING		"ELM327 v1.4"
		//risposta ad AT@1: e' quella esatta del chip originale (senza trattino in "OBDII"),
		//i tool la confrontano per riconoscere l'interfaccia
		#define ELM327_DESCR_STRING		"OBDII to RS232 Interpreter"

		// Dopo il reset l'ELM327 originale ha l'ECO ATTIVO, i LINE FEED SPENTI e mette una
		// riga vuota prima del prompt. Molti programmi (AlfaOBD tra questi) scartano la prima
		// riga ricevuta perche' si aspettano l'eco del comando: con l'eco spento buttavano via
		// la risposta vera e non riconoscevano l'interfaccia.
		// Commentando questa riga si torna al comportamento precedente (eco spento, line feed attivi).
		#define ELM327_STRICT_ELM_DEFAULTS

		#define ELM327_RX_RING_LEN		256		//buffer circolare dei byte in arrivo da usb
		#define ELM327_CMD_BUF_LEN		80		//lunghezza massima di un comando
		#define ELM327_ISOTP_MAX_LEN	255		//massima lunghezza di una risposta ISO-TP
		#define ELM327_TX_CHUNK_LEN		60		//blocco di trasmissione usb (< TX_BUF_SIZE)
		#define ELM327_PAD_BYTE			0xAA	//riempimento dei frame can (come da firmware esp32)

		// Formato dell'header stampato con ATH1.
		// Di default vengono stampati i byte separati (es. "07 E8 ..."), come nel firmware
		// esp32 gia' provato con MultiECUScan. Definendo ELM327_HEADER_3DIGITS si ottiene
		// invece il formato dell'ELM327 originale (es. "7E8 ..."): da provare se il tool
		// dovesse rifiutare le risposte a 11 bit.
		//#define ELM327_HEADER_3DIGITS

		// Comando AT non riconosciuto: di default rispondiamo "OK" perche' diversi tool
		// (AlfaOBD tra questi) chiudono la connessione appena ricevono un "?".
		// Definendo ELM327_UNKNOWN_AT_IS_ERROR si torna alla risposta "?" dell'ELM327 originale.
		//#define ELM327_UNKNOWN_AT_IS_ERROR

		// Con ATS0 l'header viene stampato senza spazi, come sull'ELM327 originale.
		// Definendo ELM327_HEADER_ALWAYS_SPACED si torna al comportamento della prima
		// versione (byte dell'header sempre separati da spazio, anche con ATS0).
		//#define ELM327_HEADER_ALWAYS_SPACED

		#define ELM327_MAX_RAW_FRAMES		64	//max frame stampati per comando in modalita' ATCAF0

		// Quando una centralina ha risposto "sto lavorando" (7F xx 78), la risposta che
		// segue e' quella definitiva: si chiude subito invece di restare in ascolto fino
		// al timeout, altrimenti il prompt arriva al programma con un secondo di ritardo.
		#define ELM327_ROUTE_CACHE_LEN		16	//quante centraline ricordare (indirizzo -> bus)
		#define ELM327_REMOTE_FIFO_LEN		12	//frame in arrivo da un bus remoto, in attesa di lettura

		// Quando ancora non si sa su quale bus stia una centralina, i bus si provano con
		// questo timeout ridotto invece di quello chiesto dal programma (che con ATST99 e'
		// di 612 ms per tentativo). Una centralina in sessione diagnostica risponde in
		// decine di millisecondi: appena il bus e' noto si torna al timeout pieno.
		#define ELM327_PROBE_TIMEOUT_MS		200

		#define ELM327_DEFAULT_TIMEOUT_MS	200	//timeout di default (ATST non impostato)
		#define ELM327_FC_TIMEOUT_MS		250	//attesa massima del Flow Control dell'ecu

		// Alcuni tool si aspettano il CR "vuoto" che l'ELM327 originale manda prima del
		// prompt ">". Attivalo se un programma non riconosce le risposte.
		//#define ELM327_EXTRA_CR_BEFORE_PROMPT

		// Registratore del dialogo con il PC: tiene in memoria gli ultimi comandi ricevuti e
		// le risposte inviate. Serve per capire cosa manda un programma che non si collega:
		// dopo il tentativo fallito basta aprire un terminale seriale e scrivere ATLOG
		// (ATLOGC azzera il registro). Per toglierlo del tutto: ELM327_TRACE_DISABLE.
		//#define ELM327_TRACE_DISABLE
		#define ELM327_TRACE_LEN		2048	//byte di memoria dedicati al registro

		void elm327_init(void);					//da chiamare dopo l'init di can e usb
		void elm327_port_reset(void);			//host che apre/chiude la porta: scarta i dati vecchi
		void elm327_rx_byte(uint8_t c);			//chiamata da cdc_process (contesto con irq disabilitati): accoda soltanto
		void elm327_process(void);				//chiamata dal main loop: interpreta ed esegue un comando per volta

		#if defined(C1baccable)
			// Accensione e spegnimento dal menu del quadro (voce "ELM327 Diag").
			// Da spento l'interprete non tocca niente: non legge l'USB, non tocca il bus CAN,
			// non parla sulla linea seriale fra i chip. La BACCAble lavora normalmente.
			void elm327_set_enabled(uint8_t on);
			uint8_t elm327_is_enabled(void);

			// Sicurezza: se per questo tempo non arriva nessun comando dal PC la modalita' si
			// spegne da sola e la BACCAble torna a funzionare normalmente. Serve perche' con
			// l'ELM327 acceso il quadro non risponde piu' ai pulsanti, quindi senza una via
			// d'uscita automatica si resterebbe bloccati fino allo stacco della batteria.
			#ifndef ELM327_IDLE_EXIT_MS
				#define ELM327_IDLE_EXIT_MS		120000	//due minuti
			#endif
		#endif

	#endif //ACT_AS_ELM327

#endif /* INC_ELM327_H_ */
