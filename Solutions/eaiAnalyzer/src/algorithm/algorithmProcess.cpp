#include "algorithmProcess.h"

//======================  C  ======================
#include <sys/time.h>
#include <math.h>

//=====================  SDK  =====================
#include "system_opt.h"
//#include "json_parser.h"
#include "ini_wrapper.h"
#include "font_engine.h"


static Scalar colorArray[10]={
    Scalar(0, 0, 255, 255),
    Scalar(0, 255, 0, 255),
    Scalar(139,0,0,255),
    Scalar(0,100,0,255),
    Scalar(0,139,139,255),
    Scalar(0,206,209,255),
    Scalar(255,127,0,255),
    Scalar(72,61,139,255),
    Scalar(0,255,0,255),
    Scalar(255,0,0,255),
};
static int plot_one_box(Mat src, int x1, int y1, int x2, int y2, char *label, char colour)
{
    int tl = round(0.002 * (src.rows + src.cols) / 2) + 1;
    rectangle(src, cv::Point(x1, y1), cv::Point(x2, y2), colorArray[(unsigned char)colour], 3);

    int tf = max(tl -1, 1);

    int base_line = 0;
    cv::Size t_size = getTextSize(label, FONT_HERSHEY_SIMPLEX, (float)tl/3, tf, &base_line);
    int x3 = x1 + t_size.width;
    int y3 = y1 - t_size.height - 3;

    rectangle(src, cv::Point(x1, y1), cv::Point(x3, y3), colorArray[(unsigned char)colour], -1);
    putText(src, label, cv::Point(x1, y1 - 2), FONT_HERSHEY_SIMPLEX, (float)tl/3, cv::Scalar(255, 255, 255, 255), tf, 8);
    return 0;
}

static int sqrt_quick(unsigned long long n)
{
    unsigned int target = 0;
//    unsigned long long before;
    unsigned long long after;

//    before = target;
    for(;;)
    {
        after = target*target;
        if(n <= after){
            return target;
        }
//        before = after;
        
        target++;
    }
}

static int algoParaIndex(std::vector<AlgoCfg_t> algoCfgs, const char *strAlgoType)
{
    int index = -1;
    for(uint32_t i = 0; i < algoCfgs.size(); i++){
        if(0 == strcmp(algoCfgs[i].algoType, strAlgoType)){
            index = i;
            break;
        }
    }
    return index;
}
static int32_t algoEnable(std::vector<AlgoCfg_t> algoCfgs, int index)
{
    if(0 <= index){
        return algoCfgs[index].enable;
    }else{
        return 0;
    }
}

static std::map<std::string, rknn_context> g_algoMap;
//初始化模型对象指针，根据modelcfg初始模型
int initModelHandle(const char *modelCfg_path)
{
    uint64_t algoBitmap = 0;
    rknn_context algoCtx;
    
    char modelSupportList[1024] = {0};
    ini_read_string2(modelCfg_path, "public", "supportList", modelSupportList, sizeof(modelSupportList));
    if (access(modelCfg_path, F_OK) == 0) {
        // 1.解析算法支持列表
        int start=0, end=0;
        std::vector<std::string> algoArray;
        std::string strModelList;
        strModelList.append(modelSupportList);
        for (unsigned int i = 0; i < strModelList.length(); i++) {
            if(('_' == strModelList[i])&&(i < (strModelList.length()-1))){
                start = i;
                for(unsigned int j = i+1; j < strModelList.length(); j++){
                    if('_' == strModelList[j]) {end=j; break;}
                }
                algoArray.push_back(strModelList.substr(start+1, end-start-1));
            }
        }

        // 2.根据算法支持列表初始化对应算法模型
        char algoSession[16];
        char type[ALGOTYPE_LENGTH];
        for(unsigned int i = 0; i < algoArray.size(); i++){
            //（1）查询列表对应type
            memset(algoSession, 0, sizeof(algoSession));
            sprintf(algoSession, "algo_%s", algoArray[i].c_str());
            ini_read_string2(modelCfg_path, algoSession, "type", type, sizeof(type));
            PRINT_DEBUG("[analyzer]: algo_type = %s", type);
            //（2）根据type初始化算法对应所需要的模型
            if(0 == strcmp(type, "peopleCount")){  //人数统计
                algoBitmap |= Person_Model_Bit;
            }else if(0 == strcmp(type, "helmet")){ //安全帽检测
                algoBitmap |= Helmet_Model_Bit;
            }else if(0 == strcmp(type,"fire")){//火焰
                algoBitmap |= Fire_Model_Bit;
            }else if(0 == strcmp(type,"car")){//车辆
                algoBitmap |= Car_Model_Bit;
#if 0
            }else if(0 == strcmp(type,"mask")){//口罩
                algoBitmap |= Face_Model_Bit;
                algoBitmap |= Face_Mask_Model_Bit;
#endif
            }else if(0 == strcmp(type,"bandArea")){//周界
                algoBitmap |= Person_Model_Bit;
            }else if(0 == strcmp(type,"peopleFell")){//摔倒
                algoBitmap |= Person_Model_Bit;
            }else if(0 == strcmp(type,"crowds")){//人群聚集
                algoBitmap |= Helmet_Model_Bit;
#if 0
            }else if(0 == strcmp(type,"smoking")){//抽烟检测
                algoBitmap |= Person_Model_Bit;
                algoBitmap |= Smoking_Model_Bit;
            }else if(0 == strcmp(type,"phonecall")){//打电话检测
                algoBitmap |= Phonecall_Model_Bit;
#endif
            }
        }

        // 3.根据所需的模型进行模型初始化，及其把name与cxt进行绑定（map）
        algoCtx = 0;
        if( algoBitmap & Person_Model_Bit ){   //人员检测---用于--->(周界, 人员摔倒, 人数统计)
            person_detect_init(&algoCtx, PERSON_MODEL_PATH);
            PRINT_DEBUG("person_ctx:%p", algoCtx);
            if(0 < algoCtx){
                g_algoMap.insert(std::pair<std::string, rknn_context>(PERSON_MODEL_PATH, algoCtx));
            }
        }
        algoCtx = 0;
        if( algoBitmap & Helmet_Model_Bit ){   //安全帽检测---用于--->(未戴安全帽, 人群密集判定)
            helmet_detect_init(&algoCtx, HELMET_MODEL_PATH);
            PRINT_DEBUG("helmet_ctx:%p", algoCtx);
            if(0 < algoCtx){
                g_algoMap.insert(std::pair<std::string, rknn_context>(HELMET_MODEL_PATH, algoCtx));
            }
        }
        algoCtx = 0;
        if( algoBitmap & Fire_Model_Bit ){   //火焰检测
            fire_detect_init(&algoCtx, FIRE_MODEL_PATH);
            PRINT_DEBUG("Fire_ctx:%p", algoCtx);
            if(0 < algoCtx){
                g_algoMap.insert(std::pair<std::string, rknn_context>(FIRE_MODEL_PATH, algoCtx));
            }
        }
        algoCtx = 0;
        if( algoBitmap & Car_Model_Bit ){   //车辆检测
            car_detect_init(&algoCtx, CAR_MODEL_PATH);
            PRINT_DEBUG("Car_ctx:%p", algoCtx);
            if(0 < algoCtx){
                g_algoMap.insert(std::pair<std::string, rknn_context>(CAR_MODEL_PATH, algoCtx));
            }
        }
        algoCtx = 0;
        if( algoBitmap & Face_Model_Bit ){   //人脸检测---用于--->(未佩戴口罩检测)
            face_detect_init(&algoCtx, FACE_MODEL_PATH);
            PRINT_DEBUG("Face_ctx:%p", algoCtx);
            if(0 < algoCtx){
                g_algoMap.insert(std::pair<std::string, rknn_context>(FACE_MODEL_PATH, algoCtx));
            }
        }
#if 0
        algoCtx = 0;
        if( algoBitmap & Face_Mask_Model_Bit ){   //佩戴口罩检测
            face_mask_judgement_init(&algoCtx, FACEMASK_MODEL_PATH);
            PRINT_DEBUG("FaceMask_ctx:%p", algoCtx);
            if(0 < algoCtx){
                g_algoMap.insert(std::pair<std::string, rknn_context>(FACEMASK_MODEL_PATH, algoCtx));
            }
        }
        algoCtx = 0;
        if( algoBitmap & Phonecall_Model_Bit ){   //打电话检测
            phonecall_detect_init(&algoCtx, PHONECALL_MODEL_PATH);
            PRINT_DEBUG("PhoneCall_ctx:%p", algoCtx);
            if(0 < algoCtx){
                g_algoMap.insert(std::pair<std::string, rknn_context>(PHONECALL_MODEL_PATH, algoCtx));
            }
        }
        algoCtx = 0;
        if( algoBitmap & Smoking_Model_Bit ){   //抽烟检测
            smoke_detect_init(&algoCtx, SMOKE_MODEL_PATH);
            PRINT_DEBUG("Smoking_ctx:%p", algoCtx);
            if(0 < algoCtx){
                g_algoMap.insert(std::pair<std::string, rknn_context>(SMOKE_MODEL_PATH, algoCtx));
            }
        }
#endif

    }else{
        return -1;
    }



    // 创建全局字体，用于给图像写中文
	global_font_create("./simhei.ttf", CODE_UTF8);
	global_font_set_fontSize(40);

    return 0;
}

int unInitModelHandle()
{
    rknn_context algoCtx;
    //释放模型
    std::map<std::string, rknn_context>::iterator iter;
    for(iter = g_algoMap.begin(); iter != g_algoMap.end(); iter++){
        if( iter->first == PERSON_MODEL_PATH ){
            algoCtx = (rknn_context)iter->second;
            if(algoCtx){
                person_detect_release(algoCtx);
            }
        }else if( iter->first == HELMET_MODEL_PATH ){
            algoCtx = (rknn_context)iter->second;
            if(algoCtx){
                helmet_detect_release(algoCtx);
            }
        }else if( iter->first == CAR_MODEL_PATH ){
            algoCtx = (rknn_context)iter->second;
            if(algoCtx){
                car_detect_release(algoCtx);
            }
#if 0
        }else if( iter->first == PHONECALL_MODEL_PATH ){
            algoCtx = (rknn_context)iter->second;
            if(algoCtx){
                phonecall_detect_release(algoCtx);
            }
#endif
        }else if( iter->first == FIRE_MODEL_PATH ){
            algoCtx = (rknn_context)iter->second;
            if(algoCtx){
                fire_detect_release(algoCtx);
            }
        }else if( iter->first == FACE_MODEL_PATH ){
            algoCtx = (rknn_context)iter->second;
            if(algoCtx){
                face_detect_release(algoCtx);
            }
#if 0
        }else if( iter->first == FACEMASK_MODEL_PATH ){
            algoCtx = (rknn_context)iter->second;
            if(algoCtx){
                face_mask_judgement_release(algoCtx);
            }
        }else if( iter->first == SMOKE_MODEL_PATH ){
            algoCtx = (rknn_context)iter->second;
            if(algoCtx){
                smoke_detect_release(algoCtx);
            }
#endif
        }
    }
    return 0;
}

int algorithmProcess(Mat imgSrc, ChannelInfo_t chnInfo, std::vector<AlgoCfg_t> algoConfigs, Result_t *pResult, AlarmMgr *pAlarmMgr)
{
    int ret = -1;
    
// =======================[算法使用以及告警处理]================================
    // 获取初始化好的modeCxt
    rknn_context personCtx = 0;
    rknn_context helmetCtx = 0;
    rknn_context fireCtx = 0;
    rknn_context carCtx = 0;
    rknn_context faceCtx = 0;
    rknn_context maskjudgeCtx = 0;
    rknn_context smokingCtx = 0;
    rknn_context phonecallCtx = 0;
    std::map<std::string, rknn_context>::iterator iter;
    for(iter = g_algoMap.begin(); iter != g_algoMap.end(); iter++){
        if( iter->first == PERSON_MODEL_PATH ){
            personCtx = (rknn_context)iter->second;
        }else if( iter->first == HELMET_MODEL_PATH ){
            helmetCtx = (rknn_context)iter->second;
        }else if( iter->first == FIRE_MODEL_PATH ){
            fireCtx = (rknn_context)iter->second;
        }else if( iter->first == CAR_MODEL_PATH ){
            carCtx = (rknn_context)iter->second;
        }else if( iter->first == FACE_MODEL_PATH ){
            faceCtx = (rknn_context)iter->second;
#if 0
        }else if( iter->first == FACEMASK_MODEL_PATH ){
            maskjudgeCtx = (rknn_context)iter->second;
        }else if( iter->first == SMOKE_MODEL_PATH ){
            smokingCtx = (rknn_context)iter->second;
        }else if( iter->first == PHONECALL_MODEL_PATH ){
            phonecallCtx = (rknn_context)iter->second;
#endif
        }
    }
    
    // 算法配置索引
    int carIndex         = algoParaIndex(algoConfigs, "car");
    int fireIndex        = algoParaIndex(algoConfigs, "fire");
    int maskIndex        = algoParaIndex(algoConfigs, "mask");
    int helmetIndex      = algoParaIndex(algoConfigs, "helmet");
    int crowdsIndex      = algoParaIndex(algoConfigs, "crowds");
    int personCountIndex = algoParaIndex(algoConfigs, "peopleCount");
    int peopleFellIndex  = algoParaIndex(algoConfigs, "peopleFell");
    int bandAreaIndex    = algoParaIndex(algoConfigs, "bandArea");
    int smokingIndex     = algoParaIndex(algoConfigs, "smoking");
    int phonecallIndex   = algoParaIndex(algoConfigs, "phonecall");

    // 算法输出结果
    int16_t personNum = 0;
    int16_t helmetNum = 0;
	int16_t fireNum = 0;
	int16_t maskNum = 0;
	int16_t carNum = 0;
//=======================[图像送入算法模型，输出结果]================================
    // 人员检测
    if( algoEnable(algoConfigs, personCountIndex) ||
        algoEnable(algoConfigs, peopleFellIndex) ||
        algoEnable(algoConfigs, phonecallIndex) ||
        algoEnable(algoConfigs, bandAreaIndex) ||
        algoEnable(algoConfigs, smokingIndex) )
    {
        if(0 != personCtx){
#if 0
            AlgoCfg_t algoCfg;
            if(0 <= personCountIndex){
                algoCfg = algoConfigs[personCountIndex];
                printf("personCount ==============>>\n");
                int areaSize = algoCfg.areas.size();
                for(int i = 0; i <areaSize; i++){
                    printf("area[%d]:\n",i);
                    int pointSize = algoCfg.areas[i].points.size();
                    for(int j = 0; j < pointSize; j++){
                        printf("(%d,%d)", algoCfg.areas[i].points[j].x, algoCfg.areas[i].points[j].y);
                    }
                    printf("\n");
                }
            }
#endif
            ret = person_detect_run(personCtx, imgSrc, &pResult->person_group);
            for (int i = 0; i < pResult->person_group.count; i++){
                detect_result_t *det_result = &(pResult->person_group.results[i]);
                if(0 == strcmp(det_result->name, "person")){
                    personNum++;
                }
            }
            if(0 <= smokingIndex){ algoConfigs[smokingIndex].number = personNum; }
            if(0 <= bandAreaIndex){ algoConfigs[bandAreaIndex].number = personNum; }
            if(0 <= phonecallIndex){ algoConfigs[phonecallIndex].number = personNum; }
            if(0 <= peopleFellIndex){ algoConfigs[peopleFellIndex].number = personNum; }
            if(0 <= personCountIndex){ algoConfigs[personCountIndex].number = personNum; }
        }
    }
    //安全帽
    if( algoEnable(algoConfigs, helmetIndex) ||
        algoEnable(algoConfigs, crowdsIndex) )
    {
        if(0 != helmetCtx){
#if 0
            AlgoCfg_t algoCfg;
            if(0 <= helmetIndex){
                algoCfg = algoConfigs[helmetIndex];
                printf("helmet ==============>>\n");
                int areaSize = algoCfg.areas.size();
                for(int i = 0; i <areaSize; i++){
                    printf("area[%d]:\n",i);
                    int pointSize = algoCfg.areas[i].points.size();
                    for(int j = 0; j < pointSize; j++){
                        printf("(%d,%d)", algoCfg.areas[i].points[j].x, algoCfg.areas[i].points[j].y);
                    }
                    printf("\n");
                }
            }
#endif
            ret = helmet_detect_run(helmetCtx, imgSrc, &pResult->helmet_group);
            for (int i = 0; i < pResult->helmet_group.count; i++){
                helmetNum++;
            }
            if(0 <= helmetIndex){ algoConfigs[helmetIndex].number = helmetNum; }
            if(0 <= crowdsIndex){ algoConfigs[crowdsIndex].number = helmetNum; }
    	}
    }
    // 火焰算法分析
    if(algoEnable(algoConfigs, fireIndex)){
        if(0 != fireCtx){
            ret = fire_detect_run(fireCtx, imgSrc, &pResult->fire_group);
            for (int i = 0; i < pResult->fire_group.count; i++){
                fireNum++;
            }
        	if(0 <= fireIndex){ algoConfigs[fireIndex].number = fireNum; }
        }
    }
    // 车辆识别算法分析
    if(algoEnable(algoConfigs, carIndex)){
        if(0 != carCtx){
            ret = car_detect_run(carCtx, imgSrc, &pResult->car_group);       
            for (int i = 0; i < pResult->car_group.count; i++){
                carNum++;
            }
        	if(0 <= carIndex){ algoConfigs[carIndex].number = carNum; }
        }
    }
    //口罩
    if(algoEnable(algoConfigs, maskIndex)){
        if((0 != faceCtx) && (0 != maskjudgeCtx)){
            /* 人脸检测执行 */
            std::vector<det> detect_result;
            face_detect_run(faceCtx, imgSrc, detect_result);
#if 0
            pResult->mask_group.face_num = (int)detect_result.size(); 
            for(int i = 0; i < (int)detect_result.size(); i++) {
                Point2f points[5];
                for(int j = 0; j < (int)detect_result[i].landmarks.size(); ++j) {
                    points[j].x = (int)detect_result[i].landmarks[j].x;
                    points[j].y = (int)detect_result[i].landmarks[j].y;
                }
                Mat face_algin;
                face_algin = face_alignment(imgSrc, points);

                /* 人脸戴口罩判断运行 */
                float mask_result[2] = {0};
                face_mask_judgement_run(maskjudgeCtx, &face_algin, mask_result);
                pResult->mask_group.results[i].x = (int)(detect_result[i].box.x);
                pResult->mask_group.results[i].y = (int)(detect_result[i].box.y);
                pResult->mask_group.results[i].w = (int)(detect_result[i].box.width);
                pResult->mask_group.results[i].h = (int)(detect_result[i].box.height);
                pResult->mask_group.results[i].prop = mask_result[0];
                maskNum++;
            }
#endif
        	if(0 <= maskIndex){ algoConfigs[maskIndex].number = maskNum; }
        }
    }
    
    //无任何异常
    if((ret < 0 ) ||(
        (personNum <= 0) &&
	    (helmetNum <= 0) &&
        (fireNum <= 0) &&
        (maskNum <= 0) &&
        (carNum <= 0)
		)
    ){
        return -1;
    }
    PRINT_TRACE("\n>>>>>>>>>>>>>>>[Chn_%02d]>>>>>>>>>>>>>>>>", chnInfo.chnId);
    PRINT_TRACE("person number : %d", personNum);
    PRINT_TRACE("helmet number : %d", helmetNum);
    PRINT_TRACE("  fire number : %d", fireNum);
    PRINT_TRACE("  mask number : %d", maskNum);
    PRINT_TRACE("   car number : %d", carNum);

//=======================[算法结果：逻辑组合，告警过滤]================================
    FontColor color = {200, 255, 0, 0};    // {A, R, G, B};
    UploadInfo_t alarmInfo;
    std::stringstream ss; ss << "ID_" << std::setw(2) << std::setfill('0') << chnInfo.chnId;
    alarmInfo.alarmChnID = ss.str();
    alarmInfo.alarmChnName = chnInfo.chnName;
#if 0
    if(0 <= personCountIndex){
        if((0 < personNum) && pResult->algo[personCountIndex].enable){
            Mat alarmImg;
            imgSrc.copyTo(alarmImg);
            
            std::string strBirdRect; strBirdRect.clear();
            float peopleCount_confid = 0;
            int peopleCountNum = 0;
            
            for (int i = 0; i < pResult->person_group.count; i++) {
                person_detect_result_t *person_result = &(pResult->person_group.results[i]);
                if( (100 * person_result->prop) < (100 - pResult->algo[personCountIndex].sensibility) ){
                    continue;
                }
                s32Rect_t personRect;
                personRect.left = person_result->box.left;
                personRect.top = person_result->box.top;
                personRect.right = person_result->box.right;
                personRect.bottom = person_result->box.bottom;
                if(0 == strcmp(person_result->name, "person")){
                    // ========= 统计人数=========
                    if(pResult->algo[personCountIndex].enable){
                        if(person_result->prop > peopleCount_confid){//筛选最大置信度
                            peopleCount_confid = person_result->prop;     
                        }
                        peopleCountNum++;
                        char label_text[50];
                        memset(label_text, 0 , sizeof(label_text));
                        sprintf(label_text, "%s %0.2f", "person", person_result->prop); 
                        plot_one_box(alarmImg, personRect.left, personRect.top, personRect.right, personRect.bottom, label_text, 0);
                        strBirdRect.append(std::to_string(personRect.left));strBirdRect.append("|");
                        strBirdRect.append(std::to_string(personRect.top));strBirdRect.append("|");
                        strBirdRect.append(std::to_string(personRect.right));strBirdRect.append("|");
                        strBirdRect.append(std::to_string(personRect.bottom));strBirdRect.append("|");
                    }
                 }
            }
            // 上报[人数统计]告警
            if(pAlarmMgr && (0 < peopleCountNum)){
                char label_text[64];
                memset(label_text, 0 , sizeof(label_text));
                sprintf(label_text, "人数统计：%d",  peopleCountNum); 
                putText(alarmImg.data, alarmImg.cols, alarmImg.rows, label_text, 50, 60, color);
                
                if (!strBirdRect.empty()) { strBirdRect.pop_back();}
                
                alarmInfo.alarmType =  pResult->algo[personCountIndex].algoType;
                alarmInfo.alarmName =  pResult->algo[personCountIndex].alarmName;
                alarmInfo.alarmTag.number = peopleCountNum;
                pAlarmMgr->uploadAlarm(alarmImg, imgSrc, alarmInfo, pResult->algo[personCountIndex].uploadFrq);
            }
        }
    }
#endif
    // ========= 在图像中标记 [未戴安全帽的头] =========
    if((0 < helmetNum) && algoEnable(algoConfigs, helmetIndex)){
        Mat alarmImg;
        imgSrc.copyTo(alarmImg);
        
        alarmInfo.alarmTag.number = 0;
        std::string strRect;
        std::vector<std::string>().swap(alarmInfo.alarmTag.vec_rect);
        std::vector<std::string>().swap(alarmInfo.alarmTag.vec_confidence);
        for (int i = 0; i < algoConfigs[helmetIndex].number; i++) {
            detect_result_t *det_result = &(pResult->helmet_group.results[i]);
            if( (100 * det_result->prop) < (100 - algoConfigs[helmetIndex].sensibility) ){
                continue;
            }

            if(0 == strcmp(det_result->name, "head")){
                strRect.clear();
                strRect += "{";
                strRect += std::to_string(det_result->box.left);  strRect += ",";
                strRect += std::to_string(det_result->box.top);   strRect += ",";
                strRect += std::to_string(det_result->box.right); strRect += ",";
                strRect += std::to_string(det_result->box.bottom);
                strRect += "}";
                alarmInfo.alarmTag.vec_rect.push_back(strRect);
                alarmInfo.alarmTag.vec_confidence.push_back(std::to_string(det_result->prop));
                alarmInfo.alarmTag.number++;         
                PRINT_TRACE("%s @ %s %s", det_result->name, strRect.c_str(), std::to_string(det_result->prop).c_str());
                
                char label_text[64];
                memset(label_text, 0 , sizeof(label_text));
                sprintf(label_text, "%s %0.2f",det_result->name, det_result->prop); 
                plot_one_box(alarmImg, det_result->box.left, det_result->box.top, det_result->box.right, det_result->box.bottom, label_text,0);
            }
        }
        
        if(pAlarmMgr && (0 < alarmInfo.alarmTag.number)){
            alarmInfo.alarmType =  algoConfigs[helmetIndex].algoType;
            alarmInfo.alarmName =  algoConfigs[helmetIndex].alarmName;
            pAlarmMgr->uploadAlarm(alarmImg, imgSrc, alarmInfo, algoConfigs[helmetIndex].uploadFrq);
        }
    }
    // ========= 在图像中标记 [未戴口罩的脸] =========
    if((0 < maskNum) && algoEnable(algoConfigs, maskIndex)){
        Mat alarmImg;
        imgSrc.copyTo(alarmImg);
    
        float mask_confid = 0;
        int targetNum = 0;
#if 0
        for (int i = 0; i < algoConfigs[maskIndex].number; i++) {
            mask_detect_result_t *det_result = &(pResult->mask_group.results[i]);
            if((100 * det_result->prop) < (100 - algoConfigs[maskIndex].sensibility)){
                continue;
            }
            if(det_result->prop > mask_confid){//筛选最大置信度
                mask_confid = det_result->prop;       
            }
            targetNum++;
            
            char label_text[50];
            memset(label_text, 0 , sizeof(label_text));
            sprintf(label_text, "%s %0.2f", "NoMask", det_result->prop); 
            plot_one_box(alarmImg, det_result->x, det_result->y, det_result->x + det_result->w, det_result->y + det_result->h, label_text, 0);
        }
#endif
        if(pAlarmMgr&&(0 < targetNum)){
            alarmInfo.alarmType =  algoConfigs[maskIndex].algoType;
            alarmInfo.alarmName =  algoConfigs[maskIndex].alarmName;
            alarmInfo.alarmTag.number = targetNum;
            pAlarmMgr->uploadAlarm(alarmImg, imgSrc, alarmInfo, algoConfigs[maskIndex].uploadFrq);
        }
    }
    // ========= 在图像中标记 [明火] =========
    if((0 < fireNum) && algoEnable(algoConfigs, fireIndex)){
        Mat alarmImg;
        imgSrc.copyTo(alarmImg);
        
        float fire_confid = 0;
        int targetNum = 0;
        for(int i = 0; i < pResult->fire_group.count; i++) {
            detect_result_t *det_result = &(pResult->fire_group.results[i]);
            if( (100 * det_result->prop) < (100 - algoConfigs[fireIndex].sensibility) ){
                continue;
            }
            if(0 == strcmp(det_result->name, "fire")){
                if(det_result->prop > fire_confid){//筛选最大置信度
                    fire_confid = det_result->prop;       
                }
                targetNum++;
                PRINT_TRACE("%s @ (%d %d %d %d) %f",
                        det_result->name,
                        det_result->box.left, det_result->box.top, det_result->box.right, det_result->box.bottom,
                        det_result->prop);
                
                char label_text[50];
                memset(label_text, 0 , sizeof(label_text));
                sprintf(label_text, "%s %0.2f", det_result->name, det_result->prop); 
                plot_one_box(alarmImg, det_result->box.left, det_result->box.top, det_result->box.right, det_result->box.bottom, label_text, 0);
            }
        }
        
        if(pAlarmMgr && (0 < targetNum)){
            alarmInfo.alarmType =  algoConfigs[fireIndex].algoType;
            alarmInfo.alarmName =  algoConfigs[fireIndex].alarmName;
            alarmInfo.alarmTag.number = targetNum;
            pAlarmMgr->uploadAlarm(alarmImg, imgSrc, alarmInfo, algoConfigs[fireIndex].uploadFrq);
        }
    }
    // ========= 在图像中标记 [车辆] =========
    if((0 < carNum) && algoEnable(algoConfigs, carIndex)){
        Mat alarmImg;
        imgSrc.copyTo(alarmImg);
        
        float max_confid = 0;
        int targetNum = 0;
        for(int i = 0; i < pResult->car_group.count; i++) {
            detect_result_t *det_result = &(pResult->car_group.results[i]);
            if( (100 * det_result->prop) < 50 ){
                continue;
            }
            if(0 == strcmp(det_result->name, "car")){
                if(det_result->prop > max_confid){//筛选最大置信度
                    max_confid = det_result->prop;       
                }
                targetNum++;
                PRINT_TRACE("%s @ (%d %d %d %d) %f",
                        det_result->name,
                        det_result->box.left, det_result->box.top, det_result->box.right, det_result->box.bottom,
                        det_result->prop);
                
                char label_text[50];
                memset(label_text, 0 , sizeof(label_text));
                sprintf(label_text, "%s %0.2f", det_result->name, det_result->prop); 
                plot_one_box(alarmImg, det_result->box.left, det_result->box.top, det_result->box.right, det_result->box.bottom, label_text, 0);
            }
        }
        
        if(pAlarmMgr && (algoConfigs[carIndex].sensibility < targetNum)){
            alarmInfo.alarmType =  algoConfigs[carIndex].algoType;
            alarmInfo.alarmName =  algoConfigs[carIndex].alarmName;
            alarmInfo.alarmTag.number = targetNum;
            pAlarmMgr->uploadAlarm(alarmImg, imgSrc, alarmInfo, algoConfigs[carIndex].uploadFrq);
        }
    }
    // ========= [聚众识别] =========
    if((0 <= crowdsIndex) && (0 <= helmetIndex)){
        if((0 < helmetNum) && algoEnable(algoConfigs, crowdsIndex)){
            Mat alarmImg;
            imgSrc.copyTo(alarmImg);
        
            std::vector<detect_result_t> vecPerson;
            float person_confid = 0;
            int crowds_intersect_count = 0;
            double x1,y1,x2,y2;
            int len = 0;
            float interval = (float)(algoConfigs[crowdsIndex].sensibility)/5;
            for(int i = 0; i < pResult->helmet_group.count; i++) {
                detect_result_t *person_result_1 = &(pResult->helmet_group.results[i]);
                if( (100 * person_result_1->prop) < (100 - algoConfigs[helmetIndex].sensibility) ){
                    continue;
                }
                if(person_result_1->prop > person_confid){//筛选最大置信度
                    person_confid = person_result_1->prop;
                }
                
                if(0 == strcmp(person_result_1->name, "head")){
                    s32Rect_t personRect_1;
                    personRect_1.left   = person_result_1->box.left;
                    personRect_1.top    = person_result_1->box.top;
                    personRect_1.right  = person_result_1->box.right;
                    personRect_1.bottom = person_result_1->box.bottom;
                    x1 = personRect_1.left + (personRect_1.right  - personRect_1.left)/2 ;
                    y1 = personRect_1.top  + (personRect_1.bottom - personRect_1.top)/2 ;
                    for(int j = 0; j < pResult->helmet_group.count; j++) {
                        if(i != j) {
                            detect_result_t person_result_2 = pResult->helmet_group.results[j];
                            if( (100 * person_result_2.prop) < (100 - algoConfigs[helmetIndex].sensibility)){
                                continue;
                            }
                            if(0 == strcmp(person_result_2.name, "head")){
                                s32Rect_t personRect_2;
                                personRect_2.left   = person_result_2.box.left;
                                personRect_2.top    = person_result_2.box.top;
                                personRect_2.right  = person_result_2.box.right;
                                personRect_2.bottom = person_result_2.box.bottom;
                                x2 = personRect_2.left + (personRect_2.right  - personRect_2.left)/2 ;
                                y2 = personRect_2.top  + (personRect_2.bottom - personRect_2.top)/2 ;      
                                len = sqrt_quick(((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2)));
                                // 距离为目标宽度的3倍以内
                                //printf("===len:%d , interval:%.2f , interval*w:%.2f===\n",len,interval,((float)(personRect_1.right -  personRect_1.left)*interval));
                                if(len < ((float)(personRect_1.right - personRect_1.left)*interval)) {
                                    auto iter = std::find_if(vecPerson.begin(), vecPerson.end(), [&](const detect_result_t& obj)->bool
                                    { return (obj.box.left == person_result_2.box.left &&
                                            obj.box.right == person_result_2.box.right&&
                                            obj.box.top == person_result_2.box.top &&
                                            obj.box.bottom ==  person_result_2.box.bottom );
                                    });
                                    if (iter == vecPerson.end()) {
                                        vecPerson.push_back(person_result_2);
                                        crowds_intersect_count++;
                                        char label_text[50];
                                        memset(label_text, 0 , sizeof(label_text));
                                        sprintf(label_text, "%s %0.2f", person_result_2.name, person_result_2.prop); 
                                        plot_one_box(alarmImg, person_result_2.box.left, person_result_2.box.top, person_result_2.box.right, person_result_2.box.bottom, label_text,0);
                                    }
                                }
                            }        
                        }
                    }
                }
            }
            int i = 0;
            while(!vecPerson.empty())
            {
                i++;
                detect_result_t person_result = vecPerson.back();
                char label_text[50];
                memset(label_text, 0 , sizeof(label_text));
                sprintf(label_text, "%s %0.2f", person_result.name, person_result.prop); 
                plot_one_box(alarmImg, person_result.box.left, person_result.box.top, person_result.box.right, person_result.box.bottom, label_text,0);
                vecPerson.pop_back();
            }
            // 总人数大于等于3， (相交的次数/总人数) > 2
            if((pResult->helmet_group.count >= 3) && (crowds_intersect_count > 3)) {
                if(pAlarmMgr) {
                    alarmInfo.alarmType =  algoConfigs[crowdsIndex].algoType;
                    alarmInfo.alarmName =  algoConfigs[crowdsIndex].alarmName;
                    alarmInfo.alarmTag.number = crowds_intersect_count;
                    pAlarmMgr->uploadAlarm(alarmImg, imgSrc, alarmInfo, algoConfigs[crowdsIndex].uploadFrq);
                }
            }
        }
    }
            
    float bandArea_confid = 0;
    float peopleFell_confid = 0;
    float peopleCount_confid = 0;
    float smoking_confid = 0;
    float phonecall_confid = 0;
    if(0 < personNum){
        Mat bandAreaImg;
        int bandAreaNum = 0;
        if(algoEnable(algoConfigs, bandAreaIndex)){
            imgSrc.copyTo(bandAreaImg);
        }
        Mat peopleFellImg;
        int peopleFellNum = 0;
        if(algoEnable(algoConfigs, peopleFellIndex)){
            imgSrc.copyTo(peopleFellImg);
        }
        Mat peopleCountImg;
        int peopleCountNum = 0;
        if(algoEnable(algoConfigs, personCountIndex)){
            imgSrc.copyTo(peopleCountImg);
        }
        Mat TakeOutPersonImg;
        Mat TakeOutPersonImg_result;
        if(algoEnable(algoConfigs, smokingIndex) || algoEnable(algoConfigs, phonecallIndex)){
            imgSrc.copyTo(TakeOutPersonImg);
        }
        Mat smokingImg;
        Mat smokingImg_result;
        int smokingNum = 0;
        if(algoEnable(algoConfigs, smokingIndex)){
            imgSrc.copyTo(smokingImg);
        }
        Mat phonecallImg;
        Mat phonecall_result;
        int phonecallNum = 0;
        if(algoEnable(algoConfigs, phonecallIndex)){
            imgSrc.copyTo(phonecallImg);
        }
        
        // 做多边形识别用
        std::vector<std::vector<cv::Point>> regions;
        regions.clear();
        for (int i = 0; i < pResult->person_group.count; i++) {
            detect_result_t *person_result = &(pResult->person_group.results[i]);
            if( (100 * person_result->prop) < 60 ){
                continue;
            }
            s32Rect_t personRect;
            personRect.left = person_result->box.left;
            personRect.top = person_result->box.top;
            personRect.right = person_result->box.right;
            personRect.bottom = person_result->box.bottom;
            //人员坐标存入vector,用于后面扣图
            std::vector<cv::Point> roi = { Point(personRect.left, personRect.top),
                                           Point(personRect.right, personRect.top),
                                           Point(personRect.right, personRect.bottom),
                                           Point(personRect.left, personRect.bottom) };
            regions.insert(regions.end(), roi);

            if(0 == strcmp(person_result->name, "person")){
                // ========= 在图像中标记 [闯入禁区的人] =========
                if(algoEnable(algoConfigs, bandAreaIndex)){
                    uint32_t j;
                    for (j = 0; j < algoConfigs[bandAreaIndex].areas.size(); j++) {
                        s32Rect_t area;
                        area.left   = algoConfigs[bandAreaIndex].areas[j].points[0].x;
                        area.top    = algoConfigs[bandAreaIndex].areas[j].points[0].y;
                        area.right  = algoConfigs[bandAreaIndex].areas[j].points[2].x;
                        area.bottom = algoConfigs[bandAreaIndex].areas[j].points[2].y;
                        double iofRatio = calc_intersect_of_min_rect(area, personRect);
                         // 人员侵入禁区比例
                        if((int32_t)(iofRatio*100) > (100 - algoConfigs[bandAreaIndex].sensibility)){
                            PRINT_TRACE("intersect of min rect ratio is %lf %%", iofRatio*100);
                            break;
                        }
                    }
                    if((0 != algoConfigs[bandAreaIndex].areas.size())&&(j < algoConfigs[bandAreaIndex].areas.size())){
                        // 人员与某一禁区重叠
                        bandAreaNum++;
                        if(person_result->prop > bandArea_confid){//筛选最大置信度
                            bandArea_confid = person_result->prop;       
                        }
                        // 标记人员
                        PRINT_TRACE("%s @ (%d %d %d %d) %f",
                                person_result->name,
                                person_result->box.left, person_result->box.top, person_result->box.right, person_result->box.bottom,
                                person_result->prop);
                        char label_text[50];
                        memset(label_text, 0 , sizeof(label_text));
                        sprintf(label_text, "%s %0.2f", person_result->name, person_result->prop); 
                        plot_one_box(bandAreaImg, person_result->box.left, person_result->box.top, person_result->box.right, person_result->box.bottom, label_text,0);
                        
                        // 标记禁区
                        for (uint32_t index = 0; index < algoConfigs[bandAreaIndex].areas.size(); index++) {
                            s32Rect_t area;
                            area.left   = algoConfigs[bandAreaIndex].areas[index].points[0].x;
                            area.top    = algoConfigs[bandAreaIndex].areas[index].points[0].y;
                            area.right  = algoConfigs[bandAreaIndex].areas[index].points[2].x;
                            area.bottom = algoConfigs[bandAreaIndex].areas[index].points[2].y;
                            memset(label_text, 0 , sizeof(label_text));
                            sprintf(label_text, "area[%02d]", index); 
                            plot_one_box(bandAreaImg, area.left, area.top, area.right, area.bottom, label_text, 2);
                        }
                    }
                }
				
				// ========= 在图像中标记 [摔倒的人] =========
                if(algoEnable(algoConfigs, peopleFellIndex)){
                    if(person_result->prop > peopleFell_confid){//筛选最大置信度
                        peopleFell_confid = person_result->prop;       
                    }
					int x1 = person_result->box.left;
					int y1 = person_result->box.top;
					int x2 = person_result->box.right;
					int y2 = person_result->box.bottom;
					float fall_scale = 0;
					fall_scale = (float)(y2-y1) / (x2-x1);

					if( fall_scale < 0.8) {
					    peopleFellNum++;
                        char label_text[50];
                        memset(label_text, 0 , sizeof(label_text));
                        sprintf(label_text, "%s %0.2f", person_result->name, person_result->prop); 
                        plot_one_box(peopleFellImg, person_result->box.left, person_result->box.top, person_result->box.right, person_result->box.bottom, label_text,0);
						//printf("Falling people detected!\n");
					}
                }
                
                // ========= 统计人数=========
                if(algoEnable(algoConfigs, personCountIndex)){
                    if(person_result->prop > peopleCount_confid){//筛选最大置信度
                        peopleCount_confid = person_result->prop;     
                    }
                    peopleCountNum++;
                    char label_text[50];
                    memset(label_text, 0 , sizeof(label_text));
                    sprintf(label_text, "%s %0.2f", "person", person_result->prop); 
                    plot_one_box(peopleCountImg, person_result->box.left, person_result->box.top, person_result->box.right, person_result->box.bottom, label_text,0);
                }
            }
        }
        
        //扣图
        if(algoEnable(algoConfigs, smokingIndex) || algoEnable(algoConfigs, phonecallIndex)){
            // 创建一个黑色的遮盖层
            cv::Mat mask = cv::Mat::zeros(TakeOutPersonImg.size(), CV_8UC1);
            // 用多边形把 ROI 以外的部分抹黑
            for (const auto &v : regions) {
                std::vector<std::vector<cv::Point> > p = {v};
                cv::fillPoly(mask, p, cv::Scalar(255, 255, 255));
            }
            // 利用遮盖层将ROI以外的区域抹黑
            cv::bitwise_and(TakeOutPersonImg, TakeOutPersonImg, TakeOutPersonImg_result, mask);
#if 0
            // =========抽烟检测=========
            if(algoEnable(algoConfigs, smokingIndex)){
                if(0 != smokingCtx){
                    smoke_detect_run(smokingCtx, TakeOutPersonImg_result, &pResult->smoking_group);
                    /* 算法结果在图像中画出并保存 */
                    for (int i = 0; i < pResult->smoking_group.count; i++) {
                        smoke_detect_result_t *smoking_result_1 = &(pResult->smoking_group.results[i]);
                        if( (100 * smoking_result_1->prop) < (100 - algoConfigs[smokingIndex].sensibility) ){
                            continue;
                        }
                        smokingNum++;
                        if(smoking_result_1->prop > smoking_confid){//筛选最大置信度
                            smoking_confid = smoking_result_1->prop;       
                        }
                        char label_text[50];
                        memset(label_text, 0 , sizeof(label_text));
                        sprintf(label_text, "%s %0.2f", "smoking", smoking_result_1->prop); 
                        plot_one_box(smokingImg, smoking_result_1->box.left, smoking_result_1->box.top, smoking_result_1->box.right, smoking_result_1->box.bottom, label_text,0);
                    }
                }
            }
            // =========打电话检测=========
            if(algoEnable(algoConfigs, phonecallIndex)){
                if(0 != phonecallCtx){
                    phonecall_detect_run(phonecallCtx, TakeOutPersonImg_result, &pResult->phonecall_group);
                    /* 算法结果在图像中画出并保存 */
                    for (int i = 0; i < pResult->phonecall_group.count; i++) 
                    {
                        phonecall_detect_result_t *phonecall_result_1 = &(pResult->phonecall_group.results[i]);
                        if( (100 * phonecall_result_1->prop) < (100 - algoConfigs[phonecallIndex].sensibility) ){
                            continue;
                        }
                        phonecallNum++;
                        if(phonecall_result_1->prop > phonecall_confid){//筛选最大置信度
                            phonecall_confid = phonecall_result_1->prop;       
                        }
                        char label_text[50];
                        memset(label_text, 0 , sizeof(label_text));
                        sprintf(label_text, "%s %0.2f", "phonecall", phonecall_result_1->prop); 
                        plot_one_box(phonecallImg, phonecall_result_1->box.left, phonecall_result_1->box.top, phonecall_result_1->box.right, phonecall_result_1->box.bottom, label_text,0);
                    }
                }
            }
#endif
        }

       // 上报[打电话]告警
        if(pAlarmMgr&&(0 < phonecallNum)){
            alarmInfo.alarmType =  algoConfigs[phonecallIndex].algoType;
            alarmInfo.alarmName =  algoConfigs[phonecallIndex].alarmName;
            alarmInfo.alarmTag.number = phonecallNum;
            pAlarmMgr->uploadAlarm(phonecallImg, imgSrc, alarmInfo, algoConfigs[phonecallIndex].uploadFrq);
        }

        // 上报[抽烟]告警
        if(pAlarmMgr&&(0 < smokingNum)){
            alarmInfo.alarmType =  algoConfigs[smokingIndex].algoType;
            alarmInfo.alarmName =  algoConfigs[smokingIndex].alarmName;
            alarmInfo.alarmTag.number = smokingNum;
            pAlarmMgr->uploadAlarm(smokingImg, imgSrc, alarmInfo, algoConfigs[smokingIndex].uploadFrq);
        }
        // 上报[闯入禁区]告警
        if(pAlarmMgr&&(0 < bandAreaNum)){
            alarmInfo.alarmType =  algoConfigs[bandAreaIndex].algoType;
            alarmInfo.alarmName =  algoConfigs[bandAreaIndex].alarmName;
            alarmInfo.alarmTag.number = bandAreaNum;
            pAlarmMgr->uploadAlarm(bandAreaImg, imgSrc, alarmInfo, algoConfigs[bandAreaIndex].uploadFrq);
        }
        // 上报[人员摔倒]告警
        if(pAlarmMgr&&(0 < peopleFellNum)){
            alarmInfo.alarmType =  algoConfigs[peopleFellIndex].algoType;
            alarmInfo.alarmName =  algoConfigs[peopleFellIndex].alarmName;
            alarmInfo.alarmTag.number = peopleFellNum;
            pAlarmMgr->uploadAlarm(peopleFellImg, imgSrc, alarmInfo, algoConfigs[peopleFellIndex].uploadFrq);
        }
        // 上报[人数统计]告警
        if(pAlarmMgr&&(0 < peopleCountNum)){
            char label_text[64];
            memset(label_text, 0 , sizeof(label_text));
            sprintf(label_text, "人数统计：%d",  peopleCountNum); 
            //plot_one_box(peopleCountImg, 100, 100, 300, 300, label_text ,0);
            putText(peopleCountImg.data, peopleCountImg.cols, peopleCountImg.rows, label_text, 50, 60, color);
            
            alarmInfo.alarmType =  algoConfigs[personCountIndex].algoType;
            alarmInfo.alarmName =  algoConfigs[personCountIndex].alarmName;
            alarmInfo.alarmTag.number = peopleCountNum;
            pAlarmMgr->uploadAlarm(peopleCountImg, imgSrc, alarmInfo, algoConfigs[personCountIndex].uploadFrq);
        }
    }
    
    return 0;
}

