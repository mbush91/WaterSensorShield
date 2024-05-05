#include "scheduler.h"
#include "scheduler_cfg.h"
#include <zephyr/kernel.h>


void scheduler_init(void)
{
    scheduler_cfg_init_tasks();
    scheduler_cfg_init_timers();
}
