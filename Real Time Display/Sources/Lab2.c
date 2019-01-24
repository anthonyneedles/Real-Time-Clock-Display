/*****************************************************************************************
* This program utilized uCOS to create a time display that is based of the RTC.
* Semaphores and Mutexs are used to handle task pending/posting and data management.
* Upon restart the clock starts counting up from 1200 in 24-Hour time on row 1.
* Can input time on second row to set main time display, using on board keypad.
*
* 01/18/2018, Anthony Needles
*****************************************************************************************/
#include "MCUType.h"
#include "app_cfg.h"
#include "os.h"
#include "K65TWR_GPIO.h"
#include "LcdLayered.h"
#include "uCOSKey.h"
#include "Time.h"

#define ROW1 1
#define ROW2 2
#define COLUMN9 9
#define COLUMN10 10
#define COLUMN12 12
#define COLUMN13 13
#define COLUMN15 15
#define COLUMN16 16
#define A_PRESS 0x11u
#define C_PRESS 0x13u
#define CURSORON 1
#define BLINKON 1
#define CURSOROFF 0
#define BLINKOFF 0

typedef enum{TIME, TIMESET} UISTATE;
typedef enum{HOURTENS, HOURONES, MINUTETENS, MINUTEONES, SECONDTENS, SECONDONES} SETSTATE;

static OS_TCB AppTaskStartTCB;
static OS_TCB UITaskTCB;
static OS_TCB TimeDispTaskTCB;

static CPU_STK AppTaskStartStk[APP_CFG_TASK_START_STK_SIZE];
static CPU_STK UITaskStk[APP_CFG_UITASK_STK_SIZE];
static CPU_STK TimeDispTaskStk[APP_CFG_TIMEDISPTASK_STK_SIZE];

static void  AppStartTask(void *p_arg);
static void  UITask(void *p_arg);
static void  TimeDispTask(void *p_arg);

void main(void) {
    OS_ERR  os_err;

    CPU_IntDis();

    OSInit(&os_err);                    /* Initialize uC/OS-III                         */
    while(os_err != OS_ERR_NONE){}

    OSTaskCreate(&AppTaskStartTCB,
                 "Start Task",
                 AppStartTask,
                 (void *) 0,
                 APP_CFG_TASK_START_PRIO,
                 &AppTaskStartStk[0],
                 (APP_CFG_TASK_START_STK_SIZE/10u),
                 APP_CFG_TASK_START_STK_SIZE,
                 0,
                 0,
                 (void *) 0,
                 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 &os_err);
    while(os_err != OS_ERR_NONE){}

    OSStart(&os_err);               /*Start multitasking(i.e. give control to uC/OS)    */
    while(os_err != OS_ERR_NONE){}
}
/********************************************************************
* AppStartTask - uCos startup task
*
* Description:  This task should run once then be suspended. Could restart
*               everything if resumed. Runs initialization functions and creates
*               UITask and TimeDispTask.
*
* Return value: None
*
* Arguments:    None
********************************************************************/
static void AppStartTask(void *p_arg) {

    OS_ERR os_err;

    (void)p_arg;                        /* Avoid compiler warning for unused variable   */

    OS_CPU_SysTickInitFreq(DEFAULT_SYSTEM_CLOCK);
    LcdInit();
    GpioDBugBitsInit();
    KeyInit();
    TimeInit();

    OSTaskCreate(&UITaskTCB,                  /* Create UITask  */
                "UITask ",
                UITask,
                (void *) 0,
                APP_CFG_UITASK_PRIO,
                &UITaskStk[0],
                (APP_CFG_UITASK_STK_SIZE / 10u),
                APP_CFG_UITASK_STK_SIZE,
                0,
                0,
                (void *) 0,
                (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                &os_err);
    while(os_err != OS_ERR_NONE){}

    OSTaskCreate(&TimeDispTaskTCB,    /* Create TimeDispTask */
                "TimeDispTask ",
                TimeDispTask,
                (void *) 0,
                APP_CFG_TIMEDISPTASK_PRIO,
                &TimeDispTaskStk[0],
                (APP_CFG_TIMEDISPTASK_STK_SIZE / 10u),
                APP_CFG_TIMEDISPTASK_STK_SIZE,
                0,
                0,
                (void *) 0,
                (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                &os_err);
    while(os_err != OS_ERR_NONE){}

    OSTaskSuspend((OS_TCB *)0, &os_err);
    while(os_err != OS_ERR_NONE){}
}
/********************************************************************
* UITask - Time/Time Set state handler
*
* Description:  This task handles keypad input, and will alter the time
*               of day if in TIMESET mode as per the lab hand out. Runs whenever
*               new key is pressed. '#' if in Time mode will trigger Time Set
*               mode, and C or A will exit Time Set and enter Time
*               (either saving current edited time or discarding). First time
*               run through will skip key pending to start in "out of reset"
*               mode (see init_check).
*
* Return value: None
*
* Arguments:    None
*
* Anthony Needles - 01/20/18
********************************************************************/
static void UITask(void *p_arg){

    OS_ERR os_err;
    (void)p_arg;
    UISTATE ui_state = TIMESET;
    SETSTATE set_state = HOURTENS;
    INT8U user_input;
    TIME_T buffertime;
    INT8U init_check = 1;
    
    while(1){
    
        if(init_check){
            init_check = 0;
            TimeGet(&buffertime);
            LcdDispTime(ROW2, COLUMN9, TIMESETLAYER, buffertime.hr, buffertime.min, buffertime.sec);
            LcdCursor(ROW2, COLUMN9, TIMESETLAYER, CURSORON, BLINKON);
        } else{
            DB0_TURN_OFF();
            user_input = KeyPend(0,&os_err);
            while(os_err != OS_ERR_NONE){}
            DB0_TURN_ON();
        }

        switch(ui_state){
            case(TIMESET):
                switch(user_input){
                    case(A_PRESS):
                        ui_state = TIME;
                        TimeSet(&buffertime);
                        LcdHideLayer(TIMESETLAYER);
                        break;
                    case(C_PRESS):
                        ui_state = TIME;
                        LcdHideLayer(TIMESETLAYER);
                        break;
                    default:
                        break;
                }
                switch(set_state){
                    case(HOURTENS):
                        switch(user_input){
                            case('0'):
                            case('1'):
                            case('2'):
                                user_input = ((user_input - 0x30)*10);
                                buffertime.hr = (buffertime.hr % 10) + user_input;
                                set_state = HOURONES;
                                LcdCursor(ROW2, COLUMN10, TIMESETLAYER, CURSORON, BLINKON);
                                break;
                            default:
                                break;
                        }
                    break;
                    case(HOURONES):
                        switch(user_input){
                            case('0'):  //Uses "fall through" property of switch
                            case('1'):  //statements for threshold checking
                            case('2'):
                            case('3'):
                                user_input = (user_input - 0x30);
                                buffertime.hr = (buffertime.hr - (buffertime.hr % 10)) + user_input;
                                set_state = MINUTETENS;
                                LcdCursor(ROW2, COLUMN12, TIMESETLAYER, CURSORON, BLINKON);
                                break;
                            case('4'):
                            case('5'):
                            case('6'):
                            case('7'):
                            case('8'):
                            case('9'):
                                if((buffertime.hr/10) != 2){
                                    user_input = (user_input - 0x30);
                                    buffertime.hr = (buffertime.hr - (buffertime.hr % 10)) + user_input;
                                    set_state = MINUTETENS;
                                    LcdCursor(ROW2, COLUMN12, TIMESETLAYER, CURSORON, BLINKON);
                                } else{
                                }
                                break;
                            default:
                                break;
                        }
                    break;
                    case(MINUTETENS):
                        switch(user_input){
                            case('0'):
                            case('1'):
                            case('2'):
                            case('3'):
                            case('4'):
                            case('5'):
                                user_input = ((user_input - 0x30)*10);
                                buffertime.min = (buffertime.min % 10) + user_input;
                                set_state = MINUTEONES;
                                LcdCursor(ROW2, COLUMN13, TIMESETLAYER, CURSORON, BLINKON);
                                break;
                            default:
                                break;
                        }
                    break;
                    case(MINUTEONES):
                        switch(user_input){
                            case('0'):
                            case('1'):
                            case('2'):
                            case('3'):
                            case('4'):
                            case('5'):
                            case('6'):
                            case('7'):
                            case('8'):
                            case('9'):
                                user_input = (user_input - 0x30);
                                buffertime.min = (buffertime.min - (buffertime.min % 10)) + user_input;
                                set_state = SECONDTENS;
                                LcdCursor(ROW2, COLUMN15, TIMESETLAYER, CURSORON, BLINKON);
                                break;
                            default:
                                break;
                        }
                    break;
                    case(SECONDTENS):
                        switch(user_input){
                            case('0'):
                            case('1'):
                            case('2'):
                            case('3'):
                            case('4'):
                            case('5'):
                                user_input = ((user_input - 0x30)*10);
                                buffertime.sec = (buffertime.sec % 10) + user_input;
                                set_state = SECONDONES;
                                LcdCursor(ROW2, COLUMN16, TIMESETLAYER, CURSORON, BLINKON);
                                break;
                            default:
                                break;
                        }
                    break;
                    case(SECONDONES):
                        switch(user_input){
                            case('0'):
                            case('1'):
                            case('2'):
                            case('3'):
                            case('4'):
                            case('5'):
                            case('6'):
                            case('7'):
                            case('8'):
                            case('9'):
                                user_input = (user_input - 0x30);
                                buffertime.sec = (buffertime.sec - (buffertime.sec % 10)) + user_input;
                                break;
                            default:
                                break;
                        }
                    break;
                }
            LcdDispTime(ROW2, COLUMN9, TIMESETLAYER, buffertime.hr, buffertime.min, buffertime.sec);
            break;
            case(TIME):
                switch(user_input){
                    case('#'):
                        TimeGet(&buffertime);
                        LcdShowLayer(TIMESETLAYER);
                        LcdDispTime(ROW2, COLUMN9, TIMESETLAYER, buffertime.hr, buffertime.min, buffertime.sec);
                        ui_state = TIMESET;
                        set_state = HOURTENS;
                        LcdCursor(ROW2, COLUMN9, TIMESETLAYER, CURSORON, BLINKON);
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }
}
/********************************************************************
* TimeDispTask - Displays time on ROW1
*
* Description:  This task will grab the current timeOfDay in Time.c every time
*               the time changes and displays it on the LCD.
*
* Return value: None
*
* Arguments:    None
*
* Anthony Needles - 01/20/18
********************************************************************/
static void TimeDispTask(void *p_arg){

//    OS_ERR os_err;

    (void)p_arg;

    TIME_T ltime;

    while(1) {                                  /* wait for Task 1 to signal semaphore  */

        DB2_TURN_OFF();                         /* Turn off debug bit while waiting     */
        TimePend(&ltime);
        DB2_TURN_ON();
        LcdDispTime(ROW1, COLUMN9, TIMEDISPLAYER, ltime.hr, ltime.min, ltime.sec);
    }
}
