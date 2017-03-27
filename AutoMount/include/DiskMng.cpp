#include "stdafx.h"
#include "DiskMng.h"

#define MAX_DEVICE 26
#define INTERFACE_DETAIL_SIZE 512

CDiskMng::CDiskMng(void)
{
}


CDiskMng::~CDiskMng(void)
{
}

/******************************************************************************
* Function: initialize the disk and create partitions
* input: disk, disk name
*        parNum, partition number
* output: N/A
* return: Succeed, 0
*         Fail, -1
******************************************************************************/
DWORD CDiskMng::CreateDisk(DWORD disk, WORD partNum)
{
    BOOL result;                  // results flag
    DWORD readed;                 // discard results

    YCHAR diskPath[MAX_PATH] = {0};
    Ysprintf(diskPath, MAX_PATH, _YTEXT("\\\\.\\PhysicalDrive%d"), disk);

    //分区数量 
    BYTE actualPartNum;
    actualPartNum = 1;
//     if (partNum > actualPartNum)
//     {
//         return (WORD)-1;
//     }

    //打开设备
    HANDLE hDevice = CreateFile(
        diskPath,
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE,
        NULL,           //default security attributes 
        OPEN_EXISTING, // disposition 
        0,              // file attributes 
        NULL
        );

    if (hDevice == INVALID_HANDLE_VALUE) // cannot open the drive
    {
        fprintf(stderr, "CreateFile() Error: %ld\n", GetLastError());
        return DWORD(-1);
    }

    //初始化硬盘
    CREATE_DISK newDisk;

    //格式MBR
    newDisk.PartitionStyle = PARTITION_STYLE_MBR;

    //签名 当前时间
    newDisk.Mbr.Signature = (DWORD)time(NULL); ;

    result = DeviceIoControl(
        hDevice,
        IOCTL_DISK_CREATE_DISK,
        &newDisk,
        sizeof(CREATE_DISK),
        NULL,
        0,
        &readed,
        NULL
        );
    if (!result)
    {
        fprintf(stderr, "IOCTL_DISK_CREATE_DISK Error: %ld\n", GetLastError());
        (void)CloseHandle(hDevice);
        return DWORD(-1);
    }

    //刷新分区表
    result = DeviceIoControl(
        hDevice,
        IOCTL_DISK_UPDATE_PROPERTIES,
        NULL,
        0,
        NULL,
        0,
        &readed,
        NULL
        );
    if (!result)
    {
        fprintf(stderr, "IOCTL_DISK_UPDATE_PROPERTIES Error: %ld\n", GetLastError());
        (void)CloseHandle(hDevice);
        return DWORD(-1);
    }

    //格式化整个磁盘
    //.....
    //

    //创建分区

    //获取磁盘信息（物理硬盘）
    DISK_GEOMETRY pdg;
    DWORD ret = GetDriveGeometry(diskPath, &pdg);
    if ((DWORD)-1 == ret)
    {
        return ret;
    }

    DWORD sectorSize = pdg.BytesPerSector;
    LARGE_INTEGER diskSize;
    LARGE_INTEGER partSize;
    diskSize.QuadPart = pdg.Cylinders.QuadPart * pdg.TracksPerCylinder *
        pdg.SectorsPerTrack * pdg.BytesPerSector;       //calculate the disk size;

    partSize.QuadPart = diskSize.QuadPart / partNum;

    DWORD layoutStructSize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + (actualPartNum - 1) * sizeof(PARTITION_INFORMATION_EX);
    DRIVE_LAYOUT_INFORMATION_EX *dl;
    dl = (DRIVE_LAYOUT_INFORMATION_EX*)malloc(layoutStructSize);
    if (NULL == dl)
    {
        (void)CloseHandle(hDevice);
        return (WORD)-1;
    }

    
    dl->PartitionStyle = (DWORD)PARTITION_STYLE_MBR;
    dl->PartitionCount = actualPartNum;
    dl->Mbr.Signature = newDisk.Mbr.Signature;

    //clear the unused partitions
    for (DWORD i = 0; i < actualPartNum; i++){
        dl->PartitionEntry[i].RewritePartition = 1;
        dl->PartitionEntry[i].Mbr.PartitionType = PARTITION_ENTRY_UNUSED;
    }

    //设置各个分区详细信息                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
    for (DWORD i = 0; i < partNum; i++){
        dl->PartitionEntry[i].PartitionStyle = PARTITION_STYLE_MBR;
        dl->PartitionEntry[i].StartingOffset.QuadPart =
            (partSize.QuadPart * i) + ((LONGLONG)(pdg.SectorsPerTrack) * (LONGLONG)(pdg.BytesPerSector));   //32256
        dl->PartitionEntry[i].PartitionLength.QuadPart = partSize.QuadPart;
        dl->PartitionEntry[i].PartitionNumber = i + 1;
        dl->PartitionEntry[i].RewritePartition = TRUE;
        dl->PartitionEntry[i].Mbr.PartitionType = PARTITION_IFS;
        dl->PartitionEntry[i].Mbr.BootIndicator = FALSE;
        dl->PartitionEntry[i].Mbr.RecognizedPartition = TRUE;
        dl->PartitionEntry[i].Mbr.HiddenSectors =
            pdg.SectorsPerTrack + (DWORD)((partSize.QuadPart / sectorSize) * i);
    }
    //设置 layout 
    result = DeviceIoControl(
        hDevice,
        IOCTL_DISK_SET_DRIVE_LAYOUT_EX,
        dl,
        layoutStructSize,
        NULL,
        0,
        &readed,
        NULL
        );
    if (!result)
    {
        fprintf(stderr, "IOCTL_DISK_SET_DRIVE_LAYOUT_EX Error: %ld\n", GetLastError());
        free(dl);
        (void)CloseHandle(hDevice);
        return DWORD(-1);
    }

    //刷新分区表
    result = DeviceIoControl(
        hDevice,
        IOCTL_DISK_UPDATE_PROPERTIES,
        NULL,
        0,
        NULL,
        0,
        &readed,
        NULL
        );
    if (!result)
    {
        fprintf(stderr, "IOCTL_DISK_UPDATE_PROPERTIES Error: %ld\n", GetLastError());
        free(dl);
        (void)CloseHandle(hDevice);
        return DWORD(-1);
    }

    free(dl);
    CloseHandle(hDevice);
    Sleep(3000);                        //wait the operations take effect
    return 0;
}

DWORD CDiskMng::GetDriveGeometry(YCHAR* strname, DISK_GEOMETRY *pdg)
{
    DWORD dwBytesReturned = 0;

    HANDLE hDev = CreateFile(strname, 
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,  
        NULL,  
        OPEN_EXISTING,  
        0,  
        NULL);
    if (hDev == INVALID_HANDLE_VALUE)
    {
        return -1;
    }

    DeviceIoControl(hDev, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &dwBytesReturned, NULL);

    OVERLAPPED ol = {0};
    if (! DeviceIoControl(hDev, IOCTL_DISK_GET_DRIVE_GEOMETRY,
        NULL, 0, pdg, sizeof(DISK_GEOMETRY),
        &dwBytesReturned, NULL) )
    {
//         LPVOID lpMsgBuf;
//         FormatMessage( 
//             FORMAT_MESSAGE_ALLOCATE_BUFFER | 
//             FORMAT_MESSAGE_FROM_SYSTEM | 
//             FORMAT_MESSAGE_IGNORE_INSERTS,
//             NULL,
//             GetLastError(),
//             0, // Default language
//             (LPTSTR) &lpMsgBuf,
//             0,
//             NULL 
//             );
//         // Process any inserts in lpMsgBuf.
//         // ...
//         // Display the string.
//         ::MessageBox(NULL, (LPCTSTR)lpMsgBuf, L"ERROR", MB_OK | MB_ICONINFORMATION );
//         // Free the buffer.
//         LocalFree( lpMsgBuf );
    }

    DeviceIoControl(hDev, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &dwBytesReturned, NULL);
    CloseHandle(hDev);
    return 0;
}

/******************************************************************************
* Function: get the disk's drive layout infomation
* input: disk, disk name
* output: drive layout info
* return: Succeed, 0
*         Fail, -1
******************************************************************************/
DWORD CDiskMng::GetDiskDriveLayout(YCHAR* disk, DRIVE_LAYOUT_INFORMATION_EX *driveLayout)
{
    BOOL result;                            // results flag
    DWORD readed;                           // discard results

    HANDLE hDevice = CreateFile(
        disk, // drive to open
        GENERIC_READ | GENERIC_WRITE,       // access to the drive
        FILE_SHARE_READ | FILE_SHARE_WRITE, //share mode
        NULL,                               // default security attributes
        OPEN_EXISTING,                      // disposition
        0,                                  // file attributes
        NULL                                // do not copy file attribute
        );
    if (hDevice == INVALID_HANDLE_VALUE)    // cannot open the drive
    {
        fprintf(stderr, "CreateFile() Error: %ld\n", GetLastError());
        return DWORD(-1);
    }

    result = DeviceIoControl(
        hDevice,                            // handle to device
        IOCTL_DISK_GET_DRIVE_LAYOUT_EX,     // dwIoControlCode
        NULL,                               // lpInBuffer
        0,                                  // nInBufferSize
        driveLayout,                        // output buffer
        sizeof(*driveLayout),               // size of output buffer
        &readed,                            // number of bytes returned
        NULL                                // OVERLAPPED structure
        );
    if (!result)
    {
        fprintf(stderr, "IOCTL_DISK_GET_DRIVE_LAYOUT_EX Error: %ld\n", GetLastError());
        (void)CloseHandle(hDevice);
        return DWORD(-1);
    }

    (void)CloseHandle(hDevice);
    return 0;
}

/******************************************************************************
* Function: delete the partition layout of the disk
* input: disk, disk name
* output: N/A
* return: Succeed, 0
*         Fail, -1
******************************************************************************/
DWORD CDiskMng::DestroyDisk(DWORD disk)
{
    BOOL result;                            // results flag
    DWORD readed;                           // discard results
    

    YCHAR diskPath[MAX_PATH] = {0};

    Ysprintf(diskPath, _YTEXT("\\\\.\\PhysicalDrive%d"), disk);

    HANDLE hDevice = CreateFile(
        diskPath, // drive to open
        GENERIC_READ | GENERIC_WRITE,       // access to the drive
        FILE_SHARE_READ | FILE_SHARE_WRITE, //share mode
        NULL,                               // default security attributes
        OPEN_EXISTING,                      // disposition
        0,                                  // file attributes
        NULL                                // do not copy file attribute
        );
    if (hDevice == INVALID_HANDLE_VALUE)    // cannot open the drive
    {
        fprintf(stderr, "CreateFile() Error: %ld\n", GetLastError());
        return DWORD(-1);
    }

    result = DeviceIoControl(
        hDevice,                            // handle to device
        IOCTL_DISK_DELETE_DRIVE_LAYOUT,     // dwIoControlCode
        NULL,                               // lpInBuffer
        0,                                  // nInBufferSize
        NULL,                               // lpOutBuffer
        0,                                  // nOutBufferSize
        &readed,                            // number of bytes returned
        NULL                                // OVERLAPPED structure
        );
    if (!result)
    {
        //fprintf(stderr, "IOCTL_DISK_DELETE_DRIVE_LAYOUT Error: %ld\n", GetLastError());
        (void)CloseHandle(hDevice);
        return DWORD(-1);
    }

    //fresh the partition table
    result = DeviceIoControl(
        hDevice,
        IOCTL_DISK_UPDATE_PROPERTIES,
        NULL,
        0,
        NULL,
        0,
        &readed,
        NULL
        );
    if (!result)
    {
        fprintf(stderr, "IOCTL_DISK_UPDATE_PROPERTIES Error: %ld\n", GetLastError());
        (void)CloseHandle(hDevice);
        return DWORD(-1);
    }

    (void)CloseHandle(hDevice);
    return 0;
}

/******************************************************************************
* Function: get disk's physical number from its drive letter
*           e.g. C-->0 (C: is on disk0)
* input: letter, drive letter
* output: N/A
* return: Succeed, disk number
*         Fail, -1
******************************************************************************/

DWORD CDiskMng::GetPhysicalDriveFromPartitionLetter(YCHAR letter)
{
    BOOL result;                            // results flag
    DWORD readed;                           // discard results

    YCHAR path[MAX_PATH] = {0};
    Ysprintf(path,_YTEXT("\\\\.\\%c:"), letter);

    HANDLE hDevice = CreateFile(path,       // drive to open
        GENERIC_READ | GENERIC_WRITE,       // access to the drive
        FILE_SHARE_READ | FILE_SHARE_WRITE, //share mode
        NULL,                               // default security attributes
        OPEN_EXISTING,                      // disposition
        0,                                  // file attributes
        NULL);                              // do not copy file attribute

    if (hDevice == INVALID_HANDLE_VALUE)    // cannot open the drive
    {
        fprintf(stderr, "CreateFile() Error: %ld\n", GetLastError());
        return DWORD(-1);
    }

    STORAGE_DEVICE_NUMBER number;
    result = DeviceIoControl(
        hDevice,                            // handle to device
        IOCTL_STORAGE_GET_DEVICE_NUMBER,    // dwIoControlCode
        NULL,                               // lpInBuffer
        0,                                  // nInBufferSize
        &number,                            // output buffer
        sizeof(number),                     // size of output buffer
        &readed,                            // number of bytes returned
        NULL                                // OVERLAPPED structure
        );

    if (!result) // fail
    {
        fprintf(stderr, "IOCTL_STORAGE_GET_DEVICE_NUMBER Error: %ld\n", GetLastError());
        CloseHandle(hDevice);
        return (DWORD)-1;
    }

    CloseHandle(hDevice);

    return number.DeviceNumber;

}

/******************************************************************************
* Function: get disk's drive letters from physical number
*           e.g. 0-->{C, D, E} (disk0 has 3 drives, C:, D: and E:)
* input: phyDriveNumber, disk's physical number
* output: letters, letters array
* return: Succeed, the amount of letters
*         Fail, -1
******************************************************************************/

DWORD CDiskMng::GetPartitionLetterFromPhysicalDrive(DWORD phyDriveNumber, YCHAR *letters)
{
    DWORD bmLetters = GetLogicalDrives();

    if (0 == bmLetters)
    {
        return (DWORD)-1;
    }

    DWORD letterNum = 0;
    for (WORD i = 0; i < sizeof(DWORD) * 8; i++)
    {
        DWORD mask = 0x1u << i;
        if ((mask & bmLetters) == 0)                //get one letter
        {
            continue;
        }

        YCHAR letter = (YCHAR)(0x41 + i);             //ASCII change

        YCHAR path[MAX_PATH] = {0};
        Ysprintf(path, _YTEXT("%c:\\"), letter);

        DWORD driveType = GetDriveType(path);

        if (driveType != DRIVE_FIXED)
        {
            bmLetters &= ~mask;                     //clear this bit
            continue;
        }

        DWORD diskNumber = GetPhysicalDriveFromPartitionLetter(letter);

        if (diskNumber != phyDriveNumber)
        {
            bmLetters &= ~mask;                     //clear this bit
            continue;
        }
        letterNum++;
    }

    //build the result
    if (NULL == letters)
    {
        return letterNum;
    }

    memset(letters , 0, sizeof(YCHAR)*letterNum);

    for (WORD i = 0; i < sizeof(DWORD) * 8; i++)
    {
        DWORD mask = 0x1u << i;
        if ((mask & bmLetters) == 0)
        {
            continue;
        }

        //ASCII change
        *letters = (YCHAR)(0x41 + i);
        letters++;
    }

    return letterNum;
}

/******************************************************************************
* Function: get the number of disk which the system installed on
* input: N/A
* output: N/A
* return: Succeed, disk number
*         Fail, -1
******************************************************************************/

DWORD CDiskMng::GetSystemDiskPhysicalNumber()
{
    YCHAR sysPath[MAX_PATH];

    DWORD ret = GetSystemDirectory(sysPath, sizeof(sysPath));

    if (ret == 0)
    {
        fprintf(stderr, "GetSystemDirectory() Error: %ld\n", GetLastError());
        return (DWORD)-1;
    }

    YCHAR diskLetter = sysPath[0];
    DWORD diskNumber = GetPhysicalDriveFromPartitionLetter(diskLetter);

    return diskNumber;
}

/******************************************************************************
* Function:
* input: disk, disk name
* output: N/A
* return: Succeed, 0
*         Fail, 1
******************************************************************************/
DWORD CDiskMng::FormatVolume(YCHAR letter)
{
    DWORD ret;
    char cmd[MAX_PATH] = {0};
    sprintf_s(cmd, MAX_PATH, "format %c: /FS:NTFS /Q /Y", letter);
    ret = (DWORD)system(cmd);
    return ret;

}

/******************************************************************************
* Function: get device path from GUID
* input: lpGuid, GUID pointer
* output: pszDevicePath, device paths
* return: Succeed, the amount of found device paths
*         Fail, -1
******************************************************************************/
DWORD CDiskMng::GetDevicePath(LPGUID lpGuid, YCHAR **pszDevicePath)
{
    DWORD nCount;
    BOOL result;

    //get a handle to a device information set
    HDEVINFO hDevInfoSet = SetupDiGetClassDevs(
        lpGuid,                                     // class GUID
        NULL,                                       // Enumerator
        NULL,                                       // hwndParent
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE       // present devices
        );

    //fail...
    if (hDevInfoSet == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "IOCTL_STORAGE_GET_DEVICE_NUMBER Error: %ld\n", GetLastError());
        return (DWORD)-1;
    }

    YCHAR* p = new YCHAR[INTERFACE_DETAIL_SIZE];
    if (p == NULL)
    {
        return (DWORD)-1;
    }
    PSP_DEVICE_INTERFACE_DETAIL_DATA pDetail = (PSP_DEVICE_INTERFACE_DETAIL_DATA)p;
    pDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

    nCount = 0;
    result = TRUE;

    // device index = 0, 1, 2... test the device interface one by one
    while (result)
    {
        SP_DEVICE_INTERFACE_DATA ifdata;
        ifdata.cbSize = sizeof(ifdata);

        //enumerates the device interfaces that are contained in a device information set
        result = SetupDiEnumDeviceInterfaces(
            hDevInfoSet,                            // DeviceInfoSet
            NULL,                                   // DeviceInfoData
            lpGuid,                                 // GUID
            nCount,                                 // MemberIndex
            &ifdata                                 // DeviceInterfaceData
            );
        if (result)
        {
            // get details about a device interface
            result = SetupDiGetDeviceInterfaceDetail(
                hDevInfoSet,                        // DeviceInfoSet
                &ifdata,                            // DeviceInterfaceData
                pDetail,                            // DeviceInterfaceDetailData
                INTERFACE_DETAIL_SIZE,              // DeviceInterfaceDetailDataSize
                NULL,                               // RequiredSize
                NULL                                // DeviceInfoData
                );
            DWORD ret = GetLastError();
            if (result)
            {
                // copy the path to output 
                int nlen = wcslen(pDetail->DevicePath);
                Ystrcpy(pszDevicePath[nCount], nlen+1, pDetail->DevicePath);
                //printf("%s\n", pDetail->DevicePath);
                nCount++;
            }
        }
    }

    delete[] pDetail;
    pDetail = NULL;

    SetupDiDestroyDeviceInfoList(hDevInfoSet);
   
    return nCount;
}

/******************************************************************************
* Function: get all present disks' physical number
* input: N/A
* output: ppDisks, array of disks' physical number
* return: Succeed, the amount of present disks
*         Fail, -1
******************************************************************************/
DWORD CDiskMng::GetAllPresentDisks(DWORD *pDisks)
{
    HANDLE hDevice;
    STORAGE_DEVICE_NUMBER number;
    BOOL result;
    DWORD readed;

    YCHAR *szDevicePath[MAX_DEVICE] = {NULL};  
    for (int i = 0; i < MAX_DEVICE; i++)
    {
        szDevicePath[i] = new YCHAR[256];
        if (NULL == szDevicePath[i])
        {
            for (int j = 0; j < i; j++)
            {
                delete[] szDevicePath[i];
            }
            return (DWORD)-1;
        }
    }

    // get the device paths
    int nDevice = GetDevicePath(const_cast<LPGUID>(&GUID_DEVINTERFACE_DISK), szDevicePath);
    if ((DWORD)-1 == nDevice)
    {
        for (int i = 0; i < MAX_DEVICE; i++)
        {
            free(szDevicePath[i]);
        }
        return (DWORD)-1;
    }

    // get the disk's physical number one by one
    for (int i = 0; i < nDevice; i++)
    {
        hDevice = CreateFile(
            szDevicePath[i],                        // drive to open
            GENERIC_READ | GENERIC_WRITE,           // access to the drive
            FILE_SHARE_READ | FILE_SHARE_WRITE,     //share mode
            NULL,                                   // default security attributes
            OPEN_EXISTING,                          // disposition
            0,                                      // file attributes
            NULL                                    // do not copy file attribute
            );
        if (hDevice == INVALID_HANDLE_VALUE)        // cannot open the drive
        {
            for (int j = 0; j < MAX_DEVICE; j++)
            {
                free(szDevicePath[j]);
            }
            fprintf(stderr, "CreateFile() Error: %ld\n", GetLastError());
            return DWORD(-1);
        }
        result = DeviceIoControl(
            hDevice,                                // handle to device
            IOCTL_STORAGE_GET_DEVICE_NUMBER,        // dwIoControlCode
            NULL,                                   // lpInBuffer
            0,                                      // nInBufferSize
            &number,                                // output buffer
            sizeof(number),                         // size of output buffer
            &readed,                                // number of bytes returned
            NULL                                    // OVERLAPPED structure
            );
        if (!result)                                // fail
        {
            fprintf(stderr, "IOCTL_STORAGE_GET_DEVICE_NUMBER Error: %ld\n", GetLastError());
            for (int j = 0; j < MAX_DEVICE; j++)
            {
                free(szDevicePath[j]);
            }
            (void)CloseHandle(hDevice);
            return (DWORD)-1;
        }
        *(pDisks + i) = number.DeviceNumber;

        (void)CloseHandle(hDevice);
    }

    for (int i = 0; i < MAX_DEVICE; i++)
    {
        delete[] szDevicePath[i];
        szDevicePath[i] = NULL;
    }
    return nDevice;
}

DWORD CDiskMng::GetDiskCount()
{
    HKEY hKEY = NULL;
    long lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
        _YTEXT("SYSTEM\\CurrentControlSet\\services\\Disk\\Enum"),
        0, KEY_READ, &hKEY);

    if ( ERROR_SUCCESS != lRet)
    {
        lRet = GetLastError();
        return lRet;
    }

    DWORD dwDiskCount = 0;
    DWORD dwValueLen = sizeof(DWORD);
    DWORD keyType = REG_DWORD;

    lRet = RegQueryValueEx(hKEY, _YTEXT("Count"), 
        NULL, &keyType, (BYTE*)&dwDiskCount, &dwValueLen);

    if ( ERROR_SUCCESS != lRet)
    {
        RegCloseKey(hKEY);
        lRet = GetLastError();
        return lRet;
    }

    RegCloseKey(hKEY);

    return dwDiskCount;
}