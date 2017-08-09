#include <iostream>
#include <string>
#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <tchar.h>
//#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")

//���ҵ�ǰĿ¼�´���ʱ�䳬��DAYS����ļ���,�����������ͶԴ��ļ�����rar����ѹ����ɾ��ԭ�ļ���

typedef bool (WINAPI *EnumerateFunc)(LPCSTR lpFileOrPath, void* pUserData);

const int DAYS = 7;
//��TCHARתΪchar
//*tchar��TCHAR����ָ�룬*_char��char����ָ��
//void TcharToChar(const TCHAR * tchar, char * _char)
//{
//    int iLength ;
//    //��ȡ�ֽڳ���
//    iLength = WideCharToMultiByte(CP_ACP, 0, tchar, -1, NULL, 0, NULL, NULL);
//    //��tcharֵ����_char
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
//�޴���ִ��cmd������
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

//�ֽ�·��
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

//�õ���ǰ���̿�ִ���ļ���·�������ļ�������׺��
//������C:\test\test.exe �õ� "C:\test\", "test", ".exe"
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

//ö���ļ���ö����Ŀ¼
//��һ������lpPathָ����������·�����ļ��У�
//�ڶ�������bRecursionָ���Ƿ�ݹ鴦����Ŀ¼
//����������bEnumFilesָ����ö���ļ�����ö����Ŀ¼
//���ĸ�����pFuncΪ�û��ص�������ö�ٹ�����ÿ����һ���ļ�����Ŀ¼�����������������������ļ�����Ŀ¼������·����
//���������pUserDataΪ�û�����ָ�������ݣ���Ҳ���������û��ص�������
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

            //�����.��..
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

            if (bIsDirectory && bRecursion) //����Ŀ¼
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

//�û��ص�����(EnumerateFunc)����������
//һ�����ļ�����Ŀ¼������·��(lpFileOrPath)
//һ�����û��Զ�������(pUserData)�������Զ����ã��û����ڴ˺����б��봦����롣
bool WINAPI myEnumerateFunc(LPCSTR lpFileOrPath, void* pUserData)
{
    //        char* pdot;
    //        if ((pdot = strrchr(lpFileOrPath, '.')) && stricmp(pdot, ".mp3") == 0)
    //        {
    //                printf("%s\n", lpFileOrPath);
    //        }

    char compressFileName[MAX_PATH];//ѹ���ļ���
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

    FILETIME lpCreationTime;   // �ļ��еĴ���ʱ��
    FILETIME lpLastAccessTime; // ���ļ��е��������ʱ��
    FILETIME lpLastWriteTime;  // �ļ��е�����޸�ʱ��

    // ��ȡ�ļ���ʱ��������Ϣ
    if (GetFileTime(hDir, &lpCreationTime, &lpLastAccessTime, &lpLastWriteTime))
    {
        FILETIME fCreat;
        FILETIME fAccess;
        FILETIME fWrite;

        // ת���ɱ���ʱ��
        FileTimeToLocalFileTime(&lpCreationTime, &fCreat);
        FileTimeToLocalFileTime(&lpLastWriteTime, &fAccess);
        FileTimeToLocalFileTime(&lpLastAccessTime, &fWrite);

        SYSTEMTIME sCreat;
        SYSTEMTIME sAccess;
        SYSTEMTIME sWrite;

        // ת����ϵͳʱ��
        FileTimeToSystemTime(&fCreat, &sCreat);
        FileTimeToSystemTime(&fAccess, &sAccess);
        FileTimeToSystemTime(&fWrite, &sWrite);

        sprintf_s(fileTimeStr, "%d-%d-%d", sCreat.wYear, sCreat.wMonth, sCreat.wDay);
        // printf("%s\n", fileTimeStr);
        // printf("�ļ��д���ʱ��: %d��%d��%d�� %d:%d:%d\n", sCreat.wYear, sCreat.wMonth, sCreat.wDay, sCreat.wHour, sCreat.wMinute, sCreat.wSecond);
        // printf("�ļ����������ʱ��: %d��%d��%d�� %d:%d:%d\n", sAccess.wYear, sAccess.wMonth, sAccess.wDay, sAccess.wHour, sAccess.wMinute, sAccess.wSecond);
        // printf("�ļ�������޸�ʱ��: %d��%d��%d�� %d:%d:%d\n", sWrite.wYear, sWrite.wMonth, sWrite.wDay, sWrite.wHour, sWrite.wMinute, sWrite.wSecond);
    }

    CloseHandle(hDir); // �رմ򿪹����ļ���

    int days = get_days(fileTimeStr, timeStr);
    // printf("From:%s\nTo:%s\n", fileTimeStr, timeStr);
    // printf("%d\n", days);
    // char *szCommand = "C:\\Progra~1\\WinRar\\Rar.exe a";
    char szCommand[MAX_PATH];//�����в���
    sprintf_s(szCommand, "C:\\Progra~1\\WinRar\\Rar.exe a -m5 -ep -df %s.rar %s", compressFileName, lpFileOrPath);

    //sprintf(szCommand, "C:\\Progra~1\\WinRar\\Rar.exe a -m5 -ep %s.rar %s", compressFileName, lpFileOrPath);
    // printf("%s\n",szCommand);
    if (days > DAYS)
    {
        ExeCuteCommand(szCommand);
        printf("������������ļ��� %s\n", lpFileOrPath);
        //Sleep(60000);
    }

    return TRUE;
}

int main(void)
{

    //�������̵�ǰĿ¼Ϊ�����ִ���ļ�����Ŀ¼
    AdjustProcessCurrentDirectory();
    // ��ȡ��ǰ��������·��
    char szPathName[MAX_PATH];
    GetProcessPathNameAndFileName(szPathName, nullptr, nullptr);

    // �ڵ�ǰĿ¼�²����ļ��в�����
    FileEnumeration(szPathName, false, false, myEnumerateFunc, nullptr);
    //system("pause");

    return 0;
}
