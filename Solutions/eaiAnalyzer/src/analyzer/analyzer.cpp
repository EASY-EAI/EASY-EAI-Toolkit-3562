//=====================  C++  =====================
#include <unordered_map>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <queue>
//=====================  SDK  =====================
#include "system_opt.h"
#include "ini_wrapper.h"
#include "log_manager.h"
#include "ipc.h"
#include "cJSON.h"
//=====================  PRJ  =====================
#include "analyzer.h"

int mediaClientHandle(void *pObj, IPC_MSG_t *pMsg)
{
    Analyzer *pAnalyzer = (Analyzer *)pObj;
    
    if(MSGTYPE_APP_EAIBOX_channelCfg == pMsg->msgType){
        // refreshImgInfo能从这里调用，说明信号管理器已经完全准备好了，仅用抓取有效信号就行
        pAnalyzer->refreshImgInfo();
        
        char *pChnData = (char *)malloc(pMsg->msgLen);
        if(pChnData){
            memcpy(pChnData, pMsg->payload, pMsg->msgLen);
            pAnalyzer->refreshMediaChannel(pChnData);
            free(pChnData);
        }
    
    }
    
    return 0;
}

Analyzer::Analyzer() :
    mpAlarmMgr(NULL),
	bObjIsInited(0)
{



#if 1
    // ====================== 5.初始化告警管理器 ======================
    if(!mpAlarmMgr){
        mpAlarmMgr = new AlarmMgr;
        mpGlobalData = (char *)mpAlarmMgr;
    }
#endif
}

Analyzer::~Analyzer()
{
    // 1，销毁告警管理器
    if(mpAlarmMgr){
        delete mpAlarmMgr;
        mpAlarmMgr = NULL;
        mpGlobalData = NULL;
    }
}

void Analyzer::init()
{
    // ==================== 1.初始化进程间通信资源 ====================
    IPC_init(ANALYZER_CLI_ID, APPCHANNELMGR_CLI_ID, mediaClientHandle);
    
	bObjIsInited = 1;
}

void Analyzer::reInitAlarmMgr()
{
    if(mpAlarmMgr){
        mpAlarmMgr->deInit();
        mpAlarmMgr->init();
    }
}

void Analyzer::delAlarmRecord(AppEaiInferrerDelAlarmInfo_t delInfo)
{
    if(mpAlarmMgr){
        mpAlarmMgr->delAlarmRecord(delInfo.chnId, delInfo.alarmType, delInfo.startTime, delInfo.endTime);
    }
}

int analyzerInit(const char *moduleName)
{
    // 0-初始化日志管理系统
    log_manager_init(LOGCONFIG_PATH, moduleName);
    
    // ==================== 1.初始化算法上下文对象 ====================
    initModelHandle(ALGODEF_CONFIG_PATH);
    
    Analyzer *pAnalyzer = new Analyzer();
    if(NULL == pAnalyzer){
        PRINT_ERROR("pChannelMgr Create faild !!!");
        return -1;
    }
    
    pAnalyzer->init();

    while(1)
    {
        sleep(5);
    }
    
    if(pAnalyzer){
        delete pAnalyzer;
        pAnalyzer = NULL;
    }
    
    unInitModelHandle();
    
    return 0;
}


