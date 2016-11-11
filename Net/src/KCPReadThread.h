/** 
* @file KCPReadThread.h
* @brief libevent 工作线程 
* @add by guozm guozm
* @version 
* @date 2012-04-04
*/

#ifndef __CKCPREADTHREAD__H
#define __CKCPREADTHREAD__H
#include "Session.h"
#include "netdef.h"

#ifndef WIN32
#include "BaseIOThread.h"
#else
#include "BaseIOThreadWin.h"
#endif

#include "IOService.h"

class NetFilter;
#ifndef WIN32
class CKCPReadThread : public CBaseIOThread
#else
class CKCPReadThread : public CBaseIOThreadWin
#endif
{
	public:
		/** 
		* @brief 构造函数 add by guozm 2012/04/04 
		*/
		CKCPReadThread();

		/** 
		* @brief 析够函数 add by guozm 2012/04/04 
		*/
		~CKCPReadThread();

		/** 
		 * @brief 清空队列 add by guozm 2012/07/02 
		 */
		virtual void clearQue()
		{
			CMutexProxy mLock(m_Mutex);
			while(!m_Queue.empty())
			{
				void *p = m_Queue.front();
				m_Queue.pop();
				CQ_KCP_RECV_ITEM *pDel = (CQ_KCP_RECV_ITEM*)p;
				delete pDel;
				pDel = NULL;
			}
		}

		/** 
		 * @brief pipe数据回调 add by guozm 2012/04/04 
		 */
		static void onPipeProcessCallback(int fd, short which, void *arg);

	protected:
		/** 
		 * @brief 释放资源 add by guozm 2012/07/02 
		 */
		virtual void onRelease();
};

#endif
