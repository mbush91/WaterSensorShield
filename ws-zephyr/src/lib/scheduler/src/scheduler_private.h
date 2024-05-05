#ifndef SCHEDULER_PRIVATE_H
#define SCHEDULER_PRIVATE_H

// if you edit these, be sure to update .clang-format MacroBlock things

/** @brief Defines beginning of section where runnables are called. */
#define SCHEDULER_BEGIN_RUNNABLES while (1) {
/** @brief Defines end of section where runnables are called. */
#define SCHEDULER_END_RUNNABLES \
    k_sleep(K_FOREVER);         \
    }
/** @brief Defines end of section where loops are called. */
#define SCHEDULER_END_LOOP }

#endif // SCHEDULER_PRIVATE_H
