#pragma once
#include "WindowControllerBase.h"
class DOTWindowController : public WindowControllerBase{
private:
	int mLastX;
	int mLastY;
	int mDragging;

public:
	DOTWindowController();
	~DOTWindowController();

	virtual int callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
};

