#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kprobes.h>
#include <asm/syscall.h>
#include <linux/kallsyms.h>
#include <asm/pgtable.h>

#include "modhook.h"

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Detect module install");

#define VMMCALL_TYPE_ERROR          0
#define VMMCALL_TYPE_VMCALL         1
#define VMMCALL_TYPE_VMMCALL        2

/* "get_vmmcall_number" vmmcall number is 0. */
#define GET_VMMCALL_NUMBER          0

static unsigned long (*_kallsyms_lookup_name)(const char *) = NULL;

static sys_call_ptr_t *_sys_call_table = NULL;

static sys_call_ptr_t orig_init_module = NULL;
static sys_call_ptr_t orig_finit_module = NULL;

static struct module *(*_find_module)(const char *) = NULL;

static char *target = "hello";
module_param(target, charp, S_IRUGO);

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

static DEFINE_MUTEX(module_mutex);

static struct module*
get_module(const char *modname)
{
    struct module *mod = NULL;

    if (mutex_lock_interruptible(&module_mutex) != 0) {
        goto mutex_fail;
    }
    
    _find_module = (void *)_kallsyms_lookup_name("find_module");
    // pr_info("find_module address is 0x%px\n", _find_module);

    mod = _find_module(modname);

    mutex_unlock(&module_mutex);

mutex_fail:
    pr_info("get_module(%s) = %p\n", modname, mod);
    
    return mod;
}

static void
call_vmm_call_function(call_vmm_function_t *function,
                        call_vmm_arg_t *arg, call_vmm_ret_t *ret)
{
    struct call_vmm_call_function_sub_data data;
    data.function = function;
    data.arg = arg;
    data.ret = ret;
    pr_info("vmcall %d(arg=0x%lx, 0x%lx)\n",data.function->vmmcall_number, data.arg->rbx, data.arg->rcx);
    switch (data.function->vmmcall_type) {
    case VMMCALL_TYPE_VMCALL:
        asm volatile("vmcall"
                        : "=a" (data.ret->rax), "=b" (data.ret->rbx),
                            "=c" (data.ret->rcx), "=d" (data.ret->rdx),
                            "=S" (data.ret->rsi), "=D" (data.ret->rdi)
                        : "a" (data.function->vmmcall_number),
                            "b" (data.arg->rbx),
                            "c" (data.arg->rcx), "d" (data.arg->rdx),
                            "S" (data.arg->rsi), "D" (data.arg->rdi)
                        : "memory");
        break;
    case VMMCALL_TYPE_VMMCALL:
        asm volatile("vmmcall"
                        : "=a" (data.ret->rax), "=b" (data.ret->rbx),
                            "=c" (data.ret->rcx), "=d" (data.ret->rdx),
                            "=S" (data.ret->rsi), "=D" (data.ret->rdi)
                        : "a" (data.function->vmmcall_number),
                            "b" (data.arg->rbx),
                            "c" (data.arg->rcx), "d" (data.arg->rdx),
                            "S" (data.arg->rsi), "D" (data.arg->rdi)
                        : "memory");
        break;
    default:
        break;
    }
}

/* Get an entry number of specified vmmcall */
void vmmcall_get_function(const char *vmmcall, call_vmm_function_t *res)
{
    call_vmm_function_t gf;
    call_vmm_arg_t gf_a;
    call_vmm_ret_t gf_r;

    gf.vmmcall_number = GET_VMMCALL_NUMBER;
    gf.vmmcall_type = VMMCALL_TYPE_VMCALL;

    gf_a.rbx = (intptr_t)vmmcall;
    pr_info("vmmcall_string: %p, 0x%lx, %s\n", vmmcall, gf_a.rbx, vmmcall);
    call_vmm_call_function(&gf, &gf_a, &gf_r);

    /* RAX = vmmcall number */
    res->vmmcall_number = (int)gf_r.rax;
    res->vmmcall_type = VMMCALL_TYPE_VMCALL; // XXX: Should get this value too.
}

static void
vmcall_modhook(void *base, unsigned long size)
{
    call_vmm_function_t modf;
    call_vmm_arg_t mod_a;
    call_vmm_ret_t mod_r;
    const char modhook[] = "modhook";

    pr_info("base = %px\n", base);
    pr_info("size = %ld\n", size);

    vmmcall_get_function(modhook, &modf);
    pr_info("modhook number = %d\n", modf.vmmcall_number);

    mod_a.rbx = (intptr_t)base;
    mod_a.rcx = size;
    call_vmm_call_function(&modf, &mod_a, &mod_r);
}

static asmlinkage long
hook_init_module(const struct pt_regs *regs)
{
    long orig;
    struct module *mod = NULL;

    pr_info("call hook_init_module\n");
    orig = orig_init_module(regs);

    mod = get_module(target);
    if (mod) {
        pr_info("module's name is %s\n", mod->name);
        vmcall_modhook(mod->core_layout.base, mod->core_layout.ro_size);
    }

    return orig;
}

static asmlinkage long
hook_finit_module(const struct pt_regs *regs)
{
    long orig;
    struct module *mod = NULL;

    pr_info("call hook_finit_module\n");
    orig = orig_finit_module(regs);

    mod = get_module(target);
    if (mod) {
        pr_info("module's name is %s\n", mod->name);
        vmcall_modhook(mod->core_layout.base, mod->core_layout.ro_size);
    }

    return orig;
}

static int __init
modhook_init(void)
{
    static struct kprobe kp = {
        .symbol_name = "kallsyms_lookup_name"
    };

    register_kprobe(&kp);
    _kallsyms_lookup_name = (void *)kp.addr;
    pr_info("kallsyms_lookup_name's address is 0x%px", _kallsyms_lookup_name);
    unregister_kprobe(&kp);
    
    _sys_call_table = (void *)_kallsyms_lookup_name("sys_call_table");
    pr_info("sys_call_table address is 0x%px\n", _sys_call_table);
    
    save_original_syscall();
    pr_info("original __x64_sys_init_module's address is %px\n", orig_init_module);
    pr_info("original __x64_sys_finit_module's address is %px\n", orig_finit_module);

    replace_system_call(hook_init_module, hook_finit_module);
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
