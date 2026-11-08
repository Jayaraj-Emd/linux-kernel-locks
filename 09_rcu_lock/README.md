# 09 - Read-Copy-Update (RCU) Lock

## 1. Objective

The objective of this experiment is to understand Read-Copy-Update (RCU), a synchronization mechanism provided by the Linux kernel.

RCU is particularly useful when multiple threads frequently read shared data, while updates happen less frequently.

In this experiment, we demonstrate:

- How readers access shared data using RCU.
- How a writer creates and publishes a new configuration.
- How `synchronize_rcu()` waits for pre-existing readers.
- Why the old configuration cannot be freed immediately after updating the pointer.
- How RCU provides safe memory reclamation.

---

## 2. Why Do We Need RCU?

Consider a shared configuration structure accessed by multiple reader threads.

For example:

```c
struct config {
    int version;
    int speed;
};
```

Multiple readers may access this configuration simultaneously.

A writer occasionally needs to update the configuration.

One approach is to protect all accesses using a mutex.

However, mutexes introduce locking overhead, and readers cannot execute concurrently while holding the same mutex.

RCU provides another approach:

1. Readers access the existing object.
2. The writer creates a new object.
3. The writer publishes the new object.
4. Existing readers can finish accessing the old object.
5. The writer waits for a grace period.
6. The writer frees the old object.

The main advantage is that RCU readers do not need to acquire a traditional mutex or spinlock.

---

## 3. Core RCU Concepts

### 3.1 RCU Read-Side Critical Section

A reader enters an RCU read-side critical section using:

```c
rcu_read_lock();
```

It accesses the shared pointer using:

```c
cfg = rcu_dereference(current_config);
```

After accessing the data, it exits using:

```c
rcu_read_unlock();
```

Example:

```c
rcu_read_lock();

cfg = rcu_dereference(current_config);

if (cfg) {
    version = cfg->version;
    speed = cfg->speed;
}

rcu_read_unlock();
```

The reader can safely access the object while inside the read-side critical section, provided the writer follows the RCU update and reclamation rules.

**Important:** Normal RCU read-side critical sections must not block or sleep.

### 3.2 RCU Pointer

The shared pointer is declared using `__rcu`:

```c
static struct config __rcu *current_config;
```

The `__rcu` annotation identifies a pointer protected by RCU.

It also helps kernel static analysis tools identify incorrect pointer usage.

### 3.3 Publishing a New Object

The writer allocates and initializes a new object:

```c
new_config = kmalloc(sizeof(*new_config), GFP_KERNEL);

new_config->version = 2;
new_config->speed = 200;
```

The new object is published using:

```c
rcu_assign_pointer(current_config, new_config);
```

This replaces the shared pointer with the new object.

Readers that subsequently access the pointer can observe the new configuration.

### 3.4 Grace Period

A grace period is the period required for all relevant pre-existing RCU read-side critical sections to finish.

The writer waits for this period using:

```c
synchronize_rcu();
```

Consider this example:

- Reader 1 is accessing version 1.
- Reader 2 is accessing version 1.
- The writer publishes version 2.
- Reader 3 starts reading and accesses version 2.
- Reader 1 and Reader 2 finish their existing read-side critical sections.
- The grace period completes.

Now the writer can safely free version 1.

**Important:** `synchronize_rcu()` does not wait for every reader in the system. It waits for the relevant pre-existing read-side critical sections.

### 3.5 Freeing the Old Object

After the grace period:

```c
synchronize_rcu();

kfree(old_config);
```

The old object can now be freed because readers that could have been using it have finished their read-side critical sections.

---

## 4. Important RCU APIs

| API | Purpose |
|---|---|
| `rcu_read_lock()` | Begins an RCU read-side critical section |
| `rcu_read_unlock()` | Ends an RCU read-side critical section |
| `rcu_dereference()` | Safely accesses an RCU-protected pointer |
| `rcu_assign_pointer()` | Publishes a new pointer |
| `synchronize_rcu()` | Waits for a grace period |
| `RCU_INIT_POINTER()` | Initializes or assigns an RCU pointer when its usage requirements permit |
| `kfree_rcu()` | Defers freeing an object until it is safe to reclaim |

---

## 5. Implementation Architecture

### Reader Thread

The reader thread continuously accesses the shared configuration.

Its flow is:

1. Enter the RCU read-side critical section.
2. Obtain the shared pointer using `rcu_dereference()`.
3. Read the configuration fields.
4. Exit the RCU read-side critical section.
5. Sleep outside the RCU critical section.

### Writer

The writer updates the configuration.

Its flow is:

1. Allocate memory for a new configuration.
2. Initialize the new configuration.
3. Obtain the old configuration pointer.
4. Publish the new pointer using `rcu_assign_pointer()`.
5. Wait for the grace period using `synchronize_rcu()`.
6. Free the old configuration.

### Module Cleanup

During module removal:

1. Stop the reader thread.
2. Remove the shared pointer.
3. Wait for a grace period.
4. Free the final configuration.

---

## 6. Project Files

```text
09_rcu_lock/
│
├── rcu_demo.c
├── Makefile
└── README.md
```

---

## 7. Build and Execution

### Build

```bash
make clean
make
```

### Load the Module

```bash
sudo insmod rcu_demo.ko
```

### Check Kernel Logs

```bash
sudo dmesg | tail -40
```

### Remove the Module

```bash
sudo rmmod rcu_demo
```

### Check Cleanup Logs

```bash
sudo dmesg | tail -20
```

---

## 8. Expected Kernel Logs

The exact ordering of reader and writer messages can vary.

Representative output:

```text
RCU demo: module loading
RCU: Reader thread started
RCU Reader: version=1, speed=100
RCU Writer: published version=2, speed=200
RCU Writer: grace period completed
RCU Writer: old configuration freed
RCU Reader: version=2, speed=200
RCU demo: module loaded successfully
```

During module removal:

```text
RCU demo: module unloading
RCU: Reader thread stopping
RCU demo: module unloaded
```

---

## 9. RCU vs Mutex

| Feature | RCU | Mutex |
|---|---|---|
| Primary use | Read-heavy workloads | General mutual exclusion |
| Reader synchronization | RCU read-side critical section | Mutex lock |
| Multiple readers | Can execute concurrently | Can execute concurrently if using separate lock acquisition intervals |
| Writer behavior | Publishes a new version and waits for a grace period before reclaiming old data | Acquires the mutex before modifying protected data |
| Reader blocking | Normal RCU readers do not block on the RCU mechanism | Readers may sleep while waiting for the mutex |
| Memory reclamation | Requires a grace period or another valid RCU reclamation mechanism | Usually handled while holding the appropriate lock |

---

## 10. Key Learnings

1. RCU is useful for read-heavy workloads.
2. Readers can access shared data without acquiring a traditional lock.
3. Writers create and publish a new object instead of modifying the existing object in place.
4. `rcu_assign_pointer()` publishes the new object.
5. `synchronize_rcu()` waits for relevant pre-existing readers.
6. The old object must not be freed before the grace period completes.
7. RCU read-side critical sections must not sleep in the normal usage demonstrated here.

---

## 11. Limitations and Future Experiments

This experiment demonstrates the basic RCU reader-writer flow.

The current implementation does not deliberately hold a reader inside an RCU critical section while the writer publishes a new object.

Future experiments:

- Demonstrate a controlled grace-period wait.
- Compare RCU and mutex performance.
- Measure read-side overhead.
- Explore `kfree_rcu()`.
- Explore RCU callback-based reclamation using `call_rcu()`.
- Study RCU usage in actual Linux kernel subsystems.

---

## 12. References

- Linux Kernel Documentation: RCU
- Linux Kernel Source: `include/linux/rcupdate.h`
- Linux Kernel Source: `include/linux/rculist.h`