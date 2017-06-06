#pragma once
#include <Windows.h>
#include "dx9hook.h"
#include "Languages.h"

class FFXIVDLL;
class OverlayRenderer;

class ImGuiConfigWindow{
private:
	FFXIVDLL *dll;
	OverlayRenderer *mRenderer;
	bool mConfigVisibility;
	bool mConfigVisibilityPrev;
	TCHAR mSettingFilePath[1024];
	char mLanguageChoice[8192];
	std::string mAboutText;

	int readIni(TCHAR *k1, TCHAR *k2, int def = 0, int min = 0x80000000, int max = 0x7fffffff);
	float readIni(TCHAR *k1, TCHAR *k2, float def = 0, float min = -1, float max = 1);
	void readIni(TCHAR *k1, TCHAR *k2, char *def, char *target, int bufSize);

	void writeIni(TCHAR *k1, TCHAR *k2, int val);
	void writeIni(TCHAR *k1, TCHAR *k2, float val);
	void writeIni(TCHAR *k1, TCHAR *k2, const char* val);

public:

	int UseDrawOverlay = true;
	bool ShowEveryDPS = true;
	int fontSize = 17;
	bool bold = true;
	int border = 2;
	int transparency = 100;
	int padding = 4;
	bool hideOtherUserName = 0;
	char capturePath[512] = "";
	int captureFormat = D3DXIFF_BMP;
	char fontName[512] = "Segoe UI";

	bool showTimes;

	int combatResetTime;

	ImGuiConfigWindow(FFXIVDLL *dll, OverlayRenderer *renderer);
	~ImGuiConfigWindow();

	void Show();
	void Render();
};

