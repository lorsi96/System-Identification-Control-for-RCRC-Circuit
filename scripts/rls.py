import numpy as np
import typing

def rls_identification(u:np.ndarray, y:np.ndarray, order:int=2) -> typing.Tuple[np.ndarray, np.ndarray]:
    theta = np.zeros((2*order+1, 1))
    p = np.eye(2*order+1)
    def __rls_iter(ublock, yblock):
        phi = np.asmatrix(np.concatenate((yblock[1:], ublock)))
        p_new = p - np.dot(np.dot(p, phi.T), phi.dot(p))/(1 + np.dot(np.dot(phi, p), phi.T))
        k = np.dot(p, phi.T)/(1 + np.dot(np.dot(phi, p), phi.T))
        error = (np.asmatrix(yblock[0]) - np.dot(phi, theta))[0, 0]
        aux = error * k 
        theta_new = theta + np.asmatrix(aux)
        return theta_new, p_new, error 

    toterr = []
    for i in range(order, len(u)):
        theta, p, error = __rls_iter(u[i-order:i+1][::-1], y[i-order:i+1][::-1])
        toterr.append(error)
    
    return theta, np.array(toterr) 

if __name__ == '__main__':
    import pandas as pd
    import matplotlib.pyplot as plt

    OUTPUTFILE = 'csvs/step_resp.csv'
    SR_F_HZ = 1000

    data2 = pd.read_csv(OUTPUTFILE)

    theta, err = rls_identification(np.array(data2['DAC']), np.array(data2['ADC']), order=2)
    print(theta)
