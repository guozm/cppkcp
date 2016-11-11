#include "TestDataTrans.h"
#include "pthread.h"
pthread_mutex_t CTestDataTrans::m_mutex = PTHREAD_MUTEX_INITIALIZER;
vector<CTestDataTrans*> CTestDataTrans::m_IOEventVec;

CTestDataTrans::CTestDataTrans(IOHandle *pIOHandle) : m_pIOHandle(pIOHandle)
{
	printf("CTestDataTrans::CTestDataTrans() \n");
	addDataTrans(this);
}

CTestDataTrans::~CTestDataTrans()
{
	printf("CTestDataTrans::~CTestDataTrans() \n");
}

void CTestDataTrans::addDataTrans(CTestDataTrans *pIOEvent)
{
	pthread_mutex_lock(&m_mutex);
	m_IOEventVec.push_back(pIOEvent);
	pthread_mutex_unlock(&m_mutex);
}

void CTestDataTrans::delDataTrans(CTestDataTrans *pIOEvent)
{
	pthread_mutex_lock(&m_mutex);
	vector<CTestDataTrans*>::iterator its;
	its = find(m_IOEventVec.begin(), m_IOEventVec.end(), pIOEvent);
	if(its != m_IOEventVec.end())
	{
		m_IOEventVec.erase(its);
	}
	printf("m_IOEventVec.size() = %zd\n", m_IOEventVec.size());
	pthread_mutex_unlock(&m_mutex);
}

void CTestDataTrans::transferAll(char *dataBuffer, int nLength)
{
	pthread_mutex_lock(&m_mutex);
	vector<CTestDataTrans*>::iterator its;
	for(its=m_IOEventVec.begin(); its!=m_IOEventVec.end(); its++)
	{
		CTestDataTrans *p = *its;
		p->m_pIOHandle->Write(dataBuffer, nLength);
	}
	pthread_mutex_unlock(&m_mutex);
}

int CTestDataTrans::OnIOEvent(IOHandle* ioHandle, IOEventType enuEvent, char* dataBuffer, int nLength)
{
	//printf("CTestDataTrans::OnIOEvent ......\n");
	if(IO_EVENT_OPEN_OK == enuEvent)
	{
		//printf("IO_EVENT_OPEN_OK == enuEvent \n");
		assert(0);
	}
	else if(IO_EVENT_READ_OK == enuEvent)
	{
		printf("CTestDataTrans IO_EVENT_READ_OK == enuEvent dataBuffer = %s nLength = %d \n", 
				dataBuffer, nLength);
		//transferAll(dataBuffer, nLength);
		ioHandle->Write(dataBuffer, nLength);
		//ioHandle->CloseHandle();
#if 1
#endif
	}
	else if(IO_EVENT_CLOSED == enuEvent)
	{
		printf("CTestDataTrans IO_EVENT_CLOSED == enuEvent\n");
		delDataTrans(this);
		delete this;
	}

	return 0;
}
