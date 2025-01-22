#pragma once
#include <sstream>
#include <iostream>
#include <iomanip>
#define RESET       "\033[0m"   // Reset to default color
#define BOLD        "\033[1m"   // Bold text
#define DIM         "\033[2m"   // Dim text
#define UNDERLINE   "\033[4m"   // Underlined text
#define BLINK       "\033[5m"   // Blink text
#define REVERSE     "\033[7m"   // Reverse (swap foreground and background)
#define HIDDEN      "\033[8m"   // Hidden text

// Standard colors
#define BLACK       "\033[30m"
#define RED         "\033[31m"
#define GREEN       "\033[32m"
#define YELLOW      "\033[33m"
#define BLUE        "\033[34m"
#define MAGENTA     "\033[35m"
#define CYAN        "\033[36m"
#define WHITE       "\033[37m"

// Bright colors
#define BRIGHT_BLACK   "\033[90m"
#define BRIGHT_RED     "\033[91m"
#define BRIGHT_GREEN   "\033[92m"
#define BRIGHT_YELLOW  "\033[93m"
#define BRIGHT_BLUE    "\033[94m"
#define BRIGHT_MAGENTA "\033[95m"
#define BRIGHT_CYAN    "\033[96m"
#define BRIGHT_WHITE   "\033[97m"

// Common color mixtures (predefined combinations)
#define ORANGE      "\033[33m"  // Yellow for orange approximation
#define PINK        "\033[95m"  // Bright magenta as pink
#define TEAL        "\033[36m"  // Cyan approximates teal
#define PURPLE      "\033[35m"  // Magenta for purple
#define WHITE  "\033[37m"  // White as light gray
#define DARK_GRAY   "\033[90m"  // Bright black for dark gray
#define GOLD        "\033[33m"  // Yellow for gold approximation

// Combined styles
#define BOLD_RED        "\033[1;31m"
#define BOLD_GREEN      "\033[1;32m"
#define BOLD_YELLOW     "\033[1;33m"
#define BOLD_BLUE       "\033[1;34m"
#define BOLD_MAGENTA    "\033[1;35m"
#define BOLD_CYAN       "\033[1;36m"
#define BOLD_WHITE      "\033[1;37m"

#define UNDERLINE_RED   "\033[4;31m"
#define UNDERLINE_GREEN "\033[4;32m"
#define UNDERLINE_BLUE  "\033[4;34m"

// Custom combinations for quick usage
#define ERROR_COLOR     "\033[1;31m"  // Bold red
#define SUCCESS_COLOR   "\033[1;32m"  // Bold green
#define WARNING_COLOR   "\033[1;33m"  // Bold yellow
#define INFO_COLOR      "\033[1;36m"  // Bold cyan
#define DEBUG_COLOR     "\033[1;35m"  // Bold magenta


std::string ColorText(const std::string& text, const std::string& hexFrom, const std::string& hexTo);
std::wstring ColorText(const std::wstring& text, const std::string& hexFrom, const std::string& hexTo);
#if defined(DISABLE_OUTPUT)
#define Log(content)
#else
#define Log(content) std::wcout << content;  // Use logger if output is enabled
#endif


#include <Windows.h>
#include <TlHelp32.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

#include "nt.hpp"
#ifdef PDB_OFFSETS
#include "SymbolsHandler.hpp"
#endif

namespace utils
{
	std::wstring GetFullTempPath();
	bool ReadFileToMemory(const std::wstring& file_path, std::vector<uint8_t>* out_buffer);
	bool CreateFileFromMemory(const std::wstring& desired_file_path, const char* address, size_t size);
	uint64_t GetKernelModuleAddress(const std::string& module_name);
	BOOLEAN bDataCompare(const BYTE* pData, const BYTE* bMask, const char* szMask);
	uintptr_t FindPattern(uintptr_t dwAddress, uintptr_t dwLen, BYTE* bMask, const char* szMask);
	PVOID FindSection(const char* sectionName, uintptr_t modulePtr, PULONG size);
}