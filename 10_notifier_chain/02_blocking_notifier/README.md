# Linux Kernel Blocking Notifier Chain

## 1. Introduction

A Blocking Notifier Chain is a Linux kernel mechanism used to notify multiple registered callback functions when an event occurs.

A subsystem acts as the **event producer**. It invokes the notifier chain with an event identifier and optional data.

Other subsystems register callback functions to act as **event consumers**.

The notifier chain invokes callbacks synchronously.

### Key characteristics

- Callbacks execute in the context of the caller.
- Callbacks may sleep.
- Callbacks may acquire mutexes.
- The chain uses an internal read-write semaphore.
- Multiple callers may execute callback traversal concurrently.
- Registration and unregistration require exclusive access to the notifier list.
- The chain must not be called from interrupt context or while holding a spinlock.

---

## 2. Notifier Chain Architecture

```text
                Event Producer
                     |
                     |
        blocking_notifier_call_chain()
                     |
                     v
            Blocking Notifier Head
                     |
           Internal rw_semaphore
                     |
          +----------+----------+
          |                     |
          v                     v
      Callback B            Callback A
      Priority 50           Priority 10
          |                     |
          v                     v
      Handles Event 2       Handles Event 1
```

The notifier head maintains a linked list of registered callbacks.

Each callback is represented by a `struct notifier_block`.

---

## 3. struct notifier_block

```c
struct notifier_block {
    int (*notifier_call)(struct notifier_block *nb,
                         unsigned long action,
                         void *data);

    struct notifier_block *next;
    int priority;
};
```

| Member | Purpose |
|---|---|
| `notifier_call` | Callback function invoked during event notification |
| `next` | Pointer to the next notifier block |
| `priority` | Determines callback execution order |

### Callback function

```c
int callback(struct notifier_block *nb,
             unsigned long action,
             void *data);
```

| Parameter | Meaning |
|---|---|
| `nb` | Pointer to the notifier block associated with the callback |
| `action` | Event identifier |
| `data` | Pointer to event-specific data |

The `data` parameter is a `void *`, allowing the event producer to pass different types of data.

The callback must interpret the data according to the event's definition.

---

## 4. Blocking Notifier APIs

### 4.1 Declare a Blocking Notifier Chain

```c
static BLOCKING_NOTIFIER_HEAD(my_chain);
```

This macro declares and initializes a blocking notifier head.

The notifier head maintains the callback list and the synchronization mechanism used by the blocking notifier chain.

### 4.2 Register a Callback

```c
blocking_notifier_chain_register(&my_chain, &nb);
```

Adds a notifier block to the chain.

Registration is a write-side operation because it modifies the callback list.

Higher numerical priority values execute earlier.

Example:

```c
static struct notifier_block nb_a = {
    .notifier_call = CallBackFuntionA,
    .priority = 10,
};

static struct notifier_block nb_b = {
    .notifier_call = CallBackFuntionB,
    .priority = 50,
};
```

Callback B has higher priority than Callback A.

Therefore, Callback B is invoked first.

However, **execution order does not guarantee that every callback prints a message**. A callback may return `NOTIFY_DONE` because it does not handle the current event.

### 4.3 Call the Notifier Chain

```c
blocking_notifier_call_chain(&my_chain, action, data);
```

Invokes registered callbacks synchronously.

The caller waits for the callback traversal to finish.

Example:

```c
int data1 = 100;
int data2 = 200;

blocking_notifier_call_chain(&my_chain, 1, &data1);
blocking_notifier_call_chain(&my_chain, 2, &data2);
```

The first call notifies callbacks about event 1.

The second call notifies callbacks about event 2.

### 4.4 Unregister a Callback

```c
blocking_notifier_chain_unregister(&my_chain, &nb);
```

Removes a notifier block from the chain.

A callback should be unregistered before its owning module is unloaded. Otherwise, the chain may retain a pointer to code that no longer exists.

---

## 5. Callback Return Values

Notifier callbacks use predefined return values.

| Return Value | Meaning |
|---|---|
| `NOTIFY_DONE` | Callback did not handle the event |
| `NOTIFY_OK` | Callback handled the event |
| `NOTIFY_STOP` | Stop further callback processing |
| `NOTIFY_BAD` | Indicates a failure condition and stops processing |

Example:

```c
static int CallBackFuntionA(struct notifier_block *nb,
                            unsigned long action,
                            void *data)
{
    if (action != 1)
        return NOTIFY_DONE;

    pr_info("Callback A handled event\n");

    return NOTIFY_OK;
}
```

When Callback A receives event 2, it returns `NOTIFY_DONE`.

This does not indicate that the notifier chain itself failed.

It means Callback A did not handle that event.

---

## 6. Understanding the Internal rw_semaphore

This is one of the most important concepts in Blocking Notifier Chains.

The blocking notifier chain uses a read-write semaphore internally.

### Why an rw_semaphore?

The notifier chain has two types of operations:

**Read-side operation**
- Traverses the callback list.
- Invokes callbacks.
- Does not modify the callback list.

**Write-side operation**
- Registers a callback.
- Unregisters a callback.
- Modifies the callback list.

Multiple readers can coexist, but a writer requires exclusive access.

### Internal synchronization model

Conceptually, the implementation uses:

```c
down_read(&rwsem);

/* Traverse callback list and invoke callbacks */

up_read(&rwsem);
```

For registration and unregistration, the implementation uses the write side:

```c
down_write(&rwsem);

/* Modify callback list */

up_write(&rwsem);
```

These are conceptual internal operations; application code should use the notifier APIs instead of manipulating the internal semaphore.

### Multiple readers

Suppose two kernel threads call the notifier chain concurrently:

```text
Thread 1                      Thread 2
   |                             |
   v                             v
 down_read()                  down_read()
   |                             |
   v                             v
 Callback traversal           Callback traversal
   |                             |
   v                             v
 up_read()                    up_read()
```

Both readers can hold read-side access concurrently.

This means callback execution from different notifier calls may overlap.

### Writer behavior

Suppose another thread attempts to unregister a callback:

```text
Thread 1: Notifier traversal
Thread 2: Notifier traversal
Thread 3: Unregister callback
```

The unregister operation requires exclusive write-side access.

It cannot modify the callback list while readers are traversing it.

This protects the notifier chain's callback list.

### Why not use a regular semaphore?

A regular semaphore provides a general mechanism for controlling access to a resource.

An `rw_semaphore` supports a more specific access pattern:

- Multiple readers can access the resource simultaneously.
- Writers require exclusive access.
- Read and write operations have different requirements.

A notifier chain is naturally suited to this pattern because callback invocation is a read-side operation, while registration and unregistration are write-side operations.

---

## 7. Callback Execution and Deadlock

The callback traversal holds the read-side semaphore while callbacks execute.

This has an important consequence.

### Do not unregister from inside a callback

Consider:

```text
Callback A
    |
    v
blocking_notifier_chain_unregister()
    |
    v
Attempts write-side access
```

The callback is already executing under the read-side lock.

Unregistration attempts to acquire write-side access to the same chain.

The operation can deadlock because the callback is waiting for exclusive access that cannot be obtained until the current read-side operation finishes.

Therefore:

**Do not register or unregister callbacks from inside the callback of the same blocking notifier chain.**

Perform registration and unregistration from an appropriate external context.

---

## 8. Protecting Shared Data Using a Mutex

The notifier chain's internal read-write semaphore protects the callback list.

It does not automatically protect variables modified by callbacks.

Consider:

```c
static DEFINE_MUTEX(lock);
static int share_data = 1;
```

Both Callback A and Callback B modify `share_data`.

Without synchronization:

```c
share_data++;
```

Concurrent execution could cause a race.

### Protect the shared variable

```c
mutex_lock(&lock);

share_data++;

mutex_unlock(&lock);
```

Both callbacks must use the same mutex when modifying the same shared variable.

### Important distinction

| Synchronization Mechanism | What It Protects |
|---|---|
| Internal `rw_semaphore` | Notifier callback list |
| Mutex in the callback | Application shared data such as `share_data` |

These mechanisms serve different purposes.

### Does the current experiment demonstrate mutex contention?

No.

The current implementation calls the notifier chain sequentially from module initialization:

```c
blocking_notifier_call_chain(&my_chain, 1, &data1);
blocking_notifier_call_chain(&my_chain, 2, &data2);
```

The callbacks execute one after another.

The mutex protects `share_data`, but the experiment does not create concurrent access to that variable.

To demonstrate actual mutex contention, multiple kernel threads would need to modify the shared variable concurrently.

---

## 9. Interrupt Context and Workqueues

Blocking Notifier Chains cannot be called directly from interrupt context.

### Why?

Interrupt handlers must not perform operations that may sleep.

A Blocking Notifier Chain uses sleepable synchronization, and its callbacks may sleep.

Therefore, directly calling the blocking notifier chain from an interrupt handler is unsafe.

### Correct approach: defer the work

```text
Hardware Interrupt
       |
       v
Interrupt Handler
       |
       v
Schedule Work
       |
       v
Workqueue Worker
       |
       v
blocking_notifier_call_chain()
       |
       v
Callbacks Execute
```

The interrupt handler schedules work.

The workqueue worker executes in process context, where sleepable operations are allowed.

The blocking notifier chain can then be called from the worker.

---

## 10. Registration Error Handling

Registration can fail.

The module should handle registration errors and clean up callbacks that were successfully registered earlier.

Example flow:

```text
Register Callback A
       |
       v
Registration Successful?
       |
       +---- No ----> Return Error
       |
       v
Register Callback B
       |
       v
Registration Successful?
       |
       +---- No ----> Unregister Callback A
       |
       v
Initialization Successful
```

If Callback B registration fails after Callback A registration succeeds, Callback A must be unregistered.

This prevents a partially initialized notifier chain from remaining registered.

---

## 11. Hands-on Implementation

### Experiment configuration

| Parameter | Value |
|---|---|
| Notifier type | Blocking |
| Callback A priority | 10 |
| Callback B priority | 50 |
| Event 1 data | 100 |
| Event 2 data | 200 |
| Initial `share_data` | 1 |

### Callback A

Callback A handles event 1.

For other events, it returns `NOTIFY_DONE`.

### Callback B

Callback B handles event 2.

For other events, it returns `NOTIFY_DONE`.

### Execution

The notifier chain is called twice:

```c
blocking_notifier_call_chain(&my_chain, 1, &data1);
blocking_notifier_call_chain(&my_chain, 2, &data2);
```

Because Callback B has priority 50, it is invoked before Callback A.

For event 1:

```text
Callback B -> NOTIFY_DONE
Callback A -> Handles event 1
```

For event 2:

```text
Callback B -> Handles event 2
Callback A -> NOTIFY_DONE
```

### Shared data result

Initial value:

```text
share_data = 1
```

Callback A increments it once.

Callback B increments it once.

Expected result:

```text
share_data = 3
```

The value is printed during module removal after the callbacks have been unregistered.

---

## 12. Blocking Notifier vs Atomic Notifier

| Feature | Blocking Notifier | Atomic Notifier |
|---|---|---|
| Internal synchronization | Read-write semaphore | Spinlock |
| Callback may sleep | Yes | No |
| Callback may use mutex | Yes | No |
| Can be called from interrupt context | No | Yes, subject to atomic-context constraints |
| Synchronous execution | Yes | Yes |
| Typical use | Process context | Atomic context |

### Core difference

A Blocking Notifier Chain is designed for sleepable contexts.

An Atomic Notifier Chain is designed for contexts where sleeping is not allowed.

---

## 13. Interview Questions

### Q1. What is a Blocking Notifier Chain?

A synchronous kernel event-notification mechanism that invokes registered callbacks in sleepable context.

### Q2. Why does a Blocking Notifier Chain use an rw_semaphore?

Callback traversal is a read-side operation. Registration and unregistration are write-side operations. Multiple readers can coexist, while writers require exclusive access.

### Q3. Can multiple threads call a Blocking Notifier Chain simultaneously?

Yes. Multiple callers can acquire read-side access concurrently. Their callbacks may execute concurrently, so shared callback data may require separate synchronization.

### Q4. Does the internal rw_semaphore protect callback shared data?

No. It protects the notifier callback list. Shared data modified by callbacks requires its own synchronization.

### Q5. Can a Blocking Notifier Chain be called from an interrupt handler?

No. It uses sleepable synchronization and callbacks may sleep.

### Q6. How can a blocking notifier be triggered from an interrupt?

Schedule work from the interrupt handler and call the blocking notifier from the workqueue worker.

### Q7. Why can unregistering from inside a callback cause deadlock?

The callback is executing while holding read-side access. Unregistration requires write-side access, which conflicts with the current read-side operation.

### Q8. What is the difference between `NOTIFY_DONE` and `NOTIFY_OK`?

`NOTIFY_DONE` indicates that the callback did not handle the event. `NOTIFY_OK` indicates that it handled the event.

### Q9. Does higher numerical priority execute first?

Yes. Higher numerical priority values execute earlier in the notifier chain.

### Q10. Why is a mutex used in the experiment?

To protect `share_data` from concurrent modifications by callbacks.

### Q11. Does the current experiment prove mutex contention?

No. The notifier calls are sequential. Concurrent kernel threads are needed to demonstrate contention.

### Q12. Why must callbacks be unregistered before module removal?

To prevent the notifier chain from retaining a callback pointer into unloaded module memory.

---

## 14. Conclusion

Blocking Notifier Chains provide synchronous event notification when callbacks require sleepable operations.

The key concepts learned in this experiment are:

- Callback registration and unregistration
- Event-based callback execution
- Callback priority
- `NOTIFY_DONE` and `NOTIFY_OK`
- Internal read-write semaphore synchronization
- Concurrent read-side callback traversal
- Exclusive write-side registration and unregistration
- Deadlock caused by unregistering from inside a callback
- Mutex protection for callback shared data
- Workqueue-based deferred notification from interrupt context
- Registration failure handling and cleanup

### Important APIs

```c
BLOCKING_NOTIFIER_HEAD()

blocking_notifier_chain_register()

blocking_notifier_call_chain()

blocking_notifier_chain_unregister()
```