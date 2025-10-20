/* Thin wrapper to reuse existing logging */
#ifndef PM_LOG_WRAP_H_
#define PM_LOG_WRAP_H_

/* Default to no-op logging to avoid pulling in stdio on constrained builds. */
#ifndef PM_LOG_INFO
#define PM_LOG_INFO(...)
#endif
#ifndef PM_LOG_WARN
#define PM_LOG_WARN(...)
#endif
#ifndef PM_LOG_ERROR
#define PM_LOG_ERROR(...)
#endif

/* If user wants to route to original PM_Log.h, they can include it before this header
 * or define PM_LOG_INFO/PM_LOG_WARN/PM_LOG_ERROR macros externally. */

#endif /* PM_LOG_WRAP_H_ */
