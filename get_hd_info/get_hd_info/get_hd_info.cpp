// get_hd_info.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
//#include <windows.h>
//#include <winioctl.h>
#pragma hdrstop
#pragma argsused

#include <tchar.h>
#include <stdio.h>
#include <windows.h>
#include "hdd.h"

const WORD IDE_ATAPI_IDENTIFY = 0xA1;   // ��ȡATAPI�豸������
const WORD IDE_ATA_IDENTIFY   = 0xEC;   // ��ȡATA�豸������
const int DISK_PATH_LEN = 128;
const int DISK_INFO_BUF_LEN = 128;


void ChangeByteOrder(PCHAR szString, USHORT uscStrSize)
{
	USHORT i;
	CHAR temp;

	for (i = 0; i < uscStrSize; i+=2)
	{
		temp = szString[i];
		szString[i] = szString[i+1];
		szString[i+1] = temp;
	}
}


DWORD GetDiskModelNumber(DWORD driver, CHAR *modelNumber, CHAR *sn_buf, CHAR * fw_buf)
{
	CHAR sFilePath[DISK_PATH_LEN];
	BOOL result;                 // results flag
	DWORD readed;                   // discard results
	HANDLE hDevice;
	//WORD i;

	//sprintf(sFilePath, "\\\\.\\PHYSICALDRIVE%d", driver);
	sprintf(sFilePath, "\\\\.\\PhysicalDrive%d", driver);
	hDevice = CreateFile(
		sFilePath, // drive to open
		GENERIC_READ | GENERIC_WRITE,     // access to the drive
		FILE_SHARE_READ | FILE_SHARE_WRITE, //share mode
		NULL,             // default security attributes
		OPEN_EXISTING,    // disposition
		0,                // file attributes
		NULL            // do not copy file attribute
		);
	if (hDevice == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "CreateFile() Error: %ld\n", GetLastError());
		return (DWORD)-1;
	}

	GETVERSIONINPARAMS gvopVersionParams;
	result = DeviceIoControl(
		hDevice,
		SMART_GET_VERSION,
		NULL,
		0,
		&gvopVersionParams,
		sizeof(gvopVersionParams),
		&readed,
		NULL);
	if (!result)        //fail
	{
		fprintf(stderr, "SMART_GET_VERSION Error: %ld\n", GetLastError());
		(void)CloseHandle(hDevice);
		return (DWORD)-1;
	}

	if(0 == gvopVersionParams.bIDEDeviceMap)
	{
		return (DWORD)-1;
	}

	// IDE or ATAPI IDENTIFY cmd
	BYTE btIDCmd;
	SENDCMDINPARAMS inParams;
	BYTE nDrive =0;
	btIDCmd = (gvopVersionParams.bIDEDeviceMap >> nDrive & 0x10) ? IDE_ATAPI_IDENTIFY : IDE_ATA_IDENTIFY;

	// output structure
	BYTE outParams[sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1];   // + 512 - 1

	//fill in the input buffer
	inParams.cBufferSize = 0;           //or IDENTIFY_BUFFER_SIZE ?
	inParams.irDriveRegs.bFeaturesReg = READ_ATTRIBUTES;
	inParams.irDriveRegs.bSectorCountReg = 1;
	inParams.irDriveRegs.bSectorNumberReg = 1;
	inParams.irDriveRegs.bCylLowReg = 0;
	inParams.irDriveRegs.bCylHighReg = 0;

	inParams.irDriveRegs.bDriveHeadReg = (nDrive & 1) ? 0xB0 : 0xA0;
	inParams.irDriveRegs.bCommandReg = btIDCmd;
	//inParams.bDriveNumber = nDrive;

	//get the attributes
	result = DeviceIoControl(
		hDevice,
		SMART_RCV_DRIVE_DATA,
		&inParams,
		sizeof(SENDCMDINPARAMS) - 1,
		outParams,
		sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1,
		&readed,
		NULL);
	if (!result)        //fail
	{
		fprintf(stderr, "SMART_RCV_DRIVE_DATA Error: %ld\n", GetLastError());
		(void)CloseHandle(hDevice);
		return (DWORD)-1;
	}

	(void)CloseHandle(hDevice);

	IDINFO *ip = (IDINFO *)((SENDCMDOUTPARAMS*)outParams)->bBuffer ;

	// get firmware number
	memset(fw_buf, 0, 8);
	memcpy(fw_buf, ip->sFirmwareRev, 8);
	fw_buf[8] = '\0';
	ChangeByteOrder(fw_buf, 8);
	printf("\n->Firmware: %s", fw_buf);


	// get Serial number
	memset(sn_buf, 0, 20);
	memcpy(sn_buf, ip->sSerialNumber, 20);
	sn_buf[20] = '\0';
	ChangeByteOrder(sn_buf, 20);
	printf("\n->Serial: %s", sn_buf);

	// get model number
	memset(modelNumber, 0, 40);
	memcpy(modelNumber, ip->sModelNumber, 40);
	modelNumber[40] = '\0';
	ChangeByteOrder(modelNumber, 40);
	printf("\n->Model: %s", modelNumber);

	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	char model_buf[40+1], sn_buf[20+1], fw_buf[8+1];
	GetDiskModelNumber(0, model_buf, sn_buf, fw_buf);

	//	printf("Model: %s\nFirmware: %s\nSN: %s", model_buf, fw_buf, sn_buf);
	getchar();
	return 0;
}