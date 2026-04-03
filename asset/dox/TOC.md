# mulle-aba Library Documentation for AI
<!-- Keywords: lock-free, memory-reclamation -->

## 1. Introduction & Purpose

mulle-aba is a lock-free, cross-platform solution to the ABA problem in concurrent programming. The ABA problem occurs when using atomic compare-and-swap (CAS) operations on shared memory that is freed and reallocated, causing false success of CAS operations. mulle-aba solves this by issuing a "timestamp" to each thread when it registers, allowing detection of use-after-free scenarios. This is a specialized utility in the mulle-concurrent ecosystem for implementing lock-free data structures (stacks, queues, linked lists) without false sharing or memory corruption.

## 2. Key Concepts & Design Philosophy

**The ABA Problem:**
- Thread A reads value X from memory
- Thread B changes X to Y, then back to X
- Thread A's CAS operation falsely succeeds, not detecting B's changes

**mulle-aba Solution:**
- Each thread has a unique timestamp
- When freeing memory, timestamp is recorded with pointer
- CAS operations check both pointer AND timestamp
- Prevents false positives from recycled memory addresses

**Design Principles:**

- **Timestamp-Based Detection:** Uses per-thread timestamps to track generations.
- **Thread-Safe:** Global state protected by locks internally; API is thread-safe.
- **Multiple Instances:** Can use single global or multiple independent ABA instances.
- **Explicit Registration:** Threads must register/unregister for timestamp tracking.
- **Allocator-Agnostic:** Works with any memory allocator (user provides free function).

## 3. Core API & Data Structures

### 3.1 Global ABA Instance

#### Types

**`struct mulle_aba`**

- **Purpose:** ABA context holding timestamp thread-local storage and internal state.
- **Usage:** Typically one global instance, but can be instantiated multiple times.

#### Initialization

**`void mulle_aba_init(struct mulle_allocator *allocator)`**

- **Purpose:** Initialize the global ABA instance.
- **Must Call:** Before any thread registers.
- **Thread Safety:** Call from main thread only.

**`void mulle_aba_done(void)`**

- **Purpose:** Destroy the global ABA instance.
- **Must Call:** After all threads unregistered.

**`struct mulle_aba *mulle_aba_get_global(void)`**

- **Purpose:** Retrieve the global ABA instance.
- **Returns:** Pointer to global instance.

**`void mulle_aba_set_global(struct mulle_aba *p)`**

- **Purpose:** Override the global ABA instance (rare).

#### Thread Registration

**`void mulle_aba_register(void)`**

- **Purpose:** Register current thread with ABA.
- **Must Call:** Once per thread, at thread startup.
- **Effect:** Issues unique timestamp to thread.

**`void mulle_aba_unregister(void)`**

- **Purpose:** Unregister current thread.
- **Must Call:** Before thread exit.
- **Effect:** Reclaims thread-local storage.

**`void mulle_aba_checkin(void)`**

- **Purpose:** Optional periodic call to advance thread's timestamp.
- **Rationale:** Without checkin, thread's timestamp never advances (may hold memory indefinitely).
- **Suggestion:** Call before blocking operations (select, wait, etc.).

#### Memory Management

**`int mulle_aba_free(void (*p_free)(void *), void *pointer)`**

- **Purpose:** Free a pointer with ABA protection.
- **Parameters:**
  - `p_free`: Callback function to actually free the pointer.
  - `pointer`: Memory to free.
- **Returns:** 0 on success; -1 if timestamp unavailable (thread not registered).
- **Behavior:** Records timestamp; calls p_free when safe for all threads.

**`int mulle_aba_free_owned_pointer(void (*p_free)(void *owner, void *pointer), void *pointer, void *owner)`**

- **Purpose:** Free pointer with owner context.
- **Parameters:**
  - `p_free`: Callback receiving owner and pointer.
  - `pointer`: Memory to free.
  - `owner`: Context passed to p_free.
- **Behavior:** p_free called as `p_free(owner, pointer)`.

#### Testing Support

**`int mulle_aba_is_registered(void)`**

- **Purpose:** Check if current thread is registered.
- **Returns:** Non-zero if registered.

**`uintptr_t mulle_aba_current_thread_get_timestamp(void)`**

- **Purpose:** Get current thread's timestamp.
- **Returns:** Current timestamp value.

**`void *mulle_aba_get_worldpointer(void)`**

- **Purpose:** Get internal world pointer (debugging).

**`void mulle_aba_reset(void)`**

- **Purpose:** Reset global ABA state (testing only).

### 3.2 Multiple ABA Instances

For scenarios requiring multiple independent ABA systems:

**`int _mulle_aba_init(struct mulle_aba *p, struct mulle_allocator *allocator)`**

- **Purpose:** Initialize a specific ABA instance (not global).
- **Returns:** 0 on success, -1 on error.

**`void _mulle_aba_done(struct mulle_aba *p)`**

- **Purpose:** Destroy a specific ABA instance.

**`int _mulle_aba_register_current_thread(struct mulle_aba *p)`**

- **Purpose:** Register current thread with specific instance.
- **Returns:** 0 on success.

**`int _mulle_aba_unregister_current_thread(struct mulle_aba *p)`**

- **Purpose:** Unregister current thread from specific instance.

## 4. Performance Characteristics

- **Register/Unregister:** O(n) first call (TSS creation); O(1) thereafter.
- **Free Operation:** O(1) + deferred cleanup when safe.
- **Checkin:** O(1) timestamp update.
- **Memory Overhead:** Per-thread TSS entry + timestamp storage.

**Scalability:**
- Scales to thousands of threads (one timestamp per thread).
- Memory cleanup deferred until all threads have advanced past timestamp.
- May hold memory longer than ideal in low-activity scenarios.

## 5. AI Usage Recommendations & Patterns

### Best Practices:

1. **Always Register/Unregister:** Every thread must register at start, unregister at end.

2. **Call Checkin Periodically:** Before blocking operations, especially long waits.

3. **Provide Correct Free Function:** p_free must actually deallocate the memory.

4. **Use Global Instance by Default:** Only use multiple instances if required.

5. **Test on Multiple CPUs:** ABA issues only manifest with true parallelism.

### Common Pitfalls:

1. **Forgetting Register/Unregister:** Unregistered threads won't release timestamps; memory accumulates.

2. **Never Calling Checkin:** Without checkin in long-running threads, old timestamps block cleanup.

3. **Incorrect Free Callback:** If p_free doesn't free, memory leaks.

4. **Using After Unregister:** Calling free after unregister causes undefined behavior.

5. **Race in Cleanup:** Cleanup still requires wait for all threads' timestamps; not instant.

## 6. Integration Examples

### Example 1: Basic Lock-Free Stack with ABA

```c
#include <mulle-aba/mulle-aba.h>
#include <mulle-thread/mulle-thread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct StackNode {
    void *data;
    struct StackNode *next;
} StackNode;

mulle_atomic_pointer_t stack_top = NULL;

void push(void *data) {
    StackNode *node = malloc(sizeof(StackNode));
    node->data = data;
    
    StackNode *old_top;
    do {
        old_top = (StackNode *)_mulle_atomic_pointer_read(&stack_top);
        node->next = old_top;
    } while (!_mulle_atomic_pointer_compare_and_swap(&stack_top, old_top, node));
}

void *pop(void) {
    StackNode *old_top;
    do {
        old_top = (StackNode *)_mulle_atomic_pointer_read(&stack_top);
        if (!old_top) return NULL;
    } while (!_mulle_atomic_pointer_compare_and_swap(&stack_top, old_top, old_top->next));
    
    void *data = old_top->data;
    mulle_aba_free(free, old_top);
    return data;
}

int main() {
    mulle_aba_init(NULL);
    mulle_aba_register();
    
    push("item1");
    push("item2");
    push("item3");
    
    printf("Popped: %s\n", (char *)pop());
    printf("Popped: %s\n", (char *)pop());
    
    mulle_aba_unregister();
    mulle_aba_done();
    
    return 0;
}
```

### Example 2: Multi-Threaded Push/Pop

```c
#include <mulle-aba/mulle-aba.h>
#include <mulle-thread/mulle-thread.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct {
    int value;
    struct Node *next;
} Node;

mulle_atomic_pointer_t head = NULL;

void *producer(void *arg) {
    int thread_id = (intptr_t)arg;
    
    mulle_aba_register();
    
    for (int i = 0; i < 10; i++) {
        Node *node = malloc(sizeof(Node));
        node->value = thread_id * 100 + i;
        
        Node *old_head;
        do {
            old_head = (Node *)_mulle_atomic_pointer_read(&head);
            node->next = old_head;
        } while (!_mulle_atomic_pointer_compare_and_swap(&head, old_head, node));
    }
    
    mulle_aba_unregister();
    return NULL;
}

void *consumer(void *arg) {
    mulle_aba_register();
    
    int count = 0;
    Node *node;
    
    while (count < 20) {
        do {
            node = (Node *)_mulle_atomic_pointer_read(&head);
            if (!node) break;
        } while (!_mulle_atomic_pointer_compare_and_swap(&head, node, node->next));
        
        if (node) {
            printf("Consumed: %d\n", node->value);
            mulle_aba_free(free, node);
            count++;
        }
    }
    
    mulle_aba_unregister();
    return NULL;
}

int main() {
    mulle_aba_init(NULL);
    
    mulle_thread_t prod1, prod2, cons;
    
    mulle_thread_create(&prod1, producer, (void *)1);
    mulle_thread_create(&prod2, producer, (void *)2);
    mulle_thread_create(&cons, consumer, NULL);
    
    mulle_thread_join(prod1, NULL);
    mulle_thread_join(prod2, NULL);
    mulle_thread_join(cons, NULL);
    
    mulle_aba_done();
    
    return 0;
}
```

### Example 3: Periodic Checkin

```c
#include <mulle-aba/mulle-aba.h>
#include <mulle-thread/mulle-thread.h>
#include <stdio.h>
#include <unistd.h>

void *long_running_task(void *arg) {
    mulle_aba_register();
    
    for (int i = 0; i < 10; i++) {
        printf("Working... %d\n", i);
        sleep(1);
        
        // Advance timestamp periodically
        mulle_aba_checkin();
    }
    
    mulle_aba_unregister();
    return NULL;
}

int main() {
    mulle_aba_init(NULL);
    
    mulle_thread_t thread;
    mulle_thread_create(&thread, long_running_task, NULL);
    mulle_thread_join(thread, NULL);
    
    mulle_aba_done();
    
    return 0;
}
```

## 7. Dependencies

Direct dependencies:
- `mulle-thread`: Atomic operations and thread-local storage
- `mulle-c11`: Compatibility macros
