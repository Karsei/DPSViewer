#pragma once
#include <string>
#include <vector>
#include <Windows.h>

class MemoryScanner {
public:
	struct {
		struct {
			PVOID ProcessWindowMessage;
			PVOID ProcessNewLine;
		} Functions;
		struct {
			PVOID ActorMap;
			PVOID TargetMap;
			PVOID PartyMap;
		} Data;
	}Result;
private:

	class DataSignature {
	public: 
		DataSignature(std::string signature, int offset, bool rip, int pathcount, int *path);

		std::string signature, mask;
		int offset;
		bool rip;
		std::vector<int> path;
	};

	std::vector<DataSignature> ActorMap, TargetMap, PartyMap;

	HMODULE hGame;
	IMAGE_NT_HEADERS *peHeader;
	IMAGE_SECTION_HEADER *sectionHeaders;

	void ScanInto(PVOID *p, std::vector<DataSignature> &map);
	DWORD DoScan();
	static DWORD WINAPI DoScanExternal(PVOID p) { return ((MemoryScanner*) p)->DoScan(); }

	HANDLE mScannerThread;

public:
	MemoryScanner();
	~MemoryScanner();

	bool IsScanComplete();
};

