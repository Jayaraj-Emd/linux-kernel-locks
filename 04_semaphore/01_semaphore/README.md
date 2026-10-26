# 04 - Linux Kernel Semaphore

## 1. Objective

Understand how Linux kernel semaphores work and how they synchronize multiple kernel threads accessing shared resources.

In this experiment, two kernel threads compete for a semaphore initialized to `1`.

- Thread 1 acquires the semaphore and holds it for 3 seconds.
- Thread 2 attempts to acquire the same semaphore.
- Thread 2 waits until the semaphore becomes available.
- After Thread 1 releases the semaphore, Thread 2 acquires it.

This experiment demonstrates semaphore initialization, acquisition, release, blocking, and synchronization between kernel threads.

## 2. What Is a Semaphore?

A semaphore is a synchronization mechanism that maintains a counter to control access to a resource.

When a thread successfully acquires a semaphore, its count decreases. If the semaphore is unavailable, the calling task can sleep until another task releases it.

Linux kernel semaphores can be used for:

- Controlling access to a limited number of resources.
- Synchronizing kernel threads.
- Making tasks wait when resources are unavailable.
- Coordinating access to shared resources.

### 2.1 Semaphore Initialized to 1

A semaphore initialized to `1` allows one successful acquisition at a time, provided acquisitions and releases are correctly paired.

```text
Initial semaphore count = 1

Thread 1                         Thread 2
   |                                |
   | down(&sem)                     |
   | Count: 1 -> 0                  |
   | Acquires semaphore             |
   |                                |
   |                                | down(&sem)
   |                                | Semaphore unavailable
   |                                | Sleeps / waits
   |                                |
   | Critical section                |
   |                                |
   | up(&sem)                       |
   | Releases semaphore             |
   |                                |
   |                                | Wakes and acquires
   |                                | semaphore
   |                                |
   |                                | Critical section
   |                                |
   |                                | up(&sem)
   |                                | Releases semaphore
```

**Important:** A semaphore initialized to `1` is commonly used for mutual exclusion, but unlike a mutex, it does not enforce ownership.

### 2.2 Semaphore Initialized to 2

A semaphore initialized to `2` allows up to two successful acquisitions at the same time.

```c
sema_init(&sem, 2);
```

```text
Initial semaphore count = 2

Thread 1                  Thread 2                  Thread 3
   |                         |                         |
   | down(&sem)              |                         |
   | Count: 2 -> 1           |                         |
   | Acquires                |                         |
   |                         | down(&sem)              |
   |                         | Count: 1 -> 0           |
   |                         | Acquires                |
   |                         |                         | down(&sem)
   |                         |                         | Semaphore unavailable
   |                         |                         | Waits
```

Thread 1 and Thread 2 can acquire the semaphore. Thread 3 must wait until a permit becomes available.

A count of `2` does not guarantee parallel execution. Actual parallelism depends on scheduling and available CPUs.

## 3. Semaphore APIs

### 3.1 `sema_init()`

Initializes a semaphore with the specified count.

```c
sema_init(&sem, 1);
```

Parameters:

- `&sem`: Address of the semaphore.
- `1`: Initial semaphore count.

Examples:

```c
sema_init(&sem, 1);  /* One acquisition at a time */
sema_init(&sem, 2);  /* Up to two acquisitions */
sema_init(&sem, 5);  /* Up to five acquisitions */
```

### 3.2 `down()`

Acquires the semaphore.

```c
down(&sem);
```

Behavior:

- Acquires the semaphore when it is available.
- Decreases the available count.
- If the semaphore is unavailable, the calling task waits.
- The calling task may sleep while waiting.

Example:

```c
down(&sem);

/* Critical section */

up(&sem);
```

**Important:** `down()` must not be used in hard IRQ context because it may sleep.

### 3.3 `up()`

Releases the semaphore.

```c
up(&sem);
```

Behavior:

- Releases a previously acquired semaphore.
- Makes a permit available.
- Can wake a task waiting on the semaphore.

Example:

```c
down(&sem);

/* Access shared resource */

up(&sem);
```

Every successful acquisition must be paired with the appropriate release.

### 3.4 `down_interruptible()`

Attempts to acquire the semaphore but allows the wait to be interrupted by a signal.

```c
int ret;

ret = down_interruptible(&sem);

if (ret) {
    return ret;
}

/* Critical section */

up(&sem);
```

Return behavior:

- `0`: Acquisition succeeded.
- Negative error code: Acquisition did not succeed, for example because the wait was interrupted by a signal.

If the function returns an error, the semaphore was not acquired. Therefore, do not call `up()` for that failed acquisition.

### 3.5 `down_trylock()`

Attempts to acquire the semaphore without waiting.

```c
int ret;

ret = down_trylock(&sem);

if (ret) {
    /* Semaphore unavailable */
    return -EBUSY;
}

/* Critical section */

up(&sem);
```

Return values:

- `0`: Acquisition succeeded.
- Nonzero: Acquisition failed.

Unlike many kernel APIs, `down_trylock()` returns `0` on success and a nonzero value on failure.

The calling task decides how to handle failure. It can retry later, return an error, or perform another operation.

## 4. Experiment

### 4.1 Directory Structure

```text
04_Semaphore/
├── Makefile
├── semaphore.c
└── README.md
```

### 4.2 Experiment Description

The kernel module creates two kernel threads that compete for a semaphore initialized to `1`.

**Thread 1**

1. Attempts to acquire the semaphore.
2. Acquires the semaphore when it is available.
3. Holds it for 3 seconds using `msleep(3000)`.
4. Releases the semaphore.
5. Signals completion.

**Thread 2**

1. Attempts to acquire the same semaphore.
2. Waits while Thread 1 holds it.
3. Acquires the semaphore after it becomes available.
4. Holds it for 1 second.
5. Releases the semaphore.
6. Signals completion.

Completions coordinate thread completion and help ensure that the module does not finish unloading while its worker threads are still running.

### 4.3 Expected Execution Flow

```text
Module Initialization
         |
         v
Initialize semaphore to 1
         |
         v
Create Thread 1
         |
         v
Thread 1 acquires semaphore
         |
         v
Thread 1 sleeps for 3 seconds
         |
         v
Create Thread 2
         |
         v
Thread 2 attempts down(&sem)
         |
         v
Semaphore unavailable
         |
         v
Thread 2 waits
         |
         v
Thread 1 releases semaphore
         |
         v
Thread 2 acquires semaphore
         |
         v
Thread 2 sleeps for 1 second
         |
         v
Thread 2 releases semaphore
         |
         v
Both threads complete
```

The actual scheduling and log order can vary. Ensure the experiment correctly coordinates thread startup if deterministic ordering is required.

## 5. Build and Execute

Run the following commands from the `04_Semaphore` directory.

### 5.1 Build the Module

```bash
make
```

### 5.2 Load the Module

```bash
sudo insmod semaphore.ko
```

### 5.3 Check Kernel Logs

```bash
sudo dmesg | tail -n 30
```

### 5.4 Remove the Module

```bash
sudo rmmod semaphore
```

### 5.5 Clean Build Files

```bash
make clean
```

## 6. Expected Observation

The following output illustrates the intended behavior. The exact messages and scheduling order depend on the implementation.

```text
Thread 1: Trying to acquire semaphore
Thread 1: Acquired semaphore
Thread 2: Trying to acquire semaphore
Thread 1: Releasing semaphore
Thread 2: Acquired semaphore
Thread 2: Releasing semaphore
```

### Explanation

1. Thread 1 acquires the semaphore.
2. The available count becomes `0`.
3. Thread 2 attempts to acquire the semaphore.
4. Because the semaphore is unavailable, Thread 2 waits.
5. Thread 1 finishes its critical section and calls `up(&sem)`.
6. Thread 2 becomes eligible to acquire the semaphore.
7. Thread 2 enters its critical section and eventually releases the semaphore.

**Observation:** The semaphore prevents another task from successfully acquiring the same single permit while it is held.

## 7. Important Concepts

### 7.1 Why Can a Task Sleep When Acquiring a Semaphore?

A semaphore may be unavailable when a task attempts to acquire it.

Instead of continuously checking the semaphore, the task can wait until the semaphore becomes available.

This allows the scheduler to run other tasks while the waiting task is blocked.

This is different from spinning on a spinlock, where the CPU repeatedly checks whether the lock has become available.

### 7.2 Can a Semaphore Be Acquired in a Hard IRQ Handler?

No. Blocking semaphore acquisition APIs such as `down()` cannot be used in hard IRQ context.

The reason is that semaphore acquisition can require the calling task to sleep. Hard IRQ handlers cannot sleep.

For data shared with a hard IRQ handler, a spinlock may be appropriate, provided the locking and interrupt rules are correct.

### 7.3 What Happens If `up()` Is Called Twice?

Suppose the semaphore is initialized to `1`.

```text
Initial count = 1

Thread 1 calls down()
Count: 1 -> 0

Thread 1 calls up()
Count: 0 -> 1

Thread 1 calls up() again
Count can become 2
```

The extra release makes an additional permit available.

Consequently, multiple tasks may acquire the semaphore even though the programmer intended to allow only one task at a time.

A semaphore does not enforce ownership, so the programmer must ensure that acquisitions and releases are correctly paired.

### 7.4 What Happens If `down_interruptible()` Returns an Error?

If `down_interruptible()` returns a negative error code, the task did not acquire the semaphore.

```c
ret = down_interruptible(&sem);

if (ret) {
    return ret;
}

/* Semaphore acquired */

/* Critical section */

up(&sem);
```

The error path must not call `up()` for an acquisition that never succeeded.

### 7.5 What Happens If `down_trylock()` Fails?

`down_trylock()` returns immediately if the semaphore cannot be acquired.

It does not make the task sleep while waiting for the semaphore.

The calling task can choose to retry later, return an error, or perform another operation.

### 7.6 Can a Semaphore Be Used to Protect Shared Data?

Yes, a semaphore initialized to `1` can be used to protect shared data between tasks.

However, a mutex is generally preferable when the goal is mutual exclusion because a mutex enforces ownership.

A counting semaphore is especially useful when controlling access to a limited number of resources.

### 7.7 What Is the Difference Between a Semaphore and a Spinlock?

A semaphore can block and allow the calling task to sleep.

A spinlock makes the caller spin while waiting for the lock to become available.

| Feature | Semaphore | Spinlock |
|---|---|---|
| Waiting behavior | Can sleep | Spins while waiting |
| Can be used in hard IRQ context | Blocking acquisition cannot | Can be used with correct IRQ-safe locking |
| Suitable for long critical sections | Potentially, in sleepable contexts | Generally unsuitable |
| Suitable for short critical sections | Sometimes | Often |
| Protects shared data | Yes | Yes |
| Enforces ownership | No | Not in the same way as a mutex |

The choice depends on the execution context, critical-section duration, and whether sleeping is permitted.

## 8. Semaphore vs Mutex

| Feature | Semaphore | Mutex |
|---|---|---|
| Primary purpose | Resource counting and synchronization | Mutual exclusion |
| Initial state | Configurable count | Unlocked |
| Multiple simultaneous acquisitions | Possible when count is greater than one | No |
| Ownership enforced | No | Yes |
| Can the waiting task sleep? | Yes, with blocking acquisition | Yes |
| Blocking acquisition allowed in hard IRQ context | No | No |
| Typical use | Resource pools and task coordination | Protecting shared data |

### Rule of Thumb

- Use a **mutex** when protecting shared data between tasks.
- Use a **counting semaphore** when controlling access to a limited number of resources.
- Use a **spinlock** when protecting short critical sections where sleeping is not permitted, including appropriate interrupt-context use.

## 9. Interview Questions

### Q1. What is a semaphore?

A semaphore is a synchronization mechanism that maintains a counter to control access to resources or coordinate tasks.

### Q2. What is the difference between `down()` and `up()`?

`down()` acquires a semaphore and may block if it is unavailable. `up()` releases the semaphore and can wake a waiting task.

### Q3. What is the difference between a mutex and a semaphore?

A mutex enforces ownership: the task that locks it is expected to unlock it. A semaphore does not enforce ownership and can be used for resource counting.

### Q4. What happens when a semaphore initialized to `1` is acquired?

Its available count becomes `0`. Another task attempting to acquire it must wait until a permit becomes available.

### Q5. What is the difference between `down()` and `down_trylock()`?

`down()` can wait for the semaphore. `down_trylock()` returns immediately if the semaphore cannot be acquired.

### Q6. What is the difference between `down()` and `down_interruptible()`?

`down()` waits without allowing the wait to be interrupted by a signal in the same way as `down_interruptible()`.

`down_interruptible()` can return an error if a signal interrupts the wait.

### Q7. Why can't a semaphore's blocking acquisition API be used in a hard IRQ handler?

The acquisition may require the current task to sleep, but hard IRQ handlers cannot sleep.

### Q8. Does a semaphore initialized to `2` guarantee parallel execution?

No. It allows up to two successful acquisitions at a time. Actual parallel execution depends on scheduling and available CPUs.

### Q9. What can happen if a semaphore is released more times than it is acquired?

Its count can become larger than intended, allowing more tasks to acquire it than expected and potentially breaking mutual exclusion.

### Q10. Why should a mutex be preferred over a semaphore for protecting shared data?

A mutex enforces ownership and is designed for mutual exclusion. A semaphore does not enforce ownership, so another task could release it accidentally.

### Q11. Can a semaphore be used in a hard IRQ handler if its count is greater than zero?

No. Blocking semaphore acquisition APIs are not suitable for hard IRQ context, even when the semaphore appears available.

### Q12. When should a counting semaphore be used?

When a fixed number of equivalent resources are available and the number of simultaneous acquisitions must be limited.

For example, a driver managing two equivalent hardware resources could use a counting semaphore initialized to `2`, provided the design and execution context are appropriate.

### Q13. What is the difference between sleeping and spinning?

A sleeping task is blocked so the scheduler can run other tasks. A spinning task repeatedly checks whether a lock is available, consuming CPU time while it waits.

## 10. Conclusion

In this experiment, two kernel threads compete for a semaphore initialized to `1`.

The experiment demonstrates:

- Semaphore initialization using `sema_init()`.
- Acquisition using `down()`.
- Release using `up()`.
- Waiting when a semaphore is unavailable.
- Non-blocking acquisition using `down_trylock()`.
- Interruptible acquisition using `down_interruptible()`.
- The difference between a semaphore and a mutex.
- The difference between a semaphore and a spinlock.
- Why blocking synchronization mechanisms cannot be used in hard IRQ context.
- The importance of pairing acquisitions and releases correctly.
- How a counting semaphore controls access to multiple resources.

**Key takeaway:** A semaphore controls access through a counter and can make tasks wait. Correct synchronization requires choosing the appropriate primitive and pairing acquisitions with releases.