
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

inline static q15_t dn10b_to_q15(uint16_t dn) {
    return ((q15_t)dn - 512) << 6;  // -1 to 1
}

inline static float q15_to_float(q15_t dn) {
    float ret;
    arm_q15_to_float(&dn, &ret, 1);
    return ret;
}

inline static float float_to_q15(float fp) {
    q15_t ret;
    arm_float_to_q15(&fp, &ret, 1);
    return ret;
}

static uint16_t float_to_dn10b(float val) {
    q15_t ret;
    arm_float_to_q15(&val, &ret, 1);
    volatile uint16_t  res =  (ret >> 6) + 512;
    return res;
}


static PIDController pid = {
    .bypassPid = false,

    /* Control parameters. */
	.Kp = 9.0,
	.Ki = 300.0, 
	.Kd = 0.005,
	.tau = 1. / SAMPLING_FREQ_HZ,
	.limMin = -1.,
	.limMax = 1.,
	.limMinInt = -10.0,
	.limMaxInt = 10.0,
    .deadZone = 0.000, 
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
    volatile q15_t adcQ;
    float32_t pidOutputF32;
    volatile float32_t adcF32;
    float32_t dacF32 = -0.5;
    uint32_t dacWaveCounter = 0;
    volatile uint32_t perfCounter = 0;

    cyclesCounterInit(EDU_CIAA_NXP_CLOCK_SPEED);
    for (;;) {
        /* Loop Performance measurement.*/
        perfCounter = cyclesCounterRead();

        /* ADC Acquisition.*/
        adcQ = dn10b_to_q15(adcRead(CH1));
        adcF32 = q15_to_float(adcQ); 

        /* Reference calculation. */
        if(dacWaveCounter++ == (SAMPLING_FREQ_HZ / WVFM_FREQ_HZ / 2)) {
            dacF32 = -dacF32;
            dacWaveCounter = 0;
        }
        
        /* PID Compute. */
        pidOutputF32 = PIDController_Update(&pid, dacF32, adcF32);

        /* DAC Update & Data Transfer.*/
        dacDn = float_to_dn10b(pidOutputF32); 
        dacWrite(DAC, dacDn);

        /* Samples UART Publishing.*/
        data_publisher_update_samples(&data_publisher,
                                      adcQ, float_to_q15(dacF32)); //   ;

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
