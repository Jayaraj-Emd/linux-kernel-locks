#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/completion.h>
#include <linux/mutex.h>
#include <linux/signal.h>

MODULE_AUTHOR("Jayaraj");
MODULE_DESCRIPTION("mutex_lock_interruptible demonstration");
MODULE_LICENSE("GPL");

static struct task_struct *thread1;
static struct task_struct *thread2;
static struct task_struct *thread3;

static struct completion done1;
static struct completion done2;
static struct completion done3;

static struct mutex lock;



static int thread_one_function(void *data)
{
    struct completion *done = data;

    mutex_lock(&lock);

    pr_info("worker1: Acquired lock...\n");

    msleep(10000);

    mutex_unlock(&lock);

    pr_info("worker1: Released lock...\n");

    complete(done);

    return 0;
}

static int thread_two_function(void *data)
{
    struct completion *done = data;
    int ret;

    pr_info("worker2: PID = %d\n", current->pid);

    pr_info("worker2: Waiting for lock...\n");

    ret = mutex_lock_killable(&lock);

    if (ret) {
        pr_info("worker2: Interrupted while waiting!\n");
        pr_info("worker2: return value = %d\n", ret);

        complete(done);

        return 0;
    }

    pr_info("worker2: Acquired lock...\n");

    mutex_unlock(&lock);

    pr_info("worker2: Released lock...\n");

    complete(done);

    return 0;
}


static int thread_three_function(void *data)
{
    struct completion *done = data;
    msleep(1000);

    pr_info("worker3: Sending SIGKILL to worker2...\n");

    send_sig(SIGKILL, thread2, 1);

    pr_info("worker3: Signal sent\n");

    complete(done);

    return 0;
}


static int __init mutex_init_module(void)
{
    pr_info("mutex module loaded...\n");

    init_completion(&done1);
    init_completion(&done2);
    init_completion(&done3);

    mutex_init(&lock);

    thread1 = kthread_run(thread_one_function,
                          &done1,
                          "worker1");

    if (IS_ERR(thread1)) {
        pr_err("Failed to create worker1\n");
        return PTR_ERR(thread1);
    }


    thread2 = kthread_run(thread_two_function,
                          &done2,
                          "worker2");

    if (IS_ERR(thread2)) {
        pr_err("Failed to create worker2\n");

        wait_for_completion(&done1);

        return PTR_ERR(thread2);
    }

    thread3 = kthread_run(thread_three_function,
                          &done3,
                          "worker3");

    if (IS_ERR(thread3)) {
        pr_err("Failed to create worker3\n");

        wait_for_completion(&done1);
        wait_for_completion(&done2);

        return PTR_ERR(thread3);
    }

    return 0;
}


static void __exit mutex_exit_module(void)
{
    wait_for_completion(&done1);
    wait_for_completion(&done2);
    wait_for_completion(&done3);
    pr_info("mutex module unloaded...\n");
}


module_init(mutex_init_module);
module_exit(mutex_exit_module);