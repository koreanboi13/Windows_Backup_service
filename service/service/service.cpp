#include <zlib.h>
#include <zip.h>
#include <zconf.h>

#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>
#include <iostream>
#include <string>
#include <fstream>
#include <windows.h>
#include <TCHAR.H>
#include <sys/stat.h>
#include <time.h>

#pragma comment(lib, "Advapi32.lib")

using namespace std;
namespace fs = std::experimental::filesystem;

#define serviceName _T("SaveFile Service")
#define servicePath _T("E:\\SMIT\\lab_2\\service\\x64\\Debug\\service.exe")

SERVICE_STATUS serviceStatus;
SERVICE_STATUS_HANDLE hStatus;

void zip();

int addLogMessage(const char* str) {
	errno_t err;
	FILE* log;
	if ((err = fopen_s(&log, "E:\\SMIT\\lab_2\\service.log", "a+")) != 0) {
		return -1;
	}
	fprintf(log, "%s\n", str);
	fclose(log);
	return 0;
}

void ControlHandler(DWORD request) {
	switch (request)
	{
	case SERVICE_CONTROL_STOP:
		serviceStatus.dwWin32ExitCode = 0;
		serviceStatus.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus(hStatus, &serviceStatus);
		return;
	case SERVICE_CONTROL_SHUTDOWN:
		addLogMessage("Shutdown.");
		serviceStatus.dwWin32ExitCode = 0;
		serviceStatus.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus(hStatus, &serviceStatus);
		return;
	default:
		break;
	}
	SetServiceStatus(hStatus, &serviceStatus);
	return;
}

void ServiceMain(int argc, char** argv) {
	int i = 0;

	serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	serviceStatus.dwCurrentState = SERVICE_START_PENDING;
	serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
	serviceStatus.dwWin32ExitCode = 0;
	serviceStatus.dwServiceSpecificExitCode = 0;
	serviceStatus.dwCheckPoint = 0;
	serviceStatus.dwWaitHint = 0;

	hStatus = RegisterServiceCtrlHandler(serviceName, (LPHANDLER_FUNCTION)ControlHandler);

	if (hStatus == (SERVICE_STATUS_HANDLE)0) {
		return;
	}



	serviceStatus.dwCurrentState = SERVICE_RUNNING;
	SetServiceStatus(hStatus, &serviceStatus);

	addLogMessage(">>Success started service");

	while (serviceStatus.dwCurrentState == SERVICE_RUNNING)
	{
		zip();
		Sleep(5000);
	}
	return;
}


bool check(string p, string m)
{
	int i = 0, j = 0, lm = -1, lp = 0;
	while (p[i])
	{
		if (m[j] == '*')
			lp = i, lm = ++j;
		else if (p[i] == m[j] || m[j] == '?')
			i++, j++;
		else if (p[i] != m[j])
		{
			if (lm == -1) return false;
			i = ++lp;
			j = lm;
		}
	}
	if (!m[j]) return !p[i];
	return false;
}

string get_filename(const experimental::filesystem::path& p) {
	return p.filename().string();
}

void add_file_to_archive(zip_t* archive, const std::string& filepath, const std::string& archivePath) {
	zip_source_t* source = zip_source_file(archive, filepath.c_str(), 0, -1);
	if (!source || zip_file_add(archive, archivePath.c_str(), source, ZIP_FL_ENC_UTF_8 | ZIP_FL_OVERWRITE) < 0) {
		zip_source_free(source);
		addLogMessage(">> Error adding file");
	}
}

void zip() {
	ifstream in("E:\\SMIT\\lab_2\\service.conf");
	if (!in) {
		addLogMessage(">> Error opening config file");
		return;
	}

	string cat, arch, mask;
	getline(in, cat);
	getline(in, arch);

	zip_t* archive = nullptr;
	int zipError;
	if (fs::exists(arch))
		archive = zip_open(arch.c_str(), 0, &zipError);
	else 
		archive = zip_open(arch.c_str(), ZIP_CREATE, &zipError);

	if (!archive) {
		addLogMessage(">> Error opening ZIP archive");
		return;
	}

	while (getline(in, mask)) {
		for (const auto& entry : fs::recursive_directory_iterator(cat)) {
			if (check(entry.path().filename().string(), mask)) {
				string relativePath = entry.path().string();
				relativePath.erase(0, cat.size() + 1);

				struct stat fileStat;
				zip_stat_t archiveStat;

				stat(entry.path().string().c_str(), &fileStat);
				if (zip_name_locate(archive, relativePath.c_str(), 0) == -1) 
					add_file_to_archive(archive, entry.path().string(), relativePath);
				
				else if (zip_stat(archive, relativePath.c_str(), 0, &archiveStat) == 0 &&
					archiveStat.mtime < fileStat.st_mtime) {
					add_file_to_archive(archive, entry.path().string(), relativePath);
				}
			}
		}
	}
	zip_close(archive);
}

int InstallService() {
	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if (!hSCManager) {
		addLogMessage(">>Error: Can't open Service Control Manager");
		return -1;
	}

	SC_HANDLE hService = CreateService(
		hSCManager,
		serviceName,
		serviceName,
		SERVICE_ALL_ACCESS,
		SERVICE_WIN32_OWN_PROCESS,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_NORMAL,
		servicePath,
		NULL, NULL, NULL, NULL, NULL
	);
	if (!hService) {
		int err = GetLastError();
		switch (err) {
		case ERROR_ACCESS_DENIED:
			addLogMessage(">>Error: ERROR_ACCESS_DENIED");
			break;
		case ERROR_CIRCULAR_DEPENDENCY:
			addLogMessage(">>Error: ERROR_CIRCULAR_DEPENDENCY");
			break;
		case ERROR_DUPLICATE_SERVICE_NAME:
			addLogMessage(">>Error: ERROR_DUPLICATE_SERVICE_NAME");
			break;
		case ERROR_INVALID_HANDLE:
			addLogMessage(">>Error: ERROR_INVALID_HANDLE");
			break;
		case ERROR_INVALID_NAME:
			addLogMessage(">>Error: ERROR_INVALID_NAME");
			break;
		case ERROR_INVALID_PARAMETER:
			addLogMessage(">>Error: ERROR_INVALID_PARAMETER");
			break;
		case ERROR_INVALID_SERVICE_ACCOUNT:
			addLogMessage(">>Error: ERROR_INVALID_SERVICE_ACCOUNT");
			break;
		case ERROR_SERVICE_EXISTS:
			addLogMessage(">>Error: ERROR_SERVICE_EXISTS");
			break;
		default:
			addLogMessage(">>Error: Undefined");
		}
		CloseServiceHandle(hSCManager);
		return -1;
	}
	CloseServiceHandle(hService);
	CloseServiceHandle(hSCManager);
	addLogMessage(">>Success install service!");
	return 0;
}

int RemoveService() {
	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!hSCManager) {
		addLogMessage(">>Error: Can't open Service Control Manager");
		return -1;
	}

	SC_HANDLE hService = OpenService(hSCManager, serviceName, SERVICE_STOP | DELETE);
	if (!hService) {
		addLogMessage(">>Error: Can't remove service");
		CloseServiceHandle(hSCManager);
		return -1;
	}

	DeleteService(hService);
	CloseServiceHandle(hService);
	CloseServiceHandle(hSCManager);
	addLogMessage(">>Success remove service!");
	return 0;
}

int StartService() {
	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	SC_HANDLE hService = OpenServiceW(hSCManager, serviceName, SERVICE_START);

	if (!StartServiceW(hService, 0, NULL)) {
		int err = GetLastError();
		addLogMessage(">>Error: Can't start service");
		switch (err) {
		case ERROR_ACCESS_DENIED:
			addLogMessage(">>Error: ERROR_ACCESS_DENIED");
			break;
		case ERROR_INVALID_HANDLE:
			addLogMessage(">>Error: ERROR_INVALID_HANDLE");
			break;
		case ERROR_PATH_NOT_FOUND:
			addLogMessage(">>Error: ERROR_PATH_NOT_FOUND");
			break;
		case ERROR_SERVICE_ALREADY_RUNNING:
			addLogMessage(">>Error: ERROR_SERVICE_ALREADY_RUNNING");
			break;
		case ERROR_SERVICE_DATABASE_LOCKED:
			addLogMessage(">>Error: ERROR_SERVICE_DATABASE_LOCKED");
			break;
		case ERROR_SERVICE_DEPENDENCY_DELETED:
			addLogMessage(">>Error: ERROR_SERVICE_DEPENDENCY_DELETED");
			break;
		case ERROR_SERVICE_DEPENDENCY_FAIL:
			addLogMessage(">>Error: ERROR_SERVICE_DEPENDENCY_FAIL");
			break;
		case ERROR_SERVICE_DISABLED:
			addLogMessage(">>Error: ERROR_SERVICE_DISABLED");
			break;
		case ERROR_SERVICE_LOGON_FAILED:
			addLogMessage(">>Error: ERROR_SERVICE_LOGON_FAILED");
			break;
		case ERROR_SERVICE_MARKED_FOR_DELETE:
			addLogMessage(">>Error: ERROR_SERVICE_MARKED_FOR_DELETE");
			break;
		case ERROR_SERVICE_NO_THREAD:
			addLogMessage(">>Error: ERROR_SERVICE_NO_THREAD");
			break;
		case ERROR_SERVICE_REQUEST_TIMEOUT:
			addLogMessage(">>Error: ERROR_SERVICE_REQUEST_TIMEOUT");
			break;
		default: addLogMessage(">>Error: UNDEFINED ERROR CODE");
		}
		CloseServiceHandle(hSCManager);

		return -1;
	}
	CloseServiceHandle(hService);
	CloseServiceHandle(hSCManager);
	return 0;
}


int StopService()
{
	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if (!hSCManager) {
		addLogMessage(">>Error: Can't open Service Control Manager");
		return -1;
	}

	SC_HANDLE hService = OpenService(hSCManager, serviceName, SERVICE_QUERY_STATUS | SERVICE_STOP);
	if (!hService) {
		addLogMessage(">>Error: Can't open service for stopping");
		CloseServiceHandle(hSCManager);
		return -1;
	}

	SERVICE_STATUS status;
	if (!QueryServiceStatus(hService, &status)) {
		addLogMessage(">>Error: Can't query service status");
		CloseServiceHandle(hService);
		CloseServiceHandle(hSCManager);
		return -1;
	}

	if (status.dwCurrentState == SERVICE_RUNNING) {
		// Если сервис работает, пытаемся его остановить
		if (!ControlService(hService, SERVICE_CONTROL_STOP, &status)) {
			addLogMessage(">>Error: Unable to stop service");
			CloseServiceHandle(hService);
			CloseServiceHandle(hSCManager);
			return -1;
		}
		addLogMessage(">>Success: Stopped the service");
	}
	else {
		addLogMessage(">>Error: Service is not running or already stopped");
	}

	CloseServiceHandle(hService);
	CloseServiceHandle(hSCManager);
	return 0;
}
int _tmain(int argc, _TCHAR* argv[]) {

	SERVICE_TABLE_ENTRY ServiceTable[2];
	ServiceTable[0].lpServiceName = (LPWSTR)serviceName;
	ServiceTable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION)ServiceMain;
	ServiceTable[1].lpServiceName = NULL;
	ServiceTable[1].lpServiceProc = NULL;

	if (argc - 1 == 0) {
		if (!StartServiceCtrlDispatcher(ServiceTable)) {
			addLogMessage("Error: StartServiceCtrlDispatcher");
		}
	}
	else if (wcscmp(argv[argc - 1], _T("install")) == 0) {
		InstallService();
	}
	else if (wcscmp(argv[argc - 1], _T("remove")) == 0) {
		RemoveService();
	}
	else if (wcscmp(argv[argc - 1], _T("stop")) == 0) {
		StopService();
	}
	else if (wcscmp(argv[argc - 1], _T("start")) == 0) {
		StartService();
	}
	
}
