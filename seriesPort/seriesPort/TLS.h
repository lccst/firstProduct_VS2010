#ifndef __TLS_H__
#define __TLS_H__

#include<process.h>  
#include <windows.h>
#include <stdio.h>
#include <TCHAR.H>  
#include "TLS.h"

//LD status
#define LD_STATE_ON 1
#define LD_STATE_OFF 0
/******************************************************************************
*							底层驱动，主要是串口通信
*******************************************************************************/
HANDLE serial_open(char* COMx,int BaudRate)    ;
void serial_close(HANDLE hCom) ;
int serial_read(HANDLE hCom, void *OutBuf,int size)    ;
int serial_write(HANDLE hCom, const void *Buf,int size)   ;
int serial_Recv(HANDLE hCom, void *outBuf,int size);
int serial_Recv_Char(HANDLE hCom, void *outBuf,int size);
int serial_waitData(HANDLE hCom);
/******************************************************************************
*								TLS驱动
*******************************************************************************/
int TLS_WriteRead(HANDLE hCom, unsigned char *writebuff,unsigned char *readbuff,unsigned int readbuff_max);
int TLS_set_wavelength(HANDLE hCom, double TLS_start);
int TLS_Start_sweep(HANDLE hCom);
int TLS_Step(HANDLE hCom, double TLS_trig_step);
int TLS_Setup(HANDLE hCom, double TLS_start,double TLS_stop);
int Init_TLS(HANDLE hCom, double startWaveLength, double stopWaveLength, double sweepStep);
int TLS_set_LD_state(HANDLE hCom, char set_ON_OFF);

#endif