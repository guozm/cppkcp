#include "BaseIOThread.h"
#include "networkio.h"
#include "IOService.h"
#include "IoUtils.h"
#include <typeinfo>

CBaseIOThread::CBaseIOThread()
{
}

CBaseIOThread::~CBaseIOThread()
{
}

void CBaseIOThread::eventloopexit()
{
	event_base_loopexit(base, 0);
}

void CBaseIOThread::onThreadRun()
{
	event_base_loop(base, 0);
	event_base_free(base);
	delMySelf();
}

void CBaseIOThread::initPipeLibevent(void (*callback)(int , short, void *), void *p)
{
	base = event_init();
	if(!base)
	{
		assert(0);
	}

	/* Listen for notifications from other threads */
	event_set(&notify_event, notify_receive_fd, EV_READ | EV_PERSIST, callback, p);
	int nRet = event_base_set(base, &notify_event);
	if(-1 == nRet)
	{
		assert(0);
	}

	if(event_add(&notify_event, 0) == -1) 
	{
		assert(0);
	}
}
