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

#include "Coeficientes_PB_8k.h"		// Pasabajo: fc = 3,65 kHz
#include "Coeficientes_PB_16k.h"	// Pasabajo: fc = 3,45 kHz
#include "Coeficientes_PB_22k.h"	// Pasabajo: fc = 3,4 kHz
#include "Coeficientes_PB_44k.h"	// Pasabajo: fc = 3,45 kHz
#include "Coeficientes_PB_48k.h"	// Pasabajo: fc = 3,24 kHz

#include "Coeficientes_PA_8k.h"		// Pasaalto: fc = 35 Hz todos
#include "Coeficientes_PA_16k.h"
#include "Coeficientes_PA_22k.h"
#include "Coeficientes_PA_44k.h"
#include "Coeficientes_PA_48k.h"

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
#define TAPS_SIZE	  (uint16_t)32    // orden del filtro

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
uint8_t i_tipo_fir = 0; // indice del tipo de filtro actual
uint8_t i_fs = 0; // indice de la frecuencia de muestreo actual

// arreglo de tamaños de filtro (4 tipos de filtro, 5 frecuencias de muestreo)
uint16_t taps_size[4][5] = {
		{pasabajo_8k_length, pasabajo_16k_length, pasabajo_22k_length, pasabajo_44k_length, pasabajo_48k_length},
		{pasaalto_8k_length, pasaalto_16k_length, pasaalto_22k_length, pasaalto_44k_length, pasaalto_48k_length},
		{0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0}
};

uint16_t adc_buf_index = 0; // indice para recorrer el buffer circular de entrada
uint16_t dac_val; // valor hacia el DAC

arm_fir_instance_q15 fir;

const q15_t* fir_coef_ptr; // aca se guarda el puntero del filtro actual

q15_t* state_ptr;

SDK_ALIGN(q15_t state_pasabajo_8k[BUFFER_SIZE + 32 - 1], 8);
SDK_ALIGN(q15_t state_pasabajo_16k[BUFFER_SIZE + 42 - 1], 8);
SDK_ALIGN(q15_t state_pasabajo_22k[BUFFER_SIZE + 58 - 1], 8);
SDK_ALIGN(q15_t state_pasabajo_44k[BUFFER_SIZE + 114 - 1], 8);
SDK_ALIGN(q15_t state_pasabajo_48k[BUFFER_SIZE + 124 - 1], 8);

SDK_ALIGN(q15_t state_pasaalto_8k[BUFFER_SIZE + 225 - 1], 8);
SDK_ALIGN(q15_t state_pasaalto_16k[BUFFER_SIZE + 449 - 1], 8);
SDK_ALIGN(q15_t state_pasaalto_22k[BUFFER_SIZE + 617 - 1], 8);
SDK_ALIGN(q15_t state_pasaalto_44k[BUFFER_SIZE + 1231 - 1], 8);
SDK_ALIGN(q15_t state_pasaalto_48k[BUFFER_SIZE + 1343 - 1], 8);

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
	    arm_fir_init_q15(&fir, taps_size[i_tipo_fir][i_fs], fir_coef_ptr, state_ptr, BUFFER_SIZE);
	    arm_fir_q15(&fir, &adc_buffer[0], &dac_buffer_q15[0], BUFFER_SIZE);
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

        // Actualizar filtro
        i_fs = sample_rate_idx;

        switch(sample_rate_idx){
        case 0:
            fir_coef_ptr = pasaalto_8k;
            state_ptr = state_pasaalto_8k;
        	break;

        case 1:
            fir_coef_ptr = pasaalto_16k;
            state_ptr = state_pasaalto_16k;
        	break;

        case 2:
            fir_coef_ptr = pasaalto_22k;
            state_ptr = state_pasaalto_22k;
        	break;

        case 3:
            fir_coef_ptr = pasaalto_44k;
            state_ptr = state_pasaalto_44k;
        	break;

        case 4:
            fir_coef_ptr = pasaalto_48k;
            state_ptr = state_pasaalto_48k;
        	break;

        default:
        	break;
        }

        // Actualizar timer
        // matchValue depende de la frecuencia de muestreo
        ctimerMatchConfig.matchValue = CTIMER_CLK_FREQ / sample_rates[sample_rate_idx];

        CTIMER_SetupMatch(CTIMER0, kCTIMER_Match_3, &ctimerMatchConfig);

        // Atualizar LED
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

    fir_coef_ptr = pasaalto_8k;
    state_ptr = state_pasaalto_8k;
    i_tipo_fir = 1;

    UpdateLedColor(sample_rate_idx);

    CTIMER_StartTimer(CTIMER0);

    while (1) {
        __WFI();
    }
}
