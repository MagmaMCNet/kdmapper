#include <sstream>
#include "service.hpp"

bool service::RegisterAndStart(const std::wstring& driver_path) {
	const static DWORD ServiceTypeKernel = 1;
	const std::wstring driver_name = intel_driver::GetDriverNameW();
	const std::wstring servicesPath = L"SYSTEM\\CurrentControlSet\\Services\\" + driver_name;
	const std::wstring nPath = L"\\??\\" + driver_path;

	HKEY dservice;
	LSTATUS status = RegCreateKeyW(HKEY_LOCAL_MACHINE, servicesPath.c_str(), &dservice); //Returns Ok if already exists
	if (status != ERROR_SUCCESS) {
		Log( "[-] Can't create service key");
		return false;
	}

	status = RegSetKeyValueW(dservice, NULL, L"ImagePath", REG_EXPAND_SZ, nPath.c_str(), (DWORD)(nPath.size()*sizeof(wchar_t)));
	if (status != ERROR_SUCCESS) {
		RegCloseKey(dservice);
		Log("[-] Can't create 'ImagePath' registry value" << std::endl);
		return false;
	}
	
	status = RegSetKeyValueW(dservice, NULL, L"Type", REG_DWORD, &ServiceTypeKernel, sizeof(DWORD));
	if (status != ERROR_SUCCESS) {
		RegCloseKey(dservice);
		Log("[-] Can't create 'Type' registry value" << std::endl);
		return false;
	}
	
	RegCloseKey(dservice);

	HMODULE ntdll = GetModuleHandleA("ntdll.dll");
	if (ntdll == NULL) {
		return false;
	}

	auto RtlAdjustPrivilege = (nt::RtlAdjustPrivilege)GetProcAddress(ntdll, "RtlAdjustPrivilege");
	auto NtLoadDriver = (nt::NtLoadDriver)GetProcAddress(ntdll, "NtLoadDriver");

	ULONG SE_LOAD_DRIVER_PRIVILEGE = 10UL;
	BOOLEAN SeLoadDriverWasEnabled;
	NTSTATUS Status = RtlAdjustPrivilege(SE_LOAD_DRIVER_PRIVILEGE, TRUE, FALSE, &SeLoadDriverWasEnabled);
	if (!NT_SUCCESS(Status)) {
		Log("Fatal error: failed to acquire SE_LOAD_DRIVER_PRIVILEGE. Make sure you are running as administrator." << std::endl);
		return false;
	}

	std::wstring wdriver_reg_path = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\" + driver_name;
	UNICODE_STRING serviceStr;
	RtlInitUnicodeString(&serviceStr, wdriver_reg_path.c_str());

	Status = NtLoadDriver(&serviceStr);


	if (Status == 0xC0000603) { //STATUS_IMAGE_CERT_REVOKED
		Log(ColorText(L"[-] ", "ff0000", "ff8c00") << "Your vulnerable Driver list is enabled and have blocked the Driver loading" << std::endl);
		Log(ColorText(L"[-] ", "ff0000", "ff8c00") << "Registry path to disable vulnerable Driver list : HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\CI\\Config" << std::endl);
		Log(ColorText(L"[-] ", "ff0000", "ff8c00") << "Set 'VulnerableDriverBlocklistEnable' as dword to 0" << std::endl);
	}
	else if (Status == 0xC0000022 || Status == 0xC000009A) { //STATUS_ACCESS_DENIED and STATUS_INSUFFICIENT_RESOURCES
		Log("[-] Access Denied or Insufficient Resources (0x"
			<< std::hex << Status
			<< "), Probably some anticheat or antivirus running blocking the load of vulnerable driver");
	}
	
	//Never should occur since kdmapper checks for "IsRunning" Driver before
	if (Status == 0xC000010E) // STATUS_IMAGE_ALREADY_LOADED
		return true;

	if (Status != 0x0) {
		Log(ORANGE << "[+] NtLoadDriver Status " << DARK_GRAY << "0x" << std::hex << Status << RESET << std::endl);
	}
	else
		Log(GREEN << "[+] Loaded Driver Successfully" << RESET << std::endl);

	return NT_SUCCESS(Status);
}

bool service::StopAndRemove(const std::wstring& driver_name) {
	HMODULE ntdll = GetModuleHandleA("ntdll.dll");
	if (ntdll == NULL)
		return false;

	std::wstring wdriver_reg_path = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\" + driver_name;
	UNICODE_STRING serviceStr;
	RtlInitUnicodeString(&serviceStr, wdriver_reg_path.c_str());

	HKEY driver_service;
	std::wstring servicesPath = L"SYSTEM\\CurrentControlSet\\Services\\" + driver_name;
	LSTATUS status = RegOpenKeyW(HKEY_LOCAL_MACHINE, servicesPath.c_str(), &driver_service);
	if (status != ERROR_SUCCESS) {
		if (status == ERROR_FILE_NOT_FOUND) {
			return true;
		}
		return false;
	}
	RegCloseKey(driver_service);

	auto NtUnloadDriver = (nt::NtUnloadDriver)GetProcAddress(ntdll, "NtUnloadDriver");
	NTSTATUS st = NtUnloadDriver(&serviceStr);
	if (st != 0x0) {
		Log(ERROR_COLOR << "[-] Driver Unload Failed!!" << RESET << std::endl);
		Log(ERROR_COLOR << "[-] NtUnloadDriver Status " << DARK_GRAY << "0x" << std::hex << st << RESET << std::endl);
		status = RegDeleteTreeW(HKEY_LOCAL_MACHINE, servicesPath.c_str());
		return false;
	}
	Log(GREEN << "[+] Unloaded Driver Successfully" << RESET << std::endl);
	

	status = RegDeleteTreeW(HKEY_LOCAL_MACHINE, servicesPath.c_str());
	if (status != ERROR_SUCCESS) {
		return false;
	}
	return true;
}
