# Mutex Lock Interruptible

## What I wanted to understand

I already learned:

    mutex_lock()
         ↓
    Lock is busy
         ↓
       WAIT
         ↓
    Lock becomes free
         ↓
    Continue

Now I wanted to understand:

    What happens if a thread is
    waiting for a mutex and a
    signal arrives?

Linux provides:

    mutex_lock_interruptible()

---

# mutex_lock() vs mutex_lock_interruptible()

## mutex_lock()

    mutex_lock(&lock);

If the mutex is busy:

    mutex_lock()
         ↓
    WAIT
         ↓
    Lock becomes free
         ↓
    Acquire lock
         ↓
    Continue

---

## mutex_lock_interruptible()

    mutex_lock_interruptible(&lock);

If the mutex is busy:

    mutex_lock_interruptible()
         ↓
    WAIT
         ↓
    Signal arrives
         ↓
    Stop waiting
         ↓
    Return error

So:

    mutex_lock()

        Wait until the mutex
        becomes available.


    mutex_lock_interruptible()

        Wait for the mutex,
        but allow the wait to
        be interrupted by a signal.

---

# Return Value

Unlike:

    mutex_lock(&lock);

`mutex_lock_interruptible()` returns
a value.

Example:

    ret = mutex_lock_interruptible(&lock);

    if (ret) {

        pr_info("Interrupted\n");

        return ret;
    }

    /* Critical section */

    mutex_unlock(&lock);

If:

    ret == 0

then:

    Mutex was successfully acquired.

If:

    ret != 0

then:

    Mutex was NOT acquired.

Therefore:

    Do NOT call mutex_unlock()

when the function returns an error.

---

# Experiment

I used three kernel threads:

    worker1
    worker2
    worker3

Their responsibilities are different.

    worker1
        ↓
    Acquire mutex
        ↓
    Hold mutex for 10 seconds


    worker2
        ↓
    mutex_lock_interruptible()
        ↓
    Wait for mutex


    worker3
        ↓
    Send signal to worker2
        ↓
    Interrupt worker2

---

# Why Three Threads?

I need one thread to hold the
mutex:

    worker1
        ↓
    LOCK
        ↓
    HOLD

I need another thread to wait:

    worker2
        ↓
    mutex_lock_interruptible()
        ↓
    WAIT

I need another thread to generate
the signal:

    worker3
        ↓
    send_sig()
        ↓
    worker2

So:

    worker1 → holds mutex

    worker2 → waits for mutex

    worker3 → interrupts worker2

---

# Worker 1

Worker1 uses:

    mutex_lock(&lock);

Then:

    msleep(10000);

So:

    worker1
        ↓
    mutex_lock()
        ↓
    Acquired
        ↓
    Sleep 10 seconds
        ↓
    mutex_unlock()

This keeps the mutex busy while
worker2 tries to acquire it.

---

# Worker 2

Worker2 uses:

    mutex_lock_interruptible(&lock);

Before that:

    allow_signal(SIGTERM);

This allows SIGTERM to be used
to interrupt the kernel thread.

Flow:

    worker2
        ↓
    mutex_lock_interruptible()
        ↓
    mutex busy
        ↓
    WAIT

Worker2 is now waiting for
the mutex.

---

# Worker 3

Worker3 waits for 1 second:

    msleep(1000);

Then:

    send_sig(SIGTERM, thread2, 1);

So:

    worker3
        ↓
    send_sig()
        ↓
    SIGTERM
        ↓
    worker2

The signal makes the waiting
operation interruptible.

---

# Experiment Flow

    Module loaded
         ↓
    Initialize mutex
         ↓
    Create worker1
         ↓
    Create worker2
         ↓
    Create worker3
         ↓

    worker1
         ↓
    mutex_lock()
         ↓
    Acquired mutex
         ↓
    Sleep 10 seconds

    worker2
         ↓
    mutex_lock_interruptible()
         ↓
    Mutex busy
         ↓
    WAIT

    worker3
         ↓
    Sleep 1 second
         ↓
    send_sig(SIGTERM, worker2)
         ↓
    worker2 receives signal
         ↓
    Waiting operation interrupted
         ↓
    mutex_lock_interruptible()
    returns error
         ↓
    worker2 exits

    worker1
         ↓
    wakes up
         ↓
    mutex_unlock()

---

# Output

Expected output:

    mutex module loaded...

    worker1: Acquired lock...

    worker2: PID = xxxx

    worker2: Waiting for lock...

    worker3: Sending SIGTERM to worker2...

    worker3: Signal sent

    worker2: Interrupted while waiting!

    worker2: return value = -4

    worker1: Released lock...

---

# Understanding the Output

First:

    worker1: Acquired lock...

This means:

    worker1
        ↓
    owns mutex

Then:

    worker2: Waiting for lock...

Worker2 calls:

    mutex_lock_interruptible()

But the mutex is already owned
by worker1.

Therefore:

    worker2
        ↓
    WAIT

Then:

    worker3: Sending SIGTERM to worker2...

Worker3 sends:

    SIGTERM

to worker2.

Then:

    worker2: Interrupted while waiting!

This means the wait was interrupted
before worker2 obtained the mutex.

---

# Return Value

The output:

    return value = -4

means:

    -4 = -EINTR

`EINTR` means:

    Interrupted system call

For our experiment:

    mutex_lock_interruptible()
             ↓
        interrupted
             ↓
          -EINTR

The important point is:

    worker2 did NOT acquire
    the mutex.

Therefore:

    worker2 must NOT call
    mutex_unlock()

---

# Why worker2 Does Not Unlock

Code:

    ret = mutex_lock_interruptible(&lock);

    if (ret) {

        pr_info("Interrupted\n");

        return 0;
    }

    mutex_unlock(&lock);

If the function returns:

    ret != 0

then:

    Mutex was not acquired.

So:

    mutex_unlock()

must not be called.

---

# Important Difference From Trylock

Previously:

    mutex_trylock()

If mutex is busy:

    trylock()
        ↓
    return immediately

No waiting.

Now:

    mutex_lock_interruptible()

If mutex is busy:

    lock_interruptible()
        ↓
    WAIT

But:

    signal
        ↓
    interrupt the wait
        ↓
    return error

So:

    mutex_trylock()

        "Try now.
         Don't wait."


    mutex_lock_interruptible()

        "Wait for the lock,
         but allow the wait
         to be interrupted."

---

# mutex_lock() vs interruptible vs trylock

    mutex_lock()

        Lock busy
            ↓
          WAIT
            ↓
        Eventually acquire


    mutex_lock_interruptible()

        Lock busy
            ↓
          WAIT
            ↓
        Signal arrives
            ↓
        Stop waiting
            ↓
        Return error


    mutex_trylock()

        Lock busy
            ↓
        Return immediately