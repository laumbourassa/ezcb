/*
 * ezcb.h - Lightweight Event/Callback Dispatcher for C
 *
 * Copyright (c) 2026 Laurent Mailloux-Bourassa
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef EZCB_H
#define EZCB_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/****************************************************************
 * Configuration switches
 ****************************************************************/

/* Disable dynamic allocation */
// #define EZCB_NO_MALLOC

/* Enable thread-safety using mutex */
// #define EZCB_THREAD_SAFE

/* Enable ISR-safe deferred triggering */
// #define EZCB_ENABLE_ISR

#ifdef EZCB_NO_MALLOC
    #ifndef EZCB_MAX_BUCKETS
        #define EZCB_MAX_BUCKETS 32
    #endif
    #ifndef EZCB_MAX_NODES
        #define EZCB_MAX_NODES 64
    #endif
    #ifndef EZCB_MAX_TRIGGER_LENGTH
        #define EZCB_MAX_TRIGGER_LENGTH 32
    #endif
#endif  /* EZCB_NO_MALLOC */

/* Event queue size for ISR support */
#ifdef EZCB_ENABLE_ISR
    #ifndef EZCB_EVENT_QUEUE_SIZE
        #define EZCB_EVENT_QUEUE_SIZE 16
    #endif
#endif  /* EZCB_ENABLE_ISR */

/****************************************************************
 * Callback
 ****************************************************************/

typedef enum
{
    EZCB_CONTINUE,
    EZCB_STOP
} ezcb_result_t;

/**
 * @brief Callback function type used by the EZCB system.
 *
 * Functions of this type are invoked when a trigger fires. The callback
 * receives the user‑supplied context pointer provided at registration,
 * along with the caller‑supplied data passed to ezcb_trigger() or
 * ezcb_trigger_isr(). The callback may return EZCB_CONTINUE to allow
 * further callbacks to run, or EZCB_STOP to halt processing.
 *
 * @param ctx   User‑defined context pointer associated with the registration.
 * @param data  Data pointer supplied by the caller when the trigger fires.
 * 
 * @return EZCB_CONTINUE to keep processing callbacks, or EZCB_STOP to halt.
 */
typedef ezcb_result_t (*ezcb_fn_t)(
    void* ctx,
    void* data
);

/****************************************************************
 * Public API
 ****************************************************************/

/**
 * @brief Initialize the EZCB callback system.
 *
 * Sets up internal tables, free lists, and counters. Will be called
 * automatically on first registration.
 */
void ezcb_init(void);

/**
 * @brief Deinitialize the EZCB callback system and release resources.
 *
 * Frees all internal memory used by the callback system. In malloc mode,
 * all dynamically allocated nodes and the hash table are released. In
 * static‑allocation mode, the internal tables and free lists are reset.
 *
 * After calling this function, the system returns to an uninitialized
 * state and ezcb_init() must be called again before further use.
 */
 void ezcb_deinit(void);

/**
 * @brief Register a callback for a given trigger.
 *
 * Registers a callback function associated with a trigger string.
 * Callbacks with higher priority values run earlier. The callback
 * remains registered until explicitly unregistered.
 *
 * @param trigger     Null‑terminated trigger name.
 * @param priority    Execution priority (higher runs first).
 * @param fn          Callback function pointer.
 * @param ctx         User‑supplied context pointer passed to the callback.
 * 
 * @return 0 on success, negative value on allocation or insertion failure.
 */
int ezcb_register(
    const char* trigger,
    uint8_t priority,
    ezcb_fn_t fn,
    void* ctx
);

/**
 * @brief Register a one‑shot callback for a given trigger.
 *
 * Same as ezcb_register(), but the callback is automatically removed
 * after it fires once.
 *
 * @param trigger     Null‑terminated trigger name.
 * @param priority    Execution priority (higher runs first).
 * @param fn          Callback function pointer.
 * @param ctx         User‑supplied context pointer passed to the callback.
 * 
 * @return 0 on success, negative value on allocation or insertion failure.
 */
int ezcb_register_once(
    const char* trigger,
    uint8_t priority,
    ezcb_fn_t fn,
    void* ctx
);

/**
 * @brief Unregister callbacks using wildcard matching.
 *
 * Removes any callback whose fields match the provided criteria.
 * A NULL parameter acts as a wildcard and matches all values.
 *
 * Examples:
 *   ezcb_unregister("event", NULL, NULL);   // remove all for trigger
 *   ezcb_unregister(NULL, fn, NULL);        // remove all using fn
 *   ezcb_unregister(NULL, NULL, ctx);       // remove all using ctx
 *   ezcb_unregister(NULL, fn, ctx);         // remove fn+ctx pair
 *   ezcb_unregister(NULL, NULL, NULL);      // remove EVERYTHING
 *
 * @param trigger  Trigger name to match, or NULL for wildcard.
 * @param fn       Function pointer to match, or NULL for wildcard.
 * @param ctx      Context pointer to match, or NULL for wildcard.
 *
 * @return Number of removed callbacks.
 */
int ezcb_unregister(
    const char* trigger,
    ezcb_fn_t fn,
    void* ctx
);

/**
 * @brief Trigger all callbacks registered under a given name.
 *
 * Executes callbacks in priority order. If a callback returns
 * EZCB_STOP, remaining callbacks for that trigger are skipped.
 *
 * @param trigger     Trigger name to fire.
 * @param data        Caller‑supplied data passed to callbacks.
 */
void ezcb_trigger(
    const char* trigger,
    void* data
);

/**
 * @brief Queue a trigger event from an ISR context.
 *
 * Adds a trigger event to the ISR‑safe queue. The event will be
 * processed later by ezcb_dispatch(). Non‑blocking and safe for ISR use.
 * Define EZCB_ENABLE_ISR for implementation.
 *
 * @param trigger     Trigger name to enqueue.
 * @param data        Caller‑supplied data pointer.
 * 
 * @return 0 on success, negative value if the queue is full.
 */
int  ezcb_trigger_isr(
    const char* trigger,
    void* data
);

/**
 * @brief Dispatch queued ISR trigger events.
 * Define EZCB_ENABLE_ISR for implementation.
 *
 * Processes all pending events queued by ezcb_trigger_isr().
 * Should be called from the main loop or a safe execution context.
 */
void ezcb_dispatch(void);

#endif  /* EZCB_H */

/****************************************************************
 * Implementation
 ****************************************************************/
#ifdef EZCB_IMPLEMENTATION

#include <string.h>
#ifndef EZCB_NO_MALLOC
    #include <stdlib.h>
#endif

#ifdef EZCB_THREAD_SAFE
    #include <threads.h>
    
    #define EZCB_MUTEX_DECLARE(m)   static mtx_t (m)
    #define EZCB_MUTEX_INIT(m)      mtx_init(&(m), mtx_plain)
    #define EZCB_MUTEX_LOCK(m)      mtx_lock(&(m))
    #define EZCB_MUTEX_UNLOCK(m)    mtx_unlock(&(m))
    #define EZCB_MUTEX_DESTROY(m)   mtx_destroy(&(m))
#else
    #define EZCB_MUTEX_DECLARE(m)
    #define EZCB_MUTEX_INIT(m)
    #define EZCB_MUTEX_LOCK(m)
    #define EZCB_MUTEX_UNLOCK(m)
    #define EZCB_MUTEX_DESTROY(m)
#endif

/****************************************************************
 * Internal structures
 ****************************************************************/

typedef struct ezcb_node ezcb_node_t;
typedef struct ezcb_node
{
#ifdef EZCB_NO_MALLOC
    char trigger[EZCB_MAX_TRIGGER_LENGTH];
#else
    char* trigger;
#endif
    uint8_t priority;
    bool once;
    ezcb_fn_t fn;
    void* ctx;
    ezcb_node_t* next;
} ezcb_node_t;

#ifdef EZCB_ENABLE_ISR
typedef struct ezcb_evt
{
    const char* trigger;
    void* data;
} ezcb_evt_t;

static volatile uint8_t ezcb_evt_head;
static volatile uint8_t ezcb_evt_tail;
static ezcb_evt_t ezcb_evt_queue[EZCB_EVENT_QUEUE_SIZE];
#endif  /* EZCB_ENABLE_ISR*/

EZCB_MUTEX_DECLARE(ezcb_mtx);

/****************************************************************
 * Hash table state
 ****************************************************************/

static ezcb_node_t** ezcb_table   = NULL;
static size_t ezcb_buckets = 0;
static size_t ezcb_count   = 0;

#ifdef EZCB_NO_MALLOC
static ezcb_node_t ezcb_nodes[EZCB_MAX_NODES];
static ezcb_node_t* ezcb_free_list = NULL;
static ezcb_node_t* ezcb_table_static[EZCB_MAX_BUCKETS];
#endif  /* EZCB_NO_MALLOC */

/****************************************************************
 * Hash
 ****************************************************************/

static uint32_t ezcb_hash(const char* s)
{
    uint32_t h = 5381;
    while (*s)
    {
        h = ((h << 5) + h) ^ (uint8_t)*s++;
    }
    return h;
}

/****************************************************************
 * Mutex helpers
 ****************************************************************/

static inline void ezcb_lock(void)
{
    EZCB_MUTEX_LOCK(ezcb_mtx);
}

static inline void ezcb_unlock(void)
{
    EZCB_MUTEX_UNLOCK(ezcb_mtx);
}

/****************************************************************
 * Allocation
 ****************************************************************/

static ezcb_node_t* ezcb_node_alloc(void)
{
#ifdef EZCB_NO_MALLOC
    if (!ezcb_free_list)
    {
        ezcb_unlock();
        return NULL;
    }
    
    ezcb_node_t* n = ezcb_free_list;
    ezcb_free_list = n->next;
    
    return n;
#else
    return (ezcb_node_t*) malloc(sizeof(ezcb_node_t));
#endif
}

static void ezcb_node_free(
    ezcb_node_t *n
)
{
#ifdef EZCB_NO_MALLOC
    ezcb_lock();
    
    n->next = ezcb_free_list;
    ezcb_free_list = n;
    
    ezcb_unlock();
#else
    free(n->trigger);
    n->trigger = NULL;
    free(n);
#endif
}

/****************************************************************
 * Resize
 ****************************************************************/

#ifndef EZCB_NO_MALLOC
static int ezcb_resize(
    size_t new_size
)
{
    ezcb_node_t** new_table = calloc(new_size, sizeof(*new_table));
    if (!new_table) return -1;

    for (size_t i = 0; i < ezcb_buckets; i++)
    {
        ezcb_node_t* n = ezcb_table[i];
        while (n)
        {
            ezcb_node_t* next = n->next;
            uint32_t idx = ezcb_hash(n->trigger) % new_size;
            n->next = new_table[idx];
            new_table[idx] = n;
            n = next;
        }
    }

    free(ezcb_table);
    ezcb_table = new_table;
    ezcb_buckets = new_size;
    return 0;
}
#endif

/****************************************************************
 * Initialize
 ****************************************************************/

void ezcb_init(void)
{
    if (ezcb_table) return;
    
    EZCB_MUTEX_INIT(ezcb_mtx);
    
#ifdef EZCB_NO_MALLOC
    ezcb_buckets = EZCB_MAX_BUCKETS;
    ezcb_table = ezcb_table_static;
    memset(ezcb_table, 0, sizeof(ezcb_table_static));

    ezcb_lock();
    
    ezcb_free_list = NULL;
    for (size_t i = 0; i < EZCB_MAX_NODES; i++)
    {
        ezcb_nodes[i].next = ezcb_free_list;
        ezcb_free_list = &ezcb_nodes[i];
    }
    
    ezcb_unlock();
    
#else
    ezcb_buckets = 16;
    ezcb_table = calloc(ezcb_buckets, sizeof(*ezcb_table));
#endif
    ezcb_count = 0;
}

/****************************************************************
 * Deinitialize
 ****************************************************************/

void ezcb_deinit(void)
{
    if (!ezcb_table)
    {
        EZCB_MUTEX_DESTROY(ezcb_mtx);
        return;
    }

    ezcb_lock();
    
    for (size_t i = 0; i < ezcb_buckets; i++)
    {
        ezcb_node_t* n = ezcb_table[i];
        while (n)
        {
            ezcb_node_t* next = n->next;
#ifdef EZCB_NO_MALLOC
            n->next = ezcb_free_list;
            ezcb_free_list = n;
#else
            free(n);
#endif
            n = next;
        }
        ezcb_table[i] = NULL;
    }

#ifndef EZCB_NO_MALLOC
    free(ezcb_table);
#endif  /* EZCB_NO_MALLOC */

    ezcb_table   = NULL;
    ezcb_buckets = 0;
    ezcb_count   = 0;

#ifdef EZCB_ENABLE_ISR
    ezcb_evt_head = 0;
    ezcb_evt_tail = 0;
#endif  /* EZCB_ENABLE_ISR */

    ezcb_unlock();
    EZCB_MUTEX_DESTROY(ezcb_mtx);
}

/****************************************************************
 * Register
 ****************************************************************/

static int ezcb_register_internal(
    const char* trigger,
    uint8_t priority,
    ezcb_fn_t fn,
    void* ctx,
    bool once
)
{
	ezcb_lock();
	
    if (!ezcb_table)
    {
        ezcb_init();
    }

#ifdef EZCB_NO_MALLOC
    size_t trigger_length = strlen(trigger);
    
    if (trigger_length >= EZCB_MAX_TRIGGER_LENGTH)
	{
		ezcb_unlock();
		return -1;
	}
#else
    
    if (ezcb_count * 4 >= ezcb_buckets * 3)
    {
        if (ezcb_resize(ezcb_buckets * 2) != 0)
        {
            ezcb_unlock();
            return -1;
        }
    }
	
#endif

    ezcb_node_t* node = ezcb_node_alloc();
    if (!node)
	{
		ezcb_unlock();
		return -1;
	}

#ifdef EZCB_NO_MALLOC
    strncpy(node->trigger, trigger, trigger_length);
    node->trigger[trigger_length] = '\0';
#else
    node->trigger = strdup(trigger);
#endif
    node->priority = priority;
    node->once = once;
    node->fn = fn;
    node->ctx = ctx;
    
    uint32_t idx = ezcb_hash(trigger) % ezcb_buckets;
    ezcb_node_t** cur = &ezcb_table[idx];

    while (*cur)
    {
        if (strcmp((*cur)->trigger, trigger) == 0 && (*cur)->priority < priority)
        {
            break;
        }
        cur = &(*cur)->next;
    }

    node->next = *cur;
    *cur = node;
    ezcb_count++;
    
    ezcb_unlock();
    return 0;
}

int ezcb_register(
    const char* trigger,
    uint8_t priority,
    ezcb_fn_t fn,
    void* ctx
)
{
    return ezcb_register_internal(trigger, priority, fn, ctx, false);
}

int ezcb_register_once(
    const char* trigger,
    uint8_t priority,
    ezcb_fn_t fn,
    void* ctx
)
{
    return ezcb_register_internal(trigger, priority, fn, ctx, true);
}

/****************************************************************
 * Unregister
 ****************************************************************/

int ezcb_unregister(
    const char* trigger,
    ezcb_fn_t fn,
    void* ctx
)
{
    ezcb_lock();
	
    if (!ezcb_table)
	{
		ezcb_unlock();
		return 0;
	}

    int removed = 0;

    if (trigger)
    {
        uint32_t idx = ezcb_hash(trigger) % ezcb_buckets;
        ezcb_node_t** cur = &ezcb_table[idx];

        while (*cur)
        {
            ezcb_node_t* n = *cur;

            if (strcmp(n->trigger, trigger) == 0 &&
                (fn  == NULL || n->fn  == fn) &&
                (ctx == NULL || n->ctx == ctx))
            {
                *cur = n->next;
                ezcb_node_free(n);
                ezcb_count--;
                removed++;
                continue;
            }

            cur = &(*cur)->next;
        }

        ezcb_unlock();
        return removed;
    }

    for (size_t i = 0; i < ezcb_buckets; i++)
    {
        ezcb_node_t** cur = &ezcb_table[i];

        while (*cur)
        {
            ezcb_node_t* n = *cur;

            if ((fn  == NULL || n->fn  == fn) &&
                (ctx == NULL || n->ctx == ctx))
            {
                *cur = n->next;
                ezcb_node_free(n);
                ezcb_count--;
                removed++;
                continue;
            }

            cur = &(*cur)->next;
        }
    }

    ezcb_unlock();
    return removed;
}


/****************************************************************
 * Trigger
 ****************************************************************/

void ezcb_trigger(
    const char* trigger,
    void* data
)
{
	ezcb_lock();
	
    if (!ezcb_table)
	{
		ezcb_unlock();
		return;
	}

    uint32_t idx = ezcb_hash(trigger) % ezcb_buckets;
    ezcb_node_t** cur = &ezcb_table[idx];

    while (*cur)
    {
        ezcb_node_t* n = *cur;

        if (strcmp(n->trigger, trigger) == 0)
        {
            ezcb_result_t r = n->fn(n->ctx, data);

            if (n->once)
            {
                *cur = n->next;
#ifdef EZCB_NO_MALLOC
                n->next = ezcb_free_list;
                ezcb_free_list = n;
#else
                free(n->trigger);
                free(n);
#endif
                ezcb_count--;
                if (r == EZCB_STOP) break;
                continue; 
            }

            if (r == EZCB_STOP) break;
        }

        cur = &(*cur)->next;
    }

    ezcb_unlock();
}


/****************************************************************
 * ISR support
 ****************************************************************/
#ifdef EZCB_ENABLE_ISR

int ezcb_trigger_isr(
    const char* trigger,
    void* data
)
{
    uint8_t next = (ezcb_evt_head + 1) % EZCB_EVENT_QUEUE_SIZE;

    if (next == ezcb_evt_tail) return -1;

    ezcb_evt_queue[ezcb_evt_head].trigger = trigger;
    ezcb_evt_queue[ezcb_evt_head].data = data;
    ezcb_evt_head = next;
    return 0;
}

void ezcb_dispatch(void)
{
    while (ezcb_evt_tail != ezcb_evt_head)
    {
        ezcb_evt_t evt = ezcb_evt_queue[ezcb_evt_tail];
        ezcb_evt_tail = (ezcb_evt_tail + 1) % EZCB_EVENT_QUEUE_SIZE;

        ezcb_trigger(evt.trigger, evt.data);
    }
}

#endif /* EZCB_ENABLE_ISR */

#endif /* EZCB_IMPLEMENTATION */
