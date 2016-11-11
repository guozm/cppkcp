/** 
* @file TCPWriteThread.h
* @brief libevent 工作线程 
* @add by guozm guozm
* @version 
* @date 2012-04-04
*/

#ifndef __CTCPWRITETHREAD__H
#define __CTCPWRITETHREAD__H
#include "Session.h"
#include "netdef.h"

#ifndef WIN32
#include "BaseIOThread.h"
class CTCPWriteThread : public CBaseIOThread
#else
#include "BaseIOThreadWin.h"
class CTCPWriteThread : public CBaseIOThreadWin
#endif
{
	public:
		/** 
		* @brief 构造函数 add by guozm 2012/04/04 
		*/
		CTCPWriteThread();

		/** 
		* @brief 析够函数 add by guozm 2012/04/04 
		*/
		~CTCPWriteThread();

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
				CQ_NOTIFYWRITE_ITEM *pDel = (CQ_NOTIFYWRITE_ITEM*)p;
				delete pDel;
				pDel = NULL;
			}
		}

		/** 
		* @brief  设置写事件 add by guozm 2012/09/02 
		*/
		void setWriteEvent(int nSockfd, CSession *pSession);

		/** 
		 * @brief pipe数据回调 add by guozm 2012/04/04 
		 */
		static void onPipeProcessCallback(int fd, short which, void *arg);

		/** 
		 * @brief 网络读取数据回调 add by guozm 2012/04/04 
		 */
		static void onNetWriteProcessCallback(int fd, short which, void *arg);

	protected:
		/** 
		 * @brief 释放资源 add by guozm 2012/07/02 
		 */
		virtual void onRelease();
};

#endif
