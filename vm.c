#include "vm.h"
#include "utf8.h"
#include "types.h"
#include "buffer.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <inttypes.h>
#include <string.h>

typedef struct
{
    uint32_t ip;          // Points the index of current machine instruction to execute: program[ip] or *(program + ip)
    uint32_t sp;          // Points the top element of the machine stack: stack[sp]
    uint32_t bp;          // Base index
    value_t* stack;
    size_t stack_size;
    buffer_t code;
    buffer_t data;
    struct {
        uint8_t halt: 1;
    } flags;
} vm_t;

static vm_t vm;

size_t vm_dasm_opcode(FILE *file, size_t ip);

// NOTE: KEEP THE ORDER AS SAME AS OPCODE ENUM
// OTHERWISE THE DASM WILL BE WRONG
const opcode_t OPCODES[] = {
    {NOP, 0, "nop"},
    {DUP, 0, "dup"},
    {DROP, 0, "drop"},
    {ALLC, 0, "allc"},
    {SWAP, 0, "swap"},
    {PROC, 4, "proc"},
    {CALL, 2, "call"},
    {RET, 0, "ret"},
    {JNZ, 2, "jnz"},
    {JEZ, 2, "jez"},
    {JMP, 2, "jmp"},
    {HALT, 0, "halt"},
    {IINC, 0, "iinc"},
    {IDEC, 0, "idec"},
    {INEG, 0, "ineg"},
    {IABS, 0, "iabs"},
    {INOT, 0, "inot"},
    {IADD, 0, "iadd"},
    {ISUB, 0, "isub"},
    {IDIV, 0, "idiv"},
    {IMOD, 0, "imod"},
    {IMUL, 0, "imul"},
    {IAND, 0, "iand"},
    {IOR, 0, "ior"},
    {IBXOR, 0, "ibxor"},
    {IBOR, 0, "ibor"},
    {IBAND, 0, "iband"},
    {ISHL, 0, "ishl"},
    {ISHR, 0, "ishr"},
    {IGT, 0, "igt"},
    {ILT, 0, "ilt"},
    {IGE, 0, "ige"},
    {ILE, 0, "ile"},
    {IEQ, 0, "ieq"},
    {INQ, 0, "inq"},
    {I8CONST, 1, "i8const"},
    {I16CONST, 2, "i16const"},
    {I32CONST, 4, "i32const"},
    {I64CONST, 8, "i64const"},
    {ICONST_0, 0, "iconst_0"},
    {ICONST_1, 0, "iconst_1"},
    {IPRINT, 1, "iprint"},
    {I8CAST, 0, "i8cast"},
    {I16CAST, 0, "i16cast"},
    {I32CAST, 0, "i32cast"},
    {I64CAST, 0, "i64cast"},
    {IU8CAST, 0, "iu8cast"},
    {IU16CAST, 0, "iu16cast"},
    {IU32CAST, 0, "iu32cast"},
    {IU64CAST, 0, "iu64cast"},
    {ILOAD, 2, "iload"},
    {ISTORE, 2, "istore"},
    {ITOR, 0, "itor"},
    {RINC, 0, "rinc"},
    {RDEC, 0, "rdec"},
    {RNEG, 0, "rneg"},
    {RABS, 0, "rabs"},
    {RADD, 0, "radd"},
    {RSUB, 0, "rsub"},
    {RDIV, 0, "rdiv"},
    {RMOD, 0, "rmod"},
    {RMUL, 0, "rmul"},
    {RPOW, 0, "rpow"},
    {RSQRT, 0, "rsqrt"},
    {REXP, 0, "rexp"},
    {RSIN, 0, "rsin"},
    {RCOS, 0, "rcos"},
    {RTAN, 0, "rtan"},
    {RASIN, 0, "rasin"},
    {RACOS, 0, "racos"},
    {RATAN2, 0, "ratan2"},
    {RLOG, 0, "rlog"},
    {RLOG10, 0, "rlog10"},
    {RLOG2, 0, "rlog2"},
    {RCEIL, 0, "rceil"},
    {RFLOOR, 0, "rfloor"},
    {RROUND, 0, "rround"},
    {RGT, 0, "rgt"},
    {RLT, 0, "rlt"},
    {RGE, 0, "rge"},
    {RLE, 0, "rle"},
    {REQ, 0, "req"},
    {RNQ, 0, "rnq"},
    {RLOAD, 2, "rload"},
    {RSTORE, 2, "rstore"},
    {RCONST, 8, "rconst"},
    {RCONST_0, 0, "rconst_0"},
    {RCONST_1, 0, "rconst_1"},
    {RCONST_PI, 0, "rconst_pi"},
    {RPRINT, 0, "rprint"},
    {RTOI, 0, "rtoi"},
    {SLOAD, 2, "sload"},
    {SSTORE, 2, "sstore"},
    {SCONST, 2, "sconst"},
    {SPRINT, 0, "sprint"},
    {SLEN, 0, "slen"},
    // XLOAD
    // XLOADG
    // XSTORE
    // XSTOREG
    {NPRINT, 0, "nprint"},
};

void vm_init()
{
    buffer_init(&vm.data, 0);
    buffer_init(&vm.code, 128);
    vm.stack_size = 32; // 32 * 8 = 256 as initial stack size
    vm.stack = malloc(sizeof (value_t) * vm.stack_size);
    vm.ip = 0;
    vm.sp = 0;
    vm.bp = 0;
    vm.flags.halt = 0;
}

void vm_free()
{
    free(vm.stack);
    buffer_free(&vm.data);
    buffer_free(&vm.code);
}

void print_stack(const char* prefix)
{
    printf("%s: [", prefix);
    for (int i=0; i<20; i++)
    {
        printf("%2lx%2c ", vm.stack[i].as_uint64, (i==vm.sp)?'<':' ');
    }
    printf("]\n");
}

void vm_check_stack(size_t n)
{
    if (vm.sp + n >= vm.stack_size)
    {
        size_t new_size = min_coverage_size(vm.sp + n);
        vm.stack = realloc(vm.stack, sizeof (value_t) * new_size);
        vm.stack_size = new_size;
    }
}

void exec_opcode(uint8_t* opcode)
{
    // print_stack("before");
    // printf("\t[ip: %3x, sp: %3d, bp: %3d] ", vm.ip, vm.sp, vm.bp);
    // vm_dasm_opcode(stdout, vm.ip);
    switch (*opcode)
    {
    case HALT:
    {
        vm.flags.halt = 1;
        ++vm.ip;
        break;
    }
    case NOP:
    {
        ++vm.ip;
        break;
    }
    case DUP:
    {
        vm_check_stack(1);
        vm.stack[vm.sp + 1] = vm.stack[vm.sp];
        ++vm.sp;
        ++vm.ip;
        break;
    }
    case SWAP:
    {
        value_t tmp = vm.stack[vm.sp];
        vm.stack[vm.sp] = vm.stack[vm.sp - 1];
        vm.stack[vm.sp - 1] = tmp;
        ++vm.ip;
        break;
    }
    case DROP:
    {
        --vm.sp;
        ++vm.ip;
        break;
    }
    case ALLC:
    {
        vm_check_stack(1);
        ++vm.sp;
        ++vm.ip;
        break;
    }
    case PROC:
    {
        uint16_t args = *((uint16_t*) (opcode + 1));
        uint16_t vars = *((uint16_t*) (opcode + 3));
        uint32_t _bp = vm.stack[vm.sp--].as_uint32;
        uint32_t _ip = vm.stack[vm.sp--].as_uint32;
        vm.stack[vm.sp+1].as_uint32 = 0; // clean
        vm.stack[vm.sp+2].as_uint32 = 0; // clean
        vm.bp = vm.sp - args + 1;
        vm_check_stack(vars); // -2 ?
        vm.sp += vars;
        vm.stack[++vm.sp].as_uint32 = _ip;
        vm.stack[++vm.sp].as_uint32 = _bp;
        vm.stack[++vm.sp].as_uint32 = args + vars;
        vm.ip += 5;
        break;
    }
    case CALL:
    {
        vm_check_stack(2);
        vm.stack[++vm.sp].as_uint32 = vm.ip + 3;
        vm.stack[++vm.sp].as_uint32 = vm.bp;
        vm.ip = *((uint16_t*) (opcode + 1));
        break;
    }
    case RET:
    {
        value_t retv = vm.stack[vm.sp--];
        uint32_t drops = vm.stack[vm.sp--].as_uint32;
        uint32_t _bp = vm.stack[vm.sp--].as_uint32;
        uint32_t _ip = vm.stack[vm.sp--].as_uint32;
        vm.sp -= drops;
        vm.stack[++vm.sp] = retv;
        vm.ip = (uint16_t) _ip;
        vm.bp = (uint16_t) _bp;
        break;
    }
    case JMP:
    {
        vm.ip = *((uint16_t*) (opcode + 1));
        break;
    }
    case JEZ:
    {
        // Check both int64 and real (for real comparison results)
        // Real comparisons return 0.0 or 1.0, so we need to check as_real first
        // to avoid reading as_int64 when the value is actually a real
        int is_zero = (vm.stack[vm.sp].as_real == 0.0) || (vm.stack[vm.sp].as_int64 == 0);
        if (is_zero)
            vm.ip = *((uint16_t*) (opcode + 1));
        else
            vm.ip += 3;
        --vm.sp;
        break;
    }
    case JNZ:
    {
        if (vm.stack[vm.sp].as_int64 != 0)
            vm.ip = *((uint16_t*) (opcode + 1));
        else
            vm.ip += 3;
        --vm.sp;
        break;
    }
    case IINC:
    {
        vm.stack[vm.sp].as_int64++;
        ++vm.ip;
        break;
    }
    case IDEC:
    {
        vm.stack[vm.sp].as_int64--;
        ++vm.ip;
        break;
    }
    case INEG:
    {
        vm.stack[vm.sp].as_int64 *= -1;
        ++vm.ip;
        break;
    }
    case IABS:
    {
        vm.stack[vm.sp].as_int64 = llabs(vm.stack[vm.sp].as_int64);
        ++vm.ip;
        break;
    }
    case INOT:
    {
        vm.stack[vm.sp].as_int64 = !vm.stack[vm.sp].as_int64;
        ++vm.ip;
        break;
    }
    case IADD:
    {
        vm.stack[vm.sp - 1].as_int64 = vm.stack[vm.sp - 1].as_int64 + vm.stack[vm.sp].as_int64;
        --vm.sp;
        ++vm.ip;
        break;
    }
    case ISUB:
    {
        vm.stack[vm.sp - 1].as_int64 = vm.stack[vm.sp - 1].as_int64 - vm.stack[vm.sp].as_int64;
        --vm.sp;
        ++vm.ip;
        break;
    }
    case IMUL:
    {
        vm.stack[vm.sp - 1].as_int64 = vm.stack[vm.sp - 1].as_int64 * vm.stack[vm.sp].as_int64;
        --vm.sp;
        ++vm.ip;
        break;
    }
    case IDIV:
    {
        vm.stack[vm.sp - 1].as_int64 = vm.stack[vm.sp - 1].as_int64 / vm.stack[vm.sp].as_int64;
        --vm.sp;
        ++vm.ip;
        break;
    }
    case IMOD:
    {
        vm.stack[vm.sp - 1].as_int64 = vm.stack[vm.sp - 1].as_int64 % vm.stack[vm.sp].as_int64;
        --vm.sp;
        ++vm.ip;
        break;
    }
    case IAND:
    {
        vm.stack[vm.sp - 1].as_int64 = vm.stack[vm.sp - 1].as_int64 && vm.stack[vm.sp].as_int64;
        --vm.sp;
        ++vm.ip;
        break;
    }
    case IOR:
    {
        vm.stack[vm.sp - 1].as_int64 = vm.stack[vm.sp - 1].as_int64 || vm.stack[vm.sp].as_int64;
        --vm.sp;
        ++vm.ip;
        break;
    }
    case IBXOR:
    {
        vm.stack[vm.sp - 1].as_int64 = vm.stack[vm.sp - 1].as_int64 ^ vm.stack[vm.sp].as_int64;
        --vm.sp;
        ++vm.ip;
        break;
    }
    case IBOR:
    {
        vm.stack[vm.sp - 1].as_int64 = vm.stack[vm.sp - 1].as_int64 | vm.stack[vm.sp].as_int64;
        --vm.sp;
        ++vm.ip;
        break;
    }
    case IBAND:
    {
        vm.stack[vm.sp - 1].as_int64 = vm.stack[vm.sp - 1].as_int64 & vm.stack[vm.sp].as_int64;
        --vm.sp;
        ++vm.ip;
        break;
    }
    case ISHL:
    {
        vm.stack[vm.sp - 1].as_int64 = vm.stack[vm.sp - 1].as_int64 << vm.stack[vm.sp].as_int64;
        --vm.sp;
        ++vm.ip;
        break;
    }
    case ISHR:
    {
        vm.stack[vm.sp - 1].as_int64 = vm.stack[vm.sp - 1].as_int64 >> vm.stack[vm.sp].as_int64;
        --vm.sp;
        ++vm.ip;
        break;
    }
    case IGT:
    {
        vm.stack[vm.sp - 1].as_int64 = vm.stack[vm.sp - 1].as_int64 > vm.stack[vm.sp].as_int64;
        --vm.sp;
        ++vm.ip;
        break;
    }
    case ILT:
    {
        vm.stack[vm.sp - 1].as_int64 = vm.stack[vm.sp - 1].as_int64 < vm.stack[vm.sp].as_int64;
        --vm.sp;
        ++vm.ip;
        break;
    }
    case IGE:
    {
        vm.stack[vm.sp - 1].as_int64 = vm.stack[vm.sp - 1].as_int64 >= vm.stack[vm.sp].as_int64;
        --vm.sp;
        ++vm.ip;
        break;
    }
    case ILE:
    {
        vm.stack[vm.sp - 1].as_int64 = vm.stack[vm.sp - 1].as_int64 <= vm.stack[vm.sp].as_int64;
        --vm.sp;
        ++vm.ip;
        break;
    }
    case IEQ:
    {
        vm.stack[vm.sp - 1].as_int64 = vm.stack[vm.sp - 1].as_int64 == vm.stack[vm.sp].as_int64;
        --vm.sp;
        ++vm.ip;
        break;
    }
    case INQ:
    {
        vm.stack[vm.sp - 1].as_int64 = vm.stack[vm.sp - 1].as_int64 != vm.stack[vm.sp].as_int64;
        --vm.sp;
        ++vm.ip;
        break;
    }
    case I8CONST:
    {
        vm_check_stack(1);
        int8_t val = (int8_t)opcode[1];
        vm.stack[++vm.sp].as_int64 = (int64_t)val;
        vm.ip += 2;
        break;
    }
    case I16CONST:
    {
        vm_check_stack(1);
        int16_t val = *((int16_t*)(opcode + 1));
        vm.stack[++vm.sp].as_int64 = (int64_t)val;
        vm.ip += 3;
        break;
    }
    case I32CONST:
    {
        vm_check_stack(1);
        int32_t val = *((int32_t*)(opcode + 1));
        vm.stack[++vm.sp].as_int64 = (int64_t)val;
        vm.ip += 5;
        break;
    }
    case I64CONST:
    {
        vm_check_stack(1);
        vm.stack[++vm.sp].as_int64 = *((int64_t*) (opcode + 1));
        vm.ip += 9;
        break;
    }
    case ICONST_0:
    {
        vm_check_stack(1);
        vm.stack[++vm.sp].as_int64 = 0;
        ++vm.ip;
        break;
    }
    case ICONST_1:
    {
        vm_check_stack(1);
        vm.stack[++vm.sp].as_int64 = 1;
        ++vm.ip;
        break;
    }
    case IPRINT:
    {
        type_t type = *((int8_t*) (opcode + 1));
        switch (type)
        {
            case MT_INT8: printf("%" PRIi8, vm.stack[vm.sp].as_int8); break;
            case MT_INT16: printf("%" PRIi16, vm.stack[vm.sp].as_int16); break;
            case MT_INT32: printf("%" PRIi32, vm.stack[vm.sp].as_int32); break;
            case MT_INT64: printf("%" PRIi64, vm.stack[vm.sp].as_int64); break;
            case MT_UINT8: printf("%" PRIu8, vm.stack[vm.sp].as_uint8); break;
            case MT_UINT16: printf("%" PRIu16, vm.stack[vm.sp].as_uint16); break;
            case MT_UINT32: printf("%" PRIu32, vm.stack[vm.sp].as_uint32); break;
            case MT_UINT64: printf("%" PRIu64, vm.stack[vm.sp].as_uint64); break;
            default: printf("%" PRIx64, vm.stack[vm.sp].as_uint64); break;
        }
        fflush(stdout);
        --vm.sp;
        vm.ip += 2;
        break;
    }
    case ITOR:
    {
        vm.stack[vm.sp].as_real = (real_t) vm.stack[vm.sp].as_int64;
        ++vm.ip;
        break;
    }
    case I8CAST:
    {
        vm.stack[vm.sp].as_int64 = (int64_t)vm.stack[vm.sp].as_int8;
        ++vm.ip;
        break;
    }
    case I16CAST:
    {
        vm.stack[vm.sp].as_int64 = (int64_t)vm.stack[vm.sp].as_int16;
        ++vm.ip;
        break;
    }
    case I32CAST:
    {
        vm.stack[vm.sp].as_int64 = (int64_t)vm.stack[vm.sp].as_int32;
        ++vm.ip;
        break;
    }
    case I64CAST:
    {
        vm.stack[vm.sp].as_int64 = (int64_t)vm.stack[vm.sp].as_int64;
        ++vm.ip;
        break;
    }
    case IU8CAST:
    {
        vm.stack[vm.sp].as_int64 = (int64_t)vm.stack[vm.sp].as_uint8;
        ++vm.ip;
        break;
    }
    case IU16CAST:
    {
        vm.stack[vm.sp].as_int64 = (int64_t)vm.stack[vm.sp].as_uint16;
        ++vm.ip;
        break;
    }
    case IU32CAST:
    {
        vm.stack[vm.sp].as_int64 = (int64_t)vm.stack[vm.sp].as_uint32;
        ++vm.ip;
        break;
    }
    case IU64CAST:
    {
        vm.stack[vm.sp].as_int64 = (int64_t)vm.stack[vm.sp].as_uint64;
        ++vm.ip;
        break;
    }
    case ILOAD:
    {
        vm_check_stack(1);
        vm.stack[vm.sp + 1].as_int64 = vm.stack[vm.bp + *((uint16_t*) (opcode + 1))].as_int64;
        ++vm.sp;
        vm.ip += 3;
        break;
    }
    case ISTORE:
    {
        vm.stack[vm.bp + *((uint16_t*) (opcode + 1))].as_int64 = vm.stack[vm.sp].as_int64;
        --vm.sp;
        vm.ip += 3;
        break;
    }
    case RLOAD:
    {
        vm_check_stack(1);
        vm.stack[vm.sp + 1].as_real = vm.stack[vm.bp + *((uint16_t*) (opcode + 1))].as_real;
        ++vm.sp;
        vm.ip += 3;
        break;
    }
    case RSTORE:
    {
        vm.stack[vm.bp + *((uint16_t*) (opcode + 1))].as_real = vm.stack[vm.sp].as_real;
        --vm.sp;
        vm.ip += 3;
        break;
    }
    case RINC:
    {
        vm.stack[vm.sp].as_real++;
        ++vm.ip;
        break;
    }
    case RDEC:
    {
        vm.stack[vm.sp].as_real--;
        ++vm.ip;
        break;
    }
    case RNEG:
    {
        vm.stack[vm.sp].as_real *= -1;
        ++vm.ip;
        break;
    }
    case RABS:
    {
        vm.stack[vm.sp].as_real = fabs(vm.stack[vm.sp].as_real);
        ++vm.ip;
        break;
    }
    case RADD:
    {
        vm.stack[vm.sp - 1].as_real = vm.stack[vm.sp - 1].as_real + vm.stack[vm.sp].as_real;
        --vm.sp;
        ++vm.ip;
        break;
    }
    case RSUB:
    {
        vm.stack[vm.sp - 1].as_real = vm.stack[vm.sp - 1].as_real - vm.stack[vm.sp].as_real;
        --vm.sp;
        ++vm.ip;
        break;
    }
    case RMUL:
    {
        vm.stack[vm.sp - 1].as_real = vm.stack[vm.sp - 1].as_real * vm.stack[vm.sp].as_real;
        --vm.sp;
        ++vm.ip;
        break;
    }
    case RDIV:
    {
        vm.stack[vm.sp - 1].as_real = vm.stack[vm.sp - 1].as_real / vm.stack[vm.sp].as_real;
        --vm.sp;
        ++vm.ip;
        break;
    }
    case RMOD:
    {
        vm.stack[vm.sp - 1].as_real = fmod(vm.stack[vm.sp - 1].as_real, vm.stack[vm.sp].as_real);
        --vm.sp;
        ++vm.ip;
        break;
    }
    case RPOW:
    {
        vm.stack[vm.sp - 1].as_real = pow(vm.stack[vm.sp - 1].as_real, vm.stack[vm.sp].as_real);
        --vm.sp;
        ++vm.ip;
        break;
    }
    case RSQRT:
    {
        vm.stack[vm.sp].as_real = sqrt(vm.stack[vm.sp].as_real);
        ++vm.ip;
        break;
    }
    case REXP:
    {
        vm.stack[vm.sp].as_real = exp(vm.stack[vm.sp].as_real);
        ++vm.ip;
        break;
    }
    case RSIN:
    {
        vm.stack[vm.sp].as_real = sin(vm.stack[vm.sp].as_real);
        ++vm.ip;
        break;
    }
    case RCOS:
    {
        vm.stack[vm.sp].as_real = cos(vm.stack[vm.sp].as_real);
        ++vm.ip;
        break;
    }
    case RTAN:
    {
        vm.stack[vm.sp].as_real = tan(vm.stack[vm.sp].as_real);
        ++vm.ip;
        break;
    }
    case RASIN:
    {
        vm.stack[vm.sp].as_real = asin(vm.stack[vm.sp].as_real);
        ++vm.ip;
        break;
    }
    case RACOS:
    {
        vm.stack[vm.sp].as_real = acos(vm.stack[vm.sp].as_real);
        ++vm.ip;
        break;
    }
    case RATAN2:
    {
        vm.stack[vm.sp - 1].as_real = atan2(vm.stack[vm.sp - 1].as_real, vm.stack[vm.sp].as_real);
        --vm.sp;
        ++vm.ip;
        break;
    }
    case RLOG:
    {
        vm.stack[vm.sp].as_real = log(vm.stack[vm.sp].as_real);
        ++vm.ip;
        break;
    }
    case RLOG10:
    {
        vm.stack[vm.sp].as_real = log10(vm.stack[vm.sp].as_real);
        ++vm.ip;
        break;
    }
    case RLOG2:
    {
        vm.stack[vm.sp].as_real = log2(vm.stack[vm.sp].as_real);
        ++vm.ip;
        break;
    }
    case RCEIL:
    {
        vm.stack[vm.sp].as_real = ceil(vm.stack[vm.sp].as_real);
        ++vm.ip;
        break;
    }
    case RFLOOR:
    {
        vm.stack[vm.sp].as_real = floor(vm.stack[vm.sp].as_real);
        ++vm.ip;
        break;
    }
    case RROUND:
    {
        vm.stack[vm.sp].as_real = round(vm.stack[vm.sp].as_real);
        ++vm.ip;
        break;
    }
    case RGT:
    {
        vm.stack[vm.sp - 1].as_real = vm.stack[vm.sp - 1].as_real > vm.stack[vm.sp].as_real;
        --vm.sp;
        ++vm.ip;
        break;
    }
    case RLT:
    {
        vm.stack[vm.sp - 1].as_real = vm.stack[vm.sp - 1].as_real < vm.stack[vm.sp].as_real;
        --vm.sp;
        ++vm.ip;
        break;
    }
    case RGE:
    {
        vm.stack[vm.sp - 1].as_real = vm.stack[vm.sp - 1].as_real >= vm.stack[vm.sp].as_real;
        --vm.sp;
        ++vm.ip;
        break;
    }
    case RLE:
    {
        vm.stack[vm.sp - 1].as_real = vm.stack[vm.sp - 1].as_real <= vm.stack[vm.sp].as_real;
        --vm.sp;
        ++vm.ip;
        break;
    }
    case REQ:
    {
        vm.stack[vm.sp - 1].as_real = vm.stack[vm.sp - 1].as_real == vm.stack[vm.sp].as_real;
        --vm.sp;
        ++vm.ip;
        break;
    }
    case RNQ:
    {
        vm.stack[vm.sp - 1].as_real = vm.stack[vm.sp - 1].as_real != vm.stack[vm.sp].as_real;
        --vm.sp;
        ++vm.ip;
        break;
    }
    case RCONST:
    {
        vm_check_stack(1);
        vm.stack[++vm.sp].as_uint64 = *((uint64_t*) (opcode + 1));
        vm.ip += 9;
        break;
    }
    case RCONST_0:
    {
        vm_check_stack(1);
        vm.stack[++vm.sp].as_real = 0.0;
        ++vm.ip;
        break;
    }
    case RCONST_1:
    {
        vm_check_stack(1);
        vm.stack[++vm.sp].as_real = 1.0;
        ++vm.ip;
        break;
    }
    case RCONST_PI:
    {
        vm_check_stack(1);
        vm.stack[++vm.sp].as_real = 3.14159265358979323846;
        ++vm.ip;
        break;
    }
    case RPRINT:
    {
        printf("%f", vm.stack[vm.sp].as_real);
        fflush(stdout);
        --vm.sp;
        ++vm.ip;
        break;
    }
    case RTOI:
    {
        vm.stack[vm.sp].as_int64 = (int64_t) vm.stack[vm.sp].as_real;
        ++vm.ip;
        break;
    }
    case SLOAD:
    {
        vm_check_stack(1);
        vm.stack[vm.sp + 1].as_uint16 = vm.stack[vm.bp + *((uint16_t*) (opcode + 1))].as_uint16;
        ++vm.sp;
        vm.ip += 3;
        break;
    }
    case SSTORE:
    {
        vm.stack[vm.bp + *((uint16_t*) (opcode + 1))].as_uint16 = vm.stack[vm.sp].as_uint16;
        --vm.sp;
        vm.ip += 3;
        break;
    }
    case SCONST:
    {
        vm_check_stack(1);
        vm.stack[++vm.sp].as_int16 = *((uint16_t*) (opcode + 1));
        vm.ip += 3;
        break;
    }
    case SPRINT:
    {
        printf("%s", &vm.data.data[vm.stack[vm.sp].as_uint16]);
        fflush(stdout);
        --vm.sp;
        ++vm.ip;
        break;
    }
    case SLEN:
    {
        vm_check_stack(1);
        uint16_t str_addr = vm.stack[vm.sp].as_uint16;
        const char* str = (const char*)&vm.data.data[str_addr];
        vm.stack[++vm.sp].as_int64 = (int64_t)utf8len(str);
        ++vm.ip;
        break;
    }
    case NPRINT:
    {
        printf("\n");
        fflush(stdout);
        ++vm.ip;
        break;
    }
    default:
        printf("BAD OPCODE [%d : %d]\n", *opcode, vm.ip);
        exit(0);
        break;
    }
}

void vm_exec()
{
    while (!vm.flags.halt)
    {
        exec_opcode(vm.code.data + vm.ip);
    }
}

void vm_dump()
{
    printf("-- begin --\n");
    printf("ip: %du  sp: %du bp: %du", vm.ip, vm.sp, vm.bp);
    printf("[ ");
    for (int i = vm.sp; i >= 0; i--)
        printf("%lu ", vm.stack[i].as_uint64);
    printf("]\n");
    printf("-- end   --\n");
}

size_t vm_dasm_opcode(FILE *file, size_t ip)
{
    opcode_t opcode = OPCODES[vm.code.data[ip]];

    fprintf(file, "%lx\t %s", ip, opcode.name);

    for (int a = 0; a < opcode.arg_size; a++)
        fprintf(file, " 0x%x", (vm.code.data[ip + a + 1] & 0xFF));
    fprintf(file, "\n");

    return opcode.arg_size;
}

void vm_dasm(const char* filename)
{
    FILE* file = fopen(filename, "w");
    if (file == NULL)
    {
        fprintf(stderr, "Error: Cannot open file '%s' for writing\n", filename);
        return;
    }

    // FILE* file = stdout;

    for (size_t ip = 0; ip < vm.code.used; ip++)
    {
        ip += vm_dasm_opcode(file, ip);
    }

    fclose(file);
}

void vm_code_emit(uint8_t* bytes, size_t len)
{
    buffer_adds(&vm.code, bytes, len);
}

void vm_code_set(size_t index, uint8_t* bytes, size_t len)
{
    buffer_sets(&vm.code, index, bytes, len);
}

size_t vm_code_addr()
{
    return buffer_size(&vm.code);
}

void vm_data_emit(uint8_t* bytes, size_t len)
{
    buffer_adds(&vm.data, bytes, len);
}

size_t vm_data_used()
{
    return buffer_size(&vm.data);
}

uint8_t* vm_data_ptr()
{
    return vm.data.data;
}

void vm_save(char* name)
{
    // this must have data ...
    FILE* file = fopen(name, "w");
    fwrite(vm.code.data, vm.code.used, 1, file);
    fclose(file);
}

void vm_load(char* name)
{
}
