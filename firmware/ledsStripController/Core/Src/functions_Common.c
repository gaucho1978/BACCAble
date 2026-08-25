/*
 * functions_Common.c
 *
 *  Created on: May 2, 2025
 *      Author: GauchoHP
 */

#include "functions_Common.h"
#include "debug.h"

void floatToStr(char* str, float num, uint8_t precision, uint8_t maxLen) {

    uint8_t i = 0;

    // Gestione dei casi speciali NaN e Inf
    if (num != num) {  // NaN check
        memset(&str[0], ' ', maxLen-1); //pad with spaces
        str[maxLen - 1] = '\0';
        return;
    }
    if (num == (float)INFINITY) {
        if (maxLen > 3) {
            str[0] = 'I'; str[1] = 'n'; str[2] = 'f'; str[3] = '\0';
        }
        return;
    }
    if (num == (float)-INFINITY) {
        if (maxLen > 4) {
            str[0] = '-'; str[1] = 'I'; str[2] = 'n'; str[3] = 'f'; str[4] = '\0';
        }
        return;
    }

    // Calcolo del fattore di arrotondamento corretto
    float roundingFactor = 0.5f;
    for (uint8_t j = 0; j < precision; j++) {
        roundingFactor /= 10.0f;
    }
    if (num < 0) {
        num -= roundingFactor;  // Arrotondamento corretto per numeri negativi
    } else {
        num += roundingFactor;  // Arrotondamento per numeri positivi
    }

    // Gestione del segno
    if (num < 0) {
        if (i < maxLen - 1) {
            str[i++] = '-';
        }
        num = -num;
    }

    // Parte intera e parte decimale
    uint32_t intPart = (uint32_t)num;
    uint32_t scale = 1;
    for (uint8_t j = 0; j < precision; j++) {
        scale *= 10;
    }
    uint32_t decPart = (uint32_t)((num - intPart) * scale);

    // Conversione della parte intera
    uint8_t intStart = i;
    if (intPart == 0) {
        if (i < maxLen - 1) {
            str[i++] = '0';
        }
    } else {
        uint8_t count = 0;
        uint32_t tmp = intPart;
        while (tmp > 0) {
            tmp /= 10;
            count++;
        }
        for (uint8_t j = count; j > 0; j--) {
            if (i < maxLen - 1) {
                str[i + j - 1] = (intPart % 10) + '0';
            }
            intPart /= 10;
        }
        i += count;
    }

    // Conversione della parte decimale
    if (precision > 0 && i < maxLen - 1) {
        str[i++] = '.';
        for (uint8_t j = 0; j < precision; j++) {
            if (i < maxLen - 1) {
                decPart *= 10;
                str[i++] = (decPart / scale) + '0';
                decPart %= scale;
            }
        }
    }

    // Rimuovere zeri finali superflui
    if (precision > 0) {
        while (i > intStart && str[i - 1] == '0') {
            str[--i] = '\0';
        }
        if (i > intStart && str[i - 1] == '.') {
            str[--i] = '\0';
        }
    }

    // Aggiungere terminatore di stringa
    if (i < maxLen) {
    	memset(&str[i], ' ', maxLen-i-1); //pad with spaces
        str[maxLen - 1] = '\0'; //if instead we want to close the string without padding, we would just need str[i] = '\0' without memset.
    } else if (maxLen > 0) {
        str[maxLen - 1] = '\0';
    }
}



uint8_t calculateCRC(uint8_t* data, uint8_t arraySize) {
	uint8_t crc = 0xFF;
	if(arraySize>1){
		//calculate sae_j1850 CRC-8 of the array (excluded last element, that will be used to store the final CRC
		for (uint8_t i=0;i<arraySize-1;i++){
			crc ^= data[i];
			for (int i = 0; i < 8; ++i){
				crc = (crc & 0x80) ? (crc << 1) ^ 0x1D : crc << 1;
			}
		}
		return (crc ^ 0xFF); //return calculated checksum
	}
	return 0; //nothing to calculate
}


//System Clock Configuration
void SystemClock_Config(void){
  HAL_Init();
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK){
    Error_Handler(2000);
  }

  // configure CRS to stabilize HSI48
  //__HAL_RCC_CRS_CLK_ENABLE();
  //
  //RCC_CRSInitTypeDef crs = {0};
  //crs.Prescaler = RCC_CRS_SYNC_DIV1;
  //crs.Source = RCC_CRS_SYNC_SOURCE_USB;
  //crs.Polarity = RCC_CRS_SYNC_POLARITY_RISING;
  //crs.ReloadValue = __HAL_RCC_CRS_RELOADVALUE_CALCULATE(48000000, 1000);
  //crs.ErrorLimitValue = RCC_CRS_ERRORLIMIT_DEFAULT;
  //crs.HSI48CalibrationValue = 0x20;
  //HAL_RCCEx_CRSConfig(&crs);



  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI48;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK){
    Error_Handler(1500);
  }

  //the following part is used by usb, used by canable

  // Set USB clock source to HSI48 (48 MHz)
  RCC_PeriphCLKInitTypeDef PeriphClkInit= {0};
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK){
	  Error_Handler(1000);
  }
  __HAL_RCC_GPIOA_CLK_ENABLE();

}


void Error_Handler(uint16_t halfPeriod){
	//onboardLed_red_on();
	//LOGS("System error\r\n");
	//__disable_irq();
	//NVIC_SystemReset();



	RCC->AHBENR |=RCC_AHBENR_GPIOAEN; //ensure clock is enabled on port gpioA


	uint8_t tmpBool01=0;
	while (1){

		//now toggle leds without using HAL, to be more resilient
		if(tmpBool01){
			GPIOA->BSRR= GPIO_PIN_0 <<16; //set PA0 low (red led)

		}else{
			GPIOA->BSRR= GPIO_PIN_0 ; //set PA0 high (red led)
		}

		tmpBool01=!tmpBool01;

		for (volatile uint32_t i = 0; i < (12500*halfPeriod) ; i++){ //12500cycles=1msec
			__asm("nop");
		}
	}
}

// Disable all interrupts
void system_irq_disable(void){
	__disable_irq();
	__DSB();
	__ISB();
}


// Enable all interrupts
void system_irq_enable(void){
        __enable_irq();
}

void system_hex32(char *out, uint32_t val){
	char *p = out + 8;
	*p-- = 0;
	while (p >= out) {
		uint8_t nybble = val & 0x0F;
		if (nybble < 10)
			*p = '0' + nybble;
		else
			*p = 'A' + nybble - 10;
		val >>= 4;
		p--;
	}
}

void saveToFilesystem(void){
	#ifdef ENABLE_USB_MASS_STORAGE

		FIL fil;
		UINT bw;
		FRESULT res;

		res = f_mount(&fs, "", 1);
		if (res == FR_OK){
			res = f_open(&fil, "hello.txt", FA_WRITE|FA_OPEN_ALWAYS);
			if (res == FR_OK) {
				f_write(&fil, "Hello, World!\r\n", 15, &bw);
				f_sync(&fil);
				f_close(&fil);
				onboardLed_blue_on();
			}


		}
		f_unmount("");

	#endif
}


void storage_init(void){

	#ifdef ENABLE_USB_MASS_STORAGE
		//FATFS fs;
		FIL fil;
		UINT bw;
		FRESULT res;
		BYTE work[FF_MIN_SS];

		res = f_mount(&fs, "", 1);
		if (res != FR_OK){
			//onboardLed_blue_on();
			MKFS_PARM opt = {.fmt = FM_FAT | FM_SFD, .n_fat = 1, .align = 0, .n_root = 32, .au_size = FF_MIN_SS};
			res = f_mkfs("", &opt, work, FF_MIN_SS);
			if (res == FR_OK){
				res = f_setlabel("BACCABLE "
					#ifdef ACT_AS_CANABLE
						"Sniffer"
					#elif defined(C1baccable)
						"C1"
					#elif defined(C2baccable)
						"C2"
					#elif defined(BHbaccable)
						"BH"
					#endif
				);
				res = f_open(&fil, "Version.txt", FA_CREATE_ALWAYS | FA_WRITE);

				if (res == FR_OK){
					onboardLed_blue_on();

					f_write(&fil, _FW_VERSION, strlen(_FW_VERSION), &bw);
					f_close(&fil);
				}else{
					onboardLed_red_on();
				}
			}

		}

		res = f_unmount("");

	#endif
}

//sniffer function 24/08/2026 - BEGIN
#if defined(C1baccable) || defined(C2baccable) || defined(BHbaccable)
//Writes one 16 byte frame into the ring buffer. Called from the can rx path, must stay short.
//SNIFFER_BUFFER_SIZE is a multiple of SNIFFER_FRAME_SIZE and snifferRingHead only ever advances
//by whole frames, so it stays frame aligned: a single frame never wraps past the end of the
//array, and the wrap mask only needs to be applied once, after the whole frame has been written.
void snifferPushFrame(CAN_RxHeaderTypeDef *snifferRxHeader, uint8_t *snifferRxData){
	uint16_t snifferBase;
	uint32_t snifferTimeStamp;
	uint32_t snifferCanId;
	uint8_t  snifferDlc;
	uint8_t  snifferByteIndex;

	//as soon as there is room for the marker plus one more frame, report the frames lost during the overflow
	if(snifferDroppedFrames>0 && snifferRingCount<=(SNIFFER_BUFFER_SIZE-(2*SNIFFER_FRAME_SIZE))){
		snifferBase=snifferRingHead;
		snifferTimeStamp=currentTime;
		snifferRingBuffer[snifferBase+0]=SNIFFER_OVERFLOW_MARKER;
		snifferRingBuffer[snifferBase+1]=(uint8_t)(snifferTimeStamp);
		snifferRingBuffer[snifferBase+2]=(uint8_t)(snifferTimeStamp>>8);
		snifferRingBuffer[snifferBase+3]=(uint8_t)(snifferTimeStamp>>16);
		snifferRingBuffer[snifferBase+4]=(uint8_t)(snifferDroppedFrames);
		snifferRingBuffer[snifferBase+5]=(uint8_t)(snifferDroppedFrames>>8);
		snifferRingBuffer[snifferBase+6]=0;
		snifferRingBuffer[snifferBase+7]=0;
		for(snifferByteIndex=8; snifferByteIndex<SNIFFER_FRAME_SIZE; snifferByteIndex++){
			snifferRingBuffer[snifferBase+snifferByteIndex]=0;
		}
		snifferRingHead=(snifferBase+SNIFFER_FRAME_SIZE)&SNIFFER_BUFFER_MASK;
		snifferRingCount+=SNIFFER_FRAME_SIZE;
		snifferDroppedFrames=0;
	}

	//no room left for a whole frame: count it as lost and give up, the marker will report it later
	if(snifferRingCount>(SNIFFER_BUFFER_SIZE-SNIFFER_FRAME_SIZE)){
		if(snifferDroppedFrames<0xFFFF) snifferDroppedFrames++;
		return;
	}

	snifferDlc=snifferRxHeader->DLC;
	if(snifferDlc>8) snifferDlc=8;
	if(snifferRxHeader->IDE==CAN_ID_EXT){
		snifferCanId=snifferRxHeader->ExtId;
	}else{
		snifferCanId=snifferRxHeader->StdId;
	}
	snifferTimeStamp=currentTime;
	snifferBase=snifferRingHead;

	snifferRingBuffer[snifferBase+0]=SNIFFER_START_NIBBLE|snifferDlc;
	snifferRingBuffer[snifferBase+1]=(uint8_t)(snifferTimeStamp);
	snifferRingBuffer[snifferBase+2]=(uint8_t)(snifferTimeStamp>>8);
	snifferRingBuffer[snifferBase+3]=(uint8_t)(snifferTimeStamp>>16);
	snifferRingBuffer[snifferBase+4]=(uint8_t)(snifferCanId);
	snifferRingBuffer[snifferBase+5]=(uint8_t)(snifferCanId>>8);
	snifferRingBuffer[snifferBase+6]=(uint8_t)(snifferCanId>>16);
	snifferRingBuffer[snifferBase+7]=(uint8_t)(snifferCanId>>24);
	for(snifferByteIndex=0; snifferByteIndex<8; snifferByteIndex++){
		snifferRingBuffer[snifferBase+8+snifferByteIndex]=(snifferByteIndex<snifferDlc)?snifferRxData[snifferByteIndex]:0; //zero padding above DLC
	}
	snifferRingHead=(snifferBase+SNIFFER_FRAME_SIZE)&SNIFFER_BUFFER_MASK;
	snifferRingCount+=SNIFFER_FRAME_SIZE;
}

//Moves buffered bytes to usb. Never waits on the usb: if the endpoint is busy we retry on the next loop.
void snifferFlush(void){
	uint16_t snifferSendLength;
	uint16_t snifferContiguousBytes;

	if(snifferUsbInited==0) return;
	if(snifferRingCount==0) return;
	if(hUsbDeviceFS.pClassData==NULL) return; //usb not enumerated yet
	if(((USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData)->TxState) return; //endpoint busy: do not enter the busy wait inside CDC_Transmit_FS

	snifferSendLength=snifferRingCount;
	//a partial buffer is sent only after a while, so that on a busy bus we always send full 64 byte packets
	if(snifferSendLength<SNIFFER_USB_CHUNK && (currentTime-snifferLastFlushTime)<SNIFFER_FLUSH_TIMEOUT_MS) return;
	if(snifferSendLength>SNIFFER_USB_CHUNK) snifferSendLength=SNIFFER_USB_CHUNK;
	//never read across the end of the ring: the remaining part goes out on the next flush
	snifferContiguousBytes=SNIFFER_BUFFER_SIZE-snifferRingTail;
	if(snifferSendLength>snifferContiguousBytes) snifferSendLength=snifferContiguousBytes;

	if(CDC_Transmit_FS(&snifferRingBuffer[snifferRingTail], snifferSendLength)==USBD_OK){
		snifferRingTail=(snifferRingTail+snifferSendLength)&SNIFFER_BUFFER_MASK;
		snifferRingCount-=snifferSendLength;
		snifferLastFlushTime=currentTime;
	}
}

//Starts the function. Only ram is touched here, so this is safe to call from the uart interrupt as well:
//the usb bring up blocks for a few milliseconds and is deferred to snifferUsbStartIfRequested().
void snifferStart(void){
	snifferRingHead=0;
	snifferRingTail=0;
	snifferRingCount=0;
	snifferDroppedFrames=0;
	snifferLastFlushTime=currentTime;
	snifferUsbStartRequested=1;
	snifferFunctionEnabled=1;
}

void snifferStop(void){
	snifferFunctionEnabled=0;
	//usb is left up as cdc on purpose: going back to mass storage would need another re-enumeration

	//usbConnectedToSlave is never cleared anywhere else once set (pre-existing behaviour, not introduced by the
	//sniffer), so on C1 it would otherwise keep blocking low consume (lowConsume.c) for the rest of the power
	//cycle even after the recording is over. Clearing it here is safe: the sniffer enable command is always
	//broadcast to both C2 and BH together (C2_Bh_BusID), so this point is only ever reached after both of them
	//have already left mass storage mode for good - there is no longer any genuine pen drive session left to
	//protect on either of them. On C2/BH this write is inert: nothing reads usbConnectedToSlave there any more.
	usbConnectedToSlave=0;
}

//Brings the usb up as cdc. Called from the main loop only: MX_USB_DEVICE_Init() waits on HAL_Delay().
void snifferUsbStartIfRequested(void){
	if(snifferUsbStartRequested==0) return;
	snifferUsbStartRequested=0;
	if(snifferUsbInited) return; //already running as cdc, nothing to do until the next power cycle. this is a one shot bring up: no retry if the host never configures us.

	//Any usb stack already running must be torn down before we come back as cdc. This is driven by the actual
	//state of the stack, not by the build flavor: besides the pen drive on C2/BH, C1 too can already have the usb
	//up at boot (main.c does that whenever ACT_AS_CANABLE, DEBUG_MODE, ENABLE_USB_MASS_STORAGE or
	//ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER is defined). Without this teardown MX_USB_DEVICE_Init() would
	//re-initialize on top of a live stack, and HAL_PCD_Init() would skip HAL_PCD_MspInit() because the pcd state
	//is not HAL_PCD_STATE_RESET, leaving the peripheral half re-initialized and the host with no clean re-attach.
	//pClass is NULL only when MX_USB_DEVICE_Init() was never called at all, and then there is nothing to tear down.
	if(hUsbDeviceFS.pClass!=NULL){
		//The teardown below (USBD_DeInit / USBD_LL_Stop+DeInit) resets and powers down the usb core, but never
		//clears BCDR_DPPU: the D+ pull-up stays asserted the whole time. From the host's point of view the device
		//never actually detaches, it just goes quiet and comes back later announcing a completely different class
		//(pen drive -> cdc) without ever having been unplugged. Most usb stacks, Android included, do not reliably
		//re-enumerate a class change that was never preceded by a real disconnect. So the pull-up is explicitly
		//released here and held low long enough for the host to notice the removal, before anything else runs.
		if(hUsbDeviceFS.pData!=NULL){
			HAL_PCD_DevDisconnect((PCD_HandleTypeDef*)hUsbDeviceFS.pData);
			HAL_Delay(250);
		}

		//pClassData is allocated by the class Init(), which only runs when the host sends SET_CONFIGURATION: with no
		//cable attached (or a cable that never enumerated us) it is still NULL. USBD_DeInit() would then call
		//pClass->DeInit() -> MSC_BOT_DeInit(), which dereferences pClassData without any NULL check and writes to it,
		//hitting address 0x00000008 (flash alias, not writable) and hard faulting. So the class teardown is done only
		//when the class was actually initialized, while the low level driver is torn down either way.
		//note: USBD_DeInit() already performs USBD_LL_Stop()+USBD_LL_DeInit() internally, so it is not called twice here.
		if(hUsbDeviceFS.pClassData!=NULL){
		USBD_DeInit(&hUsbDeviceFS);
		}else{
			USBD_LL_Stop(&hUsbDeviceFS);
			USBD_LL_DeInit(&hUsbDeviceFS);
		}
	}
	usbdDescSelectCdcMode(); //descriptors must answer as cdc before the host re-enumerates us
	MX_USB_DEVICE_Init();
	snifferUsbInited=1;
}
#endif
//sniffer function 24/08/2026 - END
