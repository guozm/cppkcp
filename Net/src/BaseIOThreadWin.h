#ifndef __CBASEIOTHREADWIN__H
#define __CBASEIOTHREADWIN__H
#include "IBaseIOThread.h"

class CSession;
class CIOService;
class CBaseIOThreadWin : public IBaseIOThread
{
public:
	/** 
	* @brief 构造函数
	*/
	CBaseIOThreadWin();

	/** 
	* @brief 析够函数
	*/
	virtual ~CBaseIOThreadWin();

	/**
	 * @brief 通知到完成端口
	 */
	virtual bool postToIocp(void* item);

	/** 
	* @brief 推出循环
	*/
	virtual void eventloopexit();

	/** 
	* @brief pipe libevent 初始化
	*/
	typedef void (*CBFUNC)(int fd, short which, void *arg);
	void initPipeLibevent(CBFUNC callback, void *p);

protected:

	/** 
	 * @brief  线程函数 add by guozm 2012/10/11 
	 */
	virtual void onThreadRun();

	/** 
	 * @brief  释放自己 add by guozm 2016/09/19 
	 */
	virtual void delMySelf()
	{
		delete this;
	}

protected:
	bool m_bRun;
	HANDLE m_iocp;
	struct event_base *base;    /* libevent handle this thread uses */
	struct event notify_event;  /* listen event for notify pipe */
};

#endif
