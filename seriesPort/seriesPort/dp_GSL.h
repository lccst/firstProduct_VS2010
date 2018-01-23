#ifndef __DP_GSL_H__
#define __DP_GSL_H__

#include "TLS.h"


unsigned int gaussse_fit(double*result, double *fit_x, double *fit_y, unsigned int fitLen, double *coef_init, double tolerance=1e-4);
unsigned int getData(HANDLE hCom, void *outbufx, void *outbufy);
int processBuffer(double *data, unsigned short size, int dist, int peakWdthThd);


#endif