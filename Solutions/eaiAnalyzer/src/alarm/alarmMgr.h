#ifndef __ALARMMGR_H__
#define __ALARMMGR_H__
//=====================   C   =====================
#include "appSystem.h"
#include "sysIPCData.h"
#include "cusIPCData.h"
#include "config.h"
//=====================  C++  =====================
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <mutex>
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>

#include "uploadProcess.h"

using namespace cv;

#define CHANNELNAME_LENGTH 64

typedef struct {
    char algoType[ALGOTYPE_LENGTH];
    uint64_t uploadFrq;
    uint64_t uploadTime;
}AlgoUploadPara_t;
typedef struct {
    char chnId[CHANNELID_LENGTH];
    std::vector<AlgoUploadPara_t> algoArray;
}ChnUploadPara_t;

typedef struct{
    char type[ALGOTYPE_LENGTH];
    char chnId[CHANNELID_LENGTH];
    char chnName[CHANNELNAME_LENGTH];
    char confidence[8];
    char fileName[128];
    uint32_t picDate;
    uint32_t picTime;
}CTAD_Info_t;

typedef struct{
    char chnId[64];
    char alarmType[128];
    char startTime[254];
    char endTime[254];
}DelAlarmRecord_t;



class AlarmMgr
{
public:
	AlarmMgr();
	~AlarmMgr();

    /************************** 这些有锁的接口不能内部互相嵌套调用 **************************/
    void init();
    void deInit();
    void uploadAlarm(Mat Image, Mat srcImage, UploadInfo_t uploadInfo, int16_t uploardFrq);
    void delAlarmRecord(char *chnId, char *alarmType, char *startTime, char *endTime);
    void doDelAlarmRecord(char *chnId, char *alarmType, char *startTime, char *endTime);
    void sendHeartBeatReq();
    void ContinuousTransmissionAfterDisconnection();
    void DiskCapacityMonitoring();
    /***************************************************************************************/


	int32_t isInited(){return bObjIsInited;}
	int32_t heartBeat(){return mKeepAliveFrq;}
    bool mbThreadIsRunning;
protected:
	
private:
    // CTAD: 断网续传(Continuous transmission after disconnection)
    void CTAD_setWorking(bool bIsWorking);
    void CTAD_EarliestTime(uint32_t *date, uint32_t *time);
    void CTAD_setEarliestTime(uint32_t date, uint32_t time);

    int32_t uploadAlarmToSrv(Mat Image, UploadInfo_t uploadInfo, std::string strCustom);
    int32_t addAlarmRecord(bool bIsUploadSucc, Mat Image, Mat srcImage, UploadInfo_t uploadInfo, std::string strCustom);
    void getAlarmDBTypeToAlarmTypeArray();
    int32_t sendAlarmTypeArrayToSoluAdapter();
    int32_t addCTADRecord(char *Type, char *Name, char *chnId, char *chnName);

    int32_t mAlarmTypeNumber;

    std::vector<DelAlarmRecord_t> mDelAlarmRecordArray;

    std::vector<AppEaiInferrerAlarmInfo_t> mAlarmTypeArray;

    std::vector<ChnUploadPara_t> mUploadPara;
    
    std::string mStrUploadUrl;
    std::string mStrKeepAliveUrl;
    int32_t mKeepAliveFrq;
    int32_t mCTADEnable;
    int32_t mSrcImageEnable;


    std::string mStrAlarmPath;

    std::string mCpuId;
    bool mbIsUploaded;
    
	int32_t bObjIsInited;
    
    pthread_mutex_t alarmMgrLock;
	pthread_t mTid;
};

#endif
