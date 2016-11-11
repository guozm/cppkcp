/** 
* @file KCPWriteThread.h
* @brief libevent 工作线程 
* @add by guozm guozm
* @version 
* @date 2012-04-04
*/

#ifndef __CKCPWRITETHREAD__H
#define __CKCPWRITETHREAD__H
#include "Session.h"
#include "netdef.h"
#include "kcpinc.h"

#ifndef WIN32
#include "BaseIOThread.h"
class CKCPWriteThread : public CBaseIOThread
#else
#include "BaseIOThreadWin.h"
class CKCPWriteThread : public CBaseIOThreadWin
#endif
{
	public:
		/** 
		* @brief 构造函数 add by guozm 2012/04/04 
		*/
		CKCPWriteThread();

		/** 
		* @brief 析够函数 add by guozm 2012/04/04 
		*/
		~CKCPWriteThread();

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
		 * @brief  调用kcp接口 add by guozm 2016/08/17 
		 */
		static void dokcp(CSession *pSession, const char *sBuf, int nLen);

		/** 
		 * @brief  调用kcp接口 add by guozm 2016/08/17 
		 */
		static void doUnreliabledata(CSession *pSession, const char *sBuf, int nLen);

		/** 
		 * @brief  设置写事件
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
