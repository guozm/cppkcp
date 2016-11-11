#ifndef __ioutils_hpp__
#define __ioutils_hpp__
#include <sys/types.h>          /* See NOTES */
#include <string>
#include "config.h"
using namespace std;

#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#endif

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#ifdef WIN32
	#define socklen_t int
#else
inline int closesocket(int s)
{
	return close(s);
}
#endif
class IOEvent;
class CUdxTcpHandler;
class CIOService;

class CIOUtils {
public:
	static int setNonblock( int fd );

	static int setBlock( int fd );

	/** 
	* @brief  创建pipe add by guozm 2012/10/12 
	*/
	static int createPipe(int *fds);

	/* tcp/sctp 共用*/
	static int Accept(int nSockfd, string &sRemoteIP, string &sRemotePort);

	/************************************* TCP ***************************************/
	static int tcpListen( const char * ip, int port, int * fd, int blocking = 1 );

	static int tcpConnect(const char *ip, int port, int *fd, string &sLocalIP, string &sLocalPort);

	/************************************* UDP ***************************************/
	static int udpListen( const char * ip, int port, int * fd, int blocking = 1 );

	static int udpConnect(const char *ip, int port, int *fd, string &sLocalIP, string &sLocalPort);

private:
	CIOUtils();
};

#endif

