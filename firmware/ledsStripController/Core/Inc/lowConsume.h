
#ifndef INC_LOWCONSUME_H_
#define INC_LOWCONSUME_H_


	#define CHIP_LOW_CONSUME_Pin GPIO_PIN_4
	#define CHIP_LOW_CONSUME_Port GPIOA
	#define CHIP_LOW_CONSUME CHIP_LOW_CONSUME_Port , CHIP_LOW_CONSUME_Pin

	#define CAN_LOW_CONSUME_Pin GPIO_PIN_5
	#define CAN_LOW_CONSUME_Port GPIOA
	#define CAN_LOW_CONSUME CAN_LOW_CONSUME_Port , CAN_LOW_CONSUME_Pin

	#define CHIP_RESET_DURATION 500 //time in milliseconds

	void lowConsume_init();
	void Reset_Other_Chips();
	void Remove_Reset_From_Other_Chips();
	void CAN_LOW_CONSUME_On();
	void CAN_LOW_CONSUME_Off();
	void lowConsume_process();

#endif /* INC_LOWCONSUME_H_ */
