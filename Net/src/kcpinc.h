/** 
* @file kcp_inc.h
* @brief 
* @add by guozm guozm
* @version 
* @date 2016-08-11
*/
#include "ikcp.h"
#include "networkio.h"
#include "net_type.h"

#ifndef __CKCPINC__H
#define __CKCPINC__H
#pragma pack (4)

const int g_nKcpVersion = 12345;

typedef enum _EKCPNETEVENT
{
	KCP_EVENT_NULL = 0,
	KCP_CREATE,
	KCP_CREATERESPONSE,
	KCP_CLOSE,
	KCP_DATA,
	KCP_DATAMSG,
	KCP_UNRELIABLEDATA,
	KCP_HEARTBEAT,
	KCP_NETDETECT
}EKCPNETEVENT;

struct SKcpHeader
{
	SKcpHeader()
	{
		netEvent = KCP_EVENT_NULL;
		nKcpVersion = g_nKcpVersion;
	}
	EKCPNETEVENT netEvent;
	int nKcpVersion;
};

struct SKcpCreateResultHeader
{
	SKcpCreateResultHeader()
	{
		netEvent = KCP_EVENT_NULL;
		nKcpVersion = g_nKcpVersion;
		nKcpServerVersion = 0;
		serverObj = -1;
		KCP_number = -1;
	}

	EKCPNETEVENT netEvent;
	int nKcpVersion;
	int nKcpServerVersion;
	long long serverObj;
	long long KCP_number;
};

struct SKcpDataHeader
{
	SKcpDataHeader()
	{
		netEvent = KCP_EVENT_NULL;
		nKcpVersion = 0;
		nServerObj = -1;
	}

	EKCPNETEVENT netEvent;
	int nKcpVersion;
	long long nServerObj;
};

struct SServerObj
{
	SServerObj(void *pObj) : pSession(pObj)
	{
		bDel = false;
	}

	void *pSession;
	bool bDel;
};

IUINT32 iclock();

void makePacket(EKCPNETEVENT eKcpNetEvent, char **sOut, int &nOutLen);
void makePacket(EKCPNETEVENT eKcpNetEvent, const char *sIn, int nInLen, char **sOut, int &nOutLen);
void makePacket(const SKcpDataHeader &mKcpDataHeader, const char *sIn, int nInLen, 
		char **sOut, int &nOutLen);
EKCPNETEVENT socketStstusToKCPNetEvent(ESOCKETSTATUS eSocketStatus);
#pragma pack ()
#endif
