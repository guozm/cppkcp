/** 
* @file Session.h
* @brief 会话类 
* @add by guozm guozm
* @version 
* @date 2012-04-04
*/

#ifndef __CSESSION__H
#define __CSESSION__H
#include "netdef.h"
#include "networkio.h"
#include "IBaseIOThread.h"
#include "netfilter.h"
#include "IOService.h"
#include "ikcp.h"
#include "kcpinc.h"

class CKCPClient;
class CSession : public IOHandle
{
	public:
		/** 
		 * @brief 构造函数 add by guozm 2012/04/04 
		 */
		CSession(IBaseIOThread *pWriteIOThread, IBaseIOThread *pReadIOThread, 
				ENETWORKTYPE eNetworkType, const IPAddr &mIPAddr);

		/** 
		 * @brief 构造函数 add by guozm 2012/04/04 
		 */
		CSession(IBaseIOThread *pWriteIOThread, IBaseIOThread *pReadIOThread, 
				ENETWORKTYPE eNetworkType, const IPAddr &mIPAddr,
				struct sockaddr_in addr, int nfd, ESOCKETTYPE eSocketType);

		/** 
		 * @brief 构造函数 add by guozm 2012/04/04 
		 */
		CSession(IBaseIOThread *pWriteIOThread, IBaseIOThread *pReadIOThread, 
				ENETWORKTYPE eNetworkType, const IPAddr &mIPAddr,
				long long nServerObj, CKCPClient *pKCPClient);

		/** 
		 * @brief 析够函数 add by guozm 2012/04/04 
		 */
		virtual ~CSession();

		/** 
		* @brief 释放kcp add by guozm 2016/09/08 
		*/
		void releaseKcp();

		/** 
		* @brief  初始化 add by guozm 2016/08/25 
		*/
		void initObj();

		/** 
		 * @brief  设置KCPServerObj add by guozm 2016/10/28 
		 */
		void setKCPServerObj(SServerObj *pObj);

		/** 
		 * @brief  获得 add by guozm 2016/10/29 
		 */
		SServerObj *getSServerObj();

		/** 
		* @brief  发送消息 add by guozm 2012/04/12 
		*/
		virtual void Write(const char* dataBuffer, int nLength, bool bReliable = true);

		/** 
		 * @brief  发送控制信令(kcp) add by guozm 2015/04/03 
		 */
		virtual void WriteMsg(const char* dataBuffer, int nLength);

		/** 
		* @brief  查询信息 add by guozm 2012/06/14 
		*/
		virtual const char* QueryHandleInfo(QueryInfoType nKey);

		/** 
		 * @brief  查询ENETWORKTYPE add by guozm 2012/11/01 
		 */
		virtual ENETWORKTYPE QueryENetWorkType();

		/** 
		 * @brief 查询ENETWORKTYPE add by guozm 2012/11/01 
		 */
		virtual string QueryENetWorkTypeToString();

		/** 
		* @brief 关闭连接 add by guozm 2012/04/24  
		*/
		virtual void CloseHandle();

		/** 
		* @brief  注册事件处理类 add by guozm 2012/04/12 
		*/
		virtual void Register(IOEvent *pIOEvent, NetFilter *pFilter = NULL);

		/** 
		* @brief  获得NetFilter add by guozm 2012/12/17 
		*/
		NetFilter *getNetFilter();

		/** 
		 * @brief  获得socket 类型 add by guozm 2014/11/16 
		 */
		virtual ESOCKETTYPE getSocketType();

		/** 
		 * @brief  only kcpadd by guozm 2014/11/16 
		 */
		virtual int getWaitSendSize();

		/** 
		 * @brief  Can send data? for udx/kcp
		 */
		virtual bool CanSend();

	public:
		/** 
		 * @brief  创建kcp对象 add by guozm 2016/08/16 
		 */
		void createKcpObj(long long nKcpNum);

		/** 
		 * @brief  获得kcp对象 add by guozm 2016/08/19 
		 */
		CMutexProxy *getKcpObj(ikcpcb **pKcpObj);

		/** 
		 * @brief  kcpconnect成功 add by guozm 2016/08/18 
		 */
		void onKcpConnectSuccess(int nfd, CIOService *pIOService);

		/** 
		 * @brief  收到kcp数据 add by guozm 2016/08/12 
		 */
		void onRecvKcpData(char *sData, int nLen);

		/** 
		 * @brief  收到kcp数据 add by guozm 2016/08/12 
		 */
		void onRecvKcpDataMsg(char *sData, int nLen);

		/** 
		 * @brief  断开连接 add by guozm 2012/04/12 
		 */
		void onRecvKcpClose();

		/** 
		 * @brief  收到readcloseok add by guozm 2016/08/16 
		 */
		void onRecvKcpReadCloseOK();

		/** 
		 * @brief 发送close消息 add by guozm 2016/08/16 
		 */
		void sendKcpClose();

		/** 
		* @brief  发送create消息 add by guozm 2016/08/25 
		*/
		void sendKcpCreate();

		/** 
		* @brief 发送kcp心跳包 add by guozm 2016/08/27 
		*/
		void sendKcpHeartbeat();

		/** 
		 * @brief 发送kcp数据 add by guozm 2016/08/27 
		 */
		void sendKcpData(ESOCKETSTATUS eSocketStatus, const char* dataBuffer = NULL, 
				int nLength = 0);

		/** 
		 * @brief  连接服务器成功 add by guozm 2012/04/13 
		 */
		void onConnectSuccess(int nfd, CIOService *pIOService);

		/** 
		 * @brief  新的连接 add by guozm 2012/04/12 
		 */
		void onNetConnect(int nfd);

		/** 
		 * @brief  新的连接 add by guozm 2012/04/12 
		 */
		void onNetKcpConnect();

		/** 
		 * @brief  收到网络数据 add by guozm 2012/04/12 
		 */
		void onRecvData(char *sData, int nLen);

		/** 
		 * @brief  收到网络数据(kcp) add by guozm 2012/04/12 
		 */
		void onRecvDataMsg(char *sData, int nLen);

		/** 
		 * @brief  断开连接 add by guozm 2012/04/12 
		 */
		void onRecvClose();

		/** 
		 * @brief  断开连接 add by guozm 2012/04/12 
		 */
		void onRecvTcpClose();

		/** 
		* @brief 断开连接 add by guozm 2012/04/12 
		*/
		void onRecvAppClose();

		/** 
		* @brief 写线程正常退出 add by guozm 2012/06/13 
		*/
		void onWriteThreadCloseOK();

		/** 
		 * @brief 写线程正常退出 add by guozm 2012/06/13 
		 */
		void onTcpWriteThreadCloseOK();

		/** 
		 * @brief 写线程正常退出 add by guozm 2012/06/13 
		 */
		void onKcpWriteThreadCloseOK();

		/** 
		* @brief 写数据失败 add by guozm 2012/08/28 
		*/
		void onWriteFailure();

		/** 
		* @brief  写事件成功 add by guozm 2012/11/09 
		*/
		void onWriteOK(char *dataBuffer, int nLength);

		/** 
		 * @brief  写事件成功 add by guozm 2012/11/09 
		 */
		void onWriteMsgOK(char *dataBuffer, int nLength);

		/** 
		* @brief  收到网络事件 add by guozm 2012/06/15 
		*/
		void onIOEvent(IOEventType enuEvent, char* dataBuffer, int nLength);

	public:
		/** 
		 * @brief 网络读取数据回调 add by guozm 2012/04/04 
		 */
		static void onNetReadProcessCallback(int fd, short which, void *arg);

	private:

		/** 
		* @brief  添加到写的队列 add by guozm 2012/06/14 
		*/
		void addWriteQue(const char* dataBuffer, int nLength, ESOCKETSTATUS eSocketStatus);

		/** 
		* @brief 通知写线程进行关闭 add by guozm 2012/07/02 
		*/
		void notifyCloseToWriteThread();

		/** 
		 * @brief 通知读线程关闭 add by guozm 2012/07/03 
		 */
		void notifyTcpCloseToReadThread();

		/** 
		 * @brief 通知读线程关闭 add by guozm 2012/07/03 
		 */
		void notifyKcpCloseToReadThread();

		/** 
		 * @brief 发送消息 add by guozm 2012/07/03 
		 */
		void writedata(const char* dataBuffer, int nLength, ESOCKETSTATUS eSocketStatus);

	public:
		/** 
		* @brief  插入数据 add by guozm 2012/09/02 
		*/
		int pushWriteQue(CQ_WRITE_ITEM *pItme);

		/** 
		* @brief  写数据头 add by guozm 2012/09/02 
		*/
		CQ_WRITE_ITEM *writeQueFront();

		/** 
		* @brief 清空发送缓冲区 add by guozm 2012/09/02 
		*/
		void clearWriteQue();

		/** 
		* @brief  队列大小 add by guozm 2012/09/02 
		*/
		int writeQueSize();

	private:
		friend class CTCPReadThread;
		friend class CTCPWriteThread;
		friend class CKCPReadThread;
		friend class CKCPWriteThread;
		friend class CKCPClient;
		friend class CKCPServer;
		struct event m_readEvent;
		struct event m_writeEvent;
		IBaseIOThread *m_pWriteThread;
		IBaseIOThread *m_pReadThread;
		IOEvent *m_pIOEvent;
		NetFilter *m_pNetFilter;
		int m_nfd;                   //网络句柄
		static const int m_nBufferLen = 65535;
		char m_pBuffer[m_nBufferLen];
		CIOService *m_pIOService;   //当作为Client保存CIOService 当Close的时候删除对象

		pthread_mutex_t m_mutex;
		queue<CQ_WRITE_ITEM*> m_WriteQueue;   //队列   
		SServerObj *m_pSServerObj;
	public:
		bool volatile m_bClose;               //标识连接是否被关闭
		bool volatile m_bSendCloseHandle;
		bool volatile m_bSendError;           //发送失败, 不在发送数据等待网络关闭
		bool volatile m_bKcpNetConnect;
		IPAddr m_IPAddr;
		struct sockaddr_in m_addr;            //kcp用此变量
		ENETWORKTYPE m_eNetWorkType;
		ESOCKETTYPE m_eSocketType;
		long long m_nServerObj;                    //kcp存放server对象
		ikcpcb* m_pkcp;
		long long m_nKcpNum;
		CMutex m_pkcpMutex;
		int m_nRecvHBCount;
		int m_nCloseCount;
		CKCPClient *m_pKCPClient;
		int m_nKcpServerVersion;
};

#endif
