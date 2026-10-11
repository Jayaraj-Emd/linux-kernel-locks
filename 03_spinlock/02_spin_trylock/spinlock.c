#include<linux/module.h>
#include<linux/kthread.h>
#include<linux/init.h>
#include<linux/delay.h>
#include<linux/kernel.h>
#include<linux/completion.h>

MODULE_AUTHOR("Jayaraj");
MODULE_DESCRIPTION("Race condition demonstration");
MODULE_LICENSE("GPL");

static struct task_struct *thread1;
static struct task_struct *thread2;


static struct completion done1;
static struct completion done2;
static spinlock_t lock;
static u64 count = 0;
static int thread_fun1(void *data){
    
    struct completion *done = (struct completion *)data;
    spin_lock(&lock);
    pr_info("thread 1 acquired lock\n");
    udelay(100);
    count++;
    spin_unlock(&lock);
    pr_info("thread 1 released lock\n");
    complete(done);
    return 0;
}

static int thread_fun2(void *data){
    struct completion *done = (struct completion *)data;
    if(spin_trylock(&lock)){
       
       pr_info("Lock acquired by thread 2..\n");
       count++;
       spin_unlock(&lock);
    }
    else{
       pr_info("Lock not acquired..\n");
    }
    complete(done);
    return 0;
}

static int __init spinlockInit(void){
	init_completion(&done1);
	init_completion(&done2);
    spin_lock_init(&lock);

    thread1 = kthread_run(thread_fun1,&done1,"worker1");
    if(IS_ERR(thread1)){
        pr_err("Failed to create thread, err code = %d\n", (int)PTR_ERR(thread1));
        return PTR_ERR(thread1);
    }

    thread2 = kthread_run(thread_fun2,&done2,"worker2");
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
	pr_info("Expected value = %d\n",2);
	pr_info("Actual value = %llu\n",(unsigned long long)count);
	pr_info("Spin lock module exited successfully...\n");
}
module_init(spinlockInit);
module_exit(spinlockExit);
