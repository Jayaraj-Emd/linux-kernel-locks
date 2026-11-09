#include<linux/init.h>
#include<linux/kernel.h>
#include<linux/notifier.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SRCU notifier Chain");
MODULE_AUTHOR("Jayaraj");



struct srcu_notifier_head chain;

int CallBackFuntionA(struct notifier_block *nb,unsigned long action,void *data);
int CallBackFuntionB(struct notifier_block *nb,unsigned long action,void *data);


int CallBackFuntionA(struct notifier_block *nb,unsigned long action,void *data){
	if(action != 1)
		return NOTIFY_DONE;
	
	pr_info("CallBackFuntionA: Event = %lu,data = %d\n",action,*(int *)data);
	return NOTIFY_OK;
}

int CallBackFuntionB(struct notifier_block *nb,unsigned long action,void *data){
	
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

static int __init SRCUInit(void){
	int data1 = 100;
	int data2 = 200;
	srcu_init_notifier_head(&chain);
	pr_info("SRCU_Notifier; Initializing and registering notifier...\n");
	int ret = 0;
	ret = srcu_notifier_chain_register(&chain,&nb_a);
    if(ret){
    	pr_err("SRCU_Notifier; notifer call registeration failed for callback function A...\n");
    	return ret;
    }
    pr_info("SRCU_Notifier; notifer call registeration successfull for callback function A...\n");
    ret = srcu_notifier_chain_register(&chain,&nb_b);
    if(ret){
    	 srcu_notifier_chain_unregister(&chain,&nb_a);
    	 srcu_cleanup_notifier_head(&chain);
    	pr_err("SRCU_Notifier;notifer call registeration failed for callback function B...\n");
    	return ret;
    }
    pr_info("SRCU_Notifier; notifer call registeration successfull for callback function B...\n");

    srcu_notifier_call_chain(&chain,1,&data1);
    srcu_notifier_call_chain(&chain,2,&data2);
    return 0;

}
static void __exit SRCUExit(void){
	srcu_notifier_chain_unregister(&chain,&nb_b);
   srcu_notifier_chain_unregister(&chain,&nb_a);
   srcu_cleanup_notifier_head(&chain);
   pr_info("SRCU_Notifier; Unregistering notifier Done...\n");   
}

module_init(SRCUInit);
module_exit(SRCUExit);
