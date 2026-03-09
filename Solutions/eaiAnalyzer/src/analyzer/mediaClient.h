#ifndef __MEDIACLIENT_H__
#define __MEDIACLIENT_H__
//=====================   C   =====================
#include "appSystem.h"
#include "sysIPCData.h"
#include "config.h"
//====================   C++   ====================
#include <string>
#include <vector>
#include <list>
#include <map>
//=====================  SDK  =====================
#include "ipc.h"
//=====================  PRJ  =====================
#include "processMgr.h"
#include "mediaChannel.h"

class MediaClient
{
public:
	MediaClient();
	~MediaClient();

	void IPC_init(int32_t cliId, int32_t serId, IPC_Client_CB func);

    void refreshImgInfo();
    void refreshMediaChannel(char *pData);

    // 进程间通信发起动作:
    //int32_t registerMySelfToSignalManager();
    int32_t requestUpdateChannelInfo();

    char *mpGlobalData;
    int32_t mGlobalDataLen;

protected:
    std::vector<AppEaiInferrerChnCfg_t> unpackChannelInfo(char *pData);
    void updateChnInfo(std::vector<AppEaiInferrerChnCfg_t> added, 
                            std::vector<AppEaiInferrerChnCfg_t> removed,
                            std::vector<AppEaiInferrerChnCfg_t> modified);
    
    MediaChannel* searchMediaChannel(int chnId);
    MediaChannel* createMediaChannel(int chnId);
    void destroyMediaChannel(int chnId);

    // 信号管理器给过来的参数，要与此同步
    std::map<int32_t, int32_t> mSigId_MemIdMap; //<signalId shareMemId>
    std::map<int32_t, ImgSignal_t> mSigId_ImgInfoMap; //<signalId imgInfo>
    
    // 通道管理器给过来的参数，要与此同步
    std::vector<AppEaiInferrerChnCfg_t> mChnInfoArray;
    // 合成器管理的多媒体通道，内含线程，共享内存映射等资源
    std::list<MediaChannel*> m_MediaChnlist;

private:
    
};


#endif
