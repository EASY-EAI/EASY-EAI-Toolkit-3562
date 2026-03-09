#ifndef __COMBINER_H__
#define __COMBINER_H__
//=====================   C   =====================
#include "appSystem.h"
#include "sysIPCData.h"
#include "config.h"
//====================   C++   ====================
#include <string>
#include <list>
#include <map>
//=====================  SDK  =====================
//=====================  PRJ  =====================
#include "mediaClient.h"

#include "../algorithm/algorithmProcess.h"

class Analyzer : public MediaClient
{
public:
	Analyzer();
	~Analyzer();

	void init();
	int32_t IsInited(){return bObjIsInited;}

// ======== 告警管理器 =========================================
    void reInitAlarmMgr();
    void delAlarmRecord(AppEaiInferrerDelAlarmInfo_t delInfo);
    AlarmMgr *mpAlarmMgr;
// =============================================================
protected:

private:
	int bObjIsInited;
};

extern int analyzerInit(const char *moduleName);

#endif
