// memory.h
#ifndef MEMORY_H
#define MEMORY_H

#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <asm/pgtable.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 61)
static inline phys_addr_t translate_linear_address(struct mm_struct *mm, uintptr_t va) {
    pgd_t *pgd = pgd_offset(mm, va);
    if (pgd_none(*pgd) || pgd_bad(*pgd)) return 0;

    p4d_t *p4d = p4d_offset(pgd, va);
    if (p4d_none(*p4d) || p4d_bad(*p4d)) return 0;

    pud_t *pud = pud_offset(p4d, va);
    if (pud_none(*pud) || pud_bad(*pud)) return 0;

    pmd_t *pmd = pmd_offset(pud, va);
    if (pmd_none(*pmd)) return 0;

    pte_t *pte = pte_offset_kernel(pmd, va);
    if (pte_none(*pte) || !pte_present(*pte)) return 0;

    phys_addr_t page_addr = (phys_addr_t)(pte_pfn(*pte) << PAGE_SHIFT);
    uintptr_t page_offset = va & (PAGE_SIZE - 1);
    return page_addr + page_offset;
}
#else
static inline phys_addr_t translate_linear_address(struct mm_struct *mm, uintptr_t va) {
    pgd_t *pgd = pgd_offset(mm, va);
    if (pgd_none(*pgd) || pgd_bad(*pgd)) return 0;

    pud_t *pud = pud_offset(pgd, va);
    if (pud_none(*pud) || pud_bad(*pud)) return 0;

    pmd_t *pmd = pmd_offset(pud, va);
    if (pmd_none(*pmd)) return 0;

    pte_t *pte = pte_offset_kernel(pmd, va);
    if (pte_none(*pte) || !pte_present(*pte)) return 0;

    phys_addr_t page_addr = (phys_addr_t)(pte_pfn(*pte) << PAGE_SHIFT);
    uintptr_t page_offset = va & (PAGE_SIZE - 1);
    return page_addr + page_offset;
}
#endif

static inline bool read_process_memory(pid_t pid, uintptr_t addr, void *buffer, size_t size) {
    struct task_struct *task = pid_task(find_vpid(pid), PIDTYPE_PID);
    if (!task) return false;

    struct mm_struct *mm = get_task_mm(task);
    if (!mm) return false;

    size_t total = 0;
    while (size > 0) {
        phys_addr_t pa = translate_linear_address(mm, addr);
        if (!pa) break;

        size_t offset = addr & (PAGE_SIZE - 1);
        size_t len = min(PAGE_SIZE - offset, size);

        void *mapped = ioremap_cache(pa, len);
        if (!mapped) break;

        if (copy_to_user(buffer, mapped, len)) {
            iounmap(mapped);
            break;
        }

        iounmap(mapped);
        addr += len;
        buffer += len;
        size -= len;
        total += len;
    }

    mmput(mm);
    return total > 0;
}

static inline bool write_process_memory(pid_t pid, uintptr_t addr, void *buffer, size_t size) {
    struct task_struct *task = pid_task(find_vpid(pid), PIDTYPE_PID);
    if (!task) return false;

    struct mm_struct *mm = get_task_mm(task);
    if (!mm) return false;

    size_t total = 0;
    while (size > 0) {
        phys_addr_t pa = translate_linear_address(mm, addr);
        if (!pa) break;

        size_t offset = addr & (PAGE_SIZE - 1);
        size_t len = min(PAGE_SIZE - offset, size);

        void *mapped = ioremap_cache(pa, len);
        if (!mapped) break;

        if (copy_from_user(mapped, buffer, len)) {
            iounmap(mapped);
            break;
        }

        iounmap(mapped);
        addr += len;
        buffer += len;
        size -= len;
        total += len;
    }

    mmput(mm);
    return total > 0;
}

#endif
