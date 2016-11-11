#include "networkio.h"
#include "IOService.h"
#include "Mutex.h"
#include "KCPThread.h"

extern "C"
{
	static CMutex g_mutex;
    /**
     * @brief   
     * @param  pFilter:将由底层进行delete,切记
     * @return                                                                      
	 */
	bool CreateServerIOService(IOServiceType serviceType, const char* sSrcIP,
			int nSrcPort, IOEvent *pEventObj, NetFilter *pFilter, 
			int nThreadNum, bool bSync)
	{
		if(IO_SERVICE_KCP_SERVER == serviceType)
		{
			CKCPThread::getKCPThread();
		}

		CMutexProxy lk(g_mutex);
		CIOService *pIOService = new CIOService;
		bool bRet = pIOService->StartUp(serviceType, string(sSrcIP), nSrcPort, "", 0,
				pEventObj, pFilter, nThreadNum, bSync);
		if(!bRet && bSync)
		{
			delete pIOService;
			pIOService = NULL;
		}
		return bRet;
	}

	bool CreateClientIOService(IOServiceType serviceType, const char* sSrcIP,
			int nSrcPort, const char* sDstIP, int nDstPort, IOEvent *pEventObj,
			NetFilter *pFilter, int nThreadNum, bool bSync)
	{
		if(IO_SERVICE_KCP_CLIENT == serviceType)
		{
			CKCPThread::getKCPThread();
		}

		CMutexProxy lk(g_mutex);
		CIOService *pIOService = new CIOService;
		bool bRet = pIOService->StartUp(serviceType, string(sSrcIP), nSrcPort, string(sDstIP), 
				nDstPort, pEventObj, pFilter, nThreadNum, bSync);
		if(!bRet && bSync)
		{
			delete pIOService;
			pIOService = NULL;
		}
		return bRet;
	}
};
