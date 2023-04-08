#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

typedef struct {
	/* Mode */
	unsigned short bypassPid; /* Disable pid entirely.*/

	/* Controller gains */
	float Kp;
	float Ki;
	float Kd;

	/* Derivative low-pass filter time constant */
	float tau;

	/* Output limits */
	float limMin;
	float limMax;
	
	/* Integrator limits */
	float limMinInt;
	float limMaxInt;

	/* Deadzone */
	float deadZone;

	/* Sample time (in seconds) */
	float T;

	/* Controller "memory" */
	float integrator;
	float prevError;			/* Required for integrator */
	float differentiator;
	float prevMeasurement;		/* Required for differentiator */

	/* Controller output */
	float out;

} PIDController;


float PIDController_Update(PIDController *pid, float setpoint, float measurement);
void  PIDController_Init(PIDController *pid);

#endif