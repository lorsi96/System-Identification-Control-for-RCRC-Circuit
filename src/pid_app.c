
#include <stdlib.h>
#include "pid_app.h"

#include "FreeRTOS.h"
#include "pid.h"
#include "arm_math.h"
#include "data_publisher.h"
#include "sapi.h"
#include "task.h"

#define SAMPLING_FREQ_HZ 1000
#define WVFM_FREQ_HZ 10
#define PRBS_FREQ_HZ 100

static PIDController pid = {
    /* Control parameters. */
	.Kp = 1.0,
	.Ki = 0.0,
	.Kd = 0.0,
	.tau = 0.0,
	.limMin = -1.65,
	.limMax = 1.65,
	.limMinInt = -1.0,
	.limMaxInt = 1.0,
	.T = 1.0 / SAMPLING_FREQ_HZ,

    /* Internal variables. */
	.integrator = 0.0,
	.prevError = 0.0,
	.differentiator = 0.0,
	.prevMeasurement = 0.0,
	.out = 0.0,
    };

static void pidTask(void *_) {
    static data_publisher_t data_publisher;
    data_publisher_init(&data_publisher, SAMPLING_FREQ_HZ);
    TickType_t lastWakeTime = xTaskGetTickCount();
    volatile uint32_t dacDn = 0;
    q15_t adcQ;
    q15_t dacSampleQ;
    float32_t pidOutputF32;
    float32_t adcF32;
    float32_t dacF32 = -1.65;
    uint32_t dacWaveCounter = 0;
    volatile uint32_t perfCounter = 0;

    cyclesCounterInit(EDU_CIAA_NXP_CLOCK_SPEED);
    for (;;) {
        /* Loop Performance measurement.*/
        perfCounter = cyclesCounterRead();

        /* ADC Acquisition.*/
        adcQ = ((q15_t)adcRead(CH1) - 512) << 6;
        adcF32 = (((float32_t)adcQ) / 0xFFFF) * 1.65;

        /* Reference calculation. */
        if(dacWaveCounter++ == (SAMPLING_FREQ_HZ / WVFM_FREQ_HZ / 2)) {
            dacF32 = -dacF32;
            dacWaveCounter = 0;
        }
        
        /* PID Compute. */
        pidOutputF32 = PIDController_Update(&pid, dacF32, adcF32);

        /* DAC Update & Data Transfer.*/
        dacDn = (uint32_t)(((pidOutputF32 + 1.65) / 3.3) * 0x3FF);
        dacWrite(DAC, dacDn);

        /* Samples UART Publishing.*/
        data_publisher_update_samples(&data_publisher,
                                      adcQ,
                                      (((q15_t)dacDn - 512) << 6));

        /* Delay to ensure loop frequency. */ 
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(1000 / SAMPLING_FREQ_HZ));

        /* Loop Performance measurement.*/
        perfCounter = cyclesCounterRead() - perfCounter; 
    }
}

int runPidApp(void) {
    /* Hardware config. */
    boardConfig();
    adcConfig(ADC_ENABLE);
    dacConfig(DAC_ENABLE);

    /* Task creation. */
    xTaskCreate(pidTask, "pidTask", configMINIMAL_STACK_SIZE, NULL,
                tskIDLE_PRIORITY + 1, NULL);
    vTaskStartScheduler();

    /* Code won't reach here. */
    for (;;);

    return 0;
}
