#include "FFXIVDLL.h"
#include "MinHook.h"
#include "Tools.h"

FFXIVDLL::FFXIVDLL(HMODULE instance) :
	hInstance(instance)
{
	TCHAR fn[1024];
	GetModuleFileName(hInstance, fn, sizeof(fn));
	wsprintf(wcsrchr(fn, L'\\') + 1, L"FFXIVDLLInfo_%d.txt", GetCurrentProcessId());
	FILE *f = _wfopen(fn, L"r");
	if (f == nullptr)
		return;

	hUnloadEvent = CreateEvent(NULL, true, false, NULL);

	pPipe = new ExternalPipe(hUnloadEvent);
	pHooks = new Hooks(this, f);

	pDataProcess = new GameDataProcess(this, f, hUnloadEvent);

	fclose(f);
	DeleteFile(fn);

	pPipe->AddChat("/e Initialized");
	pHooks->Activate();
}


FFXIVDLL::~FFXIVDLL()
{

	pPipe->AddChat("/e Unloading");
	Sleep(100);

	SetEvent(hUnloadEvent);

	delete pDataProcess;
	delete pHooks;
	delete pPipe;

	CloseHandle(hUnloadEvent);
}
