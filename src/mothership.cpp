#include "mothership.hpp"

int _tmain(int argc, TCHAR *argv[]) {
    std::cout << 
        "branch: "   << gm_git::branch  << 
        ", tag: "    << gm_git::tag     << 
        ", commit: " << gm_git::commit  << "\n";
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (argc != 2) {
        printf("Usage: %s [cmdline]\n", argv[0]);
        return EXIT_FAILURE;
    }

    //LPCSTR dir = "\"C:\\Users\\torre\\vcs\\galaga-mothership\\\"";

    if (!CreateProcess(
        NULL,
        argv[1],
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi)) {
        
        printf("CreateProcess failed (%d).\n", GetLastError());
        return EXIT_FAILURE;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    printf("[Mothership.exe]: Exiting...\n");
    return EXIT_SUCCESS;
}