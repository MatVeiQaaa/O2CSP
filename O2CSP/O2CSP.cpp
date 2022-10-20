#include "O2CSP.h"

#include "framework.h"

#include <iostream>

#include "mem.h"

#include "imgui/imgui.h"

// TODO: Everything should be done using RAII approach, Initialize-Finalize is C style and is not recommended.

uintptr_t moduleBase;

float* f_scale = (float*)0x5EED34;

float* speedMods[10] = {
(float*)0x47F656,
(float*)(0x47F656 + 9),
(float*)(0x47F656 + 9 * 2),
(float*)(0x47F656 + 9 * 3),
(float*)(0x47F656 + 9 * 4),
(float*)(0x47F656 + 9 * 5),
(float*)(0x47F656 + 9 * 6),
(float*)(0x47F656 + 9 * 7),
(float*)(0x47F656 + 9 * 8),
(float*)(0x47F656 + 9 * 9)
};

int* cspValue = (int*)(0x4CE462);

float* f_scale_first;

bool imguiInit = 0;

extern "C" {
	__declspec(dllexport) void Imgui(ImGuiContext* context, ImGuiMemAllocFunc memAlloc, ImGuiMemFreeFunc memFree, void* userData)
	{
		if (!imguiInit)
		{
			ImGui::SetCurrentContext(context);
			ImGui::SetAllocatorFunctions(memAlloc, memFree, userData);
			imguiInit = 1;
		}

		if (ImGui::CollapsingHeader("O2CSP"))
		{

			if (ImGui::TreeNode("Speed Modifiers"))
			{
				if (*speedMods[0] > 200)
					ImGui::InputInt("CSP", cspValue, 1, 50);
				else
					ImGui::InputFloat("X1.0", speedMods[0], 0.01f, 0.25f, "%.2f");
				ImGui::InputFloat("X1.5", speedMods[1], 0.01f, 0.25f, "%.2f");
				ImGui::InputFloat("X2.0", speedMods[2], 0.01f, 0.25f, "%.2f");
				ImGui::InputFloat("X2.5", speedMods[3], 0.01f, 0.25f, "%.2f");
				ImGui::InputFloat("X3.0", speedMods[4], 0.01f, 0.25f, "%.2f");
				ImGui::InputFloat("X3.5", speedMods[5], 0.01f, 0.25f, "%.2f");
				ImGui::InputFloat("X4.0", speedMods[6], 0.01f, 0.25f, "%.2f");
				ImGui::InputFloat("X4.5", speedMods[7], 0.01f, 0.25f, "%.2f");
				ImGui::InputFloat("X5.0", speedMods[8], 0.01f, 0.25f, "%.2f");
				ImGui::InputFloat("X6.0", speedMods[9], 0.01f, 0.25f, "%.2f");

				ImGui::TreePop;
			}
			if (ImGui::Button("Get address for scale"))
			{
				f_scale_first = (float*)mem::FindDMAAddy((moduleBase + 0x1C888C), { 0x3C, 0x10, 0x620, 0x10 });
			}
			
			if (f_scale_first)
			{
				ImGui::InputFloat("Speed modifier * 2", f_scale_first, 0.01f, 0.25f, "%.2f");
			}
		}
	}
}

__declspec(naked) void ImulEDXHook()
{
	__asm {
		PUSH EAX
		PUSH EDX
		MOV EAX,f_scale
		FLD [EAX]
		FIMUL [ESP]
		FISTP [ESP]
		MOV EDX,[ESP]
		ADD ESP,4
		POP EAX
		RET
	};
}

__declspec(naked) void ImulECXHook()
{
	__asm {
		PUSH EAX
		PUSH ECX
		MOV EAX,f_scale
		FLD [EAX]
		FIMUL [ESP]
		FISTP [ESP]
		MOV ECX,[ESP]
		ADD ESP,4
		POP EAX
		RET
	};
}

__declspec(naked) void ImulEBPHook()
{
	__asm {
		PUSH EAX
		PUSH EBP
		MOV EAX,f_scale
		FLD [EAX]
		FIMUL [ESP]
		FISTP [ESP]
		POP EBP
		POP EAX
		SUB EAX,EBP
		RET
	};
}

__declspec(naked) void ImulEBPHook2()
{
	__asm {
		PUSH EAX
		PUSH EBP
		MOV EAX,f_scale
		FLD [EAX]
		FIMUL [ESP]
		FISTP [ESP]
		POP EBP
		POP EAX
		SUB ECX,EBP
		RET
	};
}

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
	// TODO: check for errors.
	// https://docs.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulehandlea#return-value
	if ((moduleBase = (uintptr_t)GetModuleHandle("OTwo.exe")) == 0)
	{
		std::cout << "No OTwo.exe process found" << std::endl;
		return;
	}

	DWORD curProtection = 0;
	DWORD temp = 0;
	BOOL hResult = VirtualProtect((void*)(moduleBase + 0x07F656), 55, PAGE_EXECUTE_READWRITE, &curProtection);
	if (hResult == NULL)
	{
		std::cout << "couldn't set permission on overwrite destination" << std::endl;
	}
	
	hResult = VirtualProtect((void*)(moduleBase + 0x0D111C), 4, PAGE_EXECUTE_READWRITE, &curProtection);
	float* noteSpawnY = (float*)(moduleBase + 0x0D111C);
	*noteSpawnY = 10000;
	hResult = VirtualProtect((void*)(moduleBase + 0x0D111C), 4, curProtection, &temp);
	hResult = VirtualProtect((void*)(moduleBase + 0x0CE462), 4, PAGE_EXECUTE_READWRITE, &curProtection);

	/**double g_winver = getSysOpType();
	//g_winver = 10;
	if (g_winver >= 10)
	{
		g_win10Offset = 0x10000;
		f_scale = (float*)(0x5EED34 + g_win10Offset);
	}
	std::cout << "winver: " << g_winver << '\n';
	std::cout << "win10Offset: " << g_win10Offset << std::endl;**/

	ModifyScaleToFloat();

	mem::Detour32((void*)(moduleBase + 0x07E844), (void*)&ImulEDXHook, 7); // Apply scale for measure line position/speed
	mem::Detour32((void*)(moduleBase + 0x07E32B), (void*)&ImulECXHook, 7); // Apply scale for rice notes position/speed
	mem::Detour32((void*)(moduleBase + 0x07E428), (void*)&ImulEDXHook, 7); // Apply scale for long note head position/speed
	mem::Detour32((void*)(moduleBase + 0x07E45A), (void*)&ImulECXHook, 7); // Apply scale for long note tail position/speed
	mem::Detour32((void*)(moduleBase + 0x07E3A9), (void*)&ImulEBPHook, 5); // Apply scale for long note body start position/speed
	mem::Detour32((void*)(moduleBase + 0x07E3C0), (void*)&ImulEBPHook2, 5); // Apply scale for long note body end position/speed
}
