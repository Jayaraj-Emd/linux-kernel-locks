#include<linux/kernel.h>
#include<linux/bug.h>
#include<linux/module.h>
#include<linux/init.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jayaraj");

static int __init warninit(void){
	pr_info("warn_on: module loaded....\n");
	int val = -10;
	pr_info("warn_on : value = %d\n",val);

	WARN_ON(val < 0);
	pr_err("warn_on: Execution continued after WARN_ON..\n");
	
	return 0;

}

static void __exit warnExit(void){
	pr_info("warn_on: module exited...\n");
}


module_init(warninit);
module_exit(warnExit);