#ifndef KDLIBMODE

#include <string>
#include <vector>
#include <filesystem>

#include "kdmapper.hpp"

HANDLE iqvw64e_device_handle;

LONG WINAPI SimplestCrashHandler(EXCEPTION_POINTERS* ExceptionInfo)
{
	if (ExceptionInfo && ExceptionInfo->ExceptionRecord) {
		Log(RED << L"[!!] Crash at addr 0x"
			<< std::hex << reinterpret_cast<uintptr_t>(ExceptionInfo->ExceptionRecord->ExceptionAddress)
			<< L" by 0x"
			<< std::hex << ExceptionInfo->ExceptionRecord->ExceptionCode << RESET);
	}
	else {
		Log(L"[!!] Crash" << std::endl);
	}

	if (iqvw64e_device_handle)
		intel_driver::Unload(iqvw64e_device_handle);

	return EXCEPTION_EXECUTE_HANDLER;
}

int ExtractParam(const int argc, wchar_t** argv, const wchar_t* param) {
	size_t plen = wcslen(param);
	for (int i = 1; i < argc; i++) {
		if (wcslen(argv[i]) == plen + 1ull && _wcsicmp(&argv[i][1], param) == 0 && argv[i][0] == '/')
			return i;
		else if (wcslen(argv[i]) == plen + 2ull && _wcsicmp(&argv[i][2], param) == 0 && argv[i][0] == '-' && argv[i][1] == '-')
			return i;
	}
	return -1;
}

int wmain(const int argc, wchar_t** argv) {
	SetUnhandledExceptionFilter(SimplestCrashHandler);

	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	DWORD dwMode = 0;
	if (GetConsoleMode(hConsole, &dwMode)) {
		dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		SetConsoleMode(hConsole, dwMode);
	}

	bool free = ExtractParam(argc, argv, L"free") > 0;
	bool indPagesMode = ExtractParam(argc, argv, L"indPages") > 0;
	bool passAllocationPtr = ExtractParam(argc, argv, L"PassAllocationPtr") > 0;
	kdmapper::AllocationMode mode = kdmapper::AllocationMode::AllocatePool;

	if (free)
		Log(GOLD << L"[+] Free pool memory after usage enabled" << RESET << std::endl);

	if (indPagesMode) {
		Log(GOLD << L"[+] Allocate Independent Pages mode enabled" << RESET << std::endl);
		mode = kdmapper::AllocationMode::AllocateIndependentPages;
	}

	if (free && indPagesMode) {
		Log(ERROR_COLOR << L"[-] Can't use --free and --indPages at the same time" << RESET << std::endl);
		return -1;
	}

	if (indPagesMode)

	if (passAllocationPtr)
		Log(CYAN << L"[+] Pass Allocation Ptr as first param enabled" << RESET << std::endl);

	int drvIndex = -1;
	for (int i = 1; i < argc; i++) {
		if (std::filesystem::path(argv[i]).extension().string().compare(".sys") == 0) {
			drvIndex = i;
			break;
		}
	}

	if (drvIndex <= 0)
		return -1;

	const std::wstring driver_path = argv[drvIndex];

	if (!std::filesystem::exists(driver_path)) {
		Log(ERROR_COLOR << L"[-] File " << driver_path << L" doesn't exist" << RESET << std::endl);
		return -1;
	}

	iqvw64e_device_handle = intel_driver::Load();

	if (iqvw64e_device_handle == INVALID_HANDLE_VALUE) {
		return -1;
	}

	std::vector<uint8_t> raw_image = { 0 };
	if (!utils::ReadFileToMemory(driver_path, &raw_image)) {
		Log(ERROR_COLOR << L"[-] Failed to read image to memory" << RESET << std::endl);
		intel_driver::Unload(iqvw64e_device_handle);
		return -1;
	}


	NTSTATUS exitCode = 0;
	if (!kdmapper::MapDriver(iqvw64e_device_handle, raw_image.data(), 0, 0, free, true, mode, passAllocationPtr, &exitCode)) {
		Log(ERROR_COLOR << L"[-] Failed to map " << driver_path << RESET << std::endl);
		intel_driver::Unload(iqvw64e_device_handle);
		return -1;
	}

	if (!intel_driver::Unload(iqvw64e_device_handle))
		Log(ORANGE << L"[-] Warning failed to fully unload vulnerable Driver " << RESET << std::endl);

	Log(GOLD << L"[+] success" << RESET << std::endl);
}

#endif