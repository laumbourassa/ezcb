# ezcb.h

**ezcb.h** is a single-header C library that provides a lightweight event/callback dispatcher. It supports prioritized callbacks, one-shot handlers, wildcard unregistration, and optional ISR-safe deferred triggering or static allocation for use in constrained environments.

## Features

- Register callbacks by name (string triggers) with execution priorities
- One-shot callbacks that unregister themselves after firing
- Wildcard-style unregistration (by trigger, function, context, or all)
- Callback return value can stop further processing (EZCB_STOP)
- Optional ISR-safe trigger queue (EZCB_ENABLE_ISR)
- Optional no-malloc static mode for embedded use (EZCB_NO_MALLOC)
- Small, header-only implementation with minimal dependencies
- Configurable table sizes and event queue via preprocessor macros
- No runtime dependencies beyond the C standard library

## Requirements

- C99 (or later)
- Standard headers: stdint.h, stddef.h, stdbool.h
- Optional: define EZCB_ENABLE_ISR to enable ISR-safe triggering
- Optional: define EZCB_NO_MALLOC to compile without dynamic allocation

## Quick Start

1. Implement the library (in exactly one source file):

```c
#define EZCB_IMPLEMENTATION
#include "ezcb.h"
```

2. Include the header (anywhere necesary):

```c
#include "ezcb.h"
```

3. Register callbacks and trigger events:

### Example: Basic registration and triggering

```c
#include "ezcb.h"
#include <stdio.h>

static ezcb_result_t on_event(void* ctx, void* data)
{
    (void)ctx;
    const char* msg = (const char*)data;
    printf("Received: %s\n", msg);
    return EZCB_CONTINUE;
}

int main(void)
{
    ezcb_init();

    ezcb_register("my_event", 10, on_event, NULL);

    ezcb_trigger("my_event", "Hello, ezcb!");

    ezcb_deinit();
    return 0;
}
```

### Example: One-shot callback

```c
static ezcb_result_t once_cb(void* ctx, void* data)
{
    (void)ctx;
    printf("One-shot: %s\n", (char*)data);
    return EZCB_CONTINUE;
}

/* Register a callback that runs only once */
ezcb_register_once("connect", 50, once_cb, NULL);
```

### Example: Priority and stopping propagation

```c
static ezcb_result_t stop_cb(void* ctx, void* data)
{
    (void)ctx; (void)data;
    puts("Stopping further callbacks");
    return EZCB_STOP;
}

/* Higher priority runs first (larger numeric priority) */
ezcb_register("event", 100, stop_cb, NULL);
ezcb_register("event", 10, on_event, NULL);

/* Only stop_cb runs because it returns EZCB_STOP */
ezcb_trigger("event", NULL);
```

### Example: Unregistering callbacks

```c
/* Remove all callbacks for a trigger */
ezcb_unregister("my_event", NULL, NULL);

/* Remove all callbacks by function pointer */
ezcb_unregister(NULL, on_event, NULL);

/* Remove everything */
ezcb_unregister(NULL, NULL, NULL);
```

### Example: ISR-safe triggering (optional)

Compile with `-DEZCB_ENABLE_ISR`. From an ISR you can enqueue events (non-blocking) and later dispatch from main context:

```c
/* From ISR-safe context */
ezcb_trigger_isr("tick", NULL);

/* From main loop */
ezcb_dispatch();
```

## Configuration Macros

Customize behavior by defining these macros before including ezcb.h:

- EZCB_NO_MALLOC — Disable dynamic allocation and use static tables.
  - If defined, set EZCB_MAX_BUCKETS and EZCB_MAX_NODES to tune memory usage.
- EZCB_MAX_BUCKETS — Number of hash buckets when EZCB_NO_MALLOC is enabled (default 32).
- EZCB_MAX_NODES — Number of nodes when EZCB_NO_MALLOC is enabled (default 64).
- EZCB_ENABLE_ISR — Enable ISR-safe trigger queue and dispatch.
- EZCB_EVENT_QUEUE_SIZE — Size of ISR event queue when EZCB_ENABLE_ISR is defined (default 16).

Example:

```c
#define EZCB_NO_MALLOC
#define EZCB_MAX_BUCKETS 16
#define EZCB_MAX_NODES 32
#include "ezcb.h"
```

## API Summary

- void ezcb_init(void);
  - Initialize internal structures. Automatically called on first use but may be called explicitly.
- void ezcb_deinit(void);
  - Deinitialize and free/reset internal resources.
- int ezcb_register(const char* trigger, uint8_t priority, ezcb_fn_t fn, void* ctx);
  - Register a callback for a trigger. Returns 0 on success.
- int ezcb_register_once(const char* trigger, uint8_t priority, ezcb_fn_t fn, void* ctx);
  - Register a one-shot callback (removed after first invocation).
- int ezcb_unregister(const char* trigger, ezcb_fn_t fn, void* ctx);
  - Unregister callbacks that match the provided criteria; NULL acts as a wildcard. Returns number removed.
- void ezcb_trigger(const char* trigger, void* data);
  - Fire all callbacks registered under the trigger, in priority order.
- (Optional) int ezcb_trigger_isr(const char* trigger, void* data);
  - Enqueue an event from an ISR context (non-blocking). Returns 0 on success. Requires EZCB_ENABLE_ISR.
- (Optional) void ezcb_dispatch(void);
  - Dispatch all queued ISR events. Requires EZCB_ENABLE_ISR.

Callback type:

```c
typedef ezcb_result_t (*ezcb_fn_t)(void* ctx, void* data);
```

Return EZCB_CONTINUE to let further callbacks run, or EZCB_STOP to halt processing of remaining callbacks for that trigger.

## How it works

- ezcb.h stores registrations in a simple hash table where each bucket is a linked list of callback nodes.
- Nodes are kept in priority order (higher priority values come first) so callbacks run in the requested order.
- In dynamic mode the table auto-resizes; in static mode (EZCB_NO_MALLOC) fixed arrays and free lists are used.
- Optional ISR mode uses a ring buffer to enqueue trigger events from interrupt context; events are processed later by calling ezcb_dispatch().

## License

ezcb.h is released under the **MIT License**. You are free to use, modify, and distribute it under the terms of the license.

## Author

This header was developed by **Laurent Mailloux-Bourassa**.
