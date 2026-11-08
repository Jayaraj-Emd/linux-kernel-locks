Here is a README for your **Atomic Notifier Chain** experiment. It covers the concepts, APIs, callback execution flow, priority, event handling, and hands-on results.

# Atomic Notifier Chain in Linux Kernel

## 1. Overview

An Atomic Notifier Chain is a Linux kernel mechanism used to notify multiple registered callback functions about an event.

It is designed for situations where notification may occur in atomic context, such as interrupt context. The callbacks execute synchronously in the context of the caller.

The notifier chain allows a producer to notify multiple consumers without directly depending on their implementations.

### Objectives

- Understand the Atomic Notifier Chain mechanism.
- Learn how to declare an atomic notifier head.
- Register and unregister notifier callbacks.
- Understand callback priority.
- Understand the purpose of `action` and `data`.
- Observe callback execution order.
- Understand callback execution restrictions in atomic context.

---

## 2. Architecture

```text
                 Event Producer
                       |
                       |
                       v
          atomic_notifier_call_chain()
                       |
                       v
              Atomic Notifier Head
                       |
                       v
              Notifier Linked List
                       |
          +------------+------------+
          |            |            |
          v            v            v
      Callback B   Callback A    Callback C
      Priority 50  Priority 10   Priority 5
```

The notifier head maintains a linked list of registered notifier blocks.

Each notifier block contains:

- A callback function.
- A pointer to the next notifier block.
- A priority value.

Callbacks are executed according to their priority. A higher numerical priority generally executes earlier.

---

## 3. `struct notifier_block`

The notifier block represents an individual callback registration.

```c
struct notifier_block {
    int (*notifier_call)(struct notifier_block *nb,
                         unsigned long action,
                         void *data);

    struct notifier_block *next;
    int priority;
};
```

### Members

| Member | Description |
|---|---|
| `notifier_call` | Callback function invoked when an event occurs |
| `next` | Pointer to the next notifier block in the chain |
| `priority` | Determines callback execution order |

The `next` field is managed internally by the notifier chain registration mechanism.

---

## 4. Atomic Notifier Head

### API

```c
ATOMIC_NOTIFIER_HEAD(my_chain);
```

### Purpose

Declares and initializes an atomic notifier chain.

The atomic notifier head contains:

- A spinlock to protect the notifier chain's linked-list structure.
- A pointer to the first registered notifier block.

The internal locking and linked-list initialization are handled by the macro.

---

## 5. APIs

### 5.1 `atomic_notifier_chain_register()`

Registers a callback with the atomic notifier chain.

```c
int atomic_notifier_chain_register(
    struct atomic_notifier_head *nh,
    struct notifier_block *nb);
```

**Arguments:**

| Argument | Description |
|---|---|
| `nh` | Pointer to the atomic notifier head |
| `nb` | Pointer to the notifier block being registered |

Example:

```c
atomic_notifier_chain_register(&my_chain, &nb_a);
```

The kernel inserts the notifier block into the chain according to its priority.

**Return value:**
- `0` on successful registration.
- Negative error code on failure.

### 5.2 `atomic_notifier_call_chain()`

Invokes the registered callbacks.

```c
int atomic_notifier_call_chain(
    struct atomic_notifier_head *nh,
    unsigned long val,
    void *v);
```

**Arguments:**

| Argument | Description |
|---|---|
| `nh` | Pointer to the atomic notifier head |
| `val` | Event identifier |
| `v` | Pointer to event-specific data |

Example:

```c
int event_data = 100;

atomic_notifier_call_chain(&my_chain, 1, &event_data);
```

Here:

- `val = 1` represents the event identifier.
- `v = &event_data` provides the address of the event data.

The notifier chain invokes registered callbacks synchronously.

### 5.3 `atomic_notifier_chain_unregister()`

Removes a previously registered callback.

```c
int atomic_notifier_chain_unregister(
    struct atomic_notifier_head *nh,
    struct notifier_block *nb);
```

Example:

```c
atomic_notifier_chain_unregister(&my_chain, &nb_a);
```

Unregistration is important when removing a module because callbacks belonging to an unloaded module must not remain registered.

---

## 6. Callback Function

### Callback prototype

```c
int callback_function(struct notifier_block *nb,
                      unsigned long action,
                      void *data);
```

### Callback arguments

| Argument | Description |
|---|---|
| `nb` | Pointer to the notifier block associated with the callback |
| `action` | Event identifier provided by the producer |
| `data` | Pointer to event-specific information provided by the producer |

### Event filtering

A callback can decide whether it is interested in a particular event.

Example:

```c
if (action != 1)
    return NOTIFY_DONE;
```

If the callback is not interested in the event, it returns `NOTIFY_DONE`.

If the callback handles the event, it can return `NOTIFY_OK`.

---

## 7. Callback Return Values

Notifier callbacks use predefined return values.

| Return Value | Meaning |
|---|---|
| `NOTIFY_DONE` | Callback did not handle the event |
| `NOTIFY_OK` | Callback successfully handled the event |
| `NOTIFY_STOP` | Callback handled the event and requests that traversal stop |
| `NOTIFY_BAD` | Callback indicates a failure and requests that traversal stop |

The notifier chain stops traversing callbacks when a callback returns a value containing `NOTIFY_STOP_MASK`.

`NOTIFY_DONE` does not stop the chain.

---

## 8. Callback Priority

Consider two notifier blocks:

```c
static struct notifier_block nb_a = {
    .notifier_call = callback_function_a,
    .priority = 10,
};

static struct notifier_block nb_b = {
    .notifier_call = callback_function_b,
    .priority = 50,
};
```

The execution order is:

```text
Callback B → Priority 50
Callback A → Priority 10
```

Higher numerical priority generally means earlier execution.

Priority is useful when callbacks have dependencies on the order of notification.

---

## 9. Atomic Context Restrictions

Atomic notifier callbacks may execute in interrupt context or other atomic contexts.

Therefore, callbacks must not perform operations that can sleep.

### Operations to avoid

- Acquiring a mutex that may block.
- Calling functions that sleep.
- Performing lengthy operations.
- Waiting for a completion.

### Deferred execution

If a callback needs to perform a sleepable operation, it can schedule work using a workqueue.

```text
Atomic Notifier Callback
          |
          v
Schedule Work
          |
          v
Return Immediately
          |
          v
Workqueue Worker
          |
          v
Acquire Mutex
          |
          v
Perform Sleepable Operation
```

The worker executes in process context, where sleeping is permitted.

---

## 10. Hands-on Experiment

### Experiment Description

Created a Linux kernel module implementing an Atomic Notifier Chain.

The module contains:

- One atomic notifier head.
- Two notifier blocks.
- Two callback functions.
- Different callback priorities.
- Two event identifiers.
- Event-specific data.
- Registration and unregistration logic.

### Callback Configuration

| Callback | Priority | Event Handled |
|---|---:|---:|
| Callback A | 10 | Event 1 |
| Callback B | 50 | Event 2 |

### Event 1

```c
int data1 = 100;

atomic_notifier_call_chain(&atomic, 1, &data1);
```

Callback B executes first because of its higher priority, but it returns `NOTIFY_DONE` because it does not handle Event 1.

Callback A handles Event 1.

### Event 2

```c
int data2 = 200;

atomic_notifier_call_chain(&atomic, 2, &data2);
```

Callback B handles Event 2.

Callback A returns `NOTIFY_DONE` because it does not handle Event 2.

### Expected Output

```text
CallBackFuntionA: Event = 1, data = 100
CallBackFuntionB: Event = 2, data = 200
```

---

## 11. Module Lifecycle

```text
Module Initialization
        |
        v
Declare Atomic Notifier Head
        |
        v
Register Callback A
        |
        v
Register Callback B
        |
        v
Call Notifier Chain
        |
        v
Callbacks Execute
        |
        v
Module Cleanup
        |
        v
Unregister Callback A
        |
        v
Unregister Callback B
```

Registration errors must be handled correctly. If registering Callback B fails after Callback A has already been registered, Callback A should be unregistered before returning the initialization error.

---

## 12. Interview Questions

### Q1. What is an Atomic Notifier Chain?

An Atomic Notifier Chain is a Linux kernel mechanism used to synchronously notify multiple registered callback functions from atomic execution contexts. It maintains a linked list of notifier blocks containing callback functions and priorities. Since callbacks may execute in interrupt context, they must not perform operations that can sleep.

### Q2. Why is a notifier chain used?

It decouples the event producer from the event consumers. The producer only needs to notify the chain, while registered callbacks independently handle the event.

### Q3. What is the purpose of `struct notifier_block`?

It represents an individual callback registration. It contains the callback function pointer, a pointer to the next notifier block, and a priority value.

### Q4. What is the difference between `action` and `data`?

`action` identifies the event being reported. `data` provides additional event-specific information through a pointer.

### Q5. Why must an Atomic Notifier callback not sleep?

Atomic notifier callbacks may execute in interrupt or other non-sleepable contexts. Sleeping can violate execution-context constraints and cause kernel problems.

### Q6. How can a callback perform sleepable operations?

The callback can schedule work using a workqueue. The worker executes in process context, where it can acquire a mutex and perform operations that may sleep.

### Q7. Does `NOTIFY_DONE` stop callback traversal?

No. `NOTIFY_DONE` indicates that the callback did not handle the event. Callback traversal stops when a return value contains `NOTIFY_STOP_MASK`.

### Q8. How does callback priority work?

Callbacks are ordered by priority. A higher numerical priority generally executes earlier than a lower numerical priority.

---

## 13. Key Takeaways

- Atomic Notifier Chains provide synchronous event notification.
- The producer is decoupled from registered consumers.
- Each callback is represented by a `struct notifier_block`.
- Callback priority determines the execution order.
- `action` identifies the event.
- `data` provides event-specific information.
- Atomic notifier callbacks must not sleep.
- Workqueues can be used to defer sleepable operations.
- Callbacks must be unregistered before their owning module is unloaded.

One technical note: the README's expected output reflects your current callback filtering logic—only Callback A prints for Event 1, and only Callback B prints for Event 2.