#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/rcupdate.h>
#include <linux/kthread.h>
#include <linux/delay.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Kernel Learning");
MODULE_DESCRIPTION("Simple RCU reader-writer demonstration");

struct config {
    int version;
    int speed;
};

/* Shared pointer protected by RCU */
static struct config __rcu *current_config;

static struct task_struct *reader_task;

/* -------------------------------------------------- */
/* Reader thread                                      */
/* -------------------------------------------------- */

static int reader_thread(void *data)
{
    int version;
    int speed;
    int last_version = -1;

    pr_info("RCU: Reader thread started\n");

    while (!kthread_should_stop()) {
        struct config *cfg;

        /*
         * RCU read-side critical section starts.
         */
        rcu_read_lock();

        cfg = rcu_dereference(current_config);

        if (cfg) {
            version = cfg->version;
            speed = cfg->speed;
        } else {
            version = -1;
            speed = -1;
        }

        /*
         * RCU read-side critical section ends.
         */
        rcu_read_unlock();

        /* Log only when the configuration changes. */
        if (version != last_version) {
            pr_info("RCU Reader: version=%d, speed=%d\n",
                    version, speed);

            last_version = version;
        }

        /* Sleeping is allowed here, outside the RCU section. */
        msleep(100);
    }

    pr_info("RCU: Reader thread stopping\n");

    return 0;
}

/* -------------------------------------------------- */
/* Writer: replace the shared configuration           */
/* -------------------------------------------------- */

static int update_config(int version, int speed)
{
    struct config *new_config;
    struct config *old_config;

    new_config = kmalloc(sizeof(*new_config), GFP_KERNEL);
    if (!new_config)
        return -ENOMEM;

    new_config->version = version;
    new_config->speed = speed;

    /*
     * Only the module-init thread updates this pointer.
     * Therefore, there is only one writer.
     */
    old_config = rcu_dereference_protected(current_config, 1);

    /* Publish the new configuration. */
    rcu_assign_pointer(current_config, new_config);

    pr_info("RCU Writer: published version=%d, speed=%d\n",
            version, speed);

    /*
     * Wait for readers that could still be using old_config.
     */
    synchronize_rcu();

    pr_info("RCU Writer: grace period completed\n");

    /* Old configuration is now safe to free. */
    kfree(old_config);

    pr_info("RCU Writer: old configuration freed\n");

    return 0;
}

/* -------------------------------------------------- */
/* Module initialization                              */
/* -------------------------------------------------- */

static int __init rcu_demo_init(void)
{
    struct config *initial_config;
    int ret;

    pr_info("RCU demo: module loading\n");

    initial_config = kmalloc(sizeof(*initial_config), GFP_KERNEL);
    if (!initial_config)
        return -ENOMEM;

    initial_config->version = 1;
    initial_config->speed = 100;

    RCU_INIT_POINTER(current_config, initial_config);

    reader_task = kthread_run(reader_thread, NULL, "rcu_reader");
    if (IS_ERR(reader_task)) {
        ret = PTR_ERR(reader_task);
        reader_task = NULL;

        RCU_INIT_POINTER(current_config, NULL);
        kfree(initial_config);

        pr_err("RCU: failed to create reader thread\n");
        return ret;
    }

    /* Give the reader time to observe the initial configuration. */
    msleep(300);

    /* Replace version 1 with version 2. */
    ret = update_config(2, 200);
    if (ret) {
        kthread_stop(reader_task);
        reader_task = NULL;

        RCU_INIT_POINTER(current_config, NULL);
        synchronize_rcu();
        kfree(initial_config);

        return ret;
    }

    pr_info("RCU demo: module loaded successfully\n");

    return 0;
}

/* -------------------------------------------------- */
/* Module cleanup                                     */
/* -------------------------------------------------- */

static void __exit rcu_demo_exit(void)
{
    struct config *old_config;

    pr_info("RCU demo: module unloading\n");

    /* Stop the reader before removing the shared pointer. */
    if (reader_task) {
        kthread_stop(reader_task);
        reader_task = NULL;
    }

    old_config = rcu_dereference_protected(current_config, 1);

    RCU_INIT_POINTER(current_config, NULL);

    /* Ensure no RCU reader can still access old_config. */
    synchronize_rcu();

    kfree(old_config);

    pr_info("RCU demo: module unloaded\n");
}

module_init(rcu_demo_init);
module_exit(rcu_demo_exit);