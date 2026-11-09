#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

int my_function(void);
int my_function(void)
{
    pr_info("Module A: my_function called\n");

    return 100;
}

EXPORT_SYMBOL(my_function);

static int __init module_a_init(void)
{
    pr_info("Module A loaded\n");

    return 0;
}

static void __exit module_a_exit(void)
{
    pr_info("Module A unloaded\n");
}

module_init(module_a_init);
module_exit(module_a_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module A exports a function");
MODULE_AUTHOR("Jayaraj");
