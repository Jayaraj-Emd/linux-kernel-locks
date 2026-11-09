#include<linux/init.h>
#include<linux/kernel.h>
#include<linux/notifier.h>
#include <linux/module.h>
#include <linux/mutex.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Blocking Notifier Chain");
MODULE_AUTHOR("Jayaraj");

static DEFINE_MUTEX(lock);
static BLOCKING_NOTIFIER_HEAD(atomic);

int CallBackFuntionA(struct notifier_block *nb,unsigned long action,void *data);
int CallBackFuntionB(struct notifier_block *nb,unsigned long action,void *data);

int share_data = 1;
int CallBackFuntionA(struct notifier_block *nb,unsigned long action,void *data){
	if(action != 1)
		return NOTIFY_DONE;
	
	mutex_lock(&lock);
	share_data++;
	mutex_unlock(&lock);
	pr_info("CallBackFuntionA: Event = %lu,data = %d\n",action,*(int *)data);
	return NOTIFY_OK;
}

int CallBackFuntionB(struct notifier_block *nb,unsigned long action,void *data){
	
	if(action != 2)
		return NOTIFY_DONE;

	mutex_lock(&lock);
	share_data++;
	mutex_unlock(&lock);
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
	pr_info("blocking_Notifier; Initializing and registering notifier...\n");
	int ret = 0;
	ret = blocking_notifier_chain_register(&atomic,&nb_a);
    if(ret){
    	pr_err("blocking_Notifier: notifer call registeration failed for callback function A...\n");
    	return ret;
    }
    pr_info("blocking_Notifier: notifer call registeration successfull for callback function A...\n");
    ret = blocking_notifier_chain_register(&atomic,&nb_b);
    if(ret){
    	 blocking_notifier_chain_unregister(&atomic,&nb_a);
    	pr_err("blocking_Notifier: notifer call registeration failed for callback function B...\n");
    	return ret;
    }
    pr_info("blocking_Notifier: notifer call registeration successfull for callback function B...\n");

    blocking_notifier_call_chain(&atomic,1,&data1);
    blocking_notifier_call_chain(&atomic,2,&data2);
    return 0;

}
static void __exit atomicExit(void){
	blocking_notifier_chain_unregister(&atomic,&nb_b);
   blocking_notifier_chain_unregister(&atomic,&nb_a);
   pr_info("blocking_Notifier:Unregistering notifier Done...\n");
   pr_info("shared data = %d\n",share_data);

   
}

module_init(atomicInit);
module_exit(atomicExit);
