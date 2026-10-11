#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/spinlock.h>
#include<linux/init.h>
#include<linux/completion.h>
#include<linux/kthread.h>

#define ITERATION 100

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("spinlock demonstraction");
MODULE_AUTHOR("Jayaraj");

static u64 count = 0;

struct task_struct *thread1;
struct task_struct *thread2;
static struct completion done1;
static struct completion done2;
static spinlock_t lock;
static int thread_fun(void *data){
    struct completion *done = (struct completion *)data;
	for(u64 i = 0; i < ITERATION; i++){
       spin_lock(&lock);
       count++;
       spin_unlock(&lock);
	}
	complete(done);
	return 0;
}
static int __init spinlockInit(void){
	init_completion(&done1);
	init_completion(&done2);
    spin_lock_init(&lock);

    thread1 = kthread_run(thread_fun,&done1,"worker1");
    if(IS_ERR(thread1)){
        pr_err("Failed to create thread, err code = %d\n", (int)PTR_ERR(thread1));
        return PTR_ERR(thread1);
    }

    thread2 = kthread_run(thread_fun,&done2,"worker2");
    if(IS_ERR(thread2)){
    	wait_for_completion(&done1);
        pr_err("Failed to created thread , err code = %d\n",(int)PTR_ERR(thread2));
        return PTR_ERR(thread2);
    } 

	pr_info("Spin lock initlized successfully...\n");
	return 0;
}
static void __exit spinlockExit(void){
	wait_for_completion(&done1);
	wait_for_completion(&done2);
	pr_info("Expected value = %d\n",2 * ITERATION);
	pr_info("Actual value = %llu\n",(unsigned long long)count);
	pr_info("Spin lock module exited successfully...\n");
}
module_init(spinlockInit);
module_exit(spinlockExit);