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
#include "fsl_lpuart.h"
#include "math.h"

/*******************************************************************************
 * Definiciones
 ******************************************************************************/

// Frecuencias de muestreo requeridas
const uint32_t sample_rates[] = {8000, 16000, 22000, 44000, 48000}; // frecuencias de muestreo

#define NUM_RATES (sizeof(sample_rates)/sizeof(sample_rates[0])) // tamaño del arreglo
// NUM_RATES se define de esta forma para poder obtener el tamaño del arreglo sin importar el
// tipo de numero (en este caso uint32_t) ni la cantidad de numeros del arreglo (en este caso 5)

// Pines de botones (ajustar según board)
#define SW2_PIN   23   // PIO0_23
#define SW3_PIN   6   // PIO0_6

// Frecuencia del CTIMER
#define CTIMER_CLK_FREQ 600000 // 1.2 MHz

// Tamaños
#define BUFFER_SIZE_MAX   (uint32_t)512	// buffer de datos. Powerquad solo soporta hasta 4096?
#define BLOCK_SIZE 	  BUFFER_SIZE_MAX/2

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
//static void InitFIRPointers(void);
//static void UpdateFIRPointers(uint8_t idx);

/*******************************************************************************
 * Variables globales
 ******************************************************************************/
void * data_ptr; // DMA channel DMA0_CH0 user data

bool filter_run = true; // prender/apagar filtro
bool process_half_A = false; // primera mitad llena (buffer)
bool process_half_B = false; // segunda mitad llena (buffer)

uint8_t sample_rate_idx = 0; // indice para elegir frecuencia de muestreo

uint16_t adc_buf_index = 0; // indice para recorrer el buffer circular de entrada
uint16_t dac_val; // valor hacia el DAC

arm_rfft_instance_q15 fft;

SDK_ALIGN(q15_t adc_buffer[BUFFER_SIZE_MAX], 8); // Buffer donde se guardan las conversiones del ADC
SDK_ALIGN(q15_t dac_buffer_q15[BUFFER_SIZE_MAX * 2], 8); // Buffer de los datos procesados. Se manda al DAC
SDK_ALIGN(q15_t dac_buffer_abs_q15[BUFFER_SIZE_MAX * 2], 8);

// Estructura del timer
ctimer_match_config_t ctimerMatchConfig = {
  .matchValue = 749,
  .enableCounterReset = true,
  .enableCounterStop = false,
  .outControl = kCTIMER_Output_Toggle,
  .outPinInitState = false,
  .enableInterrupt = false
};

// Estructura resultado del ADC
lpadc_conv_result_t mLpadc_resultConfigStruct;
//
//static void InitFIRPointers(void){
//	switch(fir_type_idx){
//	    case 0:
//	    	fir_coef_8k_ptr = pasabajo_8k;
//			state_8k_ptr 	= state_pasabajo_8k;
//
//	    	fir_coef_16k_ptr = pasabajo_16k;
//			state_16k_ptr 	= state_pasabajo_16k;
//
//	    	fir_coef_22k_ptr = pasabajo_22k;
//			state_22k_ptr 	= state_pasabajo_22k;
//
//	    	fir_coef_44k_ptr = pasabajo_44k;
//			state_44k_ptr 	= state_pasabajo_44k;
//
//	    	fir_coef_48k_ptr = pasabajo_48k;
//			state_48k_ptr 	= state_pasabajo_48k;
//
//		    PRINTF("Filtro FIR pasa bajos\r\n");
//			break;
//
//	    case 1:
//	    	fir_coef_8k_ptr = pasaalto_8k;
//			state_8k_ptr 	= state_pasaalto_8k;
//
//	    	fir_coef_16k_ptr = pasaalto_16k;
//			state_16k_ptr 	= state_pasaalto_16k;
//
//	    	fir_coef_22k_ptr = pasaalto_22k;
//			state_22k_ptr 	= state_pasaalto_22k;
//
//	    	fir_coef_44k_ptr = pasaalto_44k;
//			state_44k_ptr 	= state_pasaalto_44k;
//
//	    	fir_coef_48k_ptr = pasaalto_48k;
//			state_48k_ptr 	= state_pasaalto_48k;
//
//		    PRINTF("Filtro FIR pasa altos\r\n");
//			break;
//
//	    case 2:
//	    	fir_coef_8k_ptr = pasabanda_8k;
//			state_8k_ptr 	= state_pasabanda_8k;
//
//	    	fir_coef_16k_ptr = pasabanda_16k;
//			state_16k_ptr 	= state_pasabanda_16k;
//
//	    	fir_coef_22k_ptr = pasabanda_22k;
//			state_22k_ptr 	= state_pasabanda_22k;
//
//	    	fir_coef_44k_ptr = pasabanda_44k;
//			state_44k_ptr 	= state_pasabanda_44k;
//
//	    	fir_coef_48k_ptr = pasabanda_48k;
//			state_48k_ptr 	= state_pasabanda_48k;
//
//		    PRINTF("Filtro FIR pasa banda\r\n");
//			break;
//
//	    case 3:
//	    	fir_coef_8k_ptr = rechazabanda2_8k;
//			state_8k_ptr 	= state_rechazabanda2_8k;
//
//	    	fir_coef_16k_ptr = rechazabanda2_16k;
//			state_16k_ptr 	= state_rechazabanda2_16k;
//
//	    	fir_coef_22k_ptr = rechazabanda2_22k;
//			state_22k_ptr 	= state_rechazabanda2_22k;
//
//	    	fir_coef_44k_ptr = rechazabanda2_44k;
//			state_44k_ptr 	= state_rechazabanda2_44k;
//
//	    	fir_coef_48k_ptr = rechazabanda2_48k;
//			state_48k_ptr 	= state_rechazabanda2_48k;
//
//		    PRINTF("Filtro FIR rechaza banda\r\n");
//			break;
//
//	    default:
//	    	fir_coef_8k_ptr = pasabajo_8k;
//			state_8k_ptr 	= state_pasabajo_8k;
//
//	    	fir_coef_16k_ptr = pasabajo_16k;
//			state_16k_ptr 	= state_pasabajo_16k;
//
//	    	fir_coef_22k_ptr = pasabajo_22k;
//			state_22k_ptr 	= state_pasabajo_22k;
//
//	    	fir_coef_44k_ptr = pasabajo_44k;
//			state_44k_ptr 	= state_pasabajo_44k;
//
//	    	fir_coef_48k_ptr = pasabajo_48k;
//			state_48k_ptr 	= state_pasabajo_48k;
//
//		    PRINTF("Filtro FIR pasa bajos.\r\n");
//			break;
//	    }
//}
//
//static void UpdateFIRPointers(uint8_t idx){
//	switch(idx){
//		case 0:
//			fir_coef_ptr = fir_coef_8k_ptr;
//			state_ptr = state_8k_ptr;
//			break;
//
//		case 1:
//			fir_coef_ptr = fir_coef_16k_ptr;
//			state_ptr = state_16k_ptr;
//			break;
//
//		case 2:
//			fir_coef_ptr = fir_coef_22k_ptr;
//			state_ptr = state_22k_ptr;
//			break;
//
//		case 3:
//			fir_coef_ptr = fir_coef_44k_ptr;
//			state_ptr = state_44k_ptr;
//			break;
//
//		case 4:
//			fir_coef_ptr = fir_coef_48k_ptr;
//			state_ptr = state_48k_ptr;
//			break;
//
//		default:
//			break;
//		}
//}

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

//	if(filter_run){
//		// Enviar datos de la filtracion anterior
////	    dac_val = (adc_buf_index != 0) ? (uint16_t)( abs(dac_buffer_q15[adc_buf_index]) + 32768U) >> 4 : 0U;
//	    dac_val = (uint16_t)( (dac_buffer_q15[2 * adc_buf_index] + 32768U) >> 4 );
//	}
//	else{
//		// Bypass al filtro
//		dac_val = adc_val >> 4;
//	}

	// Buffer circular
	adc_buf_index = adc_buf_index + 1U;

	if(adc_buf_index >= BUFFER_SIZE_MAX){
		process_half_A = true;
		adc_buf_index = 0U;
	}

//  Enviar al DAC
//    DAC_SetData(DAC0, dac_val);
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

//         Actualizar filtro
//        UpdateFIRPointers(sample_rate_idx);

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

//    InitFIRPointers();
//    UpdateFIRPointers(sample_rate_idx);
//    arm_rfft_init_q15(&fft, BUFFER_SIZE_MAX, 0, 1);

    UpdateLedColor(sample_rate_idx);

    CTIMER_StartTimer(CTIMER0);


    while (1) {
//        __WFI();

//        if(process_half_A){
//        	process_half_A = false;
//
//        	if(filter_run){
//        	    arm_rfft_init_q15(&fft, BUFFER_SIZE_MAX/2, 0, 1);
//        		arm_rfft_q15(&fft, &adc_buffer[0], &dac_buffer_q15[0]);
//        	}
//        }
//        if(process_half_B){
//        	process_half_B = false;
//
//        	if(filter_run){
//        	    arm_rfft_init_q15(&fft, BUFFER_SIZE_MAX/2, 0, 1);
//        		arm_rfft_q15(&fft, &adc_buffer[0] + BLOCK_SIZE, &dac_buffer_q15[0] + BLOCK_SIZE);
//        	}
//        }
        if(process_half_A){
        	process_half_A = false;

        	if(filter_run){
//        	    arm_rfft_init_q15(&fft, BUFFER_SIZE_MAX, 0, 1);
//        		arm_rfft_q15(&fft, &adc_buffer[0], &dac_buffer_q15[0]);
        	    PQ_TransformRFFT(POWERQUAD, BUFFER_SIZE_MAX, &adc_buffer[0], &dac_buffer_q15[0]);
        	    PQ_WaitDone(POWERQUAD);


        	    for(int i=0; i < BUFFER_SIZE_MAX; i++){
        	    	dac_buffer_abs_q15[i] = (sqrtf( dac_buffer_q15[2 * i]*dac_buffer_q15[2 * i] + dac_buffer_q15[2 * i + 1]*dac_buffer_q15[2 * i + 1] ) / 2);
        	    	PRINTF("%d, %d, 0, %d\r", dac_buffer_q15[2 * i] + 32768U, dac_buffer_q15[2 * i + 1] + 32768U, dac_buffer_abs_q15[i], dac_buffer_abs_q15[i]);
        	    }
        	}
        }
    }
}


