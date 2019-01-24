/*******************************************************************************
* Time.c - This module handles all time keeping and updated routines. Hosts
*          running time value in with mutex key which can be updated or grabbed
*          by main module via included functions. Uses RTC seconds IRQ to count
*          up once a second.
*
* Created on: 1/18/18
* Author: Anthony Needles
*******************************************************************************/
#include "MCUType.h"
#include "app_cfg.h"
#include "os.h"
#include "Time.h"
#include "K65TWR_GPIO.h"

static void timeTask(void *p_arg);
static OS_TCB ApptimeTaskTCB;
static CPU_STK  timeTaskStk[APP_CFG_TIME_TASK_STK_SIZE];
static TIME_T timeOfDay;
static OS_SEM timeChgFlag;
static OS_MUTEX timeMutexKey;
static OS_SEM timeSecFlag;

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
void TimeInit(void){
    OS_ERR os_err;

    OSSemCreate(&timeChgFlag, "Time Change Flag", 0, &os_err);
    while(os_err != OS_ERR_NONE){}

    OSSemCreate(&timeSecFlag, "Time Seconds Flag", 0, &os_err);
    while(os_err != OS_ERR_NONE){}

    OSMutexCreate(&timeMutexKey, "Time Mutex", &os_err);
    while(os_err != OS_ERR_NONE){}

    timeOfDay.hr = 12;
    timeOfDay.min = 0;
    timeOfDay.sec = 0;

    NVIC_ClearPendingIRQ(RTC_Seconds_IRQn);
    NVIC_EnableIRQ(RTC_Seconds_IRQn);

    RTC_CR |= RTC_CR_OSCE(1);
    RTC_IER |= RTC_IER_TSIE(1);

    OSTaskCreate((OS_TCB     *)&ApptimeTaskTCB,
                (CPU_CHAR   *)"App Time Task",
                (OS_TASK_PTR ) timeTask,
                (void       *) 0,
                (OS_PRIO     ) APP_CFG_TIME_TASK_PRIO,
                (CPU_STK    *)&timeTaskStk[0],
                (CPU_STK     )(APP_CFG_TIME_TASK_STK_SIZE / 10u),
                (CPU_STK_SIZE) APP_CFG_TIME_TASK_STK_SIZE,
                (OS_MSG_QTY  ) 0,
                (OS_TICK     ) 0,
                (void       *) 0,
                (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                (OS_ERR     *)&os_err);
    while(os_err != OS_ERR_NONE){}


}
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
void RTC_Seconds_IRQHandler(void){
    OSIntEnter();
    OS_ERR os_err;
    NVIC_ClearPendingIRQ(RTC_Seconds_IRQn);
    (void)OSSemPost(&timeSecFlag,OS_OPT_POST_1,&os_err);
    while(os_err != OS_ERR_NONE){}
    OSIntExit();
}
/********************************************************************
* timeTask - Handles counting up every second and posting time change flag
*
* Description:  This private task will update the official running clock every
*               second granted it gets told when every second is and has access
*               to the time structure via it's mutex. Whenever the time is
*               changed via this task a semaphore is posted as to tell the
*               display to update.
*
* Return value: None
*
* Arguments:    None
*
* Anthony Needles - 01/22/18
********************************************************************/
static void timeTask(void *p_arg){
    OS_ERR os_err;

    (void)p_arg;

    while(1){
        DB3_TURN_OFF();
        (void)OSSemPend(&timeSecFlag,0,OS_OPT_PEND_BLOCKING,(CPU_TS *)0,&os_err);
        while(os_err != OS_ERR_NONE){}

        OSMutexPend(&timeMutexKey, 0, OS_OPT_PEND_BLOCKING,(CPU_TS *)0, &os_err);
        while(os_err != OS_ERR_NONE){}

        DB3_TURN_ON();
        if(timeOfDay.sec > 58){
            timeOfDay.sec = 0;
            if(timeOfDay.min > 58){
                timeOfDay.min = 0;
                if(timeOfDay.hr > 22){
                    timeOfDay.hr = 0;
                }else{
                    timeOfDay.hr++;
                }
            }else{
                timeOfDay.min++;
            }
        }else{
            timeOfDay.sec++;
        }

        (void)OSSemPost(&timeChgFlag,OS_OPT_POST_1,&os_err);
        while(os_err != OS_ERR_NONE){}

        (void)OSMutexPost(&timeMutexKey, OS_OPT_POST_NONE, &os_err);
        while(os_err != OS_ERR_NONE){}
    }

}
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
void TimeSet(TIME_T *ltime){
    OS_ERR os_err;

    OSMutexPend(&timeMutexKey, 0, OS_OPT_PEND_BLOCKING,(CPU_TS *)0, &os_err);
    while(os_err != OS_ERR_NONE){}

    timeOfDay = *ltime;

    (void)OSMutexPost(&timeMutexKey, OS_OPT_POST_NONE, &os_err);
    while(os_err != OS_ERR_NONE){}
}
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
void TimeGet(TIME_T *ltime){
    OS_ERR os_err;

    OSMutexPend(&timeMutexKey, 0, OS_OPT_PEND_BLOCKING,(CPU_TS *)0, &os_err);
    while(os_err != OS_ERR_NONE){}

    *ltime = timeOfDay;

    (void)OSMutexPost(&timeMutexKey, OS_OPT_POST_NONE, &os_err);
    while(os_err != OS_ERR_NONE){}
}
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
void TimePend(TIME_T *ltime){
    OS_ERR os_err;
    (void)OSSemPend(&timeChgFlag,0,OS_OPT_PEND_BLOCKING,(CPU_TS *)0,&os_err);
    while(os_err != OS_ERR_NONE){}

    *ltime = timeOfDay;
}
