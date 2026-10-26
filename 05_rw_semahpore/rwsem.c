#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/rwsem.h>
#include <linux/delay.h>
#include <linux/completion.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kernel Learner");
MODULE_DESCRIPTION("Linux Kernel Read-Write Semaphore Experiment");

static struct rw_semaphore rwsem;

static struct task_struct *reader1_task;
static struct task_struct *reader2_task;
static struct task_struct *writer_task;

static DECLARE_COMPLETION(reader1_done);
static DECLARE_COMPLETION(reader2_done);
static DECLARE_COMPLETION(writer_done);

static int shared_data = 1;

static int reader1_function(void *data)
{
    pr_info("Reader 1: Trying to acquire read lock\n");

    down_read(&rwsem);

    pr_info("Reader 1: Acquired read lock\n");
    pr_info("Reader 1: Reading shared_data = %d\n", shared_data);

    msleep(3000);

    pr_info("Reader 1: Finished reading\n");

    up_read(&rwsem);

    pr_info("Reader 1: Released read lock\n");

    complete(&reader1_done);

    return 0;
}

static int reader2_function(void *data)
{
    pr_info("Reader 2: Trying to acquire read lock\n");

    down_read(&rwsem);

    pr_info("Reader 2: Acquired read lock\n");
    pr_info("Reader 2: Reading shared_data = %d\n", shared_data);

    msleep(3000);

    pr_info("Reader 2: Finished reading\n");

    up_read(&rwsem);

    pr_info("Reader 2: Released read lock\n");

    complete(&reader2_done);

    return 0;
}

static int writer_function(void *data)
{
    pr_info("Writer: Trying to acquire write lock\n");

    down_write(&rwsem);

    pr_info("Writer: Acquired write lock\n");

    pr_info("Writer: Old shared_data = %d\n", shared_data);

    shared_data = 2;

    pr_info("Writer: Updated shared_data = %d\n", shared_data);

    msleep(1000);

    up_write(&rwsem);

    pr_info("Writer: Released write lock\n");

    complete(&writer_done);

    return 0;
}

static int __init rwsem_init_module(void)
{
    pr_info("RWSEM: Initializing module\n");

    init_rwsem(&rwsem);
    reader1_task = kthread_run(reader1_function, NULL, "reader1");

    if (IS_ERR(reader1_task)) {
        pr_err("RWSEM: Failed to create Reader 1\n");
        return PTR_ERR(reader1_task);
    }

    msleep(100);

    reader2_task = kthread_run(reader2_function, NULL, "reader2");

    if (IS_ERR(reader2_task)) {
        pr_err("RWSEM: Failed to create Reader 2\n");

        wait_for_completion(&reader1_done);

        return PTR_ERR(reader2_task);
    }

    msleep(100);

    writer_task = kthread_run(writer_function, NULL, "writer");

    if (IS_ERR(writer_task)) {
        pr_err("RWSEM: Failed to create Writer\n");

        wait_for_completion(&reader1_done);
        wait_for_completion(&reader2_done);

        return PTR_ERR(writer_task);
    }

    pr_info("RWSEM: All threads created successfully\n");

    return 0;
}

static void __exit rwsem_exit_module(void)
{
    pr_info("RWSEM: Waiting for threads to finish\n");

    wait_for_completion(&reader1_done);
    wait_for_completion(&reader2_done);
    wait_for_completion(&writer_done);

    pr_info("RWSEM: Final shared_data = %d\n", shared_data);
    pr_info("RWSEM: Module exiting\n");
}

module_init(rwsem_init_module);
module_exit(rwsem_exit_module);