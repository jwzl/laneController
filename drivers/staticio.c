#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <wchar.h>

#include "staticio.h"

#define  deviceDescription  L"DemoDevice,BID#0"

const wchar_t* profilePath = L"../libs/DemoDevice.xml";

int io_set_value(int32 port, const uint8 data){
	int i;
	int32 bit = 1;
	ErrorCode ret = Success;
	wchar_t enumString[256];
	uint8 dataBuffer[64] = {0};
	InstantDoCtrl * instantDoCtrl = InstantDoCtrl_Create();

	DeviceInformation devInfo;
	devInfo.DeviceNumber = -1;
	devInfo.DeviceMode = ModeWrite;
	devInfo.ModuleIndex = 0;
	wcscpy(devInfo.Description, deviceDescription);

	ret = InstantDoCtrl_setSelectedDevice(instantDoCtrl, &devInfo);
	if (BioFailed(ret)){
		AdxEnumToString(L"ErrorCode", (int32)ret, 256, enumString);
		printf("Some error occurred. And the last error code is 0x%X. [%ls]\n", ret, enumString);
		return -1;
	}

	//Loads a profile to initialize the device.
	ret = InstantDoCtrl_LoadProfile(instantDoCtrl, profilePath);
	if (BioFailed(ret)){
		AdxEnumToString(L"ErrorCode", (int32)ret, 256, enumString);
		printf("Some error occurred. And the last error code is 0x%X. [%ls]\n", ret, enumString);
		return -1;
	}

	ret = InstantDoCtrl_WriteBit(instantDoCtrl, port, bit, data);
	if (BioFailed(ret)){
		AdxEnumToString(L"ErrorCode", (int32)ret, 256, enumString);
		printf("Some error occurred. And the last error code is 0x%X. [%ls]\n", ret, enumString);
		return -1;
	}

	printf("\n DO output completed !");

	InstantDoCtrl_Dispose(instantDoCtrl);
	return 0;
}

int io_get_value(int32 portStart, int32 portCount, uint8 data[]){
	int i;
	ErrorCode ret = Success;
	wchar_t enumString[256];
	InstantDiCtrl * instantDiCtrl = InstantDiCtrl_Create();

	DeviceInformation devInfo;
	devInfo.DeviceNumber = -1;
	devInfo.DeviceMode = ModeWrite;
	devInfo.ModuleIndex = 0;
	wcscpy(devInfo.Description, deviceDescription);

	ret = InstantDiCtrl_setSelectedDevice(instantDiCtrl, &devInfo);
	if (BioFailed(ret)){
		AdxEnumToString(L"ErrorCode", (int32)ret, 256, enumString);
		printf("Some error occurred. And the last error code is 0x%X. [%ls]\n", ret, enumString);
		return -1;
	}

	//Loads a profile to initialize the device.
	ret = InstantDiCtrl_LoadProfile(instantDiCtrl, profilePath);
	if (BioFailed(ret)){
		AdxEnumToString(L"ErrorCode", (int32)ret, 256, enumString);
		printf("Some error occurred. And the last error code is 0x%X. [%ls]\n", ret, enumString);
		return -1;
	}

	ret = InstantDiCtrl_ReadAny(instantDiCtrl, portStart, portCount, data);
	if (BioFailed(ret)){
		AdxEnumToString(L"ErrorCode", (int32)ret, 256, enumString);
		printf("Some error occurred. And the last error code is 0x%X. [%ls]\n", ret, enumString);
		return -1;
	}

	InstantDiCtrl_Dispose(instantDiCtrl);
	return 0;
}
