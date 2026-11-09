# Linux Kernel Notifier Chains — Raw Notifier Chain

We've covered:
1. Atomic Notifier Chain
2. Blocking Notifier Chain

Next is the **Raw Notifier Chain**. Let's understand the theory and APIs first, then we'll implement it on your Raspberry Pi 5.

## 1. What is a Raw Notifier Chain?

A Raw Notifier Chain is a Linux kernel event-notification mechanism that invokes registered callbacks **without providing internal locking**.

The notifier chain maintains a list of callback functions. When an event occurs, the registered callbacks are invoked synchronously.

The key difference is:

> The Raw Notifier Chain does not automatically synchronize callback-list access. The caller is responsible for ensuring that registration, unregistration, and callback traversal are properly synchronized.

### Example scenario

Imagine a kernel subsystem that detects a device state change.

```text
Device State Change
        |
        v
Raw Notifier Chain
        |
        +----> Callback A
        |
        +----> Callback B
        |
        +----> Callback C
```

All registered callbacks receive the notification.

Unlike Atomic and Blocking Notifier Chains, Raw Notifier Chains do not acquire an internal spinlock or read-write semaphore to protect the callback list.

## 2. Raw Notifier Chain APIs

### API 1: Declare the notifier head

```c
static RAW_NOTIFIER_HEAD(my_chain);
```

This declares and initializes a Raw Notifier Chain.

### API 2: Register a callback

```c
raw_notifier_chain_register(&my_chain, &nb);
```

Registers a callback in the chain.

### API 3: Invoke the notifier chain

```c
raw_notifier_call_chain(&my_chain, action, data);
```

Invokes registered callbacks synchronously.

### API 4: Unregister a callback

```c
raw_notifier_chain_unregister(&my_chain, &nb);
```

Removes a callback from the chain.

### Callback function

The callback signature remains the same:

```c
static int callback(struct notifier_block *nb,
                    unsigned long action,
                    void *data)
{
    /* Handle event */

    return NOTIFY_OK;
}
```

The callback can return the usual notifier return codes such as `NOTIFY_DONE`, `NOTIFY_OK`, and `NOTIFY_STOP`.

### API summary

| API | Purpose |
|---|---|
| `RAW_NOTIFIER_HEAD()` | Declare and initialize the raw notifier head |
| `raw_notifier_chain_register()` | Register callback |
| `raw_notifier_call_chain()` | Notify callbacks |
| `raw_notifier_chain_unregister()` | Unregister callback |

---

## 3. Why is there no internal lock?

Consider this scenario:

Thread A is executing the notifier chain:

```text
Thread A
   |
   v
Traverses callback list
   |
   v
Invokes Callback A
```

At the same time, Thread B unregisters Callback B:

```text
Thread B
   |
   v
Removes Callback B from list
```

Without proper synchronization, Thread A may be traversing the list while Thread B modifies it.

This can cause:
- Race conditions
- Corrupted list traversal
- Invalid callback references
- Kernel crashes

**Raw Notifier Chain does not prevent this automatically.**

The caller must ensure that the callback list is not modified concurrently with traversal.

### Important distinction

Raw Notifier Chain does not mean synchronization is unnecessary.

It means synchronization is **the responsibility of the caller**, rather than being provided internally by the notifier chain.

---

## 4. Can Raw Notifier callbacks sleep?

The Raw Notifier Chain itself does not impose the same sleep restriction as an Atomic Notifier Chain.

However, whether a callback can sleep depends on the context in which the chain is invoked.

| Calling Context | Can Callback Sleep? |
|---|---|
| Process context | Yes, if the caller's context allows sleeping |
| Interrupt context | No |
| While holding a spinlock | No |

For example, if a Raw Notifier Chain is invoked from process context, a callback may perform a sleepable operation, provided the caller's surrounding context permits it.

Remember: **Raw means no internal locking—not automatically atomic context.**

## 5. Compare all three notifier chains

| Feature | Atomic | Blocking | Raw |
|---|---|---|---|
| Internal lock | Spinlock | Read-write semaphore | No internal lock |
| Callbacks can sleep | No | Yes | Depends on caller context |
| Caller handles list synchronization | No | No | Yes |
| Can be called from interrupt context | Yes, with atomic-context restrictions | No | Depends on caller-provided synchronization and context |
| Synchronous callbacks | Yes | Yes | Yes |
| Callback priority | Yes | Yes | Yes |

### Interview takeaway

- **Atomic Notifier:** Internal spinlock; callbacks must not sleep.
- **Blocking Notifier:** Internal rwsem; callbacks can sleep.
- **Raw Notifier:** No internal locking; caller must provide synchronization.

---

## 6. When would you use a Raw Notifier Chain?

Use it when:
- The subsystem needs notifier-chain functionality without built-in locking.
- The caller already has a suitable synchronization mechanism.
- The callback list is managed in a controlled environment.
- You need to avoid the overhead or constraints of the other notifier types.

The trade-off is that the caller must carefully manage concurrency and callback-list lifetime.

---

## 7. Check your understanding

Before we implement the Raw Notifier Chain, answer these:

**Q1.** A Raw Notifier Chain is being called by Thread A. Thread B unregisters a callback at the same time. What is the potential problem?

**Q2.** Why might a developer choose a Raw Notifier Chain instead of a Blocking Notifier Chain?

**Q3.** Does Raw Notifier mean callbacks cannot sleep?

**Q4.** Who is responsible for protecting the Raw Notifier Chain's callback list?

Reply with your answers. Then we'll move to the hands-on implementation and review the code together.