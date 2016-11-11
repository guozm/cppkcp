#include "IOService.h"
#include "IoUtils.h"
#include "TCPReadThread.h"
#include "TCPWriteThread.h"
#include "KCPReadThread.h"
#include "KCPWriteThread.h"
#include "KCPServer.h"
#include "KCPClient.h"
#include <assert.h>
#include <sstream>

#ifdef WIN32
#include <Windows.h>
#endif

#ifdef WIN32
static int i = 0;
#endif

CIOService::CIOService() : m_pAcceptEventBase(NULL), m_bSuccess(true)
{
	m_nCount = 0;
	pthread_mutex_init(&m_lock , NULL);
	pthread_cond_init(&m_cond , NULL);

#ifdef WIN32
	if(i == 0)
	{
		WORD wVersionRequested;
		WSADATA wsaData;
		int err;

		/* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
		wVersionRequested = MAKEWORD(2, 2);

		err = WSAStartup(wVersionRequested, &wsaData);
		if (err != 0) {
			/* Tell the user that we could not find a usable */
			/* Winsock DLL.                                  */
			DebugMsg("WSAStartup failed with error: %d\n", err);
		}
		i++;
	}
#endif
}

CIOService::~CIOService()
{
	printf("CIOService::~CIOService\n");
	pthread_mutex_destroy(&m_lock);
	pthread_cond_destroy(&m_cond);

	if(m_pAcceptEventBase)
	{
		event_del(&m_acceptEvent);
	}

	vector<IBaseIOThread*>::iterator its;
	for(its=m_ReadThreadVec.begin(); its!=m_ReadThreadVec.end(); its++)
	{
		(*its)->stopThread();
	}
	m_ReadThreadVec.clear();

	for(its=m_WriteThreadVec.begin(); its!=m_WriteThreadVec.end(); its++)
	{
		(*its)->stopThread();
	}
	m_WriteThreadVec.clear();
}

void CIOService::startThread()
{
	pthread_t       thread;
	int             ret;
	if((ret = pthread_create(&thread, NULL, threadFun, this)) != 0) 
	{
		printf("recv is faild ... error = %s\n", strerror(errno));
		assert(0);
	}

	pthread_detach(thread);
}

void *CIOService::threadFun(void *argc)
{
	CIOService *pIOService = (CIOService*)argc;
	pthread_mutex_lock(&pIOService->m_lock);
	int nRet;
	int nSocket;
	string sSrcIP = pIOService->m_StartUpArg.mIPAddr.m_sLocalIP;
	int nSrcPort = atoi(pIOService->m_StartUpArg.mIPAddr.m_sLocalPort.c_str());
	string sDstIP = pIOService->m_StartUpArg.mIPAddr.m_sRemoteIP;
	int nDstPort = atoi(pIOService->m_StartUpArg.mIPAddr.m_sRemotePort.c_str());
	IOServiceType serviceType = pIOService->m_StartUpArg.serviceType;
	string sLocalIP, sLocalPort;
	if(IO_SERVICE_TCP_SERVER == serviceType)
	{
		nRet = CIOUtils::tcpListen(sSrcIP.c_str(), nSrcPort, &nSocket);
	}
	else if(IO_SERVICE_TCP_CLIENT == serviceType)
	{
		nRet = CIOUtils::tcpConnect(sDstIP.c_str(), nDstPort, &nSocket, sLocalIP, sLocalPort);
	}
	else if(IO_SERVICE_KCP_SERVER == serviceType)
	{
		nRet = CIOUtils::udpListen(sSrcIP.c_str(), nSrcPort, &nSocket);
	}
	else if(IO_SERVICE_KCP_CLIENT == serviceType)
	{
		nRet = CIOUtils::udpConnect(sDstIP.c_str(), nDstPort, &nSocket, sLocalIP, sLocalPort);
	}
    else
    {
        nRet = -1;
    }

	if(0 != nRet)
	{
		pIOService->m_bSuccess = false;
	}
	bool bSync = pIOService->m_StartUpArg.bSync;
	pthread_cond_signal(&pIOService->m_cond);
	pthread_mutex_unlock(&pIOService->m_lock);
	
	//失败
	if(nRet != 0)
	{
		if(false == bSync)
		{
			pIOService->m_StartUpArg.pEventObj->OnIOEvent(NULL, IO_EVENT_OPEN_FAILED, NULL, 0);
			delete pIOService;
			pIOService = NULL;
		}
		return NULL;
	}

	//启动接受线程
	for(int i=0; i<pIOService->m_StartUpArg.nThreadNum; i++)
	{
		IBaseIOThread *pReadThread = NULL;
		if(IO_SERVICE_KCP_SERVER == serviceType || IO_SERVICE_KCP_CLIENT == serviceType)
		{
			pReadThread = new CKCPReadThread();
		}
		else
		{
			pReadThread = new CTCPReadThread();
		}
		pReadThread->startThread();
		pIOService->m_ReadThreadVec.push_back(pReadThread);
	}

	//启动发送线程
	for(int i=0; i<pIOService->m_StartUpArg.nThreadNum; i++)
	{
		IBaseIOThread *pWriteThread = NULL;
		if(IO_SERVICE_KCP_SERVER == serviceType || IO_SERVICE_KCP_CLIENT == serviceType)
		{
			pWriteThread = new CKCPWriteThread();
		}
		else
		{
			pWriteThread = new CTCPWriteThread();
		}
		pWriteThread->startThread();
		pIOService->m_WriteThreadVec.push_back(pWriteThread);
	}

	if(IO_SERVICE_TCP_SERVER == serviceType || 
			IO_SERVICE_KCP_SERVER == serviceType)
	{
		pIOService->m_StartUpArg.pEventObj->OnIOEvent(NULL, IO_EVENT_INIT, NULL, 0);
	}
	else if(IO_SERVICE_TCP_CLIENT == serviceType)
	{
		pIOService->dispatcherNewCon(
				nSocket, SOCKETTYPE_CONNECT, ENETWORKTYPE_TCP, sLocalIP, sLocalPort);
	}
	else if(IO_SERVICE_KCP_CLIENT == serviceType)
	{
		tStartUpArg *pStartUpArg = &(pIOService->m_StartUpArg);
		CKCPClient *pKcpClient = new CKCPClient(nSocket, 
				pStartUpArg->pEventObj, pStartUpArg->pFilter, 
				pIOService->m_ReadThreadVec, pIOService->m_WriteThreadVec, pIOService);
		if(pKcpClient)
		{
		}
	}

	if(IO_SERVICE_TCP_SERVER == serviceType) 
	{
		pIOService->m_pAcceptEventBase = event_init();
		if(!pIOService->m_pAcceptEventBase)
		{
			assert(0);
		}

		if(IO_SERVICE_TCP_SERVER == serviceType)
		{
			event_set(&pIOService->m_acceptEvent, nSocket, EV_READ | EV_PERSIST, 
					onTcpAcceptCallback, pIOService);
		}

		event_base_set(pIOService->m_pAcceptEventBase, &pIOService->m_acceptEvent);
		event_add(&pIOService->m_acceptEvent, NULL );

		event_base_loop(pIOService->m_pAcceptEventBase, 0);
		event_base_free(pIOService->m_pAcceptEventBase);
	}
	else if(IO_SERVICE_KCP_SERVER == serviceType)
	{
		tStartUpArg *pStartUpArg = &(pIOService->m_StartUpArg);
		CKCPServer *pKcpServer = new CKCPServer(nSocket,
				pStartUpArg->pEventObj, pStartUpArg->pFilter, 
				pIOService->m_ReadThreadVec, pIOService->m_WriteThreadVec);
		if(pKcpServer)
		{
		}
	}
	return NULL;
}

bool CIOService::StartUp(IOServiceType serviceType, const string &sSrcIP, int nSrcPort, 
		const string &sDstIP, int nDstPort, IOEvent *pEventObj, NetFilter *pFilter, int nThreadNum, 
		bool bSync)
{
	//保存参数
	m_StartUpArg.serviceType = serviceType;
	m_StartUpArg.mIPAddr.m_sLocalIP = sSrcIP;
	m_StartUpArg.bSync = bSync;
	stringstream str;
	str<<nSrcPort;
	m_StartUpArg.mIPAddr.m_sLocalPort = str.str();
	m_StartUpArg.mIPAddr.m_sRemoteIP = sDstIP;
	stringstream str1;
	str1<<nDstPort;
	m_StartUpArg.mIPAddr.m_sRemotePort = str1.str();
	m_StartUpArg.nThreadNum = nThreadNum;
	m_StartUpArg.pEventObj = pEventObj;
	m_StartUpArg.pFilter = pFilter;

	pthread_mutex_lock(&m_lock);

	//启动线程
	startThread();

	if(bSync)
	{
		pthread_cond_wait(&m_cond , &m_lock);
	}
	pthread_mutex_unlock(&m_lock);
	return m_bSuccess;
}

void CIOService::onTcpAcceptCallback(int fd, short which, void *arg)
{
	string sRemoteIP,sRemotePort;
	int nClientFD = CIOUtils::Accept(fd, sRemoteIP, sRemotePort);
	CIOService *pAcceptDispatcher = (CIOService*)arg;
	pAcceptDispatcher->dispatcherNewCon(
			nClientFD, SOCKETTYPE_ACCEPT, ENETWORKTYPE_TCP, sRemoteIP, sRemotePort);
}

void CIOService::dispatcherNewCon(int nfd, ESOCKETTYPE eSocketType, ENETWORKTYPE eNetworkType, 
			const string &sIP, const string &sPort)
{
	CQ_ITEM *item = new CQ_ITEM;
	item->pFilter = m_StartUpArg.pFilter;
	item->pIOEvent = m_StartUpArg.pEventObj;
	item->pWriteIOThread = m_WriteThreadVec[m_nCount];
	if(eSocketType == SOCKETTYPE_CONNECT)   //客户端调用CloseHandle需要删除IOService对象
	{
		m_StartUpArg.mIPAddr.m_sLocalIP = sIP;
		m_StartUpArg.mIPAddr.m_sLocalPort = sPort;

		printf("SOCKETTYPE_CONNECT m_sLocalIP = %s m_sLocalPort = %s m_sRemoteIP = %s "
				"m_sRemotePort = %s\n", m_StartUpArg.mIPAddr.m_sLocalIP.c_str(), 
				m_StartUpArg.mIPAddr.m_sLocalPort.c_str(), 
				m_StartUpArg.mIPAddr.m_sRemoteIP.c_str(), 
				m_StartUpArg.mIPAddr.m_sRemotePort.c_str());

		item->pIOService = this;
		item->mIPAddr = m_StartUpArg.mIPAddr;
	}
	else if(eSocketType == SOCKETTYPE_ACCEPT)
	{
		m_StartUpArg.mIPAddr.m_sRemoteIP = sIP;
		m_StartUpArg.mIPAddr.m_sRemotePort = sPort;

		printf("SOCKETTYPE_ACCEPT m_sLocalIP = %s m_sLocalPort = %s m_sRemoteIP = %s "
				"m_sRemotePort = %s\n", m_StartUpArg.mIPAddr.m_sLocalIP.c_str(), 
				m_StartUpArg.mIPAddr.m_sLocalPort.c_str(), 
				m_StartUpArg.mIPAddr.m_sRemoteIP.c_str(), 
				m_StartUpArg.mIPAddr.m_sRemotePort.c_str());
		item->mIPAddr = m_StartUpArg.mIPAddr;
	}
	item->nfd = nfd;
	item->eSocketType = eSocketType;
	item->eSocketStatus = SOCKETSTATUS_CONNECT;
	item->bDelMySelf = false;
	item->eNetworkType = eNetworkType;

#ifndef WIN32
	IBaseIOThread *pReadIOThread = (IBaseIOThread*)m_ReadThreadVec[m_nCount];
	int nSize = pReadIOThread->push((void*)item);
	if(nSize <= 1)       //写线程还有数据没有读, 不需要通知
	{
		if(write(pReadIOThread->notify_send_fd, "", 1) != 1) 
		{
			printf("send is failure error = %s\n", strerror(errno));
		}
	}
#else
	CBaseIOThreadWin* pReadIOThread = dynamic_cast<CBaseIOThreadWin*>(m_ReadThreadVec[m_nCount]);
	if (!pReadIOThread->postToIocp(item))
	{
		printf("0x%p postToIocp return false (0x%p)\n", pReadIOThread, item);
		assert(0);
	}
#endif

	m_nCount++;
	if(m_nCount>=m_StartUpArg.nThreadNum-1)
	{
		m_nCount = 0;
	}
}

