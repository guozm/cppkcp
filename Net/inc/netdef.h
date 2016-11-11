/** 
* @file def.h
* @brief 
* @add by guozm guozm
* @version 
* @date 2012-04-04
*/

#ifndef __CDEF__H
#define __CDEF__H

#ifdef WIN32
#include <winsock2.h>
#include <io.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <iostream>
#include <event.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <vector>
#include <map>
#include <queue>
#include <errno.h>
#include <string.h>
#include <algorithm>
using namespace std;

#endif
