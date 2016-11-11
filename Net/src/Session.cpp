#include "Session.h"
#include "KCPThread.h"
#include "kcpinc.h"

#ifndef WIN32
#include "BaseIOThread.h"
#else
#include "BaseIOThreadWin.h"
#endif

CSession::CSession(IBaseIOThread *pWriteIOThread, IBaseIOThread *pReadIOThread, 
		ENETWORKTYPE eNetworkType, const IPAddr &mIPAddr) :
	m_pWriteThread(pWriteIOThread), m_pReadThread(pReadIOThread), m_nfd(0),
	m_IPAddr(mIPAddr), m_eNetWorkType(eNetworkType), m_pKCPClient(NULL)
{
	printf(" = %s CSession::CSession this = %p", 
			QueryENetWorkTypeToString().c_str(), this);
	initObj();
}

CSession::CSession(IBaseIOThread *pWriteIOThread, IBaseIOThread *pReadIOThread, 
		ENETWORKTYPE eNetworkType, const IPAddr &mIPAddr,
		struct sockaddr_in addr, int nfd, ESOCKETTYPE eSocketType) : 
	m_pWriteThread(pWriteIOThread), m_pReadThread(pReadIOThread), m_nfd(nfd), 
	m_IPAddr(mIPAddr), m_addr(addr), 
	m_eNetWorkType(eNetworkType), m_eSocketType(eSocketType), m_pKCPClient(NULL)
{
	printf(" = %s CSession::CSession this = %p ip = %s port = %d", 
			QueryENetWorkTypeToString().c_str(), this, inet_ntoa(m_addr.sin_addr), 
			htons(addr.sin_port));
	initObj();
}

CSession::CSession(IBaseIOThread *pWriteIOThread, IBaseIOThread *pReadIOThread, 
		ENETWORKTYPE eNetworkType, const IPAddr &mIPAddr,
		long long nServerObj, CKCPClient *pKCPClient) : m_pWriteThread(pWriteIOThread), 
	m_pReadThread(pReadIOThread), m_nfd(0), m_IPAddr(mIPAddr), 
	m_eNetWorkType(eNetworkType), m_nServerObj(nServerObj), m_pKCPClient(pKCPClient)
{
	printf(" = %s CSession::CSession this = %p", QueryENetWorkTypeToString().c_str(), this);
	initObj();
}

CSession::~CSession()
{
	printf(" = %s CSession::~CSession this = %p", QueryENetWorkTypeToString().c_str(), this);
	if(m_pIOService)
	{
		delete m_pIOService;
		m_pIOService = NULL;
	}

	if(m_pNetFilter)
	{
		delete m_pNetFilter;
		m_pNetFilter = NULL;
	}

	//清空写缓冲区
	clearWriteQue();

	//释放kcp资源
	releaseKcp();

	if(m_nfd > 0)
	{
		if(m_eNetWorkType != ENETWORKTYPE_KCP || m_eSocketType == SOCKETTYPE_CONNECT)
		{
#ifdef WIN32
			closesocket(m_nfd);
#else
			close(m_nfd);
#endif
		}
	}
	pthread_mutex_destroy(&m_mutex);
}

void CSession::initObj()
{
	m_pNetFilter = NULL;
	m_pIOService = NULL;
	m_bClose = false;
	m_bSendCloseHandle = false;
	m_bSendError = false;
	m_bKcpNetConnect = false;
	m_pkcp = NULL;
	pthread_mutex_init(&m_mutex, NULL);
	m_nRecvHBCount = 0;
	m_nCloseCount = 0;
	m_nKcpServerVersion = 0;
	m_pSServerObj = NULL;
}

void CSession::setKCPServerObj(SServerObj *pObj)
{
	m_pSServerObj = pObj;
}

SServerObj *CSession::getSServerObj()
{
	return m_pSServerObj;
}

void CSession::releaseKcp()
{
	if(m_eNetWorkType == ENETWORKTYPE_KCP)
	{
		CKCPThread::getKCPThread()->delKCPUpdateObj(this);

		CMutexProxy mLock(m_pkcpMutex);
		if(m_pkcp)
		{
			ikcp_release(m_pkcp);
			m_pkcp = NULL;
		}
	}
}

void CSession::Register(IOEvent *pIOEvent, NetFilter *pFilter)
{
	m_pIOEvent = pIOEvent;
	m_pNetFilter = pFilter;
}

NetFilter *CSession::getNetFilter()
{
	return m_pNetFilter;
}

ESOCKETTYPE CSession::getSocketType()
{
	return m_eSocketType;
}

int CSession::getWaitSendSize()
{
	int nSize = 0;
	if(m_eNetWorkType == ENETWORKTYPE_KCP)
	{
		nSize = 31;
		CMutexProxy mLock(m_pkcpMutex);
		if(m_pkcp)
		{
			nSize = ikcp_waitsnd(m_pkcp);
		}
	}
	return nSize;
}

bool CSession::CanSend()
{
	bool bRet = true;
	if(m_eNetWorkType == ENETWORKTYPE_KCP)
	{
		if(getWaitSendSize() > 30)
		{
			bRet = false;
		}
	}
	return bRet;
}

const char* CSession::QueryHandleInfo(QueryInfoType nKey)
{
	switch(nKey)
	{
		case QUERY_LOCAL_IP:
			return m_IPAddr.m_sLocalIP.c_str();
		case QUERY_LOCAL_PORT:
			return m_IPAddr.m_sLocalPort.c_str();
		case QUERY_REMOTE_IP:
			return m_IPAddr.m_sRemoteIP.c_str();
		case QUERY_REMOTE_PORT:
			return m_IPAddr.m_sRemotePort.c_str();
		default:
			break;
	}
	return "";
}

ENETWORKTYPE CSession::QueryENetWorkType()
{
	return m_eNetWorkType;
}

string CSession::QueryENetWorkTypeToString()
{
	string str("ENETWORKTYPE_NULL");
	if(ENETWORKTYPE_TCP == m_eNetWorkType)
	{
		str = "ENETWORKTYPE_TCP";
	}
	else if(ENETWORKTYPE_KCP == m_eNetWorkType)
	{
		str = "ENETWORKTYPE_KCP";
	}
	return str;
}

void CSession::createKcpObj(long long nKcpNum)
{
	m_nKcpNum = nKcpNum;
	m_pkcp = ikcp_create(m_nKcpNum, this);
	ikcp_nodelay(m_pkcp, 1, 10, 2, 1);  //极速模式
}

CMutexProxy *CSession::getKcpObj(ikcpcb **pKcpObj)
{
	CMutexProxy *mLock = new CMutexProxy(m_pkcpMutex);
	*pKcpObj = m_pkcp;
	return mLock;
}

void CSession::onKcpConnectSuccess(int nfd, CIOService *pIOService)
{
	m_pIOService = pIOService;
	m_nfd = nfd;

	CQ_KCP_RECV_ITEM *pKcpRecvItem = new CQ_KCP_RECV_ITEM;
	pKcpRecvItem->pSession = this;
	pKcpRecvItem->eSocketStatus = SOCKETSTATUS_CONNECT;
#ifndef WIN32
	int nSize = m_pReadThread->push((void*)pKcpRecvItem);
	if(nSize <= 1)
	{
		if(write(m_pReadThread->notify_send_fd, "", 1) != 1) 
		{
			printf("send is failure error = %s", strerror(errno));
		}
	}
#else
	if (!((CBaseIOThreadWin*)m_pReadThread)->postToIocp(pKcpRecvItem))
		ASSERTS(false, "0x%p postToIocp return false (0x%p)", m_pWriteThread, pKcpRecvItem);
#endif
}

void CSession::onRecvKcpData(char *sData, int nLen)
{
	CQ_KCP_RECV_ITEM *pKcpRecvItem = new CQ_KCP_RECV_ITEM;
	char *pBuffer = new char[nLen];
	memset(pBuffer, 0, nLen);
	memcpy(pBuffer, sData, nLen);
	pKcpRecvItem->buf = pBuffer;
	pKcpRecvItem->len = nLen;
	pKcpRecvItem->pSession = this;
	pKcpRecvItem->eSocketStatus = SOCKETSTATUS_DATA;
#ifndef WIN32
	int nSize = m_pReadThread->push((void*)pKcpRecvItem);
	if(nSize <= 1)       //写线程还有数据没有读, 不需要通知
	{
		if(write(m_pReadThread->notify_send_fd, "", 1) != 1) 
		{
			printf("send is failure error = %s", strerror(errno));
		}
	}
#else
	if (!((CBaseIOThreadWin*)m_pReadThread)->postToIocp(pKcpRecvItem))
		ASSERTS(false, "0x%p postToIocp return false (0x%p)", m_pWriteThread, pKcpRecvItem);
#endif
}

void CSession::onRecvKcpDataMsg(char *sData, int nLen)
{
	CQ_KCP_RECV_ITEM *pKcpRecvItem = new CQ_KCP_RECV_ITEM;
	char *pBuffer = new char[nLen];
	memset(pBuffer, 0, nLen);
	memcpy(pBuffer, sData, nLen);
	pKcpRecvItem->buf = pBuffer;
	pKcpRecvItem->len = nLen;
	pKcpRecvItem->pSession = this;
	pKcpRecvItem->eSocketStatus = SOCKETSTATUS_DATAMSG;
#ifndef WIN32
	int nSize = m_pReadThread->push((void*)pKcpRecvItem);
	if(nSize <= 1)       //读线程还有数据没有读, 不需要通知
	{
		if(write(m_pReadThread->notify_send_fd, "", 1) != 1) 
		{
			printf("send is failure error = %s", strerror(errno));
		}
	}
#else
	if (!((CBaseIOThreadWin*)m_pReadThread)->postToIocp(pKcpRecvItem))
		ASSERTS(false, "0x%p postToIocp return false (0x%p)", m_pWriteThread, pKcpRecvItem);
#endif
}

void CSession::onConnectSuccess(int nfd, CIOService *pIOService)
{
	m_pIOService = pIOService;
	m_nfd = nfd;
	m_eSocketType = SOCKETTYPE_CONNECT;
	onIOEvent(IO_EVENT_CLIENT_OPEN_OK, NULL, 0);
}

void CSession::onNetConnect(int nfd)
{
	m_nfd = nfd;
	m_eSocketType = SOCKETTYPE_ACCEPT;
	m_pIOEvent->OnIOEvent(this, IO_EVENT_OPEN_OK, NULL, 0);
}

void CSession::onNetKcpConnect()
{
	if(!m_bKcpNetConnect)
	{
		m_bKcpNetConnect = true;
		CQ_KCP_RECV_ITEM *pKcpRecvItem = new CQ_KCP_RECV_ITEM;
		pKcpRecvItem->pSession = this;
		pKcpRecvItem->eSocketStatus = SOCKETSTATUS_NEWCONNECT;
#ifndef WIN32
		int nSize = m_pReadThread->push((void*)pKcpRecvItem);
		if(nSize <= 1)
		{
			if(write(m_pReadThread->notify_send_fd, "", 1) != 1) 
			{
				printf("send is failure error = %s", strerror(errno));
			}
		}
#else
		if (!((CBaseIOThreadWin*)m_pReadThread)->postToIocp(pKcpRecvItem))
			ASSERTS(false, "0x%p postToIocp return false (0x%p)", m_pWriteThread, pKcpRecvItem);
#endif
	}
	else
	{
		m_pIOEvent->OnIOEvent(this, IO_EVENT_OPEN_OK, NULL, 0);
	}
}

void CSession::onRecvData(char *sData, int nLen)
{
	if(m_bSendCloseHandle || m_bSendError || m_bClose)
	{
		return;
	}

	if(m_pNetFilter)
	{
		CDataPackage package;
		if(m_pNetFilter->FilterRead(sData, nLen))
		{
			while(m_pNetFilter->GetPackage(package))
			{
				char *pBuffer = package.GetData();
				int nBufferLen = package.GetDataLength();
				onIOEvent(IO_EVENT_READ_OK, pBuffer, nBufferLen);
			}
		}
	}
	else
	{
		onIOEvent(IO_EVENT_READ_OK, sData, nLen);
	}
}

void CSession::onRecvDataMsg(char *sData, int nLen)
{
	if(m_bSendCloseHandle || m_bSendError || m_bClose)
	{
		return;
	}

	if(m_pNetFilter)
	{
		CDataPackage package;
		if(m_pNetFilter->FilterRead(sData, nLen))
		{
			while(m_pNetFilter->GetPackage(package))
			{
				char *pBuffer = package.GetData();
				int nBufferLen = package.GetDataLength();
				onIOEvent(IO_EVENT_READMSG_OK, pBuffer, nBufferLen);
			}
		}
	}
	else
	{
		onIOEvent(IO_EVENT_READMSG_OK, sData, nLen);
	}
}

void CSession::onRecvClose()
{
	printf("CSession::onRecvClose m_eNetWorkType = %s this = %p m_sLocalIP = %s "
			"m_sLocalPort = %s m_sRemoteIP = %s m_sRemotePort = %s",
			QueryENetWorkTypeToString().c_str(), this, m_IPAddr.m_sLocalIP.c_str(), 
			m_IPAddr.m_sLocalPort.c_str(), m_IPAddr.m_sRemoteIP.c_str(), 
			m_IPAddr.m_sRemotePort.c_str());

	if(m_eNetWorkType == ENETWORKTYPE_TCP)
	{
		onRecvTcpClose();
	}
	else if(m_eNetWorkType == ENETWORKTYPE_KCP)
	{
		onRecvKcpClose();
	}
}

void CSession::onRecvTcpClose()
{
	//通知
	notifyCloseToWriteThread();
}

void CSession::onRecvKcpClose()
{
	sendKcpData(SOCKETSTATUS_NOTIFYKCPCLOSE);
}

void CSession::onRecvKcpReadCloseOK()
{
	if(SOCKETTYPE_CONNECT == m_eSocketType)
	{
		//关闭回调
		onIOEvent(IO_EVENT_CLOSED, NULL, 0);
	}
	else if(SOCKETTYPE_ACCEPT == m_eSocketType && m_bKcpNetConnect)
	{
		//关闭回调
		onIOEvent(IO_EVENT_CLOSED, NULL, 0);
	}

	//取消心跳检测
	CKCPThread::getKCPThread()->delSendHBObj(this);

	//延时删除
	CKCPThread::getKCPThread()->addNetDelayDelObj(this);
}

void CSession::onRecvAppClose()
{
	printf("%s", "onRecvAppClose");

	//通知
	notifyCloseToWriteThread();
}

void CSession::onIOEvent(IOEventType enuEvent, char* dataBuffer, int nLength)
{
	if(m_eNetWorkType == ENETWORKTYPE_KCP 
			&& m_eSocketType == SOCKETTYPE_ACCEPT 
			&& false == m_bKcpNetConnect)
	{
		return;
	}

	m_pIOEvent->OnIOEvent(this, enuEvent, dataBuffer, nLength);
}

void CSession::onWriteFailure()
{
	//写数据失败回调
	onIOEvent(IO_EVENT_WRITE_FAILED, NULL, 0);
}

void CSession::onWriteOK(char *dataBuffer, int nLength)
{
	if(m_pNetFilter)
	{
		onIOEvent(IO_EVENT_WRITE_OK, dataBuffer+8, nLength-8);
	}
	else
	{
		onIOEvent(IO_EVENT_WRITE_OK, dataBuffer, nLength);
	}
}

void CSession::onWriteMsgOK(char *dataBuffer, int nLength)
{
	if(m_pNetFilter)
	{
		onIOEvent(IO_EVENT_WRITEMSG_OK, dataBuffer+8, nLength-8);
	}
	else
	{
		onIOEvent(IO_EVENT_WRITEMSG_OK, dataBuffer, nLength);
	}
}

void CSession::onWriteThreadCloseOK()
{
	if(m_bClose)
	{
		return;
	}
	m_bClose = true;

	printf("CSession::onWriteThreadCloseOK m_eNetWorkType = %s this = %p m_sLocalIP = %s "
			"m_sLocalPort = %s m_sRemoteIP = %s m_sRemotePort = %s",
			QueryENetWorkTypeToString().c_str(), this, m_IPAddr.m_sLocalIP.c_str(), 
			m_IPAddr.m_sLocalPort.c_str(), 
			m_IPAddr.m_sRemoteIP.c_str(), m_IPAddr.m_sRemotePort.c_str());

	if(m_eNetWorkType == ENETWORKTYPE_TCP)
	{
		onTcpWriteThreadCloseOK();
	}
	else if(m_eNetWorkType == ENETWORKTYPE_KCP)
	{
		onKcpWriteThreadCloseOK();
	}
}

void CSession::onTcpWriteThreadCloseOK()
{
	//关闭回调
	onIOEvent(IO_EVENT_CLOSED, NULL, 0);
	delete this;
}

void CSession::onKcpWriteThreadCloseOK()
{
	notifyKcpCloseToReadThread();
}

void CSession::Write(const char* dataBuffer, int nLength, bool bReliable)
{
	ESOCKETSTATUS eSocketStatus;
	if(bReliable)
	{
		eSocketStatus = SOCKETSTATUS_DATA;
	}
	else
	{
		eSocketStatus = SOCKETSTATUS_UNRELIABLEDATA;
	}
	writedata(dataBuffer, nLength, eSocketStatus);
}

void CSession::WriteMsg(const char* dataBuffer, int nLength)
{
	if(m_eNetWorkType != ENETWORKTYPE_KCP)
	{
		return;
	}
	writedata(dataBuffer, nLength, SOCKETSTATUS_DATAMSG);
}

void CSession::writedata(const char* dataBuffer, int nLength, ESOCKETSTATUS eSocketStatus)
{
	if(m_bClose || m_bSendCloseHandle || m_bSendError)
	{
		return;
	}

	char *pBuffer = NULL;
	int nLen = 0;
	if(m_pNetFilter)
	{
		m_pNetFilter->FilterWrite(dataBuffer, nLength, &pBuffer, nLen);
	}
	else
	{
		pBuffer = new char[nLength];
		memset(pBuffer, 0, nLength);
		memcpy(pBuffer, dataBuffer, nLength);
		nLen = nLength;
	}

	if(m_eNetWorkType == ENETWORKTYPE_TCP)
	{
		addWriteQue(pBuffer, nLen, eSocketStatus);
	}
	else if(m_eNetWorkType == ENETWORKTYPE_KCP)
	{
		sendKcpData(eSocketStatus, pBuffer, nLength);
	}
}

void CSession::CloseHandle()
{
	if(m_bSendCloseHandle)
	{
		return;
	}
	m_bSendCloseHandle = true;

	printf("CSession::CloseHandle this = %p m_sLocalIP = %s m_sLocalPort = %s "
			"m_sRemoteIP = %s m_sRemotePort = %s", this, m_IPAddr.m_sLocalIP.c_str(), 
			m_IPAddr.m_sLocalPort.c_str(), 
			m_IPAddr.m_sRemoteIP.c_str(), m_IPAddr.m_sRemotePort.c_str());

	if(m_eNetWorkType == ENETWORKTYPE_TCP)
	{
		//通知读线程关闭
		notifyTcpCloseToReadThread();
	}
	else if(m_eNetWorkType == ENETWORKTYPE_KCP)
	{
		sendKcpClose();
	}
}

void CSession::addWriteQue(const char* dataBuffer, int nLength, ESOCKETSTATUS eSocketStatus)
{
	CQ_WRITE_ITEM *item = new CQ_WRITE_ITEM;
	if(dataBuffer && nLength != 0)
	{
		item->buf = (char*)dataBuffer;
		item->len = nLength;
	}
	item->eSocketStatus = eSocketStatus;
	if(m_bClose)
	{
		delete item;
		item = NULL;
		return;
	}

	//插入数据到写线程
	int nSize = pushWriteQue(item);
	if(nSize > 2)
	{
		return;
	}

	//通知写线程有数据写
	CQ_NOTIFYWRITE_ITEM *notifyitem = new CQ_NOTIFYWRITE_ITEM;
	notifyitem->pSession = this;
	notifyitem->bDelMySelf = false;
#ifndef WIN32
	nSize = m_pWriteThread->push((void*)notifyitem);
	if(nSize <= 1)       //写线程还有数据没有读, 不需要通知
	{
		if(write(m_pWriteThread->notify_send_fd, "", 1) != 1) 
		{
			printf("send is failure error = %s", strerror(errno));
		}
	}
#else
	if (!((CBaseIOThreadWin*)m_pWriteThread)->postToIocp(notifyitem))
		ASSERTS(false, "0x%p postToIocp return false (0x%p)", m_pWriteThread, notifyitem);
#endif
}

void CSession::notifyCloseToWriteThread()
{
	//通知写线程socket关闭
	CQ_NOTIFYWRITE_ITEM *notifyitem = new CQ_NOTIFYWRITE_ITEM;
	notifyitem->pSession = this;
	notifyitem->eSocketStatus = SOCKETSTATUS_CLOSE;
	notifyitem->bDelMySelf = false;
#ifndef WIN32
	int nSize = 0;
	auto_ptr<CMutexProxy> pLock(m_pWriteThread->push((void*)notifyitem, nSize));
	if(nSize <= 1)       //读线程还有数据没有读, 不需要通知
	{
		if(write(m_pWriteThread->notify_send_fd, "", 1) != 1) 
		{
			printf("send is failure error = %s", strerror(errno));
		}
	}
#else
	if (!((CBaseIOThreadWin*)m_pWriteThread)->postToIocp(notifyitem))
		ASSERTS(false, "0x%p postToIocp return false (0x%p)", m_pWriteThread, notifyitem);
#endif
}

void CSession::notifyTcpCloseToReadThread()
{
	CQ_ITEM *item = new CQ_ITEM;
	item->nfd = m_nfd;
	item->pSession = this;
	item->eSocketStatus = SOCKETSTATUS_CLOSE;
	item->bDelMySelf = false;
#ifndef WIN32
	int nSize = m_pReadThread->push((void*)item);
	if(nSize <= 1)       //读线程还有数据没有读, 不需要通知
	{
		if(write(m_pReadThread->notify_send_fd, "", 1) != 1) 
		{
			printf("send is failure error = %s", strerror(errno));
		}
	}
#else
	if (!((CBaseIOThreadWin*)m_pReadThread)->postToIocp(item))
		ASSERTS(false, "0x%p postToIocp return false (0x%p)", m_pReadThread, item);
#endif
}

void CSession::notifyKcpCloseToReadThread()
{
	CQ_KCP_RECV_ITEM *pKcpRecvItem = new CQ_KCP_RECV_ITEM;
	pKcpRecvItem->pSession = this;
	pKcpRecvItem->eSocketStatus = SOCKETSTATUS_KCPCLOSE;
#ifndef WIN32
	int nSize = m_pReadThread->push((void*)pKcpRecvItem);
	if(nSize <= 1)
	{
		if(write(m_pReadThread->notify_send_fd, "", 1) != 1) 
		{
			printf("send is failure error = %s", strerror(errno));
		}
	}
#else
	if (!((CBaseIOThreadWin*)m_pReadThread)->postToIocp(pKcpRecvItem))
		ASSERTS(false, "0x%p postToIocp return false (0x%p)", m_pWriteThread, pKcpRecvItem);
#endif
}

void CSession::sendKcpClose()
{
	sendKcpData(SOCKETSTATUS_KCPCLOSE);
}

void CSession::sendKcpCreate()
{
	printf("%s", "CSession::sendKcpCreate .......");
	sendKcpData(SOCKETSTATUS_KCPCREATE);
}

void CSession::sendKcpHeartbeat()
{
	if(m_bClose || m_bSendCloseHandle)
	{
		return;
	}

	sendKcpData(SOCKETSTATUS_HEARTBEAT);
}

void CSession::sendKcpData(ESOCKETSTATUS eSocketStatus, const char* dataBuffer, int nLength) 
{
	CQ_NOTIFYWRITE_ITEM *notifyitem = new CQ_NOTIFYWRITE_ITEM;
	notifyitem->pSession = this;
	notifyitem->eSocketStatus = eSocketStatus;
	notifyitem->bDelMySelf = false;
	if(dataBuffer && nLength != 0)
	{
		notifyitem->buf = (char*)dataBuffer;
		notifyitem->len = nLength;
	}

#ifndef WIN32
	int nSize = 0;
	auto_ptr<CMutexProxy> pLock(m_pWriteThread->push((void*)notifyitem, nSize));
	if(nSize <= 1)       //读线程还有数据没有读, 不需要通知
	{
		if(write(m_pWriteThread->notify_send_fd, "", 1) != 1) 
		{
			printf("send is failure error = %s", strerror(errno));
		}
	}
#else
	if (!((CBaseIOThreadWin*)m_pWriteThread)->postToIocp(notifyitem))
		ASSERTS(false, "0x%p postToIocp return false (0x%p)", m_pWriteThread, notifyitem);
#endif
}

int CSession::pushWriteQue(CQ_WRITE_ITEM *pItme)
{
	int nSize = 0;
	pthread_mutex_lock(&m_mutex);
	m_WriteQueue.push(pItme);
	nSize = m_WriteQueue.size();
	pthread_mutex_unlock(&m_mutex);
	return nSize;
}

CQ_WRITE_ITEM *CSession::writeQueFront()
{
	CQ_WRITE_ITEM *p = NULL;
	pthread_mutex_lock(&m_mutex);
	if(0 == m_WriteQueue.size())
	{
		pthread_mutex_unlock(&m_mutex);
		return NULL;
	}

	while(m_WriteQueue.size() != 0)
	{
		p = m_WriteQueue.front();
		if(p->bSendOk)
		{
			delete p;
			p = NULL;
			m_WriteQueue.pop();
			continue;
		}
		break;
	}
	pthread_mutex_unlock(&m_mutex);
	return p;
}

void CSession::clearWriteQue()
{
	pthread_mutex_lock(&m_mutex);
	while(!m_WriteQueue.empty())
	{
		CQ_WRITE_ITEM *p = m_WriteQueue.front();
		m_WriteQueue.pop();
		delete p;
		p = NULL;
	}
	pthread_mutex_unlock(&m_mutex);
}

int CSession::writeQueSize()
{
	return m_WriteQueue.size();
}
