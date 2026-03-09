#ifndef __IPCDATA_H__
#define __IPCDATA_H__


#include <stdint.h>

/* 端口分配：
 *     命令：netstat -tunl     (查看端口占用情况)
 * 端口说明：
 *     [7000]：IPC服务器端口
 */

#define CLITNE_ID_BASE   7000
#define HTTPADAPTER_CLI_ID   (CLITNE_ID_BASE+1)
#define APPADAPTER_CLI_ID    (CLITNE_ID_BASE+2)
#define APPSIGNALMGR_CLI_ID  (CLITNE_ID_BASE+3)
#define APPALGOCFGMGR_CLI_ID (CLITNE_ID_BASE+4)
#define APPCHANNELMGR_CLI_ID (CLITNE_ID_BASE+5)
#define APPCOMBINER_CLI_ID   (CLITNE_ID_BASE+6)
#define ANALYZER_CLI_ID      (CLITNE_ID_BASE+7)


enum MsgType{
    MSGTYPE_NULL = 0,
    MSGTYPE_SOLU_PUBLIC_DATA,
    MSGTYPE_SOLU_PUBLIC_UPGRADE,
    MSGTYPE_SOLU_PUBLIC_NETWORK,
    //MSGTYPE_APP_EAIBOX_registerClient,
    MSGTYPE_APP_EAIBOX_reqUpdateSignals,
    MSGTYPE_APP_EAIBOX_updateSignals,
    MSGTYPE_APP_EAIBOX_releaseShareMem,
    MSGTYPE_SOLU_EAIBOX_ALARMSRV,
    MSGTYPE_SOLU_EAIBOX_3RDPF,
    MSGTYPE_SOLU_EAIBOX_ALGOSUPPORT,
    MSGTYPE_SOLU_EAIBOX_ALARMCFG,
    MSGTYPE_SOLU_EAIBOX_ALARMADDNEWTYPE,
    MSGTYPE_SOLU_EAIBOX_ALARMDELREC,
    MSGTYPE_SOLU_EAIBOX_REBOOTCHANNEL,
    MSGTYPE_APP_EAIBOX_reqAlgorithmDefCfg,
    MSGTYPE_APP_EAIBOX_AlgorithmDefCfg,
    MSGTYPE_APP_EAIBOX_reqChannelCfg,
    MSGTYPE_APP_EAIBOX_channelCfg,
    MSGTYPE_SOLU_EAIBOX_CHANNELSTATE,
    MSGTYPE_SOLU_EAIBOX_CHANNELRESOLUTION,
    MSGTYPE_SOLU_ALGOMODEL_REGUSER,
    MSGTYPE_NUM
};

//  =============================== Public ===============================
typedef struct {
    char name[16];
    char version[16];
    char upgradePath[64];
}SoftWareInfo_t;
typedef struct {
    char cpuId[32];
    char macAddr[32];
    SoftWareInfo_t solution;
    SoftWareInfo_t model;
}SoluPubInfo_t;

typedef struct {
    char packetName[32];
    char packetPath[128];
    char packetMD5[32];
}UpgradeInfo_t;

//[0-升级失败]，[1-升级成功]，[2-升级中]
#define UPGEADE_FAILD 0
#define UPGEADE_SUCC  1
#define UPGEADEING    2
typedef struct {
    int32_t httpsAdapter;
    int32_t solution;
    int32_t algoModel;
}UpgradeRes_t;

typedef struct {
    char ipAddr[16];
    char netMask[16];
    char gateWay[16];
    char DNS[16];
}NetParaInfo_t;

//  =============================== Eai-box ===============================
typedef struct {
    char alarmPicUrl[512];
    char alarmHBUrl[512];
    int32_t alarmHBRate;
    int32_t alarmCTADEnable;
    int32_t alarmSrcImageEnable;
    char alarmPath[128];
}SoluEaiBoxAlarmSrv_t;

typedef struct {
    char _3rdPlatformUrl[128];
    char _3rdPlatformHBUrl[128];
    int32_t _3rdPlatformHBRate;
}SoluEaiBox3rdPfs_t;

#define CHANNELID_LENGTH 16
#define ALGOTYPE_LENGTH 16
#define ALGONAME_LENGTH 64
#define ALGORES_LENGTH 64
#define ALARMNAME_LENGTH 64
#define ALARMTIME_LENGTH 32

typedef struct {
    char type[ALGOTYPE_LENGTH];
    char name[ALGONAME_LENGTH];
    char alarmName[ALARMNAME_LENGTH];
    char iconPath[128];
    int32_t enable;
    int32_t sensibility;
    int32_t uploadFrq;
}AppEaiInferrerAlgoInfo_t;

#define ALGO_CFG_MAX_LENGTH 2048//注意一下，这里可能会出bug
typedef struct {
    int32_t chnId;
    int32_t sigId;
    //char resolution[ALGORES_LENGTH];
    char algoCfg[ALGO_CFG_MAX_LENGTH];
}AppEaiInferrerChnCfg_t;

typedef struct {
    char alarmType[ALGOTYPE_LENGTH];
    char alarmName[ALARMNAME_LENGTH];
}AppEaiInferrerAlarmInfo_t;

typedef struct {
    char alarmType[ALGOTYPE_LENGTH];
    char chnId[CHANNELID_LENGTH];
    char startTime[ALARMTIME_LENGTH];
    char endTime[ALARMTIME_LENGTH];
}AppEaiInferrerDelAlarmInfo_t;

typedef struct {
    char id[CHANNELID_LENGTH];
    int32_t state;
}SoluEaiBoxChnState_t;

typedef struct {
        char id[CHANNELID_LENGTH];
        uint32_t Width;
        uint32_t Height;
        uint32_t Bpp;
}SoluEaiBoxChnRessolution_t;



#endif //__IPCDATA_H__

