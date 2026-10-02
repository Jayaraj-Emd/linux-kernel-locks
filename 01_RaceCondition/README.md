# Race Condition

## What I learned

Shared data:

    count

Two threads:

    worker1 ──┐
              ├──> count
    worker2 ──┘

Race condition:

    Both threads access/modify shared data
    without proper synchronization.

---

## count++

`count++` looks like one operation:

    READ count
       ↓
    ADD 1
       ↓
    WRITE count

Example:

    count = 100

    T1 → READ 100
    T2 → READ 100
    T1 → WRITE 101
    T2 → WRITE 101

    Expected = 102
    Actual   = 101

    → Lost update

Important:

    Race condition can exist even if
    the wrong result is not observed
    every time.

---

## Experiment 1 — Direct count++

Used:

    count++;

Expected:

    20000

Observed:

    Final    = 20000
    Expected = 20000

Why?

    The race window is very small.

So:

    Final == Expected

does NOT prove that the code is race-free.

---

## Experiment 2 — Make the Race Visible

To enlarge the race window:

    temp = count;
    msleep(10);
    count = temp + 1;

Now:

    READ
      ↓
    msleep(10)
      ↓
    WRITE

Two threads can read the same value
before either one writes the updated value.

Observed:

    Final    = 1693
    Expected = 20000

Observation:

    Many updates were lost.

---

## Why temp was used?

Normally:

    count++;

is what we want to demonstrate.

But the race window is very small.

Using:

    temp = count;
    msleep(10);
    count = temp + 1;

separates the READ and WRITE so that
another thread can run between them.

This is only for demonstrating the race.

---

## msleep() vs udelay()

### msleep()

    msleep(10);

    task sleeps
        ↓
    scheduler can run another task

### udelay()

    udelay(10);

    busy-wait
        ↓
    CPU keeps executing

Important:

    busy-wait != preemption disabled

`msleep()` was useful in this experiment because
it creates a clear scheduling opportunity.

---

## Concurrency vs Parallelism

### Multiple CPUs

    CPU 0              CPU 3
    worker1            worker2

    → Parallelism

Both can execute at the same time.

### Single CPU

    CPU 0
      ↓
    worker1
      ↓ preempt
    worker2
      ↓ preempt
    worker1

    → Concurrency

Race condition does NOT require multiple CPUs.

---

# Module Lifetime Problem

Initially:

    worker1 ──> Thread_function()
    worker2 ──> Thread_function()

If the module is removed while the
kernel threads are still executing code
belonging to the module:

    rmmod race
        ↓
    race.ko unloaded
        ↓
    worker still executing
        ↓
    worker executes module code
        ↓
    module memory is gone
        ↓
    Kernel Oops

Observed:

    BUG: unable to handle page fault
    Comm: worker1
    Comm: worker2

Lesson:

    Do not unload a module while its kthread
    can still execute module code.

---

# Completion

To safely wait for the worker threads
before unloading the module:

Worker:

    complete(done);

Module exit:

    wait_for_completion(&done);

Meaning:

    Worker:
        "I am finished."

    Module exit:
        "I will wait until you finish."

Flow:

    worker finishes
          ↓
    complete()
          ↓
    wait_for_completion() returns
          ↓
    module exit continues
          ↓
    module can safely unload

Important:

    Completion fixes thread lifetime.

    Completion does NOT fix the race condition.

---

# Missing complete() Bug

I initially forgot:

    complete(done);

But module exit contained:

    wait_for_completion(&done);

So:

    rmmod
       ↓
    race_exit()
       ↓
    wait_for_completion()
       ↓
    waiting forever
       ↓
    rmmod does not return

The module entered:

    initstate = going

and:

    /proc/modules

showed:

    Unloading

The `rmmod` process was still running.

---

## Why did rmmod wait forever?

The worker had no path to:

    complete(done);

Therefore:

    wait_for_completion()
            ↓
        no signal
            ↓
        wait forever

The important synchronization rule:

    wait_for_completion()
        must have a corresponding
        complete() execution path.

---

# Debugging the Stuck Module

## Check whether module is loaded

    lsmod | grep race

Example:

    race  12288  -1

This showed that the module was still present.

---

## Check module in sysfs

    ls /sys/module/race

If the module exists, its information
is available under:

    /sys/module/race/

---

## Check module state

    cat /sys/module/race/initstate

Observed:

    going

Meaning:

    The module was currently unloading.

---

## Check module reference count

    cat /sys/module/race/refcnt

Observed:

    -1

This was associated with the module
being in the unloading state.

---

## Check /proc/modules

    cat /proc/modules | grep '^race'

Observed:

    race 12288 -1 - Unloading ...

This confirmed that the module was
stuck during unloading.

---

## Check running rmmod / worker processes

    ps -ef | grep -E 'rmmod|worker1|worker2' | grep -v grep

Observed:

    rmmod race

was still running.

This confirmed that `rmmod` had not
finished.

---

## Check kernel messages

Clear old messages:

    sudo dmesg -C

Insert module:

    sudo insmod race.ko

Check messages:

    sudo dmesg

Or:

    sudo dmesg | tail -30

---

## Remove module

    sudo rmmod race

## Insert module

    sudo insmod race.ko

---

# EBUSY During insmod

When trying:

    sudo insmod race.ko

I received:

    insmod: ERROR: could not insert module race.ko:
    Device or resource busy

Checking:

    lsmod | grep race

showed:

    race 12288 -1

Then:

    cat /sys/module/race/initstate

showed:

    going

And:

    cat /proc/modules | grep '^race'

showed:

    race 12288 -1 - Unloading ...

Meaning:

    The previous module instance was
    still being unloaded.

So another copy could not be inserted.

---

# Recovery Lesson

If the worker is still running:

    worker
       ↓
    complete(done)
       ↓
    wait_for_completion() returns
       ↓
    module unloads

If the worker can never reach `complete()`:

    wait_for_completion()
           ↓
       forever

The module cannot safely finish unloading.

During kernel-module development, if the
module becomes unrecoverably stuck in its
exit path, a reboot may be required.

Important:

    Do not use forced module removal as a
    normal solution when kernel code may
    still be executing.

---

# What I learned from the Bug

The bug was not caused by `wait_for_completion()`.

The problem was:

    wait_for_completion()
            ↓
    no complete()

The correct design is:

    Worker
       ↓
    does work
       ↓
    complete(done)
       ↓
    return

and:

    rmmod
       ↓
    module_exit()
       ↓
    wait_for_completion()
       ↓
    worker signals completion
       ↓
    exit continues
       ↓
    module unloaded

---

# Key Difference

Completion:

    "Wait until this event/thread finishes."

Mutex:

    "Only one execution context can access
     the protected critical section at a time."

So:

    completion
        → thread/module lifetime

    mutex
        → protect shared data

Completion does NOT solve:

    count++

race.

A mutex will be used for that.

---

# Next

1. Bind both threads to the same CPU
2. Observe the race again
3. Start Mutex
4. Protect `count` using mutex
5. Compare results
6. Understand why mutex fixes the race
7. Start Spinlock