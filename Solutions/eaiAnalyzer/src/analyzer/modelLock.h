#ifndef __MODELLOCK_H__
#define __MODELLOCK_H__
//====================   C++   ====================
#include <mutex>
//=====================  SDK  =====================

using namespace std;

class ModelLock
{
public:
	ModelLock();
	~ModelLock();

    static ModelLock *instance()
    {
        if(m_pSelf == nullptr){
            once_flag oc;
            call_once(oc, [&] {
                m_pSelf = new ModelLock;
            });
        }
        return m_pSelf;
    }
    static void createModelLock();

    void Lock(){pthread_mutex_lock(&mModelLock);}
    void UnLock(){pthread_mutex_unlock(&mModelLock);}

protected:    
    pthread_mutex_t mModelLock;
	
private:
    static ModelLock *m_pSelf;
};

#endif
