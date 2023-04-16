
#include "control_app.h"

#include <stdlib.h>

#include "FreeRTOS.h"
#include "arm_math.h"
#include "data_publisher.h"
#include "pid.h"
#include "unit_conversions.h"
#include "sapi.h"
#include "task.h"

#define SAMPLING_FREQ_HZ 1000
#define WVFM_FREQ_HZ 10
#define PRBS_FREQ_HZ 100

#define CNT_PID 1
#define CNT_STATE 2
#define CNT_STATE_OBSERVER 3

#define CNT_TYPE CNT_STATE_OBSERVER

/* ***************************** Pole Placement **************************** */
struct {
    float32_t a00;
    float32_t a01;
    float32_t b0;
} obs_params = {
    .a00 = .19767007,
    .a01 = .34000453,
    .b0 = .4623254,
};

typedef struct {
    float32_t K[2];
    float32_t k0;
} PPController_t;

PPController_t pp_cnt = {.K = {0.37964295, 0.60778899},
                         .k0 = 1.9874319426885374};

float32_t PPController_compute(PPController_t* self, float32_t x1, float32_t x2,
                               float32_t r) {
    return r * self->k0 - (self->K[0] * x1 + self->K[1] * x2);
}

/* ********************************** PID ********************************** */

static PIDController pid = {
    .bypassPid = false,

    /* Control parameters. */
    .Kp = 9.0,
    .Ki = 300.0,
    .Kd = 0.005,
    .tau = 1. / SAMPLING_FREQ_HZ,
    .limMin = -1,
    .limMax = 1,
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

/* ************************************************************************* */
/*                                  Program                                  */
/* ************************************************************************* */

static void controlTask(void* _) {
    static data_publisher_t data_publisher;
    data_publisher_init(&data_publisher, SAMPLING_FREQ_HZ);
    TickType_t lastWakeTime = xTaskGetTickCount();
    uint32_t dacDn = 0;
    q15_t adcQ;
    q15_t adc2Q;
    float32_t controlOutputF32;
    float32_t adcF32;
    float32_t adc2F32;
    float32_t adc2F32_hat = 0.0;
    float32_t dacF32 = -0.5;
    uint32_t dacWaveCounter = 0;
    uint32_t perfCounter = 0;
    bool est_init = true;

    cyclesCounterInit(EDU_CIAA_NXP_CLOCK_SPEED);
    for (;;) {
        /* Loop Performance measurement.*/
        perfCounter = cyclesCounterRead();

        /* ADC Acquisition.*/
        adcQ = dn10b_to_q15(adcRead(CH1));
        adc2Q = dn10b_to_q15(adcRead(CH2));
        adcF32 = q15_to_float(adcQ);
        adc2F32 = q15_to_float(adc2Q);

        /* Reference calculation. */
        if (dacWaveCounter++ == (SAMPLING_FREQ_HZ / WVFM_FREQ_HZ / 2)) {
            dacF32 = -dacF32;
            dacWaveCounter = 0;
        }

#if CNT_TYPE == CNT_PID
        controlOutputF32 = PIDController_Update(&pid, dacF32, adcF32);
#elif CNT_TYPE == CNT_STATE
        controlOutputF32 =
            PPController_compute(&pp_cnt, adc2F32, adcF32, dacF32);
#elif CNT_TYPE == CNT_STATE_OBSERVER
        controlOutputF32 =
            PPController_compute(&pp_cnt, adc2F32_hat, adcF32, dacF32);
#endif

        adc2F32_hat = (obs_params.a00 * adc2F32_hat) +
                      (adcF32 * obs_params.a01) + (dacF32 * obs_params.b0);

        /* DAC Update & Data Transfer.*/
        dacDn = float_to_dn10b(controlOutputF32);
        dacWrite(DAC, dacDn);

        /* Samples UART Publishing.*/
        data_publisher_update_samples(
            &data_publisher, adcQ, float_to_q15(adc2F32_hat - adc2F32));  //   ;

        /* Delay to ensure loop frequency. */
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(1000 / SAMPLING_FREQ_HZ));

        /* Loop Performance measurement.*/
        perfCounter = cyclesCounterRead() - perfCounter;
    }
}

int runControlApp(void) {
    /* Hardware config. */
    boardConfig();
    adcConfig(ADC_ENABLE);
    dacConfig(DAC_ENABLE);

    /* Task creation. */
    xTaskCreate(controlTask, "controlTask", configMINIMAL_STACK_SIZE, NULL,
                tskIDLE_PRIORITY + 1, NULL);
    vTaskStartScheduler();

    /* Code won't reach here. */
    for (;;)
        ;

    return 0;
}
