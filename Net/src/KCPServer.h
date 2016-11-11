/** 
* @file KCPServer.h
* @brief 
* @add by guozm guozm
* @version 
* @date 2016-08-11
*/

#ifndef __CKCPSERVER__H
#define __CKCPSERVER__H
#include "Session.h"
#include "netdef.h"
#include "Mutex.h"

#ifndef WIN32
#include "BaseIOThread.h"
#else
#include "BaseIOThreadWin.h"
#endif

#include "IOService.h"

class NetFilter;
#ifndef WIN32
class CKCPServer : public CBaseIOThread
#else
class CKCPServer : public CBaseIOThreadWin
#endif
{
	public:
		/** 
		 * @brief  构造函数 add by guozm 2016/08/11 
		 */
		CKCPServer(int nSockfd, IOEvent *pIOEvent, 
				NetFilter *pNetFiler, vector<IBaseIOThread*> mReadVec, 
				vector<IBaseIOThread*> mWriteVec);

		/** 
		 * @brief 析构函数 add by guozm 2016/08/11 
		 */
		~CKCPServer();

		/** 
		 * @brief  构造出新的Session add by guozm 2012/04/04 
		 */
		CSession *newSession(ENETWORKTYPE eNetworkType, 
				const IPAddr &mIPAddr, struct sockaddr_in addr, int nfd);
		
		/** 
		 * @brief  获得AddrSessionMap add by guozm 2016/08/27 
		 */
		bool getAddrSessionMap_r(const string &sAddr, SServerObj **pObj);

		/** 
		 * @brief  添加 add by guozm 2016/08/27 
		 */
		void addAddrSServerObjMap_r(const string &sAddr, SServerObj *pObj);

		/** 
		 * @brief  删除Session add by guozm 2016/08/27 
		 */
		void delAddrSessionMap_r(const string &sAddr);

		/** 
		 * @brief pipe数据回调 add by guozm 2012/04/04 
		 */
		static void onPipeProcessCallback(int fd, short which, void *arg);

		/** 
		 * @brief 网络读取数据回调 add by guozm 2012/04/04 
		 */
		static void onNetReadProcessCallback(int fd, short which, void *arg);

		/** 
		 * @brief 网络读取数据回调 add by guozm 2012/04/04 
		 */
		static int udp_output(const char *buf, int len, ikcpcb *kcp, void *user);

	protected:
		/** 
		 * @brief 释放资源 add by guozm 2012/07/02 
		 */
		virtual void onRelease();

	private:
		struct event m_Event;
		IOEvent *m_pIOEvent;
		NetFilter *m_pNetFiler;
		vector<IBaseIOThread*> m_ReadThreadVec; 
		vector<IBaseIOThread*> m_WriteThreadVec; 
		unsigned int m_nReadCount;
		unsigned int m_nWriteCount;
		CMutex m_Mutex;
		map<string, SServerObj*> m_pAddrSServerObjMap;
		int m_nKcpServerVersion;
		char sNetBuffer[1500];
		char sKcpBuffer[64000];
};

#endif
