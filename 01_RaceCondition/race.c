#include<linux/module.h>
#include<linux/kthread.h>
#include<linux/init.h>
#include<linux/delay.h>
#include<linux/kernel.h>
#include<linux/completion.h>
#define ITERATIONS 10000
MODULE_AUTHOR("Jayaraj");
MODULE_DESCRIPTION("Race condition demonstration");
MODULE_LICENSE("GPL");

static struct task_struct *thread1;
static struct task_struct *thread2;


static struct completion done1;
static struct completion done2;

int Thread_funtion(void *data);
static int count = 0;

int Thread_funtion(void *data)
{
    int i = 0;
    int temp;
    struct completion *done = (struct completion *)data;
    for (; i < ITERATIONS; i++) {
        temp = count;
        msleep(10);
        count = temp + 1;
    }

    complete(done);
    return 0;
}
static int __init race_init(void){
    pr_info("Race module loaded...\n");
    init_completion(&done1);
    init_completion(&done2);

    thread1 = kthread_run(Thread_funtion,&done1,"worker1");
    if(IS_ERR(thread1)){
        pr_err("Failed to create a kthread...\n");
        return PTR_ERR(thread1);
    }
    thread2 = kthread_run(Thread_funtion,&done2,"worker2");
    if(IS_ERR(thread2)){
        pr_err("Failed to create a kthread...\n");
        wait_for_completion(&done1);
        return PTR_ERR(thread2);
    }
    
    return 0;
}
static void __exit race_exit(void){
    wait_for_completion(&done1);
    wait_for_completion(&done2);
    pr_info("Final Counter = %d\n",count);
    pr_info("Expected count = %d\n",2*ITERATIONS);
    pr_info("Race module unloaded...\n");
}

module_init(race_init);
module_exit(race_exit);
