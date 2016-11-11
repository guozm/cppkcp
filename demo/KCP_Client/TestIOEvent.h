/** 
* @file TestIOEvent.h
* @brief 
* @add by guozm guozm
* @version 
* @date 2012-04-11
*/

#ifndef __CTESTIOEVENT__H
#define __CTESTIOEVENT__H
#include "networkio.h"

class CTestIOEvent : public IOEvent
{
	public:
		/** 
		 * @brief 构造函数 add by guozm 2012/04/11 
		 */
		CTestIOEvent();

		/** 
		 * @brief 析够函数 add by guozm 2012/04/11 
		 */
		virtual ~CTestIOEvent();

		/** 
		* @brief 发送数据 add by guozm 2013/07/12 
		*/
		void WriteData(const char *sData, int nLen);

	protected:
		/** 
		 * @return  IO事件 add by guozm 2012/04/11 
		 */
		virtual int OnIOEvent(IOHandle* ioHandle, IOEventType enuEvent, char* dataBuffer, int nLen);
	
	private:
		IOHandle *m_ioHandle;
		long m_nCount;
};

#endif
