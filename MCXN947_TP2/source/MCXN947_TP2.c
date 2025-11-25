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

// Seno del DAC1
#define SINE_POINTS   100	// cantidad de muestras por ciclo
#define BUFFER_SIZE   512	// tamaño del buffer de datos
#define TAPS_SIZE	  22    // orden del filtro (80)

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
static void GenerateSineTable(void);
static void LED_SetColor(bool RED, bool GREEN, bool BLUE);

/*******************************************************************************
 * Variables globales
 ******************************************************************************/
volatile bool filter_run = true;
volatile uint8_t sample_rate_idx = 0; // indice para elegir frecuencia de muestreo
q15_t adc_buffer[BUFFER_SIZE]; // buffer circular para almacenar conversiones
volatile uint16_t adc_buf_index = 0; // indice para recorrer el buffer circular
volatile uint16_t dac_buf_index = 0; // indice para recorrer el buffer circular

// Filtro Fc = 3,6 kHz
float pasabajo_8k[22] = {
		-0.01220703125											 ,
		 0.016387939453125                                       ,
		-0.0194091796875                                         ,
		 0.019927978515625                                       ,
		-0.016510009765625                                       ,
		 0.007354736328125                                       ,
		 0.009674072265625                                       ,
		-0.03826904296875                                        ,
		 0.087066650390625                                       ,
		-0.188018798828125                                       ,
		 0.63397216796875                                        ,
		 0.63397216796875                                        ,
		-0.188018798828125                                       ,
		 0.087066650390625                                       ,
		-0.03826904296875                                        ,
		 0.009674072265625                                       ,
		 0.007354736328125                                       ,
		-0.016510009765625                                       ,
		 0.019927978515625                                       ,
		-0.0194091796875                                         ,
		 0.016387939453125                                       ,
		-0.01220703125											 };

float pasabajo_48k[124] = {
	-0.00135040283203125                                                           ,
	-0.00051116943359375                                                           ,
	 0.00054168701171875                                                           ,
	 0.0015716552734375                                                            ,
	 0.00235748291015625                                                           ,
	 0.0026702880859375                                                            ,
	 0.0023956298828125                                                            ,
	 0.0015411376953125                                                            ,
	 0.000244140625                                                                ,
	-0.0012359619140625                                                            ,
	-0.00257110595703125                                                           ,
	-0.003448486328125                                                             ,
	-0.00360107421875                                                              ,
	-0.0029449462890625                                                            ,
	-0.00154876708984375                                                           ,
	 0.000335693359375                                                             ,
	 0.00231170654296875                                                           ,
	 0.00394439697265625                                                           ,
	 0.00481414794921875                                                           ,
	 0.00466156005859375                                                           ,
	 0.00341796875                                                                 ,
	 0.00128173828125                                                              ,
	-0.0013427734375                                                               ,
	-0.00389862060546875                                                           ,
	-0.00579071044921875                                                           ,
	-0.00652313232421875                                                           ,
	-0.005828857421875                                                             ,
	-0.00373077392578125                                                           ,
	-0.00058746337890625                                                           ,
	 0.00299072265625                                                              ,
	 0.0062103271484375                                                            ,
	 0.0083160400390625                                                            ,
	 0.0087127685546875                                                            ,
	 0.00714111328125                                                              ,
	 0.0037689208984375                                                            ,
	-0.0008087158203125                                                            ,
	-0.00567626953125                                                              ,
	-0.009735107421875                                                             ,
	-0.011993408203125                                                             ,
	-0.01171875                                                                    ,
	-0.008697509765625                                                             ,
	-0.0033111572265625                                                            ,
	 0.003509521484375                                                             ,
	 0.0103607177734375                                                            ,
	 0.0157012939453125                                                            ,
	 0.018096923828125                                                             ,
	 0.0165863037109375                                                            ,
	 0.01093292236328125                                                           ,
	 0.00177001953125                                                              ,
	-0.0093841552734375                                                            ,
	-0.02037811279296875                                                           ,
	-0.02866363525390625                                                           ,
	-0.03180694580078125                                                           ,
	-0.02790069580078125                                                           ,
	-0.01596832275390625                                                           ,
	 0.0037841796875                                                               ,
	 0.02988433837890625                                                           ,
	 0.05971527099609375                                                           ,
	 0.08991241455078125                                                           ,
	 0.11679840087890625                                                           ,
	 0.13695526123046875                                                           ,
	 0.14774322509765625                                                           ,
	 0.14774322509765625                                                           ,
	 0.13695526123046875                                                           ,
	 0.11679840087890625                                                           ,
	 0.08991241455078125                                                           ,
	 0.05971527099609375                                                           ,
	 0.02988433837890625                                                           ,
	 0.0037841796875                                                               ,
	-0.01596832275390625                                                           ,
	-0.02790069580078125                                                           ,
	-0.03180694580078125                                                           ,
	-0.02866363525390625                                                           ,
	-0.02037811279296875                                                           ,
	-0.0093841552734375                                                            ,
	 0.00177001953125                                                              ,
	 0.01093292236328125                                                           ,
	 0.0165863037109375                                                            ,
	 0.018096923828125                                                             ,
	 0.0157012939453125                                                            ,
	 0.0103607177734375                                                            ,
	 0.003509521484375                                                             ,
	-0.0033111572265625                                                            ,
	-0.008697509765625                                                             ,
	-0.01171875                                                                    ,
	-0.011993408203125                                                             ,
	-0.009735107421875                                                             ,
	-0.00567626953125                                                              ,
	-0.0008087158203125                                                            ,
	 0.0037689208984375                                                            ,
	 0.00714111328125                                                              ,
	 0.0087127685546875                                                            ,
	 0.0083160400390625                                                            ,
	 0.0062103271484375                                                            ,
	 0.00299072265625                                                              ,
	-0.00058746337890625                                                           ,
	-0.00373077392578125                                                           ,
	-0.005828857421875                                                             ,
	-0.00652313232421875                                                           ,
	-0.00579071044921875                                                           ,
	-0.00389862060546875                                                           ,
	-0.0013427734375                                                               ,
	 0.00128173828125                                                              ,
	 0.00341796875                                                                 ,
	 0.00466156005859375                                                           ,
	 0.00481414794921875                                                           ,
	 0.00394439697265625                                                           ,
	 0.00231170654296875                                                           ,
	 0.000335693359375                                                             ,
	-0.00154876708984375                                                           ,
	-0.0029449462890625                                                            ,
	-0.00360107421875                                                              ,
	-0.003448486328125                                                             ,
	-0.00257110595703125                                                           ,
	-0.0012359619140625                                                            ,
	 0.000244140625                                                                ,
	 0.0015411376953125                                                            ,
	 0.0023956298828125                                                            ,
	 0.0026702880859375                                                            ,
	 0.00235748291015625                                                           ,
	 0.0015716552734375                                                            ,
	 0.00054168701171875                                                           ,
	-0.00051116943359375                                                           ,
	-0.00135040283203125														   };

// Filtro a usar
static float fir_taps_float[TAPS_SIZE];

//static const float fir_taps_float[TAPS_SIZE] = {1, 0};

static int16_t fir_taps_q15[TAPS_SIZE];
static int16_t out_buffer_q15[BUFFER_SIZE];
static uint16_t out_buffer_uint[BUFFER_SIZE];
q15_t dac_val_q15;
uint16_t dac_val;

arm_fir_instance_q15 fir;
q15_t state[1 + TAPS_SIZE - 1];

static bool fir_done = false;

static ctimer_match_config_t ctimerMatchConfig = {
  .matchValue = 749,
  .enableCounterReset = true,
  .enableCounterStop = false,
  .outControl = kCTIMER_Output_Toggle,
  .outPinInitState = false,
  .enableInterrupt = false
};

static lpadc_conv_result_t mLpadc_resultConfigStruct;

static uint32_t sine_table[SINE_POINTS];   // tabla de seno
static uint32_t sine_index = 0;            // índice actual en la tabla

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

static void PQ_FIRFixed16Example(void){
//    pq_config_t pqConfig;

    memset(out_buffer_q15, 0, BUFFER_SIZE);

    /*
     * Normal method:
     * The input data and taps are all in system RAM, powerquad fetches the data
     * through the same bus.
     * ESTO LO HACE EL CONFIG TOOLS
     */
//    pqConfig.inputAFormat   = kPQ_16Bit;
//    pqConfig.inputAPrescale = 0;
//    pqConfig.inputBFormat   = kPQ_16Bit;
//    pqConfig.inputBPrescale = 0;
//    pqConfig.outputFormat   = kPQ_16Bit;
//    pqConfig.outputPrescale = 0;
//    pqConfig.tmpFormat      = kPQ_Float;
//    pqConfig.tmpPrescale    = 0;
//    pqConfig.machineFormat  = kPQ_Float;
//    pqConfig.tmpBase        = (uint32_t *)0xE0000000;

//    PQ_SetConfig(POWERQUAD, &POWERQUAD_config);

//    oldTime = EXAMPLE_GetTime();
//    for (i = 0; i < EXAMPLE_CALCULATION_LOOP; i++)
//    {
	PQ_FIR(POWERQUAD, adc_buffer, BUFFER_SIZE, fir_taps_q15, TAPS_SIZE, out_buffer_q15, PQ_FIR_FIR);
	PQ_WaitDone(POWERQUAD);
//    }

//    PRINTF("%s: %d ms\r\n", "PQ fir fixed 16-bit normal method", (int)(EXAMPLE_GetTime() - oldTime));

//    for (i = 0; i < BUFFER_SIZE; i++)
//    {
//        EXAMPLE_ASSERT_TRUE(abs((int16_t)s_firOutputRef[i] - output[i]) <= 1);
//    }
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

//	Filtrar
	if(filter_run){
	    arm_fir_init_q15(&fir, TAPS_SIZE, fir_taps_q15, state, 3);
	    arm_fir_q15(&fir, &adc_buffer[adc_buf_index], &dac_val_q15, 1);
	    dac_val = (uint16_t)(dac_val_q15 + 32768U) >> 4;
	}
	else{
		dac_val = adc_val >> 4;
	}

//  Enviar al DAC
    DAC_SetData(DAC0, dac_val);
//	}
//	if(fir_done){
//
//		DAC_SetData(DAC0, out_buffer_uint[dac_buf_index]);
//		dac_buf_index = dac_buf_index + 1;
//
//		if(dac_buf_index >= BUFFER_SIZE){
//			dac_buf_index = 0;
//			fir_done = false;
//		}
//	}

	// Buffer circular
	adc_buf_index = (adc_buf_index + 1) % BUFFER_SIZE;
}

// ---- Callback de CTIMER1: genera seno ----
void ctimer1_match0_callback(uint32_t flags)
{
	/* Usa el seno generado en la funcion GenerateSineTable */
    DAC_SetData(DAC1, sine_table[sine_index]);

    sine_index = (sine_index + 1U) % SINE_POINTS;
}

// ---- ISR Botón SW2 (Run/Stop) ----
void GPIO0_INT_0_IRQHANDLER(void)
{
    uint32_t flags = GPIO_GpioGetInterruptChannelFlags(GPIO0, 0U);
    GPIO_GpioClearInterruptChannelFlags(GPIO0, flags, 0U);

    if (flags & (1U << SW2_PIN)) {
        filter_run = !filter_run;
        if (filter_run) {
//            CTIMER_StartTimer(CTIMER0);
            PRINTF("FILTER RUN\r\n");
        } else {
//            CTIMER_StopTimer(CTIMER0);
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
        PRINTF("Match value: %d \r\n", CTIMER0->MSR[kCTIMER_Match_3]);
    }
    CTIMER_Reset(CTIMER0);

//    if(filter_run)
    CTIMER_StartTimer(CTIMER0);
}

/*******************************************************************************
 * Generar tabla seno
 ******************************************************************************/
static void GenerateSineTable(void)
{
    for (uint32_t i = 0; i < SINE_POINTS; i++)
    {
        float angle = 2.0f * PI * (float)i / (float)SINE_POINTS;
        float val = 0.5f + 0.5f * arm_sin_f32(angle);   // normalizado [0..1]
        sine_table[i] = (uint16_t)(val * 4095);         // 12 bits DAC
    }
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

    PQ_Init(POWERQUAD);

    // Convierte los coeficientes del filtro a q15
    for (int i = 0; i < TAPS_SIZE; i++)
    {
    	fir_taps_q15[i] = (int16_t)(pasabajo_8k[i] * 32768);
//    	fir_taps_q15[i] = (int16_t) __SSAT(((int32_t)(fir_taps_float[i] * 32768.0f)), 16);	// Para saturar
    }

//    arm_fir_instance_q15 fir;

    arm_fir_init_q15(&fir, TAPS_SIZE, fir_taps_q15, state, 3);

    PRINTF("Filtro FIR pasa bajos\r\n");

    UpdateLedColor(sample_rate_idx);
    GenerateSineTable();

//    CTIMER_StartTimer(CTIMER1); // COMENTAR si no se usa el DAC1
    CTIMER_StartTimer(CTIMER0);

    while (1) {
        // El trabajo lo hacen las interrupciones
        __NOP();

//    	Filtrado
//    	if(adc_buf_index >= BUFFER_SIZE - 1){
////			memset(out_buffer_q15, 0, BUFFER_SIZE);
//////			Tarda 700 us
////			PQ_FIR(POWERQUAD, adc_buffer, BUFFER_SIZE, fir_taps_q15, TAPS_SIZE, out_buffer_q15, PQ_FIR_FIR);
////			PQ_WaitDone(POWERQUAD);
////    		PRINTF("filtering...\r\n");
//	        arm_fir_q15(&fir, adc_buffer, out_buffer_q15, BUFFER_SIZE);
//
//			// Enviar al buffer de salida
//			for(int i = 0; i < BUFFER_SIZE; i++){
//				out_buffer_uint[i] = (uint16_t)(out_buffer_q15[i] + 32768U) >> 4;
////				PRINTF("%d\r\n", out_buffer_uint[i]);
//			}
//			fir_done = true;
//		}
    }
}
