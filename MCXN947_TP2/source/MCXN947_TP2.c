/*
 Trabajo Practico 2: Filtro FIR

 PINES:
 	 ADC0: J4.2
 	 DAC0: J1.4
 	 DAC1: J1.2
 	 MATCH0: J7.1 o J2.13
 	 GND: J5.8 o J6.8

 PARA SELECCIONAR EL TIPO DE FILTRO CAMBIAR fir_type_idx:
 *  0: Pasa bajo
 * 	1: Pasa alto
 * 	2: Pasa banda
 * 	3: Rechaza banda
 */
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "fsl_ctimer.h"
#include "fsl_dac.h"
#include "fsl_lpadc.h"
#include "fsl_cache.h"
#include "arm_math.h"

#include "Coeficientes_PB_8k.h"		// Pasabajo: fc = 3,65 kHz
#include "Coeficientes_PB_16k.h"	// Pasabajo: fc = 3,45 kHz
#include "Coeficientes_PB_22k.h"	// Pasabajo: fc = 3,4 kHz
#include "Coeficientes_PB_44k.h"	// Pasabajo: fc = 3,45 kHz
#include "Coeficientes_PB_48k.h"	// Pasabajo: fc = 3,24 kHz

#include "Coeficientes_PA_8k.h"		// Pasaalto: fc = 35 Hz
#include "Coeficientes_PA_16k.h"	// Pasaalto: fc = 35 Hz
#include "Coeficientes_PA_22k.h"	// Pasaalto: fc = 35 Hz
#include "Coeficientes_PA_44k.h"	// Pasaalto: fc = 35 Hz
#include "Coeficientes_PA_48k.h"	// Pasaalto: fc = 35 Hz

#include "Coeficientes_PBDA_8k.h"	// Pasabanda: fpaso = 35-3540 Hz
#include "Coeficientes_PBDA_16k.h"	// Pasabanda: fpaso = 35-3540 Hz
#include "Coeficientes_PBDA_22k.h"	// Pasabanda: fpaso = 35-3490 Hz
#include "Coeficientes_PBDA_44k.h"	// Pasabanda: fpaso = 35-3490 Hz
#include "Coeficientes_PBDA_48k.h"	// Pasabanda: fpaso = 35-3450 Hz

#include "Coeficientes_RBDA2_8k.h"	// Rechazabanda: frechazo = 25-73 Hz -24db
#include "Coeficientes_RBDA2_16k.h"	// Rechazabanda: frechazo = 25-74 Hz -23db
#include "Coeficientes_RBDA2_22k.h"	// Rechazabanda: frechazo = 25-73 Hz -23db
#include "Coeficientes_RBDA2_44k.h"	// Rechazabanda: frechazo = 25-72 Hz -23db
#include "Coeficientes_RBDA2_48k.h"	// Rechazabanda: frechazo = 25-72 Hz -23db

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
#define BUFFER_SIZE   (uint32_t)2048	// buffer de datos
#define BLOCK_SIZE 	  (uint32_t)4

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
static void InitFIRPointers(void);
static void UpdateFIRPointers(uint8_t idx);

/*******************************************************************************
 * Variables globales
 ******************************************************************************/
void * data_ptr; // DMA channel DMA0_CH0 user data

bool filter_run = true; // prender/apagar filtro
bool process_half_A = false; // primera mitad llena (buffer)
bool process_half_B = false; // segunda mitad llena (buffer)

uint8_t sample_rate_idx = 0; // indice para elegir frecuencia de muestreo

/*
 * Indice del tipo de filtro actual:
 * 	0: Pasa bajo
 * 	1: Pasa alto
 * 	2: Pasa banda
 * 	3: Rechaza banda
 */
uint8_t fir_type_idx = 1;

// arreglo de tamaños de filtro (4 tipos de filtro, 5 frecuencias de muestreo)
uint16_t taps_size_v[4][5] = {
		{pasabajo_8k_length, pasabajo_16k_length, pasabajo_22k_length, pasabajo_44k_length, pasabajo_48k_length},
		{pasaalto_8k_length, pasaalto_16k_length, pasaalto_22k_length, pasaalto_44k_length, pasaalto_48k_length},
		{pasabanda_8k_length, pasabanda_16k_length, pasabanda_22k_length, pasabanda_44k_length, pasabanda_48k_length},
		{rechazabanda2_8k_length, rechazabanda2_16k_length, rechazabanda2_22k_length, rechazabanda2_44k_length, rechazabanda2_48k_length}
};
uint16_t taps_size; // tamaño del filtro actual

uint16_t adc_buf_index = 0; // indice para recorrer el buffer circular de entrada

uint16_t transient_idx[5] = {16, 21, 28, 60, 60}; // Indice para saltearse el transitorio

uint16_t dac_val; // valor hacia el DAC

arm_fir_instance_q15 fir;

const q15_t* fir_coef_ptr; // puntero del filtro actual
const q15_t* fir_coef_8k_ptr;
const q15_t* fir_coef_16k_ptr;
const q15_t* fir_coef_22k_ptr;
const q15_t* fir_coef_44k_ptr;
const q15_t* fir_coef_48k_ptr;

q15_t* state_ptr; // puntero del state actual
q15_t* state_8k_ptr;
q15_t* state_16k_ptr;
q15_t* state_22k_ptr;
q15_t* state_44k_ptr;
q15_t* state_48k_ptr;

// state de todos los filtros
// Segun chatGPT: El coprocesador PowerQuad exige que los state, buffers y taps comiencen en direcciones de memoria multiplos de 8.
// Por esto se coloca la macro SDK_ALIGN
SDK_ALIGN(q15_t state_pasabajo_8k[BLOCK_SIZE + 32 - 1], 8);
SDK_ALIGN(q15_t state_pasabajo_16k[BLOCK_SIZE + 42 - 1], 8);
SDK_ALIGN(q15_t state_pasabajo_22k[BLOCK_SIZE + 58 - 1], 8);
SDK_ALIGN(q15_t state_pasabajo_44k[BLOCK_SIZE + 114 - 1], 8);
SDK_ALIGN(q15_t state_pasabajo_48k[BLOCK_SIZE + 124 - 1], 8);

SDK_ALIGN(q15_t state_pasaalto_8k[BLOCK_SIZE + 225 - 1], 8);
SDK_ALIGN(q15_t state_pasaalto_16k[BLOCK_SIZE + 449 - 1], 8);
SDK_ALIGN(q15_t state_pasaalto_22k[BLOCK_SIZE + 617 - 1], 8);
SDK_ALIGN(q15_t state_pasaalto_44k[BLOCK_SIZE + 1231 - 1], 8);
SDK_ALIGN(q15_t state_pasaalto_48k[BLOCK_SIZE + 1343 - 1], 8);

SDK_ALIGN(q15_t state_pasabanda_8k[BLOCK_SIZE + 225 - 1], 8);
SDK_ALIGN(q15_t state_pasabanda_16k[BLOCK_SIZE + 448 - 1], 8);
SDK_ALIGN(q15_t state_pasabanda_22k[BLOCK_SIZE + 616 - 1], 8);
SDK_ALIGN(q15_t state_pasabanda_44k[BLOCK_SIZE + 1230 - 1], 8);
SDK_ALIGN(q15_t state_pasabanda_48k[BLOCK_SIZE + 1342 - 1], 8);

SDK_ALIGN(q15_t state_rechazabanda2_8k[BLOCK_SIZE + 305 - 1], 8);
SDK_ALIGN(q15_t state_rechazabanda2_16k[BLOCK_SIZE + 607 - 1], 8);
SDK_ALIGN(q15_t state_rechazabanda2_22k[BLOCK_SIZE + 835 - 1], 8);
SDK_ALIGN(q15_t state_rechazabanda2_44k[BLOCK_SIZE + 1667 - 1], 8);
SDK_ALIGN(q15_t state_rechazabanda2_48k[BLOCK_SIZE + 1819 - 1], 8);

SDK_ALIGN(q15_t adc_buffer[BUFFER_SIZE], 8); // Buffer donde se guardan las conversiones del ADC
SDK_ALIGN(q15_t dac_buffer_q15[BUFFER_SIZE], 8); // Buffer de los datos procesados. Se manda al DAC

pq_config_t PQ_config = {
  .inputAFormat = kPQ_16Bit,
  .inputAPrescale = 0,
  .inputBFormat = kPQ_16Bit,
  .inputBPrescale = 0,
  .outputFormat = kPQ_16Bit,
  .outputPrescale = -15,
  .tmpFormat = kPQ_Float,
  .tmpPrescale = 0,
  .machineFormat = kPQ_Float,
  .tmpBase = (uint32_t *)0xE0000000UL
};
/*
 * arm_fir_q15:
 * 	outputFormat: Q15
 * 	outputPrescale: -15
 * 	inputAFormat: Q15
 * 	inputAPrescale: 0
 * 	inputBFormat: Q15
 * 	inputBPrescale: 0
 * 	tmpFormat: Float
 * 	tmpPrescale: 0
 * 	tmpBase: 0xE0000000U
 */

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

static void InitFIRPointers(void){
	switch(fir_type_idx){
	    case 0:
	    	fir_coef_8k_ptr = pasabajo_8k;
			state_8k_ptr 	= state_pasabajo_8k;

	    	fir_coef_16k_ptr = pasabajo_16k;
			state_16k_ptr 	= state_pasabajo_16k;

	    	fir_coef_22k_ptr = pasabajo_22k;
			state_22k_ptr 	= state_pasabajo_22k;

	    	fir_coef_44k_ptr = pasabajo_44k;
			state_44k_ptr 	= state_pasabajo_44k;

	    	fir_coef_48k_ptr = pasabajo_48k;
			state_48k_ptr 	= state_pasabajo_48k;

		    PRINTF("Filtro FIR pasa bajos\r\n");
			break;

	    case 1:
	    	fir_coef_8k_ptr = pasaalto_8k;
			state_8k_ptr 	= state_pasaalto_8k;

	    	fir_coef_16k_ptr = pasaalto_16k;
			state_16k_ptr 	= state_pasaalto_16k;

	    	fir_coef_22k_ptr = pasaalto_22k;
			state_22k_ptr 	= state_pasaalto_22k;

	    	fir_coef_44k_ptr = pasaalto_44k;
			state_44k_ptr 	= state_pasaalto_44k;

	    	fir_coef_48k_ptr = pasaalto_48k;
			state_48k_ptr 	= state_pasaalto_48k;

		    PRINTF("Filtro FIR pasa altos\r\n");
			break;

	    case 2:
	    	fir_coef_8k_ptr = pasabanda_8k;
			state_8k_ptr 	= state_pasabanda_8k;

	    	fir_coef_16k_ptr = pasabanda_16k;
			state_16k_ptr 	= state_pasabanda_16k;

	    	fir_coef_22k_ptr = pasabanda_22k;
			state_22k_ptr 	= state_pasabanda_22k;

	    	fir_coef_44k_ptr = pasabanda_44k;
			state_44k_ptr 	= state_pasabanda_44k;

	    	fir_coef_48k_ptr = pasabanda_48k;
			state_48k_ptr 	= state_pasabanda_48k;

		    PRINTF("Filtro FIR pasa banda\r\n");
			break;

	    case 3:
	    	fir_coef_8k_ptr = rechazabanda2_8k;
			state_8k_ptr 	= state_rechazabanda2_8k;

	    	fir_coef_16k_ptr = rechazabanda2_16k;
			state_16k_ptr 	= state_rechazabanda2_16k;

	    	fir_coef_22k_ptr = rechazabanda2_22k;
			state_22k_ptr 	= state_rechazabanda2_22k;

	    	fir_coef_44k_ptr = rechazabanda2_44k;
			state_44k_ptr 	= state_rechazabanda2_44k;

	    	fir_coef_48k_ptr = rechazabanda2_48k;
			state_48k_ptr 	= state_rechazabanda2_48k;

		    PRINTF("Filtro FIR rechaza banda\r\n");
			break;

	    default:
	    	fir_coef_8k_ptr = pasabajo_8k;
			state_8k_ptr 	= state_pasabajo_8k;

	    	fir_coef_16k_ptr = pasabajo_16k;
			state_16k_ptr 	= state_pasabajo_16k;

	    	fir_coef_22k_ptr = pasabajo_22k;
			state_22k_ptr 	= state_pasabajo_22k;

	    	fir_coef_44k_ptr = pasabajo_44k;
			state_44k_ptr 	= state_pasabajo_44k;

	    	fir_coef_48k_ptr = pasabajo_48k;
			state_48k_ptr 	= state_pasabajo_48k;

		    PRINTF("Filtro FIR pasa bajos.\r\n");
			break;
	    }
}

static void UpdateFIRPointers(uint8_t idx){
	taps_size = taps_size_v[fir_type_idx][sample_rate_idx];

	switch(idx){
		case 0:
			fir_coef_ptr = fir_coef_8k_ptr;
			state_ptr = state_8k_ptr;
			break;

		case 1:
			fir_coef_ptr = fir_coef_16k_ptr;
			state_ptr = state_16k_ptr;
			break;

		case 2:
			fir_coef_ptr = fir_coef_22k_ptr;
			state_ptr = state_22k_ptr;
			break;

		case 3:
			fir_coef_ptr = fir_coef_44k_ptr;
			state_ptr = state_44k_ptr;
			break;

		case 4:
			fir_coef_ptr = fir_coef_48k_ptr;
			state_ptr = state_48k_ptr;
			break;

		default:
			break;
		}
}

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

void Invalidate_Cache_Safe(void *addr, uint32_t size) {
    uint32_t startAddr = (uint32_t)addr;
    uint32_t endAddr = startAddr + size;

    // Alinear el inicio hacia abajo (AND con máscara)

    uint32_t alignedStart = startAddr & ~(CACHE64_LINESIZE_BYTE - 1);

    // Alinear el final hacia arriba
    uint32_t alignedEnd = (endAddr + CACHE64_LINESIZE_BYTE - 1) & ~(CACHE64_LINESIZE_BYTE - 1);

    // Ejecutar invalidación sobre el bloque alineado
    DCACHE_InvalidateByRange(alignedStart, alignedEnd - alignedStart);
}

/* ADC0_IRQn interrupt handler */
void ADC0_IRQHANDLER(void) {
	uint32_t trigger_status_flag;
	uint32_t status_flag;
	uint16_t adc_val;

	static int buffer_laps = 0;
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
		// Enviar datos de la filtracion anterior
		dac_val = (uint16_t)(dac_buffer_q15[adc_buf_index] + 32768U) >> 4;

		if(adc_buf_index % 4 == 3){
			// --- PASO 1: CONSTRUIR BUFFER LINEAL PARA POWERQUAD ---

			// A. Copiar la HISTORIA (muestras viejas)
			// Necesitamos 'taps_size' muestras anteriores a la actual
			int history_start_idx = (int)adc_buf_index - 3 - (int)taps_size; // -3 porque estamos al final del bloque

			// Manejo del buffer circular para la historia
			int samples_from_end = 0;
			int samples_from_start = 0;
			static q15_t fir_temp_buffer[BUFFER_SIZE] = {0};

			if (history_start_idx < 0) {
				// La historia está partida: parte al final del buffer, parte al inicio
				history_start_idx += BUFFER_SIZE;
				samples_from_end = (int)BUFFER_SIZE - history_start_idx;

				// Copiar parte final del buffer circular al inicio del temporal
				memcpy(&fir_temp_buffer[0], &adc_buffer[history_start_idx], samples_from_end * sizeof(q15_t));

				// El resto (si falta) viene del principio del buffer circular
				samples_from_start = (int)taps_size - samples_from_end;
				if(samples_from_start > 0){
					memcpy(&fir_temp_buffer[samples_from_end], &adc_buffer[0], samples_from_start * sizeof(q15_t));
				}
			} else {
				// La historia es contigua
				memcpy(&fir_temp_buffer[0], &adc_buffer[history_start_idx], taps_size * sizeof(q15_t));
			}

			// B. Copiar el BLOQUE NUEVO (las 4 muestras actuales)
			// Las ponemos justo después de la historia en el buffer temporal
			// Como acabamos de escribir adc_buffer[idx-3]...[idx], son contiguas si no hubo wrap justo en el bloque
			// (Si BUFFER_SIZE es múltiplo de 4, el bloque de 4 nunca se parte, así que es seguro copiar directo)

			// Nota: Si adc_buf_index acaba de dar la vuelta (ej. idx=3 y buffer_size=4),
			// las muestras 0,1,2,3 están al principio.
			// Simplemente copiamos las 4 muestras actuales al final del temp.
			int block_start = adc_buf_index - 3;
			if (block_start < 0){
				block_start += BUFFER_SIZE; // Por seguridad
			}

			// Copiamos las 4 muestras actuales al final de la historia
			// OJO: Aquí asumo que el bloque de 4 no hace wrap (BUFFER_SIZE multiplo de 4)
			 memcpy(&fir_temp_buffer[taps_size], &adc_buffer[block_start], BLOCK_SIZE * sizeof(q15_t));

			// --- PASO 2: EJECUTAR FIR ---

			// Apuntamos al INICIO de los DATOS NUEVOS en el buffer temporal.
			// El PowerQuad buscará hacia atrás desde ahí para encontrar la historia.
			q15_t *pSrc = &fir_temp_buffer[taps_size];
			q15_t *pDst = &dac_buffer_q15[block_start]; // Guardar salida en buffer circular real

			arm_fir_q15(&fir, pSrc, pDst, BLOCK_SIZE);

//            // Punteros al bloque actual
//            q15_t *pInput = &adc_buffer[adc_buf_index - 3];
//            q15_t *pOutput = &dac_buffer_q15[adc_buf_index - 3];
//
//			memset(pOutput, 0, BLOCK_SIZE * sizeof(q15_t));
//            arm_fir_q15(&fir, pInput, pOutput, BLOCK_SIZE);

			// Resetear buffer de estado del Powequad. Por alguna razon despues de 4096 deja de filtrar
            if(*state_ptr == 4096){
            	*state_ptr = 0;
            }
//			printf("idx: %d, state: %d, dac: %d\r\n", adc_buf_index, *state_ptr, dac_buffer_q15[adc_buf_index-3]);
		}
	}
	else{
		// Bypass al filtro
		dac_val = adc_val >> 4;
	}

	// Buffer circular
	adc_buf_index = adc_buf_index + 1U;

	if(adc_buf_index >= BUFFER_SIZE){
		// Resetear idx
		buffer_laps++;
	    adc_buf_index = 0U;
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
        UpdateFIRPointers(sample_rate_idx);
        arm_fir_init_q15(&fir, taps_size_v[fir_type_idx][sample_rate_idx], fir_coef_ptr, state_ptr, BLOCK_SIZE);

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

    InitFIRPointers();
    UpdateFIRPointers(sample_rate_idx);

    arm_fir_init_q15(&fir, taps_size_v[fir_type_idx][sample_rate_idx], fir_coef_ptr, state_ptr, BLOCK_SIZE);

    memset(adc_buffer, 0, BUFFER_SIZE * sizeof(q15_t));
    memset(dac_buffer_q15, 0, BUFFER_SIZE * sizeof(q15_t));

    UpdateLedColor(sample_rate_idx);

    CTIMER_StartTimer(CTIMER0);

    while (1){
    	__NOP();
    }
}


