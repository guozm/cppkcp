#ifndef NETWORK_IO_H
#define NETWORK_IO_H

#ifdef WIN32
#ifdef NETWORK_IO_EXPORT
#define NETWORK_IO_API __declspec(dllexport)
#else
#define NETWORK_IO_API  
#endif
#else
#define NETWORK_IO_API  
#endif
#include "netdef.h"
#include "netfilter.h"
#ifdef WIN32
#include <memory>
#elif defined(IOS)||defined(MAC_OS)
#include <memory>
#else
#include <tr1/memory>
#endif

/** 
* @brief socket 类型
*/
enum ESOCKETTYPE
{
	SOCKETTYPE_NULL = 0,
	SOCKETTYPE_ACCEPT,           //accept新连接
	SOCKETTYPE_CONNECT           //connect新的连接
};

/*
 * 枚举类型，枚举所有的IO服务类型
 */
enum IOServiceType
{
	IO_SERVICE_UNKNOW = -1,
	IO_SERVICE_TCP_SERVER,
	IO_SERVICE_TCP_CLIENT,
	IO_SERVICE_KCP_SERVER,
	IO_SERVICE_KCP_CLIENT,
};

/** 
* @brief 网络类型 
*/
enum ENETWORKTYPE
{
	ENETWORKTYPE_NULL = 0,
	ENETWORKTYPE_TCP,
	ENETWORKTYPE_KCP,
};

/*
 * 网络事件枚举类型，枚举所有IO事件类型
 */
enum IOEventType
{
	IO_EVENT_UNKNOW = -1,

	IO_EVENT_INIT,          //服务器绑定IP、端口成功
	IO_EVENT_OPEN_OK,       //成功接收一个新的链接
	IO_EVENT_CLIENT_OPEN_OK,//连接服务器成功
	IO_EVENT_READ_OK,       //读取数据
	IO_EVENT_READMSG_OK,    //只是udx/kcp中使用
	IO_EVENT_WRITE_OK,      //发送数据成功
	IO_EVENT_WRITEMSG_OK,   //只是udx/kcp中使用
	IO_EVENT_WRITE_FAILED,  //发送数据失败

	IO_EVENT_OPEN_FAILED,   //连接服务器失败
	IO_EVENT_CLOSED,        //连接被关闭
};



class IOHandle;
class P2PConnector;
//网络事件回调接口
class IOEvent
{
	public:
	virtual ~IOEvent(){};
	virtual int OnIOEvent(IOHandle* ioHandle, IOEventType enuEvent, char* dataBuffer, int nLength) = 0;
	virtual int OnP2PIOEvent(P2PConnector* connector, IOEventType enuEvent, char* dataBuffer, int nLength){return 0;}
};

// 查询信息的类型
enum QueryInfoType
{
	QUERY_LOCAL_IP = 0,
	QUERY_LOCAL_PORT,
	QUERY_REMOTE_IP,
	QUERY_REMOTE_PORT,
};

//网络连接对象——对应每一个点对点连接
class IOHandle 
{
public:
	virtual ~IOHandle() {}

	/**
	* @brief 写数据  add by guozm 2012/12/17 
	*/
	virtual void Write(const char* dataBuffer, int nLength, bool bReliable = true) = 0;

	/** 
	 * @brief  发送控制信令(udx/kcp) add by guozm 2015/04/03 
	 */
	virtual void WriteMsg(const char* dataBuffer, int nLength) = 0;

	/**
	* 查询连接的相关信息
	*/
	virtual const char* QueryHandleInfo(QueryInfoType nKey) = 0;

	/** 
	* @brief  查询ENETWORKTYPE add by guozm 2012/11/01 
	*/
	virtual ENETWORKTYPE QueryENetWorkType() = 0;

	/** 
	* @brief 查询ENETWORKTYPE add by guozm 2012/11/01 
	*/
	virtual string QueryENetWorkTypeToString() = 0;

	/**
	* 关闭连接
	*/
	virtual void CloseHandle() = 0;

	/** 
	* @brief  注册IOEvent add by guozm 2012/04/13 
	*/
	virtual void Register(IOEvent *pIOEvent, NetFilter *pFilter = NULL) = 0;

	/** 
	* @brief  获得NetFilter 对象 add by guozm 2012/12/17 
	*/
	virtual NetFilter *getNetFilter() = 0;

	/** 
	 * @brief  获得socket 类型 add by guozm 2014/11/16 
	 */
	virtual ESOCKETTYPE getSocketType() = 0;

	/** 
	 * @brief  only kcp add by guozm 2014/11/16 
	 */
	virtual int getWaitSendSize()
	{
		return 0;
	}

	/** 
	* @brief  Can send data? for udx
	*/
	virtual bool CanSend() {return true;}

	/** 
	* @brief  Set send buf length, for udx
	*/
	virtual void SetSendBuf(int nLen) {}
};

extern "C"
{
	/**
	 *  * 提供创建网络服务的工厂类接口
	 *   */
	//NETWORK_IO_API IOFactory* CreateIOFacotry();
	NETWORK_IO_API bool CreateServerIOService(IOServiceType serviceType, const char* sSrcIP,
			int nSrcPort, IOEvent *pEventObj, NetFilter *pFilter, int nThreadNum, 
			bool bSync);

	NETWORK_IO_API bool CreateClientIOService(IOServiceType serviceType, const char* sSrcIP,
			int nSrcPort, const char* sDstIP, int nDstPort, IOEvent *pEventObj,
			NetFilter *pFilter, int nThreadNum, bool bSync);
};

#endif
