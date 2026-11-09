#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/notifier.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Raw Notifier Chain Experiment");
MODULE_AUTHOR("Jayaraj");

static RAW_NOTIFIER_HEAD(my_chain);

/* Callback A handles event 1 */
static int CallBackFuntionA(struct notifier_block *nb,
                            unsigned long action,
                            void *data)
{
    int value;

    if (action != 1)
        return NOTIFY_DONE;

    value = *(int *)data;

    pr_info("Callback A: Event = %lu, Data = %d\n",
            action, value);

    return NOTIFY_OK;
}

/* Callback B handles event 2 */
static int CallBackFuntionB(struct notifier_block *nb,
                            unsigned long action,
                            void *data)
{
    int value;

    if (action != 2)
        return NOTIFY_DONE;

    value = *(int *)data;

    pr_info("Callback B: Event = %lu, Data = %d\n",
            action, value);

    return NOTIFY_OK;
}

/* Notifier blocks */
static struct notifier_block nb_a = {
    .notifier_call = CallBackFuntionA,
    .priority = 10,
};

static struct notifier_block nb_b = {
    .notifier_call = CallBackFuntionB,
    .priority = 50,
};

/* Module initialization */
static int __init rawNotifierInit(void)
{
    int ret;
    int data1 = 100;
    int data2 = 200;

    pr_info("Raw Notifier: Module Init\n");

    ret = raw_notifier_chain_register(&my_chain, &nb_a);
    if (ret) {
        pr_err("Failed to register Callback A\n");
        return ret;
    }

    ret = raw_notifier_chain_register(&my_chain, &nb_b);
    if (ret) {
        pr_err("Failed to register Callback B\n");

        raw_notifier_chain_unregister(&my_chain, &nb_a);

        return ret;
    }

    pr_info("Raw Notifier: Trigger Event 1\n");

    raw_notifier_call_chain(&my_chain, 1, &data1);

    pr_info("Raw Notifier: Trigger Event 2\n");

    raw_notifier_call_chain(&my_chain, 2, &data2);

    return 0;
}

/* Module cleanup */
static void __exit rawNotifierExit(void)
{
    raw_notifier_chain_unregister(&my_chain, &nb_b);
    raw_notifier_chain_unregister(&my_chain, &nb_a);

    pr_info("Raw Notifier: Callbacks Unregistered\n");
    pr_info("Raw Notifier: Module Exit\n");
}

module_init(rawNotifierInit);
module_exit(rawNotifierExit);