#include "BaseIOThreadWin.h"
#include "networkio.h"
#include "IOService.h"
#include <typeinfo>
#include "logcontroller.h"

CBaseIOThreadWin::CBaseIOThreadWin()
	: m_bRun(false),base(NULL)
{
	m_iocp = NULL;
}

CBaseIOThreadWin::~CBaseIOThreadWin()
{
}

bool CBaseIOThreadWin::postToIocp(void* item)
{
	if (item == NULL)
		return false;

	push(item);
	if(send(notify_send_fd, "", 1, 0) != 1) 
	{
//		ASSERTS(0, "send error = %d", WSAGetLastError());
		return false;
	}
	return true;
}

void CBaseIOThreadWin::eventloopexit()
{
	// event loop线程中调用
	if (base != NULL)
		event_base_loopexit(base, 0);
}

void CBaseIOThreadWin::onThreadRun()
{
	event_base_loop(base, 0);
	event_base_free(base);
	delMySelf();
}

void CBaseIOThreadWin::initPipeLibevent(CBFUNC callback, void *p)
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
