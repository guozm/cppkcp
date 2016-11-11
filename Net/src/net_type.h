#ifndef __NET_TYPE_H_
#define __NET_TYPE_H_

struct IPAddr
{
	string m_sLocalIP;
	string m_sLocalPort;
	string m_sRemoteIP;
	string m_sRemotePort;
	IPAddr() : m_sLocalIP(""), m_sLocalPort(""), m_sRemoteIP(""), m_sRemotePort("")
	{
	}
};

/** 
* @brief socket 状态
*/
enum ESOCKETSTATUS
{
	SOCKETSTATUS_NULL = 0,
	SOCKETSTATUS_CONNECT,             //新的连接或者连接服务器成功
	SOCKETSTATUS_NEWCONNECT,          //新的连接或者连接服务器成功
	SOCKETSTATUS_CLOSE,               //主动断开连接
	SOCKETSTATUS_KCPCLOSE,            //close
	SOCKETSTATUS_NOTIFYKCPCLOSE,      //close
	SOCKETSTATUS_KCPCREATE,           //create
	SOCKETSTATUS_DATA,                //数据
	SOCKETSTATUS_DATAMSG,             //msg数据
	SOCKETSTATUS_UNRELIABLEDATA,      //数据不可靠
	SOCKETSTATUS_HEARTBEAT,           //heartbeat
	SOCKETSTATUS_NETWORKDATA
};


typedef struct conn_queue_item CQ_ITEM;
struct conn_queue_item
{
	void *pIOEvent;
	void *pWriteIOThread;
	void *pSession;
	void *pIOService;
	void *pFilter;
    int               nfd;
    ESOCKETTYPE eSocketType;
	ESOCKETSTATUS eSocketStatus;
	ENETWORKTYPE eNetworkType;
	bool bDelMySelf;
	IPAddr mIPAddr;
};

/** 
* @brief 通知写线程有数据
*/
typedef struct notify_write_item CQ_NOTIFYWRITE_ITEM;
struct notify_write_item
{
	notify_write_item()
	{
		eSocketStatus = SOCKETSTATUS_NULL;
		buf = NULL;
		len = 0;
		offset = 0;
		bSendOk = false;
	}

	~notify_write_item()
	{
		if(buf)
		{
			delete [] buf;
			buf = NULL;
		}
	}

	ESOCKETSTATUS eSocketStatus;
	void *pSession;
	bool bDelMySelf;

	char *buf;
	int len;      /* The length of buf. */
	int offset;
	bool bSendOk;
};

typedef struct write_buffe_item CQ_WRITE_ITEM;
struct write_buffe_item 
{
	write_buffe_item()
	{
		eSocketStatus = SOCKETSTATUS_NULL;
		buf = NULL;
		len = 0;
		offset = 0;
		bSendOk = false;
	}

	~write_buffe_item()
	{
		if(buf)
		{
			delete [] buf;
			buf = NULL;
		}
	}

	ESOCKETSTATUS eSocketStatus;
	char *buf;
	int len;      /* The length of buf. */
	int offset;
	bool bSendOk;
};

typedef struct kcp_recv_buffe_item CQ_KCP_RECV_ITEM;
struct kcp_recv_buffe_item 
{
	kcp_recv_buffe_item()
	{
		eSocketStatus = SOCKETSTATUS_NULL;
		buf = NULL;
		len = 0;
		pSession = NULL;
		bDelMySelf = false;
	}

	~kcp_recv_buffe_item()
	{
		if(buf)
		{
			delete [] buf;
			buf = NULL;
		}
	}

	ESOCKETSTATUS eSocketStatus;
	char *buf;
	int len;
	void *pSession;
	bool bDelMySelf;
};

#endif
