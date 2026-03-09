//=====================  SDK  =====================
#include "system_opt.h"
//=====================  PRJ  =====================
#include "rgaLock.h"

RgaLock *RgaLock::m_pSelf = NULL;

RgaLock::RgaLock()
{
    pthread_mutex_init(&mRgaLock, NULL);
}

RgaLock::~RgaLock()
{
    pthread_mutex_destroy(&mRgaLock);

    delete m_pSelf;
    m_pSelf = NULL;
}

void RgaLock::createRgaLock()
{
    if(m_pSelf == nullptr){
        once_flag oc;
        call_once(oc, [&] {
            m_pSelf = new RgaLock;
        });
    }
}

