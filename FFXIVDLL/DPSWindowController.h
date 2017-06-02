#pragma once
#include "WindowControllerBase.h"
class DPSWindowController : public WindowControllerBase{
private:
	int mLastX;
	int mLastY;
	int mDragging;

public:
	DPSWindowController(FILE *f);
	~DPSWindowController();

	bool hasCallback() const override {
		return true;
	}
	virtual int callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
};

