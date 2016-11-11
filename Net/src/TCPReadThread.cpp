#include "TCPReadThread.h"
#include "TCPWriteThread.h"
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

CTCPReadThread::CTCPReadThread() 
{
	initPipeLibevent(onPipeProcessCallback, this);
}

CTCPReadThread::~CTCPReadThread()
{
}

void CTCPReadThread::onRelease()
{
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

void CTCPReadThread::addAliveSockfdVec(int nfd)
{
	m_AliveSockfdVec.push_back(nfd);
}

bool CTCPReadThread::delAliveSockfdVec(int nfd)
{
	bool b = false;
	vector<int>::iterator its = find(m_AliveSockfdVec.begin(), m_AliveSockfdVec.end(), nfd);
	if(its != m_AliveSockfdVec.end())
	{
		b = true;
		m_AliveSockfdVec.erase(its);
	}
	return b;
}

void CTCPReadThread::onPipeProcessCallback(int fd, short which, void *arg)
{
	CTCPReadThread *pTcpReadIOThread = (CTCPReadThread*)arg;
	char buf[1];
	if(recv(fd, buf, 1, 0) != 1)
	{
		printf("recv is failure error = %s", strerror(errno));
	}

	CQ_ITEM *item = NULL;
	while((item = (CQ_ITEM*)pTcpReadIOThread->pop()) != NULL) 
	{
		if(item->bDelMySelf)
		{
			pTcpReadIOThread->eventloopexit();
			break;
		}
		else
		{
			CSession *pSession = NULL;
			if(item->eSocketStatus == SOCKETSTATUS_CONNECT)
			{
				pSession = pTcpReadIOThread->newSession(item->nfd, 
						(IBaseIOThread*)item->pWriteIOThread, item->eNetworkType, item->mIPAddr);
				pSession->Register((IOEvent *)item->pIOEvent, (NetFilter*)item->pFilter);

				//添加可用sockfd
				pTcpReadIOThread->addAliveSockfdVec(item->nfd);
				if(item->eSocketType == SOCKETTYPE_ACCEPT)
				{
					pSession->onNetConnect(item->nfd);
				}
				else if(item->eSocketType == SOCKETTYPE_CONNECT)
				{
					pSession->onConnectSuccess(item->nfd, (CIOService*)item->pIOService);
				}
			}
			else if(item->eSocketStatus == SOCKETSTATUS_CLOSE)
			{
				bool bRet = pTcpReadIOThread->delAliveSockfdVec(item->nfd);
				if(bRet)
				{
					pSession = (CSession*)item->pSession;
					event_del(&pSession->m_readEvent);
					pSession->onRecvTcpClose();
				}
			}
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

CSession *CTCPReadThread::newSession(int nfd, IBaseIOThread *pWriteIOThread, 
		ENETWORKTYPE eNetworkType, const IPAddr &mIPAddr)
{
	CSession *pSession = new CSession(pWriteIOThread, this, eNetworkType, mIPAddr);
	if(eNetworkType == ENETWORKTYPE_TCP)
	{
		//读事件
		event_set(&pSession->m_readEvent, nfd, EV_READ | EV_PERSIST, 
				onNetReadProcessCallback, (void *)pSession);

		event_base_set(base, &pSession->m_readEvent);
		if(event_add(&pSession->m_readEvent, 0) == -1) 
		{
			assert(0);
			return NULL;
		}

		//写事件
		((CTCPWriteThread*)pWriteIOThread)->setWriteEvent(nfd, pSession);
	}
	return pSession;
}

void CTCPReadThread::onNetReadProcessCallback(int fd, short which, void *arg)
{
	CSession *pSession = (CSession*)arg;
	memset(pSession->m_pBuffer, 0, pSession->m_nBufferLen);
	int nLen = recv(fd, pSession->m_pBuffer, pSession->m_nBufferLen, 0);
	if(nLen <= 0)//连接关闭
	{
		bool bSocketError = false; // WIN32下，如果不是WOULDBLOCK错误，置为true
		if (nLen < 0)   // nLen == SOCKET_ERROR
		{
#ifndef WIN32
			printf("recv error = %s", strerror(errno));
			if((errno == EINTR) || (errno == EAGAIN) || (errno == EWOULDBLOCK))
			{
				printf("recv strerror = %s", strerror(errno));
				return;
			}
			else
			{
				bSocketError = true;
			}
#else
			int nError = WSAGetLastError();
			switch (nError)
			{
			case WSAEWOULDBLOCK:
				return;
			default:
				bSocketError = true;
				printf("socket %d: recv error = %d", fd, nError);
				break;
			}
#endif
		}

		if(0 == nLen || bSocketError)
		{
			bool bRet = ((CTCPReadThread*)pSession->m_pReadThread)->delAliveSockfdVec(fd);
			if(bRet)
			{
				event_del(&pSession->m_readEvent);
				pSession->onRecvClose();
			}
		}
	}
	else   //收到数据
	{
		pSession->onRecvData(pSession->m_pBuffer, nLen);
	}
}
