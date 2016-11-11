/** 
* @file BaseIOThread.h
* @brief 线程基类
* @add by guozm guozm
* @version 
* @date 2012-04-05
*/

#ifndef __CBASEIOTHREAD__H
#define __CBASEIOTHREAD__H
#ifdef WIN32
#define close closesocket
#endif
#include "IBaseIOThread.h"

class CIOService;
class CBaseIOThread : public IBaseIOThread
{
	public:
		/** 
		* @brief 构造函数 add by guozm 2012/04/05 
		*/
		CBaseIOThread();

		/** 
		* @brief 析够函数 add by guozm 2012/04/05 
		*/
		virtual ~CBaseIOThread();

		/** 
		* @brief 退出循环 add by guozm 2012/07/06 
		*/
		void eventloopexit();

		/** 
		 * @brief pipe libevent 初始化 add by guozm 2012/04/05 
		 */
		void initPipeLibevent(void (*callback)(int , short, void *), void *p);
	
	protected:
		/** 
		 * @brief 线程运行 add by guozm 2012/10/11 
		 */
		virtual void onThreadRun();

		/** 
		* @brief  释放自己 add by guozm 2016/09/19 
		*/
		virtual void delMySelf()
		{
			delete this;
		}

	public:
		struct event_base *base;    /* libevent handle this thread uses */
		struct event notify_event;  /* listen event for notify pipe */
};

#endif
