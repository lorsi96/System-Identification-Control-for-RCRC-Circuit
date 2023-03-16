/* ************************************************************************* */
/*                             Public Inclusions                             */
/* ************************************************************************* */
#include "FreeRTOS.h"
#include "task.h"
#include "arm_const_structs.h"
#include "arm_math.h"

#include "sapi.h"
#include <stdlib.h>

/* ************************************************************************* */
/*                               Configuration                               */
/* ************************************************************************* */
#define SAMPLING_FREQ_HZ 1000
#define WVFM_FREQ_HZ 10
// #define PROG_FREQ_CYCLES (EDU_CIAA_NXP_CLOCK_SPEED / PROG_LOOP_HZ)

#define UART_BAUDRATE 460800
#define ADC_DATA_SZ 512
#define numTaps 2

/* ***************************** Data Transfer ***************************** */
static struct header_struct {
    char head[4];
    uint32_t id;
    uint16_t N;
    uint16_t fs;
    q15_t coeffs[numTaps];
    q15_t dbg1;
    q15_t dbg2;
    q15_t dbg3;
    char tail[4];
} header = {
    "head", 0, ADC_DATA_SZ, SAMPLING_FREQ_HZ, {0, 0}, 0, 0, 0x2000, "tail"};

/* *********************** LMS Buffers and Variables *********************** */
static q15_t dacBuffer[ADC_DATA_SZ] = {0};
static q15_t adcBuffer[ADC_DATA_SZ] = {0};
static q15_t errBuffer[ADC_DATA_SZ] = {0};
static q15_t foutBuffer[ADC_DATA_SZ] = {0};
static q15_t stateBuffer[ADC_DATA_SZ * 2 ];
static q15_t coeffs[numTaps+1] = {0x0002, 0x0003, 0x0002}; 
static uint16_t buffcount = 0;



static q15_t ciaaOutputSystemInput;

// void squareWaveGenTask(void *_) {
//     TickType_t lastWakeTime = xTaskGetTickCount();
//     for(;;) {
//         dacWrite(DAC, 0);
//         vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(1000 / WVFM_FREQ_HZ / 2));
//         dacWrite(DAC, 0xFFFF);
//         vTaskDelayUntil(&lastWakeTime,  pdMS_TO_TICKS(1000 / WVFM_FREQ_HZ / 2));
//     }
// }

void prbsWaveGenTask(void *_) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    for(;;) {
        ciaaOutputSystemInput = (q15_t)(rand() & 0xFFFF);
        dacWrite(DAC, (ciaaOutputSystemInput >> 6) + 512);
        vTaskDelayUntil(&lastWakeTime,  pdMS_TO_TICKS(1000 / WVFM_FREQ_HZ));
    }
}


void systemIdentificationTask(void *_) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    static arm_lms_instance_q15 lms = {0};
    arm_lms_init_q15(&lms, numTaps, coeffs, stateBuffer, 0x0200, ADC_DATA_SZ, 2);
    for(;;) {
        dacBuffer[buffcount] = ciaaOutputSystemInput;
        adcBuffer[buffcount] = (q15_t)((adcRead(CH1) - 512) << 6);
        uartWriteByteArray(UART_USB, (uint8_t*)&foutBuffer[buffcount], sizeof(q15_t));
        if(++buffcount == ADC_DATA_SZ) {
            header.dbg1 = foutBuffer[buffcount - 1];
            header.dbg2 = coeffs[0]; 
            header.dbg3 = errBuffer[buffcount - 1];
            arm_lms_q15(&lms, dacBuffer, adcBuffer, foutBuffer, errBuffer, ADC_DATA_SZ);
            memcpy(coeffs, header.coeffs, sizeof(q15_t) * numTaps);
            uartWriteByteArray(UART_USB, (uint8_t*)&header, sizeof(header));
            buffcount = 0;
        }
        vTaskDelayUntil(&lastWakeTime,  pdMS_TO_TICKS(1000 / SAMPLING_FREQ_HZ));
    }
}

// void sampleAndProcessingTask(void *_) {
//     q15_t adc_sample = 0;
//     uint32_t uartSamplesCount = 0;
//     for(;;) {
//         uint16_t raw_adc = adcRead(CH1);
//         adc_sample = (((raw_adc - 512)) << 6);
//         uartWriteByteArray(UART_USB, (uint8_t*)&adc_sample, sizeof(adc_sample));
//         if(++uartSamplesCount == ADC_DATA_SZ) {
//             uartWriteByteArray(UART_USB, (uint8_t*)&header, sizeof(header));
//             uartSamplesCount = 0;
//         }
//         vTaskDelay(pdMS_TO_TICKS(1000 / PROG_LOOP_HZ));
//     }
// }

int main(void) {
    // HW Config.
    boardConfig();
    uartConfig(UART_USB, UART_BAUDRATE);
    adcConfig(ADC_ENABLE);
    dacConfig(DAC_ENABLE);

    // Tasks creation.
    xTaskCreate(  prbsWaveGenTask
,            /* Pointer to the function that implements the task. */
                  "waveformTask",                 /* Text name for the task.  This is to facilitate debugging only. */
                  configMINIMAL_STACK_SIZE, /* Stack depth - most small microcontrollers will use much less stack than this. */
                  NULL,    /* Pass the text to be printed in as the task parameter. */
                  tskIDLE_PRIORITY+1,       /* This task will run at priority 1. */
                  NULL );   
    xTaskCreate(  systemIdentificationTask,            /* Pointer to the function that implements the task. */
                  "sampleAndProcessingTask",                 /* Text name for the task.  This is to facilitate debugging only. */
                  configMINIMAL_STACK_SIZE, /* Stack depth - most small microcontrollers will use much less stack than this. */
                  NULL,    /* Pass the text to be printed in as the task parameter. */
                  tskIDLE_PRIORITY+1,       /* This task will run at priority 1. */
                  NULL );   
    vTaskStartScheduler();
    for(;;);
    return 0;
}