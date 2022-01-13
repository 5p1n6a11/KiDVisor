#include <share/vmm_types.h>
#include "current.h"
#include "printf.h"
#include "vmmcall.h"
#include "initfunc.h"

static void
hello(void)
{
    ulong rbx;
    ulong ret = 10;

    /* Receive arguments */
    current->vmctl.read_general_reg(GENERAL_REG_RBX, &rbx);

    printf("Hello iVisor!, arg1 = %ld\n", rbx);

    /* Store the return value */
    current->vmctl.write_general_reg(GENERAL_REG_RAX, (ulong)ret);
}

/* Register a function */
static void
vmmcall_hello_init(void)
{
    vmmcall_register("hello", hello);
}

INITFUNC ("vmmcal0", vmmcall_hello_init);
