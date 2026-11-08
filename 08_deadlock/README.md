Let's create the deadlock README for your `linux-kernel-locks` repository.

```markdown
# 08 - Deadlocks in Linux Kernel Synchronization

## 1. Objective

Understand how deadlocks occur in Linux kernel code, how to identify their causes, and how to prevent them.

In this topic, we will learn:

- What is a deadlock?
- The four necessary conditions for deadlock.
- Circular wait and inconsistent lock ordering.
- Self-deadlock.
- Deadlocks involving mutexes and spinlocks.
- Deadlock prevention using lock ordering.
- Deadlock detection using `lockdep`.
- Debugging deadlocks using stack traces.

---

## 2. What Is a Deadlock?

A deadlock occurs when two or more threads wait indefinitely for resources held by one another, preventing them from making progress.

Consider two mutexes:

```c
static DEFINE_MUTEX(lock_A);
static DEFINE_MUTEX(lock_B);
```

Thread 1:

```c
mutex_lock(&lock_A);
mutex_lock(&lock_B);

/* Critical section */

mutex_unlock(&lock_B);
mutex_unlock(&lock_A);
```

Thread 2:

```c
mutex_lock(&lock_B);
mutex_lock(&lock_A);

/* Critical section */

mutex_unlock(&lock_A);
mutex_unlock(&lock_B);
```

Suppose the execution happens as follows:

1. Thread 1 acquires `lock_A`.
2. Thread 2 acquires `lock_B`.
3. Thread 1 requests `lock_B` and waits.
4. Thread 2 requests `lock_A` and waits.

Now:

- Thread 1 holds `lock_A` and waits for `lock_B`.
- Thread 2 holds `lock_B` and waits for `lock_A`.
- Neither thread can proceed to release its lock.

Both threads are deadlocked.

---

## 3. Four Necessary Conditions for Deadlock

Under the classical deadlock model, all four conditions must hold simultaneously for a deadlock to occur.

### 3.1 Mutual Exclusion

A resource can be held exclusively by one thread.

For example, a mutex allows only one thread to hold it at a time.

### 3.2 Hold and Wait

A thread holds one resource while waiting to acquire another resource.

Example:

```c
mutex_lock(&lock_A);
mutex_lock(&lock_B);
```

The thread holds `lock_A` while waiting for `lock_B`.

### 3.3 No Preemption

A resource cannot simply be taken away from its owner. The owner must release it according to the synchronization mechanism's rules.

For example, another thread cannot forcibly release a mutex on behalf of its owner.

### 3.4 Circular Wait

A circular chain of dependencies exists.

For example:

```text
Thread 1 holds lock_A and waits for lock_B.
Thread 2 holds lock_B and waits for lock_A.
```

Dependency:

Thread 1 → lock_B → Thread 2 → lock_A → Thread 1

This creates a circular wait.

---

## 4. Lock Ordering

Inconsistent lock ordering is a common cause of deadlocks.

### Incorrect: Different Acquisition Orders

Function 1:

```c
mutex_lock(&lock_A);
mutex_lock(&lock_B);

/* Critical section */

mutex_unlock(&lock_B);
mutex_unlock(&lock_A);
```

Function 2:

```c
mutex_lock(&lock_B);
mutex_lock(&lock_A);

/* Critical section */

mutex_unlock(&lock_A);
mutex_unlock(&lock_B);
```

The first function acquires `lock_A` before `lock_B`, while the second acquires `lock_B` before `lock_A`.

If the threads acquire their first locks simultaneously, they can become deadlocked.

### Correct: Consistent Acquisition Order

Both functions should acquire `lock_A` before `lock_B`.

```c
mutex_lock(&lock_A);
mutex_lock(&lock_B);

/* Critical section */

mutex_unlock(&lock_B);
mutex_unlock(&lock_A);
```

Use this ordering consistently in every code path that needs both locks.

**Important:** Releasing locks in reverse acquisition order is a useful convention, but consistent acquisition ordering is the key rule for preventing circular-wait deadlocks caused by conflicting lock orders.

---

## 5. Self-Deadlock

A self-deadlock occurs when a thread waits for a resource that it already holds.

Consider:

```c
static DEFINE_MUTEX(my_lock);

static void my_function(void)
{
    mutex_lock(&my_lock);

    /* Some work */

    mutex_lock(&my_lock);

    /* More work */

    mutex_unlock(&my_lock);
    mutex_unlock(&my_lock);
}
```

Execution:

1. The thread acquires `my_lock`.
2. The thread attempts to acquire `my_lock` again.
3. The second acquisition waits because the mutex is already held.
4. The thread cannot reach the unlock operation because it is blocked.

The thread deadlocks itself.

A normal Linux mutex is not recursive.

### Prevention

Do not acquire a mutex again while already holding it.

Review nested function calls carefully because a helper function might attempt to acquire a mutex that its caller already holds.

---

## 6. Deadlocks Involving Spinlocks

Spinlocks can also participate in deadlocks.

Consider:

```c
static DEFINE_SPINLOCK(lock_A);
static DEFINE_SPINLOCK(lock_B);
```

Thread 1:

```c
spin_lock(&lock_A);
spin_lock(&lock_B);

/* Critical section */

spin_unlock(&lock_B);
spin_unlock(&lock_A);
```

Thread 2:

```c
spin_lock(&lock_B);
spin_lock(&lock_A);

/* Critical section */

spin_unlock(&lock_A);
spin_unlock(&lock_B);
```

The same circular-wait problem can occur if both threads acquire their first locks simultaneously.

### Important Spinlock Rules

- Never sleep while holding a spinlock.
- Keep spinlock-protected critical sections short.
- Do not call blocking operations while holding a spinlock.
- Follow consistent lock ordering.
- Consider interrupt-context interactions when selecting spinlock APIs.

For example, calling `msleep()` while holding a spinlock is invalid.

A mutex can sleep while waiting for a lock. A spinlock instead makes the CPU wait actively when contended.

Both can cause deadlocks if used incorrectly.

---

## 7. Deadlock Prevention

Follow these practices when developing Linux kernel drivers.

### 7.1 Use Consistent Lock Ordering

Establish a global acquisition order for locks.

For example:

```text
lock_A → lock_B → lock_C
```

Functions that need multiple locks should follow the same order.

### 7.2 Avoid Recursive Mutex Acquisition

Do not attempt to acquire a normal mutex that the current thread already holds.

### 7.3 Keep Critical Sections Short

Perform only the operations that need protection while holding a lock.

### 7.4 Never Sleep While Holding a Spinlock

Spinlock-protected code must not sleep or block.

### 7.5 Understand Execution Contexts

Choose synchronization mechanisms appropriate for the execution context.

For example:

- Mutexes are suitable for sleepable contexts.
- Spinlocks are suitable for short, non-sleeping critical sections.
- Hard IRQ handlers must not acquire mutexes.
- If process context and interrupt context share data, select an appropriate IRQ-safe locking strategy.

### 7.6 Use Lockdep

Enable kernel locking validation when available and exercise the relevant code paths during testing.

---

## 8. Deadlock Detection Using Lockdep

`lockdep` is the Linux kernel's lock dependency validator.

It tracks observed locking dependencies and can report potential deadlocks caused by problematic lock ordering.

### Example

Suppose one function acquires:

```text
lock_A → lock_B
```

Another function acquires:

```text
lock_B → lock_A
```

If lockdep observes these conflicting dependencies, it can report a potential circular locking dependency.

An actual deadlock does not have to occur before lockdep reports the problem.

### Check Kernel Configuration

Run:

```bash
grep -E 'CONFIG_LOCKDEP=|CONFIG_PROVE_LOCKING=' \
    /boot/config-$(uname -r)
```

Relevant options include:

```text
CONFIG_LOCKDEP=y
CONFIG_PROVE_LOCKING=y
```

Their availability depends on the kernel configuration.

- `CONFIG_LOCKDEP` enables lock dependency tracking.
- `CONFIG_PROVE_LOCKING` enables additional locking correctness validation.

### Inspect Kernel Messages

```bash
sudo dmesg | grep -i -E \
    'lockdep|possible circular locking|deadlock'
```

To observe new kernel messages:

```bash
sudo dmesg -w
```

Look for warnings such as:

```text
possible circular locking dependency detected
```

**Important:** Lockdep detects potential locking problems; it does not automatically prevent deadlocks. A clean test does not prove that all possible execution paths are safe.

---

## 9. Debugging Deadlocks Using Stack Traces

A kernel stack trace shows the sequence of function calls associated with a task.

Suppose a thread blocks while trying to acquire a mutex.

Its stack trace may contain functions associated with:

```text
worker_thread
    |
    v
driver_function
    |
    v
mutex_lock
    |
    v
Waiting for the mutex
```

The exact stack depends on the kernel version and execution path.

### Debugging Procedure

1. Identify the threads that appear stuck.
2. Inspect their kernel stack traces.
3. Identify which locks they are waiting to acquire.
4. Determine which threads hold those locks.
5. Check whether the dependencies form a cycle.
6. Fix the locking design.

### Useful Commands

Inspect kernel messages:

```bash
sudo dmesg | tail -100
```

Inspect task states:

```bash
ps -eLo pid,tid,stat,comm,wchan:32
```

Inspect a task's kernel stack, where supported:

```bash
sudo cat /proc/<PID>/stack
```

Replace `<PID>` with the relevant process or thread ID.

A blocked task does not necessarily indicate a deadlock. It may simply be waiting for a resource that will become available.

---

## 10. Lockdep vs Stack Traces

| Feature | Lockdep | Stack Trace |
|---|---|---|
| Primary purpose | Validate locking dependencies | Show a task's execution path |
| Detects potential circular dependencies | Yes, when problematic dependencies are observed | Not directly |
| Helps identify where a task is blocked | Not its primary purpose | Often |
| Requires an actual deadlock | No | No |
| Helps investigate real hangs | Yes | Yes |

These tools complement each other.

Lockdep helps identify problematic locking patterns, while stack traces help investigate where threads are stuck.

---

## 11. Common Mistakes

### Mistake 1: Assuming Every Wait Is a Deadlock

A thread waiting for a mutex is not necessarily deadlocked. The owner may release the mutex normally.

### Mistake 2: Acquiring Locks in Different Orders

Different acquisition orders can create a circular dependency.

### Mistake 3: Acquiring a Mutex Twice

A thread attempting to reacquire a normal mutex it already holds can self-deadlock.

### Mistake 4: Sleeping While Holding a Spinlock

Operations such as `msleep()` must not be called while holding a spinlock.

### Mistake 5: Assuming Lockdep Proves the Entire Driver Is Safe

Lockdep can only validate locking behavior it can observe. It does not prove the absence of every possible deadlock.

### Mistake 6: Assuming Every Blocked Thread Is Deadlocked

Inspect the lock owners and dependencies before concluding that a deadlock exists.

---

## 12. Interview Questions

### Q1. What is a deadlock?

A deadlock occurs when threads wait indefinitely for resources held by one another, preventing progress.

### Q2. What are the four necessary conditions for deadlock?

Mutual exclusion, hold and wait, no preemption, and circular wait.

### Q3. How can inconsistent lock ordering cause deadlocks?

Different acquisition orders can allow threads to hold locks that other threads need, creating a circular wait.

### Q4. What is self-deadlock?

A thread waits for a resource that it already holds.

### Q5. Can spinlocks cause deadlocks?

Yes. Inconsistent lock ordering and incorrect interactions between execution contexts can cause deadlocks involving spinlocks.

### Q6. What is lockdep?

A Linux kernel lock dependency validator that detects potential locking problems.

### Q7. Does lockdep prevent deadlocks?

No. It reports detected locking problems so developers can correct them.

### Q8. How can a stack trace help debug a deadlock?

It helps identify where a task is blocked. Investigating the locks and their owners can reveal a circular dependency.

### Q9. How do you prevent deadlocks involving multiple locks?

Use a consistent lock acquisition order and follow the locking requirements of each execution context.

### Q10. Is every blocked kernel thread deadlocked?

No. A thread may be waiting for a resource that another thread will release normally.

---

