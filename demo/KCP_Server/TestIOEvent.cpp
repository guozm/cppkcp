#include "TestIOEvent.h"
#include "TestDataTrans.h"

CTestIOEvent::CTestIOEvent()
{
	printf("%s", "CTestIOEvent::CTestIOEvent() \n");
}

CTestIOEvent::~CTestIOEvent()
{
	printf("%s", "CTestIOEvent::~CTestIOEvent() \n");
}

int CTestIOEvent::OnIOEvent(IOHandle* ioHandle, IOEventType enuEvent, char* dataBuffer, int nLength)
{
	printf("CTestIOEvent::OnIOEvent ...... enuEvent = %d\n", enuEvent);
	if(IO_EVENT_INIT == enuEvent)
	{
		printf("%s", "IO_EVENT_INIT == enuEvent \n");
	}
	else if(IO_EVENT_OPEN_OK == enuEvent)
	{
		ioHandle->QueryHandleInfo(QUERY_LOCAL_IP);
		printf("%s", "IO_EVENT_OPEN_OK == enuEvent \n");

		CTestDataTrans *p = new CTestDataTrans(ioHandle);
		ioHandle->Register(p, NULL);
	}
	else if(IO_EVENT_READ_OK == enuEvent)
	{
		assert(0);
		printf("IO_EVENT_READ_OK == enuEvent dataBuffer = %s nLength = %d \n", 
				dataBuffer, nLength);
		ioHandle->Write(dataBuffer, nLength);
		
	}
	else if(IO_EVENT_CLOSED == enuEvent)
	{
		printf("%s", "IO_EVENT_CLOSED == enuEvent\n");
		delete this;
	}

	return 0;
}
