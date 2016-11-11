/** 
* @file TCPReadThread.h
* @brief libevent 工作线程 
* @add by guozm guozm
* @version 
* @date 2012-04-04
*/

#ifndef __CTCPREADTHREAD__H
#define __CTCPREADTHREAD__H
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
class CTCPReadThread : public CBaseIOThread
#else
class CTCPReadThread : public CBaseIOThreadWin
#endif
{
	public:
		/** 
		* @brief 构造函数 add by guozm 2012/04/04 
		*/
		CTCPReadThread();

		/** 
		* @brief 析够函数 add by guozm 2012/04/04 
		*/
		~CTCPReadThread();

		/** 
		* @brief  添加可用sockfd add by guozm 2012/04/13 
		*/
		void addAliveSockfdVec(int nfd);

		/** 
		* @brief  删除可用sockfd  add by guozm 2012/04/13 
		*/
		bool delAliveSockfdVec(int nfd);

		/** 
		 * @brief pipe数据回调 add by guozm 2012/04/04 
		 */
		static void onPipeProcessCallback(int fd, short which, void *arg);

		/** 
		 * @brief  构造出新的Session add by guozm 2012/04/04 
		 */
		CSession *newSession(int sfd, IBaseIOThread *pWriteIOThread, 
				ENETWORKTYPE eNetworkType, const IPAddr &mIPAddr);

		/** 
		 * @brief 网络读取数据回调 add by guozm 2012/04/04 
		 */
		static void onNetReadProcessCallback(int fd, short which, void *arg);

	protected:
		/** 
		 * @brief 释放资源 add by guozm 2012/07/02 
		 */
		virtual void onRelease();

	private:
		vector<int> m_AliveSockfdVec;   //可用的套接字
};

#endif
