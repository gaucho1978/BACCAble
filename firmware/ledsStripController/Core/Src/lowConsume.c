#include "lowConsume.h"

#if defined(C1baccable)

	// Initialize GPIOs
	void lowConsume_init(void){
		__HAL_RCC_GPIOA_CLK_ENABLE();
		GPIO_InitTypeDef GPIO_InitStruct;
		GPIO_InitStruct.Pin = CHIP_LOW_CONSUME_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD; //open drain so that when we set it to 1 it stays float (the other chip will set it to 3,3V)
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
		Remove_Reset_From_Other_Chips();


		GPIO_InitStruct.Pin = CAN_LOW_CONSUME_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; //we need to be able to set it to 3,3V
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
		CAN_LOW_CONSUME_Off();


		//HAL_GPIO_WritePin(CAN_LOW_CONSUME, 1);
		//HAL_GPIO_WritePin(CHIP_LOW_CONSUME, 0);
	}


	void Reset_Other_Chips(void){
		HAL_GPIO_WritePin(CHIP_LOW_CONSUME, 0); // resets the other chips
	}


	void Remove_Reset_From_Other_Chips(void){
		HAL_GPIO_WritePin(CHIP_LOW_CONSUME, 1); // remove reset of other chips
	}

	void CAN_LOW_CONSUME_On(void){
		HAL_GPIO_WritePin(CAN_LOW_CONSUME, 1); //set other can transceivers to Sleep
	}

	void CAN_LOW_CONSUME_Off(void){
		HAL_GPIO_WritePin(CAN_LOW_CONSUME, 0); //set other can transceivers to wakeUp
	}


	// Process time-based events
	void lowConsume_process(void){
		//if(lowConsumeIsActive==0) onboardLed_red_on();
		//statistics_0_100_started++;
		if(lowConsumeIsActive){ //se siamo in basso consumo

			//se l'ultimo messaggio ricevuto é meno vecchio di 5 secondi, risveglia gli altri chip
			if(currentTime-lastReceivedCanMsgTime<TIMING__C1____CAN_ACTIVITY_WINDOW_FOR_WAKEUP_MS){

				wakeUpAllProcessorsAndTransceivers();

				restartUart2();//restart serial line between chips

				lowConsumeIsActive=0;

				allProcessorsWakeupTime=currentTime;
				instructSlaveBoardsTriggerEnabled=1;
			}
		}else{ //altrimenti se non siamo in basso consumo
			//se l'ultimo messaggio ricevuto é piú vecchio di 2,5 secondi, riduci i consumi
			if(currentTime-lastReceivedCanMsgTime>TIMING__C1____CAN_INACTIVITY_TIMEOUT_BEFORE_SLEEP_MS){
				if(usbConnectedToSlave==0){
					lowConsumeIsActive=1;

					pauseUart2(); //stop serial line between chips

					reduceConsumption();
				}
			}
		}
	}

	void reduceConsumption(void){
		//CAN_LOW_CONSUME_On(); //reduce consumption of other can transceivers (set then as only RX)

		Reset_Other_Chips(); //reduce consumption of other chips (left under reset)
		front_brake_forced=0;//ensure we disabled relative functions status in master baccable
		DynoModeEnabledOnMaster=0;//ensure we disabled dyno status on master baccable too
		onboardLed_blue_on();
		//if(lowConsumeIsActive==1) onboardLed_red_on();



	}

	void wakeUpAllProcessorsAndTransceivers(void){
		if(lowConsumeIsActive){
			CAN_LOW_CONSUME_Off(); //wake up transceivers
			Remove_Reset_From_Other_Chips(); //wake up other processors
			onboardLed_blue_on();
		}
	}
#endif
