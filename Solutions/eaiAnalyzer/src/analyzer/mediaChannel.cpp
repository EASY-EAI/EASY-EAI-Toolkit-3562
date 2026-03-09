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
#include "mediaChannel.h"
#include <opencv2/opencv.hpp>

#define PTRINT uint64_t

static int32_t addResultToJson(cJSON *parentArray, Result_t algoResult)
{    
    cJSON *result = cJSON_CreateObject();
    if(!result)
        return -1;
    
    cJSON_AddStringToObject(result, "algoName", "person");
    cJSON *resultItems = cJSON_AddArrayToObject(result, "result");
    if(!resultItems){
        cJSON_Delete(result);
        return -1;
    }

    if(algoResult.person_group.count <= 0){
        cJSON_Delete(result);
        return -2;
    }
    
	for (int i = 0; i < algoResult.person_group.count; i++) {
		detect_result_t* det_result = &(algoResult.person_group.results[i]);
		if(det_result->prop < 0.4) {
			continue;
		}
        
        cJSON *item = cJSON_CreateObject();
        if(item){
            cJSON_AddStringToObject(item, "label", det_result->name);
            cJSON_AddNumberToObject(item, "prop", det_result->prop * 100);
            cJSON_AddNumberToObject(item, "x1", det_result->box.left);
            cJSON_AddNumberToObject(item, "y1", det_result->box.top);
            cJSON_AddNumberToObject(item, "x2", det_result->box.right);
            cJSON_AddNumberToObject(item, "y2", det_result->box.bottom);
            cJSON_AddItemToArray(resultItems, item);
        }
	}
    
    cJSON_AddItemToArray(parentArray, result);
    return 0;
}

static int writeAJsonFile(const char *filename, cJSON *root)
{
	struct timeval start;
	struct timeval end;
	float time_use=0;
    
    std::string tempFile;
    tempFile.clear();
    tempFile.append(filename);
    tempFile.append(".tmp");
    
    int fd = open(tempFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        PRINT_ERROR("open temp file failed");
        return -1;
    }

    // 获取排他锁
    if (flock(fd, LOCK_EX) < 0) {
        PRINT_ERROR("flock failed");
        close(fd);
        return -2;
    }
    
    char *json_str = cJSON_PrintUnformatted(root);
    if (!json_str) {
        close(fd);
        return -3;
    }
    
    ssize_t bytes = write(fd, json_str, strlen(json_str));
    if (bytes < 0) {
        PRINT_ERROR("write %s failed", tempFile.c_str());
    } else {
        // fsync(fd);  // 确保数据刷盘
        // 原子替换文件
        gettimeofday(&start,NULL);
        if (rename(tempFile.c_str(), filename) < 0) {
            PRINT_ERROR("rename %s failed", filename);
        }
        gettimeofday(&end,NULL);
    }
    
	time_use=(end.tv_sec-start.tv_sec)*1000000+(end.tv_usec-start.tv_usec);//微秒
	//printf(">>> time_use is %f ms\n",time_use/1000);
    
    cJSON_free(json_str);
    close(fd);
    return 0;
}

static PTRINT calcBufMapOffset(int chnId, int screenWidth, int screenHeight, int units)
{
    int xUnitOffset = chnId%units;
    int yUnitOffset = chnId/units;
    
    int winWidth  = screenWidth/units;
    int winHeight = screenHeight/units;
    
    PTRINT BufMapOffset = 3*(yUnitOffset*winHeight*screenWidth + xUnitOffset*winWidth);
    //printf("yU = %u, winHeight = %u, xU = %u, winWidth = %u, offset = %lu\n", yUnitOffset, winHeight, xUnitOffset, winWidth, BufMapOffset);
    return BufMapOffset;
}

static RgaSURF_FORMAT rgaFmt(char *strImgFmt)
{
    RgaSURF_FORMAT rgaFormat;
    if(0 == strcmp(strImgFmt, IMG_FORMAT_NV12)){
        rgaFormat = RK_FORMAT_YCbCr_420_SP;
    }else if(0 == strcmp(strImgFmt, IMG_FORMAT_NV21)){
        rgaFormat = RK_FORMAT_YCrCb_420_SP;
    }else if(0 == strcmp(strImgFmt, IMG_FORMAT_BGR888)){
        rgaFormat = RK_FORMAT_BGR_888;
    }else if(0 == strcmp(strImgFmt, IMG_FORMAT_RGB888)){
        rgaFormat = RK_FORMAT_RGB_888;
    }else{
        rgaFormat = RK_FORMAT_UNKNOWN;
    }
    return rgaFormat;
}

void *channelProcess_thread(void *para)
{
// ====== 信号取流线程 ======
    MediaChannel *pSelf = (MediaChannel*)para;

    // 初始化算法输出文件名
    std::string outFileName;
    outFileName.clear();
    outFileName.append(APP_PATH);
    outFileName.append("/chn");
    outFileName += std::to_string(pSelf->mediaChnId());
    outFileName.append("_reslut.json");
    
    pSelf->mThreadWorking = true;
    /* 等待信号管理器初始化完成 */
    while(!pSelf->IsInited()) {
        msleep(500);
        if(!pSelf->mThreadWorking){
            break;
        }
    }
    
    ChannelInfo_t chnInfo = {0};
    chnInfo.chnId = pSelf->mediaChnId();
    chnInfo.chnName = pSelf->mImage.name;
	PRINT_DEBUG("MediaChannel[%d] initialize completed, analy thread running ...", chnInfo.chnId);

    Result_t algoResult;

    int32_t matWidth = 1920;
    int32_t matHeight = 1080;
    cv::Mat image = cv::Mat(matHeight, matWidth, CV_8UC3);
    while(1){
        if(!pSelf->mThreadWorking){
            msleep(5);
            break;
        }
        
        if(NULL == pSelf){
            msleep(5);
            break;
        }

        // 判断通道是否还在线
        // === mark ===先在这里打个标记，逻辑还没完整
        //if(1 != state){
        //    msleep(15);
        //    continue;
        //}

        // 如果通道没有信号，会被拦在这里
        if((0 == pSelf->mImage.width)||(0 == pSelf->mImage.height)){
            msleep(15);
            continue;
        }
        
        if(!pSelf->mpAlarmMgr){
            msleep(30);
            continue;
        }
        
        // 判断是否需要重新分配mat对象大小
        if((matWidth != pSelf->mImage.width) || (matHeight != pSelf->mImage.height)){
            matWidth = pSelf->mImage.width;
            matHeight = pSelf->mImage.height;
            image = cv::Mat(matHeight, matWidth, CV_8UC3);
        }

        // 这个是线程锁，是防止IPCServer发消息过来修改 mImage.data。
        pSelf->imgLock();
        if(pSelf->mImage.data){
            // 这个是rga锁，是防止多个mediaChannel来竞争rga的。
            RgaLock::instance()->Lock();
            rga_info_t src;
            memset(&src, 0, sizeof(rga_info_t));
            src.fd = -1;
            src.virAddr = pSelf->mImage.data;
            src.mmuFlag = 1;
            src.rotation = 0;
            rga_set_rect(&src.rect, 0, 0, matWidth, matHeight, matWidth, matHeight, rgaFmt(pSelf->mImage.dataFmt));
            
            rga_info_t dst;
            memset(&dst, 0, sizeof(rga_info_t));
            dst.fd = -1;
            dst.virAddr = image.data;
            dst.mmuFlag = 1;
            rga_set_rect(&dst.rect, 0, 0, matWidth, matHeight, matWidth, matHeight, RK_FORMAT_BGR_888);
            
            if (c_RkRgaBlit(&src, &dst, NULL)) {
                printf("%s: rga fail\n", __func__);
            }
            RgaLock::instance()->UnLock();

        }
        pSelf->imgUnLock();
        // 把识别结果输出到一个json文件
        cJSON *root = cJSON_CreateObject();
        if (!root)
            continue;
        
        cJSON *algoResult_json = cJSON_AddArrayToObject(root, "algoResult");
        if (algoResult_json) {
            memset(&algoResult, 0, sizeof(algoResult));
            ModelLock::instance()->Lock(); //模型锁，由于NPU是系统全局的，在调用一个detect_run的过程中，再去调用另外一个，则会出现算法结果混乱
            
            pSelf->algoCfgRLock(); //算法参数锁，防止算法在使用的过程中被更新掉导致的异常
            algorithmProcess(image, chnInfo, pSelf->mAlgoCfgList, &algoResult, pSelf->mpAlarmMgr);
            pSelf->algoCfgUnLock();

            // === mark === 目前只提取了person result，后面继续改进
            addResultToJson(algoResult_json, algoResult);
            ModelLock::instance()->UnLock();
            
            writeAJsonFile(outFileName.c_str(), root);
            //printf("------------------ write json file -------------\n");
        }
            
        // 清理内存
        cJSON_Delete(root);

        msleep(20);
    }
    
    PRINT_DEBUG("exit MediaChannel[%d] analy thread", chnInfo.chnId);
    pthread_exit(NULL);
}


MediaChannel::MediaChannel() :
    mpAlarmMgr(NULL)
{
    //std::vector<ModelCtx_t>().swap(mModelCtxList);
    algoCfgLock_Init();
}

MediaChannel::~MediaChannel()
{
	PRINT_ERROR("start to destroy mediaChannel[%d](Signal[%d])...", mediaChnId(), signalId());
    
    destroyMediaChannelThread();
    algoCfgLock_UnInit();
    bObjIsInited = 0;
    
	PRINT_ERROR("destroy mediaChannel[%d](Signal[%d]) succ...", mediaChnId(), signalId());
	PRINT_ERROR("======================================================================");
}

int32_t MediaChannel::init()
{
    // 初始化算法信息
    std::vector<AlgoCfg_t>().swap(mAlgoCfgList);
    
	bObjIsInited = 1;

    return 0;
}

void MediaChannel::setGlobalParameter(char *pParameter)
{
    if(pParameter)
        mpAlarmMgr = (AlarmMgr *)pParameter;
#if 0
    int32_t modelNum = 0;
    if(pParameter){
        memcpy(&modelNum, pParameter, sizeof(modelNum));
        std::vector<ModelCtx_t>().swap(mModelCtxList);

        ModelCtx_t modelCtx;
        ModelCtx_t *pModelCtx = (ModelCtx_t *)(pParameter + sizeof(modelNum));
        for(int32_t i = 0; i < modelNum; i++){
            memcpy(&modelCtx, pModelCtx, sizeof(modelCtx));
            mModelCtxList.push_back(modelCtx);

            pModelCtx++;
        }
    }
#endif
    return ;
}

void MediaChannel::setAlgorithmConfig(char *pAlgoCfg)
{
    algoCfgWLock();

    std::vector<AlgoCfg_t>().swap(mAlgoCfgList);//只要工作参数变更了，就该被清空。目的是为不让分析线程采用错误的工作参数进行处理，优先保证程序稳定性。
    if(pAlgoCfg){
        cJSON* root = cJSON_Parse(pAlgoCfg);
        if (root) {
            if(!cJSON_IsArray(root)){
                cJSON_Delete(root);
                algoCfgUnLock();
                return ;
            }
            
            int arraySize = cJSON_GetArraySize(root);
            for (int i = 0; i < arraySize; i++) {
                cJSON* item = cJSON_GetArrayItem(root, i);
                if (NULL == item)
                    continue;

                AlgoCfg_t config;
            
                // 解析Type字段 -> algoType
                int algoTypeLen = sizeof(config.algoType);
                memset(config.algoType, 0, algoTypeLen);
                cJSON* typeItem = cJSON_GetObjectItem(item, "Type");
                if (typeItem != nullptr && cJSON_IsString(typeItem)) {
                    const char* typeStr = typeItem->valuestring;
                    strncpy(config.algoType, typeStr, algoTypeLen - 1);
                    config.algoType[algoTypeLen - 1] = '\0'; // 确保以null结尾
                }
            
                // 解析AlarmName字段 -> alarmName
                int alarmNameLen = sizeof(config.alarmName);
                memset(config.alarmName, 0, alarmNameLen);
                cJSON* alarmNameItem = cJSON_GetObjectItem(item, "AlarmName");
                if (alarmNameItem != nullptr && cJSON_IsString(alarmNameItem)) {
                    const char* alarmNameStr = alarmNameItem->valuestring;
                    strncpy(config.alarmName, alarmNameStr, alarmNameLen - 1);
                    config.alarmName[alarmNameLen - 1] = '\0';
                }
            
                // 解析Enable字段
                config.enable = 0;
                cJSON* enableItem = cJSON_GetObjectItem(item, "Enable");
                if (enableItem != nullptr) {
                    if (cJSON_IsBool(enableItem)) {
                        config.enable = enableItem->valueint; // cJSON的bool类型用valueint
                    } else if (cJSON_IsNumber(enableItem)) {
                        config.enable = enableItem->valueint;
                    }
                }
            
                // 解析Sensibility字段
                config.sensibility = 0;
                cJSON* sensibilityItem = cJSON_GetObjectItem(item, "Sensibility");
                if (sensibilityItem != nullptr && cJSON_IsNumber(sensibilityItem)) {
                    config.sensibility = sensibilityItem->valueint;
                }
            
                // 解析UploadFrq字段
                config.uploadFrq = 0;
                cJSON* uploadFrqItem = cJSON_GetObjectItem(item, "UploadFrq");
                if (uploadFrqItem != nullptr && cJSON_IsNumber(uploadFrqItem)) {
                    config.uploadFrq = uploadFrqItem->valueint;
                }
            
                // 解析AreaList字段
                std::vector<AreaInfo_t>().swap(config.areas);
                cJSON* areaList = cJSON_GetObjectItem(item, "AreaList");
                if (areaList != nullptr && cJSON_IsArray(areaList)) {
                    
                    /* area */
                    int areaSize = cJSON_GetArraySize(areaList);
                    for(int areaIndex = 0; areaIndex < areaSize; areaIndex++){
                        cJSON *areaItem = cJSON_GetArrayItem(areaList, areaIndex);
                        if (NULL == areaItem)
                            continue;
                        
                        AreaInfo_t area;
                        std::vector<Point_t>().swap(area.points);
                        cJSON* pointList = cJSON_GetObjectItem(areaItem, "PointList");
                        if (pointList != nullptr && cJSON_IsArray(pointList)) {
                            
                            /* point */
                            int pointSize = cJSON_GetArraySize(pointList);
                            for(int pointIndex = 0; pointIndex < pointSize; pointIndex++){
                                cJSON *pointItem = cJSON_GetArrayItem(pointList, pointIndex);
                                if (NULL == pointItem)
                                    continue;

                                Point_t point = {0};
                                cJSON *point_x = cJSON_GetObjectItem(pointItem, "x");
                                if (point_x != nullptr && cJSON_IsNumber(point_x)) {
                                    point.x = point_x->valueint;
                                }
                                cJSON *point_y = cJSON_GetObjectItem(pointItem, "y");
                                if (point_y != nullptr && cJSON_IsNumber(point_y)) {
                                    point.y = point_y->valueint;
                                }
                                area.points.push_back(point);
                            }                            
                        }
                        config.areas.push_back(area);
                    }
                }
            
                // 将配置添加到列表
                mAlgoCfgList.push_back(config);
            }
            
            cJSON_Delete(root);        
        }
    }
    algoCfgUnLock();
    return ;
}


