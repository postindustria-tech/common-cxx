/* *********************************************************************
 * This Original Work is copyright of 51 Degrees Mobile Experts Limited.
 * Copyright 2019 51 Degrees Mobile Experts Limited, 5 Charlotte Close,
 * Caversham, Reading, Berkshire, United Kingdom RG4 7BY.
 *
 * This Original Work is licensed under the European Union Public Licence (EUPL) 
 * v.1.2 and is subject to its terms as set out below.
 *
 * If a copy of the EUPL was not distributed with this file, You can obtain
 * one at https://opensource.org/licenses/EUPL-1.2.
 *
 * The 'Compatible Licences' set out in the Appendix to the EUPL (as may be
 * amended by the European Commission) shall be deemed incompatible for
 * the purposes of the Work and the provisions of the compatibility
 * clause in Article 5 of the EUPL shall not apply.
 * 
 * If using the Work as, or as part of, a network application, by 
 * including the attribution notice(s) required under Article 5 of the EUPL
 * in the end user terms of the application under an appropriate heading, 
 * such notice(s) shall fulfill the requirements of that article.
 * ********************************************************************* */

/**
 * @ingroup FiftyOneDegreesCommon
 * @defgroup FiftyOneDegreesThreading Threading
 *
 * Defines multi threading macros if the FIFTYONE_DEGREES_NO_THREADING compiler
 * directive is not explicitly requesting single threaded operation.
 *
 * @{
 */

#ifndef FIFTYONE_DEGREES_THREADING_INCLUDED
#define FIFTYONE_DEGREES_THREADING_INCLUDED

#include <stdbool.h>

#ifdef __cplusplus
#define EXTERNAL extern "C"
#else
#define EXTERNAL
#endif

/**
 * Determines if the methods that should be thread safe have been compiled so
 * they are thread safe. In single threaded operation compiling without
 * threading using the `FIFTYONE_DEGREES_NO_THREADING` directive results in
 * performance improvements.
 * @return true if the library is thread safe, otherwise false.
 */
EXTERNAL bool fiftyoneDegreesThreadingGetIsThreadSafe();

/**
 * A thread method passed to the #FIFTYONE_DEGREES_THREAD_CREATE macro.
 */
#ifdef _MSC_VER
#define FIFTYONE_DEGREES_THREAD_ROUTINE LPTHREAD_START_ROUTINE 
#else
typedef void*(*FIFTYONE_DEGREES_THREAD_ROUTINE)(void*);
#endif

/* Define NDEBUG if needed, to ensure asserts are disabled in release builds */
#if !defined(DEBUG) && !defined(_DEBUG) && !defined(NDEBUG)
#define NDEBUG
#endif

#ifdef _MSC_VER
#include <windows.h>
#include <intrin.h>
#pragma intrinsic (_InterlockedIncrement)
#pragma intrinsic (_InterlockedDecrement)
#else
#include <pthread.h>
#include <signal.h>
#endif
#include <assert.h>

/**
 * MUTEX AND THREADING MACROS
 */

/**
 * Mutex used to synchronise access to data structures that could be used
 * in parallel in a multi threaded environment.
 */
#ifdef _MSC_VER
typedef HANDLE fiftyoneDegreesMutex;
#else
typedef pthread_mutex_t fiftyoneDegreesMutex;
/**
 * Initialises the mutex passed to the method.
 * @param mutex to be initialised.
 */
EXTERNAL void fiftyoneDegreesMutexCreate(fiftyoneDegreesMutex *mutex);
/**
 * Closes the mutex passed to the method.
 * @param mutex to be closed.
 */
EXTERNAL void fiftyoneDegreesMutexClose(fiftyoneDegreesMutex *mutex);
/**
 * Locks the mutex passed to the method.
 * @param mutex to be locked.
 */
EXTERNAL void fiftyoneDegreesMutexLock(fiftyoneDegreesMutex *mutex);
/**
 * Unlocks the mutex passed to the method.
 * @param mutex to be unlocked.
 */
EXTERNAL void fiftyoneDegreesMutexUnlock(fiftyoneDegreesMutex *mutex);
#endif

/**
 * A signal used to limit the number of items that can be created by
 * the pool.
 */
#ifdef _MSC_VER
typedef HANDLE fiftyoneDegreesSignal;
#else
typedef struct fiftyone_degrees_signal_t {
	volatile bool wait; /**< Flag indicating if the thread should wait */
	pthread_cond_t cond; /**< Condition variable for the signal */
	pthread_mutex_t mutex; /**< Mutex for the signal */
} fiftyoneDegreesSignal;
#endif

/**
 * Initialises the signal pointer by setting the condition first followed by
 * the mutex if the condition was set correctly. Destroyed is set to false to
 * indicate to the other methods that the signal is still valid. The memory
 * used by the signal should be part of another structure and will be released
 * when that structure is released. If there is a problem creating the mutex
 * the condition is also released.
 * @return new signal
 */
fiftyoneDegreesSignal* fiftyoneDegreesSignalCreate();

/**
 * Closes the signal ensuring there is a lock on the signal before destroying
 * the signal. This means that no other process can be waiting on the signal
 * before it is destroyed. The destroyed field of the signal structure is set
 * to true after the condition is destroyed. All methods that could
 * subsequently try and get a lock on the signal **MUST** check the destroyed
 * field before trying to get the lock.
 * @param signal to be closed.
 */
void fiftyoneDegreesSignalClose(fiftyoneDegreesSignal *signal);

/**
 * If the signal has not been destroyed then sends a signal to a waiting
 * thread that the signal has been set and one can continue. This possible
 * because the condition will auto reset only enabling a signal thread to
 * continue even if multi threads are waiting.
 * @param signal to be set.
 */
void fiftyoneDegreesSignalSet(fiftyoneDegreesSignal *signal);

/**
 * Wait for a signal to be set. Only waits for the signal if the signal has not
 * been destroyed. Locks the mutex before the signal is waited for. This
 * ensures only one thread can be waiting on the signal at any one time.
 * @param signal pointer to the signal used to wait on.
 */
void fiftyoneDegreesSignalWait(fiftyoneDegreesSignal *signal);

/**
 * A thread created with the #FIFTYONE_DEGREES_THREAD_CREATE macro.
 */
#ifdef _MSC_VER
#define FIFTYONE_DEGREES_THREAD HANDLE
#else
#define FIFTYONE_DEGREES_THREAD pthread_t
#endif

/**
 * Creates a new signal that can be used to wait for
 * other operations to complete before continuing.
 * @param s signal to create
 */
#define FIFTYONE_DEGREES_SIGNAL_CREATE(s) s = fiftyoneDegreesSignalCreate()

/**
 * Frees the handle provided to the macro.
 * @param s signal to close
 */
#define FIFTYONE_DEGREES_SIGNAL_CLOSE(s) fiftyoneDegreesSignalClose(s)

/**
 * Signals a thread waiting for the signal to proceed.
 * @param s signal to set
 */
#define FIFTYONE_DEGREES_SIGNAL_SET(s) fiftyoneDegreesSignalSet(s)

/**
 * Waits for the signal to become set by another thread.
 * @param s signal to wait on
 */
#define FIFTYONE_DEGREES_SIGNAL_WAIT(s) fiftyoneDegreesSignalWait(s)

/**
 * Creates a new mutex at the pointer provided.
 * @param m mutex to create
 */
#ifdef _MSC_VER
#define FIFTYONE_DEGREES_MUTEX_CREATE(m) m = CreateMutex(NULL,FALSE,NULL)
#else
#define FIFTYONE_DEGREES_MUTEX_CREATE(m) fiftyoneDegreesMutexCreate(&m)
#endif

/**
 * Frees the mutex at the pointer provided.
 * @param m mutex to close
 */
#ifdef _MSC_VER
#define FIFTYONE_DEGREES_MUTEX_CLOSE(m) if (m != NULL) { CloseHandle(m); }
#else
#define FIFTYONE_DEGREES_MUTEX_CLOSE(m) fiftyoneDegreesMutexClose(&m)
#endif

/**
 * Locks the mutex at the pointer provided.
 * @param m mutex to lock
 */
#ifdef _MSC_VER
#define FIFTYONE_DEGREES_MUTEX_LOCK(m) WaitForSingleObject(*m, INFINITE)
#else
#define FIFTYONE_DEGREES_MUTEX_LOCK(m) fiftyoneDegreesMutexLock(m)
#endif

/**
 * Unlocks the mutex at the pointer provided.
 * @param m mutex to unlock
 */
#ifdef _MSC_VER
#define FIFTYONE_DEGREES_MUTEX_UNLOCK(m) ReleaseMutex(*m)
#else
#define FIFTYONE_DEGREES_MUTEX_UNLOCK(m) fiftyoneDegreesMutexUnlock(m)
#endif

/**
 * Returns true if the mutex is valid.
 * @param m mutex to check
 * @return true if valid
 */
#ifdef _MSC_VER
#define FIFTYONE_DEGREES_MUTEX_VALID(m) (*m != NULL)
#else
#define FIFTYONE_DEGREES_MUTEX_VALID(m) fiftyoneDegreesMutexValid(m)
#endif

/**
 * Creates a new thread with the following parameters:
 * @param t pointer to #FIFTYONE_DEGREES_THREAD memory
 * @param m the method to call when the thread runs
 * @param s pointer to the state data to pass to the method
 * @return new thread
 */
#ifdef _MSC_VER
#define FIFTYONE_DEGREES_THREAD_CREATE(t, m, s) t = \
	(FIFTYONE_DEGREES_THREAD)CreateThread(NULL, 0, m, s, 0, NULL)
#else
#define FIFTYONE_DEGREES_THREAD_CREATE(t, m, s) pthread_create(&t, NULL, m, s)
#endif

/**
 * Joins the thread provided to the current thread waiting
 * indefinitely for the operation to complete.
 * @param t pointer to a previously created thread
 */
#ifdef _MSC_VER
#define FIFTYONE_DEGREES_THREAD_JOIN(t) WaitForSingleObject(t, INFINITE)
#else
#define FIFTYONE_DEGREES_THREAD_JOIN(t) pthread_join(t, NULL)
#endif

/**
 * Closes the thread passed to the macro.
 * @param t thread to close
 */
#ifdef _MSC_VER
#define FIFTYONE_DEGREES_THREAD_CLOSE(t) CloseHandle(t)
#else
#define FIFTYONE_DEGREES_THREAD_CLOSE(t)
#endif

/**
 * Exits the calling thread.
 */
#ifdef _MSC_VER
#define FIFTYONE_DEGREES_THREAD_EXIT ExitThread(0)
#else
#define FIFTYONE_DEGREES_THREAD_EXIT pthread_exit(NULL)
#endif

/**
 * Increments the value and returns the final value.
 * @param v the value to decrement
 * @return value after incrementing
 */
#ifdef _MSC_VER
#define FIFTYONE_DEGREES_INTERLOCK_INC(v) _InterlockedIncrement(v)
#else
#define FIFTYONE_DEGREES_INTERLOCK_INC(v) (__sync_add_and_fetch(v, 1))
#endif

/**
 * Decrements the value and returns the final value.
 * @param v the value to decrement
 * @return value after decrementing
 */
#ifdef _MSC_VER
#define FIFTYONE_DEGREES_INTERLOCK_DEC(v) _InterlockedDecrement(v)
#else
#define FIFTYONE_DEGREES_INTERLOCK_DEC(v) (__sync_add_and_fetch(v, -1))
#endif

/**
 * Replaces the destination value with the exchange value, only if the
 * destination value matched the comparand. Returns the value of d before
 * the swap.
 * @param d the destination to swap
 * @param e the exchange value
 * @param c the comparand
 */
#ifdef _MSC_VER
#define FIFTYONE_DEGREES_INTERLOCK_EXCHANGE(d,e,c) \
	InterlockedCompareExchange(&d, e, c)
#else
#define FIFTYONE_DEGREES_INTERLOCK_EXCHANGE(d,e,c) \
	__sync_val_compare_and_swap(&d,c,e)
#endif

/**
 * 64 bit compare and swap. Replaces the destination value with the exchange
 * value, only if the destination value matched the comparand. Returns the
 * value of d before the swap.
 * @param d the destination to swap
 * @param e the exchange value
 * @param c the comparand
 */
#ifdef _MSC_VER
#define FIFTYONE_DEGREES_INTERLOCK_EXCHANGE_64(d,e,c) \
	InterlockedCompareExchange64((volatile __int64*)&d, (__int64)e, (__int64)c)
#else
#define FIFTYONE_DEGREES_INTERLOCK_EXCHANGE_64(d,e,c) \
    FIFTYONE_DEGREES_INTERLOCK_EXCHANGE(d,e,c)
#endif

/**
 * Replaces the destination pointer with the exchange pointer, only if the
 * destination pointer matched the comparand. Returns the value of d before
 * the swap.
 * @param d the destination to swap
 * @param e the exchange value
 * @param c the comparand
 */
#ifdef _MSC_VER
#ifdef _WIN64
#define FIFTYONE_DEGREES_INTERLOCK_EXCHANGE_PTR(d,e,c) \
    FIFTYONE_DEGREES_INTERLOCK_EXCHANGE_64(d,e,c)
#else
#define FIFTYONE_DEGREES_INTERLOCK_EXCHANGE_PTR(d,e,c) \
    FIFTYONE_DEGREES_INTERLOCK_EXCHANGE(d,e,c)
#endif
#else
#define FIFTYONE_DEGREES_INTERLOCK_EXCHANGE_PTR(d,e,c) \
    FIFTYONE_DEGREES_INTERLOCK_EXCHANGE(d,e,c)
#endif

#ifndef _MSC_VER
 /**
  * Implements the __sync_bool_compare_and_swap_16 function which is often not
  * implemtned by the compiler. This uses the cmpxchg16b instruction from the
  * x86-64 instruction set, the same instruction as the
  * InterlockedCompareExchange128 implementation
  * (see https://docs.microsoft.com/en-us/cpp/intrinsics/interlockedcompareexchange128?view=vs-2019#remarks).
  * It is therefore supported by modern Intel and AMD CPUs. However, most ARM
  * chips will not support this.
  * For full details of the cmpxchg16b instruction, see the manual:
  * https://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-instruction-set-reference-manual-325383.pdf
  * and other example implementations:
  * https://github.com/ivmai/libatomic_ops/blob/release-7_2/src/atomic_ops/sysdeps/gcc/x86_64.h#L148
  * https://github.com/haproxy/haproxy/blob/a7bf57352059277239794950f9aac33d05741f1a/include/common/hathreads.h#L1000
  * @param destination memory location to be replaced if the compare is true
  * @param exchange memory to copy to the destination if the compare is true
  * @param compare memory to compare to destination.
  * @return 1 if all 16 bytes of destination and compare were equal and
  * destination was replaced, otherwise 0
  */
static __inline int
__fod_sync_bool_compare_and_swap_16(
    void* destination,
    const void* exchange,
    void* compare)
{
    char result;
    __asm __volatile("lock cmpxchg16b %0; setz %3"
    : "+m" (*(void**)destination),
        "=a" (((void**)compare)[0]),
        "=d" (((void**)compare)[1]),
        "=q" (result)
        : "a" (((void**)compare)[0]),
        "d" (((void**)compare)[1]),
        "b" (((const void**)exchange)[0]),
        "c" (((const void**)exchange)[1])
        : "memory", "cc");
    return (result);
}
#endif

/**
 * Double width (64 or 128 depending on the architecture) compare and exchange.
 * Replaces the destination value with the exchange value, only if the
 * destination value matched the comparand. Returns true if the value was
 * exchanged.
 * @param d the destination to swap
 * @param e the exchange value
 * @param c the comparand
 */
#ifdef _MSC_VER
#ifdef _WIN64
#define FIFTYONE_DEGREES_INTERLOCK_EXCHANGE_PTR_DW(d,e,c) \
    InterlockedCompareExchange128((__int64*)d, *(((__int64*)&e) + 1), *(((__int64*)&e)), (__int64*)&c)
#else
#define FIFTYONE_DEGREES_INTERLOCK_EXCHANGE_PTR_DW(d,e,c) \
    (InterlockedCompareExchange64((__int64*)d, *((__int64*)&e), *(__int64*)&c) != *((__int64*)d))
#endif
#else
#define FIFTYONE_DEGREES_INTERLOCK_EXCHANGE_PTR_DW(d,e,c) \
    (sizeof(void*) == 8 ? \
    __fod_sync_bool_compare_and_swap_16((void*)d, (void*)&e, (void*)&c) : \
    __sync_bool_compare_and_swap((long*)d, *((long*)&c), *((long*)&e)))
#endif

/**
 * @}
 */

#endif