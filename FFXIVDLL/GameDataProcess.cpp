#include "stdafx.h"
#include "GameDataProcess.h"
#include<deque>
#include<vector>
#include<map>
#include <psapi.h>

GameDataProcess::GameDataProcess(FILE *f, HANDLE unloadEvent) :
	hUnloadEvent(unloadEvent),
	mSent(1048576 * 8),
	mRecv(1048576 * 8),
	mLastIdleTime(0) {
	mLastAttack.dmg = 0;
	mLastAttack.timestamp = 0;
	fwscanf(f, L"%d", &ffxivhWnd);
	fwscanf(f, L"%d%d%d%d%d%d",
		&pActorMap, &pActor.id, &pActor.name, &pActor.owner, &pActor.type, &pActor.job);
	fwscanf(f, L"%d%d%d%d",
		&pTargetMap, &pTarget.current, &pTarget.hover, &pTarget.focus);

	mLocalTimestamp = mServerTimestamp = Tools::GetLocalTimestamp();

	hUpdateInfoThreadLock = CreateEvent(NULL, false, false, NULL);
	hUpdateInfoThread = CreateThread(NULL, NULL, GameDataProcess::UpdateInfoThreadExternal, this, NULL, NULL);

	TCHAR path[256];
	int i = 0, h = 0;
	GetModuleFileNameEx(GetCurrentProcess(), NULL, path, MAX_PATH);
	for (i = 0; i < 128 && path[i*2] && path[i*2+1]; i++)
		h ^= ((int*)path)[i];
	ExpandEnvironmentStrings(L"%APPDATA%", path, MAX_PATH);
	wsprintf(path + wcslen(path), L"\\ffxiv_overlay_cache_%d.txt", h);

	FILE *f2 = _wfopen(path, L"r");
	if (f2 != nullptr) {
		mContagionApplyDelayEstimation.load(f2);
		int cnt = 0;
		fscanf(f2, "%d", &cnt);
		cnt = max(0, min(256, cnt));
		while (cnt-- > 0) {
			int buff = 0;
			fscanf(f2, "%d", &buff);
			if (buff == 0)
				break;
			mDotApplyDelayEstimation[buff].load(f2);
		}
		fclose(f2);
	}

	mTableHeaderDef.icon = L"";
	mTableHeaderDef.count = 0;
	//# Name DPS Def Indef Crit Hits Max
	mTableHeaderDef.cols[mTableHeaderDef.count++] = L"#";
	mTableHeaderDef.cols[mTableHeaderDef.count++] = L"Name";
	mTableHeaderDef.cols[mTableHeaderDef.count++] = L"DPS";
	mTableHeaderDef.cols[mTableHeaderDef.count++] = L"Total";
	mTableHeaderDef.cols[mTableHeaderDef.count++] = L"Crit";
	mTableHeaderDef.cols[mTableHeaderDef.count++] = L"C/M/H";
	mTableHeaderDef.cols[mTableHeaderDef.count++] = L"Max";
	mTableHeaderDef.cols[mTableHeaderDef.count++] = L"Death";
	for (int i = 0; i < mTableHeaderDef.count; i++) mTableHeaderDef.align[i] = DT_CENTER;
	mTableHeaderDef.align[1] = DT_LEFT;
}

GameDataProcess::~GameDataProcess() {
	WaitForSingleObject(hUpdateInfoThread, -1);
	CloseHandle(hUpdateInfoThread);
	CloseHandle(hUpdateInfoThreadLock);

	TCHAR path[256];
	int i = 0, h = 0;
	GetModuleFileNameEx(GetCurrentProcess(), NULL, path, MAX_PATH);
	for (i = 0; i < 128 && path[i * 2] && path[i * 2 + 1]; i++)
		h ^= ((int*)path)[i];
	ExpandEnvironmentStrings(L"%APPDATA%", path, MAX_PATH);
	wsprintf(path + wcslen(path), L"\\ffxiv_overlay_cache_%d.txt", h);

	FILE *f = _wfopen(path, L"w");
	if (f != nullptr) {
		mContagionApplyDelayEstimation.save(f);
		int cnt = 0;
		fprintf(f, "\n%d", mDotApplyDelayEstimation.size());
		for(auto it = mDotApplyDelayEstimation.begin(); it != mDotApplyDelayEstimation.end(); ++it){
			fprintf(f, "\n%d ", it->first);
			it->second.save(f);
		}
		fclose(f);
	}
}

int GameDataProcess::getDoTPotency(int dot) {
	switch (dot) {
	case 0xf8: return 30;
	case 0xf4: return 20;
	case 0x77: return 30;
	case 0x76: return 35;
	case 0x6a: return 25;
	case 0xf6: return 50;
	case 0x7c: return 40;
	case 0x81: return 50;
	case 0x8f: return 25;
	case 0x90: return 50;
	case 0xa1: return 40;
	case 0xa2: return 40;
	case 0xa3: return 40;
	case 0xb3: return 40;
	case 0xb4: return 35;
	case 0xbd: return 35;
	case 0xbc: return 10;
	case 0x13a: return 20;
	case 0x12: return 50;
	case 0xec: return 20;
	case 0x1ec: return 30;
	case 0x1fc: return 40;
	case 0x356: return 44;
	case 0x2e5: return 40;
	case 0x346: return 40;
	case 0x34b: return 45;
	case 0x2d5: return 50;
	case 0x31e: return 40;
	}
	return 0;
}

int GameDataProcess::getDoTDuration(int skill) {
	switch (skill) {
	case 0xf8: return 15000;
	case 0xf4: return 30000;
	case 0x77: return 24000;
	case 0x76: return 30000;
	case 0x6a: return 30000;
	case 0xf6: return 21000;
	case 0x7c: return 18000;
	case 0x81: return 18000;
	case 0x8f: return 18000;
	case 0x90: return 12000;
	case 0xa1: return 18000;
	case 0xa2: return 21000;
	case 0xa3: return 24000;
	case 0xb3: return 18000;
	case 0xb4: return 24000;
	case 0xbd: return 30000;
	case 0xbc: return 15000;
	case 0x13a: return 15000;
	case 0x12: return 15000;
	case 0xec: return 18000;
	case 0x1ec: return 30000;
	case 0x356: return 30000;
	case 0x2e5: return 30000;
	case 0x346: return 18000;
	case 0x34b: return 30000;
	case 0x31e: return 24000;
	}
	return 0;
}

TCHAR* GameDataProcess::getDoTName(int skill) {
	switch (skill) {
	case 0xf8: return L"circle of scorn";
	case 0xf4: return L"fracture";
	case 0x77: return L"phlebotomize";
	case 0x76: return L"chaos thrust";
	case 0x6a: return L"touch of death";
	case 0xf6: return L"demolish";
	case 0x7c: return L"venomous bite";
	case 0x81: return L"windbite";
	case 0x8f: return L"aero";
	case 0x90: return L"aero ii";
	case 0xa1: return L"thunder";
	case 0xa2: return L"thunder";
	case 0xa3: return L"thunder";
	case 0xb3: return L"bio";
	case 0xb4: return L"miasma";
	case 0xbd: return L"bio ii";
	case 0xbc: return L"miasma ii";
	case 0x13a: return L"inferno";
	case 0x12: return L"poison";
	case 0xec: return L"choco beak";
	case 0x1ec: return L"mutilation";
	case 0x1fc: return L"shadow fang";
	case 0x356: return L"lead shot";
	case 0x2e5: return L"scourge";
	case 0x346: return L"combust";
	case 0x34b: return L"combust ii";
	case 0x2d5: return L"goring blade";
	case 0x31e: return L"aero iii";
	}
	return L"(unknown)";
}


inline int GameDataProcess::GetActorType(int id) {
	if (mActorPointers.find(id) == mActorPointers.end())
		return -1;
	char *c = (char*)(mActorPointers[id]);
	c += pActor.type;
	return *c;
}

inline int GameDataProcess::GetTargetId(int type) {
	char *c = (char*)((int)pTargetMap + type);
	if (c == 0)
		return NULL_ACTOR;
	int ptr = *(int*)c;
	if (ptr == 0)
		return NULL_ACTOR;
	return *(int*)(ptr + pActor.id);
}

inline char* GameDataProcess::GetActorName(int id) {
	if (id == SOURCE_LIMIT_BREAK)
		return "(Limit Break)";
	if (mActorPointers.find(id) == mActorPointers.end())
		return "(unknown)";
	char *c = (char*)(mActorPointers[id]);
	c += pActor.name;
	if (!Tools::TestValidString(c))
		return "(error)";
	if (strlen(c) == 0)
		return "(empty)";
	return c;
}

inline TCHAR* GameDataProcess::GetActorJobString(int id) {
	if (id == SOURCE_LIMIT_BREAK)
		return L"LB";
	if (mActorPointers.find(id) == mActorPointers.end())
		return L"(?)";
	char *c = (char*)(mActorPointers[id]);
	c += pActor.job;
	if (!Tools::TestValidString(c))
		return L"(??)";
	switch (*c) {
	case 1: return L"GLD";
	case 2: return L"PGL";
	case 3: return L"MRD";
	case 4: return L"LNC";
	case 5: return L"ARC";
	case 6: return L"CNJ";
	case 7: return L"THM";
	case 8: return L"CPT";
	case 9: return L"BSM";
	case 10: return L"ARM";
	case 11: return L"GSM";
	case 12: return L"LTW";
	case 13: return L"WVR";
	case 14: return L"ALC";
	case 15: return L"CUL";
	case 16: return L"MIN";
	case 17: return L"BTN";
	case 18: return L"FSH";
	case 19: return L"PLD";
	case 20: return L"MNK";
	case 21: return L"WAR";
	case 22: return L"DRG";
	case 23: return L"BRD";
	case 24: return L"WHM";
	case 25: return L"BLM";
	case 26: return L"ACN";
	case 27: return L"SMN";
	case 28: return L"SCH";
	case 29: return L"ROG";
	case 30: return L"NIN";
	case 31: return L"MCH";
	case 32: return L"DRK";
	case 33: return L"AST";
	}
	return L"(???)";
}

void GameDataProcess::ResolveUsers() {
	int limit = 1372;
	mSelfId = 0;
	mDamageRedir.clear();
	mActorPointers.clear();
	__try {
		for (int i = 0; i < limit; i++) {
			PVOID ptr = (*pActorMap)[i];
			if (ptr == 0) continue;
			int id = *(int*)(char*)((int)ptr + pActor.id);
			int owner = *(int*)(char*)((int)ptr + pActor.owner);
			if (mSelfId == 0)
				mSelfId = id;
			if (owner != NULL_ACTOR)
				mDamageRedir[id] = owner;
			else
				mDamageRedir[id] = id;
			mActorPointers[id] = ptr;
		}
	} __except (EXCEPTION_EXECUTE_HANDLER) {
	}
}

void GameDataProcess::ResetMeter() {
	mLastAttack.timestamp = 0;
}

void GameDataProcess::CalculateDps(uint64_t timestamp) {
	timestamp = mLastAttack.timestamp;

	if (mLastIdleTime != timestamp) {
		mCalculatedDamages.clear();
		for (auto it = mDpsInfo.begin(); it != mDpsInfo.end(); ++it) {
			mCalculatedDamages.push_back(std::pair<int, int>(it->first, it->second.totalDamage.def + it->second.totalDamage.ind));
		}
		std::sort(mCalculatedDamages.begin(), mCalculatedDamages.end(), [](const std::pair<int, int> &a, const std::pair<int, int> &b) -> bool {
			return a.second > b.second;
		});
	}
}

void GameDataProcess::AddDamageInfo(TEMPDMG dmg, bool direct) {
	if (GetActorType(dmg.source) == ACTOR_TYPE_PC) {

		if (mLastAttack.timestamp < dmg.timestamp - IDLETIME) {
			mDpsInfo.clear();
			mLastIdleTime = dmg.timestamp - 1000;
		}
		mLastAttack = dmg;

		if (direct) {
			mDpsInfo[dmg.source].totalDamage.def += dmg.dmg;
			if (mDpsInfo[dmg.source].maxDamage.dmg < dmg.dmg)
				mDpsInfo[dmg.source].maxDamage = dmg;
			if (dmg.isCrit)
				mDpsInfo[dmg.source].critHits++;
			if (dmg.dmg == 0)
				mDpsInfo[dmg.source].missHits++;
			mDpsInfo[dmg.source].totalHits++;
		} else {
			mDpsInfo[dmg.source].totalDamage.ind += dmg.dmg;
			mDpsInfo[dmg.source].dotHits++;
			dmg.isCrit = 0;
		}
	}
}


void GameDataProcess::UpdateOverlayMessage() {
	uint64_t timestamp = mServerTimestamp - mLocalTimestamp + Tools::GetLocalTimestamp();
	TCHAR res[32768];
	CalculateDps(timestamp);
	int pos = 0;
	TCHAR tmp[512];
	SYSTEMTIME s1, s2;
	Tools::MillisToLocalTime(timestamp, &s1);
	Tools::MillisToSystemTime(timestamp*EORZEA_CONSTANT, &s2);
	pos += swprintf(res + pos, L"FPS %d / LT %02d:%02d:%02d / ET %02d:%02d:%02d",
		ffxivDll->hooks()->GetOverlayRenderer()->GetFPS(),
		(int)s1.wHour, (int)s1.wMinute, (int)s1.wSecond,
		(int)s2.wHour, (int)s2.wMinute, (int)s2.wSecond);
	if (!mDpsInfo.empty()) {
		uint64_t dur = mLastAttack.timestamp - mLastIdleTime;
		pos += swprintf(res + pos, L" / %02d:%02d.%03d",
			(int)((dur / 60000) % 60), (int)((dur/1000) % 60), (int) (dur % 1000));
	}
	ffxivDll->hooks()->GetOverlayRenderer()->SetText(res);

	if (!mDpsInfo.empty()) {
		int i = 0;
		int disp = 0;
		bool dispme = false;
		int mypos = 0;
		if (mCalculatedDamages.size() > 8) {
			for (auto it = mCalculatedDamages.begin(); it != mCalculatedDamages.end(); ++it, mypos++)
				if (it->first == mSelfId)
					break;
			mypos = max(4, mypos);
			mypos = min(mCalculatedDamages.size() - 4, mypos);
		}
		std::vector<OVERLAY_RENDER_TABLE_ROW> table;
		table.push_back(mTableHeaderDef);
		float maxDps = (float)(mCalculatedDamages.begin()->second * 1000. / (mLastAttack.timestamp - mLastIdleTime));
		for (auto it = mCalculatedDamages.begin(); it != mCalculatedDamages.end(); ++it) {
			i++;
			if (!ffxivDll->hooks()->GetOverlayRenderer()->GetUseDrawOverlayEveryone() && it->first != mSelfId)
				continue;
			else if (mCalculatedDamages.size() > 8 && !(mypos - 4 <= i && i < mypos + 4))
				continue;
			OVERLAY_RENDER_TABLE_ROW row;
			TEMPDMG &max = mDpsInfo[it->first].maxDamage;
			float curDps = (float)(it->second * 1000. / (mLastAttack.timestamp - mLastIdleTime));

			row.icon = GetActorJobString(it->first);
			row.count = 0;
			swprintf(tmp, L"%d", i);
			row.cols[row.count++] = tmp;
			if (ffxivDll->hooks()->GetOverlayRenderer()->GetHideOtherUser() && it->first != mSelfId)
				wcscpy(tmp, L"...");
			else {
				char *name = GetActorName(it->first);
				MultiByteToWideChar(CP_UTF8, 0, name, -1, tmp, sizeof(tmp) / sizeof(TCHAR));
			}
			row.cols[row.count++] = tmp;
			swprintf(tmp, L"%.2f", curDps);
			row.cols[row.count++] = tmp;
			swprintf(tmp, L"%d", it->second);
			row.cols[row.count++] = tmp;
			swprintf(tmp, L"%.2f%%", 100.f * mDpsInfo[it->first].critHits / mDpsInfo[it->first].totalHits);
			row.cols[row.count++] = tmp;
			swprintf(tmp, L"%d/%d/%d", mDpsInfo[it->first].critHits, mDpsInfo[it->first].missHits, mDpsInfo[it->first].totalHits + mDpsInfo[it->first].dotHits);
			row.cols[row.count++] = tmp;
			swprintf(tmp, L"%d%s", max.dmg, max.isCrit ? L"!" : L"");
			row.cols[row.count++] = tmp;
			swprintf(tmp, L"%d", mDpsInfo[it->first].deaths);
			row.cols[row.count++] = tmp;

			row.barSize = curDps / maxDps;
			for (int i = 0; i < row.count; i++) row.align[i] = DT_CENTER;
			row.align[1] = DT_LEFT;
			table.push_back(row);
		}
		ffxivDll->hooks()->GetOverlayRenderer()->SetTable(table);
		// pos += swprintf(res + pos, L"\n");
	}
	std::vector<TEMPBUFF> buff_sort;
	for (auto it = mActiveDoT.begin(); it != mActiveDoT.end(); ) {
		if (it->expires < timestamp)
			it = mActiveDoT.erase(it);
		else {
			if (it->source == mSelfId) {
				buff_sort.push_back(*it);
			}
			++it;
		}
	}
	pos = swprintf(res, L"DoTs\n");
	int currentTarget = GetTargetId(pTarget.current);
	int focusTarget = GetTargetId(pTarget.focus);
	int hoverTarget = GetTargetId(pTarget.hover);
	std::sort(buff_sort.begin(), buff_sort.end(), [&](const TEMPBUFF & a, const TEMPBUFF & b) {
		if (a.target == focusTarget ^ b.target == focusTarget)
			return (a.target == focusTarget ? 1 : 0) > (b.target == focusTarget ? 1 : 0);
		if (a.target == currentTarget ^ b.target == currentTarget)
			return (a.target == currentTarget ? 1 : 0) > (b.target == currentTarget ? 1 : 0);
		if (a.target == hoverTarget ^ b.target == hoverTarget)
			return (a.target == hoverTarget ? 1 : 0) > (b.target == hoverTarget ? 1 : 0);
		return a.expires < b.expires;
	});
	int i = 0;
	for (auto it = buff_sort.begin(); it != buff_sort.end(); ++it, i++) {
		if (i > 9 && it->target != currentTarget && it->target != hoverTarget && it->target != focusTarget)
			break;
		MultiByteToWideChar(CP_UTF8, 0, GetActorName(it->target), -1, tmp, sizeof(tmp) / sizeof(TCHAR));
		pos += swprintf(res + pos, L"%s[%s] %s: %.1fs%s\n",
			currentTarget == it->target ? L"[T] " :
			hoverTarget == it->target ? L"[M] " :
			focusTarget == it->target ? L"[F] " : L"",
			tmp, getDoTName(it->buffid),
			(it->expires - timestamp) / 1000.f,
			it->simulated ? (
			(getDoTDuration(it->buffid) + (it->contagioned ? 15000 : 0)) < it->expires - timestamp
				? L"(x)" :
				L"(?)") : L"");
	}
	i = buff_sort.size() - i;
	if (i > 0)
		pos += swprintf(res + pos, L"...and %d more\n", i);

	while (pos > 0 && res[pos - 1] == '\n')
		res[--pos] = 0;
	ffxivDll->hooks()->GetOverlayRenderer()->SetDoTText(res);
}

void GameDataProcess::ProcessAttackInfo(int source, int target, int skill, ATTACK_INFO *info, uint64_t timestamp) {
	for (int i = 0; i < 4; i++) {
		if (info->attack[i].swingtype == 0) continue;

		TEMPDMG dmg = { 0 };
		dmg.timestamp = timestamp;
		if (mDamageRedir.find(source) != mDamageRedir.end())
			source = mDamageRedir[source];
		dmg.source = source;
		switch (skill) {
		case 0xc8:
		case 0xc9:
		case 0xca:
		case 0xcb:
		case 0xcc:
		case 0xcd:
		case 0x1092:
		case 0x1093:
		case 0x108f:
		case 0x108e:
		case 0x1095:
		case 0x1094:
		case 0x1096:
			source = SOURCE_LIMIT_BREAK;
			break;
		}
		switch(info->attack[i].swingtype){
		case 1:
		case 10:
			if (info->attack[i].damage == 0) {
				dmg.dmg = 0;
				AddDamageInfo(dmg, true);
			}
			break;
		case 5:
			dmg.isCrit = true;
		case 3:
			dmg.dmg = info->attack[i].damage;
			AddDamageInfo(dmg, true);
			break;
		case 17:
		case 18: {
			int buffId = info->attack[i].damage;

			if (getDoTPotency(buffId) == 0)
				continue; // not an attack buff
			auto it = mActiveDoT.begin();
			bool add = true;
			while (it != mActiveDoT.end()) {
				if (it->source == source && it->target == target && it->buffid == buffId) {
					it->expires = timestamp + getDoTDuration(buffId) + mDotApplyDelayEstimation[it->buffid].get();
					it->simulated = 1;
					it->contagioned = 0;
					add = false;
					break;
				} else if (it->expires < timestamp) {
					it = mActiveDoT.erase(it);
				} else
					++it;
			}
			if (add) {
				TEMPBUFF b;
				b.buffid = buffId;
				b.source = source;
				b.target = target;
				b.expires = timestamp + getDoTDuration(buffId);
				b.potency = getDoTPotency(b.buffid);
				b.simulated = 1;
				b.contagioned = 0;
				mActiveDoT.push_back(b);
			}

			break;
		}
		case 41: { // Probably contagion
			if (skill == 795) { // it is contagion
				auto it = mActiveDoT.begin();
				while (it != mActiveDoT.end()) {
					if (it->expires - mContagionApplyDelayEstimation.get() < timestamp) {
						it = mActiveDoT.erase(it);
					} else if (it->source == source && it->target == target) {
						it->expires += 15000;
						it->simulated = 1;
						it->contagioned = 1;
						++it;
					} else 
						++it;
				}
			}
			break;
		}
		}
		/*
		char tss[512];
		sprintf(tss, "/e    => %d/%d/%d/%d/%d/%d",
			(int)info->attack[i].swingtype,
			(int)info->attack[i].damagetype,
			(int)info->attack[i].elementtype,
			(int)info->attack[i].data0_rr,
			(int)info->attack[i].damage,
			(int)info->attack[i].data1_right
			);
		ffxivDll->pipe()->AddChat(tss);
		//*/
	}
}

void GameDataProcess::ProcessGameMessage(void *data, uint64_t timestamp, int len, bool setTimestamp) {

	if (setTimestamp) {
		mServerTimestamp = timestamp;
		mLocalTimestamp = Tools::GetLocalTimestamp();
	} else
		timestamp = mServerTimestamp;

	GAME_MESSAGE *msg = (GAME_MESSAGE*)data;
	// std::string ts; char tss[512];
	switch (msg->message_cat1) {
	case GAME_MESSAGE::C1_Combat: {
		switch (msg->message_cat2) {
		case 0x0143:
		case GAME_MESSAGE::C2_ActorInfo:
			break;
		case GAME_MESSAGE::C2_Info1:
			if (msg->Combat.Info1.c1 == 23 && msg->Combat.Info1.c2 == 3) {
				if (msg->Combat.Info1.c5 == 0) {
					int total = 0;
					std::map<int, int> portions;
					for (auto it = mActiveDoT.begin(); it != mActiveDoT.end(); ++it) {
						if (it->target == msg->actor) {
							portions[it->source] += it->potency;
						}
						total += it->potency;
					}
					if (total > 0) {
						for (auto const &ent1 : portions) {
							int mine = msg->Combat.Info1.c3 * ent1.second / total;
							if (mine > 0) {
								TEMPDMG dmg;
								dmg.timestamp = timestamp;
								dmg.source = ent1.first;
								dmg.dmg = mine;
								if (mDamageRedir.find(dmg.source) != mDamageRedir.end())
									dmg.source = mDamageRedir[dmg.source];
								AddDamageInfo(dmg, false);
							}
						}
					}
				} else {
					for (auto it = mActiveDoT.begin(); it != mActiveDoT.end(); ++it) {
						if (it->source == it->target && it->buffid == msg->Combat.Info1.c5) {
							int mine = msg->Combat.Info1.c3;
							if (mine > 0) {
								TEMPDMG dmg;
								dmg.timestamp = timestamp;
								dmg.source = it->source;
								dmg.dmg = mine;
								if (mDamageRedir.find(dmg.source) != mDamageRedir.end())
									dmg.source = mDamageRedir[dmg.source];
								AddDamageInfo(dmg, false);
							}
						}
					}
				}
			} else if (msg->Combat.Info1.c1 == 21) {
				auto it = mActiveDoT.begin();
				while (it != mActiveDoT.end()) {
					if (it->expires < timestamp)
						it = mActiveDoT.erase(it);
					else
						++it;
				}
			} else if (msg->Combat.Info1.c1 == 6) {
				// death
				int who = msg->Combat.Info1.c5;
				if(GetActorType(msg->actor) == ACTOR_TYPE_PC)
					mDpsInfo[msg->actor].deaths++;
				/*
				sprintf(tss, "cmsgDeath %s killed %s", getActorName(who), getActorName(msg->actor));
				ffxivDll->pipe()->sendInfo(tss);
				//*/
				auto it = mActiveDoT.begin();
				while (it != mActiveDoT.end()) {
					if (it->expires < timestamp || it->target == msg->actor)
						it = mActiveDoT.erase(it);
					else
						++it;
				}
			}
			break;
		case GAME_MESSAGE::C2_AddBuff:
			/*
			sprintf(tss, "cmsgAddBuff %s / %d", getActorName(msg->actor), 
				msg->Combat.AddBuff._u1);
			ffxivDll->pipe()->sendInfo(tss);
			//*/
			for (int i = 0; i < msg->Combat.AddBuff.buff_count && i < 5; i++) {
				if (getDoTPotency(msg->Combat.AddBuff.buffs[i].buffID) == 0)
					continue; // not an attack buff
				auto it = mActiveDoT.begin();
				bool add = true;
				while (it != mActiveDoT.end()) {
					if (it->source == msg->Combat.AddBuff.buffs[i].actorID && it->target == msg->actor && it->buffid == msg->Combat.AddBuff.buffs[i].buffID) {
						uint64_t nexpire = timestamp + (int)(msg->Combat.AddBuff.buffs[i].duration * 1000);
						if (it->simulated) {
							if (it->contagioned) {
								mContagionApplyDelayEstimation.add((int) (nexpire - it->expires));
								//sprintf(tss, "cmsg => simulated (Contagion: %d / %d)", nexpire - it->expires, estimatedContagionDelay.get());
							} else {
								mDotApplyDelayEstimation[it->buffid].add((int) (nexpire - it->expires));
								//sprintf(tss, "cmsg => simulated (%d: %d / %d)", it->buffid, nexpire - it->expires, estimatedDelays[it->buffid].get());
							}
							//ffxivDll->pipe()->sendInfo(tss);
						}
						it->expires = nexpire;
						it->simulated = 0;
						it->contagioned = 0;
						add = false;
						break;
					} else if (it->expires < timestamp) {
						it = mActiveDoT.erase(it);
					} else
						++it;
				}
				if (add) {
					TEMPBUFF b;
					b.buffid = msg->Combat.AddBuff.buffs[i].buffID;
					b.source = msg->Combat.AddBuff.buffs[i].actorID;
					b.target = msg->actor;
					b.expires = timestamp + (int)(msg->Combat.AddBuff.buffs[i].duration * 1000);
					b.potency = getDoTPotency(b.buffid);
					b.contagioned = 0;
					b.simulated = 0;
					mActiveDoT.push_back(b);
				}
			}
			break;
		case GAME_MESSAGE::C2_UseAbility:
			// sprintf(tss, "/e Ability %s / %d / %d", GetActorName(msg->actor), msg->Combat.UseAbility.target, msg->Combat.UseAbility.skill);
			// ffxivDll->pipe()->AddChat(tss);
			
			ProcessAttackInfo(msg->actor, msg->Combat.UseAbility.target, msg->Combat.UseAbility.skill, &msg->Combat.UseAbility.attack, timestamp);
			break;
		case GAME_MESSAGE::C2_UseAoEAbility:
			// sprintf(tss, "/e AoE %s / %d", GetActorName(msg->actor), msg->Combat.UseAoEAbility.skill);
			// ffxivDll->pipe()->AddChat(tss);

			if (GetActorName(msg->actor), msg->Combat.UseAoEAbility.skill == 174) { // Bane
				int baseActor = NULL_ACTOR;
				for (int i = 0; i < 16; i++)
					if (msg->Combat.UseAoEAbility.targets[i].target != 0)
						for (int j = 0; j < 4; j++)
							if (msg->Combat.UseAoEAbility.attackToTarget[i].attack[j].swingtype == 11)
								baseActor = msg->Combat.UseAoEAbility.targets[i].target;
				if (baseActor != NULL_ACTOR) {
					for (int i = 0; i < 16; i++)
						if (msg->Combat.UseAoEAbility.targets[i].target != 0)
							for (int j = 0; j < 4; j++)
								if (msg->Combat.UseAoEAbility.attackToTarget[i].attack[j].swingtype == 17) {
									TEMPBUFF buff;
									uint64_t expires = -1;
									for (auto it = mActiveDoT.begin(); it != mActiveDoT.end(); ++it)
										if (it->buffid == msg->Combat.UseAoEAbility.attackToTarget[i].attack[j].damage &&
											it->source == msg->actor &&
											it->target == baseActor) {
											expires = it->expires;
											break;
										}
									if (expires != -1) {
										bool addNew = true;
										for (auto it = mActiveDoT.begin(); it != mActiveDoT.end(); ++it)
											if (it->buffid == msg->Combat.UseAoEAbility.attackToTarget[i].attack[j].damage &&
												it->source == msg->actor &&
												it->target == msg->Combat.UseAoEAbility.targets[i].target) {
												it->expires = expires;
												it->simulated = 1;
												addNew = false;
												break;
											}
										if (addNew) {
											buff.buffid = msg->Combat.UseAoEAbility.attackToTarget[i].attack[j].damage;
											buff.expires = expires;
											buff.source = msg->actor;
											buff.target = msg->Combat.UseAoEAbility.targets[i].target;
											buff.potency = getDoTPotency(buff.buffid);
											buff.simulated = 1;

											mActiveDoT.push_back(buff);
										}
									}
								}
				}

			} else {
				for (int i = 0; i < 16; i++)
					if (msg->Combat.UseAoEAbility.targets[i].target != 0) {
						/*
						sprintf(tss, "cmsg  => Target[%i] %d / %s", i, msg->Combat.UseAoEAbility.targets[i].target, getActorName(msg->Combat.UseAoEAbility.targets[i].target));
						ffxivDll->pipe()->sendInfo(tss);
						//*/
						ProcessAttackInfo(msg->actor, msg->Combat.UseAoEAbility.targets[i].target, msg->Combat.UseAoEAbility.skill, &msg->Combat.UseAoEAbility.attackToTarget[i], timestamp);
					}
			}
			break;
		default:
			goto DEFPRT;
		}
		break;
	}
	DEFPRT:
	default:
		/*
		int pos = sprintf(tss, "cmsg%04X:%04X (%s, %x) ", (int)msg->message_cat1, (int)msg->message_cat2, getActorName(msg->actor), msg->length);
		for (int i = 0; i < msg->length - 32 && i<64; i ++)
			pos += sprintf(tss + pos, "%02X ", (int)((unsigned char*)msg->data)[i]);
		ffxivDll->pipe()->sendInfo(tss);
		//*/
		break;
	}
}

void GameDataProcess::PacketErrorMessage(int signature, int length) {
	char t[1024];
	sprintf(t, "cmsgError: packet error %08X / %08X", signature, length);
	ffxivDll->pipe()->SendInfo(t);
}

void GameDataProcess::ParsePacket(Tools::ByteQueue &p, bool setTimestamp) {

	struct {
		union {
			uint32_t signature;		// 0 ~ 3		0x41A05252
			uint64_t signature_2[2];
		};
		uint64_t timestamp;		// 16 ~ 23
		uint32_t length;		// 24 ~ 27
		uint16_t _u2;			// 28 ~ 29
		uint16_t message_count;	// 30 ~ 31
		uint8_t _u3;			// 32
		uint8_t is_gzip;		// 33
		uint16_t _u4;			// 34 ~ 35
		uint32_t _u5;			// 36 ~ 39;
		uint8_t data[65536];
	} packet;

	while (p.getUsed() >= 28) {
		p.peek(&packet, 28);
		if ((packet.signature != 0x41A05252 && (packet.signature_2[0] | packet.signature_2[1])) || packet.length > sizeof(packet) || packet.length <= 40) {
			p.waste(1);
			PacketErrorMessage(packet.signature, packet.length);
			continue;
		}
		if (p.getUsed() < packet.length)
			break;
		p.peek(&packet, packet.length);
		unsigned char *buf = nullptr;
		if (packet.is_gzip) {
			z_stream inflater;
			memset(&inflater, 0, sizeof(inflater));
			inflater.avail_out = DEFLATE_CHUNK_SIZE;
			inflater.next_out = inflateBuffer;
			inflater.next_in = packet.data;
			inflater.avail_in = packet.length - 40;

			if (inflateInit(&inflater) == Z_OK) {
				int res = inflate(&inflater, Z_NO_FLUSH);
				if (Z_STREAM_END != res) {
					inflateEnd(&inflater);
					p.waste(1);
					ffxivDll->pipe()->SendInfo("zlib error");
					continue;
				}
				inflateEnd(&inflater);
				if (inflater.avail_out == 0) {
					p.waste(1);
					ffxivDll->pipe()->SendInfo("zlib error");
					continue;
				}
				buf = inflateBuffer;
			}
		} else {
			buf = packet.data;
		}
		if (packet.signature == 0x41A05252) {
			if (buf != nullptr) {
				for (int i = 0; i < packet.message_count; i++) {
					ProcessGameMessage(buf, packet.timestamp, ((char*)&packet.data - (char*)&packet), setTimestamp);
					buf += *((uint32_t*)buf);
				}
			}
		}
		p.waste(packet.length);
	}
}

void GameDataProcess::UpdateInfoThread() {
	while (WaitForSingleObject(hUnloadEvent, 0) == WAIT_TIMEOUT) {
		WaitForSingleObject(hUpdateInfoThreadLock, 50);
		ResolveUsers();
		ParsePacket(mSent, false);
		ParsePacket(mRecv, true);
		__try {
			UpdateOverlayMessage();
		} __except (EXCEPTION_EXECUTE_HANDLER) {
		}
	}
	TerminateThread(GetCurrentThread(), 0);
}