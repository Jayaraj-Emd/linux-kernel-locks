# Mutex Lock Killable

## What I wanted to understand

I already experimented with:

    mutex_lock()
    mutex_trylock()
    mutex_lock_interruptible()

Now I want to understand:

    mutex_lock_killable()

Specifically:

    Can a fatal signal interrupt
    a thread waiting for a mutex?

---

## API

    ret = mutex_lock_killable(&lock);

Return value:

    0
       ↓
    Mutex acquired

    Negative error
       ↓
    Waiting interrupted by
    a fatal signal

If the function returns an error,
the thread did not acquire the mutex.

Therefore, do not call mutex_unlock()
on the failure path.

---

## Experiment Design

I used three kernel threads.

    worker1
       ↓
    Acquires mutex
       ↓
    Holds mutex for 10 seconds


    worker2
       ↓
    Calls mutex_lock_killable()
       ↓
    Waits for mutex


    worker3
       ↓
    Sends SIGKILL to worker2

The objective is to investigate
what happens when a fatal signal
targets a thread waiting for a mutex.

---

## Signal Used

    SIGKILL

SIGKILL cannot be caught, blocked,
or ignored.

Unlike SIGTERM, it does not allow
the target thread to handle the
signal normally.

No allow_signal(SIGKILL) call
is required.

---

## Important Observation

Sending SIGKILL to a kernel thread
does not guarantee that the thread
will return normally from
mutex_lock_killable().

The thread may terminate before
executing its remaining code.

Therefore, I cannot assume that
worker2 will always execute:

    complete(&done2);

If module exit waits for done2
without that completion occurring,
module unloading can hang.

---

## What I Learned

- mutex_lock_killable() allows a
  mutex wait to be interrupted by
  a fatal signal.

- SIGKILL cannot be caught or ignored.

- A kernel thread is not the same
  as a normal userspace process.

- A fatal signal can terminate a
  kernel thread before its normal
  cleanup code executes.

- Thread termination and completion
  signaling must be handled carefully
  when designing kernel experiments.

---

## Key Mental Model

    mutex_lock_killable()
             ↓
       Wait for mutex
             ↓
       Fatal signal pending
             ↓
       Killable wait can return
       an error

    However:

       SIGKILL may terminate
       the kernel thread.

    Do not assume the thread
    will execute normal cleanup.
