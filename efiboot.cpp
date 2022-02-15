// efiboot.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <stdio.h>
#include <windows.h>

#include <string>
#include <sstream>

using std::hex;
using std::stringstream;

#include "DeviceDisk.h"


//********************************************************
//参数获取函数定义
//********************************************************

#define OPT_LEN 50
#define OPT_COUNT 8


#define BOOTORDER 0
#define BOOTOPTION 1
#define ALL 2
#define ADD 3
char arg[OPT_COUNT][OPT_LEN] = {
    "--BootOrder",
    "--bootorder",
    "--BootOption",
    "--bootoption",
    "--ALL",
    "--all",
    "--Add",
    "--add"
};


static int EFI_FindItem(char* str) {
    int Item_pos = -1;
    for (int pos = 0; pos < OPT_COUNT; pos++) {
        if (0 == strcmp(arg[pos], str)) {
            Item_pos = pos;
        }
    }
    if (0 == Item_pos || 1 == Item_pos) Item_pos = BOOTORDER;
    if (2 == Item_pos || 3 == Item_pos) Item_pos = BOOTOPTION;
    if (4 == Item_pos || 5 == Item_pos) Item_pos = ALL;
    if (6 == Item_pos || 7 == Item_pos) Item_pos = ADD;
    return Item_pos; //-1:未在参数数组中找到对应参数。
}


#define INIT_VALUE -2
#define ARG_START_INIT 1
#define ARG_POS_INIT -1
#define ARG_POS_END -3

int arg_start = ARG_START_INIT;
int arg_pos =ARG_POS_INIT;
static int GetOpt(int argc,char * argv[]) {

    int Num = INIT_VALUE;
    if (ARG_POS_INIT == arg_pos) arg_pos = arg_start;
    while (arg_pos < argc)
    {
        Num=EFI_FindItem(argv[arg_pos]);
        arg_pos++;
        if (Num >= 0) break;
    }

    if (Num == INIT_VALUE) {
        Num = ARG_POS_END;
        arg_pos = ARG_POS_INIT;
    }

    return Num;
}


//win10访问需要先提权
// newPrivileges -> 提升权限值
// enable -> 提升/降低(恢复)
bool promoteProcessPrivileges(LPCWSTR newPrivileges, const bool enable)
{   
    //std::cout << newPrivileges << "\n" << std::endl;
    HANDLE tokenHandle;
    //获得当前进程的access token句柄
    if (::OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &tokenHandle) == FALSE)
        return false;
    TOKEN_PRIVILEGES structTkp;
    //查找newPrivileges参数对应的Luid，并将结果写入structTkp.Privileges[0]的Luid域中
    if (::LookupPrivilegeValue(NULL, newPrivileges, &structTkp.Privileges[0].Luid) == FALSE)
    {
        CloseHandle(tokenHandle);
        return false;
    }
    //设置structTkp结构
    structTkp.PrivilegeCount = 1;
    structTkp.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;
    //通知操作系统更改权限
    if (::AdjustTokenPrivileges(tokenHandle, FALSE, &structTkp, sizeof(structTkp), NULL, NULL) == FALSE)
    {
        CloseHandle(tokenHandle);
        return false;
    }
    CloseHandle(tokenHandle);
    return true;
}





#include ".\uefi\efi.h"

static int EFI_GetBootOrder() {
    if (!efi_init()) return -1;
    EFI_BOOT_ORDER * pBoot_Order = GetBootList();
    if (NULL == pBoot_Order) return -1;
    WCHAR BootEntry[10] = L"BootFFFF";		/* change #### with 0001,0002, etc. 8 char + 1 terminator + 1 dummy (useless) */
    
    BDS_LOAD_OPTION** pLoad = GetBootDevices();
    for (int i = 0; i < GetBootCount(); i++) {
        swprintf_s(BootEntry, 10, L"Boot%04X", pBoot_Order[i]);
        printf_s("    ");
        wprintf_s(BootEntry);
        printf_s(":");
        wprintf_s(pLoad[i]->Description);
        printf_s("\n");
    }
    return 0;
}

struct EFI_Disk_Info {
    std::string productor;
    std::string version;
    std::string vendor;
    std::string length;
    std::string number;
};

struct EFI_Vol_Info {
    std::wstring system;
    std::wstring length;
    std::wstring name;
};

//static struct DevInterfaceDetaillArray DevIn = { 0 };

//static struct EFI_Disk_Info * g_efi_disk[] = {NULL};

#define PARNUM(buf) (UINT32)(buf[3] << 24 | buf[2] << 16 | buf[1] << 8 | buf[0])
//#define 


static int EFI_GetVolumeInfo(WCHAR * ParName, DWORD ParNum, TCHAR* ParInfo, CHAR* DiskInfo) {

    //定义保存数据的数据结构
    struct EFI_Disk_Info disk_info_;
    struct EFI_Vol_Info vol_info_;



    //获取分区信息：卷标，文件系统格式。
    //wprintf_s(L"ParName:%s\n", ParName);

    TCHAR VolumeName[1024] = { 0 };
    TCHAR FileSysName[1024] = { 0 };
    if (0 == GetVolumeInformation(ParName, VolumeName, 1024, NULL, NULL, NULL, FileSysName, 1024)) {
        wprintf_s(L"Line:%d Err_Code：%d\n",__LINE__, GetLastError());
        return -1;
    }
    vol_info_.name = std::wstring(VolumeName);
    vol_info_.system = std::wstring(FileSysName);


    //获取硬盘信息

    //获取分区所在硬盘


    TCHAR Buf_Par_Name[1024] = { 0 };
    memcpy(Buf_Par_Name, ParName, wstrlen(ParName)-4); //构建分区设备，用于createfile打开分区。
    //wprintf_s(L"    Line:%d ParName:%s\n",__LINE__, Buf_Par_Name);
    HANDLE h_vol = INVALID_HANDLE_VALUE;
    h_vol = CreateFile(Buf_Par_Name,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, NULL);

    if (INVALID_HANDLE_VALUE == h_vol) {
        wprintf_s(L"Line:%d Err_Code：%d\n",__LINE__, GetLastError());
        return -1;
    }

    DWORD dwOutBytes = 0;
    PVOLUME_DISK_EXTENTS vol_disk_info = (PVOLUME_DISK_EXTENTS)new BYTE[sizeof(VOLUME_DISK_EXTENTS) + sizeof(DISK_EXTENT) * 10];
    ZeroMemory(vol_disk_info, sizeof(VOLUME_DISK_EXTENTS) + sizeof(DISK_EXTENT) * 10);
    if (!DeviceIoControl(h_vol,
        IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
        NULL, 0,
        vol_disk_info, sizeof(VOLUME_DISK_EXTENTS) + sizeof(DISK_EXTENT) * 10,
        &dwOutBytes,
        NULL)
        ) {

        wprintf_s(L"DeviceIoControl Error! line:%d Error_Code:%d %d\n",__LINE__, GetLastError(), dwOutBytes);

    }

    CloseHandle(h_vol);



    //硬盘信息相关
    if (vol_disk_info->NumberOfDiskExtents > 1) {
        wprintf_s(L"Line:%d 不支持解析一个分区跨越两个硬盘！\n",__LINE__);
        return -1;
    }

    DWORD* disk_num = (PDWORD)new DWORD[vol_disk_info->NumberOfDiskExtents];

    for (int i = 0; i < vol_disk_info->NumberOfDiskExtents; i++) {
        disk_num[i] = vol_disk_info->Extents[i].DiskNumber;
    }

    

    //构建硬盘路径
    TCHAR hd_name[512] = { 0 };
    wsprintf(hd_name, L"\\\\\.\\PhysicalDrive%d", disk_num[0]);
    //wprintf_s(L"hd_name:%s\n", hd_name);
    
    HANDLE h_disk = NULL;
        h_disk = CreateFile(hd_name,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL, NULL);

        
        
        
        
        //获取硬盘的生产商productor，销售商vendor，硬盘版本version等信息。
        PSTORAGE_DEVICE_DESCRIPTOR pDevDesc;
        pDevDesc = (PSTORAGE_DEVICE_DESCRIPTOR)new BYTE[sizeof(STORAGE_DEVICE_DESCRIPTOR) + 512 - 1];
        ZeroMemory(pDevDesc, sizeof(STORAGE_DEVICE_DESCRIPTOR) + 512 - 1);
        pDevDesc->Size = sizeof(STORAGE_DEVICE_DESCRIPTOR) + 512 - 1;

        STORAGE_PROPERTY_QUERY Query;
        Query.PropertyId = StorageDeviceProperty;
        Query.QueryType = PropertyStandardQuery;

        if (!DeviceIoControl(h_disk,
            IOCTL_STORAGE_QUERY_PROPERTY,
            &Query, sizeof(STORAGE_PROPERTY_QUERY),
            pDevDesc, pDevDesc->Size,
            &dwOutBytes,
            NULL)
            ) {
            wprintf_s(L"DeviceIoControl Error! line:%d Error_Code:%d %d\n", __LINE__, GetLastError(), dwOutBytes);

        }

        char* p_char_string = (char*)pDevDesc;
        
        disk_info_.productor = std::string(&p_char_string[pDevDesc->ProductIdOffset]);
        disk_info_.version = std::string(&p_char_string[pDevDesc->ProductRevisionOffset]);
        disk_info_.vendor = std::string(&p_char_string[pDevDesc->VendorIdOffset]);




        //获取硬盘容量

        GET_LENGTH_INFORMATION disk_len = { 0 };

        if (!DeviceIoControl(h_disk, IOCTL_DISK_GET_LENGTH_INFO,
            NULL,
            0,
            &disk_len, sizeof(disk_len), &dwOutBytes, NULL)) {

            wprintf_s(L"DeviceIoControl Error! line:%d Error_Code:%d %d\n",__LINE__, GetLastError(), dwOutBytes); //122是正常的。

        }

        stringstream sz_buffer;

        int buffer_disk_len = disk_len.Length.QuadPart / 1024 / 1024 / 1024;

        sz_buffer << buffer_disk_len << "GB";

        disk_info_.length = sz_buffer.str();

        disk_info_.number = std::to_string(disk_num[0]);

        //硬盘信息获取完毕。


        //下面获取分区容量

        DWORD sizePar = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + sizeof(PARTITION_INFORMATION_EX) * 128;
        PDRIVE_LAYOUT_INFORMATION_EX ParTable = (PDRIVE_LAYOUT_INFORMATION_EX)new BYTE[sizePar];
        ZeroMemory(ParTable, sizePar);

        if (!DeviceIoControl(h_disk,
            IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
            NULL,
            0,
            ParTable,
            sizePar,
            &dwOutBytes,
            NULL)) {

            wprintf_s(L"DeviceIoControl Error! line:%d Error_Code:%d %d\n",__LINE__, GetLastError(), dwOutBytes);

        }
        TCHAR vol_length[512] = { 0 };
        wsprintf(vol_length,L"%dMB", ParTable->PartitionEntry[ParNum-1].PartitionLength.QuadPart / 1024 / 1024);  //ParNum是分区号，分区号从1开始，但是这里分区入口是一个数组，所以分区1对应0号数组元素。
        vol_info_.length = std::wstring(vol_length);

    delete vol_disk_info;
    delete disk_num;
    delete pDevDesc;
    delete ParTable;

    //返回分区信息
    wsprintf(ParInfo, L"%d(%s,%s,%s)",ParNum ,vol_info_.system.c_str(), vol_info_.name.c_str(), vol_info_.length.c_str());
    //返回硬盘信息
    sprintf(DiskInfo, "HD%s:%s%s%s", disk_info_.number.c_str(), disk_info_.vendor.c_str(), disk_info_.productor.c_str(), disk_info_.length.c_str());


    return 0;
}


static int EFI_GetBootEntry(LPCWSTR lpBootEntry) {
    if (!efi_init()) return -1;
    EFI_BOOT_ORDER* pBoot_Order = GetBootList();
    if (NULL == pBoot_Order) return -1;
    WCHAR BootEntry[10] = L"BootFFFF";		/* change #### with 0001,0002, etc. 8 char + 1 terminator + 1 dummy (useless) */

    BDS_LOAD_OPTION* lpBoot_Entry = NULL;
    for (int i = 0; i < GetBootCount(); i++) {
        swprintf_s(BootEntry, 10, L"Boot%04X", pBoot_Order[i]);
        if (0==lstrcmpW(lpBootEntry, BootEntry)) {
            lpBoot_Entry = GetBootEntry(lpBootEntry, pBoot_Order[i]);
        }
    }
    //wprintf_s(lpBootEntry);
    //wprintf_s(L":");
    //wprintf_s(L"    Attributre:0x%08x\n", lpBoot_Entry->Attributes);
    wprintf_s(L"    BootName:%s\n", lpBoot_Entry->Description);
    wprintf_s(L"    BootEntryID:%04x\n", lpBoot_Entry->LoadOptionIndex);
    //wprintf_s(L"    FilePathLength:%d\n", lpBoot_Entry->FilePathListLength);
    HARDDRIVE_DEVICE_PATH * hd = (HARDDRIVE_DEVICE_PATH *)(lpBoot_Entry->FilePathList);
    FILEPATH_DEVICE_PATH* fd = (FILEPATH_DEVICE_PATH *)((char*)(lpBoot_Entry->FilePathList) + 42);
    UINT8 ParNumArray[4] = { (hd->PartitionNumber)&0xFF ,(hd->PartitionNumber>>8)&0xFF,(hd->PartitionNumber >> 16) & 0xFF ,(hd->PartitionNumber >> 24) & 0xFF };
    //wprintf_s(L"    ParNum:%d\n", PARNUM(ParNumArray));
    //wprintf_s(L"    ParFormat:%d\n", hd->PartitionFormat);
    //wprintf_s(L"    ParSigType:%d\n", hd->SignatureType);
    /*wprintf_s(L"    ParSignature:{%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x}\n",
        hd->Signature[3],
        hd->Signature[2],
        hd->Signature[1],
        hd->Signature[0],
        hd->Signature[5],
        hd->Signature[4],
        hd->Signature[7],
        hd->Signature[6],
        hd->Signature[9],
        hd->Signature[8],
        hd->Signature[10],
        hd->Signature[11],
        hd->Signature[12],
        hd->Signature[13],
        hd->Signature[14],
        hd->Signature[15]); */
    WCHAR ParTitionPath[1024] = { 0 };
    wsprintf(ParTitionPath, L"\\\\\?\\Volume{%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x}\\",
        hd->Signature[3],
        hd->Signature[2],
        hd->Signature[1],
        hd->Signature[0],
        hd->Signature[5],
        hd->Signature[4],
        hd->Signature[7],
        hd->Signature[6],
        hd->Signature[8],
        hd->Signature[9],
        hd->Signature[10],
        hd->Signature[11],
        hd->Signature[12],
        hd->Signature[13],
        hd->Signature[14],
        hd->Signature[15]);

    TCHAR ParInfo[1024] = { 0 };
    CHAR DiskInfo[1024] = { 0 };
    EFI_GetVolumeInfo(ParTitionPath, PARNUM(ParNumArray), ParInfo, DiskInfo);
    printf_s("    HardDisk:%s\n", DiskInfo);
    wprintf_s(L"    Partition:%s\n", ParInfo);
    //wprintf(L"    File_Device_Length:%d\n", fd->Header.Length[0]);
    //memcpy(filename, fd->PathName, fd->Header.Length[0]);
    //wprintf_s(L"    FileName:%s\n", filename);
    wprintf_s(L"    BootFileName:%s\n", fd->PathName);
    //wprintf_s(L"    %x\n", lpBoot_Entry->FilePathList);
    //wprintf_s(L"    %s\n", lpBoot_Entry->OptionalData);


    return 0;
}

//#define D_TEST




int main(int argc,char * argv[])
{
    //Get arg
    int arg = -1;
    WCHAR szName[10] = { 0 };

    if (!promoteProcessPrivileges(SE_SYSTEM_ENVIRONMENT_NAME, TRUE)) {
    //if(!EnablePrivilege()){
        std::cout << "提权失败！" << std::endl;
        return -1;
    }

    if (argc == 1) {

        std::cout << "BootOrder:" << std::endl;
        EFI_GetBootOrder();

    }

    /*
    if (0 != GetALLDISK(&DevIn)) {
        wprintf_s(L"调用GetALLDISK出错！");
        FreeALLDISK(&DevIn);
        return -1;
    }
    HANDLE hdisk = INVALID_HANDLE_VALUE ;
    PSTORAGE_DEVICE_DESCRIPTOR pDevDesc;
    STORAGE_PROPERTY_QUERY Query;
    DWORD dwOutBytes;


    Query.PropertyId = StorageDeviceProperty;
    Query.QueryType = PropertyStandardQuery;
    
    pDevDesc = (PSTORAGE_DEVICE_DESCRIPTOR)new BYTE[sizeof(STORAGE_DEVICE_DESCRIPTOR) + 512 - 1];
    
    ZeroMemory(pDevDesc, sizeof(STORAGE_DEVICE_DESCRIPTOR) + 512 - 1);
    pDevDesc->Size = sizeof(STORAGE_DEVICE_DESCRIPTOR) + 512 - 1;
    //wprintf_s(L"%d", pDevDesc->Size);
    
    //struct EFI_Disk_Info efi_disk[1024] = { 0 };
    //g_efi_disk = efi_disk;



    GET_LENGTH_INFORMATION disk_len = { 0 };
    STORAGE_DEVICE_NUMBER diskNumber = { 0 };
    DWORD sizePar = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + sizeof(PARTITION_INFORMATION_EX) * 128;
    PDRIVE_LAYOUT_INFORMATION_EX ParTable = (PDRIVE_LAYOUT_INFORMATION_EX)new BYTE[sizePar];
    ZeroMemory(ParTable, sizePar);
    int devIndex = 0;

    TCHAR szPathName[MAX_PATH + 1] = { 0 };

    for (devIndex = 0; devIndex < DevIn.szCount; devIndex++) {
        wprintf_s(L"DiskInfo:%s\n", DevIn.deviceInterfaceDetailData_[devIndex]->DevicePath);
        hdisk = CreateFile(DevIn.deviceInterfaceDetailData_[devIndex]->DevicePath, 
            GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 
            NULL,
            OPEN_EXISTING, 
            FILE_ATTRIBUTE_NORMAL, NULL);
        if (INVALID_HANDLE_VALUE == hdisk) {
            wprintf_s(L"createfile error! error_code:%d\n", GetLastError());
            return -1;
        }
        if (!DeviceIoControl(hdisk,
            IOCTL_STORAGE_QUERY_PROPERTY,
            &Query, sizeof(STORAGE_PROPERTY_QUERY),
            pDevDesc, pDevDesc->Size,
            &dwOutBytes,
            NULL)
            ){ 
            wprintf_s(L"DeviceIoControl Error! line:238 Error_Code:%d %d\n", GetLastError(),dwOutBytes);
            
            
            
        }
        if (!DeviceIoControl(hdisk, IOCTL_DISK_GET_LENGTH_INFO,
            NULL,
            0,
            &disk_len, sizeof(disk_len), &dwOutBytes, NULL)) {

            wprintf_s(L"DeviceIoControl Error! line:248 Error_Code:%d %d\n", GetLastError(), dwOutBytes); //122是正常的。

        }

        if (!DeviceIoControl(hdisk,
            IOCTL_STORAGE_GET_DEVICE_NUMBER,
            NULL,
            0,
            &diskNumber,
            sizeof(STORAGE_DEVICE_NUMBER),
            &dwOutBytes,
            NULL)) {

            wprintf_s(L"DeviceIoControl Error! line:272 Error_Code:%d %d\n", GetLastError(), dwOutBytes);

        }
        dwOutBytes = 0;
        if (!DeviceIoControl(hdisk,
            IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
            NULL,
            0,
            ParTable,
            sizePar,
            &dwOutBytes,
            NULL)) {

            wprintf_s(L"DeviceIoControl Error! line:285 Error_Code:%d %d\n", GetLastError(), dwOutBytes);

        }

 

        g_efi_disk[devIndex] = (struct EFI_Disk_Info*)new (struct EFI_Disk_Info);
        CloseHandle(hdisk);
        hdisk = INVALID_HANDLE_VALUE;
        //wprintf_s(L"%d %d\n", dwOutBytes, pDevDesc->Size);
        char* p_char_string = (char*)pDevDesc;
        stringstream sz_buffer;
        g_efi_disk[devIndex]->productor = std::string(&p_char_string[pDevDesc->ProductIdOffset]);
        g_efi_disk[devIndex]->version = std::string(&p_char_string[pDevDesc->ProductRevisionOffset]);
        g_efi_disk[devIndex]->vendor = std::string(&p_char_string[pDevDesc->VendorIdOffset]);
        int buffer_disk_len = disk_len.Length.QuadPart / 1024 / 1024 / 1024;
        sz_buffer << buffer_disk_len << "GB";
        g_efi_disk[devIndex]->length = sz_buffer.str();
        g_efi_disk[devIndex]->number = std::to_string(diskNumber.DeviceNumber);
        printf_s("Productor:%s\n", g_efi_disk[devIndex]->productor.c_str());
        printf_s("Version:%s\n", g_efi_disk[devIndex]->version.c_str());
        printf_s("Vendor:%s\n", g_efi_disk[devIndex]->vendor.c_str());
        printf_s("Length:%s\n", g_efi_disk[devIndex]->length.c_str());
        printf_s("Number:%s\n", g_efi_disk[devIndex]->number.c_str());

        wprintf_s(L"===================================================================================\n");
        wprintf_s(L"ParTable:\n");
        wprintf_s(L"ParTableType:%d\n",ParTable->PartitionStyle);
        wprintf_s(L"ParCount:%d\n", ParTable->PartitionCount);
        
        wprintf_s(L"Par0:\nParNum:%d\nParName:%s\n", ParTable->PartitionEntry[0].PartitionNumber,ParTable->PartitionEntry[0].Gpt.Name);
        wprintf_s(L"PatLength:%dMB\n", ParTable->PartitionEntry[0].PartitionLength.QuadPart / 1024 / 1024);
        wprintf_s(L"ParType:{%04x-%02x-%02x-%02x%02x-%02x%02x%02x%02x%02x%02x}\n",
            ParTable->PartitionEntry[0].Gpt.PartitionType.Data1,
            ParTable->PartitionEntry[0].Gpt.PartitionType.Data2,
            ParTable->PartitionEntry[0].Gpt.PartitionType.Data3,
            ParTable->PartitionEntry[0].Gpt.PartitionType.Data4[0],
            ParTable->PartitionEntry[0].Gpt.PartitionType.Data4[1],
            ParTable->PartitionEntry[0].Gpt.PartitionType.Data4[2],
            ParTable->PartitionEntry[0].Gpt.PartitionType.Data4[3],
            ParTable->PartitionEntry[0].Gpt.PartitionType.Data4[4],
            ParTable->PartitionEntry[0].Gpt.PartitionType.Data4[5],
            ParTable->PartitionEntry[0].Gpt.PartitionType.Data4[6],
            ParTable->PartitionEntry[0].Gpt.PartitionType.Data4[7]);
        wprintf_s(L"ParGUID:{%04x-%02x-%02x-%02x%02x-%02x%02x%02x%02x%02x%02x}\n",
            ParTable->PartitionEntry[0].Gpt.PartitionId.Data1,
            ParTable->PartitionEntry[0].Gpt.PartitionId.Data2,
            ParTable->PartitionEntry[0].Gpt.PartitionId.Data3,
            ParTable->PartitionEntry[0].Gpt.PartitionId.Data4[0],
            ParTable->PartitionEntry[0].Gpt.PartitionId.Data4[1],
            ParTable->PartitionEntry[0].Gpt.PartitionId.Data4[2],
            ParTable->PartitionEntry[0].Gpt.PartitionId.Data4[3],
            ParTable->PartitionEntry[0].Gpt.PartitionId.Data4[4],
            ParTable->PartitionEntry[0].Gpt.PartitionId.Data4[5],
            ParTable->PartitionEntry[0].Gpt.PartitionId.Data4[6],
            ParTable->PartitionEntry[0].Gpt.PartitionId.Data4[7]);
        
        wprintf_s(L"返回的字节数:%d\n", dwOutBytes);
        //ParTable = { 0 };
        wprintf_s(L"===================================================================================\n");
        wsprintf(szPathName, L"\\\\\?\\Volume{%04x-%02x-%02x-%02x%02x-%02x%02x%02x%02x%02x%02x}\\",
            ParTable->PartitionEntry[0].Gpt.PartitionId.Data1,
            ParTable->PartitionEntry[0].Gpt.PartitionId.Data2,
            ParTable->PartitionEntry[0].Gpt.PartitionId.Data3,
            ParTable->PartitionEntry[0].Gpt.PartitionId.Data4[0],
            ParTable->PartitionEntry[0].Gpt.PartitionId.Data4[1],
            ParTable->PartitionEntry[0].Gpt.PartitionId.Data4[2],
            ParTable->PartitionEntry[0].Gpt.PartitionId.Data4[3],
            ParTable->PartitionEntry[0].Gpt.PartitionId.Data4[4],
            ParTable->PartitionEntry[0].Gpt.PartitionId.Data4[5],
            ParTable->PartitionEntry[0].Gpt.PartitionId.Data4[6],
            ParTable->PartitionEntry[0].Gpt.PartitionId.Data4[7]);
        wprintf_s(L"%s\n", szPathName);
    }
    delete pDevDesc;
    FreeALLDISK(&DevIn);


    
    
    TCHAR volumename[256] = { 0 };
    TCHAR filesystemname[256] = { 0 };
    GetVolumeInformation(szPathName, volumename, 256, NULL, NULL, NULL, filesystemname, 256);
    wprintf_s(L"volumename:%s\n", volumename);
    wprintf_s(L"filesystemname:%s\n", filesystemname);

    DWORD SectorPerCluster = 0;
    DWORD BytesPerSector = 0;
    DWORD NumOfFreeCluster = 0;
    DWORD TotalNumOfCluster = 0;

    GetDiskFreeSpace(szPathName, &SectorPerCluster, &BytesPerSector, &NumOfFreeCluster, &TotalNumOfCluster);
    wprintf_s(L"SectorPerCluster:%d\nBytesPerSector:%d\nNumOfFreeCluster:%d\n,TotalNumOfCluster:%d\n", SectorPerCluster,BytesPerSector,NumOfFreeCluster,TotalNumOfCluster);
    */

    while ((arg=GetOpt(argc, argv))!=ARG_POS_END)
    {   
        //std::cout << "arg:" << arg << "\n" << std::endl;
        switch (arg)
        {
        case BOOTORDER:
            std::cout << "BootOrder:" << std::endl;
            EFI_GetBootOrder();
            break;
        case BOOTOPTION:
            
            if (strlen(argv[arg_pos]) != 8) {
                std::cout << "BootOption Error! \n" << std::endl;
            }
            //printf_s("%s\n",argv[arg_pos]);
            MultiByteToWideChar(CP_ACP, 0, argv[arg_pos], strlen(argv[arg_pos]) + 1, szName, 10);
            std::cout << "BootOption:" << std::endl;

            wprintf_s(L"    BootEntry:%s\n", szName);
            EFI_GetBootEntry(szName);
            arg_pos++;
            break;
        case ALL:
            std::cout << "ALl\n" << std::endl;
            break;
        case ADD:
            std::cout << "ADD Start!\n" << std::endl;

            std::cout << "ADD Done!\n" << std::endl;
        default:
            break;
        }
    }

    /*
    for (int freeIndex = 0; freeIndex < devIndex; freeIndex++) {
       delete g_efi_disk[freeIndex];
    }*/

    return 0;
#ifdef D_TEST
    system("pause");
#endif // 
}


// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
