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
#include "cJSON.h"
//=====================  PRJ  =====================
#include "mediaClient.h"


static uint8_t *mapShm(int memId, int dataLen)
{	
	int shmid = -1;
	shmid = shmget(memId, dataLen, IPC_CREAT|0666);
	if(shmid == -1){
		PRINT_ERROR("shmget faild !\n");
		return NULL;
	}
    
    // 映射共享内存到指定地址
    // 返回值：
    //     失败：-1。
    //     成功：与指定的地址一致。
    void *pShareMem = shmat(shmid, 0, 0);
    if((void *)-1 == pShareMem){
        PRINT_ERROR("map share memory faild ![%s]", strerror(errno));
        return NULL;
    }

    return (uint8_t*)pShareMem;
}

static void unmapShm(uint8_t *pShmAddr)
{
    if(pShmAddr){// 脱离内存映射
        shmdt(pShmAddr);
    }
    return ;
}


MediaClient::MediaClient() :
    mpGlobalData(NULL),
    mGlobalDataLen(0)
{
    mSigId_MemIdMap.clear();
    mSigId_ImgInfoMap.clear();
    
    std::vector<AppEaiInferrerChnCfg_t>().swap(mChnInfoArray);
    m_MediaChnlist.clear();
}

MediaClient::~MediaClient()
{
    mSigId_MemIdMap.clear();
    mSigId_ImgInfoMap.clear();
}

void MediaClient::IPC_init(int32_t cliId, int32_t serId, IPC_Client_CB func)
{    
    // ==================== 1.初始化进程间通信资源 ====================
    IPC_client_create();
    IPC_client_init(cliId);
    IPC_client_set_callback(this, func);

    // =========== 2.1.等待 APPCHANNELMGR_CLI_ID 注册完成 =============
    waittingTargetClientReady(serId);
    requestUpdateChannelInfo();

    return ;
}

void MediaClient::refreshImgInfo()
{
    // 再建立新的内存映射
    std::string imgCfgFile;
    // /tmp/apps/signalMgr/imgData.ini
    imgCfgFile.clear();
    imgCfgFile.append(APP_PATH);
    imgCfgFile.append("/");
    imgCfgFile.append(IMAGE_CONFIG_FILE);

    ImgSignal_t imgInfo;
    char imgSession[16];
    // == mark == 这里先假定最多有32个通道，以后再想想怎么处理比较好
    for(int id = 0; id < 32; id++){
        memset(imgSession, 0, sizeof(imgSession));
        sprintf(imgSession, "img_%d", id);
        
        // 有效的配置信息
        if(0 == ini_section_exist(imgCfgFile.c_str(), imgSession)){
            memset(&imgInfo, 0, sizeof(imgInfo));
            int memId_A = ini_read_int(imgCfgFile.c_str(), imgSession, "memId_A");
            int memId_B = ini_read_int(imgCfgFile.c_str(), imgSession, "memId_B");

            int memId = 0;
            auto it = mSigId_MemIdMap.find(id);
            // 是个已经存在的img_id，下方判断是否需要脱离
            if(it != mSigId_MemIdMap.end()){
                memId = it->second;
                // 之前记录的memId与文件内的A/B id都不相等，说明memId是个被信号管理器标记释放的内存，需要脱离
                if( (memId != memId_A)&&(memId != memId_B) ){
                    // 关停多媒体通道的处理
                    MediaChannel *pMediaChannel = searchMediaChannel(id);
                    if(pMediaChannel){
                        ImgSignal_t tempImgInfo = {0};
                        pMediaChannel->setImageInfo(tempImgInfo);
                    }
                    // 脱离不使用的内存
                    auto iter = mSigId_ImgInfoMap.find(id);
                    if(iter != mSigId_ImgInfoMap.end()){
                        ImgSignal_t tempImgInfo = iter->second;
                        if(tempImgInfo.data){
                            unmapShm(tempImgInfo.data);
                        }
                        mSigId_ImgInfoMap.erase(iter);
                    }
                    mSigId_MemIdMap.erase(it);
                
                // 之前记录的memId与文件内的非0的那一个Id相等，说明内存没有变化
                }else{
                    // 因此：
                    //    1.无须内存脱离
                    //    2.mSigId_MemIdMap无须变更
                    //    3.mSigId_ImgInfoMap无须变更
                    continue;
                }

            }
            
            // 无效的共享内存信息，忽略此配置
            if((0 != memId_A) && (0 != memId_B))
                continue;
            // 此通道已经被关闭，无须根据配置创建新的映射
            if((0 == memId_A) && (0 == memId_B))
                continue;

            // 取非0的那个memId重新创建映射。
            if(memId_A)
                memId = memId_A;
            else
                memId = memId_B;

            PRINT_DEBUG("memId_A(%d),memId_B(%d)----memId(%d)", memId_A, memId_B, memId);
            if(memId) { // 有效的内存信息
                imgInfo.dataLen = ini_read_int(imgCfgFile.c_str(), imgSession, "imgLen");
                imgInfo.data = mapShm(memId, imgInfo.dataLen);
                if(imgInfo.data){
                    mSigId_MemIdMap[id] = memId;
                    
                    imgInfo.width  = ini_read_int(imgCfgFile.c_str(), imgSession, "width");
                    imgInfo.height = ini_read_int(imgCfgFile.c_str(), imgSession, "height");
                    ini_read_string2(imgCfgFile.c_str(), imgSession, "name", imgInfo.name, sizeof(imgInfo.name));
                    ini_read_string2(imgCfgFile.c_str(), imgSession, "imgFmt", imgInfo.dataFmt, sizeof(imgInfo.dataFmt));
                    mSigId_ImgInfoMap[id] = imgInfo;
                }
            }
            
        }
    }
}

void MediaClient::refreshMediaChannel(char *pData)
{
    PRINT_DEBUG("---- entry >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
    // 解包从通道管理器给过来的通道配置信息
    auto newChnInfoArry = unpackChannelInfo(pData);

    // 1. 把上一波通道配置数组(mChnInfoArray)映射到【旧】的哈希表(oldMap)
    std::unordered_map<int32_t, const AppEaiInferrerChnCfg_t*> oldMap;
    for (const auto& channel : mChnInfoArray) {
        oldMap[channel.chnId] = &channel;
    }

    // 2. 从通道管理器解包出配置信息数组(newChnCfgArry)，并马上映射到【新】的哈希表(newMap)
    std::unordered_map<int32_t, const AppEaiInferrerChnCfg_t*> newMap;
    for (const auto& channel : newChnInfoArry) {
        newMap[channel.chnId] = &channel;
    }
    
    std::vector<AppEaiInferrerChnCfg_t> addedChannels;
    for (const auto& channel : newChnInfoArry) {
        // 3. 从【新通道信息】找不存在于【旧哈希表】的元素，加进addedChannels中。
        if(oldMap.find(channel.chnId) == oldMap.end()) {
            addedChannels.push_back(channel);
        }
    }
    std::vector<AppEaiInferrerChnCfg_t> removedChannels;
    std::vector<AppEaiInferrerChnCfg_t> modifiedChannels;
    for (const auto& channel : mChnInfoArray) {
        auto it = newMap.find(channel.chnId);
        // 4. 从【旧通道信息】找不存在于【新哈希表】的元素，加进removedChannels中。
        if(it == newMap.end()) {
            removedChannels.push_back(channel);
            
        // 5. 从【旧通道信息】找到存在于【新哈希表】的元素，加进modifyChannels中。
        }else{
            // 比较新旧信号属性判断是否需要修改
            const AppEaiInferrerChnCfg_t* newChannelPtr = it->second;
            if (!newChannelPtr) continue;
            
            const AppEaiInferrerChnCfg_t& newChannel = *newChannelPtr;
            if((0 != strcmp(channel.algoCfg, newChannel.algoCfg)) ||  //通道配置信息变更
                (channel.sigId  != newChannel.sigId)) { //信号源变更
                modifiedChannels.push_back(newChannel);
            }
        }
    }

    updateChnInfo(addedChannels, removedChannels, modifiedChannels);
    
    // 更新通道信息数组
    mChnInfoArray = std::move(newChnInfoArry);
    PRINT_DEBUG("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< leave ----\n");
    return ;

}

#if 0
int32_t Combiner::registerMySelfToSignalManager()
{
    //向【信号管理器】发送【信号列表更新】请求
    int32_t cmdData = 0; //仅发送请求，不带数据。
    return IPC_client_sendData(APPSIGNALMGR_CLI_ID, MSGTYPE_APP_EAIBOX_registerClient, &cmdData, sizeof(cmdData));
}
#endif

int32_t MediaClient::requestUpdateChannelInfo()
{
    //向【信号管理器】发送【信号列表更新】请求
    int32_t cmdData = 0; //仅发送请求，不带数据。
    return IPC_client_sendData(APPCHANNELMGR_CLI_ID, MSGTYPE_APP_EAIBOX_reqChannelCfg, &cmdData, sizeof(cmdData));
}

std::vector<AppEaiInferrerChnCfg_t> MediaClient::unpackChannelInfo(char *pData)
{
    std::vector<AppEaiInferrerChnCfg_t> channelInfoArry;
    std::vector<AppEaiInferrerChnCfg_t>().swap(channelInfoArry); //清空
    
	PRINT_DEBUG("Recv a new group channel infomation: ");
    int32_t chnNum = 0;
    if(pData){
        memcpy(&chnNum, pData, sizeof(chnNum));

        AppEaiInferrerChnCfg_t chnInfo;
        AppEaiInferrerChnCfg_t *pChnInfo = (AppEaiInferrerChnCfg_t *)(pData + sizeof(chnNum));
        for(int32_t i = 0; i < chnNum; i++){
	        PRINT_DEBUG("Channel[%d] will need to bind signal[%d] ...", pChnInfo->chnId, pChnInfo->sigId);
            memcpy(&chnInfo, pChnInfo, sizeof(chnInfo));
            channelInfoArry.push_back(chnInfo);
            
            pChnInfo++;
        }
    }
	PRINT_DEBUG("Recv Over ===================================================== ");
    
    return channelInfoArry;
}

void MediaClient::updateChnInfo(std::vector<AppEaiInferrerChnCfg_t> added, 
                                     std::vector<AppEaiInferrerChnCfg_t> removed,
                                     std::vector<AppEaiInferrerChnCfg_t> modified)
{
    if(!removed.empty()){
        /*销毁已被删除的多媒体通道*/
        for (std::vector<AppEaiInferrerChnCfg_t>::iterator it = removed.begin(); it != removed.end(); ++it) {
            AppEaiInferrerChnCfg_t channelinfo = *it;
            
            destroyMediaChannel(channelinfo.chnId);
        }
    }

    if(!added.empty()){
        /*创建新增的多媒体通道*/
        for (std::vector<AppEaiInferrerChnCfg_t>::iterator it = added.begin(); it != added.end(); ++it) {
            AppEaiInferrerChnCfg_t channelinfo = *it;
            
            MediaChannel* pMediaChannel = createMediaChannel(channelinfo.chnId);
            if(pMediaChannel){
                pMediaChannel->setSignalId(channelinfo.sigId);
                pMediaChannel->setGlobalParameter(mpGlobalData);
                pMediaChannel->setAlgorithmConfig(channelinfo.algoCfg);
            
                auto it = mSigId_ImgInfoMap.find(channelinfo.sigId);
                if(it != mSigId_ImgInfoMap.end()){
                    ImgSignal_t imgInfo = it->second;
                    pMediaChannel->setImageInfo(imgInfo);
                }
            }
        }
    }

    if(!modified.empty()){
        /*给还在工作的多媒体通道修改信号源*/
        for (std::vector<AppEaiInferrerChnCfg_t>::iterator it = modified.begin(); it != modified.end(); ++it) {
            AppEaiInferrerChnCfg_t channelinfo = *it;
            
            MediaChannel* pMediaChannel = searchMediaChannel(channelinfo.chnId);
            if(pMediaChannel){
                pMediaChannel->setSignalId(channelinfo.sigId);
                pMediaChannel->setGlobalParameter(mpGlobalData);
                pMediaChannel->setAlgorithmConfig(channelinfo.algoCfg);
            
                auto it = mSigId_ImgInfoMap.find(channelinfo.sigId);
                if(it != mSigId_ImgInfoMap.end()){
                    ImgSignal_t imgInfo = it->second;
                    pMediaChannel->setImageInfo(imgInfo);
                }
            }
        }
    }

    return ;
}

MediaChannel* MediaClient::searchMediaChannel(int chnId)
{
    for(auto iter = m_MediaChnlist.begin(); iter != m_MediaChnlist.end(); ++iter){
        if(chnId == (*iter)->mediaChnId()) {
            return (*iter);
        }
    }

    return NULL;
}

MediaChannel* MediaClient::createMediaChannel(int chnId)
{
    // 在原有队列中，存在目标信号，直接输出信号操作指针
    MediaChannel* pMediaChannel = searchMediaChannel(chnId);
    if(pMediaChannel){
        return pMediaChannel;
    }
    
    pMediaChannel = new(std::nothrow) MediaChannel();
    if(pMediaChannel){
        if(0 == pMediaChannel->init()){
//            pMediaChannel->setSignalManagerCallBack(this, SignalManagerCallBack);
            pMediaChannel->setMediaChnId(chnId);
            m_MediaChnlist.push_back(pMediaChannel);
            PRINT_DEBUG("Created a new MediaChannel[%d] ------", pMediaChannel->mediaChnId());
        }else{
            delete pMediaChannel;
            PRINT_ERROR("MediaChannel[%d] init failed!", chnId);
            return NULL;
        }
        
    }else{
        PRINT_ERROR("Created MediaChannel[%d] failed!", chnId);
        return NULL;
    }
    
    return pMediaChannel;
}

void MediaClient::destroyMediaChannel(int chnId)
{
    MediaChannel* pMediaChannel = NULL;
    for(auto iter = m_MediaChnlist.begin(); iter != m_MediaChnlist.end();){
        if(chnId == (*iter)->signalId()) {
            pMediaChannel = (*iter);
            
            // 1.回收信号资源
            delete pMediaChannel;

            // 2.把当前匹配到的节点从m_Signallist中拿掉
            iter = m_MediaChnlist.erase(iter);
            break;
        }else{
            ++iter;
        }
    }
    return ;
}


