#include "stdafx.h"
#include <windows.h>
#include <WtsApi32.h>
#include "mem.h"
#include "gh_d3d9.h"
#include "Imgui/imgui.h"
#include "Imgui/imgui_impl_dx9.h"
#include "Imgui/imgui_impl_win32.h"
#include "Settings.h"
#include "sdk/SDK.h"
#include <algorithm>
#include <vector>

HMODULE Instance = nullptr;
bool bInit = false;
tEndScene oEndScene = nullptr;
tDrawIndexedPrimitive oDrawIndexedPrimitive = nullptr;
tSetStreamSource oSetStreamSource = nullptr;
WNDPROC oWndProc = nullptr;
LPDIRECT3DDEVICE9 pD3DDevice = nullptr;
void* d3d9Device[119];

void Unload()
{
	TrampHook((char*)d3d9Device[100], (char*)oSetStreamSource, 7);
	TrampHook((char*)d3d9Device[82], (char*)oDrawIndexedPrimitive, 7);
	TrampHook((char*)d3d9Device[42], (char*)oEndScene, 7);
	SetWindowLongPtr(d3dwindow, GWL_WNDPROC, (LONG_PTR)oWndProc);
	//CoFreeLibrary(Instance);
	FreeLibrary(Instance);
}

bool bShow = false;
HRESULT APIENTRY hkEndScene(LPDIRECT3DDEVICE9 pDevice)
{
	if (bInit == false)
	{
		pD3DDevice = pDevice;
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
		ImGui_ImplWin32_Init(d3dwindow);
		ImGui_ImplDX9_Init(pDevice);
		GenerateTexture(pDevice, &Red, D3DCOLOR_ARGB(255, 255, 0, 0));
		GenerateTexture(pDevice, &Green, D3DCOLOR_RGBA(0, 255, 0, 255));
		GenerateTexture(pDevice, &Blue, D3DCOLOR_ARGB(255, 0, 0, 255));
		GenerateTexture(pDevice, &Yellow, D3DCOLOR_ARGB(255, 255, 255, 0));
		bInit = true;
	}

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImDrawList* drawlist = ImGui::GetBackgroundDrawList();
	drawlist->AddText(ImVec2(10, 10), IM_COL32_WHITE, "COD : World at War cheat");
	drawlist->AddText(ImVec2(10, 30), IM_COL32_WHITE, "Developed by Lyceion & FuryFearless & Lufzys");

	if (GetAsyncKeyState(VK_INSERT) & 1)
		bShow = !bShow;

	if (GetAsyncKeyState(VK_END) & 1)
		Unload();

	if (bShow)
		ImGui::GetIO().MouseDrawCursor = 1;
	else
		ImGui::GetIO().MouseDrawCursor = 0;

	if (bShow)
	{
		ImGui::Begin("COD : World at War cheat", &bShow, ImGuiWindowFlags_NoSavedSettings || ImGuiWindowFlags_NoScrollbar);
		ImGui::Checkbox("Thirdperson", &Settings::Thirdperson);
		ImGui::Checkbox("Hand Chams", &Settings::HandChams);
		ImGui::Checkbox("Weapon Laser", &Settings::WeaponLaser);
		ImGui::Checkbox("Speed", &Settings::Speed);
		ImGui::SameLine();
		ImGui::SliderFloat("Value", &Settings::SpeedValue, 1.5f, 20.0f);
		ImGui::Spacing(); 	ImGui::Spacing();

		ImGui::Checkbox("Show Names", &Settings::NameESP);
		ImGui::Checkbox("Box ESP", &Settings::BoxESP2D);
		ImGui::SameLine();
		ImGui::ColorEdit4("Box ESP", (float*)&Settings::BoxESP2D_Color, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoInputs);
		ImGui::Spacing(); 	ImGui::Spacing();

		ImGui::Checkbox("Snaplines", &Settings::Snaplines);
		ImGui::SameLine();
		ImGui::ColorEdit4("Snaplines", (float*)&Settings::Snaplines_Color, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoInputs);
		ImGui::SliderFloat("Snaplines Thickness", &Settings::Snaplines_Thickness, 1.0f, 10.0f);
		const char* snaplinesMode_Items[] = { "Feet", "Head" };
		ImGui::Combo("Snaplines Mode", &Settings::SnaplinesMode, snaplinesMode_Items, IM_ARRAYSIZE(snaplinesMode_Items));
		ImGui::End();
	}

	std::vector<Vector3> targets;
	for (int i = 1; i < 12; i++)
	{
		uintptr_t ent = *(uintptr_t*)(0x18E73C0 + (i * 0x88)); // 0x18E73C0 - 0x18E745C
		if (!ent) continue;
		uintptr_t isZombie = *(uintptr_t*)(ent + 0x4);
		if (isZombie < 2) continue;
		//drawlist->AddText(ImVec2(10, (i * 15) + 20), IM_COL32_WHITE, std::to_string(isZombie).c_str());
		int health = *(int*)(ent + 0x1C8);
		if (health < 0) continue;
		targets.push_back(*(Vector3*)(ent + 0x154));

		if (Settings::Snaplines)
		{
			Vector3 foot_pos = *(Vector3*)(ent + 0x18);
			Vector3 head_pos = *(Vector3*)(ent + 0x154);
			Vector2 foot;
			Vector2 head;

			if (WorldToScreen(foot_pos, foot)) {
				if (WorldToScreen(head_pos, head))
				{
					float head_x = head.y - foot.y;
					float width = head_x / 2;
					float center = width / -2;
					ImVec2 mode = (Settings::SnaplinesMode == 0) ? ImVec2(foot.x, foot.y) : ImVec2(foot.x - center + ((center * 2) / 2) + 1, head.y);
					ImVec2 start = (Settings::SnaplinesMode == 0) ? ImVec2(ImGui::GetIO().DisplaySize.x / 2, ImGui::GetIO().DisplaySize.y + Settings::Snaplines_Thickness) : ImVec2(ImGui::GetIO().DisplaySize.x / 2, 0 - Settings::Snaplines_Thickness);
					drawlist->AddLine(start, mode, IM_COL32_BLACK, Settings::Snaplines_Thickness + 3.0f);
					drawlist->AddLine(start, mode, ImGui::ColorConvertFloat4ToU32(Settings::Snaplines_Color), Settings::Snaplines_Thickness);
				}
			}
		}

		if (Settings::BoxESP2D)
		{
			Vector3 foot_pos = *(Vector3*)(ent + 0x18);
			Vector3 head_pos = *(Vector3*)(ent + 0x154);
			Vector2 foot;
			Vector2 head;

			if (WorldToScreen(foot_pos, foot)) {
				if (WorldToScreen(head_pos, head))
				{
					float head_x = head.y - foot.y;
					float width = head_x / 2;
					float center = width / -2;

					float CenterX = ImGui::GetIO().DisplaySize.x / 2;
					float CenterY = ImGui::GetIO().DisplaySize.y / 2;

					drawlist->AddRect(ImVec2(foot.x + center, foot.y), ImVec2(foot.x - center, head.y), IM_COL32_BLACK, 0.0f, 0, 3.0f);
					drawlist->AddRect(ImVec2(foot.x + center, foot.y), ImVec2(foot.x - center, head.y), ImGui::ColorConvertFloat4ToU32(Settings::BoxESP2D_Color));
				}
			}
		}

		if (Settings::NameESP)
		{
			Vector3 foot_pos = *(Vector3*)(ent + 0x18);
			Vector3 head_pos = *(Vector3*)(ent + 0x154);
			Vector2 foot;
			Vector2 head;

			if (WorldToScreen(foot_pos, foot)) {
				if (WorldToScreen(head_pos, head))
				{
					float head_x = head.y - foot.y;
					float width = head_x / 2;
					float center = width / -2;

					drawlist->AddText(ImVec2(foot.x - center + (((center * 2) - ImGui::CalcTextSize("Zombie").x) / 2) + 1, head.y - 25 + 1), IM_COL32_BLACK, "Zombie");
					drawlist->AddText(ImVec2(foot.x - center + (((center * 2) - ImGui::CalcTextSize("Zombie").x) / 2) - 1, head.y - 25 - 1), IM_COL32_BLACK, "Zombie");
					drawlist->AddText(ImVec2(foot.x - center + (((center * 2) - ImGui::CalcTextSize("Zombie").x) / 2) + 1, head.y - 25 - 1), IM_COL32_BLACK, "Zombie");
					drawlist->AddText(ImVec2(foot.x - center + (((center * 2) - ImGui::CalcTextSize("Zombie").x) / 2) - 1, head.y - 25 + 1), IM_COL32_BLACK, "Zombie");

					drawlist->AddText(ImVec2(foot.x - center + (((center * 2) - ImGui::CalcTextSize("Zombie").x) / 2), head.y - 25), IM_COL32_WHITE, "Zombie");
				}
			}
		}

		if (Settings::Thirdperson)
		{
			*(byte*)(0x21C777C) = 1;

			// draw crosshair for thirdperson
			float CenterX = ImGui::GetIO().DisplaySize.x / 2;
			float CenterY = ImGui::GetIO().DisplaySize.y / 2 + 15;

			drawlist->AddLine(ImVec2(CenterX - 15, CenterY), ImVec2(CenterX - 5, CenterY), IM_COL32_BLACK, 3.0f);
			drawlist->AddLine(ImVec2(CenterX + 15, CenterY), ImVec2(CenterX + 5, CenterY), IM_COL32_BLACK, 3.0f);
			drawlist->AddLine(ImVec2(CenterX, CenterY - 15), ImVec2(CenterX, CenterY - 5), IM_COL32_BLACK, 3.0f);
			drawlist->AddLine(ImVec2(CenterX, CenterY + 15), ImVec2(CenterX, CenterY + 5), IM_COL32_BLACK, 3.0f);

			drawlist->AddLine(ImVec2(CenterX - 15, CenterY), ImVec2(CenterX - 5, CenterY), IM_COL32_WHITE);
			drawlist->AddLine(ImVec2(CenterX + 15, CenterY), ImVec2(CenterX + 5, CenterY), IM_COL32_WHITE);
			drawlist->AddLine(ImVec2(CenterX, CenterY - 15), ImVec2(CenterX, CenterY - 5), IM_COL32_WHITE);
			drawlist->AddLine(ImVec2(CenterX, CenterY + 15), ImVec2(CenterX, CenterY + 5), IM_COL32_WHITE);
		}
		else
			*(byte*)(0x21C777C) = 0;

		if (Settings::WeaponLaser)
			*(byte*)(0x21C6F94) = 1;
		else
			*(byte*)(0x21C6F94) = 0;

		if (Settings::Speed)
			*(float*)(0x021CBB68) = Settings::SpeedValue;
		else
			*(float*)(0x021CBB68) = 1.5f;
	}

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

	return oEndScene(pDevice);
}

LPDIRECT3DVERTEXBUFFER9 Stream_Data;
UINT Offset = 0;
UINT Stride = 0;

HRESULT APIENTRY hkDrawIndexedPrimitive(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE PrimType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount)
{
	if (pDevice->GetStreamSource(0, &Stream_Data, &Offset, &Stride) == D3D_OK)
		Stream_Data->Release();

	if (Settings::HandChams)
	{
		if (Stride == 32 && primCount == 3940 && NumVertices == 2446)
		{
			pDevice->SetRenderState(D3DRS_ZENABLE, false);
			pDevice->SetTexture(0, Red);
			oDrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
			pDevice->SetTexture(0, Green);
			pDevice->SetRenderState(D3DRS_ZENABLE, true);
		}
	}

	return oDrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
}

HRESULT APIENTRY hkSetStreamSource(LPDIRECT3DDEVICE9 pDevice, UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT sStride)
{
	if (StreamNumber == 0)
		Stride = sStride;

	return oSetStreamSource(pDevice, StreamNumber, pStreamData, OffsetInBytes, sStride);
}

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	if (true && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

DWORD WINAPI Init(HMODULE hModule)
{
	while(GetModuleHandle("d3d9.dll") == NULL) {	}
	Sleep(250);

	if (GetD3D9Device(d3d9Device, sizeof(d3d9Device)))
	{
		oSetStreamSource = (tSetStreamSource)TrampHook((char*)d3d9Device[100], (char*)hkSetStreamSource, 7);
		oDrawIndexedPrimitive = (tDrawIndexedPrimitive)TrampHook((char*)d3d9Device[82], (char*)hkDrawIndexedPrimitive, 7);
		oEndScene = (tEndScene)TrampHook((char*)d3d9Device[42], (char*)hkEndScene, 7);
		do
			d3dwindow = GetProcessWindow();
		while (d3dwindow == NULL);
		oWndProc = (WNDPROC)SetWindowLongPtr(d3dwindow, GWL_WNDPROC, (LONG_PTR)WndProc);
	}
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		Instance = hModule;
		DisableThreadLibraryCalls(hModule);
		CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)Init, hModule, 0, nullptr));
	}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}