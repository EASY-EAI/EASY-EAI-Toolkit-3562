#ifndef __UPLOAD_PROCESS_H__
#define __UPLOAD_PROCESS_H__

#include <vector>
#include <string>
#include <opencv2/opencv.hpp>

using namespace cv;

typedef struct {
    int number;     //目标数量
    std::vector<std::string> vec_confidence; //目标置信度
    std::vector<std::string> vec_rect;   //目标框框位置

} Target_t;

typedef struct {
    std::string alarmType;      //告警类型：某种告警的唯一标识
    std::string alarmName;      //告警名称：这种告警叫什么，比如【发现明火】
    std::string alarmChnID;     //告警通道ID：对告警通道的唯一标识
    std::string alarmChnName;   //告警通道名：产生告警所在通道的名称，比如【仓库】
    std::string alarmTime;      //告警发生时间：以毫秒为单位的时间戳
    Target_t alarmTag;          //告警对象信息：

} UploadInfo_t;



extern std::string makeCustomInfo(UploadInfo_t uploadInfo);
extern std::string uploadProcess(Mat image, const char *strCpuId, UploadInfo_t uploadInfo, std::string strCustomInfo);

#endif
