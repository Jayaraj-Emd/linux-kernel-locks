# Multiple Device Instances

## What I wanted to understand

How a mutex can be placed inside
a device structure.

If a driver has multiple devices,
each device can have its own mutex.

---

# Device Structure

Example:

    struct my_device {
        int count;
        struct mutex lock;
    };

The structure contains:

    count
        ↓
    device data

    lock
        ↓
    protects the device data

---

# Multiple Device Instances

Create two device objects:

    struct my_device dev1;
    struct my_device dev2;

Each object contains its own:

    count
    lock

So:

    dev1
        ├── count
        └── lock

    dev2
        ├── count
        └── lock

---

# Mutex Initialization

Each device has its own mutex.

Initialize:

    mutex_init(&dev1.lock);
    mutex_init(&dev2.lock);

So:

    dev1.lock → initialized

    dev2.lock → initialized

---

# Why Put Mutex Inside the Structure?

In a real driver, we may have
multiple device instances.

For example:

    Device 1
        ↓
    struct my_device
        ├── count
        └── lock


    Device 2
        ↓
    struct my_device
        ├── count
        └── lock

Each device has its own data
and its own protection.

---

# Worker Threads

Create two kernel threads:

    worker1
        ↓
    dev1

    worker2
        ↓
    dev2

Pass the device object to the
thread function:

    kthread_run(thread_function,
                &dev1,
                "worker1");

    kthread_run(thread_function,
                &dev2,
                "worker2");

---

# Getting the Device Inside Thread

The thread function receives:

    void *data

Convert it to:

    struct my_device *dev = data;

For worker1:

    data
      ↓
    &dev1
      ↓
    dev1

For worker2:

    data
      ↓
    &dev2
      ↓
    dev2

---

# Protecting Device Data

Inside the thread:

    mutex_lock(&dev->lock);

    dev->count++;

    mutex_unlock(&dev->lock);

The important point is:

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

---

# Why Separate Mutexes?

Suppose:

    worker1 → Device 1
    worker2 → Device 2

They are accessing different
device data.

Therefore:

    worker1 → dev1.lock

    worker2 → dev2.lock

They don't need to wait for
each other's mutex.

---

# What If We Use One Global Mutex?

Suppose we use:

    static DEFINE_MUTEX(lock);

Then:

    worker1 → global lock → dev1

    worker2 → global lock → dev2

If worker1 owns the lock:

    worker1
       ↓
    global lock
       ↓
    dev1

    worker2
       ↓
    global lock
       ↓
    WAIT

worker2 waits even though it is
accessing Device 2.

---

# Global Mutex vs Per-Device Mutex

## Global Mutex

    static DEFINE_MUTEX(lock);

    Device 1 ──┐
    Device 2 ──┼──> one mutex
    Device 3 ──┘

All devices share the same lock.

This can create unnecessary blocking.

---

## Per-Device Mutex

    struct my_device {
        int count;
        struct mutex lock;
    };

    Device 1 → dev1.lock

    Device 2 → dev2.lock

    Device 3 → dev3.lock

Each device protects its own data.

---

# Experiment

Two device instances:

    dev1
    dev2

Two kernel threads:

    worker1 → dev1
    worker2 → dev2

Each thread performs:

    mutex_lock(&dev->lock);

    dev->count++;

    mutex_unlock(&dev->lock);

---

# Experiment Flow

    Module loaded
         ↓
    Create dev1
         ↓
    Create dev2
         ↓
    Initialize dev1.lock
         ↓
    Initialize dev2.lock
         ↓
    Create worker1
         ↓
    Create worker2
         ↓
    worker1 → dev1
    worker2 → dev2
         ↓
    worker1 → dev1.lock
    worker2 → dev2.lock
         ↓
    Update separate counters
         ↓
    Both threads finish
         ↓
    Print final counts

---

# Expected Result

If:

    ITERATIONS = 10

Then:

    dev1.count = 10

    dev2.count = 10

Expected:

    Device 1 final count = 10
    Device 2 final count = 10

---

# Important Observation

The important part is not only
the final count.

The important part is:

    worker1 → dev1.lock

    worker2 → dev2.lock

They are using different locks.

Therefore they don't need to
block each other.

---

# Real Driver Connection

A real driver can have multiple
instances of the same device.

For example:

    Device 1
        ↓
    struct my_device
        ├── device data
        └── mutex

    Device 2
        ↓
    struct my_device
        ├── device data
        └── mutex

Each device instance can protect
its own state independently.

---

# Static vs Per-Device Mutex

Global/fixed mutex:

    static DEFINE_MUTEX(lock);

    One mutex
       ↓
    shared by everything

Per-device mutex:

    struct my_device {
        int count;
        struct mutex lock;
    };

    mutex_init(&dev1.lock);
    mutex_init(&dev2.lock);

    Each object
        ↓
    gets its own mutex

---

# What I Learned

1. A mutex can be placed inside
   a structure.

2. Each instance of the structure
   gets its own mutex.

3. `mutex_init()` is used to
   initialize each instance.

4. The thread can receive the
   device structure through
   `kthread_run()`.

5. `dev->lock` protects the data
   belonging to that device.

6. A global mutex can cause
   unnecessary blocking between
   independent devices.

7. Per-device mutexes allow
   independent devices to work
   concurrently.

---

# Key Mental Model

    struct my_device
        │
        ├── device data
        │
        └── mutex
             ↓
        protects this
        device's data


    dev1 ──> dev1.lock ──> dev1.count

    dev2 ──> dev2.lock ──> dev2.count


    Different devices
          ↓
    Different locks
          ↓
    Independent access