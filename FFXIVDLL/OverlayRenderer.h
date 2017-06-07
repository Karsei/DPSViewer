#pragma once
#include <Windows.h>
#include <deque>
#include <mutex>
#include <map>
#include <vector>
#include "ImGuiConfigWindow.h"

enum CONTROL_STATUS_ID : uint32_t{
	CONTROL_STATUS_DEFAULT,
	CONTROL_STATUS_PRESS,
	CONTROL_STATUS_HOVER,
	CONTROL_STATUS_FOCUS,
	_CONTROL_STATUS_COUNT
};
enum CONTROL_LAYOUT_DIRECTION {
	LAYOUT_DIRECTION_HORIZONTAL,
	LAYOUT_DIRECTION_VERTICAL,
	LAYOUT_DIRECTION_VERTICAL_TABLE,
	LAYOUT_ABSOLUTE
};
enum CONTROL_TEXT_TYPE : uint32_t {
	CONTROL_TEXT_STRING,
	CONTROL_TEXT_RESNAME,
	CONTROL_TEXT_FILENAME
};
enum CONTROL_CHILD_TYPE : uint32_t {
	CHILD_TYPE_FOREGROUND,
	CHILD_TYPE_NORMAL,
	CHILD_TYPE_BACKGROUND,
	_CHILD_TYPE_COUNT
};

class FFXIVDLL;
class WindowControllerBase;

#ifdef _WIN64
typedef ID3D11ShaderResourceView* PDXTEXTURETYPE;
#else
typedef LPDIRECT3DTEXTURE9 PDXTEXTURETYPE;
#endif

class OverlayRenderer {
	friend class Hooks;
	friend class ImGuiConfigWindow;
public:

	class Control {
		friend class OverlayRenderer;
		friend class OverlayRendererDX9;
		friend class OverlayRendererDX11;

		std::recursive_mutex layoutLock;
		std::vector<Control*> children[_CHILD_TYPE_COUNT];
		Control* parent;

	public:

		Control();
		Control(CONTROL_LAYOUT_DIRECTION direction);
		Control(std::wstring txt, CONTROL_TEXT_TYPE useIcon, int align);
		Control(float width, float height, DWORD background);
		~Control();

		class Status {
		public:
			int useDefault;
			DWORD foreground;
			DWORD background;
			DWORD border;
		};
		const int SIZE_WRAP = -1;
		const int SIZE_FILL = -2;

		CONTROL_LAYOUT_DIRECTION layoutDirection;

		virtual bool hasCallback() const {
			return false;
		}

		int visible;

		int relativePosition;
		int relativeSize;
		union {
			int x;
			float xF;
		};
		union {
			int y;
			float yF;
		};
		union {
			int width;
			float widthF;
		};
		union {
			int height;
			float heightF;
		};

		int calcX;
		int calcY;
		int calcWidth;
		int calcHeight;

		int border;
		int margin;
		int padding;
		int statusFlag[_CONTROL_STATUS_COUNT];
		Status statusMap[_CONTROL_STATUS_COUNT];

		CONTROL_TEXT_TYPE useIcon;
		std::wstring text;
		int textAlign;

		void setPaddingRecursive(int p);

		void measure(OverlayRenderer *target, RECT &area, int widthFixed, int heightFixed, int skipChildren);
		void draw(OverlayRenderer *target);
		int hittest(int x, int y) const;
		void removeAllChildren();
		void addChild(OverlayRenderer::Control *c, CONTROL_CHILD_TYPE type);
		void addChild(OverlayRenderer::Control *c);
		OverlayRenderer::Control* getChild(int i, CONTROL_CHILD_TYPE type);
		OverlayRenderer::Control* getChild(int i);
		OverlayRenderer::Control* removeChild(int i, CONTROL_CHILD_TYPE type);
		OverlayRenderer::Control* removeChild(int i);
		int getChildCount(CONTROL_CHILD_TYPE type) const;
		int getChildCount() const;
		Control* getParent() const;
		void requestFront();
		std::recursive_mutex& getLock();
	};

protected:

	FFXIVDLL *dll;

	std::deque<uint64_t> mFpsMeter;

	std::map<std::wstring, PDXTEXTURETYPE> mResourceTextures;
	std::map<std::wstring, PDXTEXTURETYPE> mFileTextures;

	virtual PDXTEXTURETYPE GetTextureFromResource(TCHAR *resName) = 0;
	virtual PDXTEXTURETYPE GetTextureFromFile(TCHAR *resName) = 0;

	Control mWindows;
	ImGuiConfigWindow mConfig;

	virtual void DrawTexture(int x, int y, int w, int h, PDXTEXTURETYPE tex) = 0;
	virtual void DrawBox(int x, int y, int w, int h, DWORD Color) = 0;
	virtual void MeasureText(RECT &rc, TCHAR *text, int flags) = 0;
	virtual void DrawText(int x, int y, TCHAR *text, DWORD Color) = 0;
	virtual void DrawText(int x, int y, int width, int height, TCHAR *text, DWORD Color, int align) = 0;
	virtual void RenderOverlay() = 0;

	virtual void CheckCapture() = 0;

	std::recursive_mutex mCaptureMutex;
	bool mDoCapture = false;
	HANDLE hSaverThread;

public:
	OverlayRenderer(FFXIVDLL *dll);
	virtual ~OverlayRenderer();

	int GetFPS();

	void CaptureScreen();

	virtual void ReloadFromConfig() = 0;
	virtual void OnLostDevice() = 0;
	virtual void OnResetDevice() = 0;

	void DrawOverlay();

	void SetUseDrawOverlay(bool use);
	int GetUseDrawOverlay();
	void SetUseDrawOverlayEveryone(bool use);
	int GetUseDrawOverlayEveryone();
	void GetCaptureFormat(int format);
	int GetHideOtherUserName();
	void SetHideOtherUserName();

	void AddWindow(Control *windows);
	Control* GetRoot();
	WindowControllerBase* GetWindowAt(int x, int y);
	WindowControllerBase* GetWindowAt(Control *in, int x, int y);

	class Control;
};



#ifdef _WIN64
#include "OverlayRenderer_DX11.h"
#else
#include "OverlayRenderer_DX9.h"
#endif