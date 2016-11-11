/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */
#ifdef WIN32
#include <winsock2.h>
#include <Ws2tcpip.h>
#define bzero(a,b) memset(a,0,b)
#endif
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include "IoUtils.h"
#ifdef IOS
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif
#include <sstream>
#include <netdb.h>

int CIOUtils::setNonblock( int fd )
{
#ifdef WIN32
	unsigned long nonblocking = 1;
	ioctlsocket( fd, FIONBIO, (unsigned long*) &nonblocking );
#else
	int flags;

	flags = fcntl( fd, F_GETFL );
	if( flags < 0 ) return flags;

	flags |= O_NONBLOCK;
	if( fcntl( fd, F_SETFL, flags ) < 0 ) return -1;
#endif

	return 0;
}

int CIOUtils::setBlock(int fd)
{
#ifdef WIN32
	unsigned long nonblocking = 0;
	ioctlsocket( fd, FIONBIO, (unsigned long*) &nonblocking );
#else
	int flags;
	flags = fcntl( fd, F_GETFL );
	if( flags < 0 ) return flags;

	flags &= ~O_NONBLOCK;
	if( fcntl( fd, F_SETFL, flags ) < 0 ) return -1;
#endif
	return 0;
}

#ifdef WIN32
int createSocketpair(int* fds)
{
	int sRcv = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in addr;
	memset( &addr, 0, sizeof( addr ) );
	addr.sin_family = AF_INET;
	addr.sin_port = 0;
	addr.sin_addr.s_addr = 0x0100007f; // 127.0.0.1
	if (bind(sRcv , (struct sockaddr*)&addr, sizeof( addr ) ) < 0)
	{
		ASSERTS(0, "socketpair: Failed to bind %d", sRcv);
		return -1;
	}
	int nameLen = sizeof(addr);
	getsockname(sRcv, (struct sockaddr*)&addr, &nameLen);


	int sSend = socket(AF_INET, SOCK_DGRAM, 0);
	if (connect(sSend, (struct sockaddr*)&addr, sizeof( addr ) ) < 0)
	{
		ASSERTS(0, "socketpair: Failed to connect %d   port=%d", sSend, addr.sin_port);
		return -1;
	}

	fds[0] = sRcv;
	fds[1] = sSend;
	return 0;
}
#endif

int CIOUtils::createPipe(int *fds)
{
#ifdef WIN32
	if (createSocketpair(fds) != 0)
#else
	if(socketpair(AF_UNIX, SOCK_STREAM, 0, fds)!= 0)
#endif
	{
		perror("Can't create notify pipe");
		return -1;
	}
#ifndef WIN32
	setNonblock(fds[0]);
	setNonblock(fds[1]);
#endif
	return 0;
}

int CIOUtils::tcpListen(const char * ip, int port, int * fd, int blocking)
{
	int ret = 0;

	int listenFd = socket( AF_INET, SOCK_STREAM, 0 );
	if( listenFd < 0 ) 
	{
#ifdef WIN32
		printf("socket failed, errno %d\n", WSAGetLastError());
#else
		printf("socket failed, errno %d, %s\n", errno, strerror( errno ) );
#endif
		ret = -1;
	}

	if( 0 == ret && 0 == blocking ) 
	{
		if( setNonblock( listenFd ) < 0 ) 
		{
			printf("%s", "failed to set socket to non-blocking\n");
			ret = -1;
		}
	}

	if( 0 == ret ) 
	{
		int flags = 1;
		if( setsockopt( listenFd, SOL_SOCKET, SO_REUSEADDR, (char*)&flags, sizeof( flags ) ) < 0 ) 
		{
			printf("%s", "failed to set setsock to reuseaddr\n" );
			ret = -1;
		}
		if( setsockopt(listenFd, IPPROTO_TCP, TCP_NODELAY, (char*)&flags, sizeof(flags) ) < 0 ) 
		{
			printf("%s", "failed to set socket to nodelay\n" );
			ret = -1;
		}
	}

	struct sockaddr_in addr;
	if( 0 == ret ) 
	{
		memset( &addr, 0, sizeof( addr ) );
		addr.sin_family = AF_INET;
		addr.sin_port = htons( port );

		addr.sin_addr.s_addr = INADDR_ANY;
		if( '\0' != *ip ) 
		{
#ifdef WIN32
			struct in_addr in_val;
			in_val.s_addr = inet_addr(ip);
			memcpy(&addr.sin_addr,&in_val,sizeof(in_addr));
#else
			if( 0 == inet_aton( ip, &addr.sin_addr ) ) {
				printf("failed to convert %s to inet_addr\n", ip );
				ret = -1;
			}
#endif
		}
	}

	if( 0 == ret ) 
	{
		if( ::bind( listenFd, (struct sockaddr*)&addr, sizeof( addr ) ) < 0 ) {
#ifdef WIN32
			printf("bind failed, errno %d\n", WSAGetLastError());
#else
			printf("bind failed, errno %d, %s\n", errno, strerror( errno ) );
#endif
			ret = -1;
		}
	}

	if( 0 == ret ) 
	{
		if( ::listen( listenFd, 1024 ) < 0 ) 
		{
#ifdef WIN32
			printf("listen failed, errno %d\n", WSAGetLastError());
#else
			printf("listen failed, errno %d, %s\n", errno, strerror( errno ) );
#endif
			ret = -1;
		}
	}

	if( 0 != ret && listenFd >= 0 ) 
	{
		closesocket( listenFd );
	}

	if( 0 == ret ) 
	{
		* fd = listenFd;
		printf("tcp Listen on ip = %s port = %d success...", ip, port);
	}
	else
	{
		printf("tcp Listen on ip = %s port = %d failure...", ip, port);
	}

	return ret;
}

int CIOUtils::udpListen( const char * ip, int port, int * fd, int blocking)
{
	int ret = 0;
	int listenFd = socket(AF_INET, SOCK_DGRAM, 0);
	if( listenFd < 0 ) 
	{
#ifdef WIN32
		printf("socket failed, errno %d\n", WSAGetLastError());
#else
		printf("socket failed, errno %d, %s\n", errno, strerror( errno ) );
#endif
		ret = -1;
	}

	if( 0 == ret && 0 == blocking ) 
	{
		if( setNonblock( listenFd ) < 0 ) 
		{
			printf("%s", "failed to set socket to non-blocking\n");
			ret = -1;
		}
	}

	struct sockaddr_in addr;
	if( 0 == ret )
	{
		memset( &addr, 0, sizeof( addr ));
		addr.sin_family = AF_INET;
		addr.sin_port = htons( port );

		addr.sin_addr.s_addr = INADDR_ANY;
		if( '\0' != *ip ) 
		{
#ifdef WIN32
			struct in_addr in_val;
			in_val.s_addr = inet_addr(ip);
			memcpy(&addr.sin_addr,&in_val,sizeof(in_addr));
#else
			if( 0 == inet_aton( ip, &addr.sin_addr)) 
			{
				printf("failed to convert %s to inet_addr\n", ip );
				ret = -1;
			}
#endif
		}
	}

	if( 0 == ret ) 
	{
		if( ::bind( listenFd, (struct sockaddr*)&addr, sizeof( addr ) ) < 0 )
		{
#ifdef WIN32
			printf("bind failed, errno %d\n", WSAGetLastError());
#else
			printf("bind failed, errno %d, %s\n", errno, strerror( errno ) );
#endif
			ret = -1;
		}
	}

	if( 0 == ret ) 
	{
		int flags = 1;
		if( setsockopt( listenFd, SOL_SOCKET, SO_REUSEADDR, (char*)&flags, sizeof( flags ) ) < 0 ) 
		{
			printf("%s", "failed to set setsock to reuseaddr\n" );
			ret = -1;
		}
	}

	if( 0 == ret ) 
	{
		* fd = listenFd;
		printf("udp Listen on ip = %s port = %d\n", ip, port);
	}

	return ret;
}

int CIOUtils::Accept(int nSockfd, string &sRemoteIP, string &sRemotePort)
{
	struct sockaddr_in addr;
	socklen_t addrLen = sizeof( addr );
	int nClientFD = accept(nSockfd, (struct sockaddr *)&addr, &addrLen );
	if(nClientFD != -1)
	{
		CIOUtils::setNonblock( nClientFD );
	}

	if(nClientFD != -1)
	{
		int flags = 1;
		int nErr = setsockopt(nSockfd, IPPROTO_TCP, TCP_NODELAY, (char*)&flags, sizeof(flags)); 
		if(nErr == -1)
		{
			printf("setsockopt is faild error = %s\n", strerror(errno));
		} 
	}

	sRemoteIP = inet_ntoa(addr.sin_addr);
	stringstream strRemotePort;
	strRemotePort<<htons(addr.sin_port);
	sRemotePort = strRemotePort.str();
	return nClientFD;
}

int CIOUtils::tcpConnect(const char *ip, int port, int *fd, string &sLocalIP, string &sLocalPort)
{    
	int nSockfd = socket(AF_INET , SOCK_STREAM , 0);
	if(-1 == nSockfd)
	{
#ifdef WIN32
		printf("nSockfd == -1 errno = %d\n", WSAGetLastError());
#else
		printf("nSockfd == -1 errno = %d error = %s\n", errno, strerror(errno));
#endif
		return -1;

	}

	//域名转换成ip
	char sIP[64] = {0};
	hostent* host_entry = gethostbyname(ip);
	if(host_entry !=0)
	{
		sprintf(sIP, "%d.%d.%d.%d",(host_entry->h_addr_list[0][0]&0x00ff),
				(host_entry->h_addr_list[0][1]&0x00ff),
				(host_entry->h_addr_list[0][2]&0x00ff),
				(host_entry->h_addr_list[0][3]&0x00ff));
	}

	struct sockaddr_in cliaddr;
	bzero(&cliaddr , sizeof(cliaddr));

	cliaddr.sin_family = PF_INET;
	cliaddr.sin_port = htons(port);
#ifdef WIN32
	struct in_addr in_val;
	in_val.s_addr = inet_addr(sIP);
	memcpy(&cliaddr.sin_addr,&in_val,sizeof(in_addr));
	int nRet;
#else
	int nRet = inet_pton(PF_INET , sIP , &cliaddr.sin_addr);
	if(nRet == -1)
	{
		printf("%s", "inet_pton is error\n");
		closesocket(nSockfd);
		return -1;
	}
#endif

	if(nSockfd != -1)
	{
		CIOUtils::setNonblock(nSockfd);
	}

	if(connect(nSockfd , (struct sockaddr*)&cliaddr , sizeof(cliaddr)) == -1) 
	{
#ifndef WIN32
		if(errno == EINPROGRESS)
#else
		int nError = WSAGetLastError();
		if (nError == WSAEWOULDBLOCK)
#endif
		{
			// it is in the connect process
			struct timeval tv;
			fd_set writefds;
			tv.tv_sec = 3;
			tv.tv_usec = 0;
			FD_ZERO(&writefds);
			FD_SET(nSockfd, &writefds);
			if(select(nSockfd+1, NULL, &writefds, NULL, &tv)>0)
			{
				socklen_t len=sizeof(socklen_t);
				int error;
				//下面的一句一定要，主要针对防火墙
				getsockopt(nSockfd, SOL_SOCKET, SO_ERROR, (char*)&error, &len);
				if(error==0) 
					nRet=0;
				else 
					nRet=-1;
			}
			else   
				nRet=-1;//timeout or error happen
		}
		else 
			nRet=-1;
	}
	else    
		nRet=0; 

	if(-1 == nRet)
	{
#ifdef WIN32
		printf("tcp connect ip = %s port = %d is failure .... error = %d\n", 
				ip, port, WSAGetLastError());
#else
		printf("tcp connect ip = %s port = %d is failure .... error = %s\n", 
				ip, port, strerror(errno));
#endif
		closesocket(nSockfd);
		return -1;
	}

	if(nSockfd != -1)
	{
		int flags = 1;
		int nErr = setsockopt(nSockfd, IPPROTO_TCP, TCP_NODELAY, (char*)&flags, sizeof(flags)); 
		if(nErr == -1)
		{
			printf("setsockopt is faild error = %s\n", strerror(errno));
			closesocket(nSockfd);
			return -1;
		} 
	}

	struct sockaddr_in addrMy;
	memset(&addrMy, 0, sizeof(addrMy));
	socklen_t len = sizeof(addrMy);
	nRet = getsockname(nSockfd, (struct sockaddr*)&addrMy, &len);
	if(nRet != 0)
	{
		printf("getsockname is faild error = %s\n", strerror(errno));
		closesocket(nSockfd);
		return -1;
	}

	sLocalIP = inet_ntoa(addrMy.sin_addr);
	stringstream strLocalPort;
	strLocalPort<<ntohs(addrMy.sin_port);
	sLocalPort = strLocalPort.str();

	*fd = nSockfd;
	return 0;
}

int CIOUtils::udpConnect(const char *ip, int port, int *fd, string &sLocalIP, string &sLocalPort)
{
	//域名转换成ip
	char sIP[64] = {0};
	hostent* host_entry = gethostbyname(ip);
	if(host_entry !=0)
	{
		sprintf(sIP, "%d.%d.%d.%d",(host_entry->h_addr_list[0][0]&0x00ff),
				(host_entry->h_addr_list[0][1]&0x00ff),
				(host_entry->h_addr_list[0][2]&0x00ff),
				(host_entry->h_addr_list[0][3]&0x00ff));
	}

	  //配置想要的地址信息
    struct addrinfo addrCriteria;
    memset(&addrCriteria,0,sizeof(addrCriteria));
    addrCriteria.ai_family=AF_UNSPEC;
    addrCriteria.ai_socktype=SOCK_DGRAM;
    addrCriteria.ai_protocol=IPPROTO_UDP;
    
    struct addrinfo *server_addr;
    stringstream str;
    str<<port;

    //获取地址信息
    int retVal=getaddrinfo(sIP, str.str().c_str(), &addrCriteria, &server_addr);
    if(retVal!=0)
        printf("getaddrinfo() failed! = %s",gai_strerror(retVal));
    int sock=-1;
    struct addrinfo *addr=server_addr;
    while(addr!=NULL)
    {
        if(addr->ai_family == AF_INET)
        {
            if(((struct sockaddr_in *)addr->ai_addr)->sin_port == 0)
            {
                ((struct sockaddr_in *)addr->ai_addr)->sin_port = htons(port);
            }
        }
        else if(addr->ai_family == AF_INET6)
        {
            printf("AF_INET6 port = %d", ((struct sockaddr_in6 *)addr->ai_addr)->sin6_port);
            if(((struct sockaddr_in6 *)addr->ai_addr)->sin6_port == 0)
            {
                ((struct sockaddr_in6 *)addr->ai_addr)->sin6_port = htons(port);
            }
        }
        
		//建立socket
		sock=socket(addr->ai_family,addr->ai_socktype,addr->ai_protocol);
		if(sock<0)
		{
			continue;
		}

        if(connect(sock,addr->ai_addr,addr->ai_addrlen)==0)
        {
#ifdef IOS
			int set = 1;
			setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
#endif
            
            break;
        }
        else
        {
#ifdef WIN32
			printf("udp connect sIP = %s port = %d is failure .... error = %d\n", 
					sIP, port, WSAGetLastError());
#else
			printf("connect sIP = %s port = %d is failure .... error = %s\n", 
					sIP, port, strerror(errno));
#endif
			closesocket(sock);
		}

		//没有链接成功，就继续尝试下一个
		sock=-1;
        addr=addr->ai_next;
    }
    freeaddrinfo(server_addr);
    *fd = sock;

	int bResult = -1;
	if(sock != -1)
	{
		printf("udp connect sIP = %s port = %d is successful ....", sIP, port);
		bResult = 0;
		CIOUtils::setNonblock(sock);
	}
	return bResult;
}
