#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/init.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("stack trace demo");
MODULE_AUTHOR("Jayaraj");

static void fuctionA(void);
static void fuctionB(void);
static void fuctionC(void);


static void fuctionA(void){
	pr_info("function A invoked...\n");
	fuctionB();
}
static void fuctionB(void){
	pr_info("function B invoked...\n");
	fuctionC();
}
static void fuctionC(void){
	pr_info("funtion C invoked...\n");
	dump_stack();
}

static int __init straceInit(void){
  pr_info("strace funtion Initialized...\n");
  fuctionA();
  return 0;
}
static void __exit straceExit(void){
	pr_info("module exited....\n");
}

module_init(straceInit);
module_exit(straceExit);