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

		static void pushIOEvent(CTestIOEvent *pIOEvent);

		static void transferAll(char *dataBuffer, int nLength);

	protected:
		/** 
		 * @return  IO事件 add by guozm 2012/04/11 
		 */
		virtual int OnIOEvent(IOHandle* ioHandle, IOEventType enuEvent, char* dataBuffer, 
				int nLength);
	private:
		IOHandle* m_ioHandle;

};

#endif
