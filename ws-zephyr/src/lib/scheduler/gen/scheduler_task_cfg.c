/**
 * GENERATED FILE. Defines task threads and their functions.
 */

// PCLint suppression. PFG-186 ticket submitted
//lint -e2701 -e715 -e765 -e552 -e783

#include "scheduler.h"
#include "scheduler_cfg.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pss_scheduler_task, CONFIG_PSS_SCHEDULER_LOG_LEVEL);

// Define Threads witch execute runnables
k_tid_t task_1sec_id;
struct k_thread task_1sec_thread;
K_THREAD_STACK_DEFINE(task_1sec_stack, SCHEDULER_CFG_1SEC_STACK_SIZE);
extern void task_1sec_runnables(void* a, void* b, void* c);

k_tid_t task_10sec_id;
struct k_thread task_10sec_thread;
K_THREAD_STACK_DEFINE(task_10sec_stack, SCHEDULER_CFG_10SEC_STACK_SIZE);
extern void task_10sec_runnables(void* a, void* b, void* c);



void scheduler_cfg_init_tasks(void)
{
    task_1sec_id = k_thread_create(&task_1sec_thread,
                                    task_1sec_stack,
                                    K_THREAD_STACK_SIZEOF(task_1sec_stack),
                                    task_1sec_runnables,
                                    NULL,
                                    NULL,
                                    NULL,
                                    SCHEDULER_CFG_TASK_1SEC_PRIORITY,
                                    0,
                                    K_FOREVER);
    if(k_thread_name_set(task_1sec_id,"1sec_thread") != 0) {
       LOG_WRN("Could not create thread for 1sec");
    }

    task_10sec_id = k_thread_create(&task_10sec_thread,
                                    task_10sec_stack,
                                    K_THREAD_STACK_SIZEOF(task_10sec_stack),
                                    task_10sec_runnables,
                                    NULL,
                                    NULL,
                                    NULL,
                                    SCHEDULER_CFG_TASK_10SEC_PRIORITY,
                                    0,
                                    K_FOREVER);
    if(k_thread_name_set(task_10sec_id,"10sec_thread") != 0) {
       LOG_WRN("Could not create thread for 10sec");
    }

}
