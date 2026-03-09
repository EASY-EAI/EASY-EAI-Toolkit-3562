#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "log_manager.h"

// -- logCfg --
#define LOGCONFIG_PATH    "/userdata/apps/app-eaiAnalyzer/logCfg"


// -- app Config --
#define APP_NAME    "app-analyzer"
#define APP_VERSION "V_0.00.001"
#define APP_PATH    "/tmp/apps/signalMgr" //用来存放临时文件
#define IMAGE_CONFIG_FILE "imgData.ini"
// ------> algorithm
#define ALGODEF_CONFIG_PATH "/userdata/apps/app-eaiAnalyzer/AlgoDefaultCfg.ini"
// ------> alarm
#define ALARM_RECORD_PATH   "/userdata/apps/app-eaiAnalyzer/Alarm"
#define ALARM_CONFIG_PATH   "./AlarmConfig.ini"
//#define MGRPLATFORM_CFG_PATH "./MgrPlatFormConfig.ini"



#endif //__CONFIG_H__

