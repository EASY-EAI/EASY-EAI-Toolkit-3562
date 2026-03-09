#ifndef __DB_ALARMRECORD_H__
#define __DB_ALARMRECORD_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <vector>


/*
 * 本解决方案仅使用，一个数据库，一张表
 *
 * 一个数据库可以有多张表，结构如下：
 *
 *     xxxx.db           ---> 数据库文件
 *      ┣━ xxx1_table    ---> 表1
 *      ┣━ xxx1_table    ---> 表2
 *      ┣━ ……
 *      ┗━ xxxn_table    ---> 表n
 */
#define DATABASE_PATH  "./Alarm/AlarmRecord.db"
#define ALARM_TABLE      "AlarmRecords"
#define CTAD_TABLE       "CTAD"
#define ORIGINAL_TABLE   "OriginalRecords"
#define ATYPE_TABLE      "AlarmType"


/* 设备可容纳的总告警记录数 */
#define MAX_ALARM_NUM 10000

/* 告警记录 Index 长度 */
#define INDEX_LEN 8
/* 告警记录类型 */
#define TYPE_LEN 16
/* 告警记录描述长度 */
#define DESC_LEN 64
/* 告警记录通道ID长度 */
#define CHNID_LEN 8
/* 告警记录通道名长度 */
#define CHNNAME_LEN 64
/* 告警记录文件名长度 */
#define PHOTONAME_LEN 128
/* 告警记录存储方式长度 */
#define STORAGEDEV_LEN 16
/* 告警记录发生的时间戳长度 */
#define ALARMTIME_LEN 32
/* 告警记录置信度长度 */
#define CUSTOMMSG_LEN 2048
/*
 * 该结构体用于同步数据库信息到内存上，
 * 避免特征比对时频繁操作数据库，以提升比对性能。
 * 注意：此结构体不能作任何改动(比如增加成员变量等)
 */
typedef struct {
    char recIndex[INDEX_LEN];
    char recType[TYPE_LEN];
    char recDesc[DESC_LEN];
    char recChnID[CHNID_LEN];
    char recChnName[CHNNAME_LEN];
    char recStorageDev[STORAGEDEV_LEN];
    char recPhotoName[PHOTONAME_LEN];
    char recSrcPhotoName[PHOTONAME_LEN];
    char recTime[ALARMTIME_LEN];
    char recCustomMsg[CUSTOMMSG_LEN];
}AlarmRecItem_t;



/*
*该结构体用于获取数据库中的Desc,AlarmType(人数统计|peopleCount)
*用户告警记录查询时的type
*/
typedef struct 
{
    char desc[64];
    char alarmType[64];
}AlarmInfo_t;


/* 
 * 数据库基本操作：
 *     1.初始化
 *     2.退出
 *     3.重置数据库(不删除数据)
 *     4.删除数据库(并删除所有记录)
 */
extern int  database_init(void);
extern void database_exit(void);
extern void database_reset(void);
extern int  database_delete(void);


/* AlarmRecords表格操作：
 * 
 */
// 凭筛选条件,读出数据库中符合条件的记录数量
extern int alarmRecords_recCount(const char *recType, const char *recChnId, const char *recStartTime, const char *recEndTime);
// 凭筛选条件,删除所有符合条件的记录
extern int alarmRecords_delRecord(const char *recType, const char *recChnId, const char *recStartTime, const char *recEndTime,int32_t dataNum, bool sync_flag);
// 向告警记录表插入一条记录
extern int alarmRecords_addRecord(const char *recType, const char *recDesc, const char *recChnID, const char *recChnName, const char *recStorageDev, const char *recPhotoName, const char *recSrcPhotoName, const char *recTime);
// 凭筛选条件,把数据库中合条件的记录同步到 AlarmRecItem_t 数组中
extern int alarmRecords_getRecord(AlarmRecItem_t *pItem, int32_t recNum, const char *recType, const char *recChnId, const char *recStartTime, const char *recEndTime);
//提取第一行PhotoPath
extern int alarmRecords_Minimum_Photo(char *pthoto_name, char *Srcpthoto_name);
extern int alarmRecords_delet(const char *photoname);
//统计数据库中数据的(Desc, type)的种类
extern int alarmRecords_AcquiredDataType(std::vector<AlarmInfo_t> & algo_dev);




/* CTAD表格操作：
 * 
 */
// 判断此名字是否存在于数据库
extern bool database_name_is_exist(const char *name);
// 判断此 Id 是否存在于数据库
extern bool database_id_is_exist(const char *id, char *name, size_t size);
// 向断网续传表插入一条记录
extern int add_CTAD_record(const char *recType, const char *recDesc, const char *recChnID, const char *recChnName, const char *recPhotoName, const char *recTime, int recNum, const char *recCustomMsg);
// 删除制定ID的数据
extern int delet_CTAD_record(int Id);
//查询CTAD表记录数
extern int CTAD_record_number();
//根据Id，在CTAD中提取数据
extern int CTAD_extraction_data(int Id, char *recType, char *recDesc, char *recChnID, char *recChnName, char *recPhotoName, char *recTime, int *recAlarmNum, char *recCustomMsg);
//提取CTAD表最小ID
extern int CTAD_Minimum_ID();



/* Original表格操作：
 * 
 */
// 凭筛选条件,读出数据库中符合条件的记录数量
extern int originalRecords_recCount(const char *recType, const char *recChnId, const char *recStartTime, const char *recEndTime);
// 凭筛选条件,删除所有符合条件的记录
extern int originalRecords_delRecord(const char *recType, const char *recChnId, const char *recStartTime, const char *recEndTime, int32_t dataNum, bool sync_flag);
// 向告警记录表插入一条记录
extern int originalRecords_addRecord(const char *recType, const char *recDesc, const char *recChnID, const char *recChnName, const char *recConfidence, const char *recPhotoName, const char *recTime);
// 凭筛选条件,把数据库中合条件的记录同步到 AlarmRecItem_t 数组中
extern int originalRecords_getRecord(AlarmRecItem_t *pItem, int32_t recNum, const char *recType, const char *recChnId, const char *recStartTime, const char *recEndTime);
//提取第一行PhotoPath
extern int originalRecords_Minimum_Photo(char *pthoto_name);
extern int originalRecords_delet(const char* photoname);







#endif

