#pragma once

namespace Settings
{
	bool HandChams = false;
	bool Thirdperson = false;
	bool WeaponLaser = false;
	bool Speed = false;
	float SpeedValue = 1.5f;

	bool NameESP = false;
	bool BoxESP2D = false;
	ImVec4 BoxESP2D_Color = { 255.0f, 255.0f, 255.0f, 255.0f };

	bool Snaplines = false;
	float Snaplines_Thickness = 1.0f;
	ImVec4 Snaplines_Color = { 255.0f, 255.0f, 255.0f, 255.0f };
	int SnaplinesMode = 0;
};