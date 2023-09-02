#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* sample program:
 *
 * load    r1  addr    # Load value at given address into given register
 * store   r2  addr    # Store the value in register at the given memory address
 * add     r1  r2      # Set r1 = r1 + r2
 * sub     r1  r2      # Set r1 = r1 - r2
 * halt
 * */

#define NIL     0x00
#define LOAD    0x01
#define STORE   0x02
#define ADD     0x03
#define SUB     0x04
#define HALT    0xff

#define PC 0x00
#define R1 0x01
#define R2 0x02

struct Vm {
    /* [PC, R1, R2] */
    uint8_t regs[3];
};

struct Vm vm_init() {
    struct Vm vm;
    /* instructions start at index 8 of the main memory */
    vm.regs[PC] = 8;
    vm.regs[R1] = 0;
    vm.regs[R2] = 0;
    return vm;
}

struct RawInstr {
    uint8_t kind;
    uint8_t args[2];
};

struct RawInstr raw_instr_nil() {
    struct RawInstr raw_instr;
    raw_instr.kind = NIL;
    return raw_instr;
}

struct LoadInstr {
    uint8_t reg;
    uint8_t addr;
};

struct StoreInstr {
    uint8_t reg;
    uint8_t addr;
};

struct AddInstr {
    uint8_t a; // store sum in 'a'
    uint8_t b;
};

struct SubInstr {
    uint8_t a; // store sub in 'a'
    uint8_t b;
};

/* typedef struct _HaltInstr HaltInstr; */

struct Instr {
    uint8_t kind;
    void *inner;
};

void instr_free(struct Instr instr) {
    if (instr.kind != HALT) {
        free(instr.inner);
    }
}

/* raw_instr: out */
void fetch(struct Vm vm, uint8_t mem[256], struct RawInstr *raw_instr) {
    uint16_t instr_idx = vm.regs[PC];
    if (instr_idx >= 256) {
        printf("error: tried to fetch address bigger than main memory\n");
        exit(1);
    }
    raw_instr->kind = mem[instr_idx];
    switch (raw_instr->kind) {
        case LOAD:
        case STORE:
        case ADD:
        case SUB:
            // TODO: overflow check
            raw_instr->args[0] = mem[instr_idx + 1];
            raw_instr->args[1] = mem[instr_idx + 2];
            break;
    }
}

/* instr: out */
void decode(struct RawInstr raw_instr, struct Instr *instr) {
    struct LoadInstr *load;
    struct StoreInstr *store;
    struct AddInstr *add;
    struct SubInstr *sub;

    instr->kind = raw_instr.kind;

    switch (instr->kind) {
        case LOAD:
            load = malloc(sizeof(struct LoadInstr));
            load->reg = raw_instr.args[0];
            load->addr = raw_instr.args[1];
            instr->inner = load;
            break;
        case STORE:
            store = malloc(sizeof(struct StoreInstr));
            store->reg = raw_instr.args[0];
            store->addr = raw_instr.args[1];
            instr->inner = store;
            break;
        case ADD:
            add = malloc(sizeof(struct AddInstr));
            add->a = raw_instr.args[0];
            add->b = raw_instr.args[1];
            instr->inner = add;
            break;
        case SUB:
            sub = malloc(sizeof(struct SubInstr));
            sub->a = raw_instr.args[0];
            sub->b = raw_instr.args[1];
            instr->inner = sub;
            break;
        case HALT:
            break;
        default:
            printf("error: failed to decode unknown instruction %d\n", instr->kind);
            exit(1);
    }
}

void execute(struct Vm *vm, struct Instr instr, uint8_t mem[256]) {
    uint8_t instr_count;
    struct LoadInstr *load;
    struct StoreInstr *store;
    struct AddInstr *add;
    struct SubInstr *sub;

    switch (instr.kind) {
        case LOAD:
            load = (struct LoadInstr *) instr.inner;
            vm->regs[load->reg] = mem[load->addr];
            instr_count = 3;
            break;
        case STORE:
            store = (struct StoreInstr *) instr.inner;
            mem[store->addr] = vm->regs[store->reg];
            instr_count = 3;
            break;
        case ADD:
            add = (struct AddInstr *) instr.inner;
            vm->regs[add->a] += vm->regs[add->b];
            instr_count = 3;
            break;
        case SUB:
            sub = (struct SubInstr *) instr.inner;
            vm->regs[sub->a] -= vm->regs[sub->b];
            instr_count = 3;
            break;
        case HALT:
            /* no op, main loop exits */
            instr_count = 1;
            break;
        default:
            printf("error: unknown instruction %d\n", instr.kind);
            exit(1);
    }
    vm->regs[PC] += instr_count;
}

void mem_print(uint8_t mem[256]) {
    printf("data segment:\n");
    for (int i = 0; i < 8; i++) {
       printf("mem[%d]: %d\n", i, mem[i]);
    }
}

int main(void) {
    /* main memory */
    uint8_t mem[256] = {
        /* data (8 bytes) */
        0x00, 0x03, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* instructions */
        0x01, 0x01, 0x01, /* 01 = LOAD, 01 = r1, 01 = addr in data section */
        0x01, 0x02, 0x02, /* 01 = LOAD, 02 = r2, 02 = addr in data section */
        0x03, 0x01, 0x02, /* 03 = ADD, 01 = r1, 02 = r2 */
        0x02, 0x01, 0x00, /* 02 = STORE, 01 = r1, 00 = addr in data section */
        0xff              /* ff = HALT */
    };
    struct Vm vm = vm_init();
    struct RawInstr raw_instr = raw_instr_nil();
    struct Instr instr;

    while (instr.kind != HALT) {
        fetch(vm, mem, &raw_instr);
        decode(raw_instr, &instr);
        execute(&vm, instr, mem);
        instr_free(instr);
    }

    mem_print(mem);

    return 0;
}
