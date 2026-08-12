/*
 * processingMessage0x00000226.c
 *
 *  Created on: May 3, 2025
 *      Author: GauchoHP
 */

#include "processingExtendedMessage.h"
#include "security_access_mm10ja.h" //pumpForce test25/07/2026 - algoritmo MM10JA per calcolo key ECM

void processingExtendedMessage(){
	#if defined(C1baccable)
		if(immobilizerEnabled && (engineOnSinceMoreThan5seconds<500)){ //if immo enabled and engine is off
			//if it is a message of connection to RFHUB, reset the connection periodically, but start the panic alarm only once
			if(floodTheBus==0){ //if we are not flooding the bus
				uint8_t responseOffset=rx_msg_data[0]>>4; //0=single frame , 1=first fragmented frame 2=fragmented frame, 3=frame ack
				if((rx_msg_header.ExtId & 0xFFFFFFF0)==0x18DAC7F0){ 		//if it is message from the thief
					if(responseOffset<2){ //we pass this if, in case of single frame and first fragmented frame
						switch(rx_msg_data[responseOffset+1]){
							case 0x10: //diagnostic session
							case 0x27: //security access
							case 0x29: //authentication
							case 0x3E: //tester presence
							//case 0x1A: //??
							case 0x2E: //write data by identifier
							case 0x3D: //write memory by address
								floodTheBus=1; //reset the RFHUB and start the alarm
								break;
							default:
								break;
						}
					}
				}else if((rx_msg_header.ExtId & 0xFFFFF0FF)==0x18DAF0C7) { 	//if it is a reply from rfhub
					//if(floodTheBusStartTime==0){ //this allows to read rfhub messages only if it was the first time
						if(responseOffset<2){ //we pass this if, in case of single frame and first fragmented frame
							switch(rx_msg_data[responseOffset+1]){
								case 0x50: //diagnostic session	//
								case 0x67: //security access
								case 0x69: //authentication
								case 0x7E: //tester presence 	//
								//case 0x1A: //??
								case 0x6E: //write data by identifier
								case 0x7D: //write memory by address
									floodTheBus=1; //reset the RFHUB and start the alarm
									break;
								default:
									break;
							}
						}
					//}
				}
				if(floodTheBus==1){ //if we engaged the immobilizer
					floodTheBusStartTime=currentTime; //set initial time we started to flood the bus
					onboardLed_blue_on(); //light a led
				}

			}
		} //end of immobilizer section

		if ((rx_msg_header.ExtId==single_uds_params_array[uds_params_array[function_is_diesel_enabled][dashboardPageIndex].udsParamId[currentParamElementSelection]].replyId) && baccableDashboardMenuVisible){ //if we received UDS message with current selected parameter, let's aquire it
			if(dashboard_menu_indent_level==1 && main_dashboardPageIndex==1){ //if we are in show params menu
				onboardLed_blue_on();
				if (rx_msg_header.DLC>=4+single_uds_params_array[uds_params_array[function_is_diesel_enabled][dashboardPageIndex].udsParamId[currentParamElementSelection]].replyOffset+single_uds_params_array[uds_params_array[function_is_diesel_enabled][dashboardPageIndex].udsParamId[currentParamElementSelection]].replyLen){
					uint8_t numberOfBytesToRead=single_uds_params_array[uds_params_array[function_is_diesel_enabled][dashboardPageIndex].udsParamId[currentParamElementSelection]].replyLen;
					// Limita il numero di byte a un massimo di 4 per evitare overflow
					if (numberOfBytesToRead > 4) {
						numberOfBytesToRead = 4;
					}
					uint32_t tmpVal=0; //take value of received parameter

					// Costruisce il valore a partire dai byte ricevuti
					for (size_t i = 0; i < numberOfBytesToRead; i++) {
						tmpVal |= ((uint32_t)rx_msg_data[4+single_uds_params_array[uds_params_array[function_is_diesel_enabled][dashboardPageIndex].udsParamId[currentParamElementSelection]].replyOffset+i]) << (8 * (numberOfBytesToRead - 1 - i));
					}

					tmpVal+=single_uds_params_array[uds_params_array[function_is_diesel_enabled][dashboardPageIndex].udsParamId[currentParamElementSelection]].replyValOffset;
					float tmpVal2 =tmpVal * single_uds_params_array[uds_params_array[function_is_diesel_enabled][dashboardPageIndex].udsParamId[currentParamElementSelection]].replyScale;
					tmpVal2 +=single_uds_params_array[uds_params_array[function_is_diesel_enabled][dashboardPageIndex].udsParamId[currentParamElementSelection]].replyScaleOffset;

					if(uds_params_array[function_is_diesel_enabled][dashboardPageIndex].udsParamId[currentParamElementSelection]== uds_params_array[function_is_diesel_enabled][dashboardPageIndex].udsParamId[!currentParamElementSelection]){
						currentParamElementSelection=0; //single param
					}

					if(maxHold_enabled){
						if(isnan(dashboardParamCouple[currentParamElementSelection])){
							dashboardParamMaxHold[currentParamElementSelection]=tmpVal2; //first update after navigation: reset max hold
						}else if(tmpVal2 > dashboardParamMaxHold[currentParamElementSelection]){
							dashboardParamMaxHold[currentParamElementSelection]=tmpVal2; //new maximum found
						}
						dashboardParamCouple[currentParamElementSelection]=dashboardParamMaxHold[currentParamElementSelection];
					}else{
						dashboardParamCouple[currentParamElementSelection]=tmpVal2; //normal behavior
					}
					sendDashboardPageToSlaveBaccable();//send parameters to BH
				}
			}
		}
		/*
		if(seatbeltAlarmDisabled==0xfe){ //if seatbelt status acquisition is in progress
			if(rx_msg_header.ExtId==0x18DAF160){ //if received message comes from IPC
				if (rx_msg_header.DLC>=5){ //if at least 5 bytes
					if(rx_msg_data[1]==0x62){ //if param read reply successful
						if((rx_msg_data[2]==0x55) && (rx_msg_data[3]==0xA0 )){ //if param. 55A0 (seat belt alarm status)
							seatbeltAlarmDisabled= !(rx_msg_data[4]); //coherce to boolean and negate
						}
					}
				}
			}
		}
		*/

		if((seatbeltAlarmDisabled==0x11) || (seatbeltAlarmDisabled==0x21)){ //if write param was sent (seatbelt disabling or enabling in progress)
			if(rx_msg_header.ExtId==0x18DAF160){ //if received message comes from IPC
				if (rx_msg_header.DLC>=4){ //if at least 4 bytes
					if(rx_msg_data[1]==0x6F){ //if write param reply successful
						if((rx_msg_data[2]==0x55) && (rx_msg_data[3]==0xA0)){ //if param wrote was 55A0 (en/dis seatbelt alam)
							if(seatbeltAlarmDisabled==0x11){ // if seatbelt disabling
								seatbeltAlarmDisabled=1; //seatbelt disabled
							}
							if(seatbeltAlarmDisabled==0x21){ // if seatbelt enabling
								seatbeltAlarmDisabled=0; //seatbelt enabled
							}
						}
					}
				}
			}
		}


		if((seatbeltAlarmDisabled==0x10) || (seatbeltAlarmDisabled==0x20)){ //if diag session was sent (seatbelt disabling or enabling is in progress)
			if(rx_msg_header.ExtId==0x18DAF160){ //if received message comes from IPC
				if (rx_msg_header.DLC>=2){ //if at least 2 bytes
					if(rx_msg_data[1]==0x50){ //if diag session reply successful
						// Send enable/disable seatbelt alarm message
						uds_parameter_request_msg_header.ExtId=0x18DA60F1;
						uds_parameter_request_msg_header.DLC=6;
						uds_parameter_request_msg_data[0]=0x05;
						uds_parameter_request_msg_data[1]=0x2F;
						uds_parameter_request_msg_data[2]=0x55;
						uds_parameter_request_msg_data[3]=0xA0;
						uds_parameter_request_msg_data[4]=0x03;

						if(seatbeltAlarmDisabled==0x10){ // if seatbelt disabling
							uds_parameter_request_msg_data[5]=0x00; //set byte to disable alarm
						}
						if(seatbeltAlarmDisabled==0x20){ // if seatbelt enabling
							uds_parameter_request_msg_data[5]=0x01; //send msg to enable alarm
						}
						// send message
						can_tx(&uds_parameter_request_msg_header, uds_parameter_request_msg_data); //transmit the diag session request

						seatbeltAlarmDisabled++; //record that operation was executed
						seatbeltAlarmStatusRequestTime=currentTime;
						last_sent_uds_parameter_request_Time=currentTime;
					}
				}
			}
		}



		if(function_route_msg_enabled==1){
			if (rx_msg_header.ExtId==0x18DABAF1){ //if route request and dashboard menu not shown to avoid conflicts
				if (rx_msg_header.DLC>=7){
					routeStdIdMsg=!(rx_msg_data[2]>>4); //standard or extended msgID route request
					routeOffset=(rx_msg_data[2] & 0x0F); //offset from which start to copy
					routeMsgData[2]=rx_msg_data[2]; //copy in the response

					routeMsgId=	((uint32_t)rx_msg_data[3] << 24) |  // MSB
								((uint32_t)rx_msg_data[4] << 16) |
								((uint32_t)rx_msg_data[5] << 8)  |
								((uint32_t)rx_msg_data[6]);       	 // LSB



					onboardLed_blue_on();
				}
			}

			if(baccableDashboardMenuVisible) routeStdIdMsg=0xff; //disables the route request, to avoid conflicts with show params functionality

			if(routeStdIdMsg==0){ //if we have to do it (ext id route request)
				if(rx_msg_header.ExtId==routeMsgId){ //received msg to route
					routeStdIdMsg=0xFF; //set this to disable the request. only one message is routed to avoid bus flood
					if(routeOffset<rx_msg_header.DLC){ //send only if offset is correct
						uint8_t sizeToCopy=5; //
						if((rx_msg_header.DLC - routeOffset )<sizeToCopy) sizeToCopy=rx_msg_header.DLC - routeOffset;
						memcpy(&routeMsgData[3],&rx_msg_data[routeOffset],sizeToCopy);
						if(sizeToCopy<5) memset(&routeMsgData[3+sizeToCopy],0x00, 5-sizeToCopy);

						//send it
						can_tx(&routeMsgHeader, routeMsgData);
						onboardLed_blue_on();
					}

				}
			}
		}

		//pumpForce test25/07/2026 - BEGIN
		// -----------------------------------------------------------------------
		// PUMP FORCE — Gestione risposte ECM (ECU 0x10, ExtId risposta 0x18DAF110)
		//
		// Sequenza UDS attesa:
		//   Stato 0: ricezione 50 03  → invia 27 01 (request seed)        → stato 1
		//   Stato 1: ricezione 67 01  → calcola key MM10JA, invia 27 02   → stato 2
		//   Stato 2: ricezione 67 02  → prepara frame IO Control           → stato 3
		//   Stato 3: invio periodico 2F 50 11 03 FF gestito in C1baccablePeriodicCheck()
		//   Qualsiasi 7F: abort incondizionato (no retry)
		//
		// Formato ISO-TP single-frame (tutti i messaggi di questa sequenza):
		//   byte[0] = PCI  (nibble alto=0 → single frame; nibble basso = n byte dati)
		//   byte[1] = SID risposta (SID richiesta + 0x40)
		//   byte[2] = subfunction o DID high
		//   byte[3..] = payload (seed, key, DID low, controlOption, controlValue)
		// -----------------------------------------------------------------------
		if (rx_msg_header.ExtId == 0x18DAF110 && pumpForceStateMachine != 0xFF) {

			// Risposta negativa (0x7F xx xx): la ECM ha rifiutato l'ultima richiesta
			if (rx_msg_header.DLC >= 3 && rx_msg_data[1] == 0x7F) {
				pumpForceStateMachine = 0xFF; // abort definitivo, nessun retry automatico

			// Stato 0 → 1: conferma sessione estesa ricevuta ([06, 50, 03, 00, 32, 01, F4])
			} else if (pumpForceStateMachine == 0 &&
					   rx_msg_header.DLC >= 3 &&
					   rx_msg_data[1] == 0x50 && rx_msg_data[2] == 0x03) {
				// Sessione aperta: richiedi seed per security access livello 1
				pumpForceTxHeader.DLC = 3;
				pumpForceTxData[0] = 0x02; // PCI: single frame, 2 byte dati
				pumpForceTxData[1] = 0x27; // SID: SecurityAccess
				pumpForceTxData[2] = 0x01; // subfunction: requestSeed livello 1
				can_tx(&pumpForceTxHeader, pumpForceTxData);
				lastPumpForceMsgTime = currentTime; // aggiorna timestamp per calcolo timeout
				pumpForceStateMachine = 1;

			// Stato 1 → 2: seed ricevuto ([06, 67, 01, s0, s1, s2, s3], DLC=7)
			// Il seed è a 4 byte big-endian a partire da byte[3]
			} else if (pumpForceStateMachine == 1 &&
					   rx_msg_header.DLC >= 7 &&
					   rx_msg_data[1] == 0x67 && rx_msg_data[2] == 0x01) {
				ecmSeed[0] = rx_msg_data[3];
				ecmSeed[1] = rx_msg_data[4];
				ecmSeed[2] = rx_msg_data[5];
				ecmSeed[3] = rx_msg_data[6];
				// Calcola la key con algoritmo S1_FGA_ORIGINAL_ECU_Sup0002 (costante interna 0x17591215)
				uint8_t ecmKey[4];
				mm10ja_compute_key(ecmSeed, ecmKey);
				// Invia key: 27 02 + 4 byte key (6 byte dati totali → DLC=7)
				pumpForceTxHeader.DLC = 7;
				pumpForceTxData[0] = 0x06; // PCI: single frame, 6 byte dati
				pumpForceTxData[1] = 0x27; // SID: SecurityAccess
				pumpForceTxData[2] = 0x02; // subfunction: sendKey livello 1
				pumpForceTxData[3] = ecmKey[0];
				pumpForceTxData[4] = ecmKey[1];
				pumpForceTxData[5] = ecmKey[2];
				pumpForceTxData[6] = ecmKey[3];
				can_tx(&pumpForceTxHeader, pumpForceTxData);
				lastPumpForceMsgTime = currentTime;
				pumpForceStateMachine = 2;

			// Stato 2 → 3: accesso sicurezza concesso ([02, 67, 02])
			} else if (pumpForceStateMachine == 2 &&
					   rx_msg_header.DLC >= 3 &&
					   rx_msg_data[1] == 0x67 && rx_msg_data[2] == 0x02) {
				// Prepara il frame IO Control che verrà inviato periodicamente in C1baccablePeriodicCheck()
				// InputOutputControlByIdentifier DID=0x5011, shortTermAdjustment (0x03), val=0xFF (max flow)
				pumpForceTxHeader.DLC = 6;
				pumpForceTxData[0] = 0x05; // PCI: single frame, 5 byte dati
				pumpForceTxData[1] = 0x2F; // SID: InputOutputControlByIdentifier
				pumpForceTxData[2] = 0x50; // DID high byte
				pumpForceTxData[3] = 0x11; // DID low byte
				pumpForceTxData[4] = 0x03; // controlOption: shortTermAdjustment
				pumpForceTxData[5] = 0xFF; // controlValue: massima portata (0xFF)
				lastPumpForceMsgTime = 0;   // azzera il timer → il primo invio avviene al loop successivo
				pumpForceStateMachine = 3;
			}
		}
		//pumpForce test25/07/2026 - END

		//readFaults 12/08/2026 - BEGIN
		// -----------------------------------------------------------------------
		// READ FAULTS — Gestione risposte Body ECU (ECU 0x40, ExtId risposta 0x18DAF140)
		//
		// Sequenza UDS:
		//   Stato 0: ricezione 50 03  → invia 19 02 FF (ReadDTCByStatusMask) → stato 1
		//   Stato 1: single frame (PCI nibble alto=0, SID=59, sub=02) → parsa DTC  → stato 3
		//   Stato 1: first frame  (PCI nibble alto=1, SID=59, sub=02) → invia FC   → stato 2
		//   Stato 2: consecutive frames (PCI nibble alto=2) → accumula buffer      → stato 3
		//   Risposta negativa (SID=7F): transizione forzata a stato 4 (errore/timeout display)
		//
		// Formato payload risposta ReadDTCByStatusMask (0x59 0x02):
		//   [59][02][availMask][DTChi][DTCmid][DTClo][DTCstatus] x N record
		//   Ogni record = 4 byte; parsing: offset 3 = primo DTC high byte
		// -----------------------------------------------------------------------
		if (rx_msg_header.ExtId == 0x18DAF140 && faultsStateMachine != 0xFF) {

			uint8_t pci      = rx_msg_data[0];
			uint8_t pci_type = (pci >> 4) & 0x0F;

			// Risposta negativa (0x7F): abort con display TIMEOUT
			if (rx_msg_header.DLC >= 2 && rx_msg_data[1] == 0x7F) {
				faultsStateMachine = 4;
				faultsTimer        = currentTime;

			// Stato 0: conferma sessione estesa (50 03) → invia ReadDTC
			} else if (faultsStateMachine == 0 &&
					   rx_msg_header.DLC >= 3 &&
					   rx_msg_data[1] == 0x50 && rx_msg_data[2] == 0x03) {
				faultsBodyTxHeader.DLC = 4;
				faultsBodyTxData[0]    = 0x03; // PCI: single frame, 3 byte dati
				faultsBodyTxData[1]    = 0x19; // SID: ReadDTCInformation
				faultsBodyTxData[2]    = 0x02; // subfunction: reportDTCByStatusMask
				faultsBodyTxData[3]    = 0xFF; // statusMask: tutti i DTC attivi
				can_tx(&faultsBodyTxHeader, faultsBodyTxData);
				faultsTimer        = currentTime; // riavvia timeout per la risposta ReadDTC
				faultsStateMachine = 1;

			// Stato 1 + single frame (PCI type 0): parsa DTC direttamente
			} else if (faultsStateMachine == 1 && pci_type == 0) {
				uint8_t payloadLen = pci & 0x0F;
				if (payloadLen >= 3 && rx_msg_header.DLC >= 4 &&
					rx_msg_data[1] == 0x59 && rx_msg_data[2] == 0x02) {
					if (payloadLen > 90) payloadLen = 90;
					for (uint8_t i = 0; i < payloadLen && (i + 1) < rx_msg_header.DLC; i++) {
						faultsRxBuffer[i] = rx_msg_data[1 + i]; // [0]=59 [1]=02 [2]=avail [3..]=DTC
					}
					faultsRxReceived = payloadLen;
					// Parsa: offset 3 = primo DTC, ogni record = 4 byte (3 DTC + 1 status)
					faultsDTCcount = 0;
					uint8_t off    = 3;
					while (off + 4 <= faultsRxReceived && faultsDTCcount < FAULTS_DTC_MAX) {
						faultsDTCbytes[faultsDTCcount][0] = faultsRxBuffer[off];
						faultsDTCbytes[faultsDTCcount][1] = faultsRxBuffer[off + 1];
						faultsDTCbytes[faultsDTCcount][2] = faultsRxBuffer[off + 2];
						faultsDTCcount++;
						off += 4;
					}
					faultsDTCsubmenuIndex = 0;
					faultsStateMachine    = 3;
				}

			// Stato 1 + first frame (PCI type 1): avvia riassemblaggio multiframe
			} else if (faultsStateMachine == 1 && pci_type == 1) {
				uint16_t totalLen = ((uint16_t)(pci & 0x0F) << 8) | rx_msg_data[1];
				if (totalLen > 90) totalLen = 90;
				faultsRxExpected  = totalLen;
				faultsRxReceived  = 0;
				faultsRxNextSN    = 1;
				// Copia i primi 6 byte di payload (data[2..7])
				uint8_t toCopy = (totalLen < 6) ? (uint8_t)totalLen : 6;
				for (uint8_t i = 0; i < toCopy; i++) {
					faultsRxBuffer[i] = rx_msg_data[2 + i];
				}
				faultsRxReceived = toCopy;
				// Flow Control: ContinueToSend, BlockSize=0, STmin=0ms
				faultsBodyTxHeader.DLC = 3;
				faultsBodyTxData[0]    = 0x30;
				faultsBodyTxData[1]    = 0x00;
				faultsBodyTxData[2]    = 0x00;
				can_tx(&faultsBodyTxHeader, faultsBodyTxData);
				faultsStateMachine = 2;

			// Stato 2 + consecutive frame (PCI type 2): accumula e verifica completezza
			} else if (faultsStateMachine == 2 && pci_type == 2) {
				uint8_t sn = pci & 0x0F;
				if (sn == faultsRxNextSN) {
					faultsRxNextSN = (uint8_t)((faultsRxNextSN + 1) & 0x0F);
					for (uint8_t i = 1; i < rx_msg_header.DLC && faultsRxReceived < faultsRxExpected; i++) {
						faultsRxBuffer[faultsRxReceived++] = rx_msg_data[i];
					}
					if (faultsRxReceived >= faultsRxExpected) {
						// Payload completo: parsa DTC (stesso layout del single frame)
						faultsDTCcount = 0;
						uint8_t off    = 3; // skip SID(59) subf(02) availMask
						while (off + 4 <= faultsRxReceived && faultsDTCcount < FAULTS_DTC_MAX) {
							faultsDTCbytes[faultsDTCcount][0] = faultsRxBuffer[off];
							faultsDTCbytes[faultsDTCcount][1] = faultsRxBuffer[off + 1];
							faultsDTCbytes[faultsDTCcount][2] = faultsRxBuffer[off + 2];
							faultsDTCcount++;
							off += 4;
						}
						faultsDTCsubmenuIndex = 0;
						faultsStateMachine    = 3;
					}
				}
			}
		}
		//readFaults 12/08/2026 - END

	#endif //end define

	#if defined(C2baccable)
		if (rx_msg_header.ExtId==0x18DAF128 && DynoStateMachine!=0xff ){ //if message from ABS ECU and Dyno state machine is in progress
			if (DynoStateMachine==0 && rx_msg_header.DLC>=3){ //we received a reply to diagnostic session request msg
				if(rx_msg_data[0]==0x06 && rx_msg_data[1]==0x50 && rx_msg_data[2]==0x03){ //if request was successful
					DynoStateMachine++; //send dyno sts msg
				}
			}
			if (DynoStateMachine==1 && rx_msg_header.DLC>=5){ //we received a reply to dyno status msg
				if(rx_msg_data[0]==0x05 && rx_msg_data[1]==0x62 && rx_msg_data[2]==0x30 && rx_msg_data[3]==0x02){ //if request was successful
					DynoStateMachine++; //send dyno disable
					if(rx_msg_data[4]==0x00){ //if it is disabled, we shall enable it
						DynoModeEnabled=0;//refresh current status
						DynoStateMachine++;//send dyno enable
					}else{ //it is enabled, we shall disable it
						DynoModeEnabled=1;//refresh current status
					}
				}
			}
			if (DynoStateMachine==2 && rx_msg_header.DLC>=4){ //we received a reply to dyno disable msg
				if(rx_msg_data[0]==0x03 && rx_msg_data[1]==0x6E && rx_msg_data[2]==0x30 && rx_msg_data[3]==0x02){ //if request was successful
					DynoModeEnabled=0;//success change complete
					DynoStateMachine=0xff; //disable state machine

					//send message to master to inform about the status of Dyno
					uint8_t tmpArr2[2]={C1BusID,C1cmdDynoNotActive};
					if(DynoModeEnabled) tmpArr2[1]=C1cmdDynoActive;
					addToUARTSendQueue(tmpArr2, 2);

					onboardLed_blue_on();
				}
			}
			if (DynoStateMachine==3 && rx_msg_header.DLC>=4){ //we received a reply to dyno enable msg
				if(rx_msg_data[0]==0x03 && rx_msg_data[1]==0x6E && rx_msg_data[2]==0x30 && rx_msg_data[3]==0x02){ //if request was successful
					DynoModeEnabled=1;//success change complete

					DynoStateMachine=0xff; //disable state machine

					//send message to master to inform about the status of Dyno
					uint8_t tmpArr2[2]={C1BusID,C1cmdDynoNotActive};
					if(DynoModeEnabled) tmpArr2[1]=C1cmdDynoActive;
					addToUARTSendQueue(tmpArr2, 2);

					onboardLed_blue_on();
				}
			}

			if (DynoStateMachine!=0xff && rx_msg_header.DLC>=3){ //in any case
				if( rx_msg_data[1]==0x7F ){ //if request refused, abort all
					DynoStateMachine=0xff; //disable state machine

					//send message to master to inform about the status of Dyno
					uint8_t tmpArr2[2]={C1BusID,C1cmdDynoNotActive};
					if(DynoModeEnabled) tmpArr2[1]=C1cmdDynoActive;
					addToUARTSendQueue(tmpArr2, 2);

					onboardLed_blue_on();

				}
			}
			if(DynoStateMachine!=0xff){ //if we are running, send next message
				DYNO_msg_header.DLC=DYNO_msg_data[DynoStateMachine][0]+1;
				can_tx(&DYNO_msg_header, DYNO_msg_data[DynoStateMachine]); //add to the transmission queue
				onboardLed_blue_on();
				DynoStateMachineLastUpdateTime=currentTime;//save last time it was updated
			}
		}
	#endif


}
