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
uint32_t timeSinceLastReceivedCanMessage=0;
uint32_t timeSinceLastPanicAlarmActivation=0;
uint8_t ledsStripIsOn=0; //indicates if leds strip is on
uint8_t alarmActivated=0; //indicates if the panic alarm was activated during last 10 minutes

//the following 2 arrays declares: RFHUB reset (first message) and panic alarm messages definition (the others)
CAN_TxHeaderTypeDef panicAlarmStartMsgHeader[5]={ {.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DAC7F1, .DLC=3},{.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x1E340041, .DLC=4},{.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x1EF, .DLC=8},{.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x2EC, .DLC=8},{.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x1E340041, .DLC=4},};
uint8_t panicAlarmStartMsgData[5][8]={{0x02,0x11,0x01,0x00,},{0x88,0x20,0x15,0x00,},{0x42,0x02,0xE2,0x00,0x00,0x00,0x01,0x56},{0x00,},{0x88,0x20,0x15,0x00,}};

uint8_t startAndStopEnabled=1;
uint32_t lastTimeStartAndstopDisablerButtonPressed=0;

// Storage for status and received message buffer
CAN_RxHeaderTypeDef rx_msg_header;  //msg header
uint8_t rx_msg_data[8] = {0};  //msg data
uint8_t msg_buf[SLCAN_MTU]; //msg converted in ascii to send over usb

//the following array stores buttons pressed password sequence
//these are possible values 0x90=RES,
//							0x12=Cruise control on/off,
//                          0x08=Cruise control speed gently up,
//                          0x00=Cruise control speed strong up,
//                          0x18=Cruise control speed gently down,
//                          0x20=Cruise control speed strong down
// WARNING: when you press cruise control strong up, before and after it, also cruise control gently up
//          message is fired, therefore your sequence will be altered. A workaround
//          is to use both messages in the sequence, or to use just the gently up/down message.

//uint32_t buttonPressedTimeArray[20] = {0};  //0=RES, 1=Cruise control on/off, 2=Cruise control speed gently up, 3=Cruise control speed strong up, 4=Cruise control speed gently down, 5=Cruise control speed strong down
uint8_t ButtonPressSequence1Index=0;
uint8_t ButtonPressSequence1Len=5;
uint8_t ButtonPressSequence1[5]={0x90,0x08,0x00,0x08,0x90}; //current password: RES - CC gently up  - CC strong up - CC gently up - RES

//future growth
uint8_t ButtonPressSequence2Index=0;
uint8_t ButtonPressSequence2Len=8;
uint8_t ButtonPressSequence2[8]={0x90,0x08,0x00,0x08,0x18,0x20,0x18,0x12}; //current password: RES - CC gently up - CC strong up - CC gently up - CC gently down - CC strong down - CC gently down - CC on/off

uint8_t  lastPressedWheelButton=0xff; //default value, means no button pressed on the wheel
uint32_t lastPressedWheelButtonTime=0;//stores the last time a wheel button was pressed, in msec from boot
uint32_t lastPressedWheelButtonDuration=0x00; //default value

int main(void){

	SystemClock_Config(); //set system clocks
	onboardLed_init(); //initialize onboard leds for debug purposes
	//HAL_Delay(10); //it was 100
	can_init(); //initialize can interface
	onboardLed_red_on();
	//onboardLed_blue_blink(2);


	#if defined(ACT_AS_CANABLE)
		MX_USB_DEVICE_Init();
	#endif

	#if (!defined(ACT_AS_CANABLE)) || defined(IMMOBILIZER_ENABLED)  //if we require the automatic reading of the can bus (to ctrl leds or to make the immobilizer)...
		//let's open the can bus because we may need data
		can_set_bitrate(CAN_BITRATE_500K);//set can speed to 500kpbs
		can_enable(); //enable can port
	#endif

	while (1){
		debugTimer0=HAL_GetTick();
		onboardLed_process();
		can_process();
		#if defined(ACT_AS_CANABLE)
			cdc_process(); //processa dati usb
			//just for test
			//char *data = "Hello World from USB CDC\n";
			//CDC_Transmit_FS((uint8_t*)data, strlen(data));
			//HAL_Delay (1000);

			//just for test, we can periodically send a packet to can bus
			//we can set the can bus in loopback mode, to receive back each sent message.
			// To set loopback mode, in function can_enable (can.c) we shall set can_handle.Init.Mode = CAN_MODE_LOOPBACK
			// This way we will receive whatever we send
			// Before to send can messages, speed shall be set and can port shall be enabled,
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
		#else
			//don't act as canable. One USB port pin is used to control leds.
			vuMeterInit(); //initialize leds strip controller - this is called many times to divide the operations on more loops

			if((timeSinceLastReceivedCanMessage+10000<HAL_GetTick()) && (ledsStripIsOn) ){ //if no can interesting message for 10 seconds, and the strip is on, shutdown the leds to save energy
				shutdownLedsStrip();
				ledsStripIsOn=0; //entriamo solo una volta
			}

		#endif

		#if defined(IMMOBILIZER_ENABLED)
			//the following if is used only by IMMOBILIZER functionality
			if((timeSinceLastPanicAlarmActivation+600000<HAL_GetTick()) && (alarmActivated)){ //after 10 minutes, alarm return to not activated (cause for sure it is off)
				alarmActivated=0;
			}
		#endif

		#if defined(DISABLE_START_STOP)
			if(startAndStopEnabled && (HAL_GetTick()>6000)){
				if(lastTimeStartAndstopDisablerButtonPressed==0){ //first time we arrive here, go inside
					// We will now short gpio to ground in order to disable start&stop car functionality. This will simulate car start&stop button press
					HAL_GPIO_WritePin(START_STOP_DISABLER, 0); // I use swclk pin, pin37, PA14
					lastTimeStartAndstopDisablerButtonPressed=HAL_GetTick();
					onboardLed_red_on();
				}

				if(lastTimeStartAndstopDisablerButtonPressed+500<HAL_GetTick()){ //if pressed since 500msec
					HAL_GPIO_WritePin(START_STOP_DISABLER, 1); //return to 1
					startAndStopEnabled=0;
					onboardLed_blue_on();

				}
			}
		#endif


		// If CAN message receive is pending, process the message
		if(is_can_msg_pending(CAN_RX_FIFO0)){
			// If message received from bus, parse the frame
			if (can_rx(&rx_msg_header, rx_msg_data) == HAL_OK){
				#if defined(IMMOBILIZER_ENABLED)
					//if it is a message of connection to RFHUB, reset the connection periodically, but start the panic alarm only once
					// if ((rx_msg_header.ExtId==0x18DAC7F1) && (rx_msg_header.DLC==8)){  //commented on 07/07/2024
					if (((rx_msg_header.ExtId==0x18DAC7F1)||(rx_msg_header.ExtId==0x18DAF1C7))){  //added on 07/07/2024  - If msg from thief OR reply from RFHub
						//if(((rx_msg_data[0]==0x02) && (rx_msg_data[1]==0x10) && (rx_msg_data[2]==0x03)) || (rx_msg_data[0]==0x06) && (rx_msg_data[1]==0x50) && (rx_msg_data[2]==0x03)) || ((rx_msg_data[0]==0x01) && (rx_msg_data[1]==0x51) )){ //added on 07/07/2024 - if 0x021003 (msg from thief) OR 0x0151 (reply from RFhub
							//thief connected to RFHUB: we shall reset the RFHUB and start the alarm
							uint8_t i;
							//onboardLed_red_on();
							can_tx(&panicAlarmStartMsgHeader[0], panicAlarmStartMsgData[0]); //reset the connection to RFHUB
							onboardLed_blue_on();
							if(!alarmActivated ){ //if alarm is not running, start the alarm
								alarmActivated=1;
								timeSinceLastPanicAlarmActivation=HAL_GetTick();
								for(i=1; i<5; i++){
									can_tx(&panicAlarmStartMsgHeader[i], panicAlarmStartMsgData[i]);
								}
							}
						//}
					}



					//button press on left area of the wheel (id 2FA), 3 byte length , different than 0x10 that sometimes is sent on the bus (but we are not interested in it)
					if ((rx_msg_header.StdId==0x000002FA) && (rx_msg_header.DLC==3) && rx_msg_data[0]!=0x10){
						//if 2 seconds has passed since last button press, restart password sequence
						if (lastPressedWheelButtonTime+2000<HAL_GetTick()){
							ButtonPressSequence1Index=0;
							lastPressedWheelButtonDuration=0;
							lastPressedWheelButtonTime=HAL_GetTick();
						}
						//if the previous button is still pressed, we shall consider it as the same button press
						if ((lastPressedWheelButton== rx_msg_data[0]) ){
							lastPressedWheelButtonDuration=lastPressedWheelButtonDuration+ (HAL_GetTick()-lastPressedWheelButtonTime) ;
						}else{
							//another button was pressed, let's consider it as a button release event:
							//  if the button was pressed for at least 100msec, and if it is the button expected by the secret sequence
							if(lastPressedWheelButtonDuration>100 && (lastPressedWheelButton==ButtonPressSequence1[ButtonPressSequence1Index])){
								onboardLed_blue_on();
								ButtonPressSequence1Index++; //prepare for next button in the sequence
							}else{
								//something else arrived. restart the sequence
								ButtonPressSequence1Index=0;
							}
							lastPressedWheelButtonDuration=0;
						}

						//manage the particular case of last button of the sequence pressed for enough time
						if(lastPressedWheelButtonDuration>100 && (lastPressedWheelButton==ButtonPressSequence1[ButtonPressSequence1Index]) && ButtonPressSequence1Index==ButtonPressSequence1Len-1){
							ButtonPressSequence1Index++;
						}

						lastPressedWheelButton= rx_msg_data[0]; //store last pressed button
						lastPressedWheelButtonTime=HAL_GetTick(); //store last time a button was pressed

						//if password sequence was fully correctly typed...
						if(ButtonPressSequence1Index==ButtonPressSequence1Len){
							ButtonPressSequence1Index=0; //reset password sequence, so that we can type it again in the future, then do what you shall do
							onboardLed_red_on();
							// we associate to this sequence, the toggle of panic alarm
							alarmActivated=!alarmActivated;
							timeSinceLastPanicAlarmActivation=HAL_GetTick();
							uint8_t i;
							for(i=1; i<5; i++){
								can_tx(&panicAlarmStartMsgHeader[i], panicAlarmStartMsgData[i]);
							}
						}
					}


				#endif

				#if defined(ACT_AS_CANABLE)
					uint16_t msg_len = slcan_parse_frame((uint8_t *)&msg_buf, &rx_msg_header, rx_msg_data);
					if(msg_len){
						CDC_Transmit_FS(msg_buf, msg_len); //transmit data via usb
					}
				#else
					UNUSED(msg_buf); //avoid warning when used as leds strip controller
					//se e' il messaggio che contiene la pressione dell'acceleratore (id 412), se é lungo 5 byte, se il valore é >51 (sfrutto le info ottenute sniffando)
					if ((rx_msg_header.StdId==0x00000412) && (rx_msg_header.DLC==5) && (rx_msg_data[3]>=51)){
						timeSinceLastReceivedCanMessage=HAL_GetTick();
						scaledVolume=scaleVolume(rx_msg_data[3]); //prendi il dato e scalalo, per prepararlo per l'invio alla classe vumeter
					}
					//se e' il messaggio che contiene la marcia (id 2ef) e se é lungo 8 byte
					if ((rx_msg_header.StdId==0x000002EF) && (rx_msg_header.DLC==8)){
						scaledColorSet=scaleColorSet(rx_msg_data[0]); //prendi il dato e scalalo, per prepararlo per l'invio alla classe vumeter
						//onboardLed_red_on();
					}
					vuMeterUpdate(scaledVolume,scaledColorSet);
					ledsStripIsOn=1;
				#endif
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
