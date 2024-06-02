//manages leds Strip

#include "vuMeter.h"
#include "stdlib.h"

TIM_HandleTypeDef htim1;
DMA_HandleTypeDef hdma_tim1_ch4_trig_com;

//number of leds in the strip
#define MAX_LED 46
#define USE_BRIGHTNESS 1

#define PI 3.14159265

//tangent of an angle in deg from 0 to 45degrees (46 elements in the array)
const float myTanArray[] = {0, 0.017455065f,0.034920769f,0.052407779f,0.069926812f,0.087488664f,0.105104235f,0.122784561f,0.140540835f,0.15838444f,0.176326981f,0.194380309f,0.212556562f,0.230868191f,0.249328003f,0.267949192f,0.286745386f,0.305730681f,0.324919696f,0.344327613f,0.363970234f,0.383864035f,0.404026226f,0.424474816f,0.445228685f,0.466307658f,0.487732589f,0.509525449f,0.531709432f,0.554309051f,0.577350269f,0.600860619f,0.624869352f,0.649407593f,0.674508517f,0.700207538f,0.726542528f,0.75355405f,0.781285627f,0.809784033f,0.839099631f,0.869286738f,0.900404044f,0.932515086f,0.965688775f,1.0f};

//this contais led color data (see expected structure in set_led function)
uint8_t LED_Data[MAX_LED][4];

//like previous variable but for brightness
uint8_t LED_Mod[MAX_LED][4];

//pwm data array to send to DMA. it contains the list of CCR (it sets the duty cycle of the PWM)
// the size is 24 bit for RGB (3x8) x number of leds in the strip + 50 pwm with duty 0% used to inform the leds strip that we ended one frame transmission
uint16_t pwmData[(24*MAX_LED)+50];

//this variable stores tells us if the frame transmission is in progress or completed
uint8_t datasentflag=1;

//these variables stores volume and color preset used to control the leds strip
float currentVolume=0; //volume received from can bus each millesencond, can be integrated using 10 samples
uint8_t currentColorPreset=0;

uint32_t debugTimer;

uint32_t lastVumeterUpdate=0; //last time we called the related function


//this function initializes gpio,dma,timer and everything needed to output leds protocol.
//if you change leds number you shall adapt the function, where it creates welcome effects
void vuMeterInit(void){
	//initialize GPIO, DMA, and TIMER
	MX_GPIO_Init();
	MX_DMA_Init();
	MX_TIM1_Init();

	//start sending zeros in order to avoid white leds ON at the beginning
	for(int i=0;i<24*MAX_LED;i++){
		pwmData[i]=17; //sets logical 0 (duty 17)
	}
	for(int i=24*MAX_LED;i<24*MAX_LED+50;i++){
			pwmData[i]=0; //sets duty 0
	}
	HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_4, (uint32_t *)pwmData, 24*MAX_LED+50);

	while (!datasentflag){}; //wait the frame to be sent

	datasentflag = 0; //restore the flag for next frame
	HAL_Delay (1000); //sleep 1 sec

	//welcome effect preset
	//set initial leds color like double italian flag
	//setEuropeanFlag();
	setItalianFlag();

	//welcome effect, like double heart pulse
	for (int i=0; i<30; i++){
		Set_Brightness(i);
		WS2812_Send();
		while (!datasentflag){}; //wait the frame to be sent
		datasentflag = 0; //restore the flag for next frame
		HAL_Delay (50); //sleep 50msec
	}

	for (int i=30; i>=0; i--){
		Set_Brightness(i);
		WS2812_Send();
		while (!datasentflag){}; //wait the frame to be sent
		datasentflag = 0; //restore the flag for next frame
		HAL_Delay (50); //sleep 50msec
	}
	HAL_Delay (500);
	for (int i=0; i<30; i++){
		Set_Brightness(i);
		WS2812_Send();
		while (!datasentflag){}; //wait the frame to be sent
		datasentflag = 0; //restore the flag for next frame
		HAL_Delay (50); //sleep 50msec
	}
	for (int i=30; i>=0; i--){
		Set_Brightness(i);
		WS2812_Send();
		while (!datasentflag){}; //wait the frame to be sent
		datasentflag = 0; //restore the flag for next frame
		HAL_Delay (50); //sleep 50msec
	}
}

void setItalianFlag(){
	//set initial leds color like double italian flag
	//rrrrrrrr-wwwwww-ggggggg-gggg-ggggggg-wwwwww-rrrrrrrr
	uint8_t i;
	for(i=0;i<8;i++){
	  Set_LED(i, 255, 0, 0); //red
	}
	for(i=8;i<14;i++){
	  Set_LED(i, 255, 255, 200); //white
	}
	for(i=14; i<32;i++){
	  Set_LED(i, 0, 255, 0); //green
	}
	for(i=32;i<38;i++){
	  Set_LED(i, 255, 255, 200); //white
	}
	for(i=38;i<46;i++){
	  Set_LED(i, 255, 0, 0); //red
	}
}

void setCrazyFlag(){
	uint8_t i, crazyRed, crazyGreen, crazyBlue;
	for(i=0;i<46;i++){
		crazyRed=(uint8_t)(rand() % 255);
		crazyGreen=(uint8_t)(rand() % 255);
		crazyBlue=(uint8_t)(rand() % 64); //less blue
		Set_LED(i, crazyRed, crazyGreen, crazyBlue); //random color
	}

}

void setGermanFlag(){
	//red: 221,0,0 yellow:255,206,0
	//set initial leds color like double italian flag
	//yyyyyyyy-rrrrrr-bbbbb-yyy-rrrrr-bbbbb-rrrrrr-yyyyyyyy
	uint8_t i;

	for(i=0;i<8;i++){
		Set_LED(i, 255, 90, 0); //yellow
	}
	for(i=8;i<14;i++){
		Set_LED(i, 221, 0, 0); //red
	}
	for(i=14; i<19;i++){
	  Set_LED(i, 0, 0, 0); //black
	}
	for(i=19;i<22;i++){
		Set_LED(i, 255, 90, 0); //yellow
	}
	for(i=22;i<27;i++){
		Set_LED(i, 221, 0, 0); //red
	}
	for(i=27;i<32;i++){
			Set_LED(i, 0, 0, 0); //black
	}
	for(i=32;i<38;i++){
		Set_LED(i, 221, 0, 0); //red
	}
	for(i=38;i<46;i++){
		Set_LED(i, 255, 90, 0); //yellow
	}
}

void setEuropeanFlag(){
	// yellow:255,204,0 blue:0,51,153
	//bbbybbbybbbybbbybbbybbbybbbybbbybbbybbbybbbybb
	uint8_t i;

	for(i=0;i<46;i++){
		switch( i % 4){
			case 3:
				Set_LED(i, 255, 80, 0); //yellow
				break;
			case 0:
			case 1:
			case 2:
			default:
				Set_LED(i, 0, 51, 153); //blue

				break;
		}
	}
}

//updates current volume in the vumeter effect of the leds strip, based on volume and
// color preset. We use the volume parameter to change brightness (and light only required
// leds), and we use the color_preset parameter (gear position) to change colors of the leds
void vuMeterUpdate(float volume, uint8_t colorPreset ){

	currentVolume= (currentVolume * 99.0 / 100.0)+(volume / 100.0); //integrated in time on 50 samples (one message each 2 millisecond?, more or less)

	if(colorPreset!=currentColorPreset){
		switch(colorPreset){
			case 2: //second gear
				// EU flag
				setEuropeanFlag();
				break;
			case 5: //fifth gear
				// EU flag
				setEuropeanFlag();
				break;
			case 3: // third gear
				//set German flag
				//setGermanFlag();
				setCrazyFlag();
				break;
			case 6: // sixt gear
				//set German flag
				//setGermanFlag();
				setCrazyFlag();
				break;
			default: //9 backward and 0 neutral and 1 first gear
				// italian flag
				setItalianFlag();
				break;
		}
		currentColorPreset=colorPreset;
	}
	UNUSED(lastVumeterUpdate);
	if(lastVumeterUpdate+10 > HAL_GetTick()) { //enter update function only once each 10 msec.
		return;
	}
	lastVumeterUpdate=HAL_GetTick();

	//if the frame was not completely sent, exit, otherwise send another frame
	if (datasentflag){
		datasentflag = 0;
	}else{
		return;
	}



	//currentVolume/6.66f; //scaled to be between 0 and 15
	//currentVolume/4.54f; //scaled to be between 0 and 22 (more reactive)
	//currentVolume/3.33f; //scaled to be between 0 and 30 (2 times more reactive)
	//currentVolume/1.60f; //scaled to be between 0 and 60 (4 times more reactive)

	//...since the following function has a dynamic of the volume between 0 and 15.
	setLedsVumeter((uint8_t)(currentVolume/1.60f+0.5f));
	WS2812_Send();

}


// volume between 0 and 100 (sets brightness of leds to generate the vumeter effect
void setLedsVumeter(uint8_t volume){
	uint8_t brightness;
	uint8_t i;
	uint8_t j;
	uint8_t distanzaDalSetpoint;
	//for debug, measure function duration
	//debugTimer=HAL_GetTick();

	if(volume>15) volume=15; //clip values to avoid overflows

	for(i=0; i<15;i++){
		//usiamo questa variabile per aggiungere un poco di sfumatura sulla transizione
		// per ottimizzare l'esecuzione l'ho sostituita col pezzo di codice che segue
		//float distanzaDalSetpoint=fabsf((15.0f-volume)-(float)i)+1.0f; //oscilla tra 16 e 1
		//distanzaDalSetpoint= 15 * 1/powf(distanzaDalSetpoint,3); // diventa una variabile k che si riduce proporzionalmente alla distanza dalla soglia

		//usiamo questa variabile per aggiungere un poco di sfumatura sulla transizione
		distanzaDalSetpoint=myAbs(15-volume-i); //oscilla tra 15 e 0
		if (distanzaDalSetpoint==0) distanzaDalSetpoint=22;
		if (distanzaDalSetpoint==1) distanzaDalSetpoint=3;
		if (distanzaDalSetpoint==2) distanzaDalSetpoint=2;
		if (distanzaDalSetpoint==3) distanzaDalSetpoint=1;
		if (distanzaDalSetpoint!=22 && distanzaDalSetpoint!=3 && distanzaDalSetpoint!=2 && distanzaDalSetpoint!=1 ) distanzaDalSetpoint=0;

		if(i<15-volume){ //con volume al massimo vale 0: a minimo vale 15
			brightness=0+distanzaDalSetpoint; // aggiungiamo un piccolo effetto di sfumatura
		}else{
			brightness=45-distanzaDalSetpoint; // aggiungiamo un piccolo effetto di sfumatura
		}
		LED_Mod[i][0] = LED_Data[i][0];
		for (j=1; j<4; j++){
			//double angle = 90.0-(double)brightness;  // in degrees
			//angle = angle*PI / 180.0;  // in rad
			//LED_Mod[i][j] = (LED_Data[i][j])/(tan(angle));
			LED_Mod[i][j] = (LED_Data[i][j]) * myTanArray[brightness];
		}
	}

		//between 15 and 20 are always on at max bright
		//brightness=30;
		for(i=15; i<31;i++){
			LED_Mod[i][0] = LED_Data[i][0];
			for (j=1; j<4; j++){
				//double angle = 90.0-(double)brightness;  // in degrees
				//angle = angle*PI / 180;  // in rad
				//LED_Mod[i][j] = (LED_Data[i][j])/(tan(angle));
				LED_Mod[i][j] = (LED_Data[i][j]);
			}
		}

	//let's set the brightness also for the other part of the flag
	for(i=31; i<46;i++){
		//usiamo questa variabile per aggiungere un poco di sfumatura sulla transizione
		distanzaDalSetpoint=myAbs(volume-i-31); //oscilla tra 15 e 0
		if (distanzaDalSetpoint==0) distanzaDalSetpoint=22;
		if (distanzaDalSetpoint==1) distanzaDalSetpoint=3;
		if (distanzaDalSetpoint==3) distanzaDalSetpoint=1;
		if (distanzaDalSetpoint!=15 && distanzaDalSetpoint!=2 && distanzaDalSetpoint!=1) distanzaDalSetpoint=0;

		if(i<31+volume){ //con volume al massimo vale 46: a minimo vale 31
			brightness=45-distanzaDalSetpoint; // aggiungiamo un piccolo effetto di sfumatura;
		}else{
			brightness=0+distanzaDalSetpoint; // aggiungiamo un piccolo effetto di sfumatura;
		}
		LED_Mod[i][0] = LED_Data[i][0];
		for (j=1; j<4; j++){
			//double angle = 90.0-(double)brightness;  // in degrees
			//angle = angle*PI / 180.0;  // in rad
			//LED_Mod[i][j] = (LED_Data[i][j])/(tan(angle));
			LED_Mod[i][j] = (LED_Data[i][j]) * myTanArray[brightness];
		}
	}

	//for debug, measure function duration
	//if (HAL_GetTick()>debugTimer+1){
	//	onboardLed_red_on();
	//}

}

//this function sets one led (my led strip is BRG (blue,red,green). you can set this function according to your led strip type.
void Set_LED (uint8_t LEDnum, uint8_t Red, uint8_t Green, uint8_t Blue){
	LED_Data[LEDnum][0] = LEDnum;
	LED_Data[LEDnum][1] = Blue;
	LED_Data[LEDnum][2] = Red;
	LED_Data[LEDnum][3] = Green;
}


//this function sets the brightness (value beween 0 and 45)
void Set_Brightness (uint8_t brightness){
	if (brightness > 45) brightness = 45; //clip possible values, to prevent overflows
	for (int i=0; i<MAX_LED; i++){
		LED_Mod[i][0] = LED_Data[i][0];
		for (int j=1; j<4; j++){
			//float angle = 90-brightness;  // in degrees
			//angle = angle*PI / 180;  // in rad
			//LED_Mod[i][j] = (LED_Data[i][j])/(tan(angle));
			LED_Mod[i][j] = (LED_Data[i][j]) * myTanArray[brightness];
		}
	}
}


//this function composes the array to send to DMA
void WS2812_Send (void){
	uint32_t indx=0;
	uint32_t color;
	uint8_t i;
	int8_t i2;

	for (i= 0; i<MAX_LED; i++){
		#if USE_BRIGHTNESS
			color = ((LED_Mod[i][1]<<16) | (LED_Mod[i][2]<<8) | (LED_Mod[i][3]));
		#else
			color = ((LED_Data[i][1]<<16) | (LED_Data[i][2]<<8) | (LED_Data[i][3]));
		#endif

		for (i2=23; i2>=0; i2--){
			if (color&(1<<i2)){
				pwmData[indx] = 34;  //set duty cycle for logical 1 (708usec up, period 1,25usec)
			}else{
				pwmData[indx] = 17;  //set duty cycle for logical 0 (354usec up, period 1,25usec)
			}
			indx++;
		}

	}
	//the following is not needed because that record is still zero
	//for (int i=0; i<50; i++){
	//	pwmData[indx] = 0;
	//	indx++;
	//}

	//prendi il tempo, per debug
	//debugTimer=HAL_GetTick();
	HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_4, (uint32_t *)pwmData, 24*MAX_LED+50);

}


//init Timer used to control the led strip by generating pwm output updating the duty cycle in order to implement the WS2812 protocol.
// In my case I use Timer1, channel4 because my gpio is PA11 (used as usb pin in original canable firmware.
// In order to use a Timer clock of 48Mhz I use internal HSI48 oscillator (48Mhz) and prescaler=1 (timerClock= APB_tim_clk/prescaler)
// For WS2811 and WS2812 leds strip We need a pwm period of 1,25us.
// Considering that pwmPeriod= ARR/timerClok => ARR=pwmPeriod * timerClock = 1,25us * 48Mhz = 60
// ARR is the AutoReloadRegister and defines when the timer counter shall restart from 0, therefore we set it to 60 (always -1 for
// these parameters type), in order to obtain a pwm output period of 1,25usec.
// In WS2811 and WS2812 leds strip protocol, the logical 0 impplies a pwm duty cycle of around 28% (0,28), and logical 1 implies a duty cycle
// of around 56% (0,56).
// the dutyCycle= CCRx/ARR  => CCRx=dutyCycle * ARR
// CCR is the CaptureCompareRegister and there is one for each channel. Since we use channel4, we shall set CCR4 as it follows:
//  - CCR4 (for logical bit 0) = 0,28 * 60 = 17
//  - CCR4 (for logical bit 1) = 0,56 * 60 = 34
void MX_TIM1_Init(void){

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 1-1;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 60-1;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_TIM_MspPostInit(&htim1);

}

// Enable DMA controller clock
// DMA is used to feed the timer with an array of CCR values, in order to allow efficient code execution, without
// loosing time in our main loop to control the gpio realtime toggle
void MX_DMA_Init(void){
  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();
  /* DMA interrupt init */
  /* DMA1_Channel4_5_6_7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel4_5_6_7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_5_6_7_IRQn);
}

//This function is called when DMA completes to send its bits. When this occour we will stop the pwm generation, in order to be
// ready to send a new frame, otherwise last bit continues to be sent generating a mistake in the ws281x protocol output.
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim){
	HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_4);
	datasentflag=1;
	//if (HAL_GetTick()>debugTimer+20){
	//	onboardLed_red_on();
	//}

}

// GPIO Initialization Function
void MX_GPIO_Init(void){
  __HAL_RCC_GPIOA_CLK_ENABLE();
}


uint8_t myAbs(int8_t num){
	//if the input is negative, multiply by -1
    if(num < 0){
        return (-1) * num;
    }else{
        return num;
    }
}
