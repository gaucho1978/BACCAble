#include "usbd_cdc_if.h"
/* Create buffer for reception and transmission           */
/* It's up to user to redefine and/or remove those define */
/** Received data over USB are stored in this buffer      */
//uint8_t UserRxBufferFS[APP_RX_DATA_SIZE];
static  usbrx_buf_t UserRxBufferFS = {0};

/** Data to send over USB CDC are stored in this buffer   */
uint8_t UserTxBufferFS[TX_BUF_SIZE];
static uint8_t slcan_str[SLCAN_MTU];
static uint8_t slcan_str_index = 0;

extern USBD_HandleTypeDef hUsbDeviceFS;
static int8_t CDC_Init_FS(void);
static int8_t CDC_DeInit_FS(void);
static int8_t CDC_Control_FS(uint8_t cmd, uint8_t* pbuf, uint16_t length);
static int8_t CDC_Receive_FS(uint8_t* pbuf, uint32_t *Len);

//extern uint8_t dashboardPageStringArray[19];
//uint8_t zeroArray[18]={0};
//extern uint8_t requestToSendOneFrame; //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality //set to 1 to send one frame on dashboard

// CDC Interface
USBD_CDC_ItfTypeDef USBD_Interface_fops_FS =
{
  CDC_Init_FS,
  CDC_DeInit_FS,
  CDC_Control_FS,
  CDC_Receive_FS
};

/**
  * @brief  Initializes the CDC media low layer over the FS USB IP
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Init_FS(void)
{
  USBD_CDC_SetTxBuffer(&hUsbDeviceFS, UserTxBufferFS, 0);
  USBD_CDC_SetRxBuffer(&hUsbDeviceFS, UserRxBufferFS.buf[UserRxBufferFS.head]);
  return (USBD_OK);
}

static int8_t CDC_DeInit_FS(void)
{
  return (USBD_OK);
}

/**
  * @brief  Manage the CDC class requests
  * @param  cmd: Command code
  * @param  pbuf: Buffer containing command data (request parameters)
  * @param  length: Number of data to be sent (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Control_FS(uint8_t cmd, uint8_t* pbuf, uint16_t length)
{
  /* USER CODE BEGIN 5 */
  switch(cmd)
  {
    case CDC_SEND_ENCAPSULATED_COMMAND:
    break;

    case CDC_GET_ENCAPSULATED_RESPONSE:
    break;

    case CDC_SET_COMM_FEATURE:
    break;

    case CDC_GET_COMM_FEATURE:
    break;

    case CDC_CLEAR_COMM_FEATURE:
    break;

  /*******************************************************************************/
  /* Line Coding Structure                                                       */
  /*-----------------------------------------------------------------------------*/
  /* Offset | Field       | Size | Value  | Description                          */
  /* 0      | dwDTERate   |   4  | Number |Data terminal rate, in bits per second*/
  /* 4      | bCharFormat |   1  | Number | Stop bits                            */
  /*                                        0 - 1 Stop bit                       */
  /*                                        1 - 1.5 Stop bits                    */
  /*                                        2 - 2 Stop bits                      */
  /* 5      | bParityType |  1   | Number | Parity                               */
  /*                                        0 - None                             */
  /*                                        1 - Odd                              */
  /*                                        2 - Even                             */
  /*                                        3 - Mark                             */
  /*                                        4 - Space                            */
  /* 6      | bDataBits  |   1   | Number Data bits (5, 6, 7, 8 or 16).          */
  /*******************************************************************************/
    case CDC_SET_LINE_CODING:
    break;

    case CDC_GET_LINE_CODING:
	pbuf[0] = (uint8_t)(115200);
	pbuf[1] = (uint8_t)(115200 >> 8);
	pbuf[2] = (uint8_t)(115200 >> 16);
	pbuf[3] = (uint8_t)(115200 >> 24);
	pbuf[4] = 0; // stop bits (1)
	pbuf[5] = 0; // parity (none)
	pbuf[6] = 8; // number of bits (8)
    break;

    case CDC_SET_CONTROL_LINE_STATE:
    break;

    case CDC_SEND_BREAK:
    break;

  default:
    break;
  }

  return (USBD_OK);
}

/**
  * @brief  Data received over USB OUT endpoint are sent over CDC interface
  *         through this function.
  *
  *         @note
  *         This function will issue a NAK packet on any OUT packet received on
  *         USB endpoint until exiting this function. If you exit this function
  *         before transfer is complete on CDC interface (ie. using DMA controller)
  *         it will result in receiving more data while previous ones are still
  *         not sent.
  *
  * @param  Buf: Buffer of data to be received
  * @param  Len: Number of data received (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Receive_FS(uint8_t* Buf, uint32_t *Len)
{
  // The ide automatically created the following commented code:
  //USBD_CDC_SetRxBuffer(&hUsbDeviceFS, &Buf[0]);
  //USBD_CDC_ReceivePacket(&hUsbDeviceFS);
  //return (USBD_OK);

	// Check for overflow!
	// If when we increment the head we're going to hit the tail
	// (if we're filling the last spot in the queue)
	// FIXME: Use a "full" variable instead of wasting one
	// spot in the cirbuf as we are doing now
	if( ((UserRxBufferFS.head + 1) % NUM_RX_BUFS) == UserRxBufferFS.tail)
	{
		error_assert(ERR_FULLBUF_USBRX);

		// Listen again on the same buffer. Old data will be overwritten.
	    USBD_CDC_SetRxBuffer(&hUsbDeviceFS, UserRxBufferFS.buf[UserRxBufferFS.head]);
	    USBD_CDC_ReceivePacket(&hUsbDeviceFS);
		return HAL_ERROR;
	}
	else
	{
		// Save off length
		UserRxBufferFS.msglen[UserRxBufferFS.head] = *Len;
		UserRxBufferFS.head = (UserRxBufferFS.head + 1) % NUM_RX_BUFS;

		// Start listening on next buffer. Previous buffer will be processed in main loop.
	    USBD_CDC_SetRxBuffer(&hUsbDeviceFS, UserRxBufferFS.buf[UserRxBufferFS.head]);
	    USBD_CDC_ReceivePacket(&hUsbDeviceFS);
	    return (USBD_OK);
	}
}

// Process incoming USB-CDC messages from RX FIFO
void cdc_process(void){
	system_irq_disable();
	//onboardLed_blue_on();
	if(UserRxBufferFS.tail != UserRxBufferFS.head){
		//  Process one whole buffer
		onboardLed_red_on();
		for (uint32_t i = 0; i < UserRxBufferFS.msglen[UserRxBufferFS.tail]; i++){

			if (UserRxBufferFS.buf[UserRxBufferFS.tail][i] == '\r'){
				#if defined(ACT_AS_CANABLE)
					int8_t result = slcan_parse_str(slcan_str, slcan_str_index);
					UNUSED(result); //avoid the warning of unused variable
				#endif
				//#if defined(SHOW_PARAMS_ON_DASHBOARD)
				//	onboardLed_blue_on();
				//	// Elaboriamo la stringa ricevuta dal Baccable Master, contenente la stringa della pagina da visualizzare
				//	if(slcan_str_index<=18){ //se l'array ha massimo 18 elementi (dashboardPageStringArray ha 18 caratteri + carattere di chiusura /r)
				//		//copia l'array nella destinazione
				//		memcpy(&dashboardPageStringArray[0], &slcan_str[0], slcan_str_index);
				//		//finisci di riempire di zeri
				//		memcpy(&dashboardPageStringArray[slcan_str_index], &zeroArray, 18-slcan_str_index);
				//		if (requestToSendOneFrame<=2) requestToSendOneFrame +=1;//Send one frame
				//
				//		//this is how to clear the screen (I save it here just to remember how to do it):
				//		//telematic_display_info_msg_data[0]= 0;
				//		//telematic_display_info_msg_data[1]=0x11;
				//		//telematic_display_info_msg_data[2]=0;
				//		//telematic_display_info_msg_data[3]= 0x20;
				//		//telematic_display_info_msg_data[4]=0;
				//		//telematic_display_info_msg_data[5]=0;
				//		//telematic_display_info_msg_data[6]=0;
				//		//telematic_display_info_msg_data[7]=0;
				//		//can_tx(&telematic_display_info_msg_header, telematic_display_info_msg_data); //transmit the packet
				//	}
				//#endif

				// Success
				//if(result == 0)
				//    CDC_Transmit_FS("\n", 1);
				// Failure
				//else
				//    CDC_Transmit_FS("\a", 1);

				slcan_str_index = 0;
			}else{
				// Check for overflow of buffer
				if(slcan_str_index >= SLCAN_MTU){
					// TODO: Return here and discard this CDC buffer?
					slcan_str_index = 0;
				}
				slcan_str[slcan_str_index++] = UserRxBufferFS.buf[UserRxBufferFS.tail][i];
			}
		}

		// Move on to next buffer
		UserRxBufferFS.tail = (UserRxBufferFS.tail + 1) % NUM_RX_BUFS;
	}
	system_irq_enable();
}


/**
  * @brief  CDC_Transmit_FS
  *         Data to send over USB IN endpoint are sent over CDC interface
  *         through this function.
  *         @note
  *
  *
  * @param  Buf: Buffer of data to be sent
  * @param  Len: Number of data to be sent (in bytes)
  * @retval USBD_OK if all operations are OK else USBD_FAIL or USBD_BUSY
  */

// TODO: Do some buffering here. Try to transmit 64byte packets.
uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len)
{
  // The ide automatically created the following commented code:
  // uint8_t result = USBD_OK;
  // USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData;
  // if (hcdc->TxState != 0){
  //   return USBD_BUSY;
  // }
  // USBD_CDC_SetTxBuffer(&hUsbDeviceFS, Buf, Len);
  // result = USBD_CDC_TransmitPacket(&hUsbDeviceFS);
  // return result;

  // Attempt to transmit on USB, wait until not busy
  // Future: implement TX buffering
    uint32_t start_wait = HAL_GetTick();
    while( ((USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData)->TxState){
      // If no TX within timeout, abort.
      if(HAL_GetTick() - start_wait >= 10){
          error_assert(ERR_USBTX_BUSY);
          return USBD_BUSY;
      }
    }
    // Ensure message will fit in buffer
    if(Len > TX_BUF_SIZE){
    	return 0;
    }
    // Copy data into buffer
    for (uint32_t i=0; i < Len; i++){
    	UserTxBufferFS[i] = Buf[i];
    }
    // Set transmit buffer and start TX
    USBD_CDC_SetTxBuffer(&hUsbDeviceFS, UserTxBufferFS, Len);
    return USBD_CDC_TransmitPacket(&hUsbDeviceFS);
}

#ifdef DEBUG_MODE
uint8_t print_to_usb_(char* message)
{
    uint16_t msg_len = strlen(message);
    return CDC_Transmit_FS((uint8_t*)message, msg_len < SLCAN_MTU ? msg_len : SLCAN_MTU);
}

uint8_t printf_to_usb_(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int written = vsnprintf_((char*)slcan_str, SLCAN_MTU, format, args);
    va_end(args);
    return CDC_Transmit_FS(slcan_str, written < SLCAN_MTU ? (uint8_t) written : SLCAN_MTU);
  }
#endif
