#include<linux/module.h>
#include<linux/kthread.h>
#include<linux/init.h>
#include<linux/delay.h>
#include<linux/kernel.h>
#include<linux/completion.h>
#define ITERATIONS 10
MODULE_AUTHOR("Jayaraj");
MODULE_DESCRIPTION("Race condition demonstration");
MODULE_LICENSE("GPL");

static struct task_struct *thread1;
static struct task_struct *thread2;


static struct completion done1;
static struct completion done2;
static struct mutex lock;
int ThreadOne_funtion(void *data);
int ThreadTwo_funtion(void *data);

int ThreadOne_funtion(void *data)
{
 struct completion *done = (struct completion *)data;
 mutex_lock(&lock);
 pr_info("worker1: Acquired lock...\n");
 msleep(1000);
 mutex_unlock(&lock);
 pr_info("worker1: Released lock...\n");
 complete(done);
 return 0;
}

int ThreadTwo_funtion(void *data)
{
    struct completion *done = (struct completion *)data;
    msleep(100);
    if(mutex_trylock(&lock)){
        pr_info("worker2: Acquired lock...\n");
        mutex_unlock(&lock);
        pr_info("worker2: Released lock...\n");
    }
    else{
        pr_info("worker2: Failed to acquire lock...\n");
    }
    complete(done);
    return 0;

}

static int __init race_init(void){
    pr_info("Race module loaded...\n");
    init_completion(&done1);
    init_completion(&done2);
    mutex_init(&lock);
    thread1 = kthread_run(ThreadOne_funtion,&done1,"worker1");
    if(IS_ERR(thread1)){
        pr_err("Failed to create a kthread...\n");
        return PTR_ERR(thread1);
    }
    thread2 = kthread_run(ThreadTwo_funtion,&done2,"worker2");
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
    pr_info("Race module unloaded...\n");
}

module_init(race_init);
module_exit(race_exit);
