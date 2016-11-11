

#ifdef IOS
//#import <Foundation/Foundation.h>
#import <objc/objc.h>
#include <queue>
#include <sys/time.h>
#endif

#include "kcpinc.h"

/* get system time */
static inline void itimeofday(long *sec, long *usec)
{
	#if defined(__unix) || defined(IOS)
	struct timeval time;
	gettimeofday(&time, NULL);
	if (sec) *sec = time.tv_sec;
	if (usec) *usec = time.tv_usec;
	#else
	static long mode = 0, addsec = 0;
	BOOL retval;
	static IINT64 freq = 1;
	IINT64 qpc;
	if (mode == 0) {
		retval = QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
		freq = (freq == 0)? 1 : freq;
		retval = QueryPerformanceCounter((LARGE_INTEGER*)&qpc);
		addsec = (long)time(NULL);
		addsec = addsec - (long)((qpc / freq) & 0x7fffffff);
		mode = 1;
	}
	retval = QueryPerformanceCounter((LARGE_INTEGER*)&qpc);
	retval = retval * 2;
	if (sec) *sec = (long)(qpc / freq) + addsec;
	if (usec) *usec = (long)((qpc % freq) * 1000000 / freq);
	#endif
}

/* get clock in millisecond 64 */
static inline IINT64 iclock64(void)
{
	long s, u;
	IINT64 value;
	itimeofday(&s, &u);
	value = ((IINT64)s) * 1000 + (u / 1000);
	return value;
}

IUINT32 iclock()
{
	return (IUINT32)(iclock64() & 0xfffffffful);
}

void makePacket(EKCPNETEVENT eKcpNetEvent, char **sOut, int &nOutLen)
{
	SKcpHeader mKcpHeader;
	mKcpHeader.netEvent = eKcpNetEvent;
	nOutLen = sizeof(mKcpHeader);
	*sOut = new char[nOutLen];
	memset(*sOut, 0, nOutLen);
	memcpy(*sOut, &mKcpHeader, sizeof(mKcpHeader));
}

void makePacket(EKCPNETEVENT eKcpNetEvent, const char *sIn, int nInLen, char **sOut, int &nOutLen)
{
	SKcpHeader mKcpHeader;
	mKcpHeader.netEvent = eKcpNetEvent;
	nOutLen = sizeof(mKcpHeader)+nInLen;
	*sOut = new char[nOutLen];
	memset(*sOut, 0, nOutLen);
	memcpy(*sOut, &mKcpHeader, sizeof(mKcpHeader));
	memcpy(*sOut+sizeof(mKcpHeader), sIn, nInLen);
}

void makePacket(const SKcpDataHeader &mKcpDataHeader, const char *sIn, int nInLen, 
		char **sOut, int &nOutLen)
{
	nOutLen = sizeof(mKcpDataHeader)+nInLen;
	*sOut = new char[nOutLen];
	memset(*sOut, 0, nOutLen);
	memcpy(*sOut, &mKcpDataHeader, sizeof(mKcpDataHeader));
	if(sIn && nInLen!=0)
	{
		memcpy(*sOut+sizeof(mKcpDataHeader), sIn, nInLen);
	}
}

EKCPNETEVENT socketStstusToKCPNetEvent(ESOCKETSTATUS eSocketStatus)
{
	EKCPNETEVENT eKcpNetEvent;
	if(SOCKETSTATUS_DATA == eSocketStatus)
	{
		eKcpNetEvent = KCP_DATA;
	}
	else if(SOCKETSTATUS_UNRELIABLEDATA == eSocketStatus)
	{
		eKcpNetEvent = KCP_UNRELIABLEDATA;
	}
	else if(SOCKETSTATUS_DATAMSG == eSocketStatus)
	{
		eKcpNetEvent = KCP_DATAMSG;
	}
	else if(SOCKETSTATUS_KCPCREATE == eSocketStatus)
	{
		eKcpNetEvent = KCP_CREATE;
	}
	else if(SOCKETSTATUS_HEARTBEAT == eSocketStatus)
	{
		eKcpNetEvent = KCP_HEARTBEAT;
	}
	else if(SOCKETSTATUS_KCPCLOSE == eSocketStatus)
	{
		eKcpNetEvent = KCP_CLOSE;
	}
	else
	{
		assert(0);
	}
	return eKcpNetEvent;
}
