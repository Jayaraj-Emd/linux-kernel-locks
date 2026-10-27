#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/atomic.h>
#include <linux/completion.h>

#define ITERATIONS 1000000

static atomic_t counter = ATOMIC_INIT(0);

static struct task_struct *thread1;
static struct task_struct *thread2;

static struct completion thread1_done;
static struct completion thread2_done;

static int worker(void *data)
{
    int i;
    int id = *(int *)data;

    for (i = 0; i < ITERATIONS; i++) {
        atomic_inc(&counter);
    }

    pr_info("Thread %d completed\n", id);

    if (id == 1)
        complete(&thread1_done);
    else
        complete(&thread2_done);

    return 0;
}

static int id1 = 1;
static int id2 = 2;

static int __init atomic_demo_init(void)
{
    pr_info("Atomic demo: module loaded\n");

    atomic_set(&counter, 0);

    init_completion(&thread1_done);
    init_completion(&thread2_done);

    thread1 = kthread_run(worker, &id1, "atomic_thread1");

    if (IS_ERR(thread1)) {
        pr_err("Failed to create thread 1\n");
        return PTR_ERR(thread1);
    }

    thread2 = kthread_run(worker, &id2, "atomic_thread2");

    if (IS_ERR(thread2)) {
        pr_err("Failed to create thread 2\n");

        kthread_stop(thread1);
        wait_for_completion(&thread1_done);

        return PTR_ERR(thread2);
    }

    pr_info("Expected counter: %d\n", 2 * ITERATIONS);

    return 0;
}

static void __exit atomic_demo_exit(void)
{
    wait_for_completion(&thread1_done);
    wait_for_completion(&thread2_done);

    pr_info("Final counter: %d\n", atomic_read(&counter));
    pr_info("Expected counter: %d\n", 2 * ITERATIONS);

    pr_info("Atomic demo: module unloaded\n");
}

module_init(atomic_demo_init);
module_exit(atomic_demo_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Linux kernel atomic variable demonstration");
MODULE_AUTHOR("Linux Kernel Learning");