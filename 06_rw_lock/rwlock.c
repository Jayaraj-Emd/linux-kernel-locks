#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/completion.h>

static rwlock_t data_lock;
static int shared_value;

static struct task_struct *reader1_task;
static struct task_struct *reader2_task;
static struct task_struct *writer_task;

static DECLARE_COMPLETION(reader1_done);
static DECLARE_COMPLETION(reader2_done);
static DECLARE_COMPLETION(writer_done);

/* Reader thread 1 */
static int reader1_fn(void *data)
{
    int value;

    pr_info("Reader 1: Started\n");

    read_lock(&data_lock);

    value = shared_value;

    pr_info("Reader 1: Read shared_value = %d\n", value);

    read_unlock(&data_lock);

    pr_info("Reader 1: Released read lock\n");

    complete(&reader1_done);

    return 0;
}

/* Reader thread 2 */
static int reader2_fn(void *data)
{
    int value;

    pr_info("Reader 2: Started\n");

    read_lock(&data_lock);

    value = shared_value;

    pr_info("Reader 2: Read shared_value = %d\n", value);

    read_unlock(&data_lock);

    pr_info("Reader 2: Released read lock\n");

    complete(&reader2_done);

    return 0;
}

/* Writer thread */
static int writer_fn(void *data)
{
    pr_info("Writer: Started\n");
    pr_info("Writer: Requesting write lock\n");

    write_lock(&data_lock);

    pr_info("Writer: Acquired write lock\n");

    shared_value++;

    pr_info("Writer: Updated shared_value = %d\n",
            shared_value);

    write_unlock(&data_lock);

    pr_info("Writer: Released write lock\n");

    complete(&writer_done);

    return 0;
}

static int __init rwlock_init_module(void)
{
    rwlock_init(&data_lock);
    shared_value = 0;

    pr_info("rwlock example: Module loaded\n");

    reader1_task = kthread_run(reader1_fn, NULL, "reader1");
    if (IS_ERR(reader1_task))
        return PTR_ERR(reader1_task);

    reader2_task = kthread_run(reader2_fn, NULL, "reader2");
    if (IS_ERR(reader2_task)) {
        int ret = PTR_ERR(reader2_task);

        wait_for_completion(&reader1_done);
        return ret;
    }

    writer_task = kthread_run(writer_fn, NULL, "writer");
    if (IS_ERR(writer_task)) {
        int ret = PTR_ERR(writer_task);

        wait_for_completion(&reader1_done);
        wait_for_completion(&reader2_done);
        return ret;
    }

    return 0;
}

static void __exit rwlock_exit_module(void)
{
    wait_for_completion(&reader1_done);
    wait_for_completion(&reader2_done);
    wait_for_completion(&writer_done);

    pr_info("rwlock example: Final shared_value = %d\n",
            shared_value);

    pr_info("rwlock example: Module unloaded\n");
}

module_init(rwlock_init_module);
module_exit(rwlock_exit_module);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Linux kernel rwlock example");
MODULE_AUTHOR("Kernel Learning");
