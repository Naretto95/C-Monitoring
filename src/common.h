/*!
\file common.h
\brief Shared configuration constants and small utilities used across
       the monitor, the calculators and the launcher.
*/

#ifndef COMMON_H
#define COMMON_H

/* Network configuration */
#define MONITOR_PORT 4206
#define MONITOR_HOST "127.0.0.1"

/* The simulated workload is the sum of integers in [0, TASK_RANGE_MAX]. */
#define TASK_RANGE_MAX 1000

/* Sanity upper bound reused both for the number of task cells the monitor
   will create and for the number of calculator threads the launcher can
   track; there is no deeper relationship between the two, it is simply a
   "reasonable size for this simulation" limit. */
#define MAX_CALCULATORS 200

/* Sentinel meaning "no free task slot found yet". */
#define NO_VACANCY (-1)

/* How many consecutive missed progress reports (about 1s apart) a task
   tolerates before its calculator is considered dead and the task is
   put back up for reassignment. */
#define TASK_MAX_MISSED_REPORTS 6

/* Default interval, in seconds, between evil monkey attacks. */
#define DEFAULT_KILL_FREQUENCY 20

/* Prints errno-based diagnostics for errorMessage and terminates the
   process; used for failures we have no meaningful way to recover from. */
void die(const char *error_message);

#endif /* COMMON_H */
