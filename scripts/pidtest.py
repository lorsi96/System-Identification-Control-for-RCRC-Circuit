import ctypes
c_pid_funcs = ctypes.cdll.LoadLibrary('./pid.so')

class PIDController(ctypes.Structure):
    _fields_ = [
	("Kp", ctypes.c_float ),
	("Ki", ctypes.c_float ),
	("Kd", ctypes.c_float ),
	("tau", ctypes.c_float ),
	("limMin", ctypes.c_float ),
	("limMax", ctypes.c_float ),
	("limMinInt", ctypes.c_float ),
	("limMaxInt", ctypes.c_float ),
	("T", ctypes.c_float ),
	("integrator", ctypes.c_float ),
	("prevError", ctypes.c_float ),
	("differentiator", ctypes.c_float ),
	("prevMeasurement", ctypes.c_float ),
	("out", ctypes.c_float )
]

if __name__ == '__main__':
	import pandas as pd
	import matplotlib.pyplot as plt 

	SR_F_HZ = 1000
	SQ_WAVE_F_HZ = 10
	SAMPLES_PER_PERIOD = SR_F_HZ // SQ_WAVE_F_HZ

	data = pd.read_csv('step_resp.csv')
	instance = PIDController(
		Kp = 0.0, Ki = 0.0, Kd = 100.0, limMin=-1.0, limMax=1.0, T=0.01)

	c_pid_funcs.PIDController_Init(ctypes.pointer(instance))
	response = []
	error = []
	for adc, dac in zip(data['ADC'], data['DAC']):
		c_pid_funcs.PIDController_Update(ctypes.pointer(instance), 
				   						 ctypes.c_float(dac), 
										 ctypes.c_float(adc))
		response.append(float(instance.out))
		error.append(float(instance.prevError))

	plt.plot(error[:200])
	plt.plot(response[:200], 'o')
	plt.show()
