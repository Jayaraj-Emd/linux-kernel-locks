#include<linux/kernel.h>
#include<linux/bug.h>
#include<linux/module.h>
#include<linux/init.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jayaraj");

static void checkvalue(int value);

static void checkvalue(int value){
	WARN_ON_ONCE(value < 0);
}
static int __init warninit(void){
	pr_info("warn_on: module loaded....\n");
	int val = -10;
	pr_info("warn_on_once : value = %d\n",val);

	for(int i = 0 ; i < 100; i++){
		checkvalue(val);
		pr_info("warn_on_once: interation %d\n",i);
	}
	
	return 0;

}

static void __exit warnExit(void){
	pr_info("warn_on_once: module exited...\n");
}


module_init(warninit);
module_exit(warnExit);