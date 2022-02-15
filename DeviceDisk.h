#pragma once


#ifndef DEVICE_DISK_H
#define DEVICE_DISK_H
#include <Windows.h>
#include <Setupapi.h>
#pragma comment( lib, "setupapi.lib" )
#define DevCount 1024
struct DevInterfaceDetaillArray {
	DWORD szCount;
	PSP_DEVICE_INTERFACE_DETAIL_DATA deviceInterfaceDetailData_[DevCount];
};

int GetALLDISK(struct DevInterfaceDetaillArray* DevInterfaceDetailArr_);

int FreeALLDISK(struct DevInterfaceDetaillArray* DevInterfaceDetailArr_);

#endif


