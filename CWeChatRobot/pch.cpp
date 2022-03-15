﻿// pch.cpp: 与预编译标头对应的源文件

#include "pch.h"

// 当使用预编译的头时，需要使用此源文件，编译才能成功。
DWORD SendImageOffset = 0x0;
DWORD SendTextOffset = 0x0;
DWORD SendFileOffset = 0x0;

DWORD GetFriendListInitOffset = 0x0;
DWORD GetFriendListRemoteOffset = 0x0;
DWORD GetFriendListFinishOffset = 0x0;

DWORD GetWxUserInfoOffset = 0x0;
DWORD DeleteUserInfoCacheOffset = 0x0;

DWORD GetSelfInfoOffset = 0x0;
DWORD DeleteSelfInfoCacheOffset = 0x0;
wstring SelfInfoString = L"";

HANDLE hProcess = NULL;

bool isFileExists_stat(string& name) {
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

BOOL CreateConsole(void) {
    if (AllocConsole()) {
        AttachConsole(GetCurrentProcessId());
        FILE* retStream;
        freopen_s(&retStream, "CONOUT$", "w", stdout);
        if (!retStream) throw std::runtime_error("Stdout redirection failed.");
        freopen_s(&retStream, "CONOUT$", "w", stderr);
        if (!retStream) throw std::runtime_error("Stderr redirection failed.");
        return 0;
    }
    return 1;
}

DWORD GetWeChatRobotBase() {
    if (!hProcess)
        return 0;
    DWORD dwWriteSize = 0;
    LPVOID pRemoteAddress = VirtualAllocEx(hProcess, NULL, 1, MEM_COMMIT, PAGE_READWRITE);
    if (pRemoteAddress)
        WriteProcessMemory(hProcess, pRemoteAddress, dllname, wcslen(dllname) * 2 + 2, &dwWriteSize);
    else
        return 0;
    DWORD dwHandle, dwID;
    LPVOID pFunc = GetModuleHandleW;
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pFunc, pRemoteAddress, 0, &dwID);
    if (hThread) {
        WaitForSingleObject(hThread, INFINITE);
        GetExitCodeThread(hThread, &dwHandle);
    }
    else {
        return 0;
    }
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, pRemoteAddress, 0, MEM_RELEASE);
    return dwHandle;
}

void GetProcOffset(wchar_t* workPath) {
    wchar_t* dllpath = new wchar_t[MAX_PATH];
    swprintf_s(dllpath, MAX_PATH, L"%ws%ws%ws", workPath, L"\\", dllname);
    string name = _com_util::ConvertBSTRToString((BSTR)dllpath);
    if (!isFileExists_stat(name)) {
        MessageBoxA(NULL, name.c_str(), "文件不存在", MB_ICONWARNING);
        return;
    }
    HMODULE hd = LoadLibraryW(dllpath);
    if (!hd)
        return;
    DWORD WeChatBase = (DWORD)GetModuleHandleW(dllname);

    DWORD SendImageProcAddr = (DWORD)GetProcAddress(hd, SendImageRemote);
    SendImageOffset = SendImageProcAddr - WeChatBase;
    DWORD SendTextProcAddr = (DWORD)GetProcAddress(hd, SendTextRemote);
    SendTextOffset = SendTextProcAddr - WeChatBase;
    DWORD SendFileProcAddr = (DWORD)GetProcAddress(hd, SendFileRemote);
    SendFileOffset = SendFileProcAddr - WeChatBase;

    DWORD GetFriendListInitProcAddr = (DWORD)GetProcAddress(hd, GetFriendListInit);
    GetFriendListInitOffset = GetFriendListInitProcAddr - WeChatBase;
    DWORD GetFriendListRemoteProcAddr = (DWORD)GetProcAddress(hd, GetFriendListRemote);
    GetFriendListRemoteOffset = GetFriendListRemoteProcAddr - WeChatBase;
    DWORD GetFriendListFinishProcAddr = (DWORD)GetProcAddress(hd, GetFriendListFinish);
    GetFriendListFinishOffset = GetFriendListFinishProcAddr - WeChatBase;

    DWORD GetWxUserInfoProcAddr = (DWORD)GetProcAddress(hd, GetWxUserInfoRemote);
    GetWxUserInfoOffset = GetWxUserInfoProcAddr - WeChatBase;
    DWORD DeleteUserInfoCacheProcAddr = (DWORD)GetProcAddress(hd, DeleteUserInfoCacheRemote);
    DeleteUserInfoCacheOffset = DeleteUserInfoCacheProcAddr - WeChatBase;

    DWORD GetSelfInfoProcAddr = (DWORD)GetProcAddress(hd, GetSelfInfoRemote);
    GetSelfInfoOffset = GetSelfInfoProcAddr - WeChatBase;
    DWORD DeleteSelfInfoCacheProcAddr = (DWORD)GetProcAddress(hd, DeleteSelfInfoCacheRemote);
    DeleteSelfInfoCacheOffset = DeleteSelfInfoCacheProcAddr - WeChatBase;

    FreeLibrary(hd);
    delete[] dllpath;
    dllpath = NULL;
}

DWORD GetWeChatPid() {
    HWND hCalc = FindWindow(NULL, L"微信");
    DWORD wxPid = 0;
    GetWindowThreadProcessId(hCalc, &wxPid);
    if (wxPid == 0) {
        hCalc = FindWindow(NULL, L"微信测试版");
        GetWindowThreadProcessId(hCalc, &wxPid);
    }
    return wxPid;
}

DWORD StartRobotService(wchar_t* workPath) {
    DWORD wxPid = GetWeChatPid();
    if (!wxPid) {
        MessageBoxA(NULL, "请先启动目标程序", "提示", MB_ICONWARNING);
        return 1;
    }
    GetProcOffset(workPath);
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, wxPid);
    bool status = Injert(wxPid, workPath);
    return status;
}

DWORD StopRobotService() {
    if (!hProcess)
        return 1;
    DWORD wxPid = GetWeChatPid();
    RemoveDll(wxPid);
    ZeroMemory((wchar_t*)SelfInfoString.c_str(), SelfInfoString.length() * 2 + 2);
    CloseHandle(hProcess);
    return 0;
}