/**
 * @file
 * @brief Scheduler component defines and executes tasks which in turn executes runnables
 *
 * Manages execution of threads in the system.
 *
 * The Scheduler defines OSTask(s) which are executed periodically.
 * An OSTask in turn calls the Runnable(s) which are mapped to them.
 */
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <zephyr/kernel.h>


/**
 * @brief Initializes the scheduler tasks and timers.
 */
void scheduler_init(void);

#endif // SCHEDULER_H
