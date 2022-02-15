#include "DeviceDisk.h"

#include <iostream>
#include <string>
using namespace std;




//函数调用出错一定要记得回收内存，否则会内存泄漏。
//切记要与FreeALLDISK 成对使用。

int GetALLDISK(struct DevInterfaceDetaillArray* DevInterfaceDetailArr_) {

    HDEVINFO diskClassDevices;
    GUID diskClassDeviceInterfaceGuid = GUID_DEVINTERFACE_DISK;
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA deviceInterfaceDetailData;
    DWORD requiredSize;
    DWORD deviceIndex;




    //
    // Get the handle to the device information set for installed
    // disk class devices. Returns only devices that are currently
    // present in the system and have an enabled disk device
    // interface.
    //
    diskClassDevices = SetupDiGetClassDevs(&diskClassDeviceInterfaceGuid,
        NULL,
        NULL,
        DIGCF_PRESENT |
        DIGCF_DEVICEINTERFACE);
    
    if (INVALID_HANDLE_VALUE == diskClassDevices) {
        return -1;
    }


    ZeroMemory(&deviceInterfaceData, sizeof(SP_DEVICE_INTERFACE_DATA));
    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    deviceIndex = 0;

    while (SetupDiEnumDeviceInterfaces(diskClassDevices,
        NULL,
        &diskClassDeviceInterfaceGuid,
        deviceIndex,
        &deviceInterfaceData)) {


        SetupDiGetDeviceInterfaceDetail(diskClassDevices,
            &deviceInterfaceData,
            NULL,
            0,
            &requiredSize,
            NULL);
            //回收掉临时内存
        if (!(ERROR_INSUFFICIENT_BUFFER == GetLastError())) {
            wprintf_s(L"122\n"); //正常情况下，就是获得122错误码，咱也不知道windows在干啥。
            if (INVALID_HANDLE_VALUE != diskClassDevices) {
                SetupDiDestroyDeviceInfoList(diskClassDevices);
            }
            return -1;
        }


        deviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(requiredSize);


        ZeroMemory(deviceInterfaceDetailData, requiredSize);
        deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        if (!SetupDiGetDeviceInterfaceDetail(diskClassDevices,
            &deviceInterfaceData,
            deviceInterfaceDetailData,
            requiredSize,
            NULL,
            NULL)) {
            //没有将此次的deviceInterfaceDetailData 内存返回到DevInterfaceDetailArr_中，所以在返回之前必须释放掉，防止内存泄漏。
            free(deviceInterfaceDetailData);
            //回收掉临时内存
            if (INVALID_HANDLE_VALUE != diskClassDevices) {
                SetupDiDestroyDeviceInfoList(diskClassDevices);
            }
            return -1;
        }
            
        DevInterfaceDetailArr_->deviceInterfaceDetailData_[deviceIndex] = deviceInterfaceDetailData;
        ++deviceIndex;
    }

    DWORD Err_Code = GetLastError();

    DevInterfaceDetailArr_->szCount = deviceIndex;

    //返回之前回收临时内存
    if (INVALID_HANDLE_VALUE != diskClassDevices) {
        SetupDiDestroyDeviceInfoList(diskClassDevices);
    }
    if (ERROR_NO_MORE_ITEMS == Err_Code) {
        //正常退出while的状态码：259，no more data availible。
        return 0;
    }
    return -2; //0正常执行成功，-1循环内部执行出错，-2执行出错退出循环。
}

/*
#include <Windows.h>
#include <Setupapi.h>
#include <Ntddstor.h>

#pragma comment( lib, "setupapi.lib" )

#include <iostream>
#include <string>
using namespace std;

#define START_ERROR_CHK()           \
    DWORD error = ERROR_SUCCESS;    \
    DWORD failedLine;               \
    string failedApi;

#define CHK( expr, api )            \
    if ( !( expr ) ) {              \
        error = GetLastError( );    \
        failedLine = __LINE__;      \
        failedApi = ( api );        \
        goto Error_Exit;            \
    }

#define END_ERROR_CHK()             \
    error = ERROR_SUCCESS;          \
    Error_Exit:                     \
    if ( ERROR_SUCCESS != error ) { \
        cout << failedApi << " failed at " << failedLine << " : Error Code - " << error << endl;    \
    }

int GetALLDISK(struct DevInterfaceDetaillArray* DevInterfaceDetailArr_) {

    HDEVINFO diskClassDevices;
    GUID diskClassDeviceInterfaceGuid = GUID_DEVINTERFACE_DISK;
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA deviceInterfaceDetailData;
    DWORD requiredSize;
    DWORD deviceIndex;

    HANDLE disk = INVALID_HANDLE_VALUE;
    STORAGE_DEVICE_NUMBER diskNumber;
    DWORD bytesReturned;

    START_ERROR_CHK();

    //
    // Get the handle to the device information set for installed
    // disk class devices. Returns only devices that are currently
    // present in the system and have an enabled disk device
    // interface.
    //
    diskClassDevices = SetupDiGetClassDevs(&diskClassDeviceInterfaceGuid,
        NULL,
        NULL,
        DIGCF_PRESENT |
        DIGCF_DEVICEINTERFACE);
    CHK(INVALID_HANDLE_VALUE != diskClassDevices,
        "SetupDiGetClassDevs");

    ZeroMemory(&deviceInterfaceData, sizeof(SP_DEVICE_INTERFACE_DATA));
    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    deviceIndex = 0;

    while (SetupDiEnumDeviceInterfaces(diskClassDevices,
        NULL,
        &diskClassDeviceInterfaceGuid,
        deviceIndex,
        &deviceInterfaceData)) {

        ++deviceIndex;

        SetupDiGetDeviceInterfaceDetail(diskClassDevices,
            &deviceInterfaceData,
            NULL,
            0,
            &requiredSize,
            NULL);
        CHK(ERROR_INSUFFICIENT_BUFFER == GetLastError(),
            "SetupDiGetDeviceInterfaceDetail - 1");

        deviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(requiredSize);
        CHK(NULL != deviceInterfaceDetailData,
            "malloc");

        ZeroMemory(deviceInterfaceDetailData, requiredSize);
        deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
        DevInterfaceDetailArr_->deviceInterfaceDetailData_[DevInterfaceDetailArr_->szCount] = deviceInterfaceDetailData;
        
        wprintf_s(L"%x\n", deviceInterfaceDetailData);
        wprintf_s(L"%x\n", DevInterfaceDetailArr_->deviceInterfaceDetailData_[DevInterfaceDetailArr_->szCount]);
        DevInterfaceDetailArr_->szCount++;

        CHK(SetupDiGetDeviceInterfaceDetail(diskClassDevices,
            &deviceInterfaceData,
            deviceInterfaceDetailData,
            requiredSize,
            NULL,
            NULL),
            "SetupDiGetDeviceInterfaceDetail - 2");

        

        
        //cout << deviceInterfaceDetailData->DevicePath << endl;
        //cout << "\\\\?\\PhysicalDrive" << diskNumber.DeviceNumber << endl;
        //cout << endl;
    }
    
    CHK(ERROR_NO_MORE_ITEMS == GetLastError(),
        "SetupDiEnumDeviceInterfaces");

    END_ERROR_CHK();

Exit:

    if (INVALID_HANDLE_VALUE != diskClassDevices) {
        SetupDiDestroyDeviceInfoList(diskClassDevices);
    }

    if (INVALID_HANDLE_VALUE != disk) {
        CloseHandle(disk);
    }

    return error;
}

*/
int FreeALLDISK(struct DevInterfaceDetaillArray* DevInterfaceDetailArr_) {
    for (int i = 0; i < DevInterfaceDetailArr_->szCount; i++) {
        free(DevInterfaceDetailArr_->deviceInterfaceDetailData_[i]);
    }
    return 0;
}

