#ifndef __ALGO_PROCESS_H__
#define __ALGO_PROCESS_H__

#include "../alarm/alarmMgr.h"

//=====================  C++  =====================
#include <iostream>
#include <fstream>
#include <mutex>
#include <map>
#include <opencv2/opencv.hpp>

//=====================  SDK  =====================
#include "geometry.h"
//#include "bmp_opt.h"
#include "helmet_detect.h"
#include "fire_detect.h"
#include "car_detect.h"
#include "face_detect.h"
#include "face_alignment.h"
//#include "face_mask_judgement.h"
#include "person_detect.h"
//#include "smoke_detect.h"
//#include "phonecall_detect.h"

//===========  Custom algorithm header  ===========
/*#include "../bird/bird_detect.h"*/

typedef struct{
    int  chnId;
    char *chnName;
}ChannelInfo_t;


typedef struct{
    int16_t x;
    int16_t y;
}Point_t;
typedef struct{
    std::vector<Point_t> points;
}AreaInfo_t;
typedef struct{
    int32_t number;
    int32_t enable;
    int32_t sensibility;
    int32_t uploadFrq;
    char algoType[ALGOTYPE_LENGTH];
    char alarmName[ALARMNAME_LENGTH];
    std::vector<AreaInfo_t> areas;
}AlgoCfg_t;

#define Helmet_Model_Bit    (0x1<<0)
#define Fire_Model_Bit      (0x1<<1)
#define Car_Model_Bit       (0x1<<2)
#define Face_Model_Bit      (0x1<<3)
//#define Face_Mask_Model_Bit (0x1<<4)
#define Person_Model_Bit    (0x1<<5)
//#define Phonecall_Model_Bit (0x1<<6)
//#define Smoking_Model_Bit   (0x1<<7)

#define HELMET_MODEL_PATH    "Algorithm/helmet_detect.model"
#define FIRE_MODEL_PATH      "Algorithm/fire_detect.model"
#define CAR_MODEL_PATH       "Algorithm/car_detect.model"
#define FACE_MODEL_PATH      "Algorithm/face_detect.model"
//#define FACEMASK_MODEL_PATH  "Algorithm/face_mask_judgement.model"
#define PERSON_MODEL_PATH    "Algorithm/person_detect.model"
//#define PHONECALL_MODEL_PATH "Algorithm/phonecall_detect.model"
//#define SMOKE_MODEL_PATH     "Algorithm/smoke_detect.model"

#define MAX_ALGO_NUM 6
typedef struct{
    // 模型输出
//    phonecall_detect_result_group_t phonecall_group;
    detect_result_group_t fire_group;
	detect_result_group_t person_group;
	detect_result_group_t helmet_group;
//    smoke_detect_result_group_t  smoking_group;
//    mask_detect_result_group_t   mask_group;
    detect_result_group_t car_group;
    
    // 算法配置以及输出
    //AlgoCfg_t algo[MAX_ALGO_NUM];
}Result_t;


//==================================================================================
int initModelHandle(const char *modelCfg_path);
int unInitModelHandle();
int algorithmProcess(Mat imgSrc, ChannelInfo_t chnInfo, std::vector<AlgoCfg_t> algoConfigs, Result_t *pResult, AlarmMgr *pAlarmMgr);

#endif
