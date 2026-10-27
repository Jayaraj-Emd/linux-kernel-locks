# 05 - Linux Kernel Reader-Writer Spinlock (`rwlock_t`)

## 1. Objective

Understand how Linux kernel reader-writer spinlocks work and how they synchronize multiple kernel threads accessing shared data.

In this experiment, multiple reader threads and one writer thread access a shared variable.

- Multiple readers can hold the read lock simultaneously.
- A writer requires exclusive access.
- A writer must wait while readers hold the read lock.
- Readers must wait while a writer holds the write lock.
- Waiting tasks spin instead of sleeping.

This experiment demonstrates reader-writer locking, exclusive access, and synchronization in Linux kernel drivers.

## 2. What Is `rwlock_t`?

`rwlock_t` is a Linux kernel synchronization primitive that supports shared read access and exclusive write access.

It is implemented as a reader-writer spinlock.

There are two types of lock acquisition:

### 2.1 Read Lock

Multiple threads can acquire the read lock simultaneously.

```c
read_lock(&lock);

/* Read shared data */

read_unlock(&lock);
```

Readers can access the protected data concurrently as long as no writer holds the lock.

### 2.2 Write Lock

Only one writer can hold the write lock at a time.

```c
write_lock(&lock);

/* Modify shared data */

write_unlock(&lock);
```

A writer requires exclusive access. It cannot acquire the write lock while readers or another writer hold the lock.

**Important:** Unlike `rw_semaphore`, `rwlock_t` does not allow the calling task to sleep while holding the lock.

## 3. Why Do We Need `rwlock_t`?

Consider a Linux driver maintaining a shared variable.

```c
unsigned int packet_count;
```

Multiple threads need to read `packet_count`, while another thread updates it.

If a normal spinlock is used, only one thread can hold the lock at a time, even if all threads are only reading the data.

An `rwlock_t` allows multiple readers to access the protected data simultaneously.

When a writer needs to modify the data, it obtains exclusive access.

### 3.1 Multiple Readers

```text
             Shared Variable
                    |
           +--------+--------+
           |                 |
        Reader 1          Reader 2
           |                 |
           +--------+--------+
                    |
          Both hold read locks
             simultaneously
```

### 3.2 Writer Waiting for a Reader

```text
Reader 1                         Writer 1
   |                                |
   | read_lock(&lock)               |
   | Acquires read lock             |
   |                                |
   |                                | write_lock(&lock)
   |                                | Spins and waits
   |                                |
   | Reads shared data              |
   |                                |
   | read_unlock(&lock)             |
   |                                |
   |                                | Acquires write lock
   |                                |
   |                                | Modifies shared data
   |                                |
   |                                | write_unlock(&lock)
```

The writer cannot acquire the lock until the existing readers release their read locks.

## 4. Important APIs

Include the following header:

```c
#include <linux/spinlock.h>
```

### 4.1 `rwlock_init()`

Initializes the reader-writer spinlock.

```c
rwlock_t lock;

rwlock_init(&lock);
```

Initialize the lock before using it.

### 4.2 `read_lock()`

Acquires the read lock.

```c
read_lock(&lock);

/* Read shared data */

read_unlock(&lock);
```

Multiple readers can hold the read lock simultaneously.

A writer cannot acquire the write lock while readers hold their locks.

The critical section must not sleep.

### 4.3 `read_unlock()`

Releases the read lock.

```c
read_unlock(&lock);
```

Call this after finishing the protected read operation.

If other readers still hold the read lock, a writer cannot acquire the write lock yet.

### 4.4 `write_lock()`

Acquires the write lock.

```c
write_lock(&lock);

/* Modify shared data */

write_unlock(&lock);
```

The writer requires exclusive access.

If readers or another writer hold the lock, the calling CPU spins while waiting.

The critical section must not sleep.

### 4.5 `write_unlock()`

Releases the write lock.

```c
write_unlock(&lock);
```

Call this after completing the protected write operation.

Waiting readers or writers may then be able to acquire the lock.

## 5. IRQ-Safe Locking APIs

If the same lock is shared between process context and a hard IRQ handler, appropriate interrupt-safe locking may be required.

### 5.1 `read_lock_irqsave()`

```c
unsigned long flags;

read_lock_irqsave(&lock, flags);

/* Read shared data */

read_unlock_irqrestore(&lock, flags);
```

The acquisition disables local interrupts and saves their previous state.

The unlock operation restores the saved interrupt state.

### 5.2 `write_lock_irqsave()`

```c
unsigned long flags;

write_lock_irqsave(&lock, flags);

/* Modify shared data */

write_unlock_irqrestore(&lock, flags);
```

This protects the critical section against conflicting access from a local hard IRQ handler using the same lock, provided the handler uses a compatible locking strategy.

### Important Rules

- Do not sleep while holding an `rwlock_t`.
- Do not call `msleep()` while holding the lock.
- Do not use blocking mutex or semaphore acquisition inside a hard IRQ handler.
- Use interrupt-safe locking when required by the execution contexts sharing the lock.
- Keep the critical section short.

Disabling local interrupts does not make sleeping while holding a spinlock safe.

## 6. Experiment

### 6.1 Directory Structure

```text
05_rwlock/
├── Makefile
├── rwlock.c
└── README.md
```

### 6.2 Experiment Description

Create a kernel module with:

- Two reader kernel threads.
- One writer kernel thread.
- A shared integer variable.
- One `rwlock_t` lock.

The readers access the shared variable, and the writer modifies it.

### Reader Thread

1. Acquires the read lock using `read_lock()`.
2. Reads the shared variable.
3. Prints the value using `pr_info()`.
4. Releases the read lock using `read_unlock()`.

### Writer Thread

1. Attempts to acquire the write lock using `write_lock()`.
2. Waits if readers hold the lock.
3. Updates the shared variable.
4. Prints the updated value.
5. Releases the write lock using `write_unlock()`.

### 6.3 Example Shared Variable

```c
int shared_value = 0;
rwlock_t lock;
```

Reader code:

```c
int value;

read_lock(&lock);

value = shared_value;
pr_info("Reader: shared_value = %d\n", value);

read_unlock(&lock);
```

Writer code:

```c
write_lock(&lock);

shared_value++;
pr_info("Writer: shared_value = %d\n", shared_value);

write_unlock(&lock);
```

These snippets illustrate the locking operations. A complete module must also initialize the lock and correctly manage the lifecycle of its kernel threads.

## 7. Expected Execution Flow

```text
Module Initialization
         |
         v
Initialize rwlock_t
         |
         v
Initialize shared_value = 0
         |
         v
Create Reader 1 and Reader 2
         |
         v
Readers acquire read locks
         |
         v
Readers read shared_value
         |
         v
Writer requests write lock
         |
         v
Writer spins while readers hold the lock
         |
         v
Readers release read locks
         |
         v
Writer acquires write lock
         |
         v
Writer updates shared_value
         |
         v
Writer releases write lock
```

The exact execution order depends on thread scheduling. Use appropriate synchronization in the experiment if you need to demonstrate a particular ordering reliably.

## 8. Important Concepts

### 8.1 Can Multiple Readers Acquire the Lock Simultaneously?

Yes.

Multiple readers can hold the read lock simultaneously, provided no writer holds the lock.

```text
Reader 1 ---- Read lock ----+
                            |
Reader 2 ---- Read lock ----+---- Shared data
                            |
Reader 3 ---- Read lock ----+
```

### 8.2 Can a Writer Acquire the Lock While Readers Are Active?

No.

A writer must wait until all existing readers release their read locks.

```text
Reader 1: Holds read lock
Reader 2: Holds read lock
Writer 1: Requests write lock

Writer 1 cannot acquire the lock yet.
```

### 8.3 Can a Reader Acquire the Lock While a Writer Holds It?

No.

While a writer holds the write lock, readers cannot acquire the read lock.

They must wait until the writer releases it.

### 8.4 Why Does a Writer Spin?

`rwlock_t` is a spinlock-based synchronization primitive.

When a CPU attempts to acquire an unavailable lock, it spins while waiting rather than putting the task to sleep.

This is useful for short critical sections where sleeping is not permitted.

However, long critical sections can waste CPU time.

### 8.5 Can We Sleep While Holding `rwlock_t`?

No.

Consider:

```c
write_lock(&lock);

msleep(1000);

write_unlock(&lock);
```

This is an invalid locking design because `msleep()` may sleep while the spinlock is held.

Correct approach:

```c
unsigned long flags;

write_lock_irqsave(&lock, flags);
shared_value++;
write_unlock_irqrestore(&lock, flags);

msleep(1000);

write_lock_irqsave(&lock, flags);
shared_value++;
write_unlock_irqrestore(&lock, flags);
```

The sleep occurs outside the critical sections.

**Important:** The two updates are separate operations. Other threads may modify the shared data during the sleep.

### 8.6 Why Can Sleeping While Holding a Spinlock Cause Problems?

Suppose CPU 0 holds a spinlock and sleeps.

CPU 1 attempts to acquire the same lock and spins while waiting.

CPU 1 wastes CPU time, and the locking design violates the spinlock usage rules.

A spinlock must not be held across a sleeping operation.

### 8.7 What If Process Context and Hard IRQ Share the Same Lock?

If a hard IRQ handler uses the same lock as process context, the process-context code may need to disable local interrupts while acquiring the lock.

Otherwise, the following situation can occur:

```text
CPU 0

Process context acquires lock
             |
             v
Hard IRQ interrupts process context
             |
             v
IRQ handler tries to acquire same lock
             |
             v
IRQ handler spins
             |
             v
Process context cannot resume to release lock
```

This can deadlock.

An appropriate IRQ-safe locking variant can prevent this local interrupt scenario.

The exact locking operations depend on whether the data is read or written and which contexts access it.

## 9. `rwlock_t` vs `rw_semaphore` vs Mutex

| Feature | `rwlock_t` | `rw_semaphore` | Mutex |
|---|---|---|---|
| Multiple readers | Yes | Yes | No |
| Exclusive writer | Yes | Yes | Yes |
| Waiting behavior | Spins | Can sleep | Can sleep |
| Can sleep while holding the lock? | No | Generally yes | Generally yes |
| Suitable for hard IRQ blocking acquisition | No | No | No |
| Typical use | Short reader-writer critical sections | Reader-writer synchronization in sleepable contexts | General mutual exclusion |

### Rule of Thumb

- Use `rwlock_t` when multiple readers need concurrent access and the critical sections are short and non-sleeping.
- Use `rw_semaphore` when readers and writers need reader-writer synchronization in sleepable contexts.
- Use a mutex when only one task should access the protected region at a time.

An `rwlock_t` is not automatically faster than an `rw_semaphore` or mutex. Performance depends on contention, critical-section duration, and the workload.

## 10. Interview Questions

### Q1. What is `rwlock_t`?

`rwlock_t` is a Linux kernel reader-writer spinlock that allows multiple readers or one exclusive writer.

### Q2. Can multiple readers hold the read lock simultaneously?

Yes. Multiple readers can hold the read lock simultaneously if no writer holds the lock.

### Q3. Can two writers hold the write lock simultaneously?

No. The write lock provides exclusive access.

### Q4. Can a writer acquire the lock while readers hold it?

No. The writer must wait until the existing readers release their locks.

### Q5. What happens when a reader calls `read_unlock()`?

The reader releases its read lock. A writer can acquire the write lock only when no conflicting readers or writer hold the lock.

### Q6. Why does a waiting `rwlock_t` caller spin?

Because `rwlock_t` is a spinlock-based primitive. The caller waits by spinning instead of sleeping.

### Q7. Can we use `msleep()` while holding `rwlock_t`?

No. Sleeping while holding a spinlock is not permitted.

### Q8. What is the difference between `read_lock()` and `write_lock()`?

`read_lock()` acquires shared read access. `write_lock()` requests exclusive access.

### Q9. Why might `write_lock_irqsave()` be needed?

It disables local interrupts while acquiring the lock and saves the previous interrupt state. This can prevent a local hard IRQ handler from interrupting the lock holder and deadlocking on the same lock.

### Q10. Does `rwlock_t` always outperform a mutex?

No. Performance depends on the workload, contention, critical-section duration, and the number of readers and writers.

### Q11. What happens if the same shared variable is accessed without taking the lock?

The locking mechanism cannot protect an access that does not follow the synchronization rules. Concurrent accesses may cause a race condition.

### Q12. When should `rwlock_t` be preferred over `rw_semaphore`?

Consider `rwlock_t` when reader-writer locking is needed in a context where sleeping is not permitted and the critical sections are short.

## 11. Conclusion

In this experiment, two reader threads and one writer thread access a shared variable protected by `rwlock_t`.

The experiment demonstrates:

- Initialization using `rwlock_init()`.
- Read locking using `read_lock()`.
- Read unlocking using `read_unlock()`.
- Write locking using `write_lock()`.
- Write unlocking using `write_unlock()`.
- IRQ-safe locking using `read_lock_irqsave()` and `write_lock_irqsave()`.
- Concurrent readers.
- Exclusive writers.
- Spinning while waiting for the lock.
- Why sleeping while holding a spinlock is invalid.
- The difference between `rwlock_t`, `rw_semaphore`, and mutex.

**Key takeaway:** `rwlock_t` allows concurrent readers and exclusive writers, but waiting callers spin and the lock must not be held across sleeping operations.