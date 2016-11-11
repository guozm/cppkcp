#include "KCPClient.h"
#include "kcpinc.h"
#include "KCPThread.h"
#include "KCPWriteThread.h"
#include <sstream>

CKCPClient::CKCPClient(int nSockfd, IOEvent *pIOEvent, 
		NetFilter *pNetFiler, vector<IBaseIOThread*> mReadVec, 
		vector<IBaseIOThread*> mWriteVec, CIOService *pIOService) : m_nSockfd(nSockfd), 
	m_pIOEvent(pIOEvent), m_pNetFiler(pNetFiler), 
	m_ReadThreadVec(mReadVec), m_WriteThreadVec(mWriteVec), m_pIOService(pIOService)
{
	printf("CKCPClient::CKCPClient nSockfd = %d this = %p", nSockfd, this);
	m_nReadCount = 0;
	m_nWriteCount = 0;
	m_pSession = NULL;
	m_bCreatedKCPObj = false;
	m_bCreateObjFailure = false;
	m_nTryCount = 0;
	m_bRelease = false;

	initPipeLibevent(onPipeProcessCallback, this);

	//启动线程
	startThread();

	notifyCreate();
}

CKCPClient::~CKCPClient()
{
	printf("CKCPClient::~CKCPClient this = %p", this);
	if(NULL == m_pSession)
	{
		if(m_pIOService)
		{
			delete m_pIOService;
			m_pIOService = NULL;
		}
	}
}

void CKCPClient::notifyCreate()
{
	CQ_ITEM *item = new CQ_ITEM;
	item->bDelMySelf = false;

#ifndef WIN32
	int nSize = push((void*)item);
	if(nSize <=1)
	{
		if(write(notify_send_fd, "", 1) != 1) 
		{
			printf("send is faild error = %s", strerror(errno)); 
		}
	}
#else
	if (!postToIocp(item))
		ASSERTS(false, "0x%p postToIocp return false (0x%p)", this, item);
#endif
}

void CKCPClient::pipeCreate()
{
	printf("CKCPClient::pipeCreate() this = %p", this);
	event_set(&m_Event, m_nSockfd, EV_READ | EV_PERSIST, onNetReadProcessCallback, (void *)this);
	event_base_set(base, &m_Event);
	if(event_add(&m_Event, 0) == -1) 
	{
		printf("%s", "event_add is failure ...");
		assert(0);
	}

	CKCPThread::getKCPThread()->addCreateKCPObj(this);
}

void CKCPClient::onRelease()
{
	if(m_bRelease)
	{
		printf("%s", "CKCPClient::onRelease m_bRelease == true return ...");
		return;
	}
	m_bRelease = true;

	if(m_bCreateObjFailure)
	{
		m_pIOEvent->OnIOEvent(NULL, IO_EVENT_OPEN_FAILED, NULL, 0);
	}

	CQ_ITEM *item = new CQ_ITEM;
	item->bDelMySelf = true;

#ifndef WIN32
	int nSize = push((void*)item);
	if(nSize <=1)
	{
		if(write(notify_send_fd, "", 1) != 1) 
		{
			printf("send is faild error = %s", strerror(errno)); 
		}
	}
#else
	if (!postToIocp(item))
		ASSERTS(false, "0x%p postToIocp return false (0x%p)", this, item);
#endif
}

void CKCPClient::delMySelf()
{
	CKCPThread::getKCPThread()->addKCPClientDelayDelObj(this);
}

void CKCPClient::onPipeProcessCallback(int fd, short which, void *arg)
{
	CKCPClient *pKCPClient = (CKCPClient*)arg;
	char buf[1];
	if(recv(fd, buf, 1, 0) != 1)
	{
		printf("recv is failure error = %s", strerror(errno));
	}

	CQ_ITEM *item = NULL;
	while((item = (CQ_ITEM*)pKCPClient->pop()) != NULL) 
	{
		if(item->bDelMySelf)
		{
			event_del(&pKCPClient->m_Event);
			pKCPClient->eventloopexit();
			break;
		}
		else
		{
			pKCPClient->pipeCreate();
		}

		if(item)
		{
			delete item;
			item = NULL;
		}
	}

	if(item)
	{
		delete item;
		item = NULL;
	}
}

void CKCPClient::onNetReadProcessCallback(int fd, short which, void *arg)
{
	CKCPClient *pKCPClient = (CKCPClient*)arg;
	struct sockaddr_in addr;
	socklen_t addrLen = sizeof(addr);
	memset(pKCPClient->sNetBuffer, 0, sizeof(pKCPClient->sNetBuffer));
	char *sBuf = pKCPClient->sNetBuffer;
	int nLen = recvfrom(
			fd, sBuf, sizeof(pKCPClient->sNetBuffer), 0, (struct sockaddr *)&addr, &addrLen);
	if(-1 == nLen)
	{
		int nError = 0;
#ifdef WIN32
		nError = WSAGetLastError();
		printf("recvfrom -1 == nLen strerror = %s errno = %d pKCPClient = %p m_pSession = %p", 
				"Connection refused", nError, pKCPClient, pKCPClient->m_pSession);
#else
		nError = errno;
		printf("recvfrom -1 == nLen strerror = %s errno = %d pKCPClient = %p m_pSession = %p", 
				strerror(errno), nError, pKCPClient, pKCPClient->m_pSession);
#endif
		
		if(111 == nError || 10054 == nError)  //Connection refused(linux(111), windows(10054))
		{
			if(!pKCPClient->m_pSession)
			{
				if(!pKCPClient->m_bCreateObjFailure)
				{
					CKCPThread::getKCPThread()->delCreateKCPObj(pKCPClient);
					pKCPClient->m_bCreateObjFailure = true;
				}

				if(false == pKCPClient->m_bRelease)
				{
					pKCPClient->stopThread();
				}
			}
			else
			{
				pKCPClient->m_pSession->m_nRecvHBCount = CKCPThread::getRecvHBTimeout();
			}
		}
		return;
	}

	struct SKcpHeader *pKcpHeader = (struct SKcpHeader*)sBuf;
	if(pKcpHeader->nKcpVersion != g_nKcpVersion)
	{
		printf("pKcpHeader->nKcpVersion = %d != g_nKcpVersion = %d", 
				pKcpHeader->nKcpVersion, g_nKcpVersion);
		return;
	}
	if(KCP_CREATERESPONSE == pKcpHeader->netEvent)
	{
		if(pKCPClient->m_bCreatedKCPObj == true) //收到重发
		{
			return;
		}
		bool bRet = CKCPThread::getKCPThread()->delCreateKCPObj(pKCPClient);
		if(!bRet)
		{
			printf("delCreateKCPObj return false pKCPClient = %p", pKCPClient);
			return;
		}

		struct SKcpCreateResultHeader *pKcpCreateHeader = (struct SKcpCreateResultHeader*)sBuf;
		stringstream strRemotePort;
		strRemotePort<<htons(addr.sin_port);

		IPAddr mIPAddr;
		mIPAddr.m_sRemoteIP = inet_ntoa(addr.sin_addr);
		mIPAddr.m_sRemotePort = strRemotePort.str();

		CSession *pSession = pKCPClient->newSession( 
				ENETWORKTYPE_KCP, mIPAddr, pKcpCreateHeader->serverObj);
		pSession->m_nKcpServerVersion = pKcpCreateHeader->nKcpServerVersion;

		printf("RECV KCP_CREATERESPONSE m_sRemoteIP = %s m_sRemotePort = %d pSession = %p "
				"serverpSession = %p nKcpNum = %lld nKcpServerVersion = %d", 
				inet_ntoa(addr.sin_addr), htons(addr.sin_port), pSession, 
				(CSession*)pKcpCreateHeader->serverObj, pKcpCreateHeader->KCP_number, 
				pKcpCreateHeader->nKcpServerVersion);

		//kcp设置
		pSession->createKcpObj(pKcpCreateHeader->KCP_number);
		pSession->m_pkcp->output = CKCPClient::udp_output;
		CKCPThread::getKCPThread()->addKCPUpdateObj(pSession);

		pKCPClient->m_bCreatedKCPObj = true;

		//通过kcp通知Create
		pSession->sendKcpCreate();

		pSession->Register(pKCPClient->m_pIOEvent, pKCPClient->m_pNetFiler);
		pSession->onKcpConnectSuccess(fd, pKCPClient->m_pIOService);

		//定时发送心跳
		CKCPThread::getKCPThread()->addSendHBObj(pSession);
	}
	else if(KCP_DATA == pKcpHeader->netEvent)
	{
		CSession *pSession = pKCPClient->m_pSession;
		if(!pSession)
		{
			printf("%s", "pSession == NULL return ...");
			return;
		}

		ikcpcb *kcpObj = NULL;
		auto_ptr<CMutexProxy> mLock(pSession->getKcpObj(&kcpObj));
		if(!kcpObj)
		{
			printf("kcpObj == NULL pSession = %p", pSession);
			return;
		}
		ikcp_input(kcpObj, sBuf+sizeof(SKcpDataHeader), nLen-sizeof(SKcpDataHeader));
		while(1)
		{
			memset(pKCPClient->sKcpBuffer, 0, sizeof(pKCPClient->sKcpBuffer));
			char *sKcpBuf = pKCPClient->sKcpBuffer;
			int nRet = ikcp_recv(kcpObj, sKcpBuf, sizeof(pKCPClient->sKcpBuffer));
			if(nRet < 0)
			{
				break;
			}

			if(pSession->m_bClose || pSession->m_bSendCloseHandle)
			{
				return;
			}

			pSession->m_nRecvHBCount = 0;
			struct SKcpHeader *pTmpKcpHeader = (struct SKcpHeader*)sKcpBuf;
			if(KCP_DATA == pTmpKcpHeader->netEvent)
			{
				pSession->onRecvKcpData(sKcpBuf+sizeof(SKcpHeader), nRet-sizeof(SKcpHeader));
			}
			else if(KCP_DATAMSG == pTmpKcpHeader->netEvent)
			{
				pSession->onRecvKcpDataMsg(sKcpBuf+sizeof(SKcpHeader), nRet-sizeof(SKcpHeader));
			}
			else if(KCP_CLOSE == pTmpKcpHeader->netEvent)
			{
				pKCPClient->m_pSession->onRecvClose();
			}
			else if(KCP_HEARTBEAT == pTmpKcpHeader->netEvent)
			{
				//nRet = send(pSession->m_nfd, writeItem->buf, writeItem->len, 0);
			}
		}
	}
	else if(KCP_UNRELIABLEDATA == pKcpHeader->netEvent)
	{
		CSession *pSession = pKCPClient->m_pSession;
		pSession->m_nRecvHBCount = 0;
		pSession->onRecvKcpData(sBuf+sizeof(SKcpDataHeader), nLen-sizeof(SKcpDataHeader));
	}
	else if(KCP_NETDETECT == pKcpHeader->netEvent)
	{
		//直接发送
		send(fd, sBuf, nLen, 0);
	}
}

CSession *CKCPClient::newSession(ENETWORKTYPE eNetworkType, 
		const IPAddr &mIPAddr, long long nServerObj)
{
	IBaseIOThread *pWriteIOThread = m_WriteThreadVec[m_nWriteCount];
	m_pSession = new CSession(pWriteIOThread, m_ReadThreadVec[m_nReadCount], 
			eNetworkType, mIPAddr, nServerObj, this);
	m_nWriteCount++;
	if(m_nWriteCount>=m_WriteThreadVec.size())
	{
		m_nWriteCount = 0;
	}

	m_nReadCount++;
	if(m_nReadCount>=m_ReadThreadVec.size())
	{
		m_nReadCount = 0;
	}

	//写事件
	((CKCPWriteThread*)pWriteIOThread)->setWriteEvent(m_nSockfd, m_pSession);
	return m_pSession;
}

bool CKCPClient::sendCreateObjMsg()
{
	printf("%s", "CKCPClient::sendCreateObjMsg KCP_CREATE");
	bool bRet = false;
	struct SKcpHeader kcpCreateHeader;
	kcpCreateHeader.netEvent = KCP_CREATE;
	int nRet = send(m_nSockfd, (const char*)&kcpCreateHeader, sizeof(kcpCreateHeader),0);
	if(nRet > 0)
	{
		bRet = true;
	}
	return bRet;
}

int CKCPClient::udp_output(const char *sBuf, int nLen, ikcpcb *kcp, void *user)
{
	CSession *pSession = (CSession*)user;
	struct SKcpDataHeader mKcpDataHeader;
	mKcpDataHeader.netEvent = KCP_DATA;
	mKcpDataHeader.nKcpVersion = pSession->m_nKcpServerVersion;
	if(0 != pSession->m_nServerObj)
	{
		mKcpDataHeader.nServerObj = pSession->m_nServerObj;
	}

	char *sOut = NULL;
	int nOutLen = 0;
	makePacket(mKcpDataHeader, sBuf, nLen, &sOut, nOutLen);

	CQ_WRITE_ITEM *item = new CQ_WRITE_ITEM;
	item->buf = sOut;
	item->len = nOutLen;
	int nSize = pSession->pushWriteQue(item);
	if(nSize > 2)
	{
		return 0;
	}

	CQ_NOTIFYWRITE_ITEM *notifyitem = new CQ_NOTIFYWRITE_ITEM;
	notifyitem->pSession = pSession;
	notifyitem->eSocketStatus = SOCKETSTATUS_NETWORKDATA;
	notifyitem->bDelMySelf = false;
#ifndef WIN32
	auto_ptr<CMutexProxy> pLock(pSession->m_pWriteThread->push((void*)notifyitem, nSize));
	if(nSize <= 1)       //读线程还有数据没有读, 不需要通知
	{
		if(write(pSession->m_pWriteThread->notify_send_fd, "", 1) != 1) 
		{
			printf("send is failure error = %s", strerror(errno));
		}
	}
#else
	if (!((CBaseIOThreadWin*)pSession->m_pWriteThread)->postToIocp(notifyitem))
		ASSERTS(false, "0x%p postToIocp return false (0x%p)", pSession->m_pWriteThread, notifyitem);
#endif
	return 0;
}
