#include<linux/module.h>
#include<linux/kthread.h>
#include<linux/init.h>
#include<linux/delay.h>
#include<linux/kernel.h>
#include<linux/completion.h>
#include<linux/hrtimer.h>
MODULE_AUTHOR("Jayaraj");
MODULE_DESCRIPTION("spinlock irq save demonstration");
MODULE_LICENSE("GPL");

static struct hrtimer hrt;
static spinlock_t lock;
static u64 count = 0;
enum hrtimer_restart timer_callback(struct hrtimer *timer);
enum hrtimer_restart timer_callback(struct hrtimer *timer){
    spin_lock(&lock);
    pr_info("hrtimer get the lock..\n");
    count++;
    spin_unlock(&lock);
    pr_info("hrtimer released lock..\n");
    return HRTIMER_NORESTART;
}
static int __init spinlockInit(void){
    unsigned long flag;
    spin_lock_init(&lock);
    pr_info("Spin lock initlized successfully...\n");
    hrtimer_init(&hrt,CLOCK_MONOTONIC,HRTIMER_MODE_REL);
    ktime_t interval;
    interval = ms_to_ktime(100);
    hrt.function = timer_callback;
    hrtimer_start(&hrt,interval,HRTIMER_MODE_REL);
    msleep(99);
    spin_lock_irqsave(&lock,flag);
    pr_info("status register value %lu\n",flag);
    pr_info("process context thread get lock...\n");
    count++;
    spin_unlock_irqrestore(&lock,flag);
    pr_info("procee context released a lock...\n");
	return 0;
}
static void __exit spinlockExit(void){
    hrtimer_cancel(&hrt);
	pr_info("Expected value = %d\n",2);
	pr_info("Actual value = %llu\n",(unsigned long long)count);
	pr_info("Spin lock module exited successfully...\n");
}
module_init(spinlockInit);
module_exit(spinlockExit);
