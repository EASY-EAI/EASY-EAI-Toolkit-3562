//=====================  SDK  =====================
#include "system_opt.h"
//=====================  PRJ  =====================
#include "modelLock.h"

ModelLock *ModelLock::m_pSelf = NULL;

ModelLock::ModelLock()
{
    pthread_mutex_init(&mModelLock, NULL);
}

ModelLock::~ModelLock()
{
    pthread_mutex_destroy(&mModelLock);

    delete m_pSelf;
    m_pSelf = NULL;
}

void ModelLock::createModelLock()
{
    if(m_pSelf == nullptr){
        once_flag oc;
        call_once(oc, [&] {
            m_pSelf = new ModelLock;
        });
    }
}

