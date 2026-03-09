#ifndef __RGALOCK_H__
#define __RGALOCK_H__
//====================   C++   ====================
#include <mutex>
//=====================  SDK  =====================

using namespace std;

class RgaLock
{
public:
	RgaLock();
	~RgaLock();

    static RgaLock *instance()
    {
        if(m_pSelf == nullptr){
            once_flag oc;
            call_once(oc, [&] {
                m_pSelf = new RgaLock;
            });
        }
        return m_pSelf;
    }
    static void createRgaLock();

    void Lock(){pthread_mutex_lock(&mRgaLock);}
    void UnLock(){pthread_mutex_unlock(&mRgaLock);}

protected:    
    pthread_mutex_t mRgaLock;
	
private:
    static RgaLock *m_pSelf;
};

#endif
