#include "Mutex.h"
#include <stdlib.h>
#include <assert.h>

inline void abortingMutexCall(int funcresult)
{
	if(funcresult != 0)
	{
		abort();
	}
}

CMutex::CMutex()
{
	m_pid = 0;
	abortingMutexCall(pthread_mutex_init(&m_lock, NULL));
}

CMutex::~CMutex()
{
	abortingMutexCall(pthread_mutex_destroy(&m_lock));
}

void CMutex::lock()
{
	if(m_pid == pthread_self())
	{
		assert(0);
	}
	m_pid = pthread_self();
	abortingMutexCall(pthread_mutex_lock(&m_lock));
}

void CMutex::unlock()
{
	m_pid = 0;
	abortingMutexCall(pthread_mutex_unlock(&m_lock));
}

CMutexProxy::CMutexProxy(CMutex& m) : m_m(m)
{
	m_m.lock();
}

CMutexProxy::~CMutexProxy()
{
	m_m.unlock();
}

CRWMutex::CRWMutex()
{
	abortingMutexCall(pthread_rwlock_init(&m_lock, NULL)); 
}

CRWMutex::~CRWMutex()
{
	abortingMutexCall(pthread_rwlock_destroy(&m_lock)); 
}

void CRWMutex::rLock()
{
	abortingMutexCall(pthread_rwlock_rdlock(&m_lock));
}

void CRWMutex::rUnLock()
{
	abortingMutexCall(pthread_rwlock_unlock(&m_lock));
}

void CRWMutex::wLock()
{
	abortingMutexCall(pthread_rwlock_wrlock(&m_lock));
}

void CRWMutex::wUnLock()
{
	abortingMutexCall(pthread_rwlock_unlock(&m_lock));
}

CReadMutexProxy::CReadMutexProxy(CRWMutex& m) : m_m(m)
{
	m_m.rLock();
}

CReadMutexProxy::~CReadMutexProxy()
{
	m_m.rUnLock();
}

CWriteMutexProxy::CWriteMutexProxy(CRWMutex& m) : m_m(m)
{
	m_m.wLock();
}

CWriteMutexProxy::~CWriteMutexProxy()
{
	m_m.wUnLock();
}
