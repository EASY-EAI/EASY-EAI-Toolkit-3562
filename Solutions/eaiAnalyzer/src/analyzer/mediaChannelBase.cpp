//=====================   C   =====================
//====================   C++   ====================
//=====================  SDK  =====================
#include "system_opt.h"
#include "ini_wrapper.h"
#include "log_manager.h"
//=====================  PRJ  =====================
#include "mediaChannelBase.h"


extern void *channelProcess_thread(void *para);

MediaChannelBase::MediaChannelBase() :
    mMediaChnId(-1),
    mSginalId(-1),
	bObjIsInited(0)
{
#if 0
    // init: /tmp/apps/signalMgr/imgData.ini
    mImgCfgFile.clear();
    mImgCfgFile.append(APP_PATH);
    mImgCfgFile.append("/");
    mImgCfgFile.append(IMAGE_CONFIG_FILE);
#endif

    // 初始化图像描述信息
    imgLock_Init();
    memset(&mImage, 0, sizeof(mImage));

    // 创建信号源分析线程
    mThreadWorking = false;
    if(0 != CreateJoinThread(channelProcess_thread, this, &mTid)){
        return ;
    }
}

MediaChannelBase::~MediaChannelBase()
{
}

void MediaChannelBase::destroyMediaChannelThread()
{
    // 1，等待取流线程跑起来
    int timeOut_ms = 1000; //设置n(ms)超时，超时就不等了
    while(1){
        if((true == mThreadWorking)||(timeOut_ms <= 0)){
            break;
        }
        timeOut_ms--;
        usleep(1000);
    }

    // 2，退出线程并等待其结束
    mThreadWorking = false;
    // --[等待取流线程结束]--
    while(1) {
        usleep(20*1000);
        int32_t exitCode = pthread_join(mTid, NULL);
        if(0 == exitCode){
            break;
        }else if(0 != exitCode){
            switch (exitCode) {
                case ESRCH:  // 没有找到线程ID
                    PRINT_ERROR("MediaChannel[%d] imgCombineThread exit: No thread with the given ID was found.", mediaChnId());
                    break;
                case EINVAL: // 线程不可连接或已经有其他线程在等待它
                    PRINT_ERROR("MediaChannel[%d] imgCombineThread exit: Thread is detached or already being waited on.", mediaChnId());
                    break;
                case EDEADLK: // 死锁 - 线程尝试join自己
                    PRINT_ERROR("MediaChannel[%d] imgCombineThread exit: Deadlock detected - thread is trying to join itself.",  mediaChnId());
                    break;
            }
            continue;
        }
    }

    imgLock_UnInit();

    return ;
}

void MediaChannelBase::setImageInfo(ImgSignal_t image)
{
    imgLock();
    memcpy(&mImage, &image, sizeof(mImage));
    imgUnLock();

    return ;
}

