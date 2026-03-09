#ifndef __MEDIACHANNELBASE_H__
#define __MEDIACHANNELBASE_H__
//=====================   C   =====================
#include "appSystem.h"
#include "config.h"
//====================   C++   ====================
#include <string>
//=====================  SDK  =====================
//=====================  PRJ  =====================

typedef struct{
    int width;
    int height;
    char name[128];
	int dataLen;
    char dataFmt[16];
    uint8_t *data;	
}ImgSignal_t;

class MediaChannelBase
{
public:
	MediaChannelBase();
	~MediaChannelBase();
    void destroyMediaChannelThread();

	virtual int32_t init() = 0;
	int32_t IsInited(){return bObjIsInited;}

    void setMediaChnId(int id){mMediaChnId = id;}
    int mediaChnId(){return mMediaChnId;}
    void setSignalId(int id){mSginalId = id;}
    int signalId(){return mSginalId;}
    void setImageInfo(ImgSignal_t image);
    
    void imgLock(){pthread_mutex_lock(&mImgLock);}
    void imgUnLock(){pthread_mutex_unlock(&mImgLock);}

        
    ImgSignal_t mImage;
    pthread_t mTid;
    bool mThreadWorking;

protected:
    void imgLock_Init(){pthread_mutex_init(&mImgLock, NULL);}
    void imgLock_UnInit(){    pthread_mutex_destroy(&mImgLock);}
    pthread_mutex_t mImgLock;
    
	int bObjIsInited;
private:
    std::string mImgCfgFile;
    
    int mMediaChnId;
    int mSginalId;
};


#endif
