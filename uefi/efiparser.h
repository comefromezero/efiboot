#include "efi_types.h"


#define GET_ATTRIBUTES(buf)			 (UINT32) (buf[3]<<24 | buf[2]<<16 | buf[1]<<8 | buf[0])//(UINT32)buf[0] 在硬盘上存储的应该是大端序，这里需要处理一下，转换为小端序。如果是在大端序机器上，应该不用转转。
#define GET_FILELISTLENGTH(buf)		 (UINT16) (buf[5]<<8 | buf[4])						  //(UINT16)buf[4]

#define DESCRIPTION_OFFSET(buf)			&buf[6]
#define GET_DESCRIPTION_LENGTH(buf)		wcslen((WCHAR*)DESCRIPTION_OFFSET(buf)) + 1
#define ALLOCATE_WCHAR_STRING(count)	(WCHAR*)calloc(count,sizeof(WCHAR));


#define FILEPATHLIST_OFFSET(buf,offset)		(UINT8*)(DESCRIPTION_OFFSET_PTR(buffer) + offset);


#define BUFFER_SIZE	4096

extern BDS_LOAD_OPTION* GetBootEntry(LPCWSTR BootEntry, int id);
extern void GetFilePathList(BDS_LOAD_OPTION* BdsLoadOption, char* buffer, int descSize);
extern BOOL ValidateBootEntry(UINT8* buffer);