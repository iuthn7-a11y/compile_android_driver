// hide_driver.c
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#include "comm.h"
#include "memory.h"
#include "process.h"

#define CMD_READ  0x1337
#define CMD_WRITE 0x1338
#define CMD_BASE  0x1339

static COPY_MEMORY cm;
static MODULE_BASE mb;
static char name[0x100];

static int handler_getpid_pre(struct kprobe *p, struct pt_regs *regs)
{
    unsigned long cmd = regs->di;
    unsigned long arg = regs->si;

    if (cmd == CMD_READ) {
        if (copy_from_user(&cm, (void __user *)arg, sizeof(cm)) == 0)
            read_process_memory(cm.pid, cm.addr, cm.buffer, cm.size);
    } else if (cmd == CMD_WRITE) {
        if (copy_from_user(&cm, (void __user *)arg, sizeof(cm)) == 0)
            write_process_memory(cm.pid, cm.addr, cm.buffer, cm.size);
    } else if (cmd == CMD_BASE) {
        if (copy_from_user(&mb, (void __user *)arg, sizeof(mb)) == 0 &&
            copy_from_user(name, (void __user *)mb.name, sizeof(name)) == 0) {
            mb.base = get_module_base(mb.pid, name);
            copy_to_user((void __user *)arg, &mb, sizeof(mb));
        }
    }

    return 0;
}

static struct kprobe kp = {
    .symbol_name = "__x64_sys_getpid",
    .pre_handler = handler_getpid_pre,
};

static int __init hide_driver_init(void)
{
    if (register_kprobe(&kp) < 0)
        return -1;

    // 隐藏模块
    list_del_init(&THIS_MODULE->list);
    kobject_del(&THIS_MODULE->mkobj.kobj);

    return 0;
}

static void __exit hide_driver_exit(void)
{
    unregister_kprobe(&kp);
}

module_init(hide_driver_init);
module_exit(hide_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("RT");
MODULE_DESCRIPTION("No-trace kernel driver for PUBG Mobile");
