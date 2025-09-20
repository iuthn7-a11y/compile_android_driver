// process.h
#ifndef PROCESS_H
#define PROCESS_H

#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/dcache.h>
#include <linux/pid.h>

#define ARC_PATH_MAX 256

static inline unsigned long get_module_base(pid_t pid, const char *name) {
    struct task_struct *task = pid_task(find_vpid(pid), PIDTYPE_PID);
    if (!task) return 0;

    struct mm_struct *mm = get_task_mm(task);
    if (!mm) return 0;

    struct vm_area_struct *vma;
    unsigned long base = 0;
    char buf[ARC_PATH_MAX];

    for (vma = mm->mmap; vma; vma = vma->vm_next) {
        if (vma->vm_file) {
            char *path = d_path(&vma->vm_file->f_path, buf, sizeof(buf));
            if (!IS_ERR(path) && strstr(path, name)) {
                base = vma->vm_start;
                break;
            }
        }
    }

    mmput(mm);
    return base;
}

#endif
