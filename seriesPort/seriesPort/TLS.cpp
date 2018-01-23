
#include "TLS.h"

/******************************************************************************
*							底层驱动，主要是串口通信
*
*******************************************************************************/


HANDLE serial_open(char* COMx,int BaudRate)  
{  
	HANDLE hCom;  
	char comBuf[15] = {0};
	
	DCB dcb={0};

	sprintf(comBuf, "\\\\.\\%s", COMx);

    hCom =CreateFile(comBuf,  
        GENERIC_READ|GENERIC_WRITE,   
        0,  
        0,  
        OPEN_EXISTING,  
        0,//FILE_FLAG_OVERLAPPED,   //同步方式 或 重叠方式  
        0  
        );  
  
    if(hCom ==INVALID_HANDLE_VALUE)  
    {  
        DWORD dwError=GetLastError();  
		printf("hCom ==INVALID_HANDLE_VALUE\n");
        return 0;  
    }  
  
    dcb.DCBlength = sizeof(DCB);  
  
    if(!GetCommState(hCom,&dcb))  
    {  
        DWORD dwError=GetLastError();  
		printf("cannot GetCommState\n");
        return 0;  
    }  
  
    dcb.BaudRate = BaudRate;    //波特率  
    dcb.ByteSize = 8;           //位数  
    dcb.Parity = NOPARITY;      //奇偶检验  
    dcb.StopBits =ONESTOPBIT;   //停止位数  
  
    if(!SetCommState(hCom,&dcb))  
    {  
        DWORD dwError=GetLastError();  
		printf("cannot SetCommState\n");
        return 0;  
    }  
    if( !PurgeComm( hCom, PURGE_RXCLEAR ) )    return 0;  
  
    SetupComm(hCom,1024*1024,1024*1024);  
	SetCommMask(hCom, EV_RXCHAR ) ;		// 设置通知事件

	COMMTIMEOUTS   comTimeOut; 
	comTimeOut.ReadIntervalTimeout = 6;
	comTimeOut.ReadTotalTimeoutMultiplier = 4;
	comTimeOut.ReadTotalTimeoutConstant = 4;
	SetCommTimeouts(hCom,&comTimeOut);

	printf("open series succeed\n");
    return hCom;  
}  

void serial_close(HANDLE hCom)  
{  
	printf("close series succeed\n");
    CloseHandle(hCom);  
}  

int serial_read(HANDLE hCom, void *OutBuf,int size)  
{  
    DWORD cnt=0;  
	ReadFile(hCom,OutBuf,size,&cnt,0);
	return cnt;  
}

int serial_waitData(HANDLE hCom)
{
	OVERLAPPED os; 
	DWORD dwMask,dwTrans,dwError=0,err; 

	memset(&os,0,sizeof(OVERLAPPED)); 
	os.hEvent=CreateEvent(NULL,TRUE,FALSE,NULL); 
	WaitCommEvent(hCom,&dwMask,&os);
	if((dwMask&EV_RXCHAR) == EV_RXCHAR)
		return 0;
	else
		return -1;
}



//int serial_read(void *OutBuf,int size)  
//{  
//    DWORD cnt=0;  
//	OVERLAPPED os; 
//	DWORD dwMask,dwTrans,dwError=0,err; 
//
//	memset(&os,0,sizeof(OVERLAPPED)); 
//	os.hEvent=CreateEvent(NULL,TRUE,FALSE,NULL); 
//	WaitCommEvent(hCom,&dwMask,&os);
//	if((dwMask&EV_RXCHAR) == EV_RXCHAR){
//		ReadFile(hCom,OutBuf,size,&cnt,0);
//	}
//	return cnt;  
//}

int serial_write(HANDLE hCom, const void *Buf,int size)  
{  
    DWORD dw;  
    WriteFile(hCom,Buf,size,&dw,NULL);  
    return dw;  
} 
int serial_Recv_Char(HANDLE hCom, void *outBuf,int size)
{
	char recvBuf[1024];
	char temp1[256];
	char temp2[256];
	int  recvCnt = 0;

	memset(recvBuf, '\0', sizeof(char)*1024);
	while(1)
	{
		recvCnt = 0;
		Sleep(100);		// 不延时，有时没有办法读完整，测试得到的结论
		recvCnt = serial_read(hCom, temp1, 256);
		if(recvCnt){
			for(int i=0; i<recvCnt; i++){
				memset(temp2, '\0', sizeof(char)*256);
				sprintf(temp2, "%c", temp1[i]);
				strcat(recvBuf, temp2);
			}
			strcpy((char*)outBuf, recvBuf);
			break;
		}
	}

	return recvCnt;
}

int serial_Recv(HANDLE hCom, void *outBuf,int size)
{
	unsigned char recvBuf[1024];
	unsigned char temp1[256];
	unsigned char temp2[256];
	int  recvCnt = 0;

	memset(recvBuf, '\0', sizeof(char)*1024);
	while(1)
	{
		recvCnt = 0;
		Sleep(100);		// 不延时，有时没有办法读完整，测试得到的结论
		recvCnt = serial_read(hCom, temp1, 256);
		if(recvCnt){
			for(int i=0; i<recvCnt; i++){
				memset(temp2, '\0', sizeof(char)*256);
				sprintf((char*)temp2, "%x ", temp1[i]);
				strcat((char*)recvBuf, (char*)temp2);
			}
			strcpy((char*)outBuf, (char*)recvBuf);
			break;
		}
	}

	printf("\n\nread data\n");
	for(int i=0; i<recvCnt; i++)
		printf("%x ", recvBuf[i]);
	printf("\nread data\n\n");
	return recvCnt;
}

// 因为串口发送的是 8 位的1字节，所以接收2字节数据时，需要拼接
// 接收到 换行符停止 '\r\n' 对应的  ASCII 码 0x0D 0x0A
int serial_Recv2Bytes(HANDLE hCom, void *outBuf,int size)
{
	unsigned char recvBuf[1024];
	unsigned char temp1[1024];
	unsigned char temp2[1024];
	int  recvCnt = 0;

	memset(recvBuf, '\0', sizeof(char)*1024);
	serial_read(hCom, recvBuf, 1);	// 不知道为什么，读串口第一次总是1，所以先读一次

	int data2Bytes = 0;
	while(1)
	{
		recvCnt = 0;
		recvCnt = serial_read(hCom, temp1, 1023);

	}

	return 1;
}

/******************************************************************************
*
*							底层驱动，主要是串口通信
*******************************************************************************/



/******************************************************************************
*								TLS驱动
*******************************************************************************/
char ld_state;
int TLS_WriteRead(HANDLE hCom, unsigned char *writebuff,unsigned char *readbuff,unsigned int readbuff_max)
{
	DWORD status;
	unsigned short crc16=0xffff;
	unsigned char cmdBuf[128] = {0};
	int i,j;
	unsigned char t;

	strcpy((char*)cmdBuf, (char*)writebuff);

	for(i=0;i<6;i++)
	{
		crc16 = crc16^cmdBuf[i];
		for(j=0;j<8;j++)
		{
			t = crc16&1;
			crc16 = crc16>>1;
			crc16 = crc16 & 0x7fff;
			if(t==1)
				crc16 = crc16 ^ 0xa001;
		}
	}
	cmdBuf[6] = crc16 & 0xff;
	cmdBuf[7] = (crc16 & 0xff00) >>8;
	status = serial_write(hCom, cmdBuf, 8);
//	serial_write(hCom, "\r\n", 2);
	if(readbuff)
	{
		Sleep(100);
		serial_Recv (hCom, readbuff, readbuff_max);
	}
	return 0;
}

int TLS_set_wavelength(HANDLE hCom, double TLS_start)
{
	unsigned char cmdbuff[128] = {0};
	unsigned char readBuf[128] = {0};
	unsigned long wavelength;

	//set start  01 06 10 05 70 00 0c 2c

	wavelength = TLS_start*1000 - 1520000;
	strcpy((char*)cmdbuff,"\x01\x06\x10\x05\x00\x00\x0c\x2c");
	cmdbuff[4] = (wavelength & 0xff00)>>8;
	cmdbuff[5] = (wavelength & 0xff);
	TLS_WriteRead(hCom, cmdbuff,readBuf, 128);
	Sleep(100);

	//set stop 01 06 10 06 48 00 0c 2c
	wavelength = TLS_start*1000 - 1520000;
	strcpy((char*)cmdbuff,"\x01\x06\x10\x06\x00\x00\x0c\x2c");
	cmdbuff[4] = (wavelength & 0xff00)>>8;
	cmdbuff[5] = (wavelength & 0xff);
	TLS_WriteRead(hCom, cmdbuff,readBuf, 128);
	Sleep(100);

	return 1;
}


int TLS_Start_sweep(HANDLE hCom)
{
	unsigned char cmdbuff[128] = {0};
	unsigned char readBuf[128] = {0};

	//01 06 02 02 00 01 0c 2c
	strcpy((char*)cmdbuff,"\x01\x06\x02\x02\x01\x01\x0c\x2c");
	cmdbuff[4] = 0;
	TLS_WriteRead(hCom, cmdbuff,readBuf, 128);
	Sleep(100);
	return 1;

}

int TLS_Step(HANDLE hCom, double TLS_trig_step)
{
	unsigned char cmdbuff[128] = {0};
	unsigned char readBuf[128] = {0};
	DWORD bytes_read = 0;
	unsigned long wavelength;

	//set step 01 06 10 07 00 20 0c 2c
	wavelength = TLS_trig_step * 1000;
	strcpy((char*)cmdbuff,"\x01\x06\x10\x07\x00\x00\x0c\x2c");
	cmdbuff[4] = (wavelength & 0xff00)>>8;
	cmdbuff[5] = (wavelength & 0xff);
	TLS_WriteRead(hCom, cmdbuff,readBuf, 128);
	Sleep(100);

	return 1;
}

int TLS_Setup(HANDLE hCom, double TLS_start,double TLS_stop)
{
	unsigned char cmdbuff[128] = {0};
	unsigned char readBuf[128] = {0};
	unsigned long wavelength;

	//set start  01 06 10 05 70 00 0c 2c

	wavelength = TLS_start*1000 - 1520000;
	strcpy((char*)cmdbuff,"\x01\x06\x10\x05\x00\x00\x0c\x2c");
	cmdbuff[4] = (wavelength & 0xff00)>>8;
	cmdbuff[5] = (wavelength & 0xff);
	TLS_WriteRead(hCom, cmdbuff,readBuf, 128);
	Sleep(100);
	//set stop 01 06 10 06 48 00 0c 2c
	wavelength = TLS_stop*1000 - 1520000;
	strcpy((char*)cmdbuff,"\x01\x06\x10\x06\x00\x00\x0c\x2c");
	cmdbuff[4] = (wavelength & 0xff00)>>8;
	cmdbuff[5] = (wavelength & 0xff);
	TLS_WriteRead(hCom, cmdbuff,readBuf, 128);
	Sleep(200);

	return 1;
}


int TLS_set_LD_state(HANDLE hCom, char set_ON_OFF)
{
    unsigned char cmdbuff[128] = {0};
	unsigned char readBuf[128] = {0};

    strcpy((char*)cmdbuff,"\x01\x06\x02\x00\x00");
    int i,readbyts;


	if((set_ON_OFF!=LD_STATE_OFF && set_ON_OFF!=LD_STATE_ON))
		return -1;
	if(set_ON_OFF == LD_STATE_ON)
	{
        //strcpy(cmdbuff,"\x01\x06\x02\x00\x00\x01\x49\xB2");
        cmdbuff[4] = 0;
        cmdbuff[5] = 1;
		TLS_WriteRead(hCom, cmdbuff,readBuf, 128);
		ld_state = LD_STATE_ON;
		Sleep(100);
	}
	if(set_ON_OFF == LD_STATE_OFF)
	{
        //strcpy(cmdbuff,"\x01\x06\x02\x00\x00\x00\x88\x72");
        cmdbuff[4] = 0;
        cmdbuff[5] = 0;
		TLS_WriteRead(hCom, cmdbuff,readBuf, 128);
		ld_state = LD_STATE_ON;
		Sleep(100);
	}
	return 0;
}

int Init_TLS(HANDLE hCom, double startWaveLength, double stopWaveLength, double sweepStep)
{
	TLS_Setup(hCom, startWaveLength, stopWaveLength);
	TLS_Step(hCom, sweepStep);
	return 1;
}
