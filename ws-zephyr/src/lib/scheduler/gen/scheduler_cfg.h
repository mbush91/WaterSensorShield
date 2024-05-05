/**
 * @file
 * @brief GENERATED FILE. Defines settings for timers and tasks to be configured
 */
#ifndef SCHEDULER_CFG_H
#define SCHEDULER_CFG_H

#include "scheduler.h"

// TIMER CYCLE TIME DEFINITION //////////////////
#define SCHEDULER_CFG_TIMER_1SEC    1000  /**< Cycle time in ms of the 1sec timer. */
#define SCHEDULER_CFG_TIMER_10SEC    10000  /**< Cycle time in ms of the 10sec timer. */


// TASK STACK SIZE DEFINITION //////////////////
#define SCHEDULER_CFG_1SEC_STACK_SIZE        4096 /**< Stack size of XX task thread. */
#define SCHEDULER_CFG_10SEC_STACK_SIZE        4096 /**< Stack size of XX task thread. */

// TASK PRIORITY DEFINITION /////////////////////
#define SCHEDULER_CFG_TASK_1SEC_PRIORITY          1   /**< Priority of the 1sec task thread. */
#define SCHEDULER_CFG_TASK_10SEC_PRIORITY          2   /**< Priority of the 10sec task thread. */


// TASK THREAD ID DECLARATION ///////////////////
extern k_tid_t task_1sec_id;    /**< Thread ID of the 1sec task thread. */
extern k_tid_t task_10sec_id;    /**< Thread ID of the 10sec task thread. */


/**
 * @brief Initializes the scheduler tasks.
 */
void scheduler_cfg_init_tasks(void);

/**
 * @brief Initializes the scheduler timers.
 */
void scheduler_cfg_init_timers(void);

#endif // SCHEDULER_CFG_H
