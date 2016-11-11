#ifndef TPKT_FILTER_H
#define TPKT_FILTER_H

#include "netfilter.h"
#include <queue>
#ifdef WIN32
#include <memory>
#elif defined(IOS)||defined(MAC_OS)
#include <memory>
#else
#include <tr1/memory>
#endif

/***********************************************************************
TPKT包内容：
字节说明
1 表示TPKT包的版本，目前的版本为3
2 保留
3 包长度的高8位
4 包长度的底8位
***********************************************************************/

#define PER_ALLOC_SIZE 8192
struct TPKT_HEADER
{
    unsigned char version;
    unsigned char reverse;
    unsigned int size;
};
class CTpktFilter:public NetFilter
{
public:
    CTpktFilter(void);
    ~CTpktFilter(void);

    virtual bool InitFilter(const char* szBuff, unsigned int nLength)
    {
        return true;
    };
    virtual bool FilterRead(const char* szBuff, unsigned int nLength);
    virtual bool GetPackage(CDataPackage& package);
    virtual bool FilterWrite(const char* szBuff, unsigned int nLength, CDataPackage& package);
	virtual bool FilterWrite(const char* szBuff, unsigned int nLength, 
			char **pOutPacket, int &nLen);
	virtual unsigned int GetHeaderLen() const {return sizeof(TPKT_HEADER);}

private:

    struct DataInfo
    {
        friend class CTpktFilter;
    public:
        DataInfo(char *pData, int nLength):m_pData(pData), m_nLength(nLength){}
    private:
        char *m_pData;
        int m_nLength;
    };
private:
    char    *m_pFilterBuff;
    unsigned int     m_nCurPos;
    unsigned int     m_nMaxPos;

    unsigned int     m_nWaitingLength;
    std::deque<DataInfo> m_dqPackage; 
};
#endif
