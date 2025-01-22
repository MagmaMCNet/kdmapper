#include "utils.hpp"

void HexToRGB(const std::string& hex, int& r, int& g, int& b) {
    std::stringstream ss;
    ss << std::hex << hex.substr(0, 2);
    ss >> r;
    ss.clear();
    ss << std::hex << hex.substr(2, 2);
    ss >> g;
    ss.clear();
    ss << std::hex << hex.substr(4, 2);
    ss >> b;
}
void BlendColor(int r1, int g1, int b1, int r2, int g2, int b2, float ratio, int& r, int& g, int& b) {
    r = static_cast<int>(r1 + ratio * (r2 - r1));
    g = static_cast<int>(g1 + ratio * (g2 - g1));
    b = static_cast<int>(b1 + ratio * (b2 - b1));
}

std::string ColorText(const std::string& text, const std::string& hexFrom, const std::string& hexTo) {
    int r1, g1, b1, r2, g2, b2;
    HexToRGB(hexFrom, r1, g1, b1);
    HexToRGB(hexTo, r2, g2, b2);

    int length = 0;
    for (size_t i = 0; i < text.length(); ++i) {
        if ((text[i] & 0xC0) != 0x80) {
            ++length;
        }
    }

    std::string coloredText;
    int charIndex = 0;
    for (size_t i = 0; i < text.length();) {
        float ratio = static_cast<float>(charIndex) / static_cast<float>(length - 1);
        int r, g, b;
        BlendColor(r1, g1, b1, r2, g2, b2, ratio, r, g, b);

        std::stringstream ss;
        ss << "\033[38;2;" << r << ";" << g << ";" << b << "m";

        if ((text[i] & 0x80) == 0) {
            ss << text[i];
            ++i;
        }
        else if ((text[i] & 0xE0) == 0xC0) {
            ss << text[i] << text[i + 1];
            i += 2;
        }
        else if ((text[i] & 0xF0) == 0xE0) {
            ss << text[i] << text[i + 1] << text[i + 2];
            i += 3;
        }
        else if ((text[i] & 0xF8) == 0xF0) {
            ss << text[i] << text[i + 1] << text[i + 2] << text[i + 3];
            i += 4;
        }
        coloredText += ss.str();
        ++charIndex;
    }

    coloredText += "\033[0m";
    return coloredText;
}
std::wstring ColorText(const std::wstring& wtext, const std::string& hexFrom, const std::string& hexTo) {
	std::string text(wtext.begin(), wtext.end());
	std::string color = ColorText(text, hexFrom, hexTo);
	return std::wstring(color.begin(), color.end());
}
std::wstring utils::GetFullTempPath() {
	wchar_t temp_directory[MAX_PATH + 1] = { 0 };
	const uint32_t get_temp_path_ret = GetTempPathW(sizeof(temp_directory) / 2, temp_directory);
	if (!get_temp_path_ret || get_temp_path_ret > MAX_PATH + 1) {
		Log(L"[-] Failed to get temp path" << std::endl);
		return L"";
	}
	if (temp_directory[wcslen(temp_directory) - 1] == L'\\')
		temp_directory[wcslen(temp_directory) - 1] = 0x0;

	return std::wstring(temp_directory);
}

bool utils::ReadFileToMemory(const std::wstring& file_path, std::vector<uint8_t>* out_buffer) {
	std::ifstream file_ifstream(file_path, std::ios::binary);

	if (!file_ifstream)
		return false;

	out_buffer->assign((std::istreambuf_iterator<char>(file_ifstream)), std::istreambuf_iterator<char>());
	file_ifstream.close();

	return true;
}

bool utils::CreateFileFromMemory(const std::wstring& desired_file_path, const char* address, size_t size) {
	std::ofstream file_ofstream(desired_file_path.c_str(), std::ios_base::out | std::ios_base::binary);

	if (!file_ofstream.write(address, size)) {
		file_ofstream.close();
		return false;
	}

	file_ofstream.close();
	return true;
}

uint64_t utils::GetKernelModuleAddress(const std::string& module_name) {
	void* buffer = nullptr;
	DWORD buffer_size = 0;

	NTSTATUS status = NtQuerySystemInformation(static_cast<SYSTEM_INFORMATION_CLASS>(nt::SystemModuleInformation), buffer, buffer_size, &buffer_size);

	while (status == nt::STATUS_INFO_LENGTH_MISMATCH) {
		if (buffer != nullptr)
			VirtualFree(buffer, 0, MEM_RELEASE);

		buffer = VirtualAlloc(nullptr, buffer_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		status = NtQuerySystemInformation(static_cast<SYSTEM_INFORMATION_CLASS>(nt::SystemModuleInformation), buffer, buffer_size, &buffer_size);
	}

	if (!NT_SUCCESS(status)) {
		if (buffer != nullptr)
			VirtualFree(buffer, 0, MEM_RELEASE);
		return 0;
	}

	const auto modules = static_cast<nt::PRTL_PROCESS_MODULES>(buffer);
	if (!modules)
		return 0;

	for (auto i = 0u; i < modules->NumberOfModules; ++i) {
		const std::string current_module_name = std::string(reinterpret_cast<char*>(modules->Modules[i].FullPathName) + modules->Modules[i].OffsetToFileName);

		if (!_stricmp(current_module_name.c_str(), module_name.c_str()))
		{
			const uint64_t result = reinterpret_cast<uint64_t>(modules->Modules[i].ImageBase);

			VirtualFree(buffer, 0, MEM_RELEASE);
			return result;
		}
	}

	VirtualFree(buffer, 0, MEM_RELEASE);
	return 0;
}

BOOLEAN utils::bDataCompare(const BYTE* pData, const BYTE* bMask, const char* szMask) {
	for (; *szMask; ++szMask, ++pData, ++bMask)
		if (*szMask == 'x' && *pData != *bMask)
			return 0;
	return (*szMask) == 0;
}

uintptr_t utils::FindPattern(uintptr_t dwAddress, uintptr_t dwLen, BYTE* bMask, const char* szMask) {
	size_t max_len = dwLen - strlen(szMask);
	for (uintptr_t i = 0; i < max_len; i++)
		if (bDataCompare((BYTE*)(dwAddress + i), bMask, szMask))
			return (uintptr_t)(dwAddress + i);
	return 0;
}

PVOID utils::FindSection(const char* sectionName, uintptr_t modulePtr, PULONG size) {
	size_t namelength = strlen(sectionName);
	PIMAGE_NT_HEADERS headers = (PIMAGE_NT_HEADERS)(modulePtr + ((PIMAGE_DOS_HEADER)modulePtr)->e_lfanew);
	PIMAGE_SECTION_HEADER sections = IMAGE_FIRST_SECTION(headers);
	for (DWORD i = 0; i < headers->FileHeader.NumberOfSections; ++i) {
		PIMAGE_SECTION_HEADER section = &sections[i];
		if (memcmp(section->Name, sectionName, namelength) == 0 &&
			namelength == strlen((char*)section->Name)) {
			if (!section->VirtualAddress) {
				return 0;
			}
			if (size) {
				*size = section->Misc.VirtualSize;
			}
			return (PVOID)(modulePtr + section->VirtualAddress);
		}
	}
	return 0;
}