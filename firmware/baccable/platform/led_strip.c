// manages leds Strip

#include "platform/led_strip.h"
#include "stdlib.h"

TIM_HandleTypeDef htim1;
DMA_HandleTypeDef hdma_tim1_ch4_trig_com;

// number of leds in the strip
#define MAX_LED 46
#define USE_BRIGHTNESS 1

// tangent of an angle in deg from 0 to 45degrees (46 elements in the array)
const float myTanArray[] = {0,
                            0.017455065f,
                            0.034920769f,
                            0.052407779f,
                            0.069926812f,
                            0.087488664f,
                            0.105104235f,
                            0.122784561f,
                            0.140540835f,
                            0.15838444f,
                            0.176326981f,
                            0.194380309f,
                            0.212556562f,
                            0.230868191f,
                            0.249328003f,
                            0.267949192f,
                            0.286745386f,
                            0.305730681f,
                            0.324919696f,
                            0.344327613f,
                            0.363970234f,
                            0.383864035f,
                            0.404026226f,
                            0.424474816f,
                            0.445228685f,
                            0.466307658f,
                            0.487732589f,
                            0.509525449f,
                            0.531709432f,
                            0.554309051f,
                            0.577350269f,
                            0.600860619f,
                            0.624869352f,
                            0.649407593f,
                            0.674508517f,
                            0.700207538f,
                            0.726542528f,
                            0.75355405f,
                            0.781285627f,
                            0.809784033f,
                            0.839099631f,
                            0.869286738f,
                            0.900404044f,
                            0.932515086f,
                            0.965688775f,
                            1.0f};

// this contais led color data (see expected structure in set_led function)
uint8_t LED_Data[MAX_LED][4];

// like previous variable but for brightness
uint8_t LED_Mod[MAX_LED][4];

// pwm data array to send to DMA. it contains the list of CCR (it sets the duty cycle of the PWM)
//  the size is 24 bit for RGB (3x8) x number of leds in the strip + 50 pwm with duty 0% used to inform the
//  leds strip that we ended one frame transmission
uint16_t pwmData[(24 * MAX_LED) + 50];

// this variable stores tells us if the frame transmission is in progress or completed
uint8_t datasentflag = 1;

// these variables stores volume and color preset used to control the leds strip
float currentVolume = 0; // volume received from can bus each millesencond, can be integrated using 10 samples
uint8_t currentColorPreset = 0;

uint32_t lastVumeterUpdate = 0; // last time we called the related function

uint8_t vuMeterInitState = 0;  // state machine of the initialization phase. used to divide actions on more
                               // loops and avoid blocking execution
uint32_t newInternalDelay = 0; // used to manage pauses inside init sequence
int16_t newInternalIndex = 0;  // used for cycles inside init sequence

/* Prepare the optional LED display in stages so other features can keep running. */
void led_strip_init(void) {
    switch (vuMeterInitState) {
    case 0:
        // initialize GPIO, DMA, and TIMER
        MX_GPIO_Init();
        MX_DMA_Init();
        MX_TIM1_Init();
        vuMeterInitState++;
        return;
    case 1:
        led_strip_shutdown();
        vuMeterInitState++;
        return;
    case 2:
        if (datasentflag == 1) { // wait the frame to be sent
            datasentflag = 0;    // restore the flag for next frame
            newInternalDelay = currentTime;
            vuMeterInitState++;
        }
        break;
    case 3:
        // let's sleep for 1 second. but if time is elapsed, go inside if
        if (newInternalDelay + 1000 < currentTime) {
            vuMeterInitState++;
        }
        break;
    case 4:
        // welcome effect preset
        // set initial leds color like double italian flag

        setItalianFlag();
        newInternalIndex = 0;
        vuMeterInitState++;
        return;
    case 5:
        // welcome effect, like double heart pulse
        if (newInternalIndex < 30) {
            Set_Brightness(newInternalIndex);
            WS2812_Send();
            // wait message to be sent
            vuMeterInitState++;
        } else {
            // jump to nex phase
            vuMeterInitState = 8;
            newInternalIndex = 30; // reset the index counter for newt phase
        }
        break;
    case 6:
        if (datasentflag == 1) { // wait the frame to be sent
            datasentflag = 0;    // restore the flag for next frame
            newInternalDelay = currentTime;
            vuMeterInitState++;
        }
        break;
    case 7:
        // let's sleep for 50msec. but if time is elapsed, go inside if
        if (newInternalDelay + 50 < currentTime) {
            newInternalIndex++;
            vuMeterInitState = 5;
        }
        break;
    case 8:
        // welcome effect, like double heart pulse
        if (newInternalIndex >= 0) {
            Set_Brightness(newInternalIndex);
            WS2812_Send();
            // wait message to be sent
            vuMeterInitState++;
        } else {
            // jump to nex phase
            vuMeterInitState = 11;
            newInternalDelay = currentTime; // prepare to sleep
        }
        break;
    case 9:
        if (datasentflag == 1) { // wait the frame to be sent
            datasentflag = 0;    // restore the flag for next frame
            newInternalDelay = currentTime;
            vuMeterInitState++;
        }
        break;
    case 10:
        // let's sleep for 50msec. but if time is elapsed, go inside if
        if (newInternalDelay + 50 < currentTime) {
            newInternalIndex--;
            vuMeterInitState = 8;
        }
        break;
    case 11:
        // let's sleep for 500msec. but if time is elapsed, go inside if
        if (newInternalDelay + 500 < currentTime) {
            vuMeterInitState++;
            newInternalIndex = 0; // reset the index counter for newt phase
        }
        break;
    case 12: // ex5
        // welcome effect, like double heart pulse
        if (newInternalIndex < 30) {
            Set_Brightness(newInternalIndex);
            WS2812_Send();
            // wait message to be sent
            vuMeterInitState++;
        } else {
            // jump to nex phase
            vuMeterInitState = 15;
            newInternalIndex = 30; // reset the index counter for newt phase
        }
        break;
    case 13:                     // ex 6:
        if (datasentflag == 1) { // wait the frame to be sent
            datasentflag = 0;    // restore the flag for next frame
            newInternalDelay = currentTime;
            vuMeterInitState++;
        }
        break;
    case 14: // ex 7:
        // let's sleep for 50msec. but if time is elapsed, go inside if
        if (newInternalDelay + 50 < currentTime) {
            newInternalIndex++;
            vuMeterInitState = 12;
        }
        break;
    case 15: // ex 8:
        // welcome effect, like double heart pulse
        if (newInternalIndex >= 0) {
            Set_Brightness(newInternalIndex);
            WS2812_Send();
            // wait message to be sent
            vuMeterInitState++;
        } else {
            // jump to nex phase
            vuMeterInitState = 18;
            newInternalDelay = currentTime; // prepare to sleep
        }
        break;
    case 16:                     // ex 9:
        if (datasentflag == 1) { // wait the frame to be sent
            datasentflag = 0;    // restore the flag for next frame
            newInternalDelay = currentTime;
            vuMeterInitState++;
        }
        break;
    case 17: // ex 10:
        // let's sleep for 50msec. but if time is elapsed, go inside if
        if (newInternalDelay + 50 < currentTime) {
            newInternalIndex--;
            vuMeterInitState = 15;
        }
        break;
    case 18: // ex 11:
        // let's sleep for 500msec. but if time is elapsed, go inside if
        if (newInternalDelay + 500 < currentTime) {
            vuMeterInitState++;
            newInternalIndex = 0; // reset the index counter for newt phase
        }
        break;
    case 50:
        return; // init completed
    default:
        vuMeterInitState++; // increment the counter - we leave empty position for future growth
        break;
    }
}

/* Turn off the optional LED display. */
void led_strip_shutdown() {
    // start sending zeros in order to avoid white leds ON at the beginning
    for (int i = 0; i < 24 * MAX_LED; i++) {
        pwmData[i] = 17; // sets logical 0 (duty 17)
    }
    for (int i = 24 * MAX_LED; i < 24 * MAX_LED + 50; i++) {
        pwmData[i] = 0; // sets duty 0
    }
    HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_4, (uint32_t *)pwmData, 24 * MAX_LED + 50);
}
/* Select the Italian-flag color pattern. */
void setItalianFlag() {
    // set initial leds color like double italian flag
    // rrrrrrrr-wwwwww-ggggggg-gggg-ggggggg-wwwwww-rrrrrrrr
    uint8_t i;
    for (i = 0; i < 8; i++) {
        Set_LED(i, 255, 0, 0); // red
    }
    for (i = 8; i < 14; i++) {
        Set_LED(i, 255, 255, 200); // white
    }
    for (i = 14; i < 32; i++) {
        Set_LED(i, 0, 255, 0); // green
    }
    for (i = 32; i < 38; i++) {
        Set_LED(i, 255, 255, 200); // white
    }
    for (i = 38; i < 46; i++) {
        Set_LED(i, 255, 0, 0); // red
    }
}

/* Select a randomized decorative color pattern. */
void setCrazyFlag() {
    uint8_t i, crazyRed, crazyGreen, crazyBlue;
    for (i = 0; i < 46; i++) {
        crazyRed = (uint8_t)(rand() % 255);
        crazyGreen = (uint8_t)(rand() % 255);
        crazyBlue = (uint8_t)(rand() % 64);          // less blue
        Set_LED(i, crazyRed, crazyGreen, crazyBlue); // random color
    }
}

/* Select the German-flag color pattern. */
void setGermanFlag() {
    // red: 221,0,0 yellow:255,206,0
    // set initial leds color like double italian flag
    // yyyyyyyy-rrrrrr-bbbbb-yyy-rrrrr-bbbbb-rrrrrr-yyyyyyyy
    uint8_t i;

    for (i = 0; i < 8; i++) {
        Set_LED(i, 255, 90, 0); // yellow
    }
    for (i = 8; i < 14; i++) {
        Set_LED(i, 221, 0, 0); // red
    }
    for (i = 14; i < 19; i++) {
        Set_LED(i, 0, 0, 0); // black
    }
    for (i = 19; i < 22; i++) {
        Set_LED(i, 255, 90, 0); // yellow
    }
    for (i = 22; i < 27; i++) {
        Set_LED(i, 221, 0, 0); // red
    }
    for (i = 27; i < 32; i++) {
        Set_LED(i, 0, 0, 0); // black
    }
    for (i = 32; i < 38; i++) {
        Set_LED(i, 221, 0, 0); // red
    }
    for (i = 38; i < 46; i++) {
        Set_LED(i, 255, 90, 0); // yellow
    }
}

/* Select the European-flag color pattern. */
void setEuropeanFlag() {
    // yellow:255,204,0 blue:0,51,153
    // bbbybbbybbbybbbybbbybbbybbbybbbybbbybbbybbbybb
    uint8_t i;

    for (i = 0; i < 46; i++) {
        switch (i % 4) {
        case 3:
            Set_LED(i, 255, 80, 0); // yellow
            break;
        case 0:
        case 1:
        case 2:
        default:
            Set_LED(i, 0, 51, 153); // blue

            break;
        }
    }
}

/* Refresh the LED display for the latest intensity and color selection. */
void led_strip_update(float volume, uint8_t colorPreset) {
    if (vuMeterInitState < 50)
        return; // initialization still not completed
    if (volume > 24)
        volume = 24.0;
    currentVolume = (currentVolume * 9.0 / 10.0) +
                    (volume / 10.0); // integrated in time on 10 samples (one message each ? boh!)

    if (colorPreset != currentColorPreset) {
        switch (colorPreset) {
        case 2: // second gear
            // EU flag
            setEuropeanFlag();
            break;
        case 5: // fifth gear
            // EU flag
            setEuropeanFlag();
            break;
        case 3: // third gear
            // Select a randomized pattern.
            setCrazyFlag();
            break;
        case 6: // sixt gear
            // Select a randomized pattern.
            setCrazyFlag();
            break;
        default: // 9 backward and 0 neutral and 1 first gear
            // italian flag
            setItalianFlag();
            break;
        }
        currentColorPreset = colorPreset;
    }
    if (lastVumeterUpdate + 10 > currentTime) { // enter update function only once each 10 msec.
        return;
    }
    lastVumeterUpdate = currentTime;

    // if the frame was not completely sent, exit, otherwise send another frame
    if (datasentflag) {
        datasentflag = 0;
    } else {
        return;
    }

    // currentVolume/6.66f; //scaled to be between 0 and 15
    // currentVolume/4.54f; //scaled to be between 0 and 22 (more reactive)
    // currentVolume/3.33f; //scaled to be between 0 and 30 (2 times more reactive)
    // currentVolume/1.60f; //scaled to be between 0 and 60 (4 times more reactive)

    // The intensity renderer expects a level from 0 to 15.
    setLedsVumeter((uint8_t)(currentVolume / 1.60f + 0.5f));
    WS2812_Send();
}

/* Shape the LED intensity around the current level indication. */
void setLedsVumeter(uint8_t volume) {
    uint8_t brightness;
    uint8_t i;
    uint8_t j;
    uint8_t transition_distance;

    if (volume > 15)
        volume = 15; // clip values to avoid overflows

    for (i = 0; i < 15; i++) {
        // Soften the transition around the current intensity level.

        // Soften the transition around the current intensity level.
        transition_distance = myAbs(15 - volume - i); // Distance from the intensity boundary, from 0 to 15.
        if (transition_distance == 0)
            transition_distance = 22;
        if (transition_distance == 1)
            transition_distance = 3;
        if (transition_distance == 2)
            transition_distance = 2;
        if (transition_distance == 3)
            transition_distance = 1;
        if (transition_distance != 22 && transition_distance != 3 && transition_distance != 2 &&
            transition_distance != 1)
            transition_distance = 0;

        if (i < 15 - volume) {                    // The boundary moves from 15 to 0 as the level increases.
            brightness = 0 + transition_distance; // Soften the brightness transition.
        } else {
            brightness = 45 - transition_distance; // Soften the brightness transition.
        }
        LED_Mod[i][0] = LED_Data[i][0];
        for (j = 1; j < 4; j++) {

            LED_Mod[i][j] = (LED_Data[i][j]) * myTanArray[brightness];
        }
    }

    // between 15 and 20 are always on at max bright

    for (i = 15; i < 31; i++) {
        LED_Mod[i][0] = LED_Data[i][0];
        for (j = 1; j < 4; j++) {

            LED_Mod[i][j] = (LED_Data[i][j]);
        }
    }

    // let's set the brightness also for the other part of the flag
    for (i = 31; i < 46; i++) {
        // Soften the transition around the current intensity level.
        transition_distance = myAbs(volume - i - 31); // Distance from the intensity boundary, from 0 to 15.
        if (transition_distance == 0)
            transition_distance = 22;
        if (transition_distance == 1)
            transition_distance = 3;
        if (transition_distance == 3)
            transition_distance = 1;
        if (transition_distance != 15 && transition_distance != 2 && transition_distance != 1)
            transition_distance = 0;

        if (i < 31 + volume) {                     // The boundary moves from 31 to 46 as the level increases.
            brightness = 45 - transition_distance; // Soften the brightness transition.
        } else {
            brightness = 0 + transition_distance; // Soften the brightness transition.
        }
        LED_Mod[i][0] = LED_Data[i][0];
        for (j = 1; j < 4; j++) {

            LED_Mod[i][j] = (LED_Data[i][j]) * myTanArray[brightness];
        }
    }
}

/* Set one LED's color in the selected pattern. */
void Set_LED(uint8_t LEDnum, uint8_t Red, uint8_t Green, uint8_t Blue) {
    LED_Data[LEDnum][0] = LEDnum;
    LED_Data[LEDnum][1] = Blue;
    LED_Data[LEDnum][2] = Red;
    LED_Data[LEDnum][3] = Green;
}

/* Apply an overall brightness level to the LED pattern. */
void Set_Brightness(uint8_t brightness) {
    if (brightness > 45)
        brightness = 45; // clip possible values, to prevent overflows
    for (int i = 0; i < MAX_LED; i++) {
        LED_Mod[i][0] = LED_Data[i][0];
        for (int j = 1; j < 4; j++) {

            LED_Mod[i][j] = (LED_Data[i][j]) * myTanArray[brightness];
        }
    }
}

/* Start displaying the prepared LED pattern. */
void WS2812_Send(void) {
    uint32_t indx = 0;
    uint32_t color;
    uint8_t i;
    int8_t i2;

    for (i = 0; i < MAX_LED; i++) {
#if USE_BRIGHTNESS
        color = ((LED_Mod[i][1] << 16) | (LED_Mod[i][2] << 8) | (LED_Mod[i][3]));
#else
        color = ((LED_Data[i][1] << 16) | (LED_Data[i][2] << 8) | (LED_Data[i][3]));
#endif

        for (i2 = 23; i2 >= 0; i2--) {
            if (color & (1 << i2)) {
                pwmData[indx] = 34; // set duty cycle for logical 1 (708usec up, period 1,25usec)
            } else {
                pwmData[indx] = 17; // set duty cycle for logical 0 (354usec up, period 1,25usec)
            }
            indx++;
        }
    }
    // the following is not needed because that record is still zero

    HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_4, (uint32_t *)pwmData, 24 * MAX_LED + 50);
}

/* Prepare the timing used to display LED colors. */
void MX_TIM1_Init(void) {
    /* PA11 shares the USB pin: TIM1 channel 4 uses a 48 MHz clock, period 60, and duties 17/34. */

    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};
    TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

    htim1.Instance = TIM1;
    htim1.Init.Prescaler = 1 - 1;
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = 60 - 1;
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim1) != HAL_OK) {
        Error_Handler(3000);
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK) {
        Error_Handler(3500);
    }
    if (HAL_TIM_PWM_Init(&htim1) != HAL_OK) {
        Error_Handler(4000);
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK) {
        Error_Handler(4500);
    }
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_4) != HAL_OK) {
        Error_Handler(5000);
    }
    sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
    sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
    sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
    sBreakDeadTimeConfig.DeadTime = 0;
    sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
    sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
    sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
    if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK) {
        Error_Handler(5500);
    }

    HAL_TIM_MspPostInit(&htim1);
}

/* Prepare background delivery of LED patterns. */
void MX_DMA_Init(void) {
    /* DMA controller clock enable */
    __HAL_RCC_DMA1_CLK_ENABLE();
    /* DMA interrupt init */
    /* DMA1_Channel4_5_6_7_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA1_Channel4_5_6_7_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel4_5_6_7_IRQn);
}

/* Mark an LED pattern as delivered so another update can begin. */
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
    HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_4);
    datasentflag = 1;
}

/* Enable the board output used by the LED display. */
void MX_GPIO_Init(void) { __HAL_RCC_GPIOA_CLK_ENABLE(); }

/* Measure distance from the LED intensity transition without a sign. */
uint8_t myAbs(int8_t num) {
    // if the input is negative, multiply by -1
    if (num < 0) {
        return (-1) * num;
    } else {
        return num;
    }
}
