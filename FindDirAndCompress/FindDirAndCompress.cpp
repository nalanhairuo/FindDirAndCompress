#include <iostream>
#include <string>
#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <tchar.h>
//#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")

//查找当前目录下创建时间超过DAYS天的文件夹,如果超过七天就对此文件夹用rar进行压缩并删除原文件夹

typedef bool (WINAPI *EnumerateFunc)(LPCSTR lpFileOrPath, void* pUserData);

const int DAYS = 7;
//将TCHAR转为char
//*tchar是TCHAR类型指针，*_char是char类型指针
//void TcharToChar(const TCHAR * tchar, char * _char)
//{
//    int iLength ;
//    //获取字节长度
//    iLength = WideCharToMultiByte(CP_ACP, 0, tchar, -1, NULL, 0, NULL, NULL);
//    //将tchar值赋给_char
//    WideCharToMultiByte(CP_ACP, 0, tchar, -1, _char, iLength, NULL, NULL);
//}
//
//void CharToTchar(const char * _char, TCHAR * tchar)
//{
//    int iLength ;
//
//    iLength = MultiByteToWideChar(CP_ACP, 0, _char, strlen(_char) + 1, NULL, 0) ;
//    MultiByteToWideChar(CP_ACP, 0, _char, strlen(_char) + 1, tchar, iLength) ;
//}
//无窗口执行cmd命令行
bool ExeCuteCommand(const std::string& command)
{
    const char* applicationName = "C:\\Windows\\system32\\cmd.exe";
    std::string commandLineStr = "/c";
    commandLineStr += command;

    STARTUPINFOA startupInfo;
    PROCESS_INFORMATION processInfo;

    memset(&startupInfo, 0, sizeof(startupInfo));
    memset(&processInfo, 0, sizeof(processInfo));
    startupInfo.cb = sizeof(startupInfo);

    char* commandLine = &commandLineStr.front();
    auto creationFlags = CREATE_NO_WINDOW;

    CreateProcess(applicationName, commandLine, nullptr, nullptr, 0, creationFlags, nullptr, nullptr, &startupInfo, &processInfo);
    WaitForSingleObject(processInfo.hProcess, 30 * 1000);
    DWORD exitCode;
    GetExitCodeProcess(processInfo.hProcess, &exitCode);
    return exitCode == 0;
}

//convert year-month-day to time_t
time_t convert(int year, int month, int day)
{
    tm info = { 0 };
    info.tm_year = year - 1900;
    info.tm_mon = month - 1;
    info.tm_mday = day;
    return mktime(&info);
}

//calc time into days
int  get_days(const char* from, const char* to)
{
    int year, month, day;
    sscanf_s(from, "%d-%d-%d", &year, &month, &day);
    int fromSecond = (int)convert(year, month, day);
    sscanf_s(to, "%d-%d-%d", &year, &month, &day);
    int toSecond = (int)convert(year, month, day);
    return (toSecond - fromSecond) / 24 / 3600;
}

//分解路径
void SplitPathFileName(const char *pstrPathFileName, char *pstrPath, char *pstrFileName, char *pstrExtName)
{
    if (pstrPath != nullptr)
    {
        char szTemp[MAX_PATH];
        //_splitpath(pstrPathFileName, pstrPath, szTemp, pstrFileName, pstrExtName);
        _splitpath_s(pstrPathFileName, pstrPath, _MAX_DRIVE, szTemp, _MAX_DIR, pstrFileName, _MAX_FNAME, pstrExtName, _MAX_EXT);
        //strcat(pstrPath, szTemp);
        strcat_s(pstrPath, MAX_PATH, szTemp);
    }

    else
    {
        //_splitpath(pstrPathFileName, nullptr, nullptr, pstrFileName, pstrExtName);
        _splitpath_s(pstrPathFileName, nullptr, _MAX_DRIVE, nullptr, _MAX_DIR, pstrFileName, _MAX_FNAME, pstrExtName, _MAX_EXT);
    }
}

//得到当前进程可执行文件的路径名，文件名，后缀名
//如运行C:\test\test.exe 得到 "C:\test\", "test", ".exe"
bool GetProcessPathNameAndFileName(char *pstrPath, char *pstrFileName, char *pstrExtName)
{
    TCHAR szExeFilePathFileName[MAX_PATH];

    if (GetModuleFileName(nullptr, szExeFilePathFileName, MAX_PATH) == 0)
    {
        return false;
    }

    SplitPathFileName(szExeFilePathFileName, pstrPath, pstrFileName, pstrExtName);

    return true;
}

BOOL AdjustProcessCurrentDirectory()
{
    char szPathName[MAX_PATH];
    GetProcessPathNameAndFileName(szPathName, nullptr, nullptr);
    return SetCurrentDirectory(szPathName);
}

//枚举文件和枚举子目录
//第一个参数lpPath指定欲遍历的路径（文件夹）
//第二个参数bRecursion指定是否递归处理子目录
//第三个参数bEnumFiles指定是枚举文件还是枚举子目录
//第四个参数pFunc为用户回调函数，枚举过程中每遇到一个文件或子目录，都会调用它，并传入这个文件或子目录的完整路径；
//第五个参数pUserData为用户任意指定的数据，它也将被传入用户回调函数。
void FileEnumeration(LPSTR lpPath, bool bRecursion, bool bEnumFiles, EnumerateFunc pFunc, void* pUserData)
{

    static bool s_bUserBreak = false;

    try
    {
        //-------------------------------------------------------------------------
        if (s_bUserBreak)
        {
            return;
        }

        int len = strlen(lpPath);

        if (lpPath == NULL || len <= 0)
        {
            return;
        }

        //NotifySys(NRS_DO_EVENTS, 0,0);

        char path[MAX_PATH];
        strcpy_s(path, lpPath);

        if (lpPath[len - 1] != '\\')
        {
            strcat_s(path, "\\");
        }

        strcat_s(path, "*");

        WIN32_FIND_DATA fd;
        HANDLE hFindFile = FindFirstFile(path, &fd);

        if (hFindFile == INVALID_HANDLE_VALUE)
        {
            ::FindClose(hFindFile);
            return;
        }

        char tempPath[MAX_PATH];
        bool bUserReture = true;
        bool bIsDirectory;

        bool bFinish = false;

        while (!bFinish)
        {
            strcpy_s(tempPath, lpPath);

            if (lpPath[len - 1] != '\\')
            {
                strcat_s(tempPath, "\\");
            }

            strcat_s(tempPath, fd.cFileName);

            bIsDirectory = ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);

            //如果是.或..
            if (bIsDirectory
                    && (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0))
            {
                bFinish = (FindNextFile(hFindFile, &fd) == false);
                continue;
            }

            if (pFunc && bEnumFiles != bIsDirectory)
            {
                bUserReture = pFunc(tempPath, pUserData);

                if (bUserReture == false)
                {
                    s_bUserBreak = true;
                    ::FindClose(hFindFile);
                    return;
                }
            }

            //NotifySys(NRS_DO_EVENTS, 0,0);

            if (bIsDirectory && bRecursion) //是子目录
            {
                FileEnumeration(tempPath, bRecursion, bEnumFiles, pFunc, pUserData);
            }

            bFinish = (FindNextFile(hFindFile, &fd) == FALSE);
        }

        ::FindClose(hFindFile);

        //-------------------------------------------------------------------------
    }

    catch (...)
    {
        // ASSERT(0);
        return;
    }
}

//用户回调函数(EnumerateFunc)有两个参数
//一个是文件或子目录的完整路径(lpFileOrPath)
//一个是用户自定义数据(pUserData)，它被自动调用，用户需在此函数中编码处理代码。
bool WINAPI myEnumerateFunc(LPCSTR lpFileOrPath, void* pUserData)
{
    //        char* pdot;
    //        if ((pdot = strrchr(lpFileOrPath, '.')) && stricmp(pdot, ".mp3") == 0)
    //        {
    //                printf("%s\n", lpFileOrPath);
    //        }

    char compressFileName[MAX_PATH];//压缩文件名
    std::string lastDirName(lpFileOrPath);
    int pos = lastDirName.find_last_of("\\");
    // printf("%s\n",lastDirName.substr(pos+1).c_str());
    strncpy_s(compressFileName, lastDirName.substr(pos + 1).c_str(), lastDirName.substr(pos + 1).length() + 1);
    // printf("%s\n",compressFileName);

    time_t timeCurr = time(nullptr);
    char timeStr[MAX_PATH];

    tm tmCurr;
    localtime_s(&tmCurr, &timeCurr);//localtime(&timeCurr) use localtime_s instead of localtime

    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d", &tmCurr);
    // printf("%s\n", timeStr);

    char fileTimeStr[MAX_PATH];

    // printf("%s\n", lpFileOrPath);
    HANDLE hDir = CreateFile(lpFileOrPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                             FILE_FLAG_BACKUP_SEMANTICS, nullptr);

    FILETIME lpCreationTime;   // 文件夹的创建时间
    FILETIME lpLastAccessTime; // 对文件夹的最近访问时间
    FILETIME lpLastWriteTime;  // 文件夹的最近修改时间

    // 获取文件夹时间属性信息
    if (GetFileTime(hDir, &lpCreationTime, &lpLastAccessTime, &lpLastWriteTime))
    {
        FILETIME fCreat;
        FILETIME fAccess;
        FILETIME fWrite;

        // 转换成本地时间
        FileTimeToLocalFileTime(&lpCreationTime, &fCreat);
        FileTimeToLocalFileTime(&lpLastWriteTime, &fAccess);
        FileTimeToLocalFileTime(&lpLastAccessTime, &fWrite);

        SYSTEMTIME sCreat;
        SYSTEMTIME sAccess;
        SYSTEMTIME sWrite;

        // 转换成系统时间
        FileTimeToSystemTime(&fCreat, &sCreat);
        FileTimeToSystemTime(&fAccess, &sAccess);
        FileTimeToSystemTime(&fWrite, &sWrite);

        sprintf_s(fileTimeStr, "%d-%d-%d", sCreat.wYear, sCreat.wMonth, sCreat.wDay);
        // printf("%s\n", fileTimeStr);
        // printf("文件夹创建时间: %d年%d月%d日 %d:%d:%d\n", sCreat.wYear, sCreat.wMonth, sCreat.wDay, sCreat.wHour, sCreat.wMinute, sCreat.wSecond);
        // printf("文件夹最近访问时间: %d年%d月%d日 %d:%d:%d\n", sAccess.wYear, sAccess.wMonth, sAccess.wDay, sAccess.wHour, sAccess.wMinute, sAccess.wSecond);
        // printf("文件夹最近修改时间: %d年%d月%d日 %d:%d:%d\n", sWrite.wYear, sWrite.wMonth, sWrite.wDay, sWrite.wHour, sWrite.wMinute, sWrite.wSecond);
    }

    CloseHandle(hDir); // 关闭打开过的文件夹

    int days = get_days(fileTimeStr, timeStr);
    // printf("From:%s\nTo:%s\n", fileTimeStr, timeStr);
    // printf("%d\n", days);
    // char *szCommand = "C:\\Progra~1\\WinRar\\Rar.exe a";
    char szCommand[MAX_PATH];//命令行参数
    sprintf_s(szCommand, "C:\\Progra~1\\WinRar\\Rar.exe a -m5 -ep -df %s.rar %s", compressFileName, lpFileOrPath);

    //sprintf(szCommand, "C:\\Progra~1\\WinRar\\Rar.exe a -m5 -ep %s.rar %s", compressFileName, lpFileOrPath);
    // printf("%s\n",szCommand);
    if (days > DAYS)
    {
        ExeCuteCommand(szCommand);
        printf("处理超过七天的文件夹 %s\n", lpFileOrPath);
        //Sleep(60000);
    }

    return TRUE;
}

int main(void)
{

    //调整进程当前目录为程序可执行文件所在目录
    AdjustProcessCurrentDirectory();
    // 获取当前程序运行路径
    char szPathName[MAX_PATH];
    GetProcessPathNameAndFileName(szPathName, nullptr, nullptr);

    // 在当前目录下查找文件夹并处理
    FileEnumeration(szPathName, false, false, myEnumerateFunc, nullptr);
    //system("pause");

    return 0;
}
