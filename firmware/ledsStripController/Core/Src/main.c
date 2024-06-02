#include "main.h"

// the definition of ACT_AS_CANABLE shall be placed in main.h
#if defined(ACT_AS_CANABLE)
	#include "usb_device.h"
	#include "usbd_cdc_if.h"
	#include "string.h"
#endif /* ACT_AS_CANABLE */


char *FW_VERSION="BACCA CANable V.1.0";  //this is used to store FW version, also shown on usb when used as slcan
float scaledVolume;
uint8_t scaledColorSet;

uint32_t debugTimer0;

int main(void){

	SystemClock_Config(); //set system clocks
	onboardLed_init(); //initialize onboard leds for debug purposes
	HAL_Delay(100);
	can_init(); //initialize can interface
	onboardLed_red_on();
	#if defined(ACT_AS_CANABLE)
		MX_USB_DEVICE_Init();
	#else
		vuMeterInit(); //initialize leds strip controller
		can_set_bitrate(CAN_BITRATE_500K);//set can speed to 500kpbs
		can_enable(); //enable can port
	#endif /* ACT_AS_CANABLE */

	onboardLed_blue_blink(2);

	#if defined(DISABLE_START_STOP)
		// It is assumed that there is a delay introduced by the previous activities (led intro),
		// otherwise the car ECU could not be ready to accept commands
		// We will now short gpio to ground for 500msec in order to disable start&stop car functionality
		// I use swclk pin, pin37, PA14
		HAL_GPIO_WritePin(START_STOP_DISABLER, 0);
		HAL_Delay(500);
		HAL_GPIO_WritePin(START_STOP_DISABLER, 1);
	#endif
	// Storage for status and received message buffer
	CAN_RxHeaderTypeDef rx_msg_header;
	uint8_t rx_msg_data[8] = {0};
	uint8_t msg_buf[SLCAN_MTU];

	while (1){
		debugTimer0=HAL_GetTick();

		onboardLed_process();
		//onboardLed_red_on();
		can_process();
		#if defined(ACT_AS_CANABLE)
			cdc_process(); //processa dati usb
			//just for test
			//char *data = "Hello World from USB CDC\n";
			//CDC_Transmit_FS((uint8_t*)data, strlen(data));
			//HAL_Delay (1000);

		#else
		#endif

		//just for debug purposes, we can periodically send a packet to can bus
		//we can set the can bus in loopback mode, to receive back each sent message.
		// To set loopback mode, in function can_enable (can.c) we shall set can_handle.Init.Mode = CAN_MODE_LOOPBACK
		// This way we will receive whatever we send
		// Before to send can messages, speed cshall be set and can port shall be enabled,
		// if you defined ACT_AS_CANABLE, use the following commands (not required if
		// ACT_AS_CANABLE is not defined, since this is done in this function, at the beginning
		//can_set_bitrate(CAN_BITRATE_500K);//set can speed to 500kpbs
		//can_enable(); //enable can port
		//Then we can prepare and send the following test packet
		//CAN_TxHeaderTypeDef testMsgHeader;
		//testMsgHeader.IDE= CAN_ID_STD;
		//testMsgHeader.RTR = CAN_RTR_DATA;
		//testMsgHeader.StdId=0x0412;
		//testMsgHeader.DLC=5;
		//uint8_t testMsgData[8] = {0};
		//testMsgData[0]=0x01;
		//testMsgData[1]=0x01;
		//testMsgData[2]=0x01;
		//testMsgData[3]=0x01;
		//testMsgData[4]=0xE6; //pedal position
		//can_tx(&testMsgHeader, testMsgData);

		// If CAN message receive is pending, process the message
		if(is_can_msg_pending(CAN_RX_FIFO0)){
			// If message received from bus, parse the frame
			if (can_rx(&rx_msg_header, rx_msg_data) == HAL_OK){
				#if defined(ACT_AS_CANABLE)
					uint16_t msg_len = slcan_parse_frame((uint8_t *)&msg_buf, &rx_msg_header, rx_msg_data);
					if(msg_len){
						CDC_Transmit_FS(msg_buf, msg_len); //trasmetti dati via usb
					}
				#else
					UNUSED(msg_buf); //avoid warning when used as leds strip controller
					//se e' il messaggio che contiene la pressione dell'acceleratore (id 412), se é lungo 5 byte, se il valore é >51 (sfrutto le info ottenute sniffando)
					if ((rx_msg_header.StdId==0x00000412) && (rx_msg_header.DLC==5) && (rx_msg_data[3]>=51)){
						scaledVolume=scaleVolume(rx_msg_data[3]); //prendi il dato e scalalo, per prepararlo per l'invio alla classe vumeter
					}
					//se e' il messaggio che contiene la marcia (id 2ef) e se é lungo 8 byte
					if ((rx_msg_header.StdId==0x000002EF) && (rx_msg_header.DLC==8)){
						scaledColorSet=scaleColorSet(rx_msg_data[0]); //prendi il dato e scalalo, per prepararlo per l'invio alla classe vumeter
						//onboardLed_red_on();
					}
					vuMeterUpdate(scaledVolume,scaledColorSet);
					//vuMeterUpdate(scaledVolume,3);
				#endif /* ACT_AS_CANABLE */
			}
		}
		//for debug, measure the loop duration
		if (HAL_GetTick()>debugTimer0+2){
				//onboardLed_red_on();
		}
	}

}

// this function scales value received from can bus. It is assumed that pedal position (or motor rpm) will change vumeter volume represented with the leds strip
float scaleVolume(uint8_t vol){
	//Scale this value to get a percentage between 0 and 100

	//contenuto del dato sniffato su can bus: id 412 , quarto byte, va da 33(51) va a E6 (230)

	if (vol>50){
		vol=vol-51;
	}
	return (float)(((float)vol*100.0f)/180.0f);
}

// this function scales value received from can bus. It is assumed that gear selection will change the color effect of the leds strip
uint8_t scaleColorSet(uint8_t col){
	//id 2ef, primo byte, 70=r, 00=n, f0=marcia inserita ma frizione premuta (indefinito), 10=prima, 20=seconda..

	col= col>>4;
	//onboardLed_red_blink(col);
	// 7=backward, f=gear set but frizione premuta (undefined), 1=first gear , 2=second gear, ... , 6= sixt gear
	return col;
	/*
	//output: 0=folle, 1-6=marce, 9=retromarcia
	switch (col){
		case 0x70:
			return 9; //retromarcia
			break;
		case 0x10:
			return 1; //prima
			break;
		case 0x20:
			return 2; //seconda
			break;
		case 0x30:
			return 3; //terza
			break;
		case 0x40:
			return 4; //quarta
			break;
		case 0x50:
			return 5; //quinta
			break;
		case 0x60:
			return 6; //sesta
			break;
		default:
			return 0; //folle
			break;
	}
	*/
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
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI48;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK){
    Error_Handler();
  }

  //the following part is used by usb, used by canable

  // Set USB clock source to HSI48 (48 MHz)
  RCC_PeriphCLKInitTypeDef PeriphClkInit= {0};
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;



if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK){
    Error_Handler();
  }

//  HAL_RCC_OscConfig(&RCC_OscInitStruct);
//  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1);
//  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);
//  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);
//  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
//
//  // Enable clock recovery system for internal oscillator
//  RCC_CRSInitTypeDef RCC_CRSInitStruct;
//  __HAL_RCC_CRS_CLK_ENABLE();
//
//  // Default Synchro Signal division factor (not divided)
//  RCC_CRSInitStruct.Prescaler = RCC_CRS_SYNC_DIV1;
//
//  // Set the SYNCSRC[1:0] bits according to CRS_Source value
//  RCC_CRSInitStruct.Source = RCC_CRS_SYNC_SOURCE_USB;
//
//  // Rising polarity
//  RCC_CRSInitStruct.Polarity = RCC_CRS_SYNC_POLARITY_RISING;
//
//  // HSI48 is synchronized with USB SOF at 1KHz rate
//  RCC_CRSInitStruct.ReloadValue = __HAL_RCC_CRS_RELOADVALUE_CALCULATE(48000000, 1000);
//  RCC_CRSInitStruct.ErrorLimitValue = RCC_CRS_ERRORLIMIT_DEFAULT;
//
//  // Set the TRIM[5:0] to the default value
//  RCC_CRSInitStruct.HSI48CalibrationValue = 32;
//
//  // Start automatic synchronization
//  HAL_RCCEx_CRSConfig(&RCC_CRSInitStruct);
//
//  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
//  __HAL_RCC_GPIOF_CLK_ENABLE();
//  __HAL_RCC_GPIOB_CLK_ENABLE();

	__HAL_RCC_GPIOA_CLK_ENABLE();

}

void Error_Handler(void){
  __disable_irq();
  while (1){}
}

// Disable all interrupts
void system_irq_disable(void)
{
        __disable_irq();
        __DSB();
        __ISB();
}


// Enable all interrupts
void system_irq_enable(void)
{
        __enable_irq();
}

void system_hex32(char *out, uint32_t val)
{
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

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
