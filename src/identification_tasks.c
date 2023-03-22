#include "identification_tasks.h"

#include "sapi.h"
#include "data_publisher.h"

#include "arm_math.h"
#include <math.h>

#include "identification_ls.h"

void ILS_Task (void* taskParmPtr)
{
    static data_publisher_t uart_pub;
	t_ILSdata* tILS;
	portTickType xLastWakeTime;

	tILS = (t_ILSdata*) taskParmPtr;

    data_publisher_init(&uart_pub, 1000 / tILS->ts_Ms);

	// Para implementar un delay relativo
	xLastWakeTime = xTaskGetTickCount();

	for(;;)
	{
        // Ejecuto el Identificador
        ILS_Run(tILS);
		vTaskDelayUntil( &xLastWakeTime, ( tILS->ts_Ms / portTICK_RATE_MS ) );
	}
}
