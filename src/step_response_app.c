#include "step_response_app.h"
#include "data_publisher.h"
#include "arm_math.h"
#include "sapi.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdlib.h>

#define SAMPLING_FREQ_HZ 1000 
#define WVFM_FREQ_HZ  10
#define PRBS_FREQ_HZ 100 

static int16_t last_dac = 0;

static void squareWaveGenTask(void *_) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    for(;;) {
        last_dac ^= 0xFFFF;
        last_dac &= 0xFFFF;
        dacWrite(DAC, last_dac);
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(1000 / WVFM_FREQ_HZ / 2));
    }
}

static void prbsWaveGenTask(void *_) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    for(;;) {
        last_dac = rand() % 1024; 
        dacWrite(DAC, last_dac);
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(1000 / PRBS_FREQ_HZ));
    }
}


static void systemIdentificationTask(void *_) {
    static data_publisher_t data_publisher;
    data_publisher_init(&data_publisher, SAMPLING_FREQ_HZ);
    TickType_t lastWakeTime = xTaskGetTickCount();
    for(;;) {
        data_publisher_update_samples(
            &data_publisher, (((q15_t)adcRead(CH1) - 512) << 6), (((q15_t)last_dac - 512) << 6));
        vTaskDelayUntil(&lastWakeTime,  pdMS_TO_TICKS(1000 / SAMPLING_FREQ_HZ));
    }
}

int runStepResponseApp(void) {
    /* Hardware config. */ 
    boardConfig();
    adcConfig(ADC_ENABLE);
    dacConfig(DAC_ENABLE);

    /* Task creation. */
    xTaskCreate(prbsWaveGenTask, "waveformTask", configMINIMAL_STACK_SIZE,
        NULL, tskIDLE_PRIORITY+1, NULL);   
    xTaskCreate(systemIdentificationTask, "systemIdentificationTask",
        configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+1, NULL);   
    vTaskStartScheduler();

    /* Code won't reach here. */
    for(;;);
    return 0;
}
