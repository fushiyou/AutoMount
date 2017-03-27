// AutoMount.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "include/DiskMng.h"

#define AUTO_MOUNT_NAME  _YTEXT("AutoMount")
#define AUTO_MOUNT_DESC  _YTEXT("Auto mount the physical's disk.")

#define AUTOMOUNT_ACCEPTED_CONTROLS\
    (SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_SESSIONCHANGE)

SERVICE_STATUS_HANDLE g_status_handle = NULL;
SERVICE_STATUS g_service_status;
bool g_Running = false;

DWORD WINAPI control_handler(DWORD control, DWORD event_type, LPVOID event_data,
                             LPVOID context)
{
    DWORD ret = NO_ERROR;

    switch (control)
    {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN: 
        {
        g_service_status.dwCurrentState = SERVICE_STOP_PENDING;
        SetServiceStatus(g_status_handle, &g_service_status);
        g_Running = false;
        break;
                                   }
    case SERVICE_CONTROL_INTERROGATE:
        SetServiceStatus(g_status_handle, &g_service_status);
        break;
    default:
        ret = ERROR_CALL_NOT_IMPLEMENTED;
    }
    return ret;
}

void WINAPI run(DWORD argc, TCHAR * argv[])
{
#ifndef _DEBUG
    //设置服务
    SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);
    g_service_status.dwServiceType = SERVICE_WIN32;
    g_service_status.dwCurrentState = SERVICE_STOPPED;
    g_service_status.dwControlsAccepted = 0;
    g_service_status.dwWin32ExitCode = NO_ERROR;
    g_service_status.dwServiceSpecificExitCode = NO_ERROR;
    g_service_status.dwCheckPoint = 0;
    g_service_status.dwWaitHint = 0;

    g_status_handle = RegisterServiceCtrlHandlerEx(AUTO_MOUNT_NAME, 
        control_handler,
        NULL);

    g_service_status.dwCurrentState = SERVICE_START_PENDING;
    SetServiceStatus(g_status_handle, &g_service_status);

    // service running
    g_service_status.dwControlsAccepted |= AUTOMOUNT_ACCEPTED_CONTROLS;
    g_service_status.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(g_status_handle, &g_service_status);
#endif


    g_Running = true;

    //==================================================================
    //功能实现
    CDiskMng diskMng;

    //获取一个标准值
    DWORD nDiskCount = diskMng.GetDiskCount();

    //处理所有硬盘
    //将未初始化、未分区的进行处理
    //...


    while (g_Running)
    {
        //如果现在的磁盘数量与标准的不同
        DWORD nCurCount = diskMng.GetDiskCount();
        if (nDiskCount != nCurCount)
        {
            //现在的数量少 说明删除了一个磁盘 我们不用做处理
            if (nDiskCount > nCurCount)
            {
                nDiskCount = nCurCount;
                Sleep(5000);
                continue;
            }
            else //添加了一块磁盘
            {
                //查找所有硬盘号
                DWORD Disk[26] = {0};
                nCurCount = diskMng.GetAllPresentDisks(Disk);
                for (int i = 0; i<nCurCount; i++)
                {
                    DWORD nNum = diskMng.GetPartitionLetterFromPhysicalDrive(Disk[i], NULL);
                    if (nNum == -1)
                    {
                        continue;
                    }

                    //这个就是添加且未分区的硬盘
                    if (nNum == 0)
                    {
                        diskMng.CreateDisk(Disk[i]);
                    }
                }
                nDiskCount = nCurCount;
            }
        }
        Sleep(2000);
    }
#ifndef _DEBUG
    // service was stopped
    g_service_status.dwCurrentState = SERVICE_STOP_PENDING;
    SetServiceStatus(g_status_handle, &g_service_status);

    // service is stopped
    g_service_status.dwControlsAccepted &= ~AUTOMOUNT_ACCEPTED_CONTROLS;
    g_service_status.dwCurrentState = SERVICE_STOPPED;

    SetServiceStatus(g_status_handle, &g_service_status);
#endif //_DEBUG
    return ;
}

int start()
{
#ifndef _DEBUG
    SERVICE_TABLE_ENTRY service_table[] = {{AUTO_MOUNT_NAME, run}, {0, 0}};
    return !!StartServiceCtrlDispatcher(service_table);
#else
    run(0, NULL);
    return 0;
#endif
}


bool install()
{
    bool ret = false;

    SC_HANDLE service_control_manager = OpenSCManager(0, 0, SC_MANAGER_CREATE_SERVICE);
    if (!service_control_manager) {
        printf("OpenSCManager failed\n");
        return false;
    }
    TCHAR path[_MAX_PATH + 1];
    if (!GetModuleFileName(0, path, sizeof(path) / sizeof(path[0]))) {
        printf("GetModuleFileName failed\n");
        CloseServiceHandle(service_control_manager);
        return false;
    }
    //FIXME: SERVICE_INTERACTIVE_PROCESS needed for xp only
    SC_HANDLE service = CreateService(service_control_manager, AUTO_MOUNT_NAME,
        AUTO_MOUNT_NAME, SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
        SERVICE_AUTO_START, SERVICE_ERROR_IGNORE, path,
        _YTEXT(""), 0, 0, 0, 0);
    if (service) {
        SERVICE_DESCRIPTION descr;
        descr.lpDescription = AUTO_MOUNT_DESC;
        if (!ChangeServiceConfig2(service, SERVICE_CONFIG_DESCRIPTION, &descr)) {
            printf("ChangeServiceConfig2 failed\n");
        }
        CloseServiceHandle(service);
        printf("Service installed successfully\n");
        ret = true;
    } else if (GetLastError() == ERROR_SERVICE_EXISTS) {
        printf("Service already exists\n");
        ret = true;
    } else {
        printf("Service not installed successfully, error %ld\n", GetLastError());
    }
    CloseServiceHandle(service_control_manager);
    return ret;
}

bool uninstall()
{
    bool ret = false;

    SC_HANDLE service_control_manager = OpenSCManager(0, 0, SC_MANAGER_CONNECT);
    if (!service_control_manager) {
        printf("OpenSCManager failed\n");
        return false;
    }
    SC_HANDLE service = OpenService(service_control_manager, AUTO_MOUNT_NAME,
        SERVICE_QUERY_STATUS | DELETE);
    if (!service) {
        printf("OpenService failed\n");
        CloseServiceHandle(service_control_manager);
        return false;
    }
    SERVICE_STATUS status;
    if (!QueryServiceStatus(service, &status)) {
        printf("QueryServiceStatus failed\n");
    } else if (status.dwCurrentState != SERVICE_STOPPED) {
        printf("Service is still running\n");
    } else if (DeleteService(service)) {
        printf("Service removed successfully\n");
        ret = true;
    } else {
        switch (GetLastError()) {
        case ERROR_ACCESS_DENIED:
            printf("Access denied while trying to remove service\n");
            break;
        case ERROR_INVALID_HANDLE:
            printf("Handle invalid while trying to remove service\n");
            break;
        case ERROR_SERVICE_MARKED_FOR_DELETE:
            printf("Service already marked for deletion\n");
            break;
        }
    }
    CloseServiceHandle(service);
    CloseServiceHandle(service_control_manager);
    return ret;
}

extern "C"
int _tmain(int argc, _TCHAR* argv[], TCHAR* envp[])
{
    if (argc > 1) 
    {
        if (lstrcmpi(argv[1], TEXT("install")) == 0) {
            install();
        } else if (lstrcmpi(argv[1], TEXT("uninstall")) == 0) {
            uninstall();
        } else {
            printf("Use: AutoMount install / uninstall\n");
        }
    }
    else 
    {
        start();
    }
    return 0;
}

