#include <process.h>  
#include <windows.h>
#include <stdio.h>
#include <TCHAR.H>  
#include "TLS.h"
#include "dp_GSL.h"


#define N 50000
unsigned int bufx[N] = {0};
unsigned int bufy[N] = {0};
double data[N] = {0};

void do_job(HANDLE com_TLS, HANDLE com_data, double tls_start, double tls_finish, double tls_step)
{
	int loopTimes = 1000;
	while(loopTimes--){
		TLS_Start_sweep(com_TLS);
		int cnt = getData(com_data, bufx, bufy);
		for(int i=0; i<cnt; i++)
			data[i] = bufy[i];
		processBuffer(data, cnt, (int)(500/(tls_step*1000)), (int)(200/(tls_step*1000)));	// 500pm 是fbg的半高宽度
	}
}

void main()
{
	double tls_start = 1540.5;
	double tls_finish = 1560.5;
	double tls_step = 0.02;

	HANDLE com_TLS = serial_open("COM12", 115200);
	HANDLE com_data = serial_open("COM21", 115200);

	TLS_set_LD_state(com_TLS, LD_STATE_ON);
	Sleep(3000);
	Init_TLS(com_TLS, tls_start, tls_finish, tls_step);
	//TLS_Start_sweep();


	do_job(com_TLS, com_data, tls_start, tls_finish, tls_step);

	Sleep(1000);
	serial_close(com_TLS);
	
	getchar();
}







