/*
 * Copyright (c) 2000-2007 Niels Provos <provos@citi.umich.edu>
 * Copyright (c) 2007-2012 Niels Provos and Nick Mathewson
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef EVENT_INTERNAL_H_INCLUDED_
#define EVENT_INTERNAL_H_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif

#include "event2/event-config.h"
#include "event2/watch.h"
#include "evconfig-private.h"

#include <time.h>
#include <sys/queue.h>
#include "event2/event_struct.h"
#include "minheap-internal.h"
#include "evsignal-internal.h"
#include "mm-internal.h"
#include "defer-internal.h"

/* map union members back */

/* mutually exclusive */
#define ev_signal_next	ev_.ev_signal.ev_signal_next
#define ev_io_next	ev_.ev_io.ev_io_next
#define ev_io_timeout	ev_.ev_io.ev_timeout

/* used only by signals */
#define ev_ncalls	ev_.ev_signal.ev_ncalls
#define ev_pncalls	ev_.ev_signal.ev_pncalls

#define ev_pri ev_evcallback.evcb_pri
#define ev_flags ev_evcallback.evcb_flags
#define ev_closure ev_evcallback.evcb_closure
#define ev_callback ev_evcallback.evcb_cb_union.evcb_callback
#define ev_arg ev_evcallback.evcb_arg

/** @name Event closure codes

    Possible values for evcb_closure in struct event_callback

    @{
 */
/** A regular event. Uses the evcb_callback callback */
#define EV_CLOSURE_EVENT 0
/** A signal event. Uses the evcb_callback callback */
#define EV_CLOSURE_EVENT_SIGNAL 1
/** A persistent non-signal event. Uses the evcb_callback callback */
#define EV_CLOSURE_EVENT_PERSIST 2
/** A simple callback. Uses the evcb_selfcb callback. */
#define EV_CLOSURE_CB_SELF 3
/** A finalizing callback. Uses the evcb_cbfinalize callback. */
#define EV_CLOSURE_CB_FINALIZE 4
/** A finalizing event. Uses the evcb_evfinalize callback. */
#define EV_CLOSURE_EVENT_FINALIZE 5
/** A finalizing event that should get freed after. Uses the evcb_evfinalize
 * callback. */
#define EV_CLOSURE_EVENT_FINALIZE_FREE 6
/** @} */

// 为event_structure提供统一后端接口
struct eventop {
	// 后端技术名称
	const char *name;
	// 初始化函数
	void *(*init)(struct event_base *);
	// 注册事件
	int (*add)(struct event_base *, evutil_socket_t fd, short old, short events, void *fdinfo);
	// 删除事件
	int (*del)(struct event_base *, evutil_socket_t fd, short old, short events, void *fdinfo);
	// 等待事件
	int (*dispatch)(struct event_base *, struct timeval *);
	// 释放资源
	void (*dealloc)(struct event_base *);
	// fork是否重新初始化event_base
	int need_reinit;
	// 一些I/O复用支持的一些特性
	enum event_method_feature features;
	// 有的I/O复用机制需要为每个I/O事件队列和信号事件队列分配额外的内存，
	// 以避免同一个文件描述符被重复插入I/O复用机制的事件表中.
	// evmap_io_add(或evmap_ io_ _del) 函数在调用eventop的add(或del)方法时，
	// 将这段内存的起始地址作为第5个参数传递给add (或del)方法。
	// 下面这个成员则指定了这段内存的长度
	size_t fdinfo_len;
};

#ifdef _WIN32
/* If we're on win32, then file descriptors are not nice low densely packed
   integers.  Instead, they are pointer-like windows handles, and we want to
   use a hashtable instead of an array to map fds to events.
*/
#define EVMAP_USE_HT
#endif

/* #define HT_CACHE_HASH_VALS */

#ifdef EVMAP_USE_HT
#define HT_NO_CACHE_HASH_VALUES
#include "ht-internal.h"
struct event_map_entry;
HT_HEAD(event_io_map, event_map_entry);
#else
#define event_io_map event_signal_map
#endif

/* Used to map signal numbers to a list of events.  If EVMAP_USE_HT is not
   defined, this structure is also used as event_io_map, which maps fds to a
   list of events.
*/
struct event_signal_map {
	/* An array of evmap_io * or of evmap_signal *; empty entries are
	 * set to NULL. */
	void **entries;
	/* The number of entries available in entries */
	int nentries;
};

/* A list of events waiting on a given 'common' timeout value.  Ordinarily,
 * events waiting for a timeout wait on a minheap.  Sometimes, however, a
 * queue can be faster.
 **/
struct common_timeout_list {
	/* List of events currently waiting in the queue. */
	struct event_list events;
	/* 'magic' timeval used to indicate the duration of events in this
	 * queue. */
	struct timeval duration;
	/* Event that triggers whenever one of the events in the queue is
	 * ready to activate */
	struct event timeout_event;
	/* The event_base that this timeout list is part of */
	struct event_base *base;
};

/** Mask used to get the real tv_usec value from a common timeout. */
#define COMMON_TIMEOUT_MICROSECONDS_MASK       0x000fffff

struct event_change;

/* List of 'changes' since the last call to eventop.dispatch.  Only maintained
 * if the backend is using changesets. */
struct event_changelist {
	struct event_change *changes;
	int n_changes;
	int changes_size;
};

#ifndef EVENT__DISABLE_DEBUG_MODE
/* Global internal flag: set to one if debug mode is on. */
extern int event_debug_mode_on_;
#define EVENT_DEBUG_MODE_IS_ON() (event_debug_mode_on_)
#else
#define EVENT_DEBUG_MODE_IS_ON() (0)
#endif

TAILQ_HEAD(evcallback_list, event_callback);

/* Sets up an event for processing once */
struct event_once {
	LIST_ENTRY(event_once) next_once;
	struct event ev;

	void (*cb)(evutil_socket_t, short, void *);
	void *arg;
};

/** Contextual information passed from event_base_loop to the "prepare" watcher
 * callbacks. We define this as a struct rather than individual parameters to
 * the callback function for the sake of future extensibility. */
struct evwatch_prepare_cb_info {
	/** The timeout duration passed to the underlying implementation's `dispatch`.
	 * See evwatch_prepare_get_timeout. */
	const struct timeval *timeout;
};

/** Contextual information passed from event_base_loop to the "check" watcher
 * callbacks. We define this as a struct rather than individual parameters to
 * the callback function for the sake of future extensibility. */
struct evwatch_check_cb_info {
	/** Placeholder, since empty struct is not allowed by some compilers. */
	void *unused;
};

/** Watcher types (prepare and check, perhaps others in the future). */
#define EVWATCH_PREPARE 0
#define EVWATCH_CHECK   1
#define EVWATCH_MAX     2

/** Handle to a "prepare" or "check" callback, registered in event_base. */
union evwatch_cb {
	evwatch_prepare_cb prepare;
	evwatch_check_cb check;
};
struct evwatch {
	/** Tail queue pointers, called "next" by convention in libevent.
	 * See <sys/queue.h> */
	TAILQ_ENTRY(evwatch) next;

	/** Pointer to owning event loop */
	struct event_base *base;

	/** Watcher type (see above) */
	unsigned type;

	/** Callback function */
	union evwatch_cb callback;

	/** User-defined argument for callback function */
	void *arg;
};
TAILQ_HEAD(evwatch_list, evwatch);

struct event_base {
	// 初始化Reactor选择的后端I/O复用机制
	const struct eventop *evsel;
	// 指向I/O复用机制真正存储的数据，它通过evsel成员的init函数来初始化
	void *evbase;
	// 事件变化队列。其用途是:如果一个文件描述符上注册的事件被多次修改，则可以使用缓冲.
	// 来避免重复的系统调用(比如epol1_ctl)。它仅能用于时间复杂度为0(1)的I/O复用技术
	struct event_changelist changelist;
	// 信号后端处理方法
	const struct eventop *evsigsel;
	// 信号事件处理器使用的数据结构，其中封装了一个由socketpair创建的管道。
	// 它用于信号处理函数和事件多路分发器之间的通信.
	struct evsig_info sig;

	// 虚拟事件,所有事件,激活事件数以及上限
	int virtual_event_count;
	int virtual_event_count_max;
	int event_count;
	int event_count_max;
	int event_count_active;
	int event_count_active_max;

	// 处理完活动队列任务是否退出
	int event_gotterm;
	// 是否立即退出
	int event_break;
	// 是否启动新事件循环
	int event_continue;

	// 活动队列优先级
	int event_running_priority;

	// 事件循环是否启动
	int running_loop;

	// 延迟回调函数数
	int n_deferreds_queued;

	/* Active event management. */
	// 活动事件队列
	struct evcallback_list *activequeues;
	// 活动队列长度
	int nactivequeues;
	// 稍后活动事件队列
	struct evcallback_list active_later_queue;

	/* common timeout logic */

	// 通用定时器队列, 长度以及大小
	struct common_timeout_list **common_timeout_queues;
	int n_common_timeouts;
	int n_common_timeouts_allocated;

	// 文件标识-I/O事件映射
	struct event_io_map io;

	// 信号-信号事件映射
	struct event_signal_map sigmap;

	// 时间堆
	struct min_heap timeheap;

	// 时间管理相关函数
	struct timeval tv_cache;
	struct evutil_monotonic_timer monotonic_timer;
	struct timeval tv_clock_diff;
	time_t last_updated_clock_diff;

#ifndef EVENT__DISABLE_THREAD_SUPPORT
	/* threading support */
	//当前运行event base线程
	unsigned long th_owner_id;
	// base锁
	void *th_base_lock;
	// 条件变量
	void *current_event_cond;
	// 在当前条件变量上的等待事件
	int current_event_waiters;
#endif
	/** The event whose callback is executing right now */
	struct event_callback *current_event;

#ifdef _WIN32
	/** IOCP support structure, if IOCP is enabled. */
	struct event_iocp_port *iocp;
#endif

	/** Flags that this base was configured with */
	enum event_base_config_flag flags;

	struct timeval max_dispatch_time;
	int max_dispatch_callbacks;
	int limit_callbacks_after_prio;

	/* Notify main thread to wake up break, etc. */
	// 唤醒主线程的相关函数
	/** True if the base already has a pending notify, and we don't need
	 * to add any more. */
	int is_notify_pending;
	/** A socketpair used by some th_notify functions to wake up the main
	 * thread. */
	evutil_socket_t th_notify_fd[2];
	/** An event used by some th_notify functions to wake up the main
	 * thread. */
	struct event th_notify;
	/** A function used to wake up the main thread from another thread. */
	int (*th_notify_fn)(struct event_base *base);

	/** Saved seed for weak random number generator. Some backends use
	 * this to produce fairness among sockets. Protected by th_base_lock. */
	struct evutil_weakrand_state weakrand_seed;

	/** List of event_onces that have not yet fired. */
	LIST_HEAD(once_event_list, event_once) once_events;

	/** "Prepare" and "check" watchers. */
	struct evwatch_list watchers[EVWATCH_MAX];
};

struct event_config_entry {
	TAILQ_ENTRY(event_config_entry) next;

	const char *avoid_method;
};

/** Internal structure: describes the configuration we want for an event_base
 * that we're about to allocate. */
struct event_config {
	TAILQ_HEAD(event_configq, event_config_entry) entries;

	int n_cpus_hint;
	struct timeval max_dispatch_interval;
	int max_dispatch_callbacks;
	int limit_callbacks_after_prio;
	enum event_method_feature require_features;
	enum event_base_config_flag flags;
};

/* Internal use only: Functions that might be missing from <sys/queue.h> */
#ifndef LIST_END
#define LIST_END(head)			NULL
#endif

#ifndef TAILQ_FIRST
#define	TAILQ_FIRST(head)		((head)->tqh_first)
#endif
#ifndef TAILQ_END
#define	TAILQ_END(head)			NULL
#endif
#ifndef TAILQ_NEXT
#define	TAILQ_NEXT(elm, field)		((elm)->field.tqe_next)
#endif

#ifndef TAILQ_FOREACH
#define TAILQ_FOREACH(var, head, field)					\
	for ((var) = TAILQ_FIRST(head);					\
	     (var) != TAILQ_END(head);					\
	     (var) = TAILQ_NEXT(var, field))
#endif

#ifndef TAILQ_INSERT_BEFORE
#define	TAILQ_INSERT_BEFORE(listelm, elm, field) do {			\
	(elm)->field.tqe_prev = (listelm)->field.tqe_prev;		\
	(elm)->field.tqe_next = (listelm);				\
	*(listelm)->field.tqe_prev = (elm);				\
	(listelm)->field.tqe_prev = &(elm)->field.tqe_next;		\
} while (0)
#endif

#define N_ACTIVE_CALLBACKS(base)					\
	((base)->event_count_active)

int evsig_set_handler_(struct event_base *base, int evsignal,
			  void (*fn)(int));
int evsig_restore_handler_(struct event_base *base, int evsignal);

int event_add_nolock_(struct event *ev,
    const struct timeval *tv, int tv_is_absolute);
/** Argument for event_del_nolock_. Tells event_del not to block on the event
 * if it's running in another thread. */
#define EVENT_DEL_NOBLOCK 0
/** Argument for event_del_nolock_. Tells event_del to block on the event
 * if it's running in another thread, regardless of its value for EV_FINALIZE
 */
#define EVENT_DEL_BLOCK 1
/** Argument for event_del_nolock_. Tells event_del to block on the event
 * if it is running in another thread and it doesn't have EV_FINALIZE set.
 */
#define EVENT_DEL_AUTOBLOCK 2
/** Argument for event_del_nolock_. Tells event_del to proceed even if the
 * event is set up for finalization rather for regular use.*/
#define EVENT_DEL_EVEN_IF_FINALIZING 3
int event_del_nolock_(struct event *ev, int blocking);
int event_remove_timer_nolock_(struct event *ev);

void event_active_nolock_(struct event *ev, int res, short count);
EVENT2_EXPORT_SYMBOL
int event_callback_activate_(struct event_base *, struct event_callback *);
int event_callback_activate_nolock_(struct event_base *, struct event_callback *);
int event_callback_cancel_(struct event_base *base,
    struct event_callback *evcb);

void event_callback_finalize_nolock_(struct event_base *base, unsigned flags, struct event_callback *evcb, void (*cb)(struct event_callback *, void *));
EVENT2_EXPORT_SYMBOL
void event_callback_finalize_(struct event_base *base, unsigned flags, struct event_callback *evcb, void (*cb)(struct event_callback *, void *));
int event_callback_finalize_many_(struct event_base *base, int n_cbs, struct event_callback **evcb, void (*cb)(struct event_callback *, void *));


EVENT2_EXPORT_SYMBOL
void event_active_later_(struct event *ev, int res);
void event_active_later_nolock_(struct event *ev, int res);
int event_callback_activate_later_nolock_(struct event_base *base,
    struct event_callback *evcb);
int event_callback_cancel_nolock_(struct event_base *base,
    struct event_callback *evcb, int even_if_finalizing);
void event_callback_init_(struct event_base *base,
    struct event_callback *cb);

/* FIXME document. */
EVENT2_EXPORT_SYMBOL
void event_base_add_virtual_(struct event_base *base);
void event_base_del_virtual_(struct event_base *base);

/** For debugging: unless assertions are disabled, verify the referential
    integrity of the internal data structures of 'base'.  This operation can
    be expensive.

    Returns on success; aborts on failure.
*/
EVENT2_EXPORT_SYMBOL
void event_base_assert_ok_(struct event_base *base);
void event_base_assert_ok_nolock_(struct event_base *base);


/* Helper function: Call 'fn' exactly once every inserted or active event in
 * the event_base 'base'.
 *
 * If fn returns 0, continue on to the next event. Otherwise, return the same
 * value that fn returned.
 *
 * Requires that 'base' be locked.
 */
int event_base_foreach_event_nolock_(struct event_base *base,
    event_base_foreach_event_cb cb, void *arg);

/* Cleanup function to reset debug mode during shutdown.
 *
 * Calling this function doesn't mean it'll be possible to re-enable
 * debug mode if any events were added.
 */
void event_disable_debug_mode(void);

#ifdef __cplusplus
}
#endif

#endif /* EVENT_INTERNAL_H_INCLUDED_ */
