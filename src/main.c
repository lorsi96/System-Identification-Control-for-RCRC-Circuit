/* ************************************************************************* */
/*                             Public Inclusions                             */
/* ************************************************************************* */
#include "FreeRTOS.h"
#include "task.h"
#include "arm_const_structs.h"
#include "arm_math.h"
#include "sapi.h"

/* ************************************************************************* */
/*                               Configuration                               */
/* ************************************************************************* */
#define PROG_LOOP_HZ 1000
#define SQ_WAVE_HZ 10
#define PROG_FREQ_CYCLES (EDU_CIAA_NXP_CLOCK_SPEED / PROG_LOOP_HZ)

#define UART_BAUDRATE 460800
#define ADC_DATA_SZ 512

/* ***************************** Data Transfer ***************************** */
static struct header_struct {
    char head[4];
    uint32_t id;
    uint16_t N;
    uint16_t fs;
    char tail[4];
} header = {
    "head", 0, ADC_DATA_SZ, PROG_LOOP_HZ, "tail"};

void dacTask(void *_) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    for(;;) {
        dacWrite(DAC, 0);
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(50));
        dacWrite(DAC, 0xFFFF);
        vTaskDelayUntil(&lastWakeTime,  pdMS_TO_TICKS(50));
    }
}

void adcTask(void *_) {
    q15_t adc_sample = 0;
    uint32_t uartSamplesCount = 0;
    for(;;) {
        uint16_t raw_adc = adcRead(CH1);
        adc_sample = (((raw_adc - 512)) << 6);
        uartWriteByteArray(UART_USB, (uint8_t*)&adc_sample, sizeof(adc_sample));
        if(++uartSamplesCount == ADC_DATA_SZ) {
            uartWriteByteArray(UART_USB, (uint8_t*)&header, sizeof(header));
            uartSamplesCount = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(1000 / PROG_LOOP_HZ));
    }
}

int main(void) {
    // HW Config.
    boardConfig();
    uartConfig(UART_USB, UART_BAUDRATE);
    adcConfig(ADC_ENABLE);
    dacConfig(DAC_ENABLE);

    // Tasks creation.
    xTaskCreate(  dacTask,            /* Pointer to the function that implements the task. */
                  "dacTask",                 /* Text name for the task.  This is to facilitate debugging only. */
                  configMINIMAL_STACK_SIZE, /* Stack depth - most small microcontrollers will use much less stack than this. */
                  NULL,    /* Pass the text to be printed in as the task parameter. */
                  tskIDLE_PRIORITY+1,       /* This task will run at priority 1. */
                  NULL );   
    xTaskCreate(  adcTask,            /* Pointer to the function that implements the task. */
                  "adcTask",                 /* Text name for the task.  This is to facilitate debugging only. */
                  configMINIMAL_STACK_SIZE, /* Stack depth - most small microcontrollers will use much less stack than this. */
                  NULL,    /* Pass the text to be printed in as the task parameter. */
                  tskIDLE_PRIORITY+1,       /* This task will run at priority 1. */
                  NULL );   
    vTaskStartScheduler();
    for(;;);
    return 0;
}