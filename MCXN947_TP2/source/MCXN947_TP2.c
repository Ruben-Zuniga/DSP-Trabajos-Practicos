/*
 Trabajo Practico 2: Filtro FIR

 PINES:
 	 ADC0: J4.2
 	 DAC0: J1.4
 	 DAC1: J1.2
 	 MATCH0: J7.1 o J2.13
 	 GND: J5.8 o J6.8
 */
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "fsl_ctimer.h"
#include "fsl_dac.h"
#include "fsl_lpadc.h"
#include "arm_math.h"

/*******************************************************************************
 * Definiciones
 ******************************************************************************/

// Frecuencias de muestreo requeridas
const uint32_t sample_rates[] = {8000, 16000, 22000, 44000, 48000}; // frecuencias de muestreo
//const uint32_t sample_rates[] = {1, 2, 4, 8, 16}; // frecuencias de muestreo

#define NUM_RATES (sizeof(sample_rates)/sizeof(sample_rates[0])) // tamaño del arreglo
// NUM_RATES se define de esta forma para poder obtener el tamaño del arreglo sin importar el
// tipo de numero (en este caso uint32_t) ni la cantidad de numeros del arreglo (en este caso 5)

// Pines de botones (ajustar según board)
#define SW2_PIN   23   // PIO0_23
#define SW3_PIN   6   // PIO0_6

// Frecuencia del CTIMER
#define CTIMER_CLK_FREQ 600000 // 1.2 MHz

// Tamaño de los buffers
#define BUFFER_SIZE   (uint32_t)2048	// buffer de datos
#define TAPS_SIZE	  (uint16_t)124    // orden del filtro
#define INPUT_SIZE 	  (uint32_t)4     // entrada al filtro powerquad

// Pines del LED
#define BOARD_LED_RED_GPIO GPIO0
#define BOARD_LED_RED_PIN 10
#define BOARD_LED_GREEN_GPIO GPIO0
#define BOARD_LED_GREEN_PIN 27
#define BOARD_LED_BLUE_GPIO GPIO1
#define BOARD_LED_BLUE_PIN 2

/*******************************************************************************
 * Prototipos
 ******************************************************************************/
static void UpdateLedColor(uint8_t idx);
static void LED_SetColor(bool RED, bool GREEN, bool BLUE);

/*******************************************************************************
 * Variables globales
 ******************************************************************************/
bool filter_run = true; // prender/apagar filtro
//bool process_half_A = false; // primera mitad llena (buffer)
//bool process_half_B = false; // segunda mitad llena (buffer)

uint8_t sample_rate_idx = 0; // indice para elegir frecuencia de muestreo

uint16_t adc_buf_index = 0; // indice para recorrer el buffer circular de entrada
uint16_t dac_val; // valor hacia el DAC

arm_fir_instance_q15 fir[4][5];

// Pasabajo: fc = 3,65 kHz
const q15_t pasabajo_8k[32] = {
      236,   -318,    400,   -470,    519,   -534,    500,   -402,    224,
       55,   -464,   1055,  -1934,   3381,  -6439,  20576,  20576,  -6439,
     3381,  -1934,   1055,   -464,     55,    224,   -402,    500,   -534,
      519,   -470,    400,   -318,    236
};
const q15_t pasabajo_16k[42] = {
     -133,    155,    236,   -122,   -356,     33,    475,    127,   -571,
     -368,    613,    704,   -560,  -1156,    346,   1785,    175,  -2828,
    -1575,   5892,  13512,  13512,   5892,  -1575,  -2828,    175,   1785,
      346,  -1156,   -560,    704,    613,   -368,   -571,    127,    475,
       33,   -356,   -122,    236,    155,   -133
};
const q15_t pasabajo_22k[58] = {
     -126,      0,    157,    180,     13,   -206,   -250,    -34,    263,
      340,     68,   -332,   -460,   -120,    420,    626,    202,   -540,
     -876,   -338,    725,   1304,    600,  -1082,  -2263,  -1298,   2240,
     6928,  10244,  10244,   6928,   2240,  -1298,  -2263,  -1082,    600,
     1304,    725,   -338,   -876,   -540,    202,    626,    420,   -120,
     -460,   -332,     68,    340,    263,    -34,   -250,   -206,     13,
      180,    157,      0,   -126
};
const q15_t pasabajo_44k[114] = {
      -52,    -20,     21,     61,     89,     95,     76,     33,    -23,
      -78,   -118,   -130,   -107,    -53,     22,     98,    155,    175,
      149,     80,    -17,   -120,   -200,   -233,   -206,   -120,      8,
      146,    259,    313,    287,    179,     10,   -180,   -343,   -431,
     -410,   -274,    -44,    229,    477,    628,    627,    449,    114,
     -317,   -745,  -1053,  -1130,   -894,   -314,    579,   1691,   2879,
     3976,   4818,   5275,   5275,   4818,   3976,   2879,   1691,    579,
     -314,   -894,  -1130,  -1053,   -745,   -317,    114,    449,    627,
      628,    477,    229,    -44,   -274,   -410,   -431,   -343,   -180,
       10,    179,    287,    313,    259,    146,      8,   -120,   -206,
     -233,   -200,   -120,    -17,     80,    149,    175,    155,     98,
       22,    -53,   -107,   -130,   -118,    -78,    -23,     33,     76,
       95,     89,     61,     21,    -20,    -52
};
// Pasabajo: fc = 3,3 kHz
const q15_t pasabajo_48k[124] = {
      -44,    -17,     18,     52,     77,     87,     78,     50,      8,
      -41,    -84,   -113,   -118,    -97,    -51,     11,     76,    129,
      158,    153,    112,     42,    -44,   -128,   -190,   -214,   -191,
     -122,    -19,     98,    204,    272,    285,    234,    123,    -27,
     -186,   -319,   -393,   -384,   -285,   -108,    115,    340,    514,
      593,    543,    358,     58,   -308,   -668,   -939,  -1042,   -914,
     -523,    124,    979,   1957,   2946,   3827,   4488,   4841,   4841,
     4488,   3827,   2946,   1957,    979,    124,   -523,   -914,  -1042,
     -939,   -668,   -308,     58,    358,    543,    593,    514,    340,
      115,   -108,   -285,   -384,   -393,   -319,   -186,    -27,    123,
      234,    285,    272,    204,     98,    -19,   -122,   -191,   -214,
     -190,   -128,    -44,     42,    112,    153,    158,    129,     76,
       11,    -51,    -97,   -118,   -113,    -84,    -41,      8,     50,
       78,     87,     77,     52,     18,    -17,    -44
};

SDK_ALIGN(q15_t state[INPUT_SIZE + TAPS_SIZE - 1], 8);

SDK_ALIGN(q15_t adc_buffer[BUFFER_SIZE], 8);
SDK_ALIGN(q15_t dac_buffer_q15[BUFFER_SIZE], 8);

ctimer_match_config_t ctimerMatchConfig = {
  .matchValue = 749,
  .enableCounterReset = true,
  .enableCounterStop = false,
  .outControl = kCTIMER_Output_Toggle,
  .outPinInitState = false,
  .enableInterrupt = false
};

lpadc_conv_result_t mLpadc_resultConfigStruct;

// ---- LED RGB según frecuencia ----
static void UpdateLedColor(uint8_t idx)
{
	switch(idx) {
		case 0: LED_SetColor(true, false, false); break; // rojo
		case 1: LED_SetColor(false, true, false); break; // verde
		case 2: LED_SetColor(false, false, true); break; // azul
		case 3: LED_SetColor(true, true, false); break;  // amarillo
		case 4: LED_SetColor(true, false, true); break;  // magenta
		default: LED_SetColor(false, false, false); break; // apagado
	}
}

static void LED_SetColor(bool RED, bool GREEN, bool BLUE)
{
    GPIO_PinWrite(BOARD_LED_RED_GPIO, BOARD_LED_RED_PIN, RED ? 1 : 0);
    GPIO_PinWrite(BOARD_LED_GREEN_GPIO, BOARD_LED_GREEN_PIN, GREEN ? 1 : 0);
    GPIO_PinWrite(BOARD_LED_BLUE_GPIO, BOARD_LED_BLUE_PIN, BLUE ? 1 : 0);
}

/* ADC0_IRQn interrupt handler */
void ADC0_IRQHANDLER(void) {
	uint32_t trigger_status_flag;
	uint32_t status_flag;
	uint16_t adc_val;

	/* Trigger interrupt flags */
	trigger_status_flag = LPADC_GetTriggerStatusFlags(ADC0_PERIPHERAL);
	/* Interrupt flags */
	status_flag = LPADC_GetStatusFlags(ADC0_PERIPHERAL);
	/* Clears trigger interrupt flags */
	LPADC_ClearTriggerStatusFlags(ADC0_PERIPHERAL, trigger_status_flag);
	/* Clears interrupt flags */
	LPADC_ClearStatusFlags(ADC0_PERIPHERAL, status_flag);

	LPADC_GetConvResult(ADC0_PERIPHERAL, &mLpadc_resultConfigStruct, 0);
	adc_val= mLpadc_resultConfigStruct.convValue;

//	Conversión a Q15
//	ADC: [0,65536] Q15: [-32768,32767]
	adc_buffer[adc_buf_index] = (q15_t)(adc_val - 32768U);

	if(filter_run){
	    dac_val = (uint16_t)(dac_buffer_q15[adc_buf_index] + 32768U) >> 4;
	}
	else{
		dac_val = adc_val >> 4;
	}

	// Buffer circular
	adc_buf_index = (adc_buf_index + 1) % BUFFER_SIZE;

//	Filtrar
	if(filter_run && adc_buf_index >= (BUFFER_SIZE-1)){
//	    arm_fir_q15(&fir, aux_adc_buffer, aux_dac_buffer, INPUT_SIZE);
	    arm_fir_init_q15(&fir[0][0], TAPS_SIZE, pasabajo_48k, state, BUFFER_SIZE);
	    arm_fir_q15(&fir[0][0], &adc_buffer[0], &dac_buffer_q15[0], BUFFER_SIZE);
	}

//  Enviar al DAC
    DAC_SetData(DAC0, dac_val);
}

// ---- ISR Botón SW2 (Run/Stop) ----
void GPIO0_INT_0_IRQHANDLER(void)
{
    uint32_t flags = GPIO_GpioGetInterruptChannelFlags(GPIO0, 0U);
    GPIO_GpioClearInterruptChannelFlags(GPIO0, flags, 0U);

    if (flags & (1U << SW2_PIN)) {
        filter_run = !filter_run;
        if (filter_run) {
            PRINTF("FILTER RUN\r\n");
        } else {
            PRINTF("FILTER STOP\r\n");
        }
    }
}

// ---- ISR Botón SW3 (cambio frecuencia) ----
void GPIO0_INT_1_IRQHANDLER(void)
{
    uint32_t flags = GPIO_GpioGetInterruptChannelFlags(GPIO0, 1U);
    GPIO_GpioClearInterruptChannelFlags(GPIO0, flags, 1U);

    CTIMER_StopTimer(CTIMER0);
    if (flags & (1U << SW3_PIN)) {
        sample_rate_idx = (sample_rate_idx + 1) % NUM_RATES;

        // matchValue depende de la frecuencia de muestreo
        ctimerMatchConfig.matchValue = CTIMER_CLK_FREQ / sample_rates[sample_rate_idx];

        CTIMER_SetupMatch(CTIMER0, kCTIMER_Match_3, &ctimerMatchConfig);

        UpdateLedColor(sample_rate_idx);
        PRINTF("Nueva frecuencia: %d Hz\r\n", sample_rates[sample_rate_idx]);
    }
    CTIMER_Reset(CTIMER0);
    CTIMER_StartTimer(CTIMER0);
}

/*******************************************************************************
 * Main
 ******************************************************************************/
int main(void)
{
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
    BOARD_InitDebugConsole();

    PRINTF("Filtro FIR pasa bajos\r\n");

    UpdateLedColor(sample_rate_idx);

    CTIMER_StartTimer(CTIMER0);

    while (1) {
        __WFI();
    }
}
