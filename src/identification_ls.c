/*=====[identification_rls]=============================================
 * Copyright 2019 Diego Fernández <dfernandez202@gmail.com>
 * All rights reserved.
 * License: BSD-3-Clause <https://opensource.org/licenses/BSD-3-Clause>)
 *
 * Version: 1.0.0
 * Creation Date: 2019/09/23
 */


#include <stdlib.h>
#include "arm_math.h"

#include "identification_ls.h"

static arm_matrix_instance_f32 Y;
static arm_matrix_instance_f32 T;      // Theta -> M_SIZE x 1
static arm_matrix_instance_f32 F;      // Phi -> M_SIZE x M_VALUES
static arm_matrix_instance_f32 FT;     // Phi' -> M_VALUES x M_SIZE

static arm_matrix_instance_f32 aux0;   // aux0 = Phi' Phi -> M_SIZE x M_SIZE
static arm_matrix_instance_f32 aux1;   // aux1 = aux0^(-1) -> M_SIZE x M_SIZE
static arm_matrix_instance_f32 aux2;   // aux2 = Phi' Y -> M_SIZE x 1

// Configura la estructura de datos del Identificador
void ILS_Init (t_ILSdata* iData, uint32_t n, uint32_t ts_Ms, void (*pfR)(float32_t*))
{
	iData->N = n;
	iData->ts_Ms = ts_Ms;
	iData->p_receive = pfR;
	iData->i = 2;

	// Inicialización de matrices
    /**
    * @brief  Floating-point matrix initialization.
    * @param[in,out] S         points to an instance of the floating-point 
    *                          matrix structure.
    * @param[in]     nRows     number of rows in the matrix.
    * @param[in]     nColumns  number of columns in the matrix.
    * @param[in]     pData     points to the matrix data array.
    */
	arm_mat_init_f32(&Y, iData->N, 1, iData->buffer_Y);
	arm_mat_init_f32(&T, M_SIZE, 1, iData->buffer_T);
	arm_mat_init_f32(&F, iData->N, M_SIZE, iData->buffer_F);
	arm_mat_init_f32(&FT, M_SIZE, iData->N, iData->buffer_FT);
	arm_mat_init_f32(&aux0, M_SIZE, M_SIZE, iData->buffer_aux0);
	arm_mat_init_f32(&aux1, M_SIZE, M_SIZE, iData->buffer_aux1);
	arm_mat_init_f32(&aux2, M_SIZE, 1, iData->buffer_aux2);

	// Valores iniciales
	iData->buffer_Y[1] = 0;
	iData->buffer_Y[0] = 0;
	iData->buffer_U[1] = 0;
	iData->buffer_U[0] = 0;
}

// Ejecución recurrente del Identificador
void ILS_Run(t_ILSdata* iData)
{
	float32_t buffer[2];

	iData->p_receive(buffer);
	iData->buffer_U[iData->i] = buffer[0];
	iData->buffer_Y[iData->i] = buffer[1];

	iData->buffer_F[(iData->i*5)+0] = iData->buffer_Y[iData->i-1];
	iData->buffer_F[(iData->i*5)+1] = iData->buffer_Y[iData->i-2];
	iData->buffer_F[(iData->i*5)+2] = iData->buffer_U[iData->i];
	iData->buffer_F[(iData->i*5)+3] = iData->buffer_U[iData->i-1];
	iData->buffer_F[(iData->i*5)+4] = iData->buffer_U[iData->i-2];

	if (iData->i == (iData->N - 1))
	{
		// Cálculo de matrices traspuestas
		arm_mat_trans_f32(&F, &FT);

	    // Cálculo de Theta
		arm_mat_mult_f32(&FT, &F, &aux0);
		arm_mat_inverse_f32(&aux0, &aux1);
		arm_mat_mult_f32(&FT, &Y, &aux2);
		arm_mat_mult_f32(&aux1, &aux2, &T);
		iData->buffer_Y[1] = iData->buffer_Y[iData->i];
		iData->buffer_Y[0] = iData->buffer_Y[iData->i-1];
		iData->buffer_U[1] = iData->buffer_U[iData->i];
		iData->buffer_U[0] = iData->buffer_U[iData->i-1];
		iData->i = 2;
	}
	else 
    {
        iData->i++;
    }
}

/*=====[Implementations of interrupt functions]==============================*/

/*=====[Implementations of private functions]================================*/