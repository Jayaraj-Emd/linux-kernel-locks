# 05 - Linux Kernel Read-Write Semaphore (`rw_semaphore`)

## 1. Objective

Understand how Linux kernel read-write semaphores work and how they synchronize multiple kernel threads accessing shared data.

In this experiment, two reader threads and one writer thread access a shared integer variable.

The readers can access the variable simultaneously, but the writer requires exclusive access.

This experiment demonstrates:

- Read-side locking.
- Write-side locking.
- Concurrent readers.
- Exclusive writers.
- Blocking when a writer encounters active readers.
- Synchronization of shared data using `rw_semaphore`.

## 2. What Is an `rw_semaphore`?

An `rw_semaphore` is a Linux kernel synchronization mechanism that allows multiple readers or one writer to access protected data.

It provides two types of locking:

**Read lock**
- Multiple readers can hold the read lock simultaneously.
- Readers can access shared data concurrently.
- A writer cannot acquire the write lock while readers hold their read locks.

**Write lock**
- Only one writer can hold the write lock.
- No reader can hold a read lock while the writer holds the write lock.
- Other writers must wait until the current writer releases the lock.

### 2.1 Multiple Readers

Suppose two threads want to read the same shared variable.

```text
             Shared Variable
                    |
           +--------+--------+
           |                 |
        Reader 1          Reader 2
           |                 |
           +--------+--------+
                    |
           Both read simultaneously
```

Both readers can hold the read lock simultaneously.

This is useful when reading data is a frequent operation and modifying the data is relatively infrequent.

### 2.2 Writer Waiting for Readers

Suppose Reader 1 and Reader 2 hold read locks, and Writer 1 requests the write lock.

```text
Reader 1              Reader 2              Writer 1
   |                     |                     |
   | down_read()         |                     |
   |                     |                     |
   |                     | down_read()         |
   |                     |                     |
   |                     |                     | down_write()
   |                     |                     | Must wait
   |                     |                     |
   | up_read()           |                     |
   |                     |                     | Still waiting
   |                     |                     |
   |                     | up_read()           |
   |                     |                     |
   |                     |                     | Acquires write lock
   |                     |                     |
   |                     |                     | Modifies data
   |                     |                     |
   |                     |                     | up_write()
```

The writer cannot acquire the write lock until the existing readers have released their read locks.

Once the writer acquires the write lock, it has exclusive access to the protected data.

The precise ordering of waiting readers and writers depends on the kernel implementation. Do not assume that a waiting writer will always be scheduled immediately after the current readers finish.

## 3. Why Do We Need `rw_semaphore`?

Consider a shared variable:

```c
int value = 1;
```

Multiple threads need to read this variable, while another thread occasionally updates it.

### 3.1 Using a Mutex

```c
mutex_lock(&lock);

read_shared_data();

mutex_unlock(&lock);
```

A mutex allows only one thread to hold the lock at a time.

Even if several threads only want to read the data, they must acquire the mutex one at a time.

This can unnecessarily restrict concurrent reads.

### 3.2 Using `rw_semaphore`

```c
down_read(&rwsem);

read_shared_data();

up_read(&rwsem);
```

Multiple threads can hold the read lock simultaneously.

When a thread needs to modify the data, it acquires the write lock:

```c
down_write(&rwsem);

value = 2;

up_write(&rwsem);
```

The writer waits until the existing readers release their locks.

**Important:** The read-write semaphore protects the data only if all relevant accesses follow the same locking rules.

## 4. Linux Kernel APIs

Include the following header:

```c
#include <linux/rwsem.h>
```

### 4.1 `init_rwsem()`

Initializes a read-write semaphore.

```c
struct rw_semaphore rwsem;

init_rwsem(&rwsem);
```

The semaphore must be initialized before it is used.

### 4.2 `down_read()`

Acquires the read lock.

```c
down_read(&rwsem);

/* Read shared data */

up_read(&rwsem);
```

Multiple readers can hold the read lock simultaneously.

If a writer holds the write lock, a reader may have to wait.

### 4.3 `up_read()`

Releases the read lock.

```c
up_read(&rwsem);
```

Call this after finishing the protected read operation.

If other readers still hold the read lock, a waiting writer cannot acquire the lock yet.

### 4.4 `down_write()`

Acquires the write lock.

```c
down_write(&rwsem);

/* Modify shared data */

up_write(&rwsem);
```

The writer requires exclusive access.

It must wait until conflicting existing readers and writers have released the lock.

### 4.5 `up_write()`

Releases the write lock.

```c
up_write(&rwsem);
```

Call this after finishing the protected write operation.

Other waiting readers or writers may then become eligible to acquire the lock.

### 4.6 `downgrade_write()`

Converts a held write lock into a read lock.

```c
down_write(&rwsem);

/* Modify shared data */

downgrade_write(&rwsem);

/* Continue accessing data under the read lock */

up_read(&rwsem);
```

After downgrading, the task no longer has exclusive write access. It holds a read lock instead.

Other readers can acquire read locks, but a writer cannot acquire the write lock while read locks remain held.

**Important:** These blocking locking APIs must not be used in hard IRQ context because they may sleep.

## 5. Experiment

### 5.1 Directory Structure

Create the following directory in the `linux-kernel-locks` repository:

```text
05_rw_semaphore/
├── Makefile
├── rwsem.c
└── README.md
```

### 5.2 Experiment Description

The kernel module will create three kernel threads:

- Reader 1.
- Reader 2.
- Writer 1.

The module will maintain a shared integer variable.

```c
int shared_value = 0;
```

**Reader 1**

1. Acquires the read lock using `down_read()`.
2. Reads and prints `shared_value`.
3. Holds the read lock for a short period.
4. Releases the read lock using `up_read()`.

**Reader 2**

1. Acquires the read lock using `down_read()`.
2. Reads and prints `shared_value`.
3. Holds the read lock for a short period.
4. Releases the read lock using `up_read()`.

**Writer 1**

1. Attempts to acquire the write lock using `down_write()`.
2. Waits if readers hold the lock.
3. Updates `shared_value`.
4. Releases the write lock using `up_write()`.

### 5.3 Expected Execution Flow

```text
Module Initialization
         |
         v
Initialize rw_semaphore
         |
         v
Initialize shared_value = 0
         |
         v
Create Reader 1 and Reader 2
         |
         v
Readers acquire read locks
         |
         v
Both readers read shared_value
         |
         v
Writer requests write lock
         |
         v
Writer waits for existing readers
         |
         v
Readers release read locks
         |
         v
Writer acquires write lock
         |
         v
Writer updates shared_value = 1
         |
         v
Writer releases write lock
         |
         v
Experiment completes
```

The actual execution order depends on scheduling. To demonstrate the waiting behavior reliably, the experiment should coordinate thread startup and use suitable synchronization rather than relying only on arbitrary delays.

## 6. Build and Execute

Run these commands from the `05_rw_semaphore` directory.

### 6.1 Build

```bash
make
```

### 6.2 Load the Module

```bash
sudo insmod rwsem.ko
```

### 6.3 Check Kernel Logs

```bash
sudo dmesg | tail -n 50
```

### 6.4 Remove the Module

```bash
sudo rmmod rwsem
```

### 6.5 Clean Build Files

```bash
make clean
```

## 7. Expected Observation

The following output illustrates the intended behavior. The actual log messages and order depend on the implementation.

```text
Reader 1: Acquired read lock
Reader 1: shared_value = 0

Reader 2: Acquired read lock
Reader 2: shared_value = 0

Writer 1: Requesting write lock
Writer 1: Waiting for readers

Reader 1: Releasing read lock
Reader 2: Releasing read lock

Writer 1: Acquired write lock
Writer 1: Updating shared_value = 1
Writer 1: Releasing write lock
```

### Explanation

1. Reader 1 acquires the read lock.
2. Reader 2 also acquires the read lock.
3. Both readers can access the shared variable simultaneously.
4. The writer requests the write lock.
5. The writer cannot acquire it while the existing readers hold their read locks.
6. Both readers release their locks.
7. The writer acquires the write lock.
8. The writer updates the shared variable.
9. The writer releases the write lock.

**Observation:** Read locks allow concurrent readers, while a write lock provides exclusive access.

The messages stating that the writer is waiting should be implemented carefully: a message printed before `down_write()` can show that the writer is requesting the lock, but it does not by itself prove exactly how long the writer waited.

## 8. Important Concepts

### 8.1 Can Multiple Readers Acquire the Lock Simultaneously?

Yes.

Multiple readers can hold `down_read(&rwsem)` simultaneously, provided no conflicting writer holds the lock.

```text
Reader 1 ---- Read lock ----+
                           |
Reader 2 ---- Read lock ----+---- Shared data
                           |
Reader 3 ---- Read lock ----+
```

### 8.2 Can a Writer Acquire the Lock While Readers Are Active?

No.

A writer must wait until the existing readers release their read locks.

```text
Reader 1: Holds read lock
Reader 2: Holds read lock
Writer 1: Requests write lock

Writer 1 must wait.
```

### 8.3 Can a Reader Acquire the Lock While a Writer Holds It?

No.

While a writer holds the write lock, readers cannot acquire the read lock.

They must wait until the writer releases the lock.

### 8.4 What Happens When One Reader Releases Its Lock?

Suppose two readers hold read locks and a writer is waiting.

```text
Initial state:

Reader 1: Holds read lock
Reader 2: Holds read lock
Writer 1: Waiting

Reader 1 calls up_read()

New state:

Reader 1: Read lock released
Reader 2: Still holds read lock
Writer 1: Still cannot acquire the write lock
```

The writer must wait until Reader 2 also releases its read lock.

### 8.5 What Problem Does `rw_semaphore` Prevent?

Consider this example:

```text
Shared variable = 1

Reader 1                 Writer 1
   |                        |
   | Reads value = 1        |
   |                        | Updates value = 2
   |                        |
   | Continues using        |
   | old value = 1          |
```

Without proper synchronization, the reader and writer can access the data concurrently. Depending on the operations, the reader might observe inconsistent data.

With `rw_semaphore`, a writer cannot modify the protected data while a reader holds the read lock.

However, a reader holding a read lock does not automatically receive an incorrect value. The lock prevents conflicting writes during the protected read operation, provided every relevant access follows the same locking rules.

### 8.6 What Happens If a Writer Is Waiting?

A writer requesting the write lock may have to wait for active readers to release their locks.

Do not assume that a waiting writer will always acquire the lock immediately when the last current reader exits. Exact scheduling and fairness behavior depend on the kernel implementation.

### 8.7 Can `rw_semaphore` Be Used in Hard IRQ Context?

No. The blocking acquisition APIs, including `down_read()` and `down_write()`, may sleep.

Hard IRQ handlers cannot sleep.

For shared data accessed from hard IRQ context, consider an appropriate spinlock and interrupt-safe locking strategy instead.

### 8.8 Can `rw_semaphore` Protect Data If Some Threads Do Not Use It?

No.

If one thread modifies the shared variable without taking the appropriate lock, the read-write semaphore cannot protect that access.

All relevant accesses must follow the same synchronization rules.

## 9. `rw_semaphore` vs Mutex

| Feature | `rw_semaphore` | Mutex |
|---|---|---|
| Concurrent readers | Yes | No |
| Concurrent writers | No | No |
| Reader and writer simultaneously | No | No |
| Exclusive write access | Yes | Yes |
| Can the waiting task sleep? | Yes | Yes |
| Suitable for hard IRQ blocking acquisition | No | No |
| Typical use | Shared data with multiple readers and fewer writers | General mutual exclusion |

### Rule of Thumb

Use an `rw_semaphore` when multiple tasks frequently read shared data and writes require exclusive access.

Use a mutex when only one task should access the protected region at a time, or when the added complexity of read-write locking is unnecessary.

An `rw_semaphore` is not automatically faster than a mutex. Performance depends on workload, contention, critical-section duration, and reader-to-writer ratio.

## 10. Interview Questions

### Q1. What is an `rw_semaphore`?

An `rw_semaphore` is a Linux kernel synchronization primitive that permits multiple readers or one exclusive writer.

### Q2. Can multiple threads hold a read lock simultaneously?

Yes. Multiple readers can hold the read lock simultaneously when no conflicting writer holds the lock.

### Q3. Can two writers hold the write lock simultaneously?

No. The write lock provides exclusive access.

### Q4. Can a writer acquire the write lock while readers are holding read locks?

No. The writer must wait until the existing readers release their locks.

### Q5. What happens when one reader releases its lock while another reader still holds it?

The writer cannot acquire the write lock yet because another reader still holds a read lock.

### Q6. What is the difference between `down_read()` and `down_write()`?

`down_read()` acquires a shared read lock. `down_write()` requests exclusive write access.

### Q7. What is the difference between `up_read()` and `up_write()`?

`up_read()` releases a read lock. `up_write()` releases a write lock.

### Q8. What is the purpose of `downgrade_write()`?

It converts a held write lock into a read lock, allowing the task to continue reading while no longer holding exclusive write access.

### Q9. Why use an `rw_semaphore` instead of a mutex?

An `rw_semaphore` allows multiple readers to access shared data simultaneously. A mutex allows only one holder at a time.

### Q10. Can `rw_semaphore` be used in a hard IRQ handler?

Its blocking acquisition APIs cannot be used in hard IRQ context because they may sleep.

### Q11. Does an `rw_semaphore` prevent all race conditions automatically?

No. All relevant accesses to the protected data must follow the same locking rules.

### Q12. Is an `rw_semaphore` always faster than a mutex?

No. Its performance depends on the workload, contention, critical-section duration, and ratio of readers to writers.

### Q13. What is the difference between a semaphore and an `rw_semaphore`?

A regular semaphore uses a counter to control access to resources. An `rw_semaphore` specifically supports shared read access and exclusive write access.

## 11. Conclusion

In this experiment, two reader threads and one writer thread access a shared integer protected by an `rw_semaphore`.

The experiment demonstrates:

- Initialization using `init_rwsem()`.
- Read locking using `down_read()`.
- Read unlocking using `up_read()`.
- Write locking using `down_write()`.
- Write unlocking using `up_write()`.
- Conversion from write locking to read locking using `downgrade_write()`.
- Concurrent readers.
- Exclusive writers.
- Waiting when readers hold the lock.
- The difference between `rw_semaphore` and a mutex.
- Why blocking synchronization APIs cannot be used in hard IRQ context.

**Key takeaway:** An `rw_semaphore` allows multiple readers to access protected data concurrently, but a writer requires exclusive access.