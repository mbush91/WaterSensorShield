/**
 * GENERATED FILE. Defines thread timers.
 */

// PCLint suppression. PFG-186 ticket submitted
//lint -e715 -e818 -e765 -e783

#include "scheduler.h"
#include "scheduler_cfg.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pss_scheduler_timer, CONFIG_PSS_SCHEDULER_LOG_LEVEL);

/**
 * @brief Checks if a thread has completed running and is ready to be scheduled.
 *
 * @param thread Thread id to be checked.
 * @return       true is thread is ready to be scheduled, otherwise false.
 */
static inline bool scheduler_is_thread_finished(k_tid_t thread)
{
    uint8_t state = thread->base.thread_state;

    return (state
            & (_THREAD_PENDING | _THREAD_PRESTART | _THREAD_DEAD | _THREAD_DUMMY
               | _THREAD_SUSPENDED))
           != 0U;
}

/**
 * @brief Schedules task to be executed. If still running prints an overrun message.
 *
 * @param thread Thread id to be checked.
 * @param caller Name of function calling the wakeup.
 * @return       true is thread is ready to be scheduled, otherwise false.
 */
static inline void scheduler_thread_wakeup(k_tid_t thread, const char* caller)
{
    if (scheduler_is_thread_finished(thread)) {
        k_wakeup(thread);
    } else {
#ifndef CONFIG_PSS_SCHEDULER_DEBUGGER_EN
        LOG_WRN("%s Overrun!", caller);
#endif
    }
}

// DEFINE TIMERS TO SCHEDULE TASKS //////////////
void scheduler_timer_1sec_task(struct k_timer* dummy)
{
    static bool scheduler_timer_1sec_first = true;
    if (true == scheduler_timer_1sec_first) {
        scheduler_timer_1sec_first = false;
        k_thread_start(task_1sec_id);
    } else {
        scheduler_thread_wakeup(task_1sec_id, __func__);
    }
}
K_TIMER_DEFINE(scheduler_timer_1sec, scheduler_timer_1sec_task, NULL);

void scheduler_timer_10sec_task(struct k_timer* dummy)
{
    static bool scheduler_timer_10sec_first = true;
    if (true == scheduler_timer_10sec_first) {
        scheduler_timer_10sec_first = false;
        k_thread_start(task_10sec_id);
    } else {
        scheduler_thread_wakeup(task_10sec_id, __func__);
    }
}
K_TIMER_DEFINE(scheduler_timer_10sec, scheduler_timer_10sec_task, NULL);

/////////////////////////////////////////////////

void scheduler_cfg_init_timers(void)
{
    k_timer_start(&scheduler_timer_1sec,
                  K_MSEC(SCHEDULER_CFG_TIMER_1SEC),
                  K_MSEC(SCHEDULER_CFG_TIMER_1SEC));
    k_timer_start(&scheduler_timer_10sec,
                  K_MSEC(SCHEDULER_CFG_TIMER_10SEC),
                  K_MSEC(SCHEDULER_CFG_TIMER_10SEC));
}
