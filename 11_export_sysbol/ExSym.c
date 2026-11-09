#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

extern int my_function(void);

static int __init module_b_init(void)
{
    int value;

    value = my_function();

    pr_info("Module B: Value = %d\n", value);

    return 0;
}

static void __exit module_b_exit(void)
{
    pr_info("Module B unloaded\n");
}

module_init(module_b_init);
module_exit(module_b_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module B uses an exported symbol");
MODULE_AUTHOR("Jayaraj");
