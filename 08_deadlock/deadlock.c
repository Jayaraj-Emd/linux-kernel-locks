#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/completion.h>
#include <linux/delay.h>

static DEFINE_MUTEX(lock_A);
static DEFINE_MUTEX(lock_B);

static struct task_struct *thread1;
static struct task_struct *thread2;

static DECLARE_COMPLETION(thread1_locked_A);
static DECLARE_COMPLETION(thread2_locked_B);

static int thread1_function(void *data)
{
    pr_info("Thread 1: trying to acquire lock_A\n");

    mutex_lock(&lock_A);

    pr_info("Thread 1: acquired lock_A\n");

    complete(&thread1_locked_A);

    wait_for_completion(&thread2_locked_B);

    pr_info("Thread 1: trying to acquire lock_B\n");

    mutex_lock(&lock_B);

    pr_info("Thread 1: acquired lock_B\n");

    mutex_unlock(&lock_B);
    mutex_unlock(&lock_A);

    return 0;
}

static int thread2_function(void *data)
{
    pr_info("Thread 2: trying to acquire lock_B\n");

    mutex_lock(&lock_B);

    pr_info("Thread 2: acquired lock_B\n");

    complete(&thread2_locked_B);

    
    wait_for_completion(&thread1_locked_A);

    pr_info("Thread 2: trying to acquire lock_A\n");

    mutex_lock(&lock_A);

    pr_info("Thread 2: acquired lock_A\n");

    mutex_unlock(&lock_A);
    mutex_unlock(&lock_B);

    return 0;
}

static int __init deadlock_init(void)
{
    pr_info("Deadlock demo: loading module\n");

    thread1 = kthread_run(thread1_function, NULL, "deadlock_thread1");

    if (IS_ERR(thread1))
        return PTR_ERR(thread1);

    thread2 = kthread_run(thread2_function, NULL, "deadlock_thread2");

    if (IS_ERR(thread2)) {
        pr_err("Failed to create Thread 2\n");
        return PTR_ERR(thread2);
    }

    return 0;
}

static void __exit deadlock_exit(void)
{
    pr_info("Deadlock demo: module exit called\n");
}

module_init(deadlock_init);
module_exit(deadlock_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Linux kernel mutex deadlock demonstration");
MODULE_AUTHOR("Linux Kernel Learning");