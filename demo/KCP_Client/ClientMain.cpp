#include <sstream>
#include <pthread.h>
#include "TestIOEvent.h"
#include <unistd.h>

#ifdef WIN32
#define sleep Sleep
#endif

int main()
{
	for(int i=0; i<1; i++)
	//while(1)
	{
		IOEvent *pIOEvent = new CTestIOEvent;
		bool b = CreateClientIOService(IO_SERVICE_KCP_CLIENT, "", 0, "localhost", 6666, 
				pIOEvent, NULL, 1, true);
		if(b)
		{
			printf("b = %d", b);
		}
	}

	while(1)
	{
		pause();
	}
}

