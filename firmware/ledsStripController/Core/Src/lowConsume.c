#include "stm32f0xx_hal.h"
#include "lowConsume.h"

uint32_t lastChipResetTime=0;
uint8_t chipResetRequest=0;

// Initialize GPIOs
void lowConsume_init(){
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = CHIP_LOW_CONSUME_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD; //open drain so that when we set it to 1 it stays float (the other chip will set it to 3,3V)
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = CAN_LOW_CONSUME_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; //we need to be able to set it to 3,3V
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

}


void Reset_Other_Chips(void){
	HAL_GPIO_WritePin(CHIP_LOW_CONSUME, 0); // resets the other chips
	lastChipResetTime= HAL_GetTick();
	chipResetRequest=1;
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
	// If GPIO has been on for long enough, turn it off
	if(chipResetRequest){
		if(HAL_GetTick() - lastChipResetTime > CHIP_RESET_DURATION){
			Remove_Reset_From_Other_Chips();
			chipResetRequest=0;
		}
	}
}
