#include "KCPReadThread.h"

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

CKCPReadThread::CKCPReadThread() 
{
	initPipeLibevent(onPipeProcessCallback, this);
}

CKCPReadThread::~CKCPReadThread()
{
	printf("CKCPReadThread::~CKCPReadThread\n");
}

void CKCPReadThread::onRelease()
{
	CQ_KCP_RECV_ITEM *item = new CQ_KCP_RECV_ITEM;
	item->bDelMySelf = true;

#ifndef WIN32
	int nSize = push((void*)item);
	if(nSize <=1)
	{
		if(write(notify_send_fd, "", 1) != 1) 
		{
			printf("send is faild error = %s\n", strerror(errno)); 
		}
	}
#else
	if (!postToIocp(item))
	{
		printf("0x%p postToIocp return false (0x%p)\n", this, item);
		assert(0);
	}
#endif
}

void CKCPReadThread::onPipeProcessCallback(int fd, short which, void *arg)
{
	char buf[1];
	if(recv(fd, buf, 1, 0) != 1)
	{
		printf("recv is failure error = %s\n", strerror(errno));
	}

	CKCPReadThread *pKcpReadThread = (CKCPReadThread*)arg;
	CQ_KCP_RECV_ITEM *notifyitem = NULL;
	while((notifyitem = (CQ_KCP_RECV_ITEM*)pKcpReadThread->pop()) != NULL)
	{
		if(notifyitem->bDelMySelf)
		{
			pKcpReadThread->eventloopexit();
			break;
		}
		else
		{
			CSession *pSession = (CSession*)(notifyitem->pSession);
			if(SOCKETSTATUS_KCPCLOSE == notifyitem->eSocketStatus)
			{
				pSession->onRecvKcpReadCloseOK();
			}
			else if(SOCKETSTATUS_CONNECT == notifyitem->eSocketStatus)
			{
				pSession->onConnectSuccess(pSession->m_nfd, pSession->m_pIOService);
			}
			else if(SOCKETSTATUS_NEWCONNECT == notifyitem->eSocketStatus)
			{
				pSession->onNetKcpConnect();
			}
			else if(SOCKETSTATUS_DATA == notifyitem->eSocketStatus)
			{
				pSession->onRecvData(notifyitem->buf, notifyitem->len);
			}
			else if(SOCKETSTATUS_DATAMSG == notifyitem->eSocketStatus)
			{
				pSession->onRecvDataMsg(notifyitem->buf, notifyitem->len);
			}
			if(notifyitem)
			{
				delete notifyitem;
				notifyitem = NULL;
			}
		}
	}

	if(notifyitem)
	{
		delete notifyitem;
		notifyitem = NULL;
	}
}
