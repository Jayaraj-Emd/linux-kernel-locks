#include<linux/init.h>
#include<linux/kernel.h>
#include<linux/notifier.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Atomic notifer chanin");
MODULE_AUTHOR("Jayaraj");

static ATOMIC_NOTIFIER_HEAD(atomic);

static int CallBackFuntionA(struct notifier_block *nb,unsigned long action,void *data);
static int CallBackFuntionB(struct notifier_block *nb,unsigned long action,void *data);

static int CallBackFuntionA(struct notifier_block *nb,unsigned long action,void *data){
	if(action != 1)
		return NOTIFY_DONE;
	pr_info("CallBackFuntionA: Event = %lu,data = %d\n",action,*(int *)data);
	return NOTIFY_OK;
}

static int CallBackFuntionB(struct notifier_block *nb,unsigned long action,void *data){
	
	if(action != 2)
		return NOTIFY_DONE;
	pr_info("CallBackFuntionB: Event = %lu,data = %d\n",action,*(int *)data);
	return NOTIFY_OK;
}

static struct notifier_block nb_a = {
	.notifier_call = CallBackFuntionA,
	.priority = 10,
};

static struct notifier_block nb_b = {
	.notifier_call = CallBackFuntionB,
	.priority = 50,
};

static int __init atomicInit(void){
	int data1 = 100;
	int data2 = 200;
	pr_info("Atomic_Notifier; Initializing and registering notifier...\n");
	int ret = 0;
	ret = atomic_notifier_chain_register(&atomic,&nb_a);
    if(ret){
    	pr_err("Atomic_Notifier: notifer call registeration failed for callback function A...\n");
    	return ret;
    }
    pr_info("Atomic_Notifier: notifer call registeration successfull for callback function A...\n");
    ret = atomic_notifier_chain_register(&atomic,&nb_b);
    if(ret){
    	pr_err("Atomic_Notifier: notifer call registeration failed for callback function B...\n");
    	return ret;
    }
    pr_info("Atomic_Notifier: notifer call registeration successfull for callback function B...\n");

    atomic_notifier_call_chain(&atomic,1,&data1);
    atomic_notifier_call_chain(&atomic,2,&data2);
    return 0;

}
static void __exit atomicExit(void){
   atomic_notifier_chain_unregister(&atomic,&nb_a);
   atomic_notifier_chain_unregister(&atomic,&nb_b);
}

module_init(atomicInit);
module_exit(atomicExit);
