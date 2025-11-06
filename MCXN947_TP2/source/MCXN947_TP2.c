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

#define BUFFER_SIZE   512

#define NUM_RATES (sizeof(sample_rates)/sizeof(sample_rates[0])) // tamaño del arreglo
// NUM_RATES se define de esta forma para poder obtener el tamaño del arreglo sin importar el
// tipo de numero (en este caso uint32_t) ni la cantidad de numeros del arreglo (en este caso 5)

// Pines de botones (ajustar según board)
#define SW2_PIN   23   // PIO0_23
#define SW3_PIN   6   // PIO0_6

// Frecuencia del CTIMER
#define CTIMER_CLK_FREQ 600000 // 1.2 MHz

// Seno del DAC1
#define SINE_POINTS   100          // cantidad de muestras por ciclo

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
volatile bool adc_run = true;
volatile uint8_t sample_rate_idx = 0; // indice para elegir frecuencia de muestreo
q15_t adc_buffer[BUFFER_SIZE]; // buffer circular para almacenar conversiones
volatile uint16_t buf_index = 0; // indice para recorrer el buffer circular

static const float s_firTaps[TAPS_SIZE] = {-0.010711669921875,-0.0223388671875,-0.007080078125,-0.015289306640625,
		-0.009521484375   ,-0.008209228515625,-0.002838134765625, 0.00146484375    , 0.00592041015625 ,
		0.0086669921875   , 0.009521484375   , 0.008026123046875, 0.004486083984375,-0.00042724609375 ,
		-0.00567626953125 ,-0.010040283203125,-0.01239013671875 ,-0.011993408203125,-0.008636474609375,
		-0.002777099609375,0.004425048828125 , 0.011505126953125, 0.0167236328125  , 0.0185546875     ,
		0.01611328125     , 0.00933837890625 ,-0.000885009765625,-0.012725830078125,-0.023681640625   ,
		-0.031005859375   ,-0.032135009765625,-0.02532958984375 ,-0.009857177734375, 0.013519287109375,
		0.04266357421875  , 0.074462890625   , 0.105010986328125, 0.130340576171875, 0.147064208984375,
		0.15289306640625  , 0.147064208984375, 0.130340576171875, 0.105010986328125, 0.074462890625   ,
		0.04266357421875  , 0.013519287109375,-0.009857177734375,-0.02532958984375 ,-0.032135009765625,
		-0.031005859375   ,-0.023681640625   ,-0.012725830078125,-0.000885009765625, 0.00933837890625 ,
		0.01611328125     , 0.0185546875     , 0.0167236328125  , 0.011505126953125, 0.004425048828125,
		-0.002777099609375,-0.008636474609375,-0.011993408203125,-0.01239013671875 ,-0.010040283203125,
		-0.00567626953125 ,-0.00042724609375 , 0.004486083984375, 0.008026123046875, 0.009521484375   ,
		0.0086669921875   , 0.00592041015625 , 0.00146484375    ,-0.002838134765625,-0.008209228515625,
		-0.009521484375   ,-0.015289306640625,-0.007080078125   ,-0.0223388671875  ,-0.010711669921875};

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
    static int16_t tap[TAPS_SIZE];
	static int16_t output[BUFFER_SIZE];
	static uint32_t i;
	static uint32_t oldTime;
    pq_config_t pqConfig;

    /* Initialize the filter. HACERLO EN EL MAIN */
    for (i = 0; i < TAPS_SIZE; i++)
    {
        tap[i] = (int16_t)s_firTaps[i];
    }
    memset(output, 0, BUFFER_SIZE);

    /*
     * Normal method:
     * The input data and taps are all in system RAM, powerquad fetches the data
     * through the same bus.
     */
    pqConfig.inputAFormat   = kPQ_16Bit;
    pqConfig.inputAPrescale = 0;
    pqConfig.inputBFormat   = kPQ_16Bit;
    pqConfig.inputBPrescale = 0;
    pqConfig.outputFormat   = kPQ_16Bit;
    pqConfig.outputPrescale = 0;
    pqConfig.tmpFormat      = kPQ_Float;
    pqConfig.tmpPrescale    = 0;
    pqConfig.machineFormat  = kPQ_Float;
    pqConfig.tmpBase        = (uint32_t *)0xE0000000;

    PQ_SetConfig(POWERQUAD, &pqConfig);

//    oldTime = EXAMPLE_GetTime();
    for (i = 0; i < EXAMPLE_CALCULATION_LOOP; i++)
    {
        PQ_FIR(POWERQUAD, adc_buffer, BUFFER_SIZE, tap, TAPS_SIZE, output, PQ_FIR_FIR);
        PQ_WaitDone(POWERQUAD);
    }

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
	adc_buffer[buf_index] = (q15_t)(adc_val - 32768U);
//	q15_t q15_val = ((int32_t)adc_val - 2048) << 4;

	// Buffer circular
	buf_index = (buf_index + 1) % BUFFER_SIZE;

	// Enviar al DAC
//	ADC: [0,65536] DAC: [0,4096]
	DAC_SetData(DAC0, adc_val >> 4);
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
        adc_run = !adc_run;
        if (adc_run) {
            CTIMER_StartTimer(CTIMER0);
            PRINTF("ADC RUN\r\n");
        } else {
            CTIMER_StopTimer(CTIMER0);
            PRINTF("ADC STOP\r\n");
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

    if(adc_run)
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

    PRINTF("Filtro FIR pasa bajos\r\n");

    UpdateLedColor(sample_rate_idx);
    GenerateSineTable();
//    TODO: MULTIPLICAR ACA EL ARRAY DE COEFICIENTES DEL FIR POR 2^15 PARA PASARLOS A Q15

    CTIMER_StartTimer(CTIMER1);
    CTIMER_StartTimer(CTIMER0);

    while (1) {
        // El trabajo lo hacen las interrupciones
        __NOP();
    }
}
