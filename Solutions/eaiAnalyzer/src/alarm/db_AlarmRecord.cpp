#include "appSystem.h"
#include "sysIPCData.h"
#include <sqlite3.h>

#include "log_manager.h"

#include "db_AlarmRecord.h"


static sqlite3 *g_db = NULL;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

#if 0
// COL: column
#define COL_ADESC     "Desc"
#define COL_ATYPE     "AlarmType"
#define COL_ACHNID    "ChannelId"
#define COL_ACHNNAME  "ChannelName"
#define COL_STORAGE   "StorageDev"
#define COL_PHOTO     "PhotoPath"
#define COL_SRCPHOTO  "SrcPhotoPath"
#define COL_TIMESTAMP "TimeStamp"
#define COL_CUSTOMMSG "CustomMsg"
#endif

static int32_t createAlarmTable()
{
    char *err;
    char cmd[512] = {0};
    char column[512] = {0};

    // === 表 ======================================================================================================
    snprintf(cmd + strlen(cmd), sizeof(cmd) - strlen(cmd),"CREATE TABLE IF NOT EXISTS %s ", ALARM_TABLE);
    // === 列数据 ============================[入列时顺序要一致]====================================================
    snprintf(column + strlen(column), sizeof(column) - strlen(column), "Desc varchar(%d), ",         DESC_LEN);
    snprintf(column + strlen(column), sizeof(column) - strlen(column), "AlarmType varchar(%d), ",    TYPE_LEN);
    snprintf(column + strlen(column), sizeof(column) - strlen(column), "ChannelId varchar(%d), ",    CHNID_LEN);
    snprintf(column + strlen(column), sizeof(column) - strlen(column), "ChannelName varchar(%d), ",  CHNNAME_LEN);
    snprintf(column + strlen(column), sizeof(column) - strlen(column), "StorageDev varchar(%d), ",   STORAGEDEV_LEN);
    snprintf(column + strlen(column), sizeof(column) - strlen(column), "PhotoPath varchar(%d), ",    PHOTONAME_LEN);
    snprintf(column + strlen(column), sizeof(column) - strlen(column), "SrcPhotoPath varchar(%d), ", PHOTONAME_LEN);
    snprintf(column + strlen(column), sizeof(column) - strlen(column), "TimeStamp varchar(%d)",    ALARMTIME_LEN);
    
    snprintf(cmd + strlen(cmd), sizeof(cmd) - strlen(cmd), "(Id INTEGER PRIMARY KEY AUTOINCREMENT, %s)", column);

    printf("cmd:[%s]\n", cmd);
    if (sqlite3_exec(g_db, cmd, 0, 0, &err) != SQLITE_OK) {
        return -1;
    }

    return 0;
}

#if 0
static int32_t createOriginalTable()
{
    char *err;
    char cmd[512] = {0};
    
    snprintf(cmd, sizeof(cmd),
             "CREATE TABLE IF NOT EXISTS %s (Id INTEGER PRIMARY KEY AUTOINCREMENT, Desc varchar(%d), AlarmType varchar(%d), ChannelId varchar(%d), ChannelName varchar(%d), Confidence varchar(%d),PhotoPath varchar(%d), TimeStamp varchar(%d))",
             ORIGINAL_TABLE, DESC_LEN, TYPE_LEN, CHNID_LEN, CHNNAME_LEN, CONFIDENCE_LEN, PHOTONAME_LEN, ALARMTIME_LEN);

    if (sqlite3_exec(g_db, cmd, 0, 0, &err) != SQLITE_OK) {
        return -1;
    }

    return 0;
}
#endif



static int32_t createCTADTable()
{
    char *err;
    char cmd[512] = {0};
    char column[512] = {0};
    
    // === 表 ======================================================================================================
    snprintf(cmd + strlen(cmd), sizeof(cmd) - strlen(cmd),"CREATE TABLE IF NOT EXISTS %s ", CTAD_TABLE);
    // === 列数据 ============================[入列时顺序要一致]====================================================
    snprintf(column + strlen(column), sizeof(column) - strlen(column), "Desc varchar(%d), ",        DESC_LEN);
    snprintf(column + strlen(column), sizeof(column) - strlen(column), "AlarmType varchar(%d), ",   TYPE_LEN);
    snprintf(column + strlen(column), sizeof(column) - strlen(column), "ChannelId varchar(%d), ",   CHNID_LEN);
    snprintf(column + strlen(column), sizeof(column) - strlen(column), "ChannelName varchar(%d), ", CHNNAME_LEN);
    snprintf(column + strlen(column), sizeof(column) - strlen(column), "PhotoPath varchar(%d), ",   PHOTONAME_LEN);
    snprintf(column + strlen(column), sizeof(column) - strlen(column), "TimeStamp varchar(%d), ",   ALARMTIME_LEN);
    snprintf(column + strlen(column), sizeof(column) - strlen(column), "AlarmNum INT NOT NULL, ");
    snprintf(column + strlen(column), sizeof(column) - strlen(column), "CustomMsg varchar(%d)",     CUSTOMMSG_LEN);
    
    snprintf(cmd + strlen(cmd), sizeof(cmd) - strlen(cmd), "(Id INTEGER PRIMARY KEY AUTOINCREMENT, %s)", column);
    
    if (sqlite3_exec(g_db, cmd, 0, 0, &err) != SQLITE_OK) {
        return -1;
    }

    return 0;
}

#if 0
static int32_t createTypeListTable()
{
    char *err;
    char cmd[512] = {0};
    
    snprintf(cmd, sizeof(cmd),
             "CREATE TABLE IF NOT EXISTS %s (Id INTEGER PRIMARY KEY AUTOINCREMENT, AlarmType varchar(%d), AlarmName varchar(%d))",
             ATYPE_TABLE, TYPE_LEN, DESC_LEN);

    if (sqlite3_exec(g_db, cmd, 0, 0, &err) != SQLITE_OK) {
        return -1;
    }

    return 0;
}
#endif

/* ==========================================================================
 * 初始化数据库
 *     1.打开数据库文件
 */
int database_init(void)
{
    if (sqlite3_open(DATABASE_PATH, &g_db) != SQLITE_OK) {
        PRINT_ERROR("open database %s failed!", DATABASE_PATH);
        return -1;
    }

    if (0 != createAlarmTable()){
        PRINT_ERROR("create table %s failed!", ALARM_TABLE);
        goto errExit;
    }

    if (0 != createCTADTable()){
        PRINT_ERROR("create table %s failed!", CTAD_TABLE);
        goto errExit;
    }

    // if (0 != createOriginalTable()){
    //     PRINT_ERROR("create table %s failed!", ORIGINAL_TABLE);
    //     goto errExit;
    // }
    // if (0 != createTypeListTable()){
    //     PRINT_ERROR("create table %s failed!", ATYPE_TABLE);
    //     goto errExit;
    // }
    
	PRINT_DEBUG("database_init OK");
    return 0;
    
errExit:
    sqlite3_close(g_db);
    g_db = NULL;
    return -1;
}

/* ==========================================================================
 * 退出数据库
 *     1.关闭数据库文件
 */
void database_exit(void)
{
    sqlite3_close(g_db);
    g_db = NULL;    
}

/* ==========================================================================
 * 重置数据库
 *     1.关闭数据库文件
 *     2.打开数据库文件
 */
void database_reset(void)
{
    pthread_mutex_lock(&g_mutex);
    database_exit();
    unlink(DATABASE_PATH);
    database_init();
    pthread_mutex_unlock(&g_mutex);
}

/* ==========================================================================
 * 删除所有记录
 */
int database_delete(void)
{
    // 关闭数据库文件
    database_exit();
    if(NULL != g_db){
        PRINT_ERROR("database close error");
        return -1;
    }

    // 删除整个数据库
    char sys_cmd[256] = {0};
    sprintf(sys_cmd,"rm %s", DATABASE_PATH);
    system(sys_cmd);

    // 重新打开数据库
    database_init();
    if(NULL == g_db){
        PRINT_ERROR("database open error");
        return -1;
    }
    return 0;
}

static bool makeFilter(char *cmd, size_t cmdLen, const char *table, const char *recType, const char *recChnId, const char *recStartTime, const char *recEndTime)
{
    char tempStr[64] = {0};
    snprintf(tempStr, sizeof(tempStr),"t.* FROM %s t where ", table);
    if(strlen(cmd)+strlen(tempStr) >= cmdLen){return false;}
    strcat(cmd, tempStr);

    // startTime
    bzero(tempStr, sizeof(tempStr));
    if(NULL == recStartTime){
        snprintf(tempStr, sizeof(tempStr),"0< CAST(t.TimeStamp as integer) ");
    }else{
        snprintf(tempStr, sizeof(tempStr),"%s< CAST(t.TimeStamp as integer) ", recStartTime);
    }
    if(strlen(cmd)+strlen(tempStr) >= cmdLen){return false;}
    strcat(cmd, tempStr);
    // endTime
    if(recEndTime){
        bzero(tempStr, sizeof(tempStr));
        snprintf(tempStr, sizeof(tempStr),"and CAST(t.TimeStamp as integer) <%s ", recEndTime);
        if(strlen(cmd)+strlen(tempStr) >= cmdLen){return false;}
        strcat(cmd, tempStr);
    }
    // AlarmType
    if( 0 != strcmp(recType , "all")){
        bzero(tempStr, sizeof(tempStr));
        snprintf(tempStr, sizeof(tempStr),"and t.AlarmType = '%s' ", recType);
        if(strlen(cmd)+strlen(tempStr) >= cmdLen){return false;}
        strcat(cmd, tempStr);
    }
    // channelName
    if( 0 != strcmp(recChnId , "all") ){
        bzero(tempStr, sizeof(tempStr));
        snprintf(tempStr, sizeof(tempStr),"and t.ChannelId = '%s' ", recChnId);
        if(strlen(cmd)+strlen(tempStr) >= cmdLen){return false;}
        strcat(cmd, tempStr);
    }
    strcat(cmd, ";");

    return true;
}

/* ==========================================================================
 * 读出记录数量
 *     【参数】 recType ：告警记录类型
 *     【参数】 recChnName ：告警记录通道名称
 *     【参数】 recStartTime ：告警记录起始时间
 *     【参数】 recEndTime ：告警记录结束时间
 *     【返回】：记录条目数量(若为-1,则为调用异常)
 */
int alarmRecords_recCount(const char *recType, const char *recChnId, const char *recStartTime, const char *recEndTime)
{
    int ret = 0;
    sqlite3_stmt *stat = NULL;

    if(NULL == g_db) {
        PRINT_ERROR("database is unInit !!!");
        return -1;
    }

    char cmd[512] = {0};
    pthread_mutex_lock(&g_mutex);
  
    if(0 ==  strcmp(recChnId , "all")){
        if(0 ==  strcmp(recType , "all")){
            snprintf(cmd, sizeof(cmd), "select count(Id) from %s where TimeStamp between '%s' and '%s';", ALARM_TABLE , recStartTime , recEndTime );      
        }else{
            snprintf(cmd, sizeof(cmd), "select count(Id) from %s where TimeStamp between '%s' and '%s' and AlarmType = '%s';", ALARM_TABLE, recStartTime, recEndTime, recType );
        }
    }else{
        if(0 ==  strcmp(recType , "all")){
            snprintf(cmd, sizeof(cmd), "select count(Id) from %s where TimeStamp between '%s' and '%s'  and ChannelId = '%s';", ALARM_TABLE, recStartTime, recEndTime, recChnId);     
        }else{
            snprintf(cmd, sizeof(cmd), "select count(Id) from %s where TimeStamp between '%s' and '%s' and AlarmType = '%s' and ChannelId = '%s';", ALARM_TABLE, recStartTime, recEndTime, recType, recChnId);
        }
    }

    
    PRINT_DEBUG("alarmRecords_recCount_CMD: %s", cmd);
    if (sqlite3_prepare(g_db, cmd, -1, &stat, 0) != SQLITE_OK) {
        pthread_mutex_unlock(&g_mutex);
        return 0;
    }
    if (sqlite3_step(stat) == SQLITE_ROW)
        ret = sqlite3_column_int(stat, 0);
    sqlite3_finalize(stat);
    pthread_mutex_unlock(&g_mutex);
    return ret;
}

/* ==========================================================================
 * 凭筛选条件,删除所有符合条件的记录
 *     【参数】 recType ：告警记录类型
 *     【参数】 recChnName ：告警记录通道名称
 *     【参数】 recStartTime ：告警记录起始时间
 *     【参数】 recEndTime ：告警记录结束时间
 *      如果不是立即同步，有可能导致本次操作在上电重启后丢失
 */
int alarmRecords_delRecord(const char *recType, const char *recChnId, const char *recStartTime, const char *recEndTime, int32_t dataNum, bool sync_flag)
{
    if(NULL == g_db) {
        PRINT_ERROR("database is unInit !!!");
        return -1;
    }
    
    char cmd[512] = {0};
    if(0 != strcmp(recChnId , "all")){//不是all
        if(0 ==  strcmp(recType , "all")){
            snprintf(cmd, sizeof(cmd), "delete from %s where  id in(select id from %s where %s < TimeStamp and TimeStamp <  %s  and ChannelId = '%s' limit %d);", ALARM_TABLE ,ALARM_TABLE,recStartTime , recEndTime  , recChnId,dataNum);
        }else{
            snprintf(cmd, sizeof(cmd), "delete from %s where  id in(select id from %s where %s < TimeStamp and TimeStamp <  %s and AlarmType = '%s' and ChannelId = '%s' limit %d);", ALARM_TABLE ,ALARM_TABLE,recStartTime , recEndTime , recType , recChnId,dataNum);
        }
        
    }else{
        if(0 ==  strcmp(recType , "all")){
            snprintf(cmd, sizeof(cmd), "delete from %s where  id in(select id from %s where %s < TimeStamp and TimeStamp <  %s  limit %d);", ALARM_TABLE ,ALARM_TABLE,recStartTime , recEndTime  ,dataNum);
        }else{
            snprintf(cmd, sizeof(cmd), "delete from %s where  id in(select id from %s where %s < TimeStamp and TimeStamp <  %s and AlarmType = '%s' limit %d);", ALARM_TABLE ,ALARM_TABLE,recStartTime , recEndTime , recType ,dataNum);
        }
        
    }
    
    pthread_mutex_lock(&g_mutex);
    sqlite3_exec(g_db, "begin transaction", NULL, NULL, NULL);
    PRINT_DEBUG("alarmRecords_delRecord:[%s]\n",cmd);
    sqlite3_exec(g_db, cmd, NULL, NULL, NULL);
    sqlite3_exec(g_db, "commit transaction", NULL, NULL, NULL);
    if (sync_flag)
        sync();
    pthread_mutex_unlock(&g_mutex);

    return 0;
}

/* ==========================================================================
 * 插入一条新告警记录
 *     【参数】*recType           : 告警记录类型
 *     【参数】*recDesc           : 告警记录描述
 *     【参数】*recChnID          : 告警记录通道ID
 *     【参数】*recChnName        : 告警记录通道名称
 *     【参数】*recStorageDev : 告警记录对应存储位置名称
 *     【参数】*recPhotoName : 告警记录对应图片名称
*      【参数】*recSrcPhotoName : 告警记录对应图片名称
 *     【参数】*recTime          : 告警记录产生时间戳(ms级)
 *     【参数】sync_flag         : 是否立即同步到文件系统(1-立即同步；0-系统绝对)
 *      如果不是立即同步，有可能导致本次操作在上电重启后丢失
 */
static int alarmRecord_insert(const char *recType, const char *recDesc, const char *recChnID, const char *recChnName, 
                                     const char *recStorageDev, const char *recPhotoName, const char *recSrcPhotoName, 
                                     const char *recTime, bool sync_flag)
{
    char cmd[64 + INDEX_LEN+TYPE_LEN+DESC_LEN+CHNID_LEN+CHNNAME_LEN+STORAGEDEV_LEN+PHOTONAME_LEN+PHOTONAME_LEN+ALARMTIME_LEN] = {0};
    sqlite3_stmt *stat = NULL;

    pthread_mutex_lock(&g_mutex);
    snprintf(cmd, sizeof(cmd), "REPLACE INTO %s VALUES(NULL, '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s');", ALARM_TABLE,
             recDesc, recType, recChnID, recChnName, recStorageDev, recPhotoName, recSrcPhotoName, recTime);

    PRINT_DEBUG("%s", cmd);
    if (sqlite3_prepare(g_db, cmd, -1, &stat, 0) != SQLITE_OK) {
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }
    sqlite3_exec(g_db, "begin transaction", NULL, NULL, NULL);
    sqlite3_step(stat);
    sqlite3_finalize(stat);
    sqlite3_exec(g_db, "commit transaction", NULL, NULL, NULL);
    
    if (sync_flag)
        sync();
    
    pthread_mutex_unlock(&g_mutex);
    return 0;
}
int alarmRecords_addRecord(const char *recType, const char *recDesc, const char *recChnID, const char *recChnName,
                                    const char *recStorageDev, const char *recPhotoName, const char *recSrcPhotoName,
                                    const char *recTime)
{
    if(NULL == g_db) {
        PRINT_ERROR("database is unInit !!!");
        return -1;
    }
    if(strlen(recType) > TYPE_LEN){
        PRINT_ERROR("recType length is too long !!!");
        return -1;
    }
    if(strlen(recDesc) > DESC_LEN){
        PRINT_ERROR("recDesc length is too long !!!");
        return -1;
    }
    if(strlen(recChnID) > CHNID_LEN){
        PRINT_ERROR("recChnID length is too long !!!");
        return -1;
    }
    if(strlen(recChnName) > CHNNAME_LEN){
        PRINT_ERROR("recChnName length is too long !!!");
        return -1;
    }
    if(strlen(recStorageDev) > STORAGEDEV_LEN){
        PRINT_ERROR("recStorageDev length is too long !!!");
        return -1;
    }
    if(strlen(recPhotoName) > PHOTONAME_LEN){
        PRINT_ERROR("recPhotoName length is too long !!!");
        return -1;
    }
    // if(strlen(recSrcPhotoName) > PHOTONAME_LEN){
    //     PRINT_ERROR("recSrcPhotoName length is too long !!!");
    //     return -1;
    // }
    if(strlen(recTime) > ALARMTIME_LEN){
        PRINT_ERROR("recTime length is too long !!!");
        return -1;
    }

    // 若Id已存在，先删除，先不同步
    //database_delete((const char *)recId, 0);

    // 插入记录，并同步
    return alarmRecord_insert(recType, recDesc, recChnID, recChnName, recStorageDev, recPhotoName, recSrcPhotoName, recTime, 1);
}

/* ==========================================================================
 * 提取所有记录
 *     【参数】*pData             : 用于缓存的记录的内存地址
 *     【参数】itemNum            : 记录条数总数量
 */
static int database_get_data(const char * table, AlarmRecItem_t *pData, int itemNum, const char *recType, const char *recChnId, const char *recStartTime, const char *recEndTime)
{
    int ret = 0;
    sqlite3_stmt *stat = NULL;

    int index = 0;
    AlarmRecItem_t *pItem = pData;
    const unsigned char *pColData;
    size_t colDataLen;
    
    if(0 == itemNum) {
        return -1;
    }

    char cmd[512] = {0};
    strcat(cmd, "SELECT ");
    if(false == makeFilter(cmd, sizeof(cmd), table, recType, recChnId, recStartTime, recEndTime)){
        PRINT_ERROR("make cmd faild! cmd[%s]", cmd);
        return -1;        
    }

    pthread_mutex_lock(&g_mutex);

    if (sqlite3_prepare(g_db, cmd, -1, &stat, 0) != SQLITE_OK) {
        pthread_mutex_unlock(&g_mutex);
        return 0;
    }

    // 初始化内存数据
    memset(pData, 0, itemNum * sizeof(AlarmRecItem_t));    
    while (1) {
        ret = sqlite3_step(stat);
        if (ret != SQLITE_ROW)
            break;
#if 0
        // 取第0列数据
        pColData   = sqlite3_column_text(stat, 0);
        colDataLen = sqlite3_column_bytes(stat, 0);
        if (colDataLen <= INDEX_LEN){
            memcpy(pItem->recIndex, (const char *)pColData, colDataLen);
        }else{
            PRINT_ERROR("read AlarmRecord <Id> faild!");
        }
#endif
        // 取第1列数据
        pColData   = sqlite3_column_text(stat, 1);
        colDataLen = sqlite3_column_bytes(stat, 1);
        if (colDataLen <= DESC_LEN){
            memcpy(pItem->recDesc, (const char *)pColData, colDataLen);
        }else{
            PRINT_ERROR("read AlarmRecord <Desc> faild!");
        }
        // 取第2列数据
        pColData   = sqlite3_column_text(stat, 2);
        colDataLen = sqlite3_column_bytes(stat, 2);
        if (colDataLen <= TYPE_LEN){
            memcpy(pItem->recType, (const char *)pColData, colDataLen);
        }else{
            PRINT_ERROR("read AlarmRecord <AlarmType> faild!");
        }
        // 取第3列数据
        pColData   = sqlite3_column_text(stat, 3);
        colDataLen = sqlite3_column_bytes(stat, 3);
        if (colDataLen <= CHNID_LEN){
            memcpy(pItem->recChnID, (const char *)pColData, colDataLen);
        }else{
            PRINT_ERROR("read AlarmRecord <ChannelId> faild!");
        }
        // 取第4列数据
        pColData   = sqlite3_column_text(stat, 4);
        colDataLen = sqlite3_column_bytes(stat, 4);
        if (colDataLen <= CHNNAME_LEN){
            memcpy(pItem->recChnName, (const char *)pColData, colDataLen);
        }else{
            PRINT_ERROR("read AlarmRecord <ChannelName> faild!");
        }
        // 取第5列数据
        pColData   = sqlite3_column_text(stat, 5);
        colDataLen = sqlite3_column_bytes(stat, 5);
        if (colDataLen <= STORAGEDEV_LEN){
            memcpy(pItem->recStorageDev, (const char *)pColData, colDataLen);
        }else{
            PRINT_ERROR("read AlarmRecord <StorageDev> faild!");
        }
        // 取第6列数据
        pColData   = sqlite3_column_text(stat, 6);
        colDataLen = sqlite3_column_bytes(stat, 6);
        if (colDataLen <= PHOTONAME_LEN){
            memcpy(pItem->recPhotoName, (const char *)pColData, colDataLen);
        }else{
            PRINT_ERROR("read AlarmRecord <PhotoPath> faild!");
        }
        // 取第7列数据
        pColData   = sqlite3_column_text(stat, 7);
        colDataLen = sqlite3_column_bytes(stat, 7);
        if (colDataLen <= PHOTONAME_LEN){
            memcpy(pItem->recSrcPhotoName, (const char *)pColData, colDataLen);
        }else{
            PRINT_ERROR("read AlarmRecord <SrcPhotoName> faild!");
        }
        // 取第8列数据
        pColData   = sqlite3_column_text(stat, 8);
        colDataLen = sqlite3_column_bytes(stat, 8);
        if (colDataLen <= ALARMTIME_LEN){
            memcpy(pItem->recTime, (const char *)pColData, colDataLen);
        }else{
            PRINT_ERROR("read AlarmRecord <TimeStamp> faild!");
        }
#if 0
        // 取第5列数据
        pColData   = sqlite3_column_text(stat, 5);
        colDataLen = sqlite3_column_bytes(stat, 5);
        if (colDataLen <= CONFIDENCE_LEN){
            memcpy(pItem->recConfidence, (const char *)pColData, colDataLen);
        }else{
            PRINT_ERROR("read AlarmRecord <CONFIDENCE_LEN> faild!");
        }
#endif
        
        /*
         * 读二进制数据的方法:
         *   pColData = sqlite3_column_blob(stat, 0);
         *   dataLen = sqlite3_column_bytes(stat, 0);
         */

        pItem++;
        
        if (++index >= itemNum)
            break;
    }
    sqlite3_finalize(stat);
    pthread_mutex_unlock(&g_mutex);

    return index;
}
/* ==========================================================================
 * 提取所有记录
 *     【参数】*pData : 用于缓存的记录的内存地址
 *     【返回】       : 记录条目数量，即告警记录数(若为-1,则为调用异常)
 */
int alarmRecords_getRecord(AlarmRecItem_t *pItem, int32_t recNum, const char *recType, const char *recChnId, const char *recStartTime, const char *recEndTime)
{
	int ret = -1;
    if(NULL == pItem) {
        return ret;
    }
    if(NULL == g_db) {
        PRINT_ERROR("database is unInit !!!");
        return ret;
    }
    // 读出数据库符合条件的告警数量
    int alarmRecItemNum = alarmRecords_recCount(recType, recChnId, recStartTime, recEndTime);
    PRINT_DEBUG("alarmRecords_recCount:%d\n",alarmRecItemNum);
	if(0 == alarmRecItemNum){
		return 0;
	}
    int readNum = alarmRecItemNum >= recNum ? recNum : alarmRecItemNum;

    // 提取符合条件的数据到内存中
    ret = database_get_data(ALARM_TABLE, pItem, readNum, recType, recChnId, recStartTime, recEndTime);
	if(ret < 0)
		return ret;
	
	return readNum;
}

int alarmRecords_delet(const char *photoname)
{
    sqlite3_stmt *stat = NULL;
    char cmd[256] = {0};
    
    pthread_mutex_lock(&g_mutex);
    snprintf(cmd, sizeof(cmd), "delete from AlarmRecords where PhotoPath = '%s';", photoname);
    
    if (sqlite3_prepare(g_db, cmd, -1, &stat, 0) != SQLITE_OK) {
        pthread_mutex_unlock(&g_mutex);
        return 0;
    }
    if (sqlite3_step(stat) != SQLITE_OK){
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }
    
    sqlite3_finalize(stat);
    pthread_mutex_unlock(&g_mutex);
    return 0;
}


//统计数据库中数据的(Desc, type)的种类
int alarmRecords_AcquiredDataType(std::vector<AlarmInfo_t> &algo_dev)
{
    sqlite3_stmt *stat = NULL;
    char cmd[256] = {0};
    pthread_mutex_lock(&g_mutex);

    snprintf(cmd, sizeof(cmd), "select Desc,AlarmType from AlarmRecords group by Desc,AlarmType;");

   if (sqlite3_prepare(g_db, cmd, -1, &stat, 0) != SQLITE_OK) {
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }
    
    // 清空
    std::vector<AlarmInfo_t>().swap(algo_dev);

    while (sqlite3_step(stat) == SQLITE_ROW) {
        const unsigned char *desc = sqlite3_column_text(stat, 0);//中文
        const unsigned char *alarmType = sqlite3_column_text(stat, 1);//英文
        PRINT_DEBUG("AcquiredDataType  Desc:%s , Type:%s\n",desc, alarmType);
        AlarmInfo_t  algo_type;
        strcpy(algo_type.desc,(char *)desc);
        strcpy(algo_type.alarmType,(char *)alarmType);
        algo_dev.push_back(algo_type);
    }

    sqlite3_finalize(stat);    
    pthread_mutex_unlock(&g_mutex);
    return 0;
}

















/* ==========================================================================
 * 查询这个“名字”是否在数据库中
 *
 *     【返回】true : “名字”在数据库中
 *     【返回】false: “名字”不在数据库中
 */
bool database_name_is_exist(const char *name)
{
    bool exist = false;
    int ret = 0;
    char cmd[256];
    sqlite3_stmt *stat = NULL;

    pthread_mutex_lock(&g_mutex);
    snprintf(cmd, sizeof(cmd), "SELECT * FROM %s WHERE name = '%s' LIMIT 1;", ALARM_TABLE, name);

    if (sqlite3_prepare(g_db, cmd, -1, &stat, 0) != SQLITE_OK) {
        pthread_mutex_unlock(&g_mutex);
        return false;
    }
    ret = sqlite3_step(stat);
    if (ret == SQLITE_ROW) //表示查询的数据，以多行的形式显示出来
        exist = true;
    else
        exist = false;
    sqlite3_finalize(stat);
    pthread_mutex_unlock(&g_mutex);

    return exist;
}

/* ==========================================================================
 * 查询这个“id”是否在数据库中，并且把“id”对应的“名字”输出到“name”所指的内存中
 *
 *     【返回】true : “名字”在数据库中
 *     【返回】false: “名字”不在数据库中
 */
bool database_id_is_exist(const char *id, char *name, size_t size)
{
    bool exist = false;
    int ret = 0;
    char cmd[256];
    sqlite3_stmt *stat = NULL;

    memset(name, 0, size);
    snprintf(cmd, sizeof(cmd), "SELECT * FROM %s WHERE id = '%s' LIMIT 1;", ALARM_TABLE, id);

    pthread_mutex_lock(&g_mutex);
    if (sqlite3_prepare(g_db, cmd, -1, &stat, 0) != SQLITE_OK) {
        pthread_mutex_unlock(&g_mutex);
        return false;
    }
    ret = sqlite3_step(stat);
    if (ret == SQLITE_ROW) {
        const unsigned char *n = sqlite3_column_text(stat, 1);
        size_t s = sqlite3_column_bytes(stat, 1);
        if (s < size)
            strncpy(name, (const char *)n, s);
        else
            strncpy(name, (const char *)n, size - 1);
        exist = true;
    } else {
        exist = false;
    }
    sqlite3_finalize(stat);
    pthread_mutex_unlock(&g_mutex);

    return exist;
}


/* ==========================================================================
 * 获取未被使用ID？
 *
 *     返回: 未知意义
 */
int database_get_user_name_id(void)
{
    int ret = 0;
    char cmd[256];
    sqlite3_stmt *stat = NULL;
    int id = 0;
    int max_id = -1;
    int *save_id = NULL;
    int ret_id = 0;

    pthread_mutex_lock(&g_mutex);

    /* 提取数据库的表内的行数量 */
    snprintf(cmd, sizeof(cmd), "SELECT * FROM %s where id=(select max(id) from %s);", ALARM_TABLE, ALARM_TABLE);
    if (sqlite3_prepare(g_db, cmd, -1, &stat, 0) != SQLITE_OK) {
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }
    while (1) {
        ret = sqlite3_step(stat);
        if (ret != SQLITE_ROW)
            break;
        max_id = sqlite3_column_int(stat, 2);
    }
    sqlite3_finalize(stat);

    if (max_id < 0) {
        pthread_mutex_unlock(&g_mutex);
        return 0;
    }
    save_id = (int*)calloc(max_id + 1, sizeof(int));
    if (!save_id) {
        PRINT_ERROR("memory alloc fail!");
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }

    snprintf(cmd, sizeof(cmd), "SELECT * FROM %s;", ALARM_TABLE);
    if (sqlite3_prepare(g_db, cmd, -1, &stat, 0) != SQLITE_OK) {
        ret_id = -1;
        goto exit;
    }
    while (1) {
        ret = sqlite3_step(stat);
        if (ret != SQLITE_ROW)
            break;
        id = sqlite3_column_int(stat, 2);
        save_id[id] = 1;
    }
    sqlite3_finalize(stat);

    for (int i = 0; i < max_id + 1; i++) {
        if (!save_id[i]) {
            ret_id = i;
            goto exit;
        }
    }
    ret_id = max_id + 1;

exit:
    if (save_id)
        free(save_id);
    pthread_mutex_unlock(&g_mutex);
    return ret_id;
}

/* ==========================================================================
 * 插入一条新告警记录
 *     【参数】*recType          : 告警记录类型
 *     【参数】*recDesc          : 告警记录描述
 *     【参数】*recChnID         : 告警记录通道ID
 *     【参数】*recChnName       : 告警记录通道名称
 *     【参数】*recPhotoName : 告警记录对应图片名称
 *     【参数】*recTime          : 告警记录产生时间戳(ms级)
 *     【参数】sync_flag         : 是否立即同步到文件系统(1-立即同步；0-系统绝对)
 *      如果不是立即同步，有可能导致本次操作在上电重启后丢失
 */
static int CTADRecord_insert(const char *recType, const char *recDesc, const char *recChnID, const char *recChnName,
                                    const char *recPhotoName, const char *recTime, int recAlarmNum, const char *recCustomMsg,
                                    bool sync_flag)
{
    char cmd[64 + INDEX_LEN+TYPE_LEN+DESC_LEN+CHNID_LEN+CHNNAME_LEN+PHOTONAME_LEN+ALARMTIME_LEN+4/*alarmNum*/+CUSTOMMSG_LEN] = {0};
    sqlite3_stmt *stat = NULL;
    
    pthread_mutex_lock(&g_mutex);
    snprintf(cmd, sizeof(cmd), "REPLACE INTO %s VALUES(NULL, '%s', '%s', '%s', '%s', '%s', '%s', %d, '%s');", CTAD_TABLE,
             recDesc, recType, recChnID, recChnName, recPhotoName, recTime, recAlarmNum, recCustomMsg);

    PRINT_DEBUG("%s", cmd);
    if (sqlite3_prepare(g_db, cmd, -1, &stat, 0) != SQLITE_OK) {
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }
    sqlite3_exec(g_db, "begin transaction", NULL, NULL, NULL);
    sqlite3_step(stat);
    sqlite3_finalize(stat);
    sqlite3_exec(g_db, "commit transaction", NULL, NULL, NULL);
    
    if (sync_flag)
        sync();
    
    pthread_mutex_unlock(&g_mutex);

    return 0;
}
int add_CTAD_record(const char *recType, const char *recDesc, const char *recChnID, const char *recChnName,
                          const char *recPhotoName, const char *recTime, int recNum, const char *recCustomMsg)
{
    if(NULL == g_db) {
        PRINT_ERROR("database is unInit !!!");
        return -1;
    }
    if(strlen(recType) > TYPE_LEN){
        PRINT_ERROR("recType length is too long !!!");
        return -1;
    }
    if(strlen(recDesc) > DESC_LEN){
        PRINT_ERROR("recDesc length is too long !!!");
        return -1;
    }
    if(strlen(recChnID) > CHNID_LEN){
        PRINT_ERROR("recChnID length is too long !!!");
        return -1;
    }
    if(strlen(recChnName) > CHNNAME_LEN){
        PRINT_ERROR("recChnName length is too long !!!");
        return -1;
    }
    if(strlen(recPhotoName) > PHOTONAME_LEN){
        PRINT_ERROR("recPhotoName length is too long !!!");
        return -1;
    }
    if(strlen(recTime) > ALARMTIME_LEN){
        PRINT_ERROR("recTime length is too long !!!");
        return -1;
    }
    if(strlen(recCustomMsg) > CUSTOMMSG_LEN){
        PRINT_ERROR("recCustomMsg length is too long !!!");
        return -1;
    }

    // 若Id已存在，先删除，先不同步
    //database_delete((const char *)recId, 0);

    // 插入记录，并同步
    return CTADRecord_insert(recType, recDesc, recChnID, recChnName, recPhotoName, recTime, recNum, recCustomMsg, 1);

}

int delet_CTAD_record(int Id)
{
    sqlite3_stmt *stat = NULL;
    char cmd[256] = {0};
    pthread_mutex_lock(&g_mutex);
    snprintf(cmd, sizeof(cmd), "delete from CTAD where id = %d;",Id);


    if (sqlite3_prepare(g_db, cmd, -1, &stat, 0) != SQLITE_OK) {
        pthread_mutex_unlock(&g_mutex);
        return 0;
    }
    if (sqlite3_step(stat) != SQLITE_OK){
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }
    sqlite3_finalize(stat);
    pthread_mutex_unlock(&g_mutex);
    return 0;
}


//返回CTAD表记录数量
int CTAD_record_number()
{
    int ret = 0;
    sqlite3_stmt *stat = NULL;
    char cmd[256] = {0};
    pthread_mutex_lock(&g_mutex);
    snprintf(cmd, sizeof(cmd), "select count(Id) from CTAD;");


    if (sqlite3_prepare(g_db, cmd, -1, &stat, 0) != SQLITE_OK) {
        pthread_mutex_unlock(&g_mutex);
        return 0;
    }
    if (sqlite3_step(stat) == SQLITE_ROW)
        ret = sqlite3_column_int(stat, 0);
    sqlite3_finalize(stat);
    pthread_mutex_unlock(&g_mutex);


    return ret;
}

//根据ID查到数据
int CTAD_extraction_data(int Id, char *recType, char *recDesc, char *recChnID, char *recChnName, char *recPhotoName, char *recTime, int *recAlarmNum, char *recCustomMsg)
{
    char *errmsg = NULL;
    char** dbResult = NULL;
    char cmd[256] = {0};
    pthread_mutex_lock(&g_mutex);
    snprintf(cmd, sizeof(cmd), "select * from CTAD where Id = %d;", Id);

    int nRow, nColumn;
    if( sqlite3_get_table(g_db, cmd, &dbResult, &nRow, &nColumn, &errmsg) != SQLITE_OK){
         pthread_mutex_unlock(&g_mutex);
        return -1;
    } else {
        strcpy(recType,dbResult[nColumn+1]);
        strcpy(recDesc,dbResult[nColumn+2]);   
        strcpy(recChnID,dbResult[nColumn+3]);
        strcpy(recChnName,dbResult[nColumn+4]);
        strcpy(recPhotoName,dbResult[nColumn+5]);
        strcpy(recTime,dbResult[nColumn+6]);
        *recAlarmNum = strtol(dbResult[nColumn+7], NULL, 10);
        strcpy(recCustomMsg,dbResult[nColumn+8]);
    }

    sqlite3_free_table(dbResult);//释放查询空间 
    pthread_mutex_unlock(&g_mutex);
    return 0;
}




//提取最小ID
int CTAD_Minimum_ID()
{
    sqlite3_stmt *stat = NULL;
    int Id = -1;
    char cmd[256] = {0};
    pthread_mutex_lock(&g_mutex);
    snprintf(cmd, sizeof(cmd), "select Id from CTAD limit 1;");

    if (sqlite3_prepare(g_db, cmd, -1, &stat, 0) != SQLITE_OK) {
        pthread_mutex_unlock(&g_mutex);
        return Id;
    }
    if (sqlite3_step(stat) == SQLITE_ROW)
        Id = sqlite3_column_int(stat, 0);
    sqlite3_finalize(stat);
    pthread_mutex_unlock(&g_mutex);
    return Id;
}

//提取第一行PhotoPath
int alarmRecords_Minimum_Photo(char *pthoto_name, char *Srcpthoto_name)
{   
    char *errmsg = NULL;
    char** dbResult = NULL;
    char cmd[256] = {0};
    pthread_mutex_lock(&g_mutex);
    snprintf(cmd, sizeof(cmd), "select PhotoPath,SrcPhotoPath from AlarmRecords limit 1;");

    int nRow, nColumn;
    if( sqlite3_get_table(g_db, cmd, &dbResult, &nRow, &nColumn, &errmsg) != SQLITE_OK){
         pthread_mutex_unlock(&g_mutex);
        return -1;
    } else {
        strcpy(pthoto_name,dbResult[nColumn]);
        strcpy(Srcpthoto_name,dbResult[nColumn + 1]);
    }

    sqlite3_free_table(dbResult);//释放查询空间 
    pthread_mutex_unlock(&g_mutex);
    return 0;
}
 






/* ==========================================================================
 * 读出记录数量
 *     【参数】 recType ：告警记录类型
 *     【参数】 recChnName ：告警记录通道名称
 *     【参数】 recStartTime ：告警记录起始时间
 *     【参数】 recEndTime ：告警记录结束时间
 *     【返回】：记录条目数量(若为-1,则为调用异常)
 */
int originalRecords_recCount(const char *recType, const char *recChnId, const char *recStartTime, const char *recEndTime)
{
    int ret = 0;
    sqlite3_stmt *stat = NULL;
    
    if(NULL == g_db) {
        PRINT_ERROR("database is unInit !!!");
        return -1;
    }
    
    char cmd[512] = {0};
    pthread_mutex_lock(&g_mutex);
    if(0 ==  strcmp(recChnId , "all")){
        snprintf(cmd, sizeof(cmd), "select count(Id) from %s where TimeStamp between '%s' and '%s' and AlarmType = '%s';", ORIGINAL_TABLE, recStartTime, recEndTime, recType );
    }else{
        snprintf(cmd, sizeof(cmd), "select count(Id) from %s where TimeStamp between '%s' and '%s' and AlarmType = '%s' and ChannelId = '%s';", ORIGINAL_TABLE, recStartTime, recEndTime, recType, recChnId);
    }
    
    if (sqlite3_prepare(g_db, cmd, -1, &stat, 0) != SQLITE_OK) {
        pthread_mutex_unlock(&g_mutex);
        return 0;
    }
    if (sqlite3_step(stat) == SQLITE_ROW)
        ret = sqlite3_column_int(stat, 0);
    sqlite3_finalize(stat);
    pthread_mutex_unlock(&g_mutex);
    
    return ret;
}


/* ==========================================================================
 * 凭筛选条件,删除所有符合条件的记录
 *     【参数】 recType ：告警记录类型
 *     【参数】 recChnName ：告警记录通道名称
 *     【参数】 recStartTime ：告警记录起始时间
 *     【参数】 recEndTime ：告警记录结束时间
 *      如果不是立即同步，有可能导致本次操作在上电重启后丢失
 */
int originalRecords_delRecord(const char *recType, const char *recChnId, const char *recStartTime, const char *recEndTime, int32_t dataNum, bool sync_flag)
{
    if(NULL == g_db) {
        PRINT_ERROR("database is unInit !!!");
        return -1;
    }
    
   char cmd[512] = {0};
    //snprintf(cmd, sizeof(cmd), "delete from %s where  id in(select id from %s where %s < TimeStamp and TimeStamp <  %s and AlarmType = '%s' and ChannelId = '%s' limit %d);", ORIGINAL_TABLE ,ORIGINAL_TABLE,recStartTime , recEndTime , recType , recChnId,dataNum);
    
    if(0 != strcmp(recChnId , "all")){
        snprintf(cmd, sizeof(cmd), "delete from %s where  id in(select id from %s where %s < TimeStamp and TimeStamp <  %s and AlarmType = '%s' and ChannelId = '%s' limit %d);", ORIGINAL_TABLE, ORIGINAL_TABLE, recStartTime, recEndTime, recType, recChnId, dataNum);
    }else{
        snprintf(cmd, sizeof(cmd), "delete from %s where  id in(select id from %s where %s < TimeStamp and TimeStamp <  %s and AlarmType = '%s' limit %d);", ORIGINAL_TABLE, ORIGINAL_TABLE, recStartTime, recEndTime, recType, dataNum);
    }

    pthread_mutex_lock(&g_mutex);
    sqlite3_exec(g_db, "begin transaction", NULL, NULL, NULL);
    PRINT_DEBUG("originalRecords_delRecord:[%s]\n",cmd);
    sqlite3_exec(g_db, cmd, NULL, NULL, NULL);
    sqlite3_exec(g_db, "commit transaction", NULL, NULL, NULL);
    if (sync_flag)
        sync();
    pthread_mutex_unlock(&g_mutex);

    return 0;
}

/* ==========================================================================
 * 插入一条新告警记录
 *     【参数】*recType          : 告警记录类型
 *     【参数】*recDesc          : 告警记录描述
 *     【参数】*recChnID         : 告警记录通道ID
 *     【参数】*recChnName       : 告警记录通道名称
 *     【参数】*recPhotoName : 告警记录对应图片名称
 *     【参数】*recTime          : 告警记录产生时间戳(ms级)
 *     【参数】sync_flag         : 是否立即同步到文件系统(1-立即同步；0-系统绝对)
 *      如果不是立即同步，有可能导致本次操作在上电重启后丢失
 */
static int originalRecord_insert(const char *recType, const char *recDesc, const char *recChnID, const char *recChnName,
                                         const char *recPhotoName, const char *recTime,
                                         bool sync_flag)
{
    char cmd[64 + INDEX_LEN+TYPE_LEN+DESC_LEN+CHNID_LEN+CHNNAME_LEN+PHOTONAME_LEN+ALARMTIME_LEN] = {0};
    sqlite3_stmt *stat = NULL;

    pthread_mutex_lock(&g_mutex);
    snprintf(cmd, sizeof(cmd), "REPLACE INTO %s VALUES(NULL, '%s', '%s', '%s', '%s', '%s', '%s');", ORIGINAL_TABLE,
             recDesc, recType, recChnID, recChnName, recPhotoName, recTime);

    PRINT_DEBUG("%s", cmd);
    if (sqlite3_prepare(g_db, cmd, -1, &stat, 0) != SQLITE_OK) {
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }
    sqlite3_exec(g_db, "begin transaction", NULL, NULL, NULL);
    sqlite3_step(stat);
    sqlite3_finalize(stat);
    sqlite3_exec(g_db, "commit transaction", NULL, NULL, NULL);
    
    if (sync_flag)
        sync();
    
    pthread_mutex_unlock(&g_mutex);
    return 0;
}
int originalRecords_addRecord(const char *recType, const char *recDesc, const char *recChnID, const char *recChnName, const char *recPhotoName, const char *recTime)
{
    if(NULL == g_db) {
        PRINT_ERROR("database is unInit !!!");
        return -1;
    }
    if(strlen(recType) > TYPE_LEN){
        PRINT_ERROR("recType length is too long !!!");
        return -1;
    }
    if(strlen(recDesc) > DESC_LEN){
        PRINT_ERROR("recDesc length is too long !!!");
        return -1;
    }
    if(strlen(recChnID) > CHNID_LEN){
        PRINT_ERROR("recChnID length is too long !!!");
        return -1;
    }
    if(strlen(recChnName) > CHNNAME_LEN){
        PRINT_ERROR("recChnName length is too long !!!");
        return -1;
    }
    if(strlen(recPhotoName) > PHOTONAME_LEN){
        PRINT_ERROR("recPhotoName length is too long !!!");
        return -1;
    }
    if(strlen(recTime) > ALARMTIME_LEN){
        PRINT_ERROR("recTime length is too long !!!");
        return -1;
    }

    // 若Id已存在，先删除，先不同步
    //database_delete((const char *)recId, 0);

    // 插入记录，并同步
    return originalRecord_insert(recType, recDesc, recChnID, recChnName, recPhotoName, recTime, 1);
}

/* ==========================================================================
 * 提取所有记录
 *     【参数】*pData : 用于缓存的记录的内存地址
 *     【返回】       : 记录条目数量，即告警记录数(若为-1,则为调用异常)
 */
int originalRecords_getRecord(AlarmRecItem_t *pItem, int32_t recNum, const char *recType, const char *recChnId, const char *recStartTime, const char *recEndTime)
{
	int ret = -1;
    if(NULL == pItem) {
        return ret;
    }
    if(NULL == g_db) {
        PRINT_ERROR("database is unInit !!!");
        return ret;
    }
    // 读出数据库符合条件的告警数量
    int alarmRecItemNum = originalRecords_recCount(recType, recChnId, recStartTime, recEndTime);
    PRINT_DEBUG("originalRecords_recCount:%d\n",alarmRecItemNum);
	if(0 == alarmRecItemNum){
		return 0;
	}
    int readNum = alarmRecItemNum >= recNum ? recNum : alarmRecItemNum;

    // 提取符合条件的数据到内存中
    ret = database_get_data(ORIGINAL_TABLE, pItem, readNum, recType, recChnId, recStartTime, recEndTime);
	if(ret < 0)
		return ret;
	
	return readNum;
}


//提取第一行PhotoPath
int originalRecords_Minimum_Photo(char *pthoto_name)
{   
    char *errmsg = NULL;
    char** dbResult = NULL;
    char cmd[256] = {0};
    pthread_mutex_lock(&g_mutex);
    snprintf(cmd, sizeof(cmd), "select PhotoPath from OriginalRecords limit 1;");

    int nRow, nColumn;
    if( sqlite3_get_table(g_db, cmd, &dbResult, &nRow, &nColumn, &errmsg) != SQLITE_OK){
         pthread_mutex_unlock(&g_mutex);
        return -1;
    }
    else
    {
        strcpy(pthoto_name, dbResult[nColumn]);
    }

    sqlite3_free_table(dbResult);//释放查询空间 
    pthread_mutex_unlock(&g_mutex);
    return 0;
}


int originalRecords_delet(const char* photoname)
{
    sqlite3_stmt *stat = NULL;
    char cmd[256] = {0};
    pthread_mutex_lock(&g_mutex);

    snprintf(cmd, sizeof(cmd), "delete from OriginalRecords where PhotoPath = '%s';",photoname);


    if (sqlite3_prepare(g_db, cmd, -1, &stat, 0) != SQLITE_OK) {
        pthread_mutex_unlock(&g_mutex);
        return 0;
    }
    if (sqlite3_step(stat) != SQLITE_OK){
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }
    sqlite3_finalize(stat);
    pthread_mutex_unlock(&g_mutex);
    return 0;
}





