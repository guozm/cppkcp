#include "KCPWriteThread.h"
#include "Session.h"
#include "KCPThread.h"

#ifdef WIN32
#define MSG_NOSIGNAL 0
#endif

#ifdef WIN32
#include <io.h>
#define EWOULDBLOCK WSAEWOULDBLOCK
#else
#include <unistd.h>
#endif

CKCPWriteThread::CKCPWriteThread()
{
	initPipeLibevent(onPipeProcessCallback, this);
}

CKCPWriteThread::~CKCPWriteThread()
{
	printf("CKCPWriteThread::~CKCPWriteThread\n");
}

void CKCPWriteThread::onRelease()
{
	//通知析够此对象
	CQ_NOTIFYWRITE_ITEM *notifyitem = new CQ_NOTIFYWRITE_ITEM;
	notifyitem->bDelMySelf = true;

#ifndef WIN32
	int nSize = push((void*)notifyitem);
	if(nSize <= 1)
	{
		if(write(notify_send_fd, "", 1) != 1) 
		{
			printf("send is failure error = %s", strerror(errno));
		}
	}
#else
	if (!postToIocp(notifyitem))
		ASSERTS(false, "0x%p postToIocp return false (0x%p)", this, notifyitem);
#endif
}

void CKCPWriteThread::onPipeProcessCallback(int fd, short which, void *arg)
{
	char buf[1];
	if(recv(fd, buf, 1, 0) != 1)
	{
		printf("recv is failure error = %s", strerror(errno));
	}

	CKCPWriteThread *pWriteIOThread = (CKCPWriteThread*)arg;
	CQ_NOTIFYWRITE_ITEM *notifyitem = NULL;
	while((notifyitem = (CQ_NOTIFYWRITE_ITEM*)pWriteIOThread->pop()) != NULL) 
	{
		if(notifyitem->bDelMySelf) 
		{
			pWriteIOThread->eventloopexit();
			break;
		}

		CSession *pSession = (CSession*)notifyitem->pSession;
		ESOCKETSTATUS eSocketStatus = notifyitem->eSocketStatus;
		if(SOCKETSTATUS_NETWORKDATA == eSocketStatus)
		{
			event_add(&pSession->m_writeEvent, NULL);
		}
		else if(SOCKETSTATUS_KCPCREATE == eSocketStatus ||
				SOCKETSTATUS_KCPCLOSE == eSocketStatus ||
				SOCKETSTATUS_HEARTBEAT == eSocketStatus)
		{
			if(SOCKETSTATUS_KCPCLOSE == eSocketStatus)
			{
				if(0 == pSession->m_nServerObj)
				{
					printf("SEND SOCKETSTATUS_KCPCLOSE pSession = %p m_nKcpNum = %lld", 
							pSession, pSession->m_nKcpNum);
				}
				else
				{
					printf("SEND SOCKETSTATUS_KCPCLOSE pSession = %p serverpSession = %p "
							"m_nKcpNum = %lld", pSession, (CSession*)pSession->m_nServerObj, 
							pSession->m_nKcpNum);
				}
				pSession->onWriteThreadCloseOK();
			}

			char *sOut = NULL;
			int nOutLen = 0;
			makePacket(socketStstusToKCPNetEvent(eSocketStatus), &sOut, nOutLen);
			dokcp(pSession, sOut, nOutLen);
		}
		else if(SOCKETSTATUS_NOTIFYKCPCLOSE == eSocketStatus)
		{
			pSession->onWriteThreadCloseOK();
		}
		else
		{
			char *sOut = NULL;
			int nOutLen = 0;
			if(eSocketStatus == SOCKETSTATUS_UNRELIABLEDATA)
			{
				doUnreliabledata(pSession, notifyitem->buf, notifyitem->len);
			}
			else if(eSocketStatus == SOCKETSTATUS_DATA || 
					eSocketStatus == SOCKETSTATUS_DATAMSG)
			{
				makePacket(socketStstusToKCPNetEvent(eSocketStatus), notifyitem->buf, 
						notifyitem->len, &sOut, nOutLen);
				dokcp(pSession, sOut, nOutLen);
			}
			else
			{
				assert(0);
			}

			if(!pSession->m_bClose)
			{
				//发到kcp队列通知调用者发送成功
				if(eSocketStatus == SOCKETSTATUS_DATA 
						|| eSocketStatus == SOCKETSTATUS_UNRELIABLEDATA)
				{
					pSession->onWriteOK(notifyitem->buf, notifyitem->len);
				}
				else if(eSocketStatus == SOCKETSTATUS_DATAMSG)
				{
					pSession->onWriteMsgOK(notifyitem->buf, notifyitem->len);
				}
			}
		}

		if(notifyitem)
		{
			delete notifyitem;
			notifyitem = NULL;
		}
	}

	if(notifyitem)
	{
		delete notifyitem;
		notifyitem = NULL;
	}
}

void CKCPWriteThread::setWriteEvent(int nSockfd, CSession *pSession)
{
	//写事件
	event_set(&pSession->m_writeEvent, nSockfd, EV_WRITE, onNetWriteProcessCallback, 
			(void *)pSession);
	event_base_set(base, &pSession->m_writeEvent);
}

void CKCPWriteThread::onNetWriteProcessCallback(int fd, short which, void *arg)
{
	CSession *pSession = (CSession*)arg;
	CQ_WRITE_ITEM *writeItem = pSession->writeQueFront();
	if(NULL == writeItem)
	{
		return;
	}

	int nRet = -1;
	string str="NULL";
	if(SOCKETTYPE_ACCEPT == pSession->m_eSocketType)
	{
		nRet = sendto(pSession->m_nfd, writeItem->buf, writeItem->len, 0, 
				(sockaddr*)&pSession->m_addr, sizeof(pSession->m_addr));
		if(nRet<=0)
		{
			str = "SOCKETTYPE_ACCEPT";
		}
	}
	else if(SOCKETTYPE_CONNECT == pSession->m_eSocketType)
	{
		nRet = send(pSession->m_nfd, writeItem->buf, writeItem->len, 0);
		if(nRet<=0)
		{
			str = "SOCKETTYPE_CONNECT";
		}
	}
	if(nRet<=0)
	{
		bool bInvalidArgument = false;       //当切网络发送失败
#ifdef WIN32
		if(10022 == WSAGetLastError())
		{
			bInvalidArgument = true;
		}

		printf("send is falure nRet = %d nSockfd = %d m_eSocketType = %s errno = %d "
				"nOutLen = %d", nRet, pSession->m_nfd, str.c_str(), WSAGetLastError(), 
				writeItem->len);
#else
		printf("send is falure nRet = %d nSockfd = %d m_eSocketType = %s errno = %d "
				"strerror(errno) = %s nOutLen = %d", nRet, pSession->m_nfd, str.c_str(), errno, 
				strerror(errno), writeItem->len);
#ifdef ANDROID
		if(22 == errno)
		{
			bInvalidArgument = true;
		}
#endif

#ifdef IOS
		if(49 == errno)
		{
			bInvalidArgument = true;
		}
#endif
#endif
		
		//不进行重新发送
		if(bInvalidArgument)
		{
			writeItem->bSendOk = true;
		}
	}
	else
	{
		writeItem->bSendOk = true;
	}
	event_add(&pSession->m_writeEvent, NULL);
}

void CKCPWriteThread::dokcp(CSession *pSession, const char *sBuf, int nLen)
{
	ikcpcb *kcpObj = NULL;
	auto_ptr<CMutexProxy> mLock(pSession->getKcpObj(&kcpObj));
	if(kcpObj)
	{
		int nRet = ikcp_send(kcpObj, sBuf, nLen);
		if(nRet != 0)
		{
			//todo
			assert(0);
		}
	}
	else
	{
		printf("CKCPWriteThread::dokcp kcpObj == NULL pSession = %p", pSession);
	}

	if(sBuf)
	{
		delete [] sBuf;
		sBuf = NULL;
	}
}

void CKCPWriteThread::doUnreliabledata(CSession *pSession, const char *sBuf, int nLen)
{
	struct SKcpDataHeader mKcpDataHeader;
	mKcpDataHeader.netEvent = KCP_UNRELIABLEDATA;
	mKcpDataHeader.nKcpVersion = pSession->m_nKcpServerVersion;
	if(0 != pSession->m_nServerObj)
	{
		mKcpDataHeader.nServerObj = pSession->m_nServerObj;
	}
	else
	{
		mKcpDataHeader.nKcpVersion = g_nKcpVersion;
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
		return;
	}
	event_add(&pSession->m_writeEvent, NULL);
}


