# Mutex

## What I learned

Mutex = Mutual Exclusion

Purpose:

    Protect shared data
    from concurrent access.

Example:

    worker1 ──┐
              ├──> count
    worker2 ──┘

Without protection:

    Both threads can access
    count at the same time.

With mutex:

    Only one thread can enter
    the protected critical section
    at a time.

---

## Basic Mutex Flow

    mutex_lock()
         ↓
    Critical Section
         ↓
    mutex_unlock()

Example:

    mutex_lock(&lock);

    count++;

    mutex_unlock(&lock);

The critical section is the code
between lock and unlock.

---

# Mutex Initialization

There are two common ways to initialize
a mutex.

---

## Static Initialization

Use:

    static DEFINE_MUTEX(lock);

The mutex is initialized as part of
the definition.

Example:

    static DEFINE_MUTEX(count_lock);

Then directly use:

    mutex_lock(&count_lock);

    count++;

    mutex_unlock(&count_lock);

No separate:

    mutex_init()

is required.

Use this when the mutex is a
fixed/global object.

Example:

    static DEFINE_MUTEX(count_lock);

    worker1 ──┐
              ├──> count
    worker2 ──┘

    One fixed mutex protects count.

---

## Dynamic Initialization

First declare the mutex:

    struct mutex lock;

Then initialize it:

    mutex_init(&lock);

Example:

    struct my_device {
        int count;
        struct mutex lock;
    };

Each device object has its own
mutex.

Initialize:

    mutex_init(&dev1.lock);
    mutex_init(&dev2.lock);

---

# Why Put a Mutex Inside a Structure?

In a real driver, we may have
multiple devices.

For example:

    Device 1
        count
        mutex

    Device 2
        count
        mutex

    Device 3
        count
        mutex

We can represent one device using
a structure:

    struct my_device {
        int count;
        struct mutex lock;
    };

Now create two device objects:

    struct my_device dev1;
    struct my_device dev2;

Each object has its own data
and its own mutex:

    dev1
     ├── count
     └── lock

    dev2
     ├── count
     └── lock

---

## Why Separate Mutexes?

Suppose:

    worker1 → Device 1
    worker2 → Device 2

They are accessing different devices.

So:

    worker1
       ↓
    dev1.lock
       ↓
    dev1.count

    worker2
       ↓
    dev2.lock
       ↓
    dev2.count

worker1 does not need to block
worker2.

They can work independently.

---

## What If We Use One Global Mutex?

For example:

    static DEFINE_MUTEX(lock);

Then:

    worker1 → global lock → Device 1
    worker2 → global lock → Device 2

If worker1 owns the lock:

    worker1 → Device 1
                  ↓
                LOCK

    worker2 → Device 2
                  ↓
                WAIT

Even though they are accessing
different devices.

This creates unnecessary blocking.

---

# Global Mutex vs Per-Device Mutex

## Global Mutex

    static DEFINE_MUTEX(lock);

    One lock
       ↓
    protects multiple devices

    Device 1 ──┐
    Device 2 ──┼──> one mutex
    Device 3 ──┘

---

## Per-Device Mutex

    struct my_device {
        int count;
        struct mutex lock;
    };

    Device 1 → its own mutex
    Device 2 → its own mutex
    Device 3 → its own mutex

This allows independent devices
to be accessed concurrently.

---

# Static vs Dynamic — Simple Difference

    Static:

        static DEFINE_MUTEX(lock);

        Fixed/global mutex
              ↓
        One known mutex object


    Dynamic:

        struct mutex lock;
        mutex_init(&lock);

        Mutex belongs to an object/structure
              ↓
        Each object can have its own mutex

---

## Real Driver Example

Suppose a driver supports multiple devices:

    Device 1
       ↓
    struct my_device
       ├── data
       └── lock

    Device 2
       ↓
    struct my_device
       ├── data
       └── lock

Each device gets its own mutex:

    mutex_init(&dev1.lock);
    mutex_init(&dev2.lock);

So:

    worker1 → dev1.lock → dev1.data

    worker2 → dev2.lock → dev2.data

They can work independently.

---

## Why mutex_init() Exists

For a fixed global mutex:

    static DEFINE_MUTEX(lock);

The mutex is initialized as part
of the definition.

For a mutex inside a structure:

    struct my_device {
        int count;
        struct mutex lock;
    };

Each device object needs its own
mutex initialization:

    mutex_init(&dev1.lock);
    mutex_init(&dev2.lock);

So:

    Global/fixed mutex
        ↓
    DEFINE_MUTEX()

    Mutex inside a device/object
        ↓
    mutex_init()

---

# Mutex Experiment

Two kernel threads:

    worker1
    worker2

Both execute:

    count++;

Shared variable:

    static int count = 0;

Expected:

    worker1 = ITERATIONS
    worker2 = ITERATIONS

Therefore:

    Expected = 2 × ITERATIONS

---

# Without Mutex

The operation:

    count++;

is conceptually:

    READ count
       ↓
    ADD 1
       ↓
    WRITE count

Possible execution:

    T1 → READ 100
    T2 → READ 100
    T1 → WRITE 101
    T2 → WRITE 101

Expected:

    102

Actual:

    101

    → Lost update

---

# With Mutex

Code:

    mutex_lock(&lock);

    count++;

    mutex_unlock(&lock);

Execution:

    T1 → LOCK
         count++
         UNLOCK

    T2 → LOCK
         count++
         UNLOCK

If T2 tries to lock while T1 owns
the mutex:

    T1 → LOCK
         count++

    T2 → mutex_lock()
             ↓
           WAIT

    T1 → mutex_unlock()
             ↓
    T2 → gets the mutex
         count++
         UNLOCK

---

# Scheduler and Mutex

The scheduler can still switch
between worker1 and worker2.

Mutex does NOT stop scheduling.

Example:

    worker1
       ↓
    mutex_lock()
       ↓
    count++
       ↓
    preempted
       ↓
    worker2
       ↓
    mutex_lock()
       ↓
    WAIT

worker2 can run, but it cannot
enter the critical section because
worker1 owns the mutex.

Then:

    worker1 resumes
       ↓
    mutex_unlock()
       ↓
    worker2 gets mutex
       ↓
    count++

So:

    Scheduler
        → decides which runnable
          task executes

    Mutex
        → controls access to
          shared data

---

# Why msleep() Is Not Required

In the race-condition experiment,
`msleep()` was used to make the
race window large.

Example:

    temp = count;
    msleep(10);
    count = temp + 1;

This makes it easier to observe
the lost update.

For the mutex experiment:

    mutex_lock(&lock);
    count++;
    mutex_unlock(&lock);

is enough.

The scheduler can run both threads
without `msleep()`.

---

# Why Second Thread Runs Without sleep

Both kernel threads are runnable
after:

    kthread_run()

The scheduler decides when each
thread gets CPU time.

Single CPU example:

    CPU 0
      ↓
    worker1
      ↓
    worker1
      ↓
    preempt
      ↓
    worker2
      ↓
    worker2
      ↓
    preempt
      ↓
    worker1

This is concurrency.

Multiple CPUs can execute them
at the same time.

    CPU 0              CPU 1

    worker1            worker2

This is parallelism.

Important:

    msleep() is NOT required
    for concurrency.

It was only used earlier to
make the race easier to observe.

---

# Why Not Sleep Inside the Mutex?

Avoid:

    mutex_lock(&lock);

    count++;
    msleep(10);

    mutex_unlock(&lock);

The mutex stays locked while
the thread sleeps.

Example:

    T1 → LOCK
         count++
         sleep
              │
              │ T2 waits
              │
         wake
         UNLOCK
              ↓
         T2 continues

The second thread is blocked even
though the protected operation
has already completed.

The critical section should normally
be kept as small as practical.

---

# Printing Thread Information

To identify the current kernel thread:

    current->comm

Example:

    pr_info("Running thread: %s\n",
            current->comm);

To print the thread PID:

    pr_info("Thread: %s, PID: %d\n",
            current->comm,
            current->pid);

Example:

    Started: worker1, PID: 1234
    Started: worker2, PID: 1235

Important:

    Do not print inside a huge loop.

For example:

    ITERATIONS = 50000000

and:

    pr_info() inside every iteration

would generate an enormous amount
of kernel logging and distort the
experiment.

Print only when useful, such as
when the thread starts.

---

# Mutex Rules

1. Lock before accessing protected
   shared data.

2. Unlock after the critical section.

3. Do not forget `mutex_unlock()`.

4. Keep the critical section small.

5. Do not unnecessarily sleep while
   holding the mutex.

6. The mutex protects the shared data;
   it does not stop scheduling.

7. A mutex can sleep while waiting
   for another owner to release it.

---

# Completion vs Mutex

These solve different problems.

## Completion

    complete(&done);

    wait_for_completion(&done);

Purpose:

    Wait for an event/thread to finish.

Used for:

    Worker lifetime
    Module unload synchronization

Meaning:

    "Wait until this is finished."

---

## Mutex

    mutex_lock(&lock);

    /* shared data */

    mutex_unlock(&lock);

Purpose:

    Protect shared data from
    concurrent access.

Meaning:

    "Only one execution context
     can enter here at a time."

---

# What Mutex Solved

Without mutex:

    Race condition
         ↓
    Lost updates
         ↓
    Final != Expected

With mutex:

    mutex_lock()
         ↓
    count++
         ↓
    mutex_unlock()
         ↓
    Protected access

    Final == Expected

Important:

    Mutex does not stop concurrency.

    Mutex makes concurrent access
    to shared data safe.

---

# Key Mental Model

    Scheduler
        ↓
    Allows multiple runnable
    threads to make progress

    Mutex
        ↓
    Controls access to shared data

    Completion
        ↓
    Synchronizes an event/lifetime

---

# Next

1. Understand mutex limitations.
2. Learn why mutex cannot be used
   in every kernel execution context.
3. Understand why spinlocks exist.
4. Learn:
       spin_lock()
       spin_unlock()
5. Compare mutex vs spinlock.
6. Start Spinlock experiment.