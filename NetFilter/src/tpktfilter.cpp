#include "tpktfilter.h"
#include "memory.h"
#include <assert.h>
#include <stdlib.h>

CTpktFilter::CTpktFilter(void) : m_pFilterBuff(NULL), m_nCurPos(0), m_nWaitingLength(0)
{
    m_pFilterBuff = (char*)malloc(PER_ALLOC_SIZE);
    if( m_pFilterBuff == NULL )
    {
        assert(0);
    }
    m_nMaxPos = PER_ALLOC_SIZE;
}

CTpktFilter::~CTpktFilter(void)
{
    free(m_pFilterBuff);
    m_pFilterBuff = NULL;
}

bool CTpktFilter::FilterRead(const char* szBuff, unsigned int nLength)
{
    bool bHasPackage = false;
    while ( m_nCurPos + nLength > m_nMaxPos )
    {
        // 重新申请内存
        m_nMaxPos += PER_ALLOC_SIZE;
		m_pFilterBuff = (char*)realloc(m_pFilterBuff, m_nMaxPos);        
    }

    // 此处可以优化，当m_pFilterBuff中没有数据缓存时，可以直接在szBuff上进行分析，减少拷贝动作
    memcpy(m_pFilterBuff + m_nCurPos, szBuff, nLength);
    m_nCurPos += nLength;

    if( m_nWaitingLength > nLength )
    {
        // 缓存数据并继续等待后续的包
        m_nWaitingLength -= nLength;
        return false;
    }
    else
    {
        // 开始解包
        bool    bFinished = false;
        unsigned int     nFilterPos = 0; // 已经解好的包位置
        
        while (!bFinished)
        {
            // TPKT包头不完整
            if( nFilterPos + sizeof(TPKT_HEADER) > m_nCurPos )
            {
                // 等待字节长度置为0,保证下次包能正常执行
                m_nWaitingLength = 0;
                
                // 前移缓冲区位置
                unsigned nStartPos = nFilterPos;
                m_nCurPos -= nStartPos;
                memmove(m_pFilterBuff, m_pFilterBuff + nStartPos, m_nCurPos);
                break;
            }
            TPKT_HEADER header;

            memcpy(&header, m_pFilterBuff + nFilterPos, sizeof(TPKT_HEADER));
            nFilterPos += sizeof(TPKT_HEADER); // 增加包头的长度
            if( header.size == 0 )
            {
				return false;
            }

            if( header.reverse != 0x01) 
            {
				return false;
            }

            if(header.version != 0x03)
            {
				return false;
            }

            if(nFilterPos + header.size > m_nCurPos )
            {
                // 后续包不完整，移动到内存开始的位置并等待后续数据.不使用
                // memcpy的原因是src与dst的部分区域可能重叠
                // 没有解出完整包时，需要将其包头保存下来
                m_nWaitingLength = nFilterPos + header.size - m_nCurPos;
                unsigned nStartPos = nFilterPos - sizeof(TPKT_HEADER);
                m_nCurPos -= nStartPos;
                memmove(m_pFilterBuff, m_pFilterBuff + nStartPos, m_nCurPos);
                break;
            }
            else
            {
                // 拷贝并创建一个包
                char *pPackage = new char[header.size];
                memcpy(pPackage, m_pFilterBuff + nFilterPos, header.size);
                
                // 缓存，等待外部调用
                m_dqPackage.push_back(DataInfo(pPackage, header.size));
                nFilterPos += header.size;

                bHasPackage = true;
                // 全部解析完成，并且正好所有的包都是完整的
                if( m_nCurPos == nFilterPos)
                {
                    m_nWaitingLength = 0;
                    m_nCurPos = 0;
                    break;
                }
            }
            
        }
    }
	return bHasPackage;
}

bool CTpktFilter::GetPackage(CDataPackage& package)
{
    if( m_dqPackage.size() == 0 )
    {
        return false;
    }
    else
    {
        package.SetPackage(m_dqPackage[0].m_pData, m_dqPackage[0].m_nLength);
        m_dqPackage.pop_front();
    }
    return true;
}

bool CTpktFilter::FilterWrite(const char* szBuff, unsigned int nLength, CDataPackage& package)
{
    if( nLength > 0xFFFFFFF )
    {
        assert(0);
        return false;
    }
    
    TPKT_HEADER header;
    size_t hdSize = sizeof(TPKT_HEADER);
    char *pPackage = new char[nLength + hdSize];
	memset(pPackage, 0, nLength+hdSize);

    if( pPackage == NULL )
    {
        assert(0);
        return false;
    }

    header.size = (unsigned int)nLength;
    header.version = 0x03;
    header.reverse = 0x01;

    memcpy(pPackage, &header, hdSize);
    memcpy(pPackage+hdSize, szBuff, nLength);
    package.SetPackage(pPackage, nLength + hdSize);
	return true;
}

bool CTpktFilter::FilterWrite(const char* szBuff, unsigned int nLength, 
		char **pOutPacket, int &nLen)
{
	if( nLength > 0xFFFFFFF )
    {
        assert(0);
        return false;
    }

	TPKT_HEADER header;
	size_t hdSize = sizeof(TPKT_HEADER);
    *pOutPacket = new char[nLength + hdSize];
	memset(*pOutPacket, 0, nLength+hdSize);

	header.size = (unsigned int)nLength;
	header.version = 0x03;
	header.reverse = 0x01;

	memcpy(*pOutPacket, &header, hdSize);
	memcpy(*pOutPacket+hdSize, szBuff, nLength);
	nLen =  nLength + hdSize;
	return true;
}
