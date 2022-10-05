#include "O2CSP.h"

#include "framework.h"

#include <iostream>

#include "mem.h"
#include "winver.h"

// TODO: Everything should be done using RAII approach, Initialize-Finalize is C style and is not recommended.

float* f_scale = (float*)0x5EED34;

void ImulEDXHook()
{
	int valueEDX = 0;
	__asm {
		MOV valueEDX,EDX
		PUSH EAX
	};
	float f_result = *f_scale * valueEDX;
	int i_result = static_cast<int>(f_result);
	__asm {
		MOV EDX,i_result
		POP EAX
	};
	return;
	/**std::cout <<
		"Multiplier: " << *f_scale << "\n"
		"EDX: " << valueEDX << "\n"
		"Float result: " << f_result << "\n"
		"Int result: " << i_result << std::endl;**/
}

void ImulECXHook()
{
	int valueECX = 0;
	__asm {
		MOV valueECX, ECX
		PUSH EAX
	};
	float f_result = *f_scale * valueECX;
	int i_result = static_cast<int>(f_result);
	__asm {
		MOV ECX, i_result
		POP EAX
	};
	return;
	/**std::cout <<
		"Multiplier: " << *f_scale << "\n"
		"ECX: " << valueECX << "\n"
		"Float result: " << f_result << "\n"
		"Int result: " << i_result << std::endl;**/
}

void ImulEBPHook()
{
	int valueEBP = 0;
	__asm {
		PUSH EAX
		PUSH ECX
		MOV ECX,[EBP]
		MOV valueEBP,ECX
	};
	float f_result = *f_scale * valueEBP;
	int i_result = static_cast<int>(f_result);
	__asm {
		MOV ECX, i_result
		MOV [EBP],ECX
		POP ECX
		POP EAX
		SUB EAX,[EBP]
	};
	return;
	/**std::cout <<
		"Multiplier: " << *f_scale << "\n"
		"EDX: " << valueEDX << "\n"
		"Float result: " << f_result << "\n"
		"Int result: " << i_result << std::endl;**/
}

void ImulEBPHook2()
{
	int valueEBP = 0;
	__asm {
		PUSH EAX
		PUSH ECX
		MOV ECX, [EBP]
		MOV valueEBP, ECX
	};
	float f_result = *f_scale * valueEBP;
	int i_result = static_cast<int>(f_result);
	__asm {
		MOV ECX, i_result
		MOV[EBP], ECX
		POP ECX
		POP EAX
		SUB ECX, [EBP]
	};
	return;
	/**std::cout <<
		"Multiplier: " << *f_scale << "\n"
		"EDX: " << valueEDX << "\n"
		"Float result: " << f_result << "\n"
		"Int result: " << i_result << std::endl;**/
}

uintptr_t moduleBase;

void ModifyScaleToFloat()
{
	DWORD curProtection = 0;
	BOOL hResult = VirtualProtect((void*)(moduleBase + 0x07F6B0), 5, PAGE_EXECUTE_READWRITE, &curProtection);
	if (hResult == NULL)
	{
		std::cout << "couldn't set permission on overwrite destination" << std::endl;
	}
	char data[5] = { 0xD9, 0x5E, 0x10, 0xEB, 0x25 };
	char* segmentForReplace = (char*)(moduleBase + 0x07F6B0);
	for (int i = 0; i < 5; i++)
	{
		*segmentForReplace = data[i];
		segmentForReplace++;
	}
	DWORD temp = 0;
	hResult = VirtualProtect((void*)(moduleBase + 0x07F6B0), 5, curProtection, &temp);
	if (hResult == NULL)
	{
		// TODO: Throw an error.
		// https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualprotect#return-value
	}

	hResult = VirtualProtect((void*)(moduleBase + 0x07E8E7), 1, PAGE_EXECUTE_READWRITE, &curProtection);
	char FMUL = 0xD8;
	segmentForReplace = (char*)(moduleBase + 0x07E8E7);
	*segmentForReplace = FMUL;
	hResult = VirtualProtect((void*)(moduleBase + 0x07E8E7), 1, curProtection, &temp);
	if (hResult == NULL)
	{
		// TODO: Throw an error.
		// https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualprotect#return-value
	}
}

void O2CSP::Hook()
{
	unsigned int g_win10Offset = 0;
	// TODO: check for errors.
	// https://docs.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulehandlea#return-value
	if ((moduleBase = (uintptr_t)GetModuleHandle("OTwo.exe")) == 0)
	{
		std::cout << "No OTwo.exe process found" << std::endl;
		return;
	}

	double g_winver = getSysOpType();
	//g_winver = 10;
	if (g_winver >= 10)
	{
		f_scale = (float*)(0x5EED34 + g_win10Offset);
	}
	std::cout << "winver: " << g_winver << '\n';
	std::cout << "win10Offset: " << g_win10Offset << std::endl;

	ModifyScaleToFloat();

	mem::Detour32((void*)(moduleBase + 0x07E844), (void*)&ImulEDXHook, 7); // Apply scale for measure line position/speed
	mem::Detour32((void*)(moduleBase + 0x07E32B), (void*)&ImulECXHook, 7); // Apply scale for rice notes position/speed
	mem::Detour32((void*)(moduleBase + 0x07E428), (void*)&ImulEDXHook, 7); // Apply scale for long note head position/speed
	mem::Detour32((void*)(moduleBase + 0x07E45A), (void*)&ImulECXHook, 7); // Apply scale for long note tail position/speed
	mem::Detour32((void*)(moduleBase + 0x07E3A9), (void*)&ImulEBPHook, 5); // Apply scale for long note body start position/speed
	mem::Detour32((void*)(moduleBase + 0x07E3C0), (void*)&ImulEBPHook2, 5); // Apply scale for long note body end position/speed
}
