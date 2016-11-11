#include <iostream>
#include <unistd.h>
#include "TestIOEvent.h"
int main()
{
	bool b = CreateServerIOService(IO_SERVICE_KCP_SERVER, "0.0.0.0", 6666, 
			new CTestIOEvent, NULL, 3, true);
	if(b)
	{
		printf("%s", "===============");
	}
#if 0
	bool bRet = CreateServerIOService(IO_SERVICE_UDX_SERVER, "0.0.0.0",
			atoi(sPort.c_str()), new CDataServer, NULL, 3, IO_THREAD_FREE, false);
#endif
	printf("CreateServerIOService b = %d", b);
	//sleep(300);
	//return 0;
	while(1)
	{
		pause();
	}
}

