/*=====[app]===================================================================
 * Copyright 2019 Diego Fernández <dfernandez202@gmail.com>
 * All rights reserved.
 * License: BSD-3-Clause <https://opensource.org/licenses/BSD-3-Clause>)
 *
 * Version: 1.0.0
 * Creation Date: 2019/09/23
 */

/*=====[Inclusions of function dependencies]=================================*/

#include "FreeRTOS.h"
#include "arm_math.h"
#include "data_publisher.h"
#include "identification_ls.h"
#include "identification_tasks.h"
#include "sapi.h"
#include "stdlib.h"
#include "task.h"

static StackType_t taskIdentificationStack[configMINIMAL_STACK_SIZE * 15];
static StaticTask_t taskIdentificationTCB;
static data_publisher_t uart_pub;

t_ILSdata* tILS1;

void receiveData(float* buffer);

int runIdentificationApp(void) {
    boardConfig();

    adcInit(ADC_ENABLE);
    dacInit(DAC_ENABLE);

    data_publisher_init(&uart_pub, 0);

    tILS1 = (t_ILSdata*)pvPortMalloc(sizeof(t_ILSdata));

    ILS_Init(tILS1, 50, 10, receiveData);

    xTaskCreateStatic(
        ILS_Task,                  // task function
        "Identification Task",     // human-readable neame of task
        configMINIMAL_STACK_SIZE,  // task stack size
        (void*)tILS1,              // task parameter (cast to void*)
        tskIDLE_PRIORITY + 1,      // task priority
        taskIdentificationStack,   // task stack (StackType_t)
        &taskIdentificationTCB     // pointer to Task TCB (StaticTask_t)
    );

    vTaskStartScheduler();

    for (;;)
        ;

    return 0;
}

/*=====[Implementations of private functions]================================*/

// Generación del DAC y captura del ADC
void receiveData(float* buffer) {
    float Y, U;

    uint16_t dacValue = 0;

    dacValue = rand() % 1024;

    dacWrite(DAC, dacValue);
    delayInaccurateUs(5);
    U = (float)dacValue * 3.3 / 1023.0;
    uint32_t sample = adcRead(CH1);
    Y = 3.3 * (float)sample / 1023.0;

    data_publisher_update_samples(&uart_pub, ((int16_t)sample - 512) << 6,
                                  ((int16_t)dacValue - 512) << 6);

    buffer[0] = U;
    buffer[1] = Y;
}
