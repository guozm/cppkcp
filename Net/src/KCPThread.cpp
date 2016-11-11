#include "KCPThread.h"
#include "kcpinc.h"
#include "KCPClient.h"
CKCPThread *CKCPThread::m_pKCPThread = NULL;

void CKCPThread::addKCPUpdateObj(CSession* pObj)
{
	CMutexProxy mLock(m_UpdateMutex);
	vector<CSession*>::iterator its = find(m_UpdateObjVec.begin(), m_UpdateObjVec.end(), pObj);
	if(its == m_UpdateObjVec.end())
	{
		m_UpdateObjVec.push_back(pObj);
	}
}

void CKCPThread::delKCPUpdateObj(CSession* pObj)
{
	CMutexProxy mLock(m_UpdateMutex);
	vector<CSession*>::iterator its = find(m_UpdateObjVec.begin(), m_UpdateObjVec.end(), pObj);
	if(its != m_UpdateObjVec.end())
	{
		m_UpdateObjVec.erase(its);
	}
}

void CKCPThread::addSendHBObj(CSession* pObj)
{
	if(1)
	{
		CMutexProxy mLock(m_SendHBMutex);
		vector<CSession*>::iterator its = find(m_SendHBObjVec.begin(), m_SendHBObjVec.end(), pObj);
		if(its == m_SendHBObjVec.end())
		{
			m_SendHBObjVec.push_back(pObj);
		}
	}

	addRecvHBObj(pObj);
}

void CKCPThread::delSendHBObj(CSession* pObj, bool bDelRecvHBObj)
{
	printf("CKCPThread::delSendHBObj bDelRecvHBObj = %d", bDelRecvHBObj);
	if(1)
	{
		CMutexProxy mLock(m_SendHBMutex);
		vector<CSession*>::iterator its = find(m_SendHBObjVec.begin(), m_SendHBObjVec.end(), pObj);
		if(its != m_SendHBObjVec.end())
		{
			m_SendHBObjVec.erase(its);
		}
	}

	if(bDelRecvHBObj)
	{
		delRecvHBObj(pObj);
	}
}

void CKCPThread::addRecvHBObj(CSession* pObj)
{
	printf("CKCPThread::addRecvHBObj pObj = %p", pObj);
	CMutexProxy mLock(m_RecvHBMutex);
	vector<CSession*>::iterator its = find(m_RecvHBObjVec.begin(), m_RecvHBObjVec.end(), pObj);
	if(its == m_RecvHBObjVec.end())
	{
		m_RecvHBObjVec.push_back(pObj);
	}
}

void CKCPThread::delRecvHBObj(CSession* pObj)
{
	CMutexProxy mLock(m_RecvHBMutex);
	vector<CSession*>::iterator its = find(m_RecvHBObjVec.begin(), m_RecvHBObjVec.end(), pObj);
	if(its != m_RecvHBObjVec.end())
	{
		m_RecvHBObjVec.erase(its);
	}
}

void CKCPThread::addNetDelayDelObj(CSession* pObj)
{
	int nDelayDelTime = 0;
	ESOCKETTYPE eSockType = pObj->getSocketType();
	if(SOCKETTYPE_CONNECT == eSockType)
	{
		nDelayDelTime = 12;
	}
	else
	{
		nDelayDelTime = 18;
	}

	CMutexProxy mLock(m_DelayMutex);
	map<CSession*, int>::iterator its = m_DelaySessionMap.find(pObj);
	if(its == m_DelaySessionMap.end())
	{
		printf("CKCPThread::addNetDelayDelObj pObj = %p nDelayDelTime = %d", pObj, nDelayDelTime);
		m_DelaySessionMap.insert(make_pair(pObj, nDelayDelTime));
	}
}

void CKCPThread::dealNetDelayDelObj()
{
	map<CSession*, int>::iterator its;
	for(its=m_DelaySessionMap.begin(); its!=m_DelaySessionMap.end();)
	{
		if(0 == (--its->second))
		{
			CSession *pSession = its->first;
			if(pSession->m_pKCPClient)
			{
				pSession->m_pKCPClient->stopThread();
			}

			SServerObj *pSServerObj = pSession->getSServerObj();
			if(pSServerObj)
			{
				pSServerObj->bDel = true;
				addSServerObjDelayDelObj(pSServerObj);
			}

			delete pSession;
			pSession = NULL;
			m_DelaySessionMap.erase(its++);
		}
		else
		{
			its++;
		}
	}
}

void CKCPThread::addSServerObjDelayDelObj(SServerObj *pObj)
{
	//保存
	m_DelaySServerObjVec.push_back(make_pair(pObj, time(0)));

	vector<pair<SServerObj*, long> >::iterator its;
	for(its=m_DelaySServerObjVec.begin(); its!=m_DelaySServerObjVec.end();)
	{
		//24小时后删除
		if(time(0)-its->second > 86400)
		{
			printf("its->first = %p is del ...", its->first);
			delete its->first;
			its = m_DelaySServerObjVec.erase(its);
		}
		else
		{
			return;
		}
	}
}

void CKCPThread::addKCPClientDelayDelObj(CKCPClient *pKCPClient)
{
	int nDelayDelTime = 2;
	CMutexProxy mLock(m_DelayMutex);
	map<CKCPClient*, int>::iterator its = m_DelayKCPClientMap.find(pKCPClient);
	if(its == m_DelayKCPClientMap.end())
	{
		printf("CKCPThread::addKCPClientDelayDelObj pKCPClient = %p nDelayDelTime = %d", 
				pKCPClient, nDelayDelTime);
		m_DelayKCPClientMap.insert(make_pair(pKCPClient, nDelayDelTime));
	}
}

void CKCPThread::dealKCPClientDelayDelObj()
{
	map<CKCPClient*, int>::iterator its;
	for(its=m_DelayKCPClientMap.begin(); its!=m_DelayKCPClientMap.end();)
	{
		if(0 == (--its->second))
		{
			CKCPClient *pKCPClient = its->first;
			delete pKCPClient;
			pKCPClient = NULL;
			m_DelayKCPClientMap.erase(its++);
		}
		else
		{
			its++;
		}
	}
}

void CKCPThread::addCreateKCPObj(CKCPClient *pKCPClient)
{
	CMutexProxy mLock(m_CreateObjMutex);
	vector<CKCPClient*>::iterator its = 
		find(m_CreateObjVec.begin(), m_CreateObjVec.end(), pKCPClient);
	if(its == m_CreateObjVec.end())
	{
		m_CreateObjVec.push_back(pKCPClient);
	}
	else
	{
		printf("CKCPThread::addCreateKCPObj its != m_CreateObjVec pKCPClient = %p", pKCPClient);
	}
}

bool CKCPThread::delCreateKCPObj(CKCPClient *pKCPClient)
{
	bool bRet = false;
	CMutexProxy mLock(m_CreateObjMutex);
	vector<CKCPClient*>::iterator its = 
		find(m_CreateObjVec.begin(), m_CreateObjVec.end(), pKCPClient);
	if(its != m_CreateObjVec.end())
	{
		bRet = true;
		m_CreateObjVec.erase(its);
	}
	return bRet;
}

void *CKCPThread::updateThreadFun(void *argc)
{
	CKCPThread *pKCPThread = (CKCPThread*)argc;
	while(1)
	{
		if(1)
		{
			CMutexProxy mLock(pKCPThread->m_UpdateMutex);
			for(size_t i=0; i<pKCPThread->m_UpdateObjVec.size(); i++)
			{
				if(1)
				{
					ikcpcb *kcpObj = NULL;
					CSession *pSession = pKCPThread->m_UpdateObjVec[i];
					auto_ptr<CMutexProxy> mLock(pSession->getKcpObj(&kcpObj));
					ikcp_update(kcpObj, iclock());
				}
			}
		}
#ifdef WIN32
		Sleep(10);
#else
		usleep(10);
#endif
	}
	return NULL;
}

void *CKCPThread::threadFun(void *argc)
{
	CKCPThread *pKCPThread = (CKCPThread*)argc;
	while(1)
	{
		//sendheartbeat
		pKCPThread->dealSendHeartbeat();

		//recvheartbeat
		pKCPThread->dealRecvHeartbeat();

		//发送创建KCPObj 
		pKCPThread->dealCreateKCPObj();

#ifdef WIN32
		Sleep(1000);
#else
		sleep(1);
#endif
	}
	return NULL;
}

void *CKCPThread::delayThreadFun(void *argc)
{
	CKCPThread *pKCPThread = (CKCPThread*)argc;
	while(1)
	{
		if(1)
		{
			CMutexProxy mLock(pKCPThread->m_DelayMutex);
			pKCPThread->dealNetDelayDelObj();
			pKCPThread->dealKCPClientDelayDelObj();
		}
#ifdef WIN32
		Sleep(10000);
#else
		sleep(10);
#endif
	}
	return NULL;
}

void CKCPThread::dealSendHeartbeat()
{
	CMutexProxy mLock(m_SendHBMutex);
	for(size_t i=0; i<m_SendHBObjVec.size(); i++)
	{
		m_SendHBObjVec[i]->sendKcpHeartbeat();
	}
}

void CKCPThread::dealRecvHeartbeat()
{
	CMutexProxy mLock(m_RecvHBMutex);
	vector<CSession*>::iterator its;
	for(its=m_RecvHBObjVec.begin(); its!=m_RecvHBObjVec.end();)
	{
		CSession *pSession = *its;
		if(RECVHBTIMEOUT <= ++(pSession->m_nRecvHBCount))
		{
			printf("pSession = %p heartbeat is timeout ....", pSession);
			delSendHBObj(pSession, false);

			//清空kcp对象
			pSession->releaseKcp();

			pSession->onRecvKcpClose();
			its = m_RecvHBObjVec.erase(its);
		}
		else
		{
			its++;
		}
	}
}

void CKCPThread::dealCreateKCPObj()
{
	CMutexProxy mLock(m_CreateObjMutex);
	vector<CKCPClient*>::iterator its;
	for(its=m_CreateObjVec.begin(); its!=m_CreateObjVec.end();)
	{
		CKCPClient *pKCPClient = *its;
		if(pKCPClient->m_nTryCount >= 6)
		{
			printf("pKCPClient = %p nTryCount >= 6", pKCPClient);
			pKCPClient->m_bCreateObjFailure = true;
			pKCPClient->stopThread();
			its = m_CreateObjVec.erase(its);
			continue;
		}

		pKCPClient->sendCreateObjMsg();
		pKCPClient->m_nTryCount++;
		its++;
	}
}
