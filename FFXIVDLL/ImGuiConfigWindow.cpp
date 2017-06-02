#include "imgui/imgui.h"
#include "ImGuiConfigWindow.h"
#include <Windows.h>
#include <windowsx.h>
#include <Psapi.h>
#include "FFXIVDLL.h"
#include "DPSWindowController.h"
#include "DOTWindowController.h"

ImGuiConfigWindow::ImGuiConfigWindow(FFXIVDLL *dll, OverlayRenderer *renderer) :
	dll(dll),
	mRenderer(renderer),
	mConfigVisibility(true)
{
	int i = 0, h = 0;
	GetModuleFileNameEx(GetCurrentProcess(), NULL, mSettingFilePath, MAX_PATH);
	for (i = 0; i < 128 && mSettingFilePath[i * 2] && mSettingFilePath[i * 2 + 1]; i++)
		h ^= ((int*)mSettingFilePath)[i];
	ExpandEnvironmentStrings(L"%APPDATA%", mSettingFilePath, MAX_PATH);
	wsprintf(mSettingFilePath + wcslen(mSettingFilePath), L"\\ffxiv_overlay_config_%d.txt", h);

	Languages::language = (Languages::LANGUAGE)readIni(L"UI", L"Language", 0, 0, Languages::_LANGUAGE_COUNT);
	UseDrawOverlay = readIni(L"UI", L"UseOverlay", 1, 0, 1);
	fontSize_old = fontSize = readIni(L"UI", L"FontSize", 17, 9, 36);
	bold = readIni(L"UI", L"FontBold", 1, 0, 1)?true:false;
	border = readIni(L"UI", L"FontBorder", 0, 0, 3);
	readIni(L"UI", L"FontName", "Segoe UI", fontName, sizeof(fontName));
	
	readIni(L"Capture", L"Path", "", capturePath, sizeof(capturePath));
	captureFormat = readIni(L"Capture", L"Format", D3DXIFF_BMP);
	switch (captureFormat) {
	case D3DXIFF_BMP:
	case D3DXIFF_PNG:
	case D3DXIFF_JPG:
		break;
	default:
		captureFormat = D3DXIFF_BMP;
	}

	if (readIni(L"Meters", L"Locked", 0, 0, 1)) {
		dll->process()->wDOT.lock();
		dll->process()->wDPS.lock();
	}
	transparency = readIni(L"Meters", L"Transparency", 100, 0, 255);
	padding = readIni(L"Meters", L"Padding", 4, 0, 32);
	dll->process()->wDOT.setTransparency(transparency);
	dll->process()->wDPS.setTransparency(transparency);
	dll->process()->wDOT.setPaddingRecursive(padding);
	dll->process()->wDPS.setPaddingRecursive(padding);

	dll->process()->wDOT.visible = readIni(L"DOTMeter", L"Visible", 1, 0, 1);
	dll->process()->wDOT.xF = readIni(L"DOTMeter", L"x", 0.1f, 0.f, 1.f);
	dll->process()->wDOT.yF = readIni(L"DOTMeter", L"y", 0.1f, 0.f, 1.f);

	dll->process()->wDPS.visible = readIni(L"DPSMeter", L"Visible", 1, 0, 1);
	dll->process()->wDPS.xF = readIni(L"DPSMeter", L"x", 0.1f, 0.f, 1.f);
	dll->process()->wDPS.yF = readIni(L"DPSMeter", L"y", 0.1f, 0.f, 1.f);
	UseDrawOverlayEveryone = readIni(L"DPSMeter", L"ShowEveryone", 1, 0, 1);
	combatResetTime = readIni(L"DPSMeter", L"CombatResetTime", 10, 5, 60);

	dll->process()->ReloadLocalization();

	char *ptr = mLanguageChoice;
	char cur[512];
	for (int i = 0; i < Languages::_LANGUAGE_COUNT; i++) {
		TCHAR *curw = Languages::getLanguageName(i);
		WideCharToMultiByte(CP_UTF8, 0, curw, -1, cur, 512, 0, 0);
		int len = strlen(cur);
		strncpy(ptr, cur, len);
		ptr += len;
		*(ptr++) = 0;
	}
	*(ptr++) = 0;
}

ImGuiConfigWindow::~ImGuiConfigWindow() {
	writeIni(L"UI", L"Language", Languages::language);
	writeIni(L"UI", L"UseOverlay", UseDrawOverlay);
	writeIni(L"UI", L"FontSize", fontSize);
	writeIni(L"UI", L"FontBold", (int)bold);
	writeIni(L"UI", L"FontBorder", border);
	writeIni(L"UI", L"FontName", fontName);

	writeIni(L"Capture", L"Path", capturePath);
	writeIni(L"Capture", L"Format", captureFormat);

	writeIni(L"Meters", L"Locked", (int) dll->process()->wDOT.isLocked());
	writeIni(L"Meters", L"Transparency", transparency);
	writeIni(L"Meters", L"Padding", padding);

	writeIni(L"DOTMeter", L"Visible", dll->process()->wDOT.visible);
	writeIni(L"DOTMeter", L"x", dll->process()->wDOT.xF);
	writeIni(L"DOTMeter", L"y", dll->process()->wDOT.yF);

	writeIni(L"DPSMeter", L"Visible", dll->process()->wDPS.visible);
	writeIni(L"DPSMeter", L"x", dll->process()->wDPS.xF);
	writeIni(L"DPSMeter", L"y", dll->process()->wDPS.yF);
	writeIni(L"DPSMeter", L"ShowEveryone", UseDrawOverlayEveryone);
	writeIni(L"DPSMeter", L"CombatResetTime", combatResetTime);

}

int ImGuiConfigWindow::readIni(TCHAR *k1, TCHAR *k2, int def, int min, int max) {
	int val = GetPrivateProfileInt(k1, k2, def, mSettingFilePath);
	return max(min, min(max, val));
}
float ImGuiConfigWindow::readIni(TCHAR *k1, TCHAR *k2, float def, float min, float max) {
	TCHAR buf[256];
	float val = def;
	GetPrivateProfileString(k1, k2, L"0", buf, sizeof(buf) / sizeof(TCHAR), mSettingFilePath);
	swscanf(buf, L"%f", &val);
	return max(min, min(max, val));
}
void ImGuiConfigWindow::readIni(TCHAR *k1, TCHAR *k2, char *def, char *target, int bufSize) {
	TCHAR *buf = new TCHAR[bufSize];
	GetPrivateProfileString(k1, k2, nullptr, buf, bufSize, mSettingFilePath);
	if (wcslen(buf) == 0)
		strncpy(target, def, bufSize);
	else 
		WideCharToMultiByte(CP_UTF8, 0, buf, -1, target, bufSize, 0, nullptr);
	delete[] buf;
}

void ImGuiConfigWindow::writeIni(TCHAR *k1, TCHAR *k2, int val) {
	TCHAR str[256];
	swprintf(str, sizeof(str)/sizeof(TCHAR), L"%d", val);
	WritePrivateProfileString(k1, k2, str, mSettingFilePath);
}
void ImGuiConfigWindow::writeIni(TCHAR *k1, TCHAR *k2, float val) {
	TCHAR str[256];
	swprintf(str, sizeof(str) / sizeof(TCHAR), L"%f", val);
	WritePrivateProfileString(k1, k2, str, mSettingFilePath);
}
void ImGuiConfigWindow::writeIni(TCHAR *k1, TCHAR *k2, char* val) {
	TCHAR buf[512] = { 0 };
	MultiByteToWideChar(CP_UTF8, NULL, val, -1, buf, sizeof(buf) / sizeof(TCHAR));
	WritePrivateProfileString(k1, k2, buf, mSettingFilePath);
}

void ImGuiConfigWindow::Show() {
	mConfigVisibility = true;
}

void ImGuiConfigWindow::Render() {
	if (mConfigVisibility) {
		ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = ImVec4(0, 0, 0, transparency/255.f);
		ImGui::SetNextWindowSize(ImVec2(320, 240), ImGuiSetCond_FirstUseEver);
		ImGui::Begin(Languages::get("OPTION_WINDOW_TITLE"), &mConfigVisibility);

		ImGui::Combo(Languages::get("OPTION_LANGUAGE"), &Languages::language, mLanguageChoice);

		if (ImGui::Button(dll->process()->wDPS.isLocked() ? 
			Languages::get("OPTION_UNLOCK") : 
			Languages::get("OPTION_LOCK"))) {
			dll->process()->wDPS.lock();
			dll->process()->wDOT.lock();
		} ImGui::SameLine();
		if (ImGui::Button(dll->process()->wDPS.visible ? Languages::get("OPTION_DPS_HIDE") : Languages::get("OPTION_DPS_SHOW"))) {
			dll->process()->wDPS.toggleVisibility();
		} ImGui::SameLine();
		if (ImGui::Button(dll->process()->wDOT.visible ? Languages::get("OPTION_DOT_HIDE") : Languages::get("OPTION_DOT_SHOW"))) {
			dll->process()->wDOT.toggleVisibility();
		} ImGui::SameLine();
		if (ImGui::Button(hideOtherUser ? Languages::get("OPTION_OTHERS_SHOW") : Languages::get("OPTION_OTHERS_HIDE"))) {
			hideOtherUser = !hideOtherUser;
		}
		ImGui::Checkbox(Languages::get("OPTION_FONT_BOLD"), &bold);
		ImGui::SliderInt(Languages::get("OPTION_TEXT_BORDER"), &border, 0, 3);
		ImGui::SliderInt(Languages::get("OPTION_FONT_SIZE"), &fontSize, 9, 36);
		ImGui::SliderInt(Languages::get("OPTION_TRANSPARENCY"), &transparency, 0, 255);
		ImGui::InputText(Languages::get("OPTION_FONT_NAME"), fontName, sizeof(fontName));
		ImGui::SliderInt(Languages::get("OPTION_DPS_RESET_TIME"), &combatResetTime, 5, 60);
		ImGui::SliderInt(Languages::get("OPTION_METER_PADDING"), &padding, 0, 32);
		if (ImGui::Button(Languages::get("OPTION_APPLY"))) {
			dll->process()->wDPS.setTransparency(transparency);
			dll->process()->wDOT.setTransparency(transparency);
			dll->process()->wDPS.setPaddingRecursive(padding);
			dll->process()->wDOT.setPaddingRecursive(padding);
			dll->process()->mCombatResetTime = combatResetTime * 1000;
			if (fontSize_old != fontSize) {
				mRenderer->ReloadImGuiFromConfig();
				fontSize_old = fontSize;
			}
			mRenderer->ReloadFromConfig();
			dll->process()->ReloadLocalization();
		}
		ImGui::InputText(Languages::get("OPTION_CAPTURE_PATH"), capturePath, sizeof(capturePath));
		ImGui::RadioButton("BMP", &captureFormat, D3DXIFF_BMP);
		ImGui::SameLine();
		ImGui::RadioButton("PNG", &captureFormat, D3DXIFF_PNG);
		ImGui::SameLine();
		ImGui::RadioButton("JPG", &captureFormat, D3DXIFF_JPG);
		if (ImGui::Button(Languages::get("OPTION_DPS_RESET"))) {
			dll->process()->ResetMeter();
		}
		if (ImGui::Button(Languages::get("OPTION_QUIT"))) {
			dll->spawnSelfUnloader();
		}
		ImGui::End();
	}
}