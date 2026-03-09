#ifndef __MEDIAPROCESSING_H__
#define __MEDIAPROCESSING_H__
//=====================   C   =====================
#include "appSystem.h"
#include "codecDefine.h"
#include "config.h"
#include "sysIPCData.h"
//====================   C++   ====================
#include <string>
#include <list>
#include <map>
//=====================  SDK  =====================
#include <rga/RgaApi.h>
//#include "geometry.h"
//=====================  PRJ  =====================
#include "mediaChannelBase.h"

#include "../algorithm/algorithmProcess.h"
#include "rgaLock.h"
#include "modelLock.h"

#if 0
typedef struct{
    rknn_context ctx;
    char name[32];
}ModelCtx_t;
#endif

class MediaChannel : public MediaChannelBase
{
public:
	MediaChannel();
	~MediaChannel();

	int32_t init();
    
    void setGlobalParameter(char *pParameter);
    void setAlgorithmConfig(char *pAlgoCfg);

    //std::vector<ModelCtx_t> mModelCtxList;
    std::vector<AlgoCfg_t> mAlgoCfgList;
    
    AlarmMgr *mpAlarmMgr;

    // ---算法配置锁---
    void algoCfgWLock(){pthread_rwlock_wrlock(&mAlgoCfglock);}
    void algoCfgRLock(){pthread_rwlock_rdlock(&mAlgoCfglock);}
    void algoCfgUnLock(){pthread_rwlock_unlock(&mAlgoCfglock);}
protected:
    // ---算法配置锁---
    void algoCfgLock_Init(){pthread_rwlock_init(&mAlgoCfglock, NULL);}
    void algoCfgLock_UnInit(){    pthread_rwlock_destroy(&mAlgoCfglock);}
    pthread_rwlock_t mAlgoCfglock;

private:
};

#endif
