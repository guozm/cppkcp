#include "TestIOEvent.h"
#include <unistd.h>
#include <sstream>

CTestIOEvent::CTestIOEvent()
{
	m_ioHandle = NULL;
	m_nCount = 0;
	printf("CTestIOEvent::CTestIOEvent() \n");
}

CTestIOEvent::~CTestIOEvent()
{
	printf("CTestIOEvent::~CTestIOEvent() \n");
}

void CTestIOEvent::WriteData(const char *sData, int nLen)
{
	m_ioHandle->Write(sData, nLen);
}

int CTestIOEvent::OnIOEvent(IOHandle* ioHandle, IOEventType enuEvent, char* dataBuffer, 
		int nLength)
{
	if(IO_EVENT_CLIENT_OPEN_OK == enuEvent)
	{
		printf("IO_EVENT_CLIENT_OPEN_OK == enuEvent \n");
		m_ioHandle = ioHandle;
		stringstream str;
		str<<"hello "<<this<<" "<<++m_nCount;
		WriteData(str.str().c_str(), str.str().length()+1);

		//m_ioHandle->CloseHandle();
	}
	else if(IO_EVENT_OPEN_FAILED == enuEvent)
	{
		printf("IO_EVENT_OPEN_FAILED == enuEvent \n");

#if 0
		IOEvent *pIOEvent = new CTestIOEvent;
		bool b = CreateClientIOService(IO_SERVICE_KCP_CLIENT, "", 0, "192.168.0.82", 6666, 
				pIOEvent, NULL, 1, IO_THREAD_FREE, true);
		if(!b)
		{
			assert(0);
		}
		#endif
	}
	else if(IO_EVENT_OPEN_OK == enuEvent)
	{
		printf("IO_EVENT_OPEN_OK == enuEvent \n");
	}
	else if(IO_EVENT_READ_OK == enuEvent)
	{
		printf("IO_EVENT_READ_OK == enuEvent dataBuffer = %s nLength = %d time = %ld\n", 
				dataBuffer, nLength, time(0));
		//m_ioHandle->CloseHandle();
		stringstream str;
		str<<"hello "<<this<<" "<<++m_nCount;
		WriteData(str.str().c_str(), str.str().length()+1);

		//if(1)
		if(50 == m_nCount)
		{
			printf("500 == m_nCount\n");
			m_ioHandle->CloseHandle();

#if 0
			IOEvent *pIOEvent = new CTestIOEvent;
			bool b = CreateClientIOService(IO_SERVICE_KCP_CLIENT, "", 0, "192.168.0.113", 6666, 
					pIOEvent, NULL, 1, IO_THREAD_FREE, true);
			if(!b)
			{
				assert(0);
			}
		#endif
		}
	}
	else if(IO_EVENT_CLOSED == enuEvent)
	{
		printf("IO_EVENT_CLOSED == enuEvent\n");
		delete this;
	}
	else if(IO_EVENT_WRITE_OK == enuEvent)
	{
		printf("%s\n", "IO_EVENT_WRITE_OK == enuEvent");
		//m_ioHandle->CloseHandle();
	}
	return 0;
}
