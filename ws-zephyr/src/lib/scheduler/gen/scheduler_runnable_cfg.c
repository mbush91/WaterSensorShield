/**
 * @file
 * @brief GENERATED FILE. Maps runnables to tasks.
 */

// PCLint suppression. PFG-186 ticket submitted
//lint -e534 -e716 -e715

#include "scheduler.h"
#include "scheduler_cfg.h"
#include "scheduler_private.h"

#include "pss_mqtt.h"
#include "trigger.h"
#include "battery.h"
#include "main.h"

void task_1sec_runnables(void* a, void* b, void* c)
{
    SCHEDULER_BEGIN_RUNNABLES

        pss_mqtt_main();

        trigger_main();

        battery_main();

        main_main_blink();

    SCHEDULER_END_RUNNABLES
}


void task_10sec_runnables(void* a, void* b, void* c)
{
    SCHEDULER_BEGIN_RUNNABLES

        main_main_hearbeat();

    SCHEDULER_END_RUNNABLES
}


