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


struct mydevice{
    int count;
    struct mutex lock;
};

static struct mydevice dev1;
static struct mydevice dev2;

int Thread_funtion(void *data);

int Thread_funtion(void *data)
{
    int i = 0;
    struct completion *done;
    struct mydevice *dev = data;
    

    if(dev == &dev1){
        done = &done1;
    }
    else{
        done = &done2;
    }
    for (i = 0; i < ITERATIONS; i++) {
        mutex_lock(&dev->lock);
        dev->count++;
        pr_info("Running thread: %s\n", current->comm);
        mutex_unlock(&dev->lock);
        msleep(100);
    }
    pr_info("%s finished\n", current->comm);
    complete(done);
    return 0;
}
static int __init race_init(void){
    pr_info("Race module loaded...\n");
    init_completion(&done1);
    init_completion(&done2);

    mutex_init(&dev1.lock);
    mutex_init(&dev2.lock);

    thread1 = kthread_run(Thread_funtion,&dev1,"worker1");
    if(IS_ERR(thread1)){
        pr_err("Failed to create a kthread...\n");
        return PTR_ERR(thread1);
    }
    thread2 = kthread_run(Thread_funtion,&dev2,"worker2");
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
    pr_info("Device 1 final count = %d\n", dev1.count);
    pr_info("Device 2 final count = %d\n", dev2.count);
    pr_info("Multiple device mutex module unloaded\n");
}

module_init(race_init);
module_exit(race_exit);
