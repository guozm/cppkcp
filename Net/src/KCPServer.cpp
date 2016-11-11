#include "KCPServer.h"
#include "kcpinc.h"
#include "KCPThread.h"
#include "KCPWriteThread.h"
#include <sstream>

CKCPServer::CKCPServer(int nSockfd, IOEvent *pIOEvent, 
		NetFilter *pNetFiler, vector<IBaseIOThread*> mReadVec, 
		vector<IBaseIOThread*> mWriteVec) : 
	m_pIOEvent(pIOEvent), m_pNetFiler(pNetFiler), 
	m_ReadThreadVec(mReadVec), m_WriteThreadVec(mWriteVec)
{
	printf("CKCPServer::CKCPServer nSockfd = %d", nSockfd);
	m_nReadCount = 0;
	m_nWriteCount = 0;
	m_nKcpServerVersion = time(0);
	memset(sNetBuffer, 0, sizeof(sNetBuffer));
	initPipeLibevent(onPipeProcessCallback, this);

	//启动线程
	startThread();

	//读事件
	event_set(&m_Event, nSockfd, EV_READ | EV_PERSIST, onNetReadProcessCallback, (void *)this); 

	event_base_set(base, &m_Event);
	if(event_add(&m_Event, 0) == -1) 
	{
		printf("%s", "event_add is failure ...");
		assert(0);
	}
}

CKCPServer::~CKCPServer()
{
}

bool CKCPServer::getAddrSessionMap_r(const string &sAddr, SServerObj **pObj)
{
	CMutexProxy mLock(m_Mutex);
	bool bRet = false;
	map<string, SServerObj*>::iterator its = m_pAddrSServerObjMap.find(sAddr);
	if(its != m_pAddrSServerObjMap.end())
	{
		*pObj = its->second;
		bRet = true;
	}
	return bRet;
}

void CKCPServer::addAddrSServerObjMap_r(const string &sAddr, SServerObj *pObj)
{
	CMutexProxy mLock(m_Mutex);
	map<string, SServerObj*>::iterator its = m_pAddrSServerObjMap.find(sAddr);
	if(its == m_pAddrSServerObjMap.end())
	{
		m_pAddrSServerObjMap.insert(make_pair(sAddr, pObj));
	}
}

void CKCPServer::delAddrSessionMap_r(const string &sAddr)
{
	CMutexProxy mLock(m_Mutex);
	map<string, SServerObj*>::iterator its = m_pAddrSServerObjMap.find(sAddr);
	if(its != m_pAddrSServerObjMap.end())
	{
		m_pAddrSServerObjMap.erase(sAddr);
	}
}

CSession *CKCPServer::newSession(ENETWORKTYPE eNetworkType, 
		const IPAddr &mIPAddr, struct sockaddr_in addr, int nfd)
{
	IBaseIOThread *pWriteIOThread = m_WriteThreadVec[m_nWriteCount];
	CSession *pSession = new CSession(pWriteIOThread, m_ReadThreadVec[m_nReadCount], 
			eNetworkType, mIPAddr, addr, nfd, SOCKETTYPE_ACCEPT);
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
	((CKCPWriteThread*)pWriteIOThread)->setWriteEvent(nfd, pSession);
	return pSession;
}

void CKCPServer::onPipeProcessCallback(int fd, short which, void *arg)
{
}

void CKCPServer::onNetReadProcessCallback(int fd, short which, void *arg)
{
	CKCPServer *pKcpServer = (CKCPServer*)arg;
	struct sockaddr_in addr;
	socklen_t addrLen = sizeof(addr);
	memset(pKcpServer->sNetBuffer, 0, sizeof(pKcpServer->sNetBuffer));
	char *sBuf = pKcpServer->sNetBuffer;
	int nLen = recvfrom(
			fd, sBuf, sizeof(pKcpServer->sNetBuffer), 0, (struct sockaddr *)&addr, &addrLen);
	if(-1 == nLen)
	{
		assert(0);
		return;
	}

	struct SKcpHeader *pKcpHeader = (struct SKcpHeader*)sBuf;
	if(KCP_CREATE == pKcpHeader->netEvent)
	{
		if(pKcpHeader->nKcpVersion != g_nKcpVersion)
		{
			printf("pKcpHeader->nKcpVersion = %d != g_nKcpVersion = %d", 
					pKcpHeader->nKcpVersion, g_nKcpVersion);
			return;
		}
	}
	else if(KCP_DATA == pKcpHeader->netEvent)
	{
		if(pKcpHeader->nKcpVersion != pKcpServer->m_nKcpServerVersion)
		{
			printf("pKcpHeader->nKcpVersion = %d != pKcpServer->m_nKcpServerVersion = %d", 
					pKcpHeader->nKcpVersion, pKcpServer->m_nKcpServerVersion);
			return;
		}
	}

	if(KCP_CREATE == pKcpHeader->netEvent)
	{
		stringstream strRemotePort;
		strRemotePort<<htons(addr.sin_port);

		stringstream str;
		str<<inet_ntoa(addr.sin_addr)<<":"<<strRemotePort.str();
		SServerObj *pSServerObj = NULL;
		bool bRet = pKcpServer->getAddrSessionMap_r(str.str(), &pSServerObj);
		if(bRet && pSServerObj)  //收到重发
		{
			//通知kcpclient收到消息
			SKcpCreateResultHeader mHeader;
			mHeader.netEvent = KCP_CREATERESPONSE;
			mHeader.serverObj = (long long)pSServerObj;
			mHeader.KCP_number = ((CSession*)(pSServerObj->pSession))->m_nKcpNum;
			mHeader.nKcpServerVersion = pKcpServer->m_nKcpServerVersion;
			sendto(fd, (const char*)&mHeader, sizeof(mHeader), 0, (sockaddr*)&addr, sizeof(addr));
			return;
		}

		IPAddr mIPAddr;
		mIPAddr.m_sRemoteIP = inet_ntoa(addr.sin_addr);
		mIPAddr.m_sRemotePort = strRemotePort.str();

		CSession *pSession = pKcpServer->newSession(ENETWORKTYPE_KCP, mIPAddr, addr, fd);

		//生成SserverObj
		SServerObj *pSserverObj = new SServerObj(pSession);
		pSession->setKCPServerObj(pSserverObj);
		pKcpServer->addAddrSServerObjMap_r(str.str(), pSserverObj);

		//定时发送心跳
		CKCPThread::getKCPThread()->addSendHBObj(pSession);

		long long nKcpNum = 0;
		stringstream num;
		num<<time(0)<<(long long)pSession;
		sscanf(num.str().c_str(), "%lld", &nKcpNum);

		//kcp设置
		pSession->createKcpObj(nKcpNum);
		pSession->m_pkcp->output = CKCPServer::udp_output;
		CKCPThread::getKCPThread()->addKCPUpdateObj(pSession);


		//通知kcpclient收到消息
		SKcpCreateResultHeader mHeader;
		mHeader.netEvent = KCP_CREATERESPONSE;
		mHeader.serverObj = (long long)pSserverObj;
		mHeader.KCP_number = nKcpNum;
		mHeader.nKcpServerVersion = pKcpServer->m_nKcpServerVersion;
		sendto(fd, (const char*)&mHeader, sizeof(mHeader), 0, (sockaddr*)&addr, sizeof(addr));
	}
	else if(KCP_DATA == pKcpHeader->netEvent)
	{
		struct SKcpDataHeader *pKcpDataHeader = (struct SKcpDataHeader*)sBuf;
		SServerObj *pSServerObj = (SServerObj*)pKcpDataHeader->nServerObj;
		if(pSServerObj->bDel)
		{
			printf("pSServerObj = %p is del ....", pSServerObj);
			return;
		}

		CSession *pSession = (CSession*)pSServerObj->pSession;
#if 0
		int nn = iclock();
		int nCurTime = ikcp_check(pSession->m_pkcp, iclock());
		int nDecTime = nCurTime-nn;
		printf("nCurTime = 0x%x nDecTime = 0x%x kcpObj->current = 0x%x nn = 0x%x\n", 
				nCurTime, nDecTime, nn, nn);
#endif

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
			memset(pKcpServer->sKcpBuffer, 0, sizeof(pKcpServer->sKcpBuffer));
			char *sKcpBuf = pKcpServer->sKcpBuffer;
			int nRet = ikcp_recv(kcpObj, sKcpBuf, sizeof(pKcpServer->sKcpBuffer));
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
			if(KCP_CREATE == pTmpKcpHeader->netEvent)
			{
				pSession->Register(pKcpServer->m_pIOEvent, pKcpServer->m_pNetFiler);
				pSession->onNetKcpConnect();

				printf("RECV KCP_CREATE onNetKcpConnect ... m_sRemoteIP = %s m_sRemotePort = %d "
						"pSession = %p m_nKcpNum = %lld" ,inet_ntoa(addr.sin_addr), 
						htons(addr.sin_port), pSession, pSession->m_nKcpNum);

				stringstream str;
				str<<inet_ntoa(addr.sin_addr)<<":"<<htons(addr.sin_port);
				pKcpServer->delAddrSessionMap_r(str.str());
			}
			else if(KCP_DATA == pTmpKcpHeader->netEvent)
			{
				pSession->onRecvKcpData(sKcpBuf+sizeof(SKcpHeader), nRet-sizeof(SKcpHeader));
			}
			else if(KCP_DATAMSG == pTmpKcpHeader->netEvent)
			{
				pSession->onRecvKcpDataMsg(sKcpBuf+sizeof(SKcpHeader), nRet-sizeof(SKcpHeader));
			}
			else if(KCP_CLOSE == pTmpKcpHeader->netEvent)
			{
				pSession->onRecvClose();
			}
		}
	}
	else if(KCP_UNRELIABLEDATA == pKcpHeader->netEvent)
	{
		struct SKcpDataHeader *pKcpDataHeader = (struct SKcpDataHeader*)sBuf;
		SServerObj *pSServerObj = (SServerObj*)pKcpDataHeader->nServerObj;
		if(pSServerObj->bDel)
		{
			printf("pSServerObj = %p is del ....", pSServerObj);
			return;
		}

		CSession *pSession = (CSession*)pSServerObj->pSession;
		pSession->m_nRecvHBCount = 0;
		pSession->onRecvKcpData(sBuf+sizeof(SKcpDataHeader), nLen-sizeof(SKcpDataHeader));
	}
}

int CKCPServer::udp_output(const char *sBuf, int nLen, ikcpcb *kcp, void *user)
{
	CSession *pSession = (CSession*)user;
	struct SKcpDataHeader mKcpDataHeader;
	mKcpDataHeader.netEvent = KCP_DATA;
	mKcpDataHeader.nKcpVersion = g_nKcpVersion;

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

void CKCPServer::onRelease()
{
}
