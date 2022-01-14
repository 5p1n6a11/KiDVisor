#include <share/vmm_types.h>
#include "current.h"
#include "printf.h"
#include "vmmcall.h"
#include "initfunc.h"

static void
modhook(void)
{
    ulong modaddr;
    ulong modsize;
    ulong ret = 0;

    current->vmctl.read_general_reg(GENERAL_REG_RBX, &modaddr);
    current->vmctl.read_general_reg(GENERAL_REG_RCX, &modsize);

    printf("modhook: module has loaded at 0x%lx\n", modaddr);
    printf("modhook: module size is %ld\n", modsize);

    current->vmctl.write_general_reg(GENERAL_REG_RAX, (ulong)ret);
}

static void
vmmcall_modhook_init(void)
{
    vmmcall_register("modhook", modhook);
}
INITFUNC ("vmmcal0", vmmcall_modhook_init);
