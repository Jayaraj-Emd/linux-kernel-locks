# Spinlock

## 1. Why do we need `spin_lock_irqsave()`?

A shared variable is accessed by process context and an interrupt handler.

Both contexts use the same spinlock.

Problem:
- Process context acquires the lock.
- An interrupt occurs on the same CPU.
- The interrupt handler tries to acquire the same lock.
- The handler spins because the process context holds it.
- The process context cannot resume until the handler returns.

This causes deadlock.

```text
Process Context                 Interrupt Handler
      |                                |
      | Acquires lock                  |
      |                                |
      | <---------- Interrupted -------|
      |                                |
      |                         Tries same lock
      |                         Spins forever
      |                                |
      X Cannot resume
        to release lock
```

## 2. `spin_lock_irqsave()` API

```c
unsigned long flags;

spin_lock_irqsave(&lock, flags);

/* Critical section */

spin_unlock_irqrestore(&lock, flags);
```

### `spin_lock_irqsave()`

- Saves the previous local interrupt state in `flags`.
- Disables local interrupts.
- Acquires the spinlock.

### `spin_unlock_irqrestore()`

- Releases the spinlock.
- Restores the previous local interrupt state.

### Why save the interrupt state?

The caller might have entered the critical section with interrupts already disabled.

If we unconditionally enable interrupts after releasing the lock, we could change the caller's original interrupt state.

`spin_lock_irqsave()` and `spin_unlock_irqrestore()` preserve that state.

## 3. `spin_lock_irq()` vs `spin_lock_irqsave()`

| API | Behaviour |
|---|---|
| `spin_lock_irq()` | Disables local interrupts and acquires the lock |
| `spin_unlock_irq()` | Releases the lock and enables local interrupts |
| `spin_lock_irqsave()` | Saves the previous interrupt state, disables local interrupts, and acquires the lock |
| `spin_unlock_irqrestore()` | Releases the lock and restores the saved interrupt state |

### Example

Suppose a function is called with local interrupts already disabled.

Using `spin_lock_irq()` and `spin_unlock_irq()`:

```text
Before function: Interrupts DISABLED
          |
          v
    spin_lock_irq()
          |
          v
    Critical section
          |
          v
    spin_unlock_irq()
          |
          v
After function: Interrupts ENABLED
```

The original state was disabled, but the function enabled interrupts.

Using `spin_lock_irqsave()` and `spin_unlock_irqrestore()`:

```text
Before function: Interrupts DISABLED
          |
          v
spin_lock_irqsave(&lock, flags)
          |
          | Save previous state = DISABLED
          v
    Critical section
          |
          v
spin_unlock_irqrestore(&lock, flags)
          |
          v
After function: Interrupts DISABLED
```

The original interrupt state is restored.

## 4. Hrtimer Basics

### What is an hrtimer?

An hrtimer is a Linux high-resolution timer used to schedule a callback at a specified time.

When the timer expires, Linux executes the registered callback.

Example use cases:
- Periodic activities
- Timeouts
- Measuring elapsed time
- Scheduling time-sensitive operations

### Important APIs

#### `hrtimer_init()`

Initializes the timer.

```c
hrtimer_init(&hrt, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
```

Arguments:

- `&hrt` — Timer object.
- `CLOCK_MONOTONIC` — Clock used to measure time.
- `HRTIMER_MODE_REL` — Expiry is relative to the current time.

#### `CLOCK_MONOTONIC`

Measures elapsed time and does not jump when the system's wall clock is adjusted.

Example:

```text
Monotonic time: 100s -> 101s -> 102s -> 103s

Wall clock changes from 10:00 AM to 9:00 AM.

Monotonic time continues progressing.
```

This makes it useful for timeouts and elapsed-time measurement.

#### `HRTIMER_MODE_REL`

Specifies a relative expiry.

For example, a timer configured for 5 seconds expires approximately 5 seconds after it starts.

#### `ms_to_ktime()`

Converts milliseconds to the kernel's `ktime_t` representation.

```c
ktime_t interval;

interval = ms_to_ktime(100);
```

#### `hrtimer_start()`

Starts the timer.

```c
hrtimer_start(&hrt, interval, HRTIMER_MODE_REL);
```

#### Timer callback

The callback has this signature:

```c
static enum hrtimer_restart timer_callback(struct hrtimer *timer)
{
    /* Timer expiry handling */

    return HRTIMER_NORESTART;
}
```

Return values:

| Return value | Meaning |
|---|---|
| `HRTIMER_NORESTART` | Do not restart the timer |
| `HRTIMER_RESTART` | Restart the timer according to its updated expiry |

#### `hrtimer_cancel()`

Cancels the timer.

```c
hrtimer_cancel(&hrt);
```

Call it during module cleanup to cancel the timer and wait for a running callback to finish.

### Hrtimer callback context

An hrtimer callback may execute in hard-interrupt context or in softirq context, depending on the timer mode and kernel configuration.

The callback must not perform operations that sleep when executing in atomic context.

Examples of operations that must not be used in such a callback:
- `msleep()`
- Acquiring a mutex that might block

Keep the callback short and defer longer work to an appropriate context.

## 5. Hrtimer Forwarding

Linux provides APIs for periodic timers.

### `hrtimer_forward_now()`

Advances the timer's expiry relative to the current timer time.

Example:

```c
static enum hrtimer_restart timer_callback(struct hrtimer *timer)
{
    ktime_t interval = ms_to_ktime(100);

    hrtimer_forward_now(timer, interval);

    return HRTIMER_RESTART;
}
```

Suppose:
- Timer interval = 100 ms.
- Original expiry = 100 ms.
- Callback executes at 350 ms.

The forwarding operation advances the expiry to the next appropriate interval, 400 ms.

It does not simply schedule the next expiry at 450 ms.

**Why?**

The timer maintains its periodic schedule rather than accumulating additional delay whenever the callback executes late.

## 6. Hrtimer and Process Context Experiment

### Objective

Understand how process context and a timer callback can access shared data protected by a spinlock.

### Shared variable

```c
static struct hrtimer hrt;
static spinlock_t lock;
static u64 count;
```

### Process context

```c
unsigned long flags;

spin_lock_irqsave(&lock, flags);

count++;

spin_unlock_irqrestore(&lock, flags);
```

### Hrtimer callback

```c
static enum hrtimer_restart timer_callback(struct hrtimer *timer)
{
    spin_lock(&lock);

    count++;

    spin_unlock(&lock);

    return HRTIMER_NORESTART;
}
```

### Experiment setup

- Initialize the spinlock.
- Initialize the hrtimer using `CLOCK_MONOTONIC`.
- Configure a relative timer expiry of 100 ms.
- Start the timer.
- Call `msleep(100)` before acquiring the lock in process context.
- Increment the shared counter from both contexts.
- Cancel the timer during module cleanup.
- Print the final counter value.

### Observation

Example kernel log:

```text
Spin lock initialized successfully...
hrtimer get the lock..
hrtimer released lock..
status register value 582
process context thread get lock...
process context released a lock...
Expected value = 2
Actual value = 2
Spin lock module exited successfully...
```

### What happened?

1. The hrtimer was scheduled to expire after 100 ms.
2. Process context called `msleep(100)`.
3. The timer callback executed and incremented `count`.
4. Process context resumed and acquired the lock.
5. Process context incremented `count`.

Final count = 2.

The callback ran before process context acquired the lock.

The exact execution order depends on timing and scheduling.

**Important:** This experiment demonstrates access to shared data from two contexts, but it does not reproduce the deadlock scenario.

The callback completed before process context acquired the lock.

## 7. Spinlock and Interrupt Safety

### Scenario

Process context and an interrupt handler share the same lock.

Process context acquires the lock using `spin_lock()`.

A relevant local interrupt occurs while process context holds the lock.

The interrupt handler tries to acquire the same lock.

```text
Process Context                 Interrupt Handler
      |                                |
      | Acquires lock                  |
      |                                |
      | <---------- Interrupted -------|
      |                                |
      |                         Tries same lock
      |                         Spins and waits
      |                                |
      X Cannot resume
        to release lock
```

This can deadlock because the interrupted context cannot resume until the interrupt handler returns.

### Solution

Process context uses:

```c
spin_lock_irqsave(&lock, flags);

/* Critical section */

spin_unlock_irqrestore(&lock, flags);
```

This prevents the relevant local interrupt from interrupting the protected critical section.

The spinlock still protects the data against concurrent access from other CPUs.

### Important distinction

- Local interrupt disabling affects interrupt delivery on the current CPU.
- It does not disable interrupts system-wide.
- A spinlock protects shared data across CPUs.
- An interrupt that becomes pending while masked may execute after interrupts are restored.

## 8. Process Context vs Hard IRQ vs Softirq

### Process context

Process context can be preempted under normal scheduling rules.

A process-context function can sleep when the operation and locking rules permit it.

If the process shares a lock with a local hard IRQ handler, a common pattern is:

```c
spin_lock_irqsave(&lock, flags);

/* Critical section */

spin_unlock_irqrestore(&lock, flags);
```

### Hard IRQ context

A hard IRQ handler executes in response to a hardware interrupt.

It must not perform operations that sleep.

If a hard IRQ handler shares a lock with process context, the process context must use suitable interrupt-safe locking to avoid the handler interrupting a lock holder.

### Softirq context

Softirqs perform deferred work associated with interrupt processing.

Softirq context cannot sleep.

Hard interrupts can generally interrupt softirq processing, depending on the interrupt configuration.

If a softirq shares a lock with process context, `spin_lock_bh()` can prevent local bottom-half processing from interrupting the protected process-context section.

If a hard IRQ handler also shares the lock, the locking design must additionally prevent that handler from interrupting a lock holder.

## 9. `spin_lock_bh()` API

### Why do we need it?

Suppose process context and a softirq access the same shared variable.

Both use the same spinlock.

If a softirq interrupts process context while process context holds the lock, the softirq may spin while the interrupted process cannot resume.

This can cause deadlock.

### Solution

Process context:

```c
spin_lock_bh(&lock);

/* Critical section */

spin_unlock_bh(&lock);
```

`spin_lock_bh()`:
- Disables local bottom-half processing.
- Acquires the spinlock.

`spin_unlock_bh()`:
- Releases the spinlock.
- Re-enables bottom-half processing as appropriate.

The spinlock protects shared data against concurrent access from other CPUs.

**Note:** `spin_lock_bh()` does not disable hard interrupts.

### Comparing the APIs

| API | Purpose |
|---|---|
| `spin_lock()` | Acquires a spinlock without changing local interrupt or bottom-half state |
| `spin_lock_irqsave()` | Saves interrupt state, disables local interrupts, and acquires the lock |
| `spin_lock_bh()` | Disables local bottom-half processing and acquires the lock |

Choose the API according to the execution contexts that share the lock.

## 10. What I Learned

- A spinlock protects shared data from concurrent access.
- `spin_lock_irqsave()` saves the previous interrupt state and disables local interrupts.
- `spin_unlock_irqrestore()` restores the previous interrupt state.
- Saving interrupt state prevents a function from accidentally enabling interrupts that were already disabled.
- `CLOCK_MONOTONIC` is useful for measuring elapsed time independently of wall-clock adjustments.
- `HRTIMER_MODE_REL` specifies a relative timer expiry.
- Hrtimer callbacks must respect their execution-context restrictions.
- A lock holder can deadlock if interrupted by a handler that tries to acquire the same lock.
- `spin_lock_bh()` protects against local bottom-half processing interrupting the protected process-context section.
- Disabling local interrupts on one CPU does not protect shared data against other CPUs.
- Sleeping while holding a spinlock is invalid.

## 11. Interview Questions

### Q1. Why do we use `spin_lock_irqsave()`?

To protect shared data when process context shares a lock with an interrupt handler, preventing the relevant local interrupt from interrupting the lock holder.

### Q2. Why save the interrupt state?

To restore the previous state correctly instead of unconditionally enabling interrupts.

### Q3. Does disabling local interrupts protect shared data across multiple CPUs?

No. The spinlock provides mutual exclusion across CPUs. Disabling local interrupts prevents the relevant local interrupt from interrupting the critical section on the current CPU.

### Q4. What is the difference between `spin_lock()` and `spin_lock_irqsave()`?

`spin_lock()` acquires the lock without changing the local interrupt state.

`spin_lock_irqsave()` saves the previous interrupt state, disables local interrupts, and acquires the lock.

### Q5. Can we call `msleep()` while holding a spinlock?

No. Sleeping while holding a spinlock is invalid because spinlocks are used in atomic contexts where sleeping is not permitted.

### Q6. What is `CLOCK_MONOTONIC`?

A clock that measures elapsed time and is not affected by changes to the system's wall clock.

### Q7. What is the difference between `HRTIMER_MODE_REL` and `HRTIMER_MODE_ABS`?

- `HRTIMER_MODE_REL`: Expiry is relative to the current time.
- `HRTIMER_MODE_ABS`: Expiry is specified as an absolute time in the selected clock domain.

### Q8. Why use `hrtimer_cancel()` during module cleanup?

To cancel the timer and wait for an executing callback to finish before the module is unloaded.

### Q9. What is the purpose of `spin_lock_bh()`?

It disables local bottom-half processing while acquiring the lock, preventing local softirq processing from interrupting the protected process-context section.

### Q10. Why can a hard IRQ handler deadlock when acquiring a lock held by the interrupted process?

The handler spins while the interrupted process cannot resume to release the lock.

## 12. Next Topics

- More spinlock APIs and usage patterns
- `spin_trylock()`
- Locking across process, softirq, and hard IRQ contexts
- Deadlocks and lock ordering
- Lock performance analysis