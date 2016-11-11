/** 
* @file TestDataTrans.h
* @brief 
* @add by guozm guozm
* @version 
* @date 2012-04-11
*/

#ifndef __CTESTDATATRANSEVENT__H
#define __CTESTDATATRANSEVENT__H
#include "networkio.h"

class CTestDataTrans : public IOEvent
{
	public:
		/** 
		 * @brief 构造函数 add by guozm 2012/04/11 
		 */
		CTestDataTrans(IOHandle *pIOHandle);

		/** 
		 * @brief 析够函数 add by guozm 2012/04/11 
		 */
		virtual ~CTestDataTrans();

		static void addDataTrans(CTestDataTrans *pIOEvent);

		static void delDataTrans(CTestDataTrans *pIOEvent);

		static void transferAll(char *dataBuffer, int nLength);

	protected:
		/** 
		 * @return  IO事件 add by guozm 2012/04/11 
		 */
		virtual int OnIOEvent(IOHandle* ioHandle, IOEventType enuEvent, char* dataBuffer, 
				int nLength);

	private:
		IOHandle *m_pIOHandle;
		static pthread_mutex_t m_mutex;
		static vector<CTestDataTrans*> m_IOEventVec;
};

#endif
