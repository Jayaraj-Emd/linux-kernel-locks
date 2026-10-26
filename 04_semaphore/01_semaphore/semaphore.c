#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/completion.h>
#include <linux/delay.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jayaraj");
MODULE_DESCRIPTION("Linux Kernel Counting Semaphore Experiment");

#define SEMAPHORE_COUNT 1

static struct semaphore sem;

static struct task_struct *thread1;
static struct task_struct *thread2;

static struct completion done1;
static struct completion done2;

static int thread1_function(void *data)
{
    pr_info("Thread 1: Waiting to acquire semaphore\n");

    down(&sem);

    pr_info("Thread 1: Acquired semaphore\n");

    msleep(3000);

    pr_info("Thread 1: Releasing semaphore\n");

    up(&sem);

    complete(&done1);

    return 0;
}

static int thread2_function(void *data)
{
    pr_info("Thread 2: Waiting to acquire semaphore\n");

    down(&sem);

    pr_info("Thread 2: Acquired semaphore\n");

    msleep(1000);

    pr_info("Thread 2: Releasing semaphore\n");

    up(&sem);

    complete(&done2);

    return 0;
}

static int __init semaphore_init_module(void)
{
    pr_info("Semaphore module initialized\n");

    sema_init(&sem, SEMAPHORE_COUNT);

    init_completion(&done1);
    init_completion(&done2);

    thread1 = kthread_run(thread1_function, NULL, "sem_thread1");

    if (IS_ERR(thread1)) {
        pr_err("Failed to create Thread 1\n");
        return PTR_ERR(thread1);
    }

    thread2 = kthread_run(thread2_function, NULL, "sem_thread2");

    if (IS_ERR(thread2)) {
        int ret = PTR_ERR(thread2);

        wait_for_completion(&done1);

        pr_err("Failed to create Thread 2\n");
        return ret;
    }

    pr_info("Both kernel threads started\n");

    return 0;
}

static void __exit semaphore_exit_module(void)
{
    wait_for_completion(&done1);
    wait_for_completion(&done2);

    pr_info("Semaphore module exited\n");
}

module_init(semaphore_init_module);
module_exit(semaphore_exit_module);