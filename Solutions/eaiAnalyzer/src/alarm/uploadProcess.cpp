//=====================  PRJ  =====================
#include "uploadProcess.h"

//=====================  SDK  =====================
#include "log_manager.h"
#include "base64.h"
#include "cJSON.h"
#include "ipc.h"

/***********************************************************
    向3对齐
************************************************************/
#define ALIGN3(n) ((n + 2)/3*3)


std::string makeCustomInfo(UploadInfo_t uploadInfo)
{
    std::string jsonStr;
    jsonStr.clear();

    return jsonStr;
}

std::string uploadProcess(Mat image, const char *strCpuId, UploadInfo_t uploadInfo, std::string strCustomInfo/*由上面的makeCustomInfo返回值构造*/)
{
    std::string reqStr;
    reqStr.clear();
    
    cJSON *root = cJSON_CreateArray();
    if(!root){    
        return reqStr;
    }

    cJSON* pJsonObj = cJSON_CreateObject();
    if (!pJsonObj){
        cJSON_Delete(root);    
        return reqStr;
    }
    
    //void *pJsonObj = create_json_object();

    // 事件类型
    std::string strTypeNum;
    strTypeNum.clear();
         if(0 == strcmp(uploadInfo.alarmType.c_str(), "person")   ){strTypeNum.append("1");}   //人形检测(未穿反光衣)
    else if(0 == strcmp(uploadInfo.alarmType.c_str(), "smoking")  ){strTypeNum.append("2");}   //抽烟检测
    else if(0 == strcmp(uploadInfo.alarmType.c_str(), "calling")  ){strTypeNum.append("3");}   //打电话检测
    else if(0 == strcmp(uploadInfo.alarmType.c_str(), "phone")    ){strTypeNum.append("4");}   //玩手机检测
    else if(0 == strcmp(uploadInfo.alarmType.c_str(), "helmet")   ){strTypeNum.append("5");}   //安全帽检测
    else if(0 == strcmp(uploadInfo.alarmType.c_str(), "person")   ){strTypeNum.append("6");}   //主流工服
    else if(0 == strcmp(uploadInfo.alarmType.c_str(), "person")   ){strTypeNum.append("7");}   //反光衣
    else if(0 == strcmp(uploadInfo.alarmType.c_str(), "bandArea") ){strTypeNum.append("8");}   //区域入侵
    else if(0 == strcmp(uploadInfo.alarmType.c_str(), "person")   ){strTypeNum.append("9");}   //攀爬
    else if(0 == strcmp(uploadInfo.alarmType.c_str(), "person")   ){strTypeNum.append("10");}  //离岗
    else if(0 == strcmp(uploadInfo.alarmType.c_str(), "peopleCount")){strTypeNum.append("11");}  //人数统计
    else if(0 == strcmp(uploadInfo.alarmType.c_str(), "crowds")   ){strTypeNum.append("12");}  //人员聚集
    else if(0 == strcmp(uploadInfo.alarmType.c_str(), "garbage")  ){strTypeNum.append("13");}  //大件垃圾
    else if(0 == strcmp(uploadInfo.alarmType.c_str(), "e-bike")   ){strTypeNum.append("14");}  //电瓶车
    else if(0 == strcmp(uploadInfo.alarmType.c_str(), "fire")     ){strTypeNum.append("15");}  //明火
    else if(0 == strcmp(uploadInfo.alarmType.c_str(), "smoke")    ){strTypeNum.append("16");}  //烟雾
    else if(0 == strcmp(uploadInfo.alarmType.c_str(), "gasoline") ){strTypeNum.append("17");}  //漏油渗油
    else if(0 == strcmp(uploadInfo.alarmType.c_str(), "carDetect")){strTypeNum.append("18");}  //车辆检测
    else if(0 == strcmp(uploadInfo.alarmType.c_str(), "carDetect")){strTypeNum.append("19");}  //通达占用
    else if(0 == strcmp(uploadInfo.alarmType.c_str(), "carDetect")){strTypeNum.append("20");}  //车辆违停
    else if(0 == strcmp(uploadInfo.alarmType.c_str(), "digger")   ){strTypeNum.append("21");}  //挖掘机
    else if(0 == strcmp(uploadInfo.alarmType.c_str(), "cable")    ){strTypeNum.append("22");}  //输电线上异物
    else if(0 == strcmp(uploadInfo.alarmType.c_str(), "detect")   ){strTypeNum.append("23");}  //特定物品识别
    else if(0 == strcmp(uploadInfo.alarmType.c_str(), "mask")   ){strTypeNum.append("24");}  //口罩识别
    else if(0 == strcmp(uploadInfo.alarmType.c_str(), "peopleFell")   ){strTypeNum.append("25");}  //摔倒识别
    else{strTypeNum.append("99");}
    
    cJSON_AddStringToObject(pJsonObj, "eventType", strTypeNum.c_str());
    // 需要在图片上打上时间
    
    // cv::mat  === to === >> jpg
    std::vector<unsigned char> jpg_data;
	std::vector<int> param = std::vector<int>(2);
	// 设置压缩数据的格式，以及压缩的质量
	// param[0]=CV_IMWRITE_JPEG_QUALITY;
    param[0]=IMWRITE_JPEG_QUALITY;
	param[1]=60;//default(95) 0-100
	// 将rgb数据压缩成jpg数据
	if(cv::imencode(".jpg", image, jpg_data, param)){
        if(0 < jpg_data.size()){
            char *pSrcJpgData = NULL;
            pSrcJpgData = (char *)malloc(jpg_data.size());
            if(pSrcJpgData){
                //PRINT_DEBUG("[before] pSrcJpgData : 0x%08x", pSrcJpgData);
            	// 将压缩的jpg数据，放到输出buf中
            	std::copy(jpg_data.begin(), jpg_data.end(), pSrcJpgData);

                char *pB64Image = NULL;

                int32_t base64ImageSize = ALIGN3(jpg_data.size())*4/3 + 1;

                pB64Image = (char *)malloc(base64ImageSize);
                memset(pB64Image, 0, base64ImageSize);
                if(pB64Image){
                    //PRINT_DEBUG("[before] pB64Image : 0x%08x", pB64Image);
                    base64_encode(pB64Image, (const char*)pSrcJpgData, jpg_data.size());
                    //printf("%s", pB64Image);
                    cJSON_AddStringToObject(pJsonObj, "image", pB64Image);
                    //PRINT_DEBUG("[after] pB64Image : 0x%08x", pB64Image);
                    free(pB64Image);
                }
                //PRINT_DEBUG("[after] pSrcJpgData : 0x%08x", pSrcJpgData);
                free(pSrcJpgData);
            }
        }    	
    }
    
    // 告警发生时间
    cJSON_AddStringToObject(pJsonObj, "time", uploadInfo.alarmTime.c_str());
    
    // 合作商编码
    cJSON_AddStringToObject(pJsonObj, "code", "AI-SERVER");
    
    // CpuId
    cJSON_AddStringToObject(pJsonObj, "serialNo", strCpuId);
    
    // 告警数量
    cJSON_AddNumberToObject(pJsonObj, "num", uploadInfo.alarmTag.number);
    
    // 可信度 Confidence
    std::string strConfidence;
    if(uploadInfo.alarmTag.vec_confidence.empty()){
        // 拆解strCustomInfo
    }else{
        for (auto it = uploadInfo.alarmTag.vec_confidence.begin(); it != uploadInfo.alarmTag.vec_confidence.end(); ++it) {
            strConfidence += *it;
            strConfidence.append(",");
        }
        if (!strConfidence.empty()) { strConfidence.pop_back();}
        cJSON_AddStringToObject(pJsonObj, "score", strConfidence.c_str());
    }
    
    // 目标框坐标
    std::string strRect;
    if(uploadInfo.alarmTag.vec_rect.empty()){
        // 拆解strCustomInfo
    }else{
        for (auto it = uploadInfo.alarmTag.vec_rect.begin(); it != uploadInfo.alarmTag.vec_rect.end(); ++it) {
            strRect += *it;
            strRect.append(",");
        }
        if (!strRect.empty()) { strRect.pop_back();}
        cJSON_AddStringToObject(pJsonObj, "Rect", strRect.c_str());
    }
    
    // 告警内容
    cJSON_AddStringToObject(pJsonObj, "context", uploadInfo.alarmName.c_str());
    
    // channel Id
    cJSON_AddStringToObject(pJsonObj, "channelId", uploadInfo.alarmChnID.c_str());
    
    // 告警设备名
    cJSON_AddStringToObject(pJsonObj, "deviceName", " ");
    
    // 告警位置
    cJSON_AddStringToObject(pJsonObj, "position", uploadInfo.alarmChnName.c_str());


    cJSON_AddItemToArray(root, pJsonObj);
    
    // 将cJSON对象转换为字符串
    char* pReqStr = cJSON_PrintUnformatted(root);
    if (pReqStr) {
        reqStr.append(pReqStr);
        free(pReqStr);  // 释放cJSON分配的内存
    }
    
    // 释放cJSON树
    cJSON_Delete(root);

    return reqStr;
}
