#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kprobes.h>
#include <asm/syscall.h>
#include <linux/kallsyms.h>
#include <asm/pgtable.h>

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Detect module install");

static struct kprobe kp = {
    .symbol_name = "kallsyms_lookup_name"
};

typedef unsigned long (*_kallsyms_lookup_name_t)(const char *);
static _kallsyms_lookup_name_t _kallsyms_lookup_name = NULL;

static sys_call_ptr_t *_sys_call_table = NULL;

static sys_call_ptr_t orig_init_module;
static sys_call_ptr_t orig_finit_module;

static void
save_original_syscall(void)
{
    orig_init_module  = _sys_call_table[__NR_init_module];
    orig_finit_module = _sys_call_table[__NR_finit_module];
}

static void
change_page_attr_to_rw(pte_t *pte)
{
    set_pte_atomic(pte, pte_mkwrite(*pte));
}

static void
change_page_attr_to_ro(pte_t *pte)
{
    set_pte_atomic(pte, pte_clear_flags(*pte, _PAGE_RW));
}

static void
replace_system_call(void *new_init_module, void *new_finit_module)
{
    unsigned int level = 0;
    pte_t *pte;
    
    pte = lookup_address((unsigned long) _sys_call_table, &level);
    /* Need to set r/w to a page which _sys_call_table is in. */
    change_page_attr_to_rw(pte);
    _sys_call_table[__NR_init_module]  = new_init_module;
    _sys_call_table[__NR_finit_module] = new_finit_module;
    /* set back to read only */
    change_page_attr_to_ro(pte);
}

static asmlinkage long
my_init_module(const struct pt_regs *regs)
{
    long orig;

    pr_info("call my_init_module\n");
    orig = orig_init_module(regs);

    return orig;
}

static asmlinkage long
my_finit_module(const struct pt_regs *regs)
{
    long orig;

    pr_info("call my_finit_module\n");
    orig = orig_finit_module(regs);

    return orig;
}

static int __init
modhook_init(void)
{
    register_kprobe(&kp);
    _kallsyms_lookup_name = (void *)kp.addr;
    pr_info("kallsyms_lookup_name's address is 0x%px", _kallsyms_lookup_name);
    unregister_kprobe(&kp);
    
    _sys_call_table = (void *)_kallsyms_lookup_name("sys_call_table");
    pr_info("sys_call_table address is 0x%px\n", _sys_call_table);
    
    save_original_syscall();
    pr_info("original __x64_sys_init_module's address is %px\n", orig_init_module);
    pr_info("original __x64_sys_finit_module's address is %px\n", orig_finit_module);

    replace_system_call(my_init_module, my_finit_module);
    pr_info("system call replaced\n");
    
    return 0;
}

static void __exit
modhook_cleanup(void)
{
    pr_info("cleanup");
    if (orig_init_module && orig_finit_module)
        replace_system_call(orig_init_module, orig_finit_module);
}

module_init(modhook_init);
module_exit(modhook_cleanup);
