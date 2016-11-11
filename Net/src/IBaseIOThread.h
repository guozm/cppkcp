/** 
* @file IBaseIOThread.h
* @brief IBaseIOThread 
* @add by guozm guozm
* @version 
* @date 2012-10-11
*/

#ifndef __IBASEIOTHREAD__H
#define __IBASEIOTHREAD__H
#include "netdef.h"
#include "networkio.h"
#include "IoUtils.h"
#include "net_type.h"
#include "Mutex.h"

#ifdef WIN32
#include <Windows.h>
#endif

class IBaseIOThread
{
	public:

		/** 
		* @brief  构造函数 add by guozm 2012/10/11 
		*/
		IBaseIOThread()
		{
			int fds[2];
			CIOUtils::createPipe(fds);
			notify_receive_fd = fds[0];
			notify_send_fd = fds[1];
		}

		/** 
		* @brief 析够函数 add by guozm 2012/10/11 
		*/
		virtual ~IBaseIOThread()
		{
			closesocket(notify_receive_fd);
			closesocket(notify_send_fd);
		}

		/** 
		 * @brief  push add by guozm 2012/04/12 
		 */
		int push(void *pVoid)
		{
			CMutexProxy mLock(m_Mutex);
			int nSize = 0;
			m_Queue.push(pVoid);
			nSize = m_Queue.size();
			return nSize;
		}

		CMutexProxy *push(void *pVoid, int &nSize)
		{
			CMutexProxy *pMutexProxy = new CMutexProxy(m_Mutex);
			m_Queue.push(pVoid);
			nSize = m_Queue.size();
			return pMutexProxy;
		}

		/** 
		 * @brief  pop add by guozm 2012/04/12 
		 */
		void *pop()
		{
			CMutexProxy mLock(m_Mutex);
			if(0 == m_Queue.size())
			{
				return NULL;
			}
			void *p = m_Queue.front();
			m_Queue.pop();
			return p;
		}

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
				CQ_ITEM *pDel = (CQ_ITEM*)p;
				delete pDel;
				pDel = NULL;
			}
		}

		/** 
		 * @brief  线程函数 add by guozm 2012/04/04 
		 */
		static void *threadFun(void *argc)
		{
			IBaseIOThread *p = static_cast<IBaseIOThread*>(argc);
			p->onThreadRun();
			return NULL;
		}

		/** 
		 * @brief 启动线程 add by guozm 2012/04/05 
		 */
		void startThread()
		{
			pthread_t       thread;
			int             ret;
			if((ret = pthread_create(&thread, NULL, threadFun, this)) != 0) 
			{
				printf("pthread_create is failure ... error = %s\n", strerror(errno));
				assert(0);
			}

			pthread_detach(thread);
		}

		/** 
		 * @brief 停止线程 add by guozm 2012/04/14 
		 */
		void stopThread()
		{
			//清空队列
			clearQue();

			//通知子类释放资源
			onRelease();
		}

	protected:
		/** 
		 * @brief 释放资源 add by guozm 2012/10/11 
		 */
		virtual void onRelease() = 0;

		/** 
		* @brief 线程运行 add by guozm 2012/10/11 
		*/
		virtual void onThreadRun() = 0;

	protected:
		CMutex m_Mutex;
		queue<void*> m_Queue;   //队列   

	public:
		int notify_receive_fd;      /* receiving end of notify pipe */
		int notify_send_fd;         /* sending end of notify pipe */
};

#endif
