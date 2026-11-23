// FOUC Ultrawide — ultrawide (21:9, 32:9, ...) support for FlatOut: Ultimate Carnage
// (Steam Collector's Edition, Fouc.exe 4242504 bytes). Loaded by Ultimate ASI Loader.
//
// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <windows.h>
#include <cstdint>
#include <cstring>
#include <cstddef>
#include <cmath>
#include <initializer_list>
#include "memory.h"

// ---- minimal game types ----
class DeviceD3d {
public:
	uint8_t _0[0x8];
	uint32_t nResolutionX; // +8
	uint32_t nResolutionY; // +C
	uint8_t _10[0x8F4];
	float fAspectX; // +904
	float fAspectY; // +908
};
static_assert(offsetof(DeviceD3d, fAspectX) == 0x904);

class GameFlow {
public:
	uint8_t _0[0x4A4];
	int nGameState; // +4A4
	uint8_t _4A8[0x6C];
	int nGameRules; // +514
};
static_assert(offsetof(GameFlow, nGameRules) == 0x514);

enum { GAME_STATE_RACE = 2 };
enum { GR_STUNT = 2 };

auto& pDeviceD3d = *(DeviceD3d**)0x84665C;
auto& pGameFlow = *(GameFlow**)0x9298FAC;
auto& pLoadingScreen = *(void**)0x929902C;

// ---- aspect state ----
double fAspectRatio = 16.0 / 9.0;
double fSpacingFixAmount43 = (4.0 / 3.0) / fAspectRatio;
double f43AspectCorrection = 480 * fAspectRatio; // 853
double f43AspectCorrectionCenter = f43AspectCorrection * 0.5; // 426
double fButtonPromptSpacing = 16 * fSpacingFixAmount43;
float fFontScaleMultiplier = fSpacingFixAmount43;
float fFontScaleMultiplierActive = fFontScaleMultiplier; // what the text-justify patches point at
float fLUAAspect = 2.25;
float f43AspectCorrection_flt = f43AspectCorrection;
float f43AspectCorrectionCenter_flt = f43AspectCorrectionCenter;
double f43AspectCorrection_324 = f43AspectCorrectionCenter + 4;

void RecalculateAspectRatio() {
	auto y = *(float*)0x724BB8;
	if (y <= 0) return;
	fAspectRatio = *(float*)0x724BB4 / y;
	f43AspectCorrection = 480 * fAspectRatio;
	fSpacingFixAmount43 = (4.0 / 3.0) / fAspectRatio;
	f43AspectCorrection_flt = f43AspectCorrection;
	f43AspectCorrectionCenter = f43AspectCorrection * 0.5;
	f43AspectCorrectionCenter_flt = f43AspectCorrectionCenter;
	fButtonPromptSpacing = 16 * fSpacingFixAmount43;
	fFontScaleMultiplier = 0.5 * fSpacingFixAmount43;
	fLUAAspect = ((*(float*)0x71098C) * 640.0) / f43AspectCorrection;
	f43AspectCorrection_324 = f43AspectCorrectionCenter + 4;
}

// ---- aspect ratio: always derived from the selected resolution ----
// The game turns its stored aspect index (0x764A94, set in the configuration tool, defaults to 16:9)
// into an aspect pair at 0x45741D and hands it to the renderer at 0x45747F. Overwrite that pair with
// resX/resY so any resolution gets the right aspect regardless of the stored setting.
void __cdecl SetAspectFromResolution(float* aspectXY) {
	int resX = *(int*)0x764A84;
	int resY = *(int*)0x764A88;
	if (resX <= 0 || resY <= 0) return;
	aspectXY[0] = (float)((double)resX / resY * 9.0);
	aspectXY[1] = 9.0f;
}

uintptr_t AspectFromResolutionASM_jmp = 0x457485;
void __attribute__((naked)) AspectFromResolutionASM() {
	__asm__ (
		"pushad\n\t"
		"lea eax, [esp+0x30]\n\t" // [esp+0x10] before pushad
		"push eax\n\t"
		"call %1\n\t"
		"add esp, 4\n\t"
		"popad\n\t"
		"mov ecx, dword ptr ds:0x8464C4\n\t"
		"jmp %0\n\t"
			:
			: "m" (AspectFromResolutionASM_jmp), "i" (SetAspectFromResolution)
	);
}

// Extra 21:9 / 32:9 entries for the configuration tool's aspect list (indices 4 and 5), so configs
// saved with those values still show a valid entry. The real aspect comes from the resolution above.
float f219_21 = 21.0;
float f219_9 = 9.0;
uintptr_t Aspect219ASM_jmp = 0x45747F;
void __attribute__((naked)) __fastcall Aspect219ASM() {
	__asm__ (
		"fstp st(1)\n\t"
		"fstp st\n\t"
		"fld dword ptr %1\n\t"
		"fstp dword ptr [esp+0x10]\n\t"
		"fld dword ptr %2\n\t"
		"fstp dword ptr [esp+0x14]\n\t"
		"jmp %0\n\t"
			:
			: "m" (Aspect219ASM_jmp), "m" (f219_21), "m" (f219_9)
	);
}

float f329_32 = 32.0;
float f329_9 = 9.0;
uintptr_t Aspect329ASM_jmp = 0x45747F;
void __attribute__((naked)) __fastcall Aspect329ASM() {
	__asm__ (
		"fstp st(1)\n\t"
		"fstp st\n\t"
		"fld dword ptr %1\n\t"
		"fstp dword ptr [esp+0x10]\n\t"
		"fld dword ptr %2\n\t"
		"fstp dword ptr [esp+0x14]\n\t"
		"jmp %0\n\t"
			:
			: "m" (Aspect329ASM_jmp), "m" (f329_32), "m" (f329_9)
	);
}

uintptr_t AspectJumpTable[] = {
	0x457429,
	0x457435,
	0x45744F,
	0x457469,
	(uintptr_t)&Aspect219ASM,
	(uintptr_t)&Aspect329ASM,
};

// The configuration tool's aspect names, by aspect index. The game's own table stops at 5:4.
const char* aAspectNames[] = { "4:3", "16:10", "16:9", "5:4", "21:9", "32:9" };
constexpr double aAspectValues[] = { 4.0 / 3.0, 16.0 / 10.0, 16.0 / 9.0, 5.0 / 4.0, 21.0 / 9.0, 32.0 / 9.0 };
constexpr int nAspectCount = sizeof(aAspectValues) / sizeof(aAspectValues[0]);

LRESULT __stdcall SendDlgItemMessageAHooked(HWND hDlg, int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam) {
	// the aspect combo (1124) is filled with the 4 stock entries, then its selection is set: add ours first
	if (nIDDlgItem == 1124 && Msg == CB_SETCURSEL) {
		for (int i = 4; i < nAspectCount; i++) {
			if (SendDlgItemMessageA(hDlg, 1124, CB_FINDSTRINGEXACT, -1, (LPARAM)aAspectNames[i]) != CB_ERR) continue;
			auto idx = SendDlgItemMessageA(hDlg, 1124, CB_ADDSTRING, 0, (LPARAM)aAspectNames[i]);
			SendDlgItemMessageA(hDlg, 1124, CB_SETITEMDATA, idx, i);
		}
	}
	return SendDlgItemMessageA(hDlg, nIDDlgItem, Msg, wParam, lParam);
}
auto pSendDlgItemMessageAHooked = &SendDlgItemMessageAHooked;

// Replaces the game's resolution -> aspect index detection (which only knows 4:3 .. 16:9 and picks
// 16:9 for anything wider): returns the nearest known aspect, so 3440x1440 is 21:9 and 5120x1440 is 32:9.
int GetAspectRatioForSetup(int x, int y) {
	if (x <= 0 || y <= 0) return 2;
	auto aspect = (double)x / (double)y;
	int best = 0;
	for (int i = 1; i < nAspectCount; i++) {
		if (std::abs(aspect - aAspectValues[i]) < std::abs(aspect - aAspectValues[best])) best = i;
	}
	return best;
}

// Display mode list label "%ld x %ld  (%s)": the game falls back to "<unknown>" for index >= 4.
// Hooked at 0x456066 (mov eax, "<unknown>"), with eax = aspect index; returns to 0x45606B.
const char* __cdecl GetAspectName(int index) {
	if (index >= 0 && index < nAspectCount) return aAspectNames[index];
	return (const char*)0x6DDF68; // "<unknown>"
}
uintptr_t AspectLabelASM_jmp = 0x45606B;
void __attribute__((naked)) AspectLabelASM() {
	__asm__ (
		"push ecx\n\t"
		"push edx\n\t"
		"push eax\n\t"
		"call %1\n\t"
		"add esp, 4\n\t"
		"pop edx\n\t"
		"pop ecx\n\t"
		"jmp %0\n\t"
			:
			: "m" (AspectLabelASM_jmp), "i" (GetAspectName)
	);
}

// ---- 3D camera: Hor+ frustum ----
class Camera {
public:
	uint8_t _0[0xF0];
	float fLeft;
	float fRight;
	float fTop;
	float fBottom;
	uint8_t _100[0xC];
	float fNearZ;
	float fFarZ;
	float fFOV;
};
static_assert(offsetof(Camera, fFOV) == 0x114);

struct CameraExtents {
	float fLeft;
	float fRight;
	float fTop;
	float fBottom;
};

const double fOrigAspect = 16.0 / 9.0; // the game's FOV values are tuned for 16:9

void __fastcall UltrawideFOV(Camera* pCam) {
	RecalculateAspectRatio();

	auto mult = 1.0 / fOrigAspect;
	auto v = tan(pCam->fFOV * 0.5) * pCam->fNearZ;
	pCam->fRight = v * fAspectRatio * mult;
	pCam->fLeft = -v * fAspectRatio * mult;
	pCam->fBottom = v * mult;
	pCam->fTop = -v * mult;
}

uintptr_t UltrawideFOVASM_jmp = 0x4F44FC;
void __attribute__((naked)) __fastcall UltrawideFOVASM() {
	__asm__ (
		"fstp dword ptr [esi+0x104]\n\t"
		"pushad\n\t"
		"mov ecx, esi\n\t"
		"call %1\n\t"
		"popad\n\t"
		"sub dword ptr [esp+0x1C], 1\n\t"
		"jmp %0\n\t"
			:
			: "m" (UltrawideFOVASM_jmp), "i" (UltrawideFOV)
	);
}

void __fastcall UltrawideFOVSky(CameraExtents* pExtents, Camera* pCam) {
	auto mult = 1.0 / fOrigAspect;
	auto v = tan(pCam->fFOV * 0.5);
	pExtents->fRight = v * fAspectRatio * mult;
	pExtents->fLeft = -v * fAspectRatio * mult;
	pExtents->fBottom = v * mult;
	pExtents->fTop = -v * mult;
}

uintptr_t UltrawideFOVSkyASM_jmp = 0x591FAD;
void __attribute__((naked)) __fastcall UltrawideFOVSkyASM() {
	__asm__ (
		"pushad\n\t"
		"mov ecx, esi\n\t"
		"mov edx, ebx\n\t"
		"call %1\n\t"
		"popad\n\t"
		"mov eax, [edi]\n"
		"mov edx, [eax+0xD0]\n\t"
		"jmp %0\n\t"
			:
			: "m" (UltrawideFOVSkyASM_jmp), "i" (UltrawideFOVSky)
	);
}

void ApplyAspectAndCameraPatches() {
	// configuration tool: extra aspect entries + auto-select from resolution
	Memory::Patch(0x45642F, &pSendDlgItemMessageAHooked);
	Memory::PatchRelative(Memory::JMP, 0x455C80, &GetAspectRatioForSetup);
	Memory::PatchRelative(Memory::JMP, 0x456066, &AspectLabelASM);

	// aspect switch: allow indices 4/5, then always override with the resolution's aspect
	Memory::Patch<uint8_t>(0x45741D + 2, 5);
	Memory::Patch(0x457422 + 3, &AspectJumpTable);
	Memory::PatchRelative(Memory::JMP, 0x45747F, &AspectFromResolutionASM);

	// 3D camera and sky frustum
	Memory::Fill(0x4F44A1, 0x90, 0x4F44A6 - 0x4F44A1);
	Memory::PatchRelative(Memory::JMP, 0x4F44F6, &UltrawideFOVASM);
	Memory::PatchRelative(Memory::JMP, 0x591FA5, &UltrawideFOVSkyASM);
}

// ---- menu text scale ----
float* fTextScale = nullptr;
void UltrawideTextScale() {
	*fTextScale *= 640.0;
	*fTextScale /= f43AspectCorrection;
}

uintptr_t UltrawideTextScaleASM_jmp = 0x4CAF7B;
void __attribute__((naked)) __fastcall UltrawideTextScaleASM() {
	__asm__ (
		"fstp dword ptr [esp]\n\t"
		"mov %2, esp\n\t"

		"pushad\n\t"
		"call %1\n\t"
		"popad\n\t"

		"mov [esp+0x5C], ebp\n\t"
		"mov dword ptr [eax], 0xFFFFFFFF\n\t"
		"jmp %0\n\t"
			:
			: "m" (UltrawideTextScaleASM_jmp), "i" (UltrawideTextScale), "m" (fTextScale)
	);
}

// ---- LUA GUI element re-centering ----
class GUIRectangle {
public:
	uint8_t _0[0x4];
	float fPosX;
	float fPosY;
	float fSizeX;
	float fSizeY;
};

void __fastcall LUAResizer(GUIRectangle* pRect) {
	// hack for listboxes
	if (pRect->fPosX == 0.0 && pRect->fSizeX > 640.0) return;

	pRect->fPosX -= 320.0;
	pRect->fPosX += f43AspectCorrectionCenter;
}

void __fastcall LUASliderResizer(GUIRectangle* pRect) {
	static float pLastX = 0;
	if (pRect->fPosX == pLastX) return;

	pRect->fPosX -= 320.0;
	pRect->fPosX += f43AspectCorrectionCenter;

	pLastX = pRect->fPosX;
}

uintptr_t LUAResizerASM_jmp = 0x5EC912;
void __attribute__((naked)) __fastcall LUAResizerASM() {
	__asm__ (
		"pushad\n\t"
		"mov ecx, esi\n\t"
		"call %1\n\t"
		"popad\n\t"

		"test edx, edx\n\t"
		"fld dword ptr [esi+4]\n\t"
		"jmp %0\n\t"
			:
			: "m" (LUAResizerASM_jmp), "i" (LUAResizer)
	);
}

void __fastcall LUATextResizer(float* pText) {
	RecalculateAspectRatio();

	pText[0] -= 320.0;
	pText[0] += f43AspectCorrectionCenter;
}

uintptr_t LUATextResizerASM_jmp = 0x5F8544;
void __attribute__((naked)) __fastcall LUATextResizerASM() {
	__asm__ (
		"pushad\n\t"
		"mov ecx, eax\n\t"
		"call %1\n\t"
		"popad\n\t"

		"fld dword ptr [eax]\n"
		"fstp dword ptr [esp+0x20]\n"
		"lea ecx, [esp+0x14]\n\t"
		"fld dword ptr [eax+4]\n\t"
		"mov eax, [ebx]\n\t"
		"mov edx, [eax+0x14]\n\t"
		"jmp %0\n\t"
			:
			: "m" (LUATextResizerASM_jmp), "i" (LUATextResizer)
	);
}

uintptr_t LUASliderResizerASM_jmp = 0x5ED0B6;
void __attribute__((naked)) __fastcall LUASliderResizerASM() {
	__asm__ (
		"fstp dword ptr [esp+0x1C]\n\t"

		"pushad\n\t"
		"mov ecx, esi\n\t"
		"call %1\n\t"
		"popad\n\t"

		"fld dword ptr [esi+4]\n\t"
		"jmp %0\n\t"
			:
			: "m" (LUASliderResizerASM_jmp), "i" (LUASliderResizer)
	);
}

float* pUpgradePos1 = nullptr;
float* pUpgradePos2 = nullptr;
float* pUpgradePos3 = nullptr;
float* pUpgradePos4 = nullptr;
float fUpgradePosNew1[2];
float fUpgradePosNew2[2];
float fUpgradePosNew3[2];
float fUpgradePosNew4[2];
void LUAUpgradeSliderResizer() {
	memcpy(fUpgradePosNew1, pUpgradePos1, sizeof(fUpgradePosNew1));
	memcpy(fUpgradePosNew2, pUpgradePos2, sizeof(fUpgradePosNew2));
	memcpy(fUpgradePosNew3, pUpgradePos3, sizeof(fUpgradePosNew3));
	memcpy(fUpgradePosNew4, pUpgradePos4, sizeof(fUpgradePosNew4));

	fUpgradePosNew1[0] += f43AspectCorrectionCenter - 320.0;
	fUpgradePosNew2[0] += f43AspectCorrectionCenter - 320.0;
	fUpgradePosNew3[0] += f43AspectCorrectionCenter - 320.0;
	fUpgradePosNew4[0] += f43AspectCorrectionCenter - 320.0;

	pUpgradePos1 = fUpgradePosNew1;
	pUpgradePos2 = fUpgradePosNew2;
	pUpgradePos3 = fUpgradePosNew3;
	pUpgradePos4 = fUpgradePosNew4;
}

uintptr_t LUAUpgradeSliderResizerASM_jmp = 0x5ED2FA;
void __attribute__((naked)) __fastcall LUAUpgradeSliderResizerASM() {
	__asm__ (
		"mov ebx, [esp+0x74]\n\t"
		"mov %2, ebx\n\t"
		"mov ebx, [esp+0x78]\n\t"
		"mov %3, ebx\n\t"
		"mov ebx, [esp+0x7C]\n\t"
		"mov %4, ebx\n\t"
		"mov ebx, [esp+0x80]\n\t"
		"mov %5, ebx\n\t"

		"pushad\n\t"
		"call %1\n\t"
		"popad\n\t"

		"mov ebx, %2\n\t"
		"mov [esp+0x74], ebx\n\t"
		"mov ebx, %3\n\t"
		"mov [esp+0x78], ebx\n\t"
		"mov ebx, %4\n\t"
		"mov [esp+0x7C], ebx\n\t"
		"mov ebx, %5\n\t"
		"mov [esp+0x80], ebx\n\t"

		"mov ebx, [esp+0x74]\n\t"
		"fld dword ptr [ebx]\n\t"

		"jmp %0\n\t"
			:
			: "m" (LUAUpgradeSliderResizerASM_jmp), "i" (LUAUpgradeSliderResizer), "m" (pUpgradePos1), "m" (pUpgradePos2), "m" (pUpgradePos3), "m" (pUpgradePos4)
	);
}

// ---- in-race 2D UI (pause menu, message boxes, finish screens) ----
void PatchIngameUIScale(bool patch) {
	static bool bLast = false;
	if (bLast == patch) return;
	bLast = patch;

	Memory::Patch(0x46BA9F + 2,  patch ? (uintptr_t)&f43AspectCorrection_flt : 0x6F7920);
	Memory::Patch(0x489878 + 2,  patch ? (uintptr_t)&f43AspectCorrection_flt : 0x6F7920);

	Memory::Patch(0x4AB421 + 2, patch ? (uintptr_t)&f43AspectCorrectionCenter : 0x6F7BE0); // pause menu options center
	Memory::Patch(0x4B11BC + 2, patch ? (uintptr_t)&f43AspectCorrection_flt : 0x6F7920); // pause menu width
	Memory::Patch(0x4B1207 + 2, patch ? (uintptr_t)&f43AspectCorrectionCenter : 0x6F7BE0); // pause menu selection center
	Memory::Patch(0x4B13FF + 2, patch ? (uintptr_t)&f43AspectCorrectionCenter : 0x6F7BE0); // pause menu stuff
	Memory::Patch(0x4B6864 + 2, patch ? (uintptr_t)&f43AspectCorrection_flt : 0x6F7920); // arcade race finish background width
	Memory::Patch(0x4B52AA + 2, patch ? (uintptr_t)&f43AspectCorrection_flt : 0x6F7920); // beat the bomb finish background width
	Memory::Patch(0x4C30AF + 2, patch ? (uintptr_t)&f43AspectCorrection_flt : 0x6F7920); // arcade race finish background width
	Memory::Patch(0x4B3F60 + 2, patch ? (uintptr_t)&f43AspectCorrection_flt : 0x6F7920); // race finish top bar
	Memory::Patch(0x4B5A54 + 2, patch ? (uintptr_t)&f43AspectCorrection_flt : 0x6F7920); // career timetrial finish top bar
	Memory::Patch(0x4B73E4 + 2, patch ? (uintptr_t)&f43AspectCorrection_flt : 0x6F7920); // highjump finish top bar
	Memory::Patch(0x4C3B59 + 2, patch ? (uintptr_t)&f43AspectCorrection_flt : 0x6F7920); // derby finish top bar

	Memory::Patch(0x4CEF0B + 2, patch ? (uintptr_t)&f43AspectCorrection : 0x6F7918); // messagebox scale
	Memory::Patch(0x4BAE6C + 2, patch ? (uintptr_t)&f43AspectCorrectionCenter_flt : 0x6F7D80); // messagebox pos
	Memory::Patch(0x4BB7CC + 2, patch ? (uintptr_t)&f43AspectCorrectionCenter_flt : 0x6F7D80);
	Memory::Patch(0x4BEBAB + 2, patch ? (uintptr_t)&f43AspectCorrectionCenter_flt : 0x6F7D80);
	Memory::Patch(0x4BF567 + 2, patch ? (uintptr_t)&f43AspectCorrectionCenter_flt : 0x6F7D80);
	Memory::Patch(0x4C0079 + 2, patch ? (uintptr_t)&f43AspectCorrectionCenter_flt : 0x6F7D80);

	// The confirmation message box section (0x4BE8E8..0x4BEFAF, e.g. "Retry race?") always uses the
	// uniform scale (patched permanently in ApplyUltrawidePatches) and is excluded from the race text
	// squeeze, so its box quad must stay uniform in menus too.
	bool menu = !patch;
	if (menu) {
		Memory::Patch(0x4CEF0B + 2, (uintptr_t)&f43AspectCorrection); // messagebox quad scale
		Memory::Patch(0x4BEBAB + 2, (uintptr_t)&f43AspectCorrectionCenter_flt); // messagebox pos
		Memory::Patch(0x4BAE6C + 2, (uintptr_t)&f43AspectCorrectionCenter_flt);
		Memory::Patch(0x4BB7CC + 2, (uintptr_t)&f43AspectCorrectionCenter_flt);
		Memory::Patch(0x4BF567 + 2, (uintptr_t)&f43AspectCorrectionCenter_flt);
		Memory::Patch(0x4C0079 + 2, (uintptr_t)&f43AspectCorrectionCenter_flt);
	}

	// Lua GUI:InitMessageBox dialog (main menu "Are you sure?" etc.), drawn by 0x4CE130
	Memory::Patch(0x4CE165 + 2, menu ? (uintptr_t)&f43AspectCorrection : 0x6F7918); // scale X = resX / width
	Memory::Patch(0x4CE425 + 2, menu ? (uintptr_t)&f43AspectCorrection : 0x6F7918); // full-screen dim quad width
	Memory::Patch(0x4CE4A2 + 2, menu ? (uintptr_t)&f43AspectCorrectionCenter : 0x6F7BE0); // box centre X
	Memory::Patch(0x4CE6B2 + 2, menu ? (uintptr_t)&f43AspectCorrectionCenter : 0x6F7BE0); // text centre X
}

// ---- full-screen Bink videos (intro logos, trailer): fit to height instead of filling the width ----
// Hooked call at 0x4FA8DF: DrawVideoQuad(void*, int w, int h, float x, float y, float sx, float sy, float a)
void FixVideoRect(uint32_t* args) {
	if (!pDeviceD3d) return;
	double resX = pDeviceD3d->nResolutionX;
	double resY = pDeviceD3d->nResolutionY;
	double w = args[1], h = args[2];
	if (w <= 0 || h <= 0 || resY <= 0) return;
	if (resX / resY <= w / h) return; // screen not wider than the video: stock behaviour is fine

	auto f = (float*)&args[3];
	double width = resY * (w / h);
	f[0] = (resX - width) * 0.5; // x
	f[1] = 0;                    // y
	f[2] = width / w;            // sx
	f[3] = resY / h;             // sy
}

uintptr_t VideoDrawASM_jmp = 0x4FA4A0;
void __attribute__((naked)) VideoDrawASM() {
	__asm__ (
		"pushad\n\t"
		"lea eax, [esp+0x24]\n\t"
		"push eax\n\t"
		"call %1\n\t"
		"add esp, 4\n\t"
		"popad\n\t"
		"jmp %0\n\t"
			:
			: "m" (VideoDrawASM_jmp), "i" (FixVideoRect)
	);
}

uint8_t bInMusicBar = 0;

// wraps the only call to the music bar draw (0x46C8B9 -> 0x46D160, this in edi, one stack arg, ret 4)
uintptr_t MusicBarDraw_call = 0x46D160;
void __attribute__((naked)) MusicBarDrawASM() {
	__asm__ (
		"mov byte ptr %1, 1\n\t"
		"push dword ptr [esp+4]\n\t"
		"call %0\n\t"
		"mov byte ptr %1, 0\n\t"
		"ret 4\n\t"
			:
			: "m" (MusicBarDraw_call), "m" (bInMusicBar)
	);
}

// confirmation message box section: entry 0x4BE8E8 (mov eax,[0x9298FD8]), single exit via call 0x46FE30 at 0x4BEFA4
uint8_t bInMessageBox = 0;
uintptr_t MessageBoxEnterASM_jmp = 0x4BE8ED;
void __attribute__((naked)) MessageBoxEnterASM() {
	__asm__ (
		"mov byte ptr %1, 1\n\t"
		"mov eax, dword ptr ds:0x9298FD8\n\t"
		"jmp %0\n\t"
			:
			: "m" (MessageBoxEnterASM_jmp), "m" (bInMessageBox)
	);
}

uintptr_t MessageBoxExitASM_jmp = 0x46FE30;
void __attribute__((naked)) MessageBoxExitASM() {
	__asm__ (
		"mov byte ptr %1, 0\n\t"
		"jmp %0\n\t"
			:
			: "m" (MessageBoxExitASM_jmp), "m" (bInMessageBox)
	);
}

// The other four confirmation message box HUD elements are virtual Draw methods (ret 4). Wrap them via
// their vtable slots: set the flag, call the original with the caller's args, clear the flag.
#define MESSAGEBOX_WRAPPER(name, orig) \
	uintptr_t name##_orig = orig; \
	uintptr_t name##_ret = 0; \
	void __attribute__((naked)) name() { \
		__asm__ ( \
			"mov byte ptr %2, 1\n\t" \
			"pop dword ptr %1\n\t" \
			"call %0\n\t" \
			"mov byte ptr %2, 0\n\t" \
			"push dword ptr %1\n\t" \
			"ret\n\t" \
				: \
				: "m" (name##_orig), "m" (name##_ret), "m" (bInMessageBox) \
		); \
	}

MESSAGEBOX_WRAPPER(MessageBoxDrawA, 0x4BAAE0)
MESSAGEBOX_WRAPPER(MessageBoxDrawB, 0x4BB440)
MESSAGEBOX_WRAPPER(MessageBoxDrawC, 0x4BF1B0)
MESSAGEBOX_WRAPPER(MessageBoxDrawD, 0x4BFBD0)

bool IsInRace() {
	if (pLoadingScreen) return true;
	if (!pGameFlow) return false;
	return pGameFlow->nGameState == GAME_STATE_RACE && pGameFlow->nGameRules != GR_STUNT;
}

void PatchTextJustify(bool patch) {
	PatchIngameUIScale(IsInRace());

	// the music bar already uses a uniform scale (0x46D1C8 patch), so it must not get the race text squeeze too
	bool isInGame = (IsInRace() && !bInMusicBar && !bInMessageBox && *(float*)0x716034 == 480.0f);

	// The loading screen (0x5984D0) lays its text out inside the pillarboxed 16:9 image, not the
	// full screen, so its glyphs need the 16:9 correction rather than the full-aspect one.
	if (pLoadingScreen) {
		double loadingAspect = fAspectRatio < 16.0 / 9.0 ? fAspectRatio : 16.0 / 9.0;
		fFontScaleMultiplierActive = 0.5 * (4.0 / 3.0) / loadingAspect;
	}
	else {
		fFontScaleMultiplierActive = fFontScaleMultiplier;
	}

	static bool bLast = false;
	if (bLast != (patch && isInGame)) {
		Memory::Patch(0x5A9A76 + 2, patch && isInGame ? (uintptr_t)&fFontScaleMultiplierActive : 0x7138DC);
	}
	bLast = patch && isInGame;

	static bool bLast2 = false;
	if (bLast2 != isInGame) {
		Memory::Patch(0x5AF262 + 2, isInGame ? (uintptr_t)&fFontScaleMultiplierActive : 0x7138DC);
		Memory::Patch(0x5AF13B + 2, isInGame ? (uintptr_t)&fFontScaleMultiplierActive : 0x7138DC);
	}
	bLast2 = isInGame;
}

void DoTextJustify() { PatchTextJustify(true); }
void UndoTextJustify() { PatchTextJustify(false); }

float* fMPPlayerNameText = nullptr;
void UndoTextJustifyMPPlayerNames() {
	PatchTextJustify(false);
	PatchIngameUIScale(false);
}

uintptr_t TextJustifyASM_jmp = 0x5A9139;
void __attribute__((naked)) __fastcall TextJustifyASM() {
	__asm__ (
		"pushad\n\t"
		"call %1\n\t"
		"popad\n\t"

		"push ebp\n\t"
		"mov ebp, esp\n\t"
		"and esp, 0xFFFFFFF8\n\t"
		"sub esp, 0x7C\n\t"
		"jmp %0\n\t"
			:
			: "m" (TextJustifyASM_jmp), "i" (DoTextJustify)
	);
}

void __attribute__((naked)) __fastcall TextJustifyUndoASM() {
	__asm__ (
		"pushad\n\t"
		"call %0\n\t"
		"popad\n\t"
		"ret 0x18\n\t"
			:
			: "i" (UndoTextJustify)
	);
}

uintptr_t DrawMPNamesASM_jmp = 0x5A913B;
void __attribute__((naked)) DrawMPNamesASM() {
	__asm__ (
		"mov %2, esp\n\t"

		"pushad\n\t"
		"call %1\n\t"
		"popad\n\t"

		"push ebp\n\t"
		"mov ebp, esp\n\t"
		"and esp, 0xFFFFFFF8\n\t"
		"sub esp, 0x7C\n\t"
		"push ebx\n\t"
		"push esi\n\t"
		"jmp %0\n\t"
			:
			: "m" (DrawMPNamesASM_jmp), "i" (UndoTextJustifyMPPlayerNames), "m" (fMPPlayerNameText)
	);
}

// ---- loading screen: pillarbox the 16:9 image instead of stretching ----
void __fastcall FixLoadingScreenScale(float* min, float* max) {
	auto resX = pDeviceD3d->nResolutionX;
	auto resY = pDeviceD3d->nResolutionY;
	auto fDisplayAspect = pDeviceD3d->fAspectX / pDeviceD3d->fAspectY;

	min[1] = 0;
	max[1] = resY;

	double fLetterboxMultiplier = (resX * ((16.0 / 9.0) / fDisplayAspect) * 0.5);
	min[0] = resX * 0.5 - fLetterboxMultiplier;
	max[0] = resX * 0.5 + fLetterboxMultiplier;
}

void __attribute__((naked)) LoadingScreenScaleASM() {
	__asm__ (
		"pushad\n\t"
		"mov ecx, edi\n\t"
		"mov edx, esi\n\t"
		"call %0\n\t"
		"popad\n\t"
		"ret\n\t"
			:
			: "i" (FixLoadingScreenScale)
	);
}

void ApplyUltrawidePatches() {
	Memory::PatchRelative(Memory::JMP, 0x564350, &LoadingScreenScaleASM);

	Memory::PatchRelative(Memory::JMP, 0x4CAF6E, &UltrawideTextScaleASM);

	Memory::Patch(0x4CF39B, &f43AspectCorrection); // menu button prompts
	Memory::Patch(0x5F08C7, &f43AspectCorrection); // menu selected option text
	Memory::Patch(0x5F0B4A, &f43AspectCorrectionCenter); // menu selected option text centered
	Memory::Patch(0x4CB242 + 2, &fButtonPromptSpacing); // prompt spacing
	Memory::Patch(0x4CB428 + 2, &f43AspectCorrection); // prompt spacing

	Memory::Patch(0x4B1AB5 + 2, &f43AspectCorrection); // 3 2 1 go
	Memory::Patch(0x4B1E4B + 2, &f43AspectCorrection); // finish
	Memory::Patch(0x4B0AA6 + 2, &f43AspectCorrectionCenter); // replay hud pos

	Memory::PatchRelative(Memory::JMP, 0x5EC90D, &LUAResizerASM);
	Memory::PatchRelative(Memory::JMP, 0x5F8532, &LUATextResizerASM);
	Memory::PatchRelative(Memory::JMP, 0x5ED0AF, &LUASliderResizerASM);
	Memory::PatchRelative(Memory::JMP, 0x5ED2F4, &LUAUpgradeSliderResizerASM);

	// always draw lua sprites even if "offscreen"
	Memory::PatchRelative(Memory::JMP, 0x5E8CC0, 0x5E8CFF);

	PatchIngameUIScale(true);

	Memory::PatchRelative(Memory::JMP, 0x5A9130, &TextJustifyASM);
	Memory::PatchRelative(Memory::JMP, 0x5A9979, &TextJustifyUndoASM);
	Memory::PatchRelative(Memory::CALL, 0x4826F9, &DrawMPNamesASM);

	// "now playing" music bar (0x46D160): use uniform scale instead of resX/640
	Memory::Patch(0x46D1C8 + 2, &f43AspectCorrection);
	Memory::PatchRelative(Memory::CALL, 0x46C8B9, &MusicBarDrawASM);

	// confirmation message box (0x4BE8E8 section): uniform scale everywhere, no race text squeeze
	Memory::Patch(0x4BEA17 + 2, &f43AspectCorrection); // text scale
	Memory::Patch(0x4BEBED + 2, &f43AspectCorrectionCenter); // text centre
	Memory::Patch(0x4BEE14 + 2, &f43AspectCorrection_324); // arrows
	Memory::Patch(0x4BEF7B + 2, &f43AspectCorrection_324);
	Memory::PatchRelative(Memory::JMP, 0x4BE8E8, &MessageBoxEnterASM);
	Memory::PatchRelative(Memory::CALL, 0x4BEFA4, &MessageBoxExitASM);

	// the other four message box variants (same layout): text scale, text centre, arrows, plus vtable wrap
	for (auto addr : { 0x4BAD39, 0x4BB699, 0x4BF3D7, 0x4BFF37 }) Memory::Patch(addr + 2, &f43AspectCorrection);
	for (auto addr : { 0x4BAEA0, 0x4BB800, 0x4BF5A9, 0x4C00AD }) Memory::Patch(addr + 2, &f43AspectCorrectionCenter);
	for (auto addr : { 0x4BB06C, 0x4BB1D5, 0x4BB9CC, 0x4BBB35, 0x4BF7CE, 0x4BF935, 0x4C0279, 0x4C03E2 }) {
		Memory::Patch(addr + 2, &f43AspectCorrection_324);
	}
	Memory::Patch(0x6E7918, &MessageBoxDrawA);
	Memory::Patch(0x6E7940, &MessageBoxDrawB);
	Memory::Patch(0x6E79FC, &MessageBoxDrawC);
	Memory::Patch(0x6E7A20, &MessageBoxDrawD);

	// full-screen videos
	Memory::PatchRelative(Memory::CALL, 0x4FA8DF, &VideoDrawASM);

	uintptr_t aLUAAspectRefs[] = {
			0x5E2E0D, 0x5E2E50, 0x5E2FD0, 0x5E31A1, 0x5E325D, 0x5E3917, 0x5E3AFC,
			0x5EC661, 0x5EC6D6,
			0x5EC914, // menu options
			0x5ECA09, 0x5ECC1F, 0x5ECD0B, 0x5ECE7D, 0x5ECED3, 0x5ED0B8, 0x5ED10E,
			0x5ED1CE, 0x5ED261, 0x5ED4B0, 0x5ED530, 0x5ED5A8, 0x5ED626, 0x5EDA96,
			0x5EDF34, 0x5F443C, 0x5F855C, 0x5F865D, 0x5FECB8, 0x6009BC, 0x60121F,
			0x601290, 0x601379, 0x601466, 0x6015BC,
	};
	for (auto& addr : aLUAAspectRefs) {
		Memory::Patch(addr, &fLUAAspect);
	}
}

BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID) {
	if (fdwReason == DLL_PROCESS_ATTACH) {
		if (Memory::GetEntryPoint() != 0x24CEF7) {
			MessageBoxA(nullptr, "Unsupported Fouc.exe version (expected the Steam Collector's Edition, 4242504 bytes). Plugin disabled.", "FOUC Ultrawide", MB_ICONWARNING);
			return TRUE;
		}
		ApplyAspectAndCameraPatches();
		ApplyUltrawidePatches();
	}
	return TRUE;
}
