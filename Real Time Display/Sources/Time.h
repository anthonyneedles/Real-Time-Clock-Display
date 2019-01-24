/*******************************************************************************
* Time.h - Project header for Time.c
*
* Created on: 1/18/2017
* Author: Anthony Needles
*******************************************************************************/
#ifndef SOURCES_TIME_H_
#define SOURCES_TIME_H_

typedef struct { //Main structure used to hold current/buffer 24-hour times
    INT8U hr;
    INT8U min;
    INT8U sec;
}TIME_T;

/********************************************************************
* TimePend - Copies running time to passed time structure when a time change is
*            detected.
*
* Description:  This function will copy over running time to passed time
*               structure it can access timeMutexKey, just like TimeGet,
*               except only when the time changes. Used so that time to
*               display is only updated once a second.
*
* Return value: *ltime - Pointer to time structure to copy to
*
* Arguments:    None
*
* Anthony Needles - 01/23/18
********************************************************************/
void TimePend(TIME_T *ltime);
/********************************************************************
* TimeGet - Copies running time to passed time structure
*
* Description:  This function will copy over running time to passed time
*               structure granted it can access timeMutexKey, then returns key.
*               Used to 'Get' the current time.
*
* Return value: None
*
* Arguments:    *ltime - Pointer to time structure to copy to
*
* Anthony Needles - 01/23/18
********************************************************************/
void TimeGet(TIME_T *ltime);
/********************************************************************
* TimeSet - Copies passed time structure contents to current running time
*
* Description:  This function will copy over the passed time structure contents
*               to current running time, granted it can access timeMutexKey,
*               then returns key. Used to 'Set' the current time.
*
* Return value: None
*
* Arguments:    *ltime - Pointer to time structure to copy from
*
* Anthony Needles - 01/23/18
********************************************************************/
void TimeSet(TIME_T *ltime);
/********************************************************************
* TimeInit - Initializes time keeping processes
*
* Description:  This initialization routine creates 2 Semaphores and 1 Mutex
*               to be used in time keeping process, and enables RTC Seconds
*               IRQ for up counting. Creates timeTask.
*
* Return value: None
*
* Arguments:    None
*
* Anthony Needles - 01/22/18
********************************************************************/
void TimeInit(void);
/********************************************************************
* RTC_Seconds_IRQHandler - Sets timeSecFlag every one second
*
*
* Description:  This IRQ Handler enters upon RTC one second up count, and
*               posts timeSecFlag semaphore. Enables uCOS IRQ recognition.
*
* Return value: None
*
* Arguments:    None
*
* Anthony Needles - 01/31/18
********************************************************************/
void RTC_Seconds_IRQHandler(void);



#endif /* SOURCES_TIME_H_ */
