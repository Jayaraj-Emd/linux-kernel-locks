# Mutex Trylock

## What I wanted to understand

I already learned:

    mutex_lock()
         ↓
    Critical Section
         ↓
    mutex_unlock()

If the mutex is already locked:

    mutex_lock()
         ↓
    WAIT
         ↓
    Lock becomes free
         ↓
    Continue

Now I wanted to understand:

    What if I don't want to wait
    for the mutex?

Linux provides:

    mutex_trylock()

---

# What is mutex_trylock()?

`mutex_trylock()` tries to acquire
the mutex without waiting.

Example:

    if (mutex_trylock(&lock)) {

        /* Lock acquired */

        mutex_unlock(&lock);
    }
    else {

        /* Lock is already taken */
    }

The important difference:

    mutex_lock()
         ↓
    Lock busy?
         ↓
       WAIT


    mutex_trylock()
         ↓
    Lock busy?
         ↓
    Return immediately

---

# Return Value

    mutex_trylock(&lock)

Returns:

    1 → Lock successfully acquired

    0 → Lock is already held

Example:

    if (mutex_trylock(&lock)) {

        pr_info("Lock acquired\n");

        mutex_unlock(&lock);
    }
    else {

        pr_info("Lock is busy\n");
    }

If `trylock()` returns 0:

    Current thread did NOT
    acquire the mutex.

Therefore:

    Do NOT call mutex_unlock()

---

# Experiment

I created two kernel threads:

    worker1
    worker2

Both use the same mutex.

Flow:

    worker1
        ↓
    mutex_lock()
        ↓
    Acquired lock
        ↓
    sleep for 1 second
        ↓
    mutex_unlock()


    worker2
        ↓
    wait 100 ms
        ↓
    mutex_trylock()
        ↓
    Lock is already held
        ↓
    Return immediately

---

# Worker 1

Worker1 uses normal:

    mutex_lock()

Code:

    mutex_lock(&lock);

    pr_info("worker1: Acquired lock...\n");

    msleep(1000);

    mutex_unlock(&lock);

    pr_info("worker1: Released lock...\n");

So worker1 keeps the mutex
for approximately 1 second.

---

# Worker 2

Worker2 waits for 100 ms:

    msleep(100);

Then:

    if (mutex_trylock(&lock)) {

        pr_info("worker2: Acquired lock...\n");

        mutex_unlock(&lock);

        pr_info("worker2: Released lock...\n");
    }
    else {

        pr_info("worker2: Failed to acquire lock...\n");
    }

At this time worker1 is still
holding the mutex.

Therefore:

    mutex_trylock()
         ↓
    Lock busy
         ↓
    Return 0
         ↓
    worker2 continues

worker2 does NOT wait.

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
    worker1 acquires mutex
         ↓
    worker1 sleeps
         ↓
    worker2 starts
         ↓
    worker2 calls trylock()
         ↓
    Mutex is busy
         ↓
    trylock() returns 0
         ↓
    worker2 continues
         ↓
    worker1 wakes up
         ↓
    worker1 releases mutex
         ↓
    Both threads finish
         ↓
    Module unloaded

---

# Output

Observed:

    worker1: Acquired lock...
    worker2: Failed to acquire lock...
    worker1: Released lock...

This shows:

    worker1
        ↓
    LOCK
        ↓
    holds mutex

    worker2
        ↓
    trylock()
        ↓
    FAIL
        ↓
    returns immediately

    worker1
        ↓
    UNLOCK

---

# mutex_lock() vs mutex_trylock()

## mutex_lock()

    mutex_lock(&lock);

If the lock is free:

    mutex_lock()
         ↓
    Acquire
         ↓
    Continue

If the lock is busy:

    mutex_lock()
         ↓
    WAIT
         ↓
    Lock becomes free
         ↓
    Acquire
         ↓
    Continue

---

## mutex_trylock()

    mutex_trylock(&lock);

If the lock is free:

    trylock()
         ↓
    Acquire
         ↓
    Return 1

If the lock is busy:

    trylock()
         ↓
    Return 0
         ↓
    Continue

No waiting.

---

# What Happens If I Trylock Again?

Suppose worker2 does:

    if (mutex_trylock(&lock)) {
        ...
    }
    else {
        pr_info("Failed\n");
    }

    if (mutex_trylock(&lock)) {
        ...
    }
    else {
        pr_info("Failed again\n");
    }

If worker1 is still holding
the mutex:

    First trylock
         ↓
       FAIL
         ↓
    Return 0

    Second trylock
         ↓
       FAIL
         ↓
    Return 0

Each `mutex_trylock()` is
a separate attempt.

It does NOT automatically retry.

---

# trylock() Does Not Mean "Wait a Little"

Important:

    mutex_trylock()

does NOT mean:

    Try
      ↓
    Wait
      ↓
    Try again

It means:

    Try NOW
       ↓
    ┌───────────────┐
    │               │
    ↓               ↓
 Success          Failure
    ↓               ↓
 Acquire          Return 0
    ↓               ↓
 Continue         Continue

If retry behavior is required,
the code must implement it.

---

# Why Completion Is Used

The mutex and completion solve
different problems.

Mutex:

    mutex_lock()
         ↓
    Protect shared data
         ↓
    mutex_unlock()

Completion:

    complete()
         ↓
    Tell another execution context
    that something has finished

In this experiment:

    worker1
        ↓
    complete(&done1)

    worker2
        ↓
    complete(&done2)

Module exit:

    wait_for_completion(&done1);
    wait_for_completion(&done2);

So:

    Mutex
        ↓
    Protect shared data

    Completion
        ↓
    Wait for thread completion

---

# Real Driver Connection

Suppose a driver has a device
that is protected by a mutex.

Normally:

    mutex_lock(&dev->lock);

    /* use device */

    mutex_unlock(&dev->lock);

The caller may have to wait.

Sometimes the driver may want
to check whether the device is
currently available.

Then:

    if (mutex_trylock(&dev->lock)) {

        /* use device */

        mutex_unlock(&dev->lock);
    }
    else {

        /* Device is busy */
    }

So the mental model is:

    mutex_lock()
        ↓
    "I need the resource.
     I can wait."

    mutex_trylock()
        ↓
    "Try now.
     If busy, I'll handle it."

---

# Important Observation

`mutex_trylock()` itself does not
wait for the mutex.

It only performs one attempt.

So:

    trylock()
        ↓
    success
        → lock acquired

    trylock()
        ↓
    failure
        → return immediately

If the application wants to
retry, the application must
decide when and how to retry.

---

# What I Learned

- `mutex_trylock()` is used to
  attempt to acquire a mutex
  without waiting.

- Return `1` means the mutex
  was acquired.

- Return `0` means the mutex
  was already held.

- If `trylock()` fails, the
  thread does not wait.

- If `trylock()` fails, I should
  NOT call `mutex_unlock()`.

- Calling `trylock()` again is
  another independent attempt.

- `trylock()` does not automatically
  retry.

- Mutex protects shared data.

- Completion is used for
  synchronization of thread
  completion/lifetime.

---

# Key Mental Model

    mutex_lock()

        "I need the lock.
         I can wait."

            ↓

        WAIT if busy


    mutex_trylock()

        "Try to get the lock
         right now."

            ↓

        Success → acquire

        Failure → return immediately