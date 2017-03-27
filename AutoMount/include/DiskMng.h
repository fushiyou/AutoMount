#pragma once
#include <windows.h>
#include <setupapi.h>
#include <time.h>
#include "helper.h"

#pragma comment(lib,"Setupapi.lib")  


class CDiskMng
{
public:
    CDiskMng(void);
    ~CDiskMng(void);

    //获取总磁盘数量

    //初始化、创建磁盘
    DWORD CreateDisk(DWORD disk, WORD partNum = 1);

    //查询磁盘分区信息
    DWORD GetDiskDriveLayout(YCHAR *disk, DRIVE_LAYOUT_INFORMATION_EX *driveLayout);

    //删除磁盘分区表
    DWORD DestroyDisk(DWORD disk);

    //根据逻辑分区号查询物理磁盘号
    DWORD GetPhysicalDriveFromPartitionLetter(YCHAR letter);

    //查询某块物理磁盘上的所有分区
    DWORD GetPartitionLetterFromPhysicalDrive(DWORD phyDriveNumber, YCHAR *letters);

    //获取操作系统所在的物理磁盘号
    DWORD GetSystemDiskPhysicalNumber();

    //格式化分区 C,D,E
    DWORD FormatVolume(YCHAR letter);

    //查询磁盘物理信息
    DWORD GetDriveGeometry(YCHAR* strname, DISK_GEOMETRY *pdg);

    //获取所有物理磁盘编号
    DWORD GetAllPresentDisks(DWORD *pDisks);

    //获取物理磁盘总数
    DWORD GetDiskCount();

protected:
    DWORD GetDevicePath(LPGUID lpGuid, YCHAR **pszDevicePath);
};

