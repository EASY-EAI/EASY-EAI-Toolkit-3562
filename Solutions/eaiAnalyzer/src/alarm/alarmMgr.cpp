//======================  C  ======================
#include <sys/vfs.h> 
#include <sys/time.h>
#include <curl/curl.h>
#include <libgen.h>
//=====================  PRJ  =====================
#include "db_AlarmRecord.h"
#include "alarmMgr.h"
//=====================  SDK  =====================
#include "base64.h"
#include "system_opt.h"
#include "ini_wrapper.h"
#include "log_manager.h"
#include "cJSON.h"
#include "ipc.h"

typedef struct{
	AlarmMgr *pSelf;
    char chnId[128];
    char alarmType[128];
    char startTime[128];
    char endTime[128];
	
}AlarmMgr_para_t;

static int mkdir_p(const char *path, mode_t mode) {
    char *path_copy = strdup(path);
    char *parent_dir = dirname(path_copy);
    
    // 如果父目录不存在，递归创建父目录
    if (access(parent_dir, F_OK) != 0) {
        if (mkdir_p(parent_dir, mode) != 0) {
            free(path_copy);
            return -1;
        }
    }
    
    free(path_copy);
    
    // 创建当前目录（如果已存在会失败，但这不是错误）
    if (mkdir(path, mode) != 0 && errno != EEXIST) {
        return -1;
    }
    
    return 0;
}

#define PICTURE_STORE "img/"
#define SRC_PICTURE_STORE "src_img/"
static void mkImgSotreDir(const char *alarmPath)
{
    //img目录
    std::string imageSotrePath = alarmPath;
    if('/' != imageSotrePath.at(imageSotrePath.length()-1)){
        imageSotrePath.append("/");
    }
    imageSotrePath.append(PICTURE_STORE);

    // 目录不存在
    if(0 != access(imageSotrePath.c_str(), F_OK)){
        // 创建目录
        if(0 != mkdir_p(imageSotrePath.c_str(), 0777)){
            PRINT_ERROR("[%s]:%s!", imageSotrePath.c_str() ,strerror(errno));
        }
    }
    //src_img目录
    std::string srcimageSotrePath = alarmPath;
    if('/' != srcimageSotrePath.at(srcimageSotrePath.length()-1)){
        srcimageSotrePath.append("/");
    }
    srcimageSotrePath.append(SRC_PICTURE_STORE);

    // 目录不存在
    if(0 != access(srcimageSotrePath.c_str(), F_OK)){
        // 创建目录
        if(0 != mkdir_p(srcimageSotrePath.c_str(), 0777)){
            PRINT_ERROR("[%s]:%s!", srcimageSotrePath.c_str(), strerror(errno));
        }
    }

    return ;
}

static void getCpuId(std::string &cpuId)
{
    char id[32]={0};

    cpuId.clear();
    
    if(0 == exec_cmd_by_popen("cat /proc/cpuinfo | grep Serial | awk '{print $3}'", id)){
        id[strlen(id)-1] = 0;
        cpuId.append(id);
    }
}

static std::string getUploadDateTime()
{
    std::string dateTime;
    char tmp[64];
    
    time_t tm;
    time(&tm); //获取time_t类型的当前时间

    strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", localtime(&tm));
    dateTime.clear();
    dateTime.append(tmp);
    
    return dateTime;
}
static std::string getLocalTimeStamp_ms()
{
    return std::to_string(get_timeval_ms());
}

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t data_len = size * nmemb;
    PRINT_DEBUG("======Http_callback_start======");
    PRINT_DEBUG("%s",ptr);
    PRINT_DEBUG("======Http_callback_end======");
    return data_len;
}

static int32_t send_json_to_Http_curl(const char *url, const char *json)
{
    CURL *curl = curl_easy_init();
    if (!curl) {
        PRINT_DEBUG( "Curl initialization failed");
        return -1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 1);
    
    //打印调试信息
    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        PRINT_DEBUG( "curl_easy_perform() failed: %s", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        return -1;
    }
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    return 0;
}

#if 0
static int32_t send_json_to_Http(const char *server, const char *func, const char *json, char *result, uint32_t result_length)
{
    httplib::Client cli(server);
	int minResultLength = result_length;
    std::string content_type = "application/json";
    std::string strfunc;
    std::string body;
    std::string resBody;
    resBody.clear();
    strfunc.clear();
    strfunc.append(func);
//    cout << strfunc << endl;
    body.clear();
    body.append(json);
//    cout << body << endl;
    /*
     * Send data
     */ 
    if (auto res = cli.Post(strfunc.c_str(), body, content_type.c_str())) {
        std::cout << res->status << std::endl;
        std::cout << res->get_header_value("Content-Type") << std::endl;
//        cout << res->body << endl;
        if(200 == res->status)
            resBody = res->body;
    } else {
         std::cout << "error status: " << res->status << std::endl;
        std::cout << "error code: " << res.error() << std::endl;
    }
    /*
     * handle Response
     */
	if(resBody.empty())
		return 0;
	if(resBody.length() <= result_length){
		minResultLength = resBody.length();
	}
	if(NULL == result)
		return -1;
	memcpy(result, resBody.c_str(), minResultLength);
	return 0;
}
#endif


//获取数据库中所有数据的type,name保存到AlarmTypeArray;
void AlarmMgr::getAlarmDBTypeToAlarmTypeArray()
{
    std::vector<AlarmInfo_t> algo_dev;
    alarmRecords_AcquiredDataType(algo_dev);

    //    清空
    std::vector<AppEaiInferrerAlarmInfo_t>().swap(mAlarmTypeArray);
    AppEaiInferrerAlarmInfo_t alarmItem;

    for (std::vector<AlarmInfo_t>::iterator it = algo_dev.begin(); it != algo_dev.end(); ++it) {
            AlarmInfo_t AlarmInfo_dev = *it;
            PRINT_DEBUG("AlarmInfo_desc:%s , AlarmInfo_Type:%s",AlarmInfo_dev.desc,AlarmInfo_dev.alarmType);
            //转移到mAlarmTypeArray
            strcpy(alarmItem.alarmType , AlarmInfo_dev.alarmType);//英文
            strcpy(alarmItem.alarmName , AlarmInfo_dev.desc);//中文
            mAlarmTypeArray.push_back(alarmItem);
    }
}
// static std::string changeDateTimeFmt(uint32_t date, uint32_t time)
// {
//     std::string dateTime;
//     char tmp[64] = {0};
    
//     sprintf(tmp, "%04u-%02u-%02u %02u:%02u:%02u", date/10000, (date%10000)/100, date%100,
//                                                   time/10000, (time%10000)/100, time%100);

//     dateTime.clear();
//     dateTime.append(tmp);
    
//     return dateTime;
// }
static uint64_t changToTimeStamp_s(char *strTime)
{
    struct tm stm;
    strptime(strTime, "%Y-%m-%d %H:%M:%S", &stm);
    uint64_t t = mktime(&stm);
    return t;
}
static uint64_t changToTimeStamp_ms(char *strTime)
{
    return 1000*changToTimeStamp_s(strTime);
}

#if 0
static bool takeAPartHttpApi(std::string &httpRequest, std::string &donet,  std::string &api)
{
    bool ret = false;
    
    if(httpRequest.empty())
        return ret;

    donet.clear();
    api.clear();

    std::string prefix = "http://";
    std::string delimiter = "/";
    std::string host;
    
    // 去掉URL前缀
    size_t pos = httpRequest.find(prefix);
    if (pos != std::string::npos) {
        host = httpRequest.substr(pos + prefix.length());
    }
    // 提取主机名和路径
    pos = host.find(delimiter);
    if (pos != std::string::npos) {
        api = host.substr(pos);
        donet = prefix +host.substr(0, pos);
    } 
    ret = true;

    return ret;
}
#endif

// static bool find_CTAD_picture(std::vector<CTAD_Info_t> &ctadFileList, uint32_t elDate, uint32_t elTime)
// {

//     return false;
// }

void *alarmMgr_thread(void *para)
{
    // 告警管理器对象
	AlarmMgr_para_t *pAlarmMgrPara = (AlarmMgr_para_t *)para;
    AlarmMgr *pSelf = pAlarmMgrPara->pSelf;
    
    // 本线程的控制参数
    int cout = 1;
    
    pSelf->mbThreadIsRunning = true;
    while(1){
        
        // AlarmMgr析构了
        if(NULL == pSelf){ msleep(5); break; }
        
        // 其它对象通过 AlarmMgr 对象控制本线程退出        
        if(!pSelf->mbThreadIsRunning){ usleep(20*1000); break; }

        // AlarmMgr 对象并没有初始化好
        if(0 == pSelf->isInited()){ usleep(100*1000); continue; }
        
       /* 说明：
        * if(exeAtStart == (count%TimeInterval))
        * exeAtStart: 是否在线程启动时执行。[0]-初次启动不执行，[n]-在第一个周期内的第n个单位时间执行
        * TimeInterval: 一个周期包含的时间单位
        */
        if(1 == (cout%(pSelf->heartBeat()*100))){    //heartBeat()s执行, 且启动时执行
            pSelf->sendHeartBeatReq();
        }
        if(0 == (cout%(5*100))){    //5s执行, 且启动时不执行
            pSelf->ContinuousTransmissionAfterDisconnection();//断网续传
            pSelf->DiskCapacityMonitoring();//磁盘容量监测
        }
        
        // 计时操作
        cout++;
        cout %= 50000;
        // 时间单位：10ms
		usleep(10*1000);
    }

    if(pAlarmMgrPara){
        pSelf = NULL;
        
        free(pAlarmMgrPara);
        pAlarmMgrPara = NULL;
    }

	pthread_exit(NULL);
}

void *delAlarmRec_thread(void *para){
	AlarmMgr_para_t *pAlarmMgrPara = (AlarmMgr_para_t *)para;
    AlarmMgr *pSelf = pAlarmMgrPara->pSelf;
    char *pChnId = pAlarmMgrPara->chnId;
    char *pAlarmType = pAlarmMgrPara->alarmType;
    char *pStartTime = pAlarmMgrPara->startTime;
    char *pEndTime = pAlarmMgrPara->endTime;

    if(NULL == pSelf){
        PRINT_ERROR("alarm Manager is Empty !");
        goto exit_delThread;
    }

    if((0 == strlen(pChnId))||(0 == strlen(pAlarmType))||(0 == strlen(pStartTime))||(0 == strlen(pEndTime))){
        PRINT_ERROR("alarm delete paras invalid !");
        goto exit_delThread;
    }

    pSelf->doDelAlarmRecord(pChnId, pAlarmType, pStartTime, pEndTime);

exit_delThread:
    if(pAlarmMgrPara){
        pSelf = NULL;
        
        free(pAlarmMgrPara);
        pAlarmMgrPara = NULL;
    }


    pthread_exit(NULL);
}



AlarmMgr::AlarmMgr()
{
    pthread_mutex_init(&alarmMgrLock, NULL);

	bObjIsInited = false;
    std::vector<AppEaiInferrerAlarmInfo_t>().swap(mAlarmTypeArray);
    mAlarmTypeNumber = 0;
    
    std::vector<ChnUploadPara_t>().swap(mUploadPara);

    mStrAlarmPath.clear();
        
    mStrUploadUrl.clear();
    mStrKeepAliveUrl.clear();
    mKeepAliveFrq = 0;

    mbThreadIsRunning = false;

    init();
}
AlarmMgr::~AlarmMgr()
{
    deInit();

    pthread_mutex_destroy(&alarmMgrLock);
}

void AlarmMgr::init()
{
    if(bObjIsInited)
        return ;

    char url[512];
    char alarmPath[128] = {0};
    if(access(ALARM_CONFIG_PATH, F_OK) == 0){
        pthread_mutex_lock(&alarmMgrLock);
        // 1.读取配置文件的上报地址
        memset(url, 0, sizeof(url));
        ini_read_string2(ALARM_CONFIG_PATH, "alarmSrv", "pictureUrl", url, sizeof(url));
        mStrUploadUrl.append(url);

        // 2.读取配置文件的心跳地址，以及心跳频率
        memset(url, 0, sizeof(url));
        ini_read_string2(ALARM_CONFIG_PATH, "alarmSrv", "heartBeatUrl", url,  sizeof(url));
        mStrKeepAliveUrl.append(url);
        
        mKeepAliveFrq = ini_read_int(ALARM_CONFIG_PATH, "alarmSrv", "heartBeatRate");

        mCTADEnable = ini_read_int(ALARM_CONFIG_PATH, "CTAD", "CTADEnable");
        mSrcImageEnable = ini_read_int(ALARM_CONFIG_PATH, "SrcImage", "SrcImageEnable");
        // 3.读取告警图片存放位置
        ini_read_string2(ALARM_CONFIG_PATH, "alarmPub", "alarmPath", alarmPath,  sizeof(alarmPath));
        mStrAlarmPath.append(alarmPath);
        mkImgSotreDir(mStrAlarmPath.c_str());
        
        // 4.读取目前已有的告警类型数量（用作遍历）
        mAlarmTypeNumber = ini_read_int(ALARM_CONFIG_PATH, "alarmPub", "alarmListSize");

#if 0
        // 5.遍历读出告警类型列表
        //    清空
        std::vector<AppEaiInferrerAlarmInfo_t>().swap(mAlarmTypeArray);

        //    插入 mAlarmTypeArray
        char itemSession[128];
        AppEaiInferrerAlarmInfo_t alarmItem;
        for(int32_t i = 0; i < mAlarmTypeNumber; i++){
            memset(&alarmItem, 0, sizeof(AppEaiInferrerAlarmInfo_t));
        
            memset(itemSession, 0, sizeof(itemSession));
            sprintf(itemSession, "alarmItem_%d", i);
            ini_read_string(ALARM_CONFIG_PATH, itemSession, "type", alarmItem.alarmType, sizeof(alarmItem.alarmType));
            ini_read_string(ALARM_CONFIG_PATH, itemSession, "name", alarmItem.alarmName, sizeof(alarmItem.alarmName));
            
            mAlarmTypeArray.push_back(alarmItem);
        }
#endif

        // 6.初始化告警记录数据库
        database_init();

        getAlarmDBTypeToAlarmTypeArray();
        sendAlarmTypeArrayToSoluAdapter();

        // 7.读出CPU_id;
        getCpuId(mCpuId);
 
        pthread_mutex_unlock(&alarmMgrLock);
        mbThreadIsRunning = false;
        AlarmMgr_para_t *pAlarmMgr = (AlarmMgr_para_t *)malloc(sizeof(AlarmMgr_para_t));
        memset(pAlarmMgr, 0, sizeof(AlarmMgr_para_t));
        pAlarmMgr->pSelf = this;
        if(0 != CreateNormalThread(alarmMgr_thread, pAlarmMgr, &mTid)){
            free(pAlarmMgr);
            return ;
        }

        bObjIsInited = true;
        PRINT_DEBUG("alarm Manager init OK !");
    }
}

void AlarmMgr::deInit()
{
    if(!bObjIsInited)
        return ;
    
    //手动退出线程
    if(mbThreadIsRunning){
        mbThreadIsRunning = false;
        //等待线程资源销毁。
        sleep(1); 
    }

    pthread_mutex_lock(&alarmMgrLock);

    mStrUploadUrl.clear();
    mStrKeepAliveUrl.clear();
    mKeepAliveFrq = 0;

    mStrAlarmPath.clear();

    std::vector<ChnUploadPara_t>().swap(mUploadPara);

    std::vector<AppEaiInferrerAlarmInfo_t>().swap(mAlarmTypeArray);
    mAlarmTypeNumber = 0;

    database_exit();

    pthread_mutex_unlock(&alarmMgrLock);


    bObjIsInited = false;
}

void AlarmMgr::uploadAlarm(Mat Image, Mat srcImage, UploadInfo_t uploadInfo, int16_t uploardFrq)
{ 
    if(!bObjIsInited)
        return ;

    if(access(ALARM_CONFIG_PATH, F_OK) != 0)
        return ;
    pthread_mutex_lock(&alarmMgrLock);
    bool bIsNeedAddUploadPara = true;
    uint64_t curUploardFrq = 0;
    uint64_t preTime = 0;
    for(uint32_t i = 0; i < mUploadPara.size(); i++){
    if(0 == strcmp(mUploadPara[i].chnId, uploadInfo.alarmChnID.c_str())){
        for(uint32_t j = 0; j <mUploadPara[i].algoArray.size(); j++){
        // 待上报的告警，参数已有历史记录
        if(0 == strcmp(mUploadPara[i].algoArray[j].algoType, uploadInfo.alarmType.c_str())){
            bIsNeedAddUploadPara = false;
            // 更新待上报告警的历史参数记录
            mUploadPara[i].algoArray[j].uploadFrq = (uint64_t)uploardFrq;
            curUploardFrq = mUploadPara[i].algoArray[j].uploadFrq;
            preTime = mUploadPara[i].algoArray[j].uploadTime;
        }
        }
    }
    }

    
    uint64_t curTime = get_timeval_ms();
    if((0 == curUploardFrq)     /* 无上报间隔，每一次识别都要上报 */
    || (0 == preTime)           /* 无上报记录，即首次上报 */
    || (curUploardFrq*1000 <= curTime - preTime) /* 超过上报时间：uploardFrq(秒) */
    ){
        mbIsUploaded = true;
        //printf("curUploardFrq:%llu,   curTime:%llu , preTime:%llu   ,curTime - preTime:%llu \n  ",curUploardFrq*1000,curTime,preTime,curTime - preTime);
        // 上报到客户服务器
        uploadInfo.alarmTime = getUploadDateTime();
        // char strConfidence[8] = {0};
        // snprintf(strConfidence, sizeof(strConfidence), "%.2f", 0.8369);
        std::string customInfo = makeCustomInfo(uploadInfo);
        if (0 != uploadAlarmToSrv(Image, uploadInfo, customInfo)){
            mbIsUploaded = false;
        }

        // 插记录、存图片、写文件
        addAlarmRecord(mbIsUploaded, Image, srcImage, uploadInfo, customInfo);

          // 记录发送时间
        for(uint32_t i = 0; i < mUploadPara.size(); i++){
            if(0 == strcmp(mUploadPara[i].chnId, uploadInfo.alarmChnID.c_str())){
                for(uint32_t j = 0; j < mUploadPara[i].algoArray.size(); j++){
                    if(0 == strcmp(mUploadPara[i].algoArray[j].algoType, uploadInfo.alarmType.c_str())){
                        mUploadPara[i].algoArray[j].uploadTime = curTime;
                    }
                }
            }
        }
    }
    
    // 新的，未曾记录的通道ID，在此处记录下来
    if(bIsNeedAddUploadPara){
        ChnUploadPara_t uploadItem = {0};
        AlgoUploadPara_t algoItem = {0};
    
        memcpy(uploadItem.chnId, uploadInfo.alarmChnID.c_str(), uploadInfo.alarmChnID.length());
        memcpy(algoItem.algoType, uploadInfo.alarmType.c_str(), uploadInfo.alarmType.length());
        algoItem.uploadFrq = (uint64_t)uploardFrq;
        algoItem.uploadTime = 0;
        
        uploadItem.algoArray.push_back(algoItem);
        
        mUploadPara.push_back(uploadItem);
    }

    pthread_mutex_unlock(&alarmMgrLock);
}


void AlarmMgr::delAlarmRecord(char *chnId, char *alarmType, char *startTime, char *endTime)
{
    pthread_t delAlarmRecTid;

    AlarmMgr_para_t *pAlarmMgr = (AlarmMgr_para_t *)malloc(sizeof(AlarmMgr_para_t));
    memset(pAlarmMgr, 0, sizeof(AlarmMgr_para_t));
    pAlarmMgr->pSelf = this;
    strcpy(pAlarmMgr->chnId, chnId);
    strcpy(pAlarmMgr->alarmType, alarmType);
    strcpy(pAlarmMgr->startTime, startTime);
    strcpy(pAlarmMgr->endTime, endTime);
    
    if(0 != CreateNormalThread(delAlarmRec_thread, pAlarmMgr, &delAlarmRecTid)){
        free(pAlarmMgr);
        return ;
    }
}






void AlarmMgr::doDelAlarmRecord(char *chnId, char *alarmType, char *startTime, char *endTime)
{
    if(!bObjIsInited)
        return ;
    pthread_mutex_lock(&alarmMgrLock);
    PRINT_DEBUG("【告警删除】！！！\n");
    AlarmRecItem_t items[50] = {0};
    //AlarmRecItem_t originalitems[50] = {0};
    int32_t itemNum = (sizeof(items)/sizeof(AlarmRecItem_t));
    int32_t getSuccItemNum = 0;
    int32_t getSuccoriginalItemNum = 0;
    char strChnId[8] = {0};
    if((0 == strcmp(chnId, "all")) ||
        (0 == strcmp(chnId, "All")) ||
        (0 == strcmp(chnId, "ALL"))
    ){
        strcpy(strChnId, "all");
    }
    
    char startTimeStamp[32] = {0};
    char endTimeStamp[32] = {0};
    if((0 == strcmp(startTime, "all")) ||
        (0 == strcmp(startTime, "All")) ||
        (0 == strcmp(startTime, "ALL"))
    ){
        strcpy(startTimeStamp, "0");
    }else{
        snprintf(startTimeStamp, sizeof(startTimeStamp), "%lu", changToTimeStamp_ms(startTime));
        PRINT_DEBUG("%llu, %s", changToTimeStamp_ms(startTime), startTimeStamp);
    }
    
    if((0 == strcmp(endTime, "all")) ||
        (0 == strcmp(endTime, "All")) ||
        (0 == strcmp(endTime, "ALL"))
    ){
        strcpy(endTimeStamp, "9999999999999"); // 2^63 = 9223372036854775808;
    }else{
        snprintf(endTimeStamp, sizeof(endTimeStamp), "%lu", changToTimeStamp_ms(endTime));
        PRINT_DEBUG("%lu, %s", changToTimeStamp_ms(endTime), endTimeStamp);
    }

    //将待删除的告警记录存入队列
    DelAlarmRecord_t delalarmInfo = {0};
    memcpy(delalarmInfo.alarmType, alarmType, strlen(alarmType));
    if(0 == strcmp(strChnId, "all")){
        memcpy(delalarmInfo.chnId, strChnId, strlen(strChnId));
    }else{
        memcpy(delalarmInfo.chnId, chnId, strlen(chnId));
    }
    memcpy(delalarmInfo.startTime, startTimeStamp, strlen(startTimeStamp));
    memcpy(delalarmInfo.endTime, endTimeStamp, strlen(endTimeStamp));
    mDelAlarmRecordArray.push_back(delalarmInfo);

    PRINT_DEBUG("【告警删除】进入while！！！\n");
    //告警记录队列加锁
    
    while(1){
        getSuccItemNum = alarmRecords_getRecord(items, itemNum, alarmType, chnId, startTimeStamp, endTimeStamp);
       // getSuccoriginalItemNum = originalRecords_getRecord(originalitems, itemNum, alarmType, chnId, startTimeStamp, endTimeStamp);
        PRINT_DEBUG("getSuccItemNum:%d===getSuccoriginalItemNum:%d",getSuccItemNum ,getSuccoriginalItemNum);

        if((getSuccItemNum <= 0)){
            PRINT_DEBUG("【告警删除】数据空！！！\n");
            break;
        }
        PRINT_DEBUG("【告警删除】开始删除！！！\n");


        // 删图片
        for(int i = 0; i < getSuccItemNum; i++){
            //先判断图片有没有存在，不存在读取会崩
            // char Photo[200]={0};
            // sprintf(Photo, "Alarm/%s", items[i].recPhotoName);
            PRINT_DEBUG("rm imgPath[%d] : %s",i, items[i].recPhotoName);
            PRINT_DEBUG("rm SrcimgPath[%d] : %s",i , items[i].recSrcPhotoName);
            if ( !access(items[i].recPhotoName,0) ){
                    //删除图片
                    if (remove(items[i].recPhotoName) != 0){
                        PRINT_DEBUG("【告警删除】删除告警图片失败！！！\n");
                    }else{
                        PRINT_DEBUG("【告警删除《%d》】删除告警图片成功%s！！！\n",i,items[i].recPhotoName);
                    }
            }else{
                PRINT_DEBUG("【告警删除】告警图片不存在！！！\n");
            }

            if ( !access(items[i].recSrcPhotoName,0) ){
                    //删除图片
                    if (remove(items[i].recSrcPhotoName) != 0){
                        PRINT_DEBUG("【告警删除】删除告警src图片失败！！！\n");
                    }else{
                        PRINT_DEBUG("【告警删除《%d》】删除告警src图片成功%s！！！\n",i,items[i].recSrcPhotoName);
                    }
            }else{
                PRINT_DEBUG("【告警删除】告警src图片不存在！！！\n");
            }
        }
        //删数据库记录

        alarmRecords_delRecord(alarmType,chnId,startTimeStamp,endTimeStamp,getSuccItemNum,1);

#if 0
        // 删src图片
        for(int i = 0; i < getSuccoriginalItemNum; i++){
            //先判断图片有没有存在，不存在读取会崩
            // char Photo2[200]={0};
            // sprintf(Photo2, "Alarm/%s", originalitems[i].recPhotoName);
            PRINT_DEBUG("rm SrcimgPath[%d] : %s",i , originalitems[i].recPhotoName);
            if ( !access(originalitems[i].recPhotoName,0) ){
                    //删除图片
                    if (remove(originalitems[i].recPhotoName) != 0){
                        PRINT_DEBUG("【告警删除】删除告警src图片失败！！！\n");
                    }else{
                        PRINT_DEBUG("【告警删除《%d》】删除告警src图片成功%s！！！\n",i,originalitems[i].recPhotoName);
                    }
            }else{
                PRINT_DEBUG("【告警删除】告警src图片不存在！！！\n");
            }
        }
        //删数据库记录

        originalRecords_delRecord(alarmType,chnId,startTimeStamp,endTimeStamp,getSuccoriginalItemNum,1);  
#endif

        usleep(100);
    }

    getAlarmDBTypeToAlarmTypeArray();
    sendAlarmTypeArrayToSoluAdapter();

    pthread_mutex_unlock(&alarmMgrLock);

}

void AlarmMgr::sendHeartBeatReq()
{
    if(!bObjIsInited)
        return ;

    pthread_mutex_lock(&alarmMgrLock);

    std::string req;
    req.clear();
    
    cJSON* pJsonObj = cJSON_CreateObject();
    
    cJSON_AddStringToObject(pJsonObj, "code", "AI-SERVER");
    cJSON_AddStringToObject(pJsonObj, "serialNo", mCpuId.c_str());
    std::string strDateTime = getUploadDateTime();
    cJSON_AddStringToObject(pJsonObj, "time", strDateTime.c_str());

    char *pReqStr = cJSON_PrintUnformatted(pJsonObj);
    if(pReqStr){
        req.append(pReqStr);
        free(pReqStr);
    }
    cJSON_Delete(pJsonObj);

    //char result[1024] = {0};
    if(!mStrKeepAliveUrl.empty()){
        // std::string donet;
        // std::string api;
        // if(takeAPartHttpApi(mStrKeepAliveUrl, donet, api)){
        //     PRINT_TRACE("target: %s%s", donet.c_str(), api.c_str());
        //     PRINT_TRACE("data: %s", req.c_str());
        //     send_json_to_Http(donet.c_str(), api.c_str(), req.c_str(), result, sizeof(result));
        // }
        send_json_to_Http_curl(mStrKeepAliveUrl.c_str(),req.c_str());
    }

    pthread_mutex_unlock(&alarmMgrLock);

}

/*
1.监测磁盘占用率
2.大于85%时删除旧告警图片(原图也删)及对应数据库内容(批量删除100张)
*/
void AlarmMgr::DiskCapacityMonitoring()
{
    if(!bObjIsInited)
        return ;
    double DiskUsage=0;
    pthread_mutex_lock(&alarmMgrLock);
    DiskUsage = partition_usage("/userdata");
    if( DiskUsage >= 85){//删除告警记录
        int i = 0;
        char* PhotoName = (char*)malloc(200);
        char* PhotoName_src = (char*)malloc(200);
        for(i=0;i<200;i++){
            alarmRecords_Minimum_Photo(PhotoName,PhotoName_src);//删除告警图片
            //先判断图片有没有存在，不存在读取会崩
            if ( !access(PhotoName,0) ){
                    alarmRecords_delet(PhotoName);
                    //删除图片
                    if (remove(PhotoName) != 0){
                        PRINT_DEBUG("【告警上限】删除告警图片失败！！！\n");
                    }else{
                        PRINT_DEBUG("【告警上限%d】删除告警图片成功%s！！！\n",i,PhotoName);
                    }
            }else{
                PRINT_DEBUG("【告警上限】告警图片不存在！！！\n");
                alarmRecords_delet(PhotoName);
            }

            if ( !access(PhotoName_src,0) ){
                    //删除图片
                    if (remove(PhotoName_src) != 0){
                        PRINT_DEBUG("【告警上限】删除SRC告警图片失败！！！\n");
                    }else{
                        PRINT_DEBUG("【告警上限%d】删除SRC告警图片成功%s！！！\n",i,PhotoName_src);
                    }
            }



#if 0
            originalRecords_Minimum_Photo(PhotoName_src);//删除告警原图
            char Photo_src[200]={0};
            sprintf(Photo_src, "Alarm/%s", PhotoName_src);
            if ( !access(Photo_src,0) ){
                    originalRecords_delet(PhotoName_src);
                    //删除图片
                    if (remove(Photo_src) != 0){
                        PRINT_DEBUG("【告警上限】删除src告警图片失败！！！\n");
                    }else{
                        PRINT_DEBUG("【告警上限%d】删除src告警图片成功%s！！！\n",i,Photo);
                    }
            }else{
                PRINT_DEBUG("【告警上限】src告警图片不存在！！！\n");
                originalRecords_delet(PhotoName_src);
            }
#endif

            usleep(2 * 1000);
        }
        sync();
        free(PhotoName);
        free(PhotoName_src);
    }

    pthread_mutex_unlock(&alarmMgrLock);
}



void AlarmMgr::ContinuousTransmissionAfterDisconnection()
{
    if(!bObjIsInited)
        return ;
    if(!mCTADEnable)
        return ;
    pthread_mutex_lock(&alarmMgrLock);

    //进行断网续传
    if(true ==  mbIsUploaded ){//判断此时网络状态（网络状态正常）
        //查找CTAD有无记录
        int  ret = -1;
        int  CTAD_id = CTAD_Minimum_ID();
        int  CTAD_recNum = CTAD_record_number();
        char recPhotoName[100] = {0};
        
        char recType[TYPE_LEN] = {0};
        char recDesc[DESC_LEN] = {0};
        char recChnID[CHNID_LEN] = {0};
        char recChnName[CHNNAME_LEN] = {0};
        char recTime[ALARMTIME_LEN] = {0};
        int  recTagNum = 0;
        
        char recCustomInfo[2048] = {0};
        if( CTAD_recNum > 0){//CTAD表有数据
            ret = CTAD_extraction_data(CTAD_id, recType, recDesc, recChnID, recChnName, recPhotoName, recTime, &recTagNum, recCustomInfo);
            if( -1 ==ret ){
                printf("CTAD_数据提取失败\n");
            }

            printf("CTAD_id:%d,%s,%s,%s,%s,%s,%s,%d,%s\n",CTAD_id,recType,recDesc,recChnID,recChnName,recPhotoName,recTime,recTagNum,recCustomInfo);
            //先判断图片有没有存在，不存在读取会崩
            char Photo[200]={0};
            sprintf(Photo, "Alarm/%s", recPhotoName);
            if (!access(Photo, 0) ){
                // 读取图片  
                cv::Mat img = cv::imread(Photo);
                // 构建告警信息
                UploadInfo_t uploadInfo;
                uploadInfo.alarmType.append(recType);
                uploadInfo.alarmName.append(recDesc);
                uploadInfo.alarmChnID.append(recChnID);
                uploadInfo.alarmChnName.append(recChnName);
                uploadInfo.alarmTime.append(recTime);
                uploadInfo.alarmTag.number = recTagNum;
                std::string strCustom;
                strCustom.append(recCustomInfo);

                // 上报告警操作
                uploadAlarmToSrv(img, uploadInfo, strCustom);//发送记录到服务器
                
                // 删除数据库记录
                delet_CTAD_record(CTAD_id);
                // 删除图片
                if (remove(Photo) != 0){
                    printf("【断网续传】删除告警图片失败！！！\n");
                }
            }else{
                printf("【断网续传】告警图片不存在！！！\n");
                // 删除数据库记录
                delet_CTAD_record(CTAD_id);
            }         
        }
    }

    pthread_mutex_unlock(&alarmMgrLock);
}

int32_t AlarmMgr::uploadAlarmToSrv(Mat Image, UploadInfo_t uploadInfo, std::string strCustom)
{
    int ret = -1;
    
    if(!bObjIsInited)
        return -1;

    std::string reqStr = uploadProcess(Image, mCpuId.c_str(), uploadInfo, strCustom);

    if(!mStrUploadUrl.empty()){
        int i =0;
        std::string donet;
        std::string api;
        PRINT_TRACE("mStrUploadUrl: %s", mStrUploadUrl.c_str());
        //printf("%s",reqStr.c_str());
        struct timeval start;
        struct timeval end;
        float time_use = 0;
        gettimeofday(&start,NULL);
        for(i = 0; i < 3; i++){
            ret = send_json_to_Http_curl(mStrUploadUrl.c_str(), reqStr.c_str());
            if(ret == 0){
                return 0;
            }
            usleep(200*1000);
        }
        gettimeofday(&end,NULL);
        time_use=(end.tv_sec-start.tv_sec)*1000000+(end.tv_usec-start.tv_usec);//微秒
        PRINT_DEBUG("http_curl_send_time_use is %f ms",time_use/1000);
    }
    return ret;
}


//初始化时发送一遍数据库中的type到solu->>>>>http
int32_t AlarmMgr::sendAlarmTypeArrayToSoluAdapter()
{
    int ret = -1;
    
    // 通知 EAI-box 的 adapter 更新 httpAdapter 的告警列表
    uint32_t AlarmType_number = mAlarmTypeArray.size();
    char *pAlarmTypeData = (char *)malloc(sizeof(AlarmType_number) + AlarmType_number*sizeof(AppEaiInferrerAlarmInfo_t));
    if(pAlarmTypeData){
        memset(pAlarmTypeData, 0, sizeof(AlarmType_number) + AlarmType_number*sizeof(AppEaiInferrerAlarmInfo_t));
    
    // 把数量写在头部
    memcpy(pAlarmTypeData, &AlarmType_number, sizeof(AlarmType_number));

    // 填充算法信息
    AppEaiInferrerAlarmInfo_t *pAlgoInfo = (AppEaiInferrerAlarmInfo_t *)(pAlarmTypeData + sizeof(AlarmType_number));
    for (std::vector<AppEaiInferrerAlarmInfo_t>::iterator it = mAlarmTypeArray.begin(); it != mAlarmTypeArray.end(); ++it) {
        AppEaiInferrerAlarmInfo_t AlarmInfo_dev = *it;
        //转移到内存
        strcpy(pAlgoInfo->alarmType , AlarmInfo_dev.alarmType);//英文
        strcpy(pAlgoInfo->alarmName , AlarmInfo_dev.alarmName);//中文
        PRINT_DEBUG("SOLU_alarmMgr pAlgoInfo : %s[%s]", pAlgoInfo->alarmType, pAlgoInfo->alarmName);

        pAlgoInfo++;
    }
    ret = IPC_client_sendData(APPADAPTER_CLI_ID, MSGTYPE_SOLU_EAIBOX_ALARMADDNEWTYPE, pAlarmTypeData, sizeof(AlarmType_number) + AlarmType_number*sizeof(AppEaiInferrerAlarmInfo_t));
    free(pAlarmTypeData);
    }

    return ret;
}



int32_t AlarmMgr::addAlarmRecord(bool bIsUploadSucc, Mat Image,  Mat srcImage, UploadInfo_t uploadInfo, std::string strCustom)
{
    int ret = -1;

    if(!bObjIsInited)
        return ret;

    if(mStrAlarmPath.empty())
        return ret;

    uint32_t curDate, curTime;
    get_system_date_time(&curDate, &curTime);
    std::string timeStamp = getLocalTimeStamp_ms(); //UTC
    //std::string strDateTime = changeDateTimeFmt(curDate, curTime);

    std::string imageFileName;
    std::string imageFilePath = mStrAlarmPath;

    std::string srcimageFileName;
    std::string srcimageFilePath = mStrAlarmPath;

    imageFileName.clear();
    imageFileName.append(PICTURE_STORE);
    imageFileName += uploadInfo.alarmType;
    imageFileName.append("-"); 
    imageFileName += uploadInfo.alarmChnID;
    imageFileName.append("-");
    imageFileName += std::to_string(curDate);
    imageFileName.append("-");
    imageFileName += std::to_string(curTime);
    imageFileName.append(".jpeg");
    if('/' != imageFilePath.at(imageFilePath.length()-1)){
        imageFilePath.append("/");
    }
    imageFilePath += imageFileName;

    // 2.保存图片
    PRINT_DEBUG("save imageFile : %s", imageFilePath.c_str());
    imwrite(imageFilePath.c_str(), Image);
    sync();

    //保存原图
    if(1 == mSrcImageEnable){
        PRINT_DEBUG("[alarmMgr]mSrcImageEnable:%d",mSrcImageEnable);
        srcimageFileName.clear();
        srcimageFileName.append(SRC_PICTURE_STORE);
        srcimageFileName += uploadInfo.alarmType;
        srcimageFileName.append("-");
        srcimageFileName += uploadInfo.alarmChnID;
        srcimageFileName.append("-");
        srcimageFileName += std::to_string(curDate);
        srcimageFileName.append("-");
        srcimageFileName += std::to_string(curTime);
        srcimageFileName.append(".jpeg");

        if('/' != srcimageFilePath.at(srcimageFilePath.length()-1)){
            srcimageFilePath.append("/");
        }
        srcimageFilePath += srcimageFileName;

        // 2.保存图片
        PRINT_DEBUG("save imageFile : %s", imageFilePath.c_str());
        imwrite(srcimageFilePath.c_str(), srcImage);
        sync();
        // //存数据库
        // ret = originalRecords_addRecord(Type, Name, chnId, chnName, Confidence ,(char *)srcimageFilePath.c_str(), (char *)timeStamp.c_str());
    }
    
    if(1 == mSrcImageEnable){
        //告警图存数据库
        ret = alarmRecords_addRecord(uploadInfo.alarmType.c_str(),  uploadInfo.alarmName.c_str(),
                                     uploadInfo.alarmChnID.c_str(), uploadInfo.alarmChnName.c_str(),
                                     "emmc", 
                                     imageFilePath.c_str(), srcimageFilePath.c_str(),
                                     timeStamp.c_str());
    }else{
        //告警图存数据库
        ret = alarmRecords_addRecord(uploadInfo.alarmType.c_str(),  uploadInfo.alarmName.c_str(),
                                     uploadInfo.alarmChnID.c_str(), uploadInfo.alarmChnName.c_str(),
                                     "emmc",
                                     imageFilePath.c_str(), "", timeStamp.c_str());       
    }

    // 3.若上报失败，则把消息加入断网续传队列
    if(!bIsUploadSucc &&  (!mStrUploadUrl.empty())){
        if(1 == mCTADEnable){
            ret = add_CTAD_record(uploadInfo.alarmType.c_str(),  uploadInfo.alarmName.c_str(),
                                  uploadInfo.alarmChnID.c_str(), uploadInfo.alarmChnName.c_str(),
                                  imageFileName.c_str(), timeStamp.c_str(), 
                                  uploadInfo.alarmTag.number, strCustom.c_str());
        }
    }

    // 4.判断Type是否已存在
    bool bIsNeedAddAlarmType = true;
    for(uint32_t i = 0; i < mAlarmTypeArray.size(); i++){
        // 该条告警类型记录已存在配置文件中
        if(0 == strcmp(mAlarmTypeArray[i].alarmType, uploadInfo.alarmType.c_str())){
            bIsNeedAddAlarmType = false;
            break;
        }
    }

    // 4.1若不存在，则插入一条新的告警类型
    if(bIsNeedAddAlarmType){
        // 写入文件
        // char itemSession[128] = {0};
        // sprintf(itemSession, "alarmItem_%d", mAlarmTypeNumber);
        // ret = ini_write_string(ALARM_CONFIG_PATH, itemSession, "type", Type);
        // ret = ini_write_string(ALARM_CONFIG_PATH, itemSession, "name", Name);
        // 新的告警记录写入成功
        if(0 == ret){
            // 写入新的告警记录数量
            //mAlarmTypeNumber += 1;
            //ini_write_int(ALARM_CONFIG_PATH, "alarmPub", "alarmListSize", mAlarmTypeNumber);

            // 同步好新的告警信息
            AppEaiInferrerAlarmInfo_t alarmInfo = {0};
            memcpy(alarmInfo.alarmType, uploadInfo.alarmType.c_str(), uploadInfo.alarmType.length());
            memcpy(alarmInfo.alarmName, uploadInfo.alarmName.c_str(), uploadInfo.alarmName.length());
            mAlarmTypeArray.push_back(alarmInfo);

            sendAlarmTypeArrayToSoluAdapter();
        }

    // 4.2若已存在，则看一下是否需要修改告警名称
    }else{
        char itemSession[128] = {0};
        char itemType[ALGOTYPE_LENGTH];
        char itemName[ALARMNAME_LENGTH];
        for(uint32_t i = 0; i < mAlarmTypeArray.size(); i++){
        if(0 == strcmp(mAlarmTypeArray[i].alarmType, uploadInfo.alarmType.c_str())){
            // 修改内存中的告警名称
            memset(mAlarmTypeArray[i].alarmName, 0, sizeof(mAlarmTypeArray[i].alarmName));
            memcpy(mAlarmTypeArray[i].alarmName, uploadInfo.alarmName.c_str(), uploadInfo.alarmName.length());

            // 修改文件中的记录
            for(int32_t j = 0; j <mAlarmTypeNumber; j++){
                memset(itemType, 0, sizeof(itemType));
                memset(itemName, 0, sizeof(itemName));
                sprintf(itemSession, "alarmItem_%d", j);
                ret = ini_read_string2(ALARM_CONFIG_PATH, itemSession, "type", itemType, sizeof(itemType));
                ret = ini_read_string2(ALARM_CONFIG_PATH, itemSession, "name", itemName, sizeof(itemName));
                if( (0 == ret) &&
                    (0 == strcmp(itemType, uploadInfo.alarmType.c_str())) &&
                    (0 != strcmp(itemName, uploadInfo.alarmName.c_str()))
                ){
                    ini_write_string(ALARM_CONFIG_PATH, itemSession, "name", uploadInfo.alarmName.c_str());
                    break;
                }
            }
            break;
        }
        }
    }

    return ret;
}


