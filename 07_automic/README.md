Here is the complete `06_atomic_variables/README.md` in a single Markdown code block, following the same structure as your previous Linux kernel synchronization notes.

```markdown
# 06 - Linux Kernel Atomic Variables (`atomic_t`)

## 1. Objective

Understand how atomic variables work in the Linux kernel and how they help prevent race conditions when multiple kernel threads access shared data.

In this experiment, multiple kernel threads increment a shared counter.

- Normal variables can cause race conditions.
- Atomic operations make individual operations indivisible.
- Atomic variables are useful for simple counters and flags.
- Atomic operations do not automatically protect a sequence of multiple operations.

---

## 2. What Is an Atomic Variable?

An atomic variable is a variable that can be accessed or modified using operations that execute indivisibly.

Consider two kernel threads incrementing the same normal variable:

```c
int counter = 0;

counter++;
```

At a conceptual level, `counter++` involves three steps:

1. Read the current value.
2. Add one.
3. Write the updated value.

Suppose two threads increment `counter` simultaneously.

Initially:

```text
counter = 0
```

The following sequence can occur:

```text
Thread 1 reads counter = 0
Thread 2 reads counter = 0

Thread 1 calculates 0 + 1
Thread 2 calculates 0 + 1

Thread 1 writes counter = 1
Thread 2 writes counter = 1
```

Expected result:

```text
counter = 2
```

Actual result:

```text
counter = 1
```

One increment is lost. This is a race condition.

**Important:** A normal `counter++` operation is not guaranteed to be atomic.

---

## 3. How Does `atomic_t` Solve This Problem?

The Linux kernel provides `atomic_t` for atomic integer operations.

Header file:

```c
#include <linux/atomic.h>
```

Declare an atomic variable:

```c
atomic_t counter;
```

Initialize it:

```c
atomic_set(&counter, 0);
```

Increment it:

```c
atomic_inc(&counter);
```

Read its value:

```c
int value = atomic_read(&counter);
```

If two kernel threads execute:

```c
atomic_inc(&counter);
```

once each, the final value will be:

```text
counter = 2
```

This assumes there are no other modifications to the counter.

Each `atomic_inc()` operation updates the counter atomically, preventing the lost-update problem associated with an ordinary increment.

### Important distinction

Atomic operations protect individual operations.

They do not automatically make an entire sequence of operations atomic.

We will examine this distinction later.

---

## 4. Important Atomic APIs

### 4.1 `atomic_set()`

Sets an atomic variable to a specified value.

Function:

```c
atomic_set(&counter, 10);
```

Result:

```text
counter = 10
```

### 4.2 `atomic_read()`

Reads the current value of an atomic variable.

Function:

```c
int value = atomic_read(&counter);
```

If:

```text
counter = 10
```

then:

```text
value = 10
```

### 4.3 `atomic_inc()`

Increments the atomic variable by one.

Function:

```c
atomic_inc(&counter);
```

Example:

```text
Before: counter = 10
After:  counter = 11
```

### 4.4 `atomic_dec()`

Decrements the atomic variable by one.

Function:

```c
atomic_dec(&counter);
```

Example:

```text
Before: counter = 10
After:  counter = 9
```

### 4.5 `atomic_add()`

Adds a specified value to the atomic variable.

Function:

```c
atomic_add(5, &counter);
```

Example:

```text
Before: counter = 10
After:  counter = 15
```

### 4.6 `atomic_sub()`

Subtracts a specified value from the atomic variable.

Function:

```c
atomic_sub(3, &counter);
```

Example:

```text
Before: counter = 10
After:  counter = 7
```

### 4.7 `atomic_inc_return()`

Increments the variable and returns the updated value.

Function:

```c
int value = atomic_inc_return(&counter);
```

Example:

```text
Before: counter = 10

Operation:
    atomic_inc_return(&counter);

After:
    counter = 11
    value = 11
```

The returned value is the value after incrementing.

### 4.8 `atomic_dec_and_test()`

Decrements the variable and checks whether the new value is zero.

Function:

```c
int result = atomic_dec_and_test(&counter);
```

Example 1:

```text
Before:
    counter = 2

Operation:
    atomic_dec_and_test(&counter);

After:
    counter = 1
    result = 0
```

The new value is not zero.

Example 2:

```text
Before:
    counter = 1

Operation:
    atomic_dec_and_test(&counter);

After:
    counter = 0
    result = 1
```

The new value is zero.

This API is useful when tracking references or counting down to a final event.

### 4.9 `atomic_cmpxchg()`

The name means atomic compare-and-exchange.

Function:

```c
int old = atomic_cmpxchg(&counter, old_value, new_value);
```

The operation works as follows:

1. Read the current value.
2. Compare it with `old_value`.
3. If they match, replace the current value with `new_value`.
4. Return the value that was present before the operation.

The comparison and conditional replacement happen atomically.

#### Example 1: Comparison succeeds

Initially:

```text
counter = 10
```

Execute:

```c
int old = atomic_cmpxchg(&counter, 10, 20);
```

Result:

```text
counter = 20
old = 10
```

The original value matched `10`, so the replacement occurred.

#### Example 2: Comparison fails

Initially:

```text
counter = 15
```

Execute:

```c
int old = atomic_cmpxchg(&counter, 10, 20);
```

Result:

```text
counter = 15
old = 15
```

The original value did not match `10`, so no replacement occurred.

**Important:** `atomic_cmpxchg()` returns the previous value, not a Boolean success/failure value.

---

## 5. Practical Example: Claiming a Resource

Suppose a device can be claimed by only one kernel thread at a time.

We use these values:

```text
0 = Resource available
1 = Resource already claimed
```

Declare and initialize the variable:

```c
atomic_t device_status;

atomic_set(&device_status, 0);
```

A thread attempts to claim the resource:

```c
int old;

old = atomic_cmpxchg(&device_status, 0, 1);

if (old == 0) {
    pr_info("Resource claimed successfully\n");
} else {
    pr_info("Resource is already claimed\n");
}
```

### What happens if two threads try simultaneously?

Initially:

```text
device_status = 0
```

Thread 1 executes:

```c
atomic_cmpxchg(&device_status, 0, 1);
```

The value changes from `0` to `1`.

Thread 1 receives:

```text
old = 0
```

Therefore, Thread 1 claims the resource.

Thread 2 then executes the same operation.

The current value is already `1`, so the comparison fails.

Thread 2 receives:

```text
old = 1
```

Therefore, Thread 2 cannot claim the resource.

Only one thread can successfully change the state from `0` to `1`.

To make the resource available again, the owner must release it appropriately:

```c
atomic_set(&device_status, 0);
```

In a real driver, the claim and release operations must follow the device's lifecycle and ownership rules.

**Important:** Atomic compare-and-exchange can implement a simple claim operation, but it does not automatically provide a complete resource-management mechanism.

---

## 6. Important Limitation: Multiple Shared Variables

Consider two atomic variables:

```c
atomic_t packet_count;
atomic_t error_count;
```

Suppose every transaction must increment both variables.

```c
atomic_inc(&packet_count);
atomic_inc(&error_count);
```

Each increment is individually atomic.

However, the two increments are not one indivisible operation.

### Example

Initially:

```text
packet_count = 0
error_count  = 0
```

Thread 1 starts processing a transaction.

First, it executes:

```c
atomic_inc(&packet_count);
```

The state becomes:

```text
packet_count = 1
error_count  = 0
```

Before Thread 1 increments `error_count`, Thread 2 reads both variables.

Thread 2 observes:

```text
packet_count = 1
error_count  = 0
```

Thread 1 then executes:

```c
atomic_inc(&error_count);
```

The final state becomes:

```text
packet_count = 1
error_count  = 1
```

The final values are correct, but Thread 2 observed an intermediate state.

If the application requires both values to be read consistently, independent atomic operations are insufficient.

### Solution: Protect the Related Operations With a Lock

For example:

```c
static DEFINE_SPINLOCK(stats_lock);

static int packet_count;
static int error_count;
```

Update both variables while holding the same lock:

```c
spin_lock(&stats_lock);

packet_count++;
error_count++;

spin_unlock(&stats_lock);
```

A reader that needs a consistent snapshot must use the same lock:

```c
int packets;
int errors;

spin_lock(&stats_lock);

packets = packet_count;
errors = error_count;

spin_unlock(&stats_lock);
```

The reader cannot enter the protected section between the two updates.

**Important:** This example assumes the protected sections execute in a context where taking this spinlock is valid. Do not sleep while holding the spinlock. If the code must sleep, consider a mutex instead.

When all accesses are protected by the same lock, ordinary integer variables are generally sufficient; atomic variables are not necessary for those protected accesses.

---

## 7. Atomic Variables vs Mutex vs Spinlock

| Feature | Atomic Variable | Mutex | Spinlock |
|---|---|---|---|
| Main purpose | Atomic operations on a value | Mutual exclusion | Mutual exclusion |
| Protects a single increment | Yes, with an appropriate atomic API | Yes, when locked | Yes, when locked |
| Protects multiple related operations as one critical section | No, not by itself | Yes | Yes |
| Can the acquisition sleep? | No lock acquisition is involved in ordinary atomic operations | Yes | No |
| Suitable for hard IRQ context | Appropriate atomic operations may be used | No | Yes, with correct IRQ-context handling |
| Protects multiple variables together | No, not by itself | Yes | Yes |
| Best use | Simple counters, flags, and atomic state transitions | Longer operations in sleepable contexts | Short, non-sleeping critical sections |

Choose the synchronization mechanism according to the operation and the execution context.

Do not replace every lock with an atomic variable simply because atomic operations are available.

---

## 8. Common Mistakes

### Mistake 1: Using a normal increment for shared data

```c
counter++;
```

This is not guaranteed to be atomic.

For a simple shared counter, consider:

```c
atomic_inc(&counter);
```

### Mistake 2: Assuming two atomic operations form one transaction

```c
atomic_inc(&packet_count);
atomic_inc(&error_count);
```

Each increment is atomic, but the pair is not atomic.

Use a suitable lock if the pair must be updated and observed consistently.

### Mistake 3: Treating `atomic_cmpxchg()` as a Boolean operation

Incorrect assumption:

```text
Return value 1 means success.
Return value 0 means failure.
```

Actual behavior:

```text
The function returns the value that was present before the operation.
```

For a `0`-to-`1` claim:

```c
if (atomic_cmpxchg(&device_status, 0, 1) == 0) {
    /* Successfully claimed the resource. */
}
```

### Mistake 4: Using atomic variables when a lock is required

Atomic variables do not automatically protect complex operations involving several variables, pointers, or data structures.

Use a lock when the complete critical section must be protected.

### Mistake 5: Assuming atomic operations solve every concurrency problem

Atomic operations help prevent races on the value being modified by the atomic operation.

They do not automatically guarantee that related data is consistent or that every memory-ordering requirement is satisfied.

Use the appropriate synchronization and memory-ordering mechanisms for the complete design.

---

## 9. When Should You Use Atomic Variables?

Atomic variables are useful when:

- Maintaining a shared counter.
- Incrementing or decrementing a reference count.
- Tracking the number of active operations.
- Updating a simple state variable.
- Performing a conditional state transition with `atomic_cmpxchg()`.
- Detecting when a counter reaches zero with `atomic_dec_and_test()`.

A mutex or spinlock may be more appropriate when:

- Several variables must be updated consistently.
- A data structure requires multiple related operations.
- A reader must obtain a consistent snapshot.
- A complete critical section must be protected.
- The operation cannot be expressed safely using the available atomic operations.

---

## 10. Interview Questions

### Q1. Why is `counter++` not necessarily atomic?

Because it involves reading, modifying, and writing the value. Concurrent threads can interfere with these steps and cause lost updates.

### Q2. What is `atomic_t`?

It is a Linux kernel type used for atomic integer operations.

### Q3. What does `atomic_inc()` do?

It atomically increments an atomic variable by one.

### Q4. What does `atomic_dec_and_test()` return?

It decrements the variable and returns a nonzero value if the new value is zero.

### Q5. What does `atomic_cmpxchg()` return?

It returns the value that was present before the operation, whether or not the comparison succeeds.

### Q6. Can atomic variables protect two variables as one unit?

Not by themselves. Each atomic operation is separate. Use a suitable synchronization mechanism when the two variables must be updated or read consistently.

### Q7. What is the difference between an atomic variable and a spinlock?

An atomic variable provides specific atomic operations on a value. A spinlock protects a critical section that can contain multiple operations.

### Q8. Can an atomic operation be used in interrupt context?

Appropriate atomic operations can be used in interrupt context, provided the specific API and its requirements are suitable for that context. A blocking lock such as a mutex must not be used in a hard IRQ handler.

### Q9. Does `atomic_read()` make a sequence of reads atomic?

No. Reading one atomic variable does not make a sequence of reads across several variables a consistent snapshot.

### Q10. Should every shared integer use `atomic_t`?

No. Choose the synchronization mechanism according to the operation, the required consistency, and the execution context.

---

## 11. Summary

Remember these key points:

1. A normal `counter++` is not guaranteed to be atomic.
2. `atomic_t` provides atomic operations on an integer.
3. `atomic_inc()` and `atomic_dec()` prevent lost updates to that counter when used appropriately.
4. `atomic_cmpxchg()` compares and conditionally replaces a value atomically.
5. `atomic_dec_and_test()` checks whether the decremented value becomes zero.
6. Separate atomic operations do not form one atomic transaction.
7. Use a lock when several related operations must execute as one protected critical section.
8. Select the synchronization mechanism based on the execution context and required behavior.

**Next topic:** Learn how to use `WARN_ON()`, `WARN_ON_ONCE()`, `BUG_ON()`, and stack-trace facilities to diagnose problems in Linux kernel code.
```
