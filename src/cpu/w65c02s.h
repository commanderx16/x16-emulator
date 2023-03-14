/*******************************************************************************
            w65c02s.h -- cycle-accurate C emulator of the WDC 65C02S
                         as a single-header library
            by ziplantil 2022-2023 -- under the CC0 license
            version: 2023-03-14
            please report issues to <https://github.com/ziplantil/w65c02s.h>
*******************************************************************************/

#ifdef __cplusplus
extern "C" {
#undef W65C02S_HAS_BOOL
#define W65C02S_HAS_BOOL 1
#endif

#ifndef W65C02S_H
#define W65C02S_H

/* defines */

/* 1: always runs instructions as a whole */
/* 0: accurate cycle counter, can stop and resume mid-instruction. */
#ifndef W65C02S_COARSE
#define W65C02S_COARSE 0
#endif

/* 1: define uint8_t w65c02s_read(uint16_t); 
         and    void w65c02s_write(uint16_t, uint8_t); through linking. */
/* 0: define them through function pointers passed to w65c02s_init. */
#ifndef W65C02S_LINK
#define W65C02S_LINK 0
#endif

/* 1: allow hook_brk */
/* 0: do not allow hook_brk */
#ifndef W65C02S_HOOK_BRK
#define W65C02S_HOOK_BRK 0
#endif

/* 1: allow hook_stp */
/* 0: do not allow hook_stp */
#ifndef W65C02S_HOOK_STP
#define W65C02S_HOOK_STP 0
#endif

/* 1: allow hook_end_of_instruction */
/* 0: do not allow hook_end_of_instruction */
#ifndef W65C02S_HOOK_EOI
#define W65C02S_HOOK_EOI 0
#endif

/* 1: has bool without stdbool.h */
/* 0: does not have bool without stdbool.h */
#ifndef W65C02S_HAS_BOOL 
#define W65C02S_HAS_BOOL 0
#endif

/* 1: has uint8_t, uint16_t without stdint.h */
/* 0: does not have uint8_t, uint16_t without stdint.h */
#ifndef W65C02S_HAS_UINTN_T
#define W65C02S_HAS_UINTN_T 0
#endif

#if __STDC_VERSION__ >= 199901L
#define W65C02S_C99 1
#if __STDC_VERSION__ >= 201112L
#define W65C02S_C11 1
#if __STDC_VERSION__ >= 202309L
#define W65C02S_C23 1
#endif
#endif
#endif

#ifdef _MSC_VER
#define W65C02S_MSVC 1
#elif __GNUC__ >= 5
#define W65C02S_GNUC 1
#endif

#if W65C02S_HAS_UINTN_T
/* do nothing, we have these types */
#elif W65C02S_C99 || HAS_STDINT_H || HAS_STDINT
#include <stdint.h>
#else
#include <limits.h>
#if UCHAR_MAX == 255
typedef unsigned char uint8_t;
#else
#error no uint8_t available
#endif
#if USHRT_MAX == 65535
typedef unsigned short uint16_t;
#else
#error no uint16_t available
#endif
#endif

#if W65C02S_HAS_BOOL
/* has bool */
#elif W65C02S_C99 || HAS_STDBOOL_H
#include <stdbool.h>
#else
typedef int bool;
#define true 1
#define false 0
#endif

#if W65C02S_C11
#define W65C02S_ALIGNAS(n) _Alignas(n)
#else
#define W65C02S_ALIGNAS(n)
#endif

struct w65c02s_cpu;

/** w65c02s_init
 *
 *  Initializes a new CPU instance.
 *
 *  mem_read and mem_write only need to be specified if W65C02S_LINK is set
 *  to 0. If set to 1, they may simply be passed as NULL.
 *
 *  [Parameter: cpu] The CPU instance to run
 *  [Parameter: mem_read] The read callback, ignored if W65C02S_LINK is 1.
 *  [Parameter: mem_write] The write callback, ignored if W65C02S_LINK is 1.
 *  [Parameter: cpu_data] The pointer to be returned by w65c02s_get_cpu_data().
 *  [Return value] The number of cycles that were actually run
 */
void w65c02s_init(struct w65c02s_cpu *cpu,
                  uint8_t (*mem_read)(struct w65c02s_cpu *, uint16_t),
                  void (*mem_write)(struct w65c02s_cpu *, uint16_t, uint8_t),
                  void *cpu_data);

#if W65C02S_LINK
/** w65c02s_read
 *
 *  Reads a value from memory.
 *
 *  This method only exists if the library is compiled with W65C02S_LINK.
 *  In that case, you must provide an implementation.
 *
 *  [Parameter: address] The 16-bit address to read from
 *  [Return value] The value read from memory
 */
extern uint8_t w65c02s_read(uint16_t address);

/** w65c02s_write
 *
 *  Writes a value to memory.
 *
 *  This method only exists if the library is compiled with W65C02S_LINK.
 *  In that case, you must provide an implementation.
 *
 *  [Parameter: address] The 16-bit address to write to
 *  [Parameter: value] The value to write
 */
extern void w65c02s_write(uint16_t address, uint8_t value);
#endif

/** w65c02s_run_cycles
 *
 *  Runs the CPU for the given number of cycles.
 *
 *  If w65c02ce is compiled with W65C02S_COARSE, the actual number of
 *  cycles executed may not always match the given argument, but the return
 *  value is always correct. Without W65C02S_COARSE the return value is
 *  always the same as `cycles`.
 *
 *  This function is not reentrant. Calling it from a callback (for a memory
 *  read, write, STP, etc.) will result in undefined behavior.
 *
 *  [Parameter: cpu] The CPU instance to run
 *  [Parameter: cycles] The number of cycles to run
 *  [Return value] The number of cycles that were actually run
 */
unsigned long w65c02s_run_cycles(struct w65c02s_cpu *cpu, unsigned long cycles);

/** w65c02s_step_instruction
 *
 *  Runs the CPU for one instruction, or if an instruction is already running,
 *  finishes that instruction.
 *
 *  Prefer using w65c02s_run_instructions or w65c02s_run_cycles instead if you
 *  can to run many instructions at once.
 *
 *  This function is not reentrant. Calling it from a callback (for a memory
 *  read, write, STP, etc.) will result in undefined behavior.
 *
 *  [Parameter: cpu] The CPU instance to run
 *  [Return value] The number of cycles that were actually run
 */
unsigned long w65c02s_step_instruction(struct w65c02s_cpu *cpu);

/** w65c02s_run_instructions
 *
 *  Runs the CPU for the given number of instructions.
 *
 *  finish_existing only matters if the CPU is currently mid-instruction.
 *  If it is, specifying finish_existing as true will finish the current
 *  instruction before running any new ones, and the existing instruction
 *  will not count towards the instruction count limit. If finish_existing is
 *  false, the current instruction will be counted towards the limit.
 *
 *  Entering an interrupt counts as an instruction here.
 *
 *  This function is not reentrant. Calling it from a callback (for a memory
 *  read, write, STP, etc.) will result in undefined behavior.
 *
 *  [Parameter: cpu] The CPU instance to run
 *  [Parameter: instructions] The number of instructions to run
 *  [Parameter: finish_existing] Whether to finish the current instruction
 *  [Return value] The number of cycles that were actually run
 *                 (mind the overflow with large values of instructions!)
 */
unsigned long w65c02s_run_instructions(struct w65c02s_cpu *cpu,
                                       unsigned long instructions,
                                       bool finish_existing);

/** w65c02s_get_cycle_count
 *
 *  Gets the total number of cycles executed by this CPU.
 *
 *  If it hasn't, the value returned by this function reflects how many
 *  cycles have been run. For example, on the first reset cycle (spurious
 *  read of the PC) of a new CPU instance, this will return 0.
 *
 *  [Parameter: cpu] The CPU instance to run
 *  [Return value] The number of cycles run in total
 */
unsigned long w65c02s_get_cycle_count(const struct w65c02s_cpu *cpu);

/** w65c02s_get_instruction_count
 *
 *  Gets the total number of instructions executed by this CPU.
 *
 *  Entering an interrupt counts does not count as an instruction here.
 *
 *  [Parameter: cpu] The CPU instance to run
 *  [Return value] The number of cycles run in total
 */
unsigned long w65c02s_get_instruction_count(const struct w65c02s_cpu *cpu);

/** w65c02s_get_cpu_data
 *
 *  Gets the cpu_data pointer associated to the CPU with w65c02s_init.
 *
 *  [Parameter: cpu] The CPU instance to get the pointer from
 *  [Return value] The cpu_data pointer
 */
void *w65c02s_get_cpu_data(const struct w65c02s_cpu *cpu);

/** w65c02s_reset_cycle_count
 *
 *  Resets the total cycle counter (w65c02s_get_cycle_count).
 *
 *  [Parameter: cpu] The CPU instance
 */
void w65c02s_reset_cycle_count(struct w65c02s_cpu *cpu);

/** w65c02s_reset_instruction_count
 *
 *  Resets the total instruction counter (w65c02s_get_instruction_count).
 *
 *  [Parameter: cpu] The CPU instance
 */
void w65c02s_reset_instruction_count(struct w65c02s_cpu *cpu);

/** w65c02s_is_cpu_waiting
 *
 *  Checks whether the CPU is currently waiting for an interrupt (WAI).
 *
 *  [Parameter: cpu] The CPU instance
 *  [Return value] Whether the CPU ran a WAI instruction and is waiting
 */
bool w65c02s_is_cpu_waiting(const struct w65c02s_cpu *cpu);

/** w65c02s_is_cpu_stopped
 *
 *  Checks whether the CPU is currently stopped and waiting for a reset.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Return value] Whether the CPU ran a STP instruction
 */
bool w65c02s_is_cpu_stopped(const struct w65c02s_cpu *cpu);

/** w65c02s_break
 *
 *  Triggers a CPU break.
 *
 *  If w65c02s_run_cycles or w65c02s_run_instructions is running, it will
 *  return as soon as possible. In coarse mode, this will finish the current
 *  instruction.
 *
 *  In non-coarse mode, it will finish the current cycle. Note that this means
 *  that if `w65c02s_get_cycle_count` returns 999 (i.e. 999 cycles have been
 *  run), the cycle count will tick over to 1000 before the run call returns.
 *  This is because the thousandth cycle will still get finished.
 *
 *  If the CPU is not running, the function does nothing.
 *
 *  This function is designed to be called from callbacks (e.g. memory access
 *  or end of instruction).
 *
 *  [Parameter: cpu] The CPU instance
 */
void w65c02s_break(struct w65c02s_cpu *cpu);

/** w65c02s_stall
 *
 *  Stalls the CPU stall for the given number of cycles. This feature is
 *  intended for use with e.g. memory accesses that require additional cycles
 *  (which, on a real chip, would generally work by pulling RDY low).
 *
 *  The stall cycles will run at the beginning of the next w65c02s_run_cycles
 *  or w65c02s_run_instructions. w65c02s_stall will automatically call
 *  w65c02s_break to break execution if any is running.
 *
 *  This function is designed to be called from callbacks (e.g. memory access
 *  or end of instruction).
 *
 *  [Parameter: cpu] The CPU instance
 *  [Parameter: cycles] The number of cycles to stall
 */
void w65c02s_stall(struct w65c02s_cpu *cpu, unsigned long cycles);

/** w65c02s_nmi
 *
 *  Queues a NMI (non-maskable interrupt) on the CPU.
 *
 *  The NMI will generally execute before the next instruction, but it must
 *  be triggered before the final cycle of an instruction, or it will be
 *  postponed to after the next instruction. NMIs cannot be masked.
 *  Calling this function will trigger an NMI only once.
 *
 *  [Parameter: cpu] The CPU instance
 */
void w65c02s_nmi(struct w65c02s_cpu *cpu);

/** w65c02s_reset
 *
 *  Triggers a CPU reset.
 *
 *  The RESET will begin executing before the next instruction. The RESET
 *  does not behave exactly like in the original chip, and will not carry out
 *  any extra cycles in between the end of the instruction and beginning of
 *  the reset (like some chips do). There is also no requirement that RESET be
 *  asserted before the last cycle, like with NMI or IRQ.
 *
 *  [Parameter: cpu] The CPU instance
 */
void w65c02s_reset(struct w65c02s_cpu *cpu);

/** w65c02s_irq
 *
 *  Pulls the IRQ line high (logically) on the CPU.
 *
 *  If interrupts are enabled on the CPU, the IRQ handler will execute before
 *  the next instruction. Like with NMI, an IRQ on the last cycle will be
 *  delayed until after the next instruction. The IRQ line is held until
 *  w65c02s_irq_cancel is called.
 *
 *  [Parameter: cpu] The CPU instance
 */
void w65c02s_irq(struct w65c02s_cpu *cpu);

/** w65c02s_irq_cancel
 *
 *  Pulls the IRQ line low (logically) on the CPU.
 *
 *  The IRQ is cancelled and will no longer be triggered. Triggering an IRQ
 *  only once is tricky. w65c02s_irq and w65c02s_irq_cancel immediately will
 *  not cause an IRQ (since the CPU never samples it). It must be held for
 *  at least 7-8 cycles, but that may still not be enough if interrupts are
 *  disabled from the CPU side.
 *
 *  Generally you would want to hold the IRQ line high and cancel the IRQ
 *  only once it has been acknowledged by the CPU (e.g. through MMIO). 
 *
 *  [Parameter: cpu] The CPU instance
 */
void w65c02s_irq_cancel(struct w65c02s_cpu *cpu);

/** w65c02s_set_overflow
 *
 *  Sets the overflow (V) flag on the status register (P) of the CPU.
 *
 *  This corresponds to the S/O pin on the physical CPU.
 *
 *  [Parameter: cpu] The CPU instance
 */
void w65c02s_set_overflow(struct w65c02s_cpu *cpu);

/** w65c02s_hook_brk
 *
 *  Hooks the BRK instruction on the CPU.
 *
 *  The hook function should take a single uint8_t parameter, which corresponds
 *  to the immediate parameter after the BRK opcode. If the hook function
 *  returns a non-zero value, the BRK instruction is skipped, and otherwise
 *  it is treated as normal.
 *
 *  Passing NULL as the hook disables the hook.
 *
 *  This function does nothing if the library was not compiled with
 *  W65C02S_HOOK_BRK.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Parameter: brk_hook] The new BRK hook
 *  [Return value] Whether the hook was set (false only if the library
 *                 was compiled without W65C02S_HOOK_BRK)
 */
bool w65c02s_hook_brk(struct w65c02s_cpu *cpu, bool (*brk_hook)(uint8_t));

/** w65c02s_hook_stp
 *
 *  Hooks the STP instruction on the CPU.
 *
 *  The hook function should take no parameters. If it returns a non-zero
 *  value, the STP instruction is skipped, and otherwise it is treated
 *  as normal.
 *
 *  Passing NULL as the hook disables the hook.
 *
 *  This function does nothing if the library was not compiled with
 *  W65C02S_HOOK_STP.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Parameter: stp_hook] The new STP hook
 *  [Return value] Whether the hook was set (0 only if the library
 *                 was compiled without W65C02S_HOOK_STP)
 */
bool w65c02s_hook_stp(struct w65c02s_cpu *cpu, bool (*stp_hook)(void));

/** w65c02s_hook_end_of_instruction
 *
 *  Hooks the end-of-instruction on the CPU.
 *
 *  The hook function should take no parameters. It is called when an
 *  instruction finishes. The interrupt entering routine counts
 *  as an instruction here.
 *
 *  Passing NULL as the hook disables the hook.
 *
 *  This function does nothing if the library was not compiled with
 *  W65C02S_HOOK_EOI.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Parameter: instruction_hook] The new end-of-instruction hook
 *  [Return value] Whether the hook was set (0 only if the library
 *                 was compiled without W65C02S_HOOK_EOI)
 */
bool w65c02s_hook_end_of_instruction(struct w65c02s_cpu *cpu,
                                     void (*instruction_hook)(void));

/** w65c02s_reg_get_a
 *
 *  Returns the value of the A register on the CPU.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Return value] The value of the A register
 */
uint8_t w65c02s_reg_get_a(const struct w65c02s_cpu *cpu);

/** w65c02s_reg_get_x
 *
 *  Returns the value of the X register on the CPU.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Return value] The value of the X register
 */
uint8_t w65c02s_reg_get_x(const struct w65c02s_cpu *cpu);

/** w65c02s_reg_get_y
 *
 *  Returns the value of the Y register on the CPU.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Return value] The value of the Y register
 */
uint8_t w65c02s_reg_get_y(const struct w65c02s_cpu *cpu);

/** w65c02s_reg_get_p
 *
 *  Returns the value of the P (processor status) register on the CPU.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Return value] The value of the P register
 */
uint8_t w65c02s_reg_get_p(const struct w65c02s_cpu *cpu);

/** w65c02s_reg_get_s
 *
 *  Returns the value of the S (stack pointer) register on the CPU.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Return value] The value of the S register
 */
uint8_t w65c02s_reg_get_s(const struct w65c02s_cpu *cpu);

/** w65c02s_reg_get_pc
 *
 *  Returns the value of the PC (program counter) register on the CPU.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Return value] The value of the PC register
 */
uint16_t w65c02s_reg_get_pc(const struct w65c02s_cpu *cpu);

/** w65c02s_reg_set_a
 *
 *  Replaces the value of the A register on the CPU.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Parameter: v] The new value of the A register
 */
void w65c02s_reg_set_a(struct w65c02s_cpu *cpu, uint8_t v);

/** w65c02s_reg_set_x
 *
 *  Replaces the value of the X register on the CPU.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Parameter: v] The new value of the X register
 */
void w65c02s_reg_set_x(struct w65c02s_cpu *cpu, uint8_t v);

/** w65c02s_reg_set_y
 *
 *  Replaces the value of the Y register on the CPU.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Parameter: v] The new value of the Y register
 */
void w65c02s_reg_set_y(struct w65c02s_cpu *cpu, uint8_t v);

/** w65c02s_reg_set_p
 *
 *  Replaces the value of the P (processor status) register on the CPU.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Parameter: v] The new value of the P register
 */
void w65c02s_reg_set_p(struct w65c02s_cpu *cpu, uint8_t v);

/** w65c02s_reg_set_s
 *
 *  Replaces the value of the S (stack pointer) register on the CPU.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Parameter: v] The new value of the S register
 */
void w65c02s_reg_set_s(struct w65c02s_cpu *cpu, uint8_t v);

/** w65c02s_reg_set_pc
 *
 *  Replaces the value of the PC (program counter) register on the CPU.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Parameter: v] The new value of the PC register
 */
void w65c02s_reg_set_pc(struct w65c02s_cpu *cpu, uint16_t v);

#endif /* W65C02S_H */

#if W65C02S_IMPL

#include <stddef.h>
#include <limits.h>



/* +------------------------------------------------------------------------+ */
/* |                                                                        | */
/* |                            INTERNAL STRUCTS                            | */
/* |                                                                        | */
/* +------------------------------------------------------------------------+ */



/* temporary values for various addressing modes */

struct w65c02s_temp_ea {
    uint16_t ea; /* effective address */
};

struct w65c02s_temp_ea8 {
    uint8_t ea; /* effective address */
};

struct w65c02s_temp_ea_page {
    uint16_t ea; /* effective address */
    uint16_t ea_wrong; /* wrong effective address (wrong high byte) */
    bool page_penalty; /* page wrap penalty */
};

struct w65c02s_temp_ea_zp {
    uint16_t ea; /* effective address */
    uint8_t zp; /* zero page address */
    bool page_penalty; /* page wrap penalty */
};

struct w65c02s_temp_jump {
    uint16_t ea; /* effective address */
    uint16_t ta; /* temporary address */
};

struct w65c02s_temp_branch {
    uint16_t new_pc; /* new program counter */
    uint16_t old_pc; /* old program counter */
    bool penalty; /* page crossing penalty */
};

struct w65c02s_temp_branch_bit {
    uint16_t new_pc; /* new program counter */
    uint16_t old_pc; /* old program counter */
    uint8_t data; /* data read */
    bool penalty; /* page crossing penalty */
};

struct w65c02s_temp_rmw {
    uint16_t ea; /* effective address */
    uint8_t data; /* data read/to Write */
};

struct w65c02s_temp_rmw8 {
    uint8_t ea; /* effective address */
    uint8_t data; /* data read/to Write */
};

struct w65c02s_temp_brk {
    uint16_t ea; /* effective address */
    bool is_brk; /* is this BRK? */
};

struct w65c02s_temp_wai {
    bool is_stp; /* is this STP? */
};



/* +------------------------------------------------------------------------+ */
/* |                                                                        | */
/* |                                 STRUCT                                 | */
/* |                                                                        | */
/* +------------------------------------------------------------------------+ */



/* please treat w65c02s_cpu as an opaque type */
struct w65c02s_cpu {
    unsigned long total_cycles;
#if !W65C02S_COARSE
    unsigned long target_cycles;
#endif
    /* run/wait/stop, interrupt trigger flags (see W65C02S_CPU_STATE_)... */
    unsigned cpu_state;
    /* currently active interrupts, interrupt mask */
    unsigned int_trig, int_mask;

    /* 6502 registers: PC, A, X, Y, S (stack pointer), P (flags). */
    W65C02S_ALIGNAS(2) uint16_t pc;
    uint8_t a, x, y, s, p, p_adj;
    /* p_adj for decimal mode; it contains the "correct" flags. */

#if !W65C02S_COARSE
    union {
        struct w65c02s_temp_ea ea;
        struct w65c02s_temp_ea8 ea8;
        struct w65c02s_temp_ea_page ea_page;
        struct w65c02s_temp_ea_zp ea_zp;
        struct w65c02s_temp_jump jump;
        struct w65c02s_temp_branch branch;
        struct w65c02s_temp_branch_bit branch_bit;
        struct w65c02s_temp_rmw rmw;
        struct w65c02s_temp_rmw8 rmw8;
        struct w65c02s_temp_brk brk;
        struct w65c02s_temp_wai wai;
    } temp;
#endif

#if !W65C02S_COARSE
    /* current instruction */
    uint8_t ir;
    /* cycle of current instruction */
    unsigned int cycl;
#endif

    unsigned long total_instructions;

    /* entering NMI, resetting or IRQ? */
    bool in_nmi, in_rst, in_irq;

#if !W65C02S_COARSE
    /* how many cycles we are limited to. changed when w65c02s_break called. */
    unsigned long maximum_cycles;
#endif

#if !W65C02S_LINK
    /* memory read/write callbacks */
    uint8_t (*mem_read)(struct w65c02s_cpu *, uint16_t);
    void (*mem_write)(struct w65c02s_cpu *, uint16_t, uint8_t);
#endif

    /* BRK, STP, end of instruction hooks */
    bool (*hook_brk)(uint8_t);
    bool (*hook_stp)(void);
    void (*hook_eoi)(void);

    /* how many cycles we must still stall */
    unsigned long stall_cycles;

    /* data pointer from w65c02s_init */
    void *cpu_data;
};



/* +------------------------------------------------------------------------+ */
/* |                                                                        | */
/* |                         MACROS AND ENUMERATIONS                        | */
/* |                                                                        | */
/* +------------------------------------------------------------------------+ */



/* W65C02S_INLINE: inline function/method if at all possible */
#if W65C02S_C99
#if W65C02S_GNUC
#define W65C02S_INLINE __attribute__((always_inline)) static inline
#elif W65C02S_MSVC
#define W65C02S_INLINE __forceinline static inline
#else
#define W65C02S_INLINE static inline
#endif
#else
#define W65C02S_INLINE static
#endif

#if W65C02S_C99
#define W65C02S_LOOKUP_TYPE uint_fast8_t
#else
#define W65C02S_LOOKUP_TYPE unsigned
#endif

/* W65C02S_UNREACHABLE: means that the code could never be reached.
                        used in w65c02s.h to optimize switches */
/* W65C02S_ASSUME: asserts that (x) is always true. for optimization */
#if W65C02S_C23
#define W65C02S_UNREACHABLE() unreachable()
#define W65C02S_ASSUME(x) do { if (!(x)) unreachable(); } while (0)
#elif W65C02S_GNUC
#define W65C02S_UNREACHABLE() __builtin_unreachable()
#define W65C02S_ASSUME(x) do { if (!(x)) __builtin_unreachable(); } while (0)
#elif W65C02S_MSVC
#define W65C02S_UNREACHABLE() __assume(0)
#define W65C02S_ASSUME(x) __assume(x)
#else
#define W65C02S_UNREACHABLE()
#define W65C02S_ASSUME(x)
#endif

/* W65C02S_LIKELY, W65C02S_UNLIKELY: explicit branch prediction */
#if W65C02S_GNUC
#define W65C02S_LIKELY(x) __builtin_expect((x), 1)
#define W65C02S_UNLIKELY(x) __builtin_expect((x), 0)
#else
#define W65C02S_LIKELY(x) (x)
#define W65C02S_UNLIKELY(x) (x)
#endif

/* cpu_state flags */
#define W65C02S_CPU_STATE_RUN 0
#define W65C02S_CPU_STATE_WAIT 1
#define W65C02S_CPU_STATE_STOP 2
#define W65C02S_CPU_STATE_IRQ 4
#define W65C02S_CPU_STATE_NMI 8
#define W65C02S_CPU_STATE_RESET 16
#define W65C02S_CPU_STATE_BREAK 128

/* cpu_state: low 2 bits are used for RUN/WAIT/STOP */
#define W65C02S_CPU_STATE_EXTRACT(S)                ((S) & 3)
#define W65C02S_CPU_STATE_INSERT(S, s)              ((S) = ((S) & ~3) | (s))
/* cpu_state: low 5 bits are used for RUN/WAIT/STOP and interrupt flags */
#define W65C02S_CPU_STATE_EXTRACT_WITH_INTS(S)      ((S) & 127)

/* used for interrupt and break flags */
#define W65C02S_CPU_STATE_HAS_FLAG(cpu, f)          ((cpu)->cpu_state & (f))
#define W65C02S_CPU_STATE_SET_FLAG(cpu, f)          ((cpu)->cpu_state |= (f))
#define W65C02S_CPU_STATE_RST_FLAG(cpu, f)          ((cpu)->cpu_state &= ~(f))

#define W65C02S_CPU_STATE_HAS_NMI(cpu)                                         \
        W65C02S_CPU_STATE_HAS_FLAG(cpu, W65C02S_CPU_STATE_NMI)
#define W65C02S_CPU_STATE_ASSERT_NMI(cpu)                                      \
        W65C02S_CPU_STATE_SET_FLAG(cpu, W65C02S_CPU_STATE_NMI)
#define W65C02S_CPU_STATE_CLEAR_NMI(cpu)                                       \
        W65C02S_CPU_STATE_RST_FLAG(cpu, W65C02S_CPU_STATE_NMI)

#define W65C02S_CPU_STATE_HAS_RESET(cpu)                                       \
        W65C02S_CPU_STATE_HAS_FLAG(cpu, W65C02S_CPU_STATE_RESET)
#define W65C02S_CPU_STATE_ASSERT_RESET(cpu)                                    \
        W65C02S_CPU_STATE_SET_FLAG(cpu, W65C02S_CPU_STATE_RESET)
#define W65C02S_CPU_STATE_CLEAR_RESET(cpu)                                     \
        W65C02S_CPU_STATE_RST_FLAG(cpu, W65C02S_CPU_STATE_RESET)

#define W65C02S_CPU_STATE_HAS_IRQ(cpu)                                         \
        W65C02S_CPU_STATE_HAS_FLAG(cpu, W65C02S_CPU_STATE_IRQ)
#define W65C02S_CPU_STATE_ASSERT_IRQ(cpu)                                      \
        W65C02S_CPU_STATE_SET_FLAG(cpu, W65C02S_CPU_STATE_IRQ)
#define W65C02S_CPU_STATE_CLEAR_IRQ(cpu)                                       \
        W65C02S_CPU_STATE_RST_FLAG(cpu, W65C02S_CPU_STATE_IRQ)

#define W65C02S_CPU_STATE_HAS_BREAK(cpu)                                       \
        W65C02S_CPU_STATE_HAS_FLAG(cpu, W65C02S_CPU_STATE_BREAK)

/* memory read/write macros */
#if W65C02S_LINK
#define W65C02S_READ(a) w65c02s_read(a)
#define W65C02S_WRITE(a, v) w65c02s_write(a, v)
#else
#define W65C02S_READ(a) (*cpu->mem_read)(cpu, a)
#define W65C02S_WRITE(a, v) (*cpu->mem_write)(cpu, a, v)
#endif

/* used to implement instructions, etc. */
#if W65C02S_COARSE
/* increment the total cycle counter. */
#define W65C02S_CYCLE_TOTAL_INC     ++cpu->total_cycles;
/* end the instruction immediately. before a read/write! */
#define W65C02S_SKIP_REST           return cyc
/* skip to the next cycle n, which should always be the next cycle.
   must happen before a read/write! */
#define W65C02S_SKIP_TO_NEXT(n)     goto cycle_##n;
#define W65C02S_BEGIN_INSTRUCTION   unsigned cyc = 1; (void)oper;
#define W65C02S_CYCLE_END           ++cyc; W65C02S_CYCLE_TOTAL_INC
#define W65C02S_CYCLE_LABEL_1(n)        
#define W65C02S_CYCLE_LABEL_X(n)    goto cycle_##n; cycle_##n:
#define W65C02S_END_INSTRUCTION     W65C02S_CYCLE_END                          \
                                W65C02S_SKIP_REST;
/* skip the rest of the instruction *after* a read/write. */
#define W65C02S_SKIP_REST_AFTER     do {                                       \
                                        W65C02S_CYCLE_END;                     \
                                        return cyc;                            \
                                    } while (0)
#define W65C02S_USE_TR(type)        struct w65c02s_temp_##type tmp_;
#define W65C02S_TR                  tmp_
#else /* !W65C02S_COARSE */

#define W65C02S_CYCLE_CONDITION     ++cpu->total_cycles == cpu->target_cycles
/* end the instruction immediately. before a read/write! */
#define W65C02S_SKIP_REST           return 0
/* skip to the next cycle n, which should always be the next cycle
   we have to increment cpu->cycl here, otherwise we might continue on the
   wrong cycle. must happen before a read/write! */
#define W65C02S_SKIP_TO_NEXT(n)     do { ++cpu->cycl; goto cycle_##n; }        \
                                                            while (0)
/* skips must happen before a read/write on that cycle, except for this: */
/* the skip that takes effect after a read/write. */
#define W65C02S_SKIP_POST_ACCESS    return 0
#define W65C02S_BEGIN_INSTRUCTION   if (W65C02S_LIKELY(!cont))                 \
                                        goto cycle_1;                          \
                                    (void)oper;                                \
                                    switch (cpu->cycl) {
#define W65C02S_CYCLE_END           if (W65C02S_UNLIKELY(                      \
                                        W65C02S_CYCLE_CONDITION                \
                                    )) return 1;
#define W65C02S_CYCLE_END_LAST      cpu->cycl = 0; W65C02S_CYCLE_END
                              
#define W65C02S_CYCLE_LABEL_1(n)    cycle_##n: case n:
#define W65C02S_CYCLE_LABEL_X(n)    goto cycle_##n; cycle_##n: case n:
#define W65C02S_END_INSTRUCTION     W65C02S_CYCLE_END_LAST                     \
                                    W65C02S_SKIP_REST;                         \
                                }                                              \
                                W65C02S_UNREACHABLE();                         \
                                W65C02S_SKIP_REST;
/* skip the rest of the instruction *after* a read/write. */
#define W65C02S_SKIP_REST_AFTER     do {                                       \
                                        W65C02S_CYCLE_END_LAST;                \
                                        return 0;                              \
                                    } while (0)
#define W65C02S_USE_TR(type)        struct w65c02s_temp_##type * const tmp_ =  \
                                                &cpu->temp.type;
#define W65C02S_TR                  (*tmp_)
#endif

#define W65C02S_CYCLE_1                           W65C02S_CYCLE_LABEL_1(1)
#define W65C02S_CYCLE_2         W65C02S_CYCLE_END W65C02S_CYCLE_LABEL_X(2)
#define W65C02S_CYCLE_3         W65C02S_CYCLE_END W65C02S_CYCLE_LABEL_X(3)
#define W65C02S_CYCLE_4         W65C02S_CYCLE_END W65C02S_CYCLE_LABEL_X(4)
#define W65C02S_CYCLE_5         W65C02S_CYCLE_END W65C02S_CYCLE_LABEL_X(5)
#define W65C02S_CYCLE_6         W65C02S_CYCLE_END W65C02S_CYCLE_LABEL_X(6)
#define W65C02S_CYCLE_7         W65C02S_CYCLE_END W65C02S_CYCLE_LABEL_X(7)

#define W65C02S_CYCLE(n)        W65C02S_CYCLE_##n

/* example instruction:

    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            foo();
        W65C02S_CYCLE(2)
            bar();
    W65C02S_END_INSTRUCTION

becomes something like (with W65C02S_COARSE=0, W65C02S_CYCLE_COUNTER=1)

    switch (cpu->cycl) {
        case 1:
            foo();
            if (!--cpu->left_cycles) return 1;
        case 2:
            bar();
            if (!--cpu->left_cycles) {
                cpu->cycl = 0;
                return 1;
            }
    }
    return 0;

*/

/* get flag of P. returns 1 if flag set, 0 if not */
#define W65C02S_GET_P(p, flag) (!!((p) & (flag)))
/* set flag of P */
#define W65C02S_SET_P(p, flag, v) ((p) = (v) ? ((p) | (flag)) : ((p) & ~(flag)))

/* P flags */
#define W65C02S_P_N 0x80
#define W65C02S_P_V 0x40
#define W65C02S_P_A1 0x20 /* N/C, always 1 */
#define W65C02S_P_B 0x10  /* always 1, but sometimes pushed as 0 */
#define W65C02S_P_D 0x08
#define W65C02S_P_I 0x04
#define W65C02S_P_Z 0x02
#define W65C02S_P_C 0x01

/* stack offset */
#define W65C02S_STACK_OFFSET 0x0100U
#define W65C02S_STACK_ADDR(x) (W65C02S_STACK_OFFSET | ((x) & 0xFF))

/* vectors for NMI, RESET, IRQ */
#define W65C02S_VEC_NMI 0xFFFAU
#define W65C02S_VEC_RST 0xFFFCU
#define W65C02S_VEC_IRQ 0xFFFEU

/* helpers for getting/setting high and low bytes */
#define W65C02S_GET_HI(x) (((x) >> 8) & 0xFF)
#define W65C02S_GET_LO(x) ((x) & 0xFF)
#define W65C02S_SET_HI(x, v) ((x) = ((x) & 0x00FFU) | ((v) << 8))
#define W65C02S_SET_LO(x, v) ((x) = ((x) & 0xFF00U) | (v))

/* returns <>0 if a+b overflows, 0 if not (a, b are uint8_t) */
#define W65C02S_OVERFLOW8(a, b) (((unsigned)(a) + (unsigned)(b)) >> 8)



/* +------------------------------------------------------------------------+ */
/* |                                                                        | */
/* |               ADDRESSING MODE AND OPERATION ENUMERATIONS               | */
/* |                                                                        | */
/* +------------------------------------------------------------------------+ */



/* all possible values for cpu->mode            (example opcode, ! = only) */
#define W65C02S_MODE_IMPLIED                        0       /*  CLD, DEC A */
#define W65C02S_MODE_IMPLIED_X                      1       /*  INX */
#define W65C02S_MODE_IMPLIED_Y                      2       /*  INY */

#define W65C02S_MODE_IMMEDIATE                      3       /*  LDA # */
#define W65C02S_MODE_RELATIVE                       4       /*  BRA # */
#define W65C02S_MODE_RELATIVE_BIT                   5       /*  BBR0 # */
#define W65C02S_MODE_ZEROPAGE                       6       /*  LDA zp */
#define W65C02S_MODE_ZEROPAGE_X                     7       /*  LDA zp,x */
#define W65C02S_MODE_ZEROPAGE_Y                     8       /*  LDA zp,y */
#define W65C02S_MODE_ZEROPAGE_BIT                   9       /*  RMB0 zp */
#define W65C02S_MODE_ABSOLUTE                       10      /*  LDA abs */
#define W65C02S_MODE_ABSOLUTE_X                     11      /*  LDA abs,x */
#define W65C02S_MODE_ABSOLUTE_Y                     12      /*  LDA abs,y */
#define W65C02S_MODE_ZEROPAGE_INDIRECT              13      /*  ORA (zp) */

#define W65C02S_MODE_ZEROPAGE_INDIRECT_X            14      /*  LDA (zp,x) */
#define W65C02S_MODE_ZEROPAGE_INDIRECT_Y            15      /*  LDA (zp),y */
#define W65C02S_MODE_ABSOLUTE_INDIRECT              16      /*! JMP (abs) */
#define W65C02S_MODE_ABSOLUTE_INDIRECT_X            17      /*! JMP (abs,x) */
#define W65C02S_MODE_ABSOLUTE_JUMP                  18      /*! JMP abs */

#define W65C02S_MODE_RMW_ZEROPAGE                   19      /*  LSR zp */
#define W65C02S_MODE_RMW_ZEROPAGE_X                 20      /*  LSR zp,x */
#define W65C02S_MODE_SUBROUTINE                     21      /*! JSR abs */
#define W65C02S_MODE_RETURN_SUB                     22      /*! RTS */
#define W65C02S_MODE_RMW_ABSOLUTE                   23      /*  LSR abs */
#define W65C02S_MODE_RMW_ABSOLUTE_X                 24      /*  LSR abs,x */
#define W65C02S_MODE_NOP_5C                         25      /*! NOP ($5C) */
#define W65C02S_MODE_INT_WAIT_STOP                  26      /*  WAI, STP */

#define W65C02S_MODE_STACK_PUSH                     27      /*  PHA */
#define W65C02S_MODE_STACK_PULL                     28      /*  PLA */
#define W65C02S_MODE_STACK_BRK                      29      /*! BRK # */
#define W65C02S_MODE_STACK_RTI                      30      /*! RTI */

#define W65C02S_MODE_IMPLIED_1C                     31      /*! NOP */

#define W65C02S_MODE_ABSOLUTE_X_STORE               32      /*  STA abs,x */
#define W65C02S_MODE_ABSOLUTE_Y_STORE               33      /*! STA abs,y */
#define W65C02S_MODE_ZEROPAGE_INDIRECT_Y_STORE      34      /*! STA (zp),y */

/* all possible values for oper. note that for
   W65C02S_MODE_ZEROPAGE_BIT and W65C02S_MODE_RELATIVE_BIT,
   the value is always 0-7 (bit) + 8 (S/R) */
/* NOP */
#define W65C02S_OPER_NOP                    0

/* read/store instrs */
/* NOP = 0 */
#define W65C02S_OPER_AND                    1
#define W65C02S_OPER_EOR                    2
#define W65C02S_OPER_ORA                    3
#define W65C02S_OPER_ADC                    4
#define W65C02S_OPER_SBC                    5
#define W65C02S_OPER_CMP                    6
#define W65C02S_OPER_CPX                    7
#define W65C02S_OPER_CPY                    8
#define W65C02S_OPER_BIT                    9
#define W65C02S_OPER_LDA                    10
#define W65C02S_OPER_LDX                    11
#define W65C02S_OPER_LDY                    12
#define W65C02S_OPER_STA                    13
#define W65C02S_OPER_STX                    14
#define W65C02S_OPER_STY                    15
#define W65C02S_OPER_STZ                    16

/* RMW instrs */
/* NOP = 0 */
#define W65C02S_OPER_DEC                    1  /* RMW, A, X, Y */
#define W65C02S_OPER_INC                    2  /* RMW, A, X, Y */
#define W65C02S_OPER_ASL                    3  /* RMW, A */
#define W65C02S_OPER_ROL                    4  /* RMW, A */
#define W65C02S_OPER_LSR                    5  /* RMW, A */
#define W65C02S_OPER_ROR                    6  /* RMW, A */
#define W65C02S_OPER_TSB                    7  /* RMW */
#define W65C02S_OPER_TRB                    8  /* RMW */
/* implied instrs */
#define W65C02S_OPER_CLV                    7  /*      impl */
#define W65C02S_OPER_CLC                    8  /*      impl */
#define W65C02S_OPER_SEC                    9  /*      impl */
#define W65C02S_OPER_CLI                    10 /*      impl */
#define W65C02S_OPER_SEI                    11 /*      impl */
#define W65C02S_OPER_CLD                    12 /*      impl */
#define W65C02S_OPER_SED                    13 /*      impl */
#define W65C02S_OPER_TAX                    14 /*      impl */
#define W65C02S_OPER_TXA                    15 /*      impl */
#define W65C02S_OPER_TAY                    16 /*      impl */
#define W65C02S_OPER_TYA                    17 /*      impl */
#define W65C02S_OPER_TSX                    18 /*      impl */
#define W65C02S_OPER_TXS                    19 /*      impl */

/* branch instrs. no NOPs use this mode, so 0 is ok */
#define W65C02S_OPER_BPL                    0
#define W65C02S_OPER_BMI                    1
#define W65C02S_OPER_BVC                    2
#define W65C02S_OPER_BVS                    3
#define W65C02S_OPER_BCC                    4
#define W65C02S_OPER_BCS                    5
#define W65C02S_OPER_BNE                    6
#define W65C02S_OPER_BEQ                    7
#define W65C02S_OPER_BRA                    8

/* stack instrs. no NOPs use this mode, so 0 is ok */
#define W65C02S_OPER_PHP                    0
#define W65C02S_OPER_PHA                    1
#define W65C02S_OPER_PHX                    2
#define W65C02S_OPER_PHY                    3
#define W65C02S_OPER_PLP                    0
#define W65C02S_OPER_PLA                    1
#define W65C02S_OPER_PLX                    2
#define W65C02S_OPER_PLY                    3

/* wait/stop instrs. no NOPs use this mode, so 0 is ok */
#define W65C02S_OPER_WAI                    0
#define W65C02S_OPER_STP                    1

/* ! (only instructions for their modes, set them as 0) */
#define W65C02S_OPER_JMP                    0
#define W65C02S_OPER_JSR                    0
#define W65C02S_OPER_RTS                    0
#define W65C02S_OPER_BRK                    0
#define W65C02S_OPER_RTI                    0



/* +------------------------------------------------------------------------+ */
/* |                                                                        | */
/* |                           INTERNAL OPERATIONS                          | */
/* |                                                                        | */
/* +------------------------------------------------------------------------+ */



/* update flags. N: bit 7 of q. Z: whether q is 0 */
W65C02S_INLINE uint8_t w65c02s_mark_nz(struct w65c02s_cpu *cpu, uint8_t q) {
    uint8_t p = cpu->p;
    W65C02S_SET_P(p, W65C02S_P_N, q & 0x80);
    W65C02S_SET_P(p, W65C02S_P_Z, q == 0);
    cpu->p = p;
    return q;
}

/* update flags. N: bit 7 of q. Z: whether q is 0, C: as given */
W65C02S_INLINE uint8_t w65c02s_mark_nzc(struct w65c02s_cpu *cpu,
                                        uint8_t q, bool c) {
    uint8_t p = cpu->p;
    W65C02S_SET_P(p, W65C02S_P_N, q & 0x80);
    W65C02S_SET_P(p, W65C02S_P_Z, q == 0);
    W65C02S_SET_P(p, W65C02S_P_C, c);
    cpu->p = p;
    return q;
}

/* update flags. N: bit 7 of q. Z: whether q is 0, C: bit 8 of q */
W65C02S_INLINE uint8_t w65c02s_mark_nzc8(struct w65c02s_cpu *cpu, unsigned q) {
    /* q is a 9-bit value with carry at bit 8 */
    return w65c02s_mark_nzc(cpu, q, q >> 8);
}

/* increment and return v, and update N, Z according to return value. */
W65C02S_INLINE uint8_t w65c02s_oper_inc(struct w65c02s_cpu *cpu, uint8_t v) {
    return w65c02s_mark_nz(cpu, v + 1);
}

/* decrement and return v, and update N, Z according to return value. */
W65C02S_INLINE uint8_t w65c02s_oper_dec(struct w65c02s_cpu *cpu, uint8_t v) {
    return w65c02s_mark_nz(cpu, v - 1);
}

/* left-shift and return v, and update N, Z according to return value.
   fill in a 0, and shift the leftover bit to C. */
W65C02S_INLINE uint8_t w65c02s_oper_asl(struct w65c02s_cpu *cpu, uint8_t v) {
    /* new carry is the highest bit */
    return w65c02s_mark_nzc(cpu, v << 1, v >> 7);
}

/* right-shift and return v, and update N, Z according to return value.
   fill in a 0, and shift the leftover bit to C. */
W65C02S_INLINE uint8_t w65c02s_oper_lsr(struct w65c02s_cpu *cpu, uint8_t v) {
    /* new carry is the lowest bit */
    return w65c02s_mark_nzc(cpu, v >> 1, v & 1);
}

/* left-shift and return v, and update N, Z according to return value.
   fill in the old C, and shift the leftover bit to C. */
W65C02S_INLINE uint8_t w65c02s_oper_rol(struct w65c02s_cpu *cpu, uint8_t v) {
    /* new carry is the highest bit */
    uint8_t c = W65C02S_GET_P(cpu->p, W65C02S_P_C);
    return w65c02s_mark_nzc(cpu, (v << 1) | c, v >> 7);
}

/* right-shift and return v, and update N, Z according to return value.
   fill in the old C, and shift the leftover bit to C. */
W65C02S_INLINE uint8_t w65c02s_oper_ror(struct w65c02s_cpu *cpu, uint8_t v) {
    /* new carry is the lowest bit */
    uint8_t c = W65C02S_GET_P(cpu->p, W65C02S_P_C);
    return w65c02s_mark_nzc(cpu, (v >> 1) | (c << 7), v & 1);
}

/* compute the correct V flag for ADC a, b, c (SBC a, b, c = ADC a, ~b, c)*/
W65C02S_INLINE bool w65c02s_oper_adc_v(uint8_t a, uint8_t b, uint8_t c) {
    unsigned c6 = ((a & 0x7F) + (b & 0x7F) + c) >> 7; /* carry for A6 + B6 */
    unsigned c7 = (a + b + c) >> 8;                   /* carry for A7 + B7 */
    return c6 ^ c7;                                   /* V = C6 XOR C7 */
}

static uint8_t w65c02s_oper_adc_d(struct w65c02s_cpu *cpu,
                                  uint8_t a, uint8_t b, unsigned c) {
    /* BCD addition one nibble/digit at a time */
    unsigned lo, hi, hc, fc, q;
    uint8_t p_adj;

    lo = (a & 15) + (b & 15) + c;       /* add low nibbles */
    hc = lo >= 10;                      /* half carry */
    lo = (hc ? lo - 10 : lo) & 15;        /* adjust result on half carry set */

    hi = (a >> 4) + (b >> 4) + hc;      /* add high nibbles */
    fc = hi >= 10;                      /* full carry */
    hi = (fc ? hi - 10 : hi) & 15;        /* adjust result on full carry set */

    q = (hi << 4) | lo;                 /* build result */

    p_adj = fc ? W65C02S_P_C : 0;
    W65C02S_SET_P(cpu->p, W65C02S_P_C, fc);
    W65C02S_SET_P(p_adj, W65C02S_P_N, q >> 7);
    W65C02S_SET_P(p_adj, W65C02S_P_Z, q == 0);
    /* keep W65C02S_P_V as in binary addition */
    cpu->p_adj = p_adj;
    return (uint8_t)q;
}

static uint8_t w65c02s_oper_sbc_d(struct w65c02s_cpu *cpu,
                                  uint8_t a, uint8_t b, unsigned c) {
    /* BCD subtraction one nibble/digit at a time */
    unsigned lo, hi, hc, fc, q;
    uint8_t p_adj;
    
    lo = (a & 15) + (b & 15) + c;       /* subtract low nibbles */
    hc = lo >= 16;                      /* half carry */
    lo = (hc ? lo : 10 + lo) & 15;      /* adjust result on half carry set */

    hi = (a >> 4) + (b >> 4) + hc;      /* subtract high nibbles */
    fc = hi >= 16;                      /* full carry */
    hi = (fc ? hi : 10 + hi) & 15;      /* adjust result on full carry set */

    q = (hi << 4) | lo;                 /* build result */
    
    p_adj = fc ? W65C02S_P_C : 0;
    W65C02S_SET_P(cpu->p, W65C02S_P_C, fc);
    W65C02S_SET_P(p_adj, W65C02S_P_N, q >> 7);
    W65C02S_SET_P(p_adj, W65C02S_P_Z, q == 0);
    /* keep W65C02S_P_V as in binary addition */
    cpu->p_adj = p_adj;
    return (uint8_t)q;
}

/* ADC a, b = a + b + c (except in BCD mode). updates N, Z, V, C. */
W65C02S_INLINE uint8_t w65c02s_oper_adc(struct w65c02s_cpu *cpu,
                                        uint8_t a, uint8_t b) {
    uint8_t r, p = cpu->p;
    uint8_t c = W65C02S_GET_P(p, W65C02S_P_C); /* old carry */
    /* update V flag */
    cpu->p = W65C02S_SET_P(p, W65C02S_P_V, w65c02s_oper_adc_v(a, b, c));
    r = w65c02s_mark_nzc8(cpu, a + b + c); /* update N, Z, C */
    if (!W65C02S_GET_P(p, W65C02S_P_D)) return r;
    return w65c02s_oper_adc_d(cpu, a, b, c); /* use decimal mode instead */
}

/* SBC a, b = a + ~b + c (except in BCD mode). updates N, Z, V, C. */
W65C02S_INLINE uint8_t w65c02s_oper_sbc(struct w65c02s_cpu *cpu,
                                        uint8_t a, uint8_t b) {
    uint8_t r, p = cpu->p;
    uint8_t c = W65C02S_GET_P(p, W65C02S_P_C); /* old carry */
    b = ~b; /* flip B -- SBC A, B == ADC A, ~B */
    /* update V flag */
    cpu->p = W65C02S_SET_P(p, W65C02S_P_V, w65c02s_oper_adc_v(a, b, c));
    r = w65c02s_mark_nzc8(cpu, a + b + c); /* update N, Z, C */
    if (!W65C02S_GET_P(p, W65C02S_P_D)) return r;
    return w65c02s_oper_sbc_d(cpu, a, b, c); /* use decimal mode instead */
}

/* CMP a, b = a - b, but update only flags N, Z, C. */
W65C02S_INLINE void w65c02s_oper_cmp(struct w65c02s_cpu *cpu,
                                     uint8_t a, uint8_t b) {
    w65c02s_mark_nzc8(cpu, a + (uint8_t)(~b) + 1);
}

/* BIT a, b = update Z based on a & b, copy P bits 7 and 6 (N, V) from b. */
static void w65c02s_oper_bit(struct w65c02s_cpu *cpu, uint8_t a, uint8_t b) {
    /* in BIT, N (b7) and V (b6) are bits 7 and 6 of the memory operand */
    cpu->p = (b & 0xC0) | (cpu->p & 0x3F);
    W65C02S_SET_P(cpu->p, W65C02S_P_Z, !(a & b));
}

/* BIT a, #b ($89) does not update N and V, only Z. */
static void w65c02s_oper_bit_imm(struct w65c02s_cpu *cpu, uint8_t b) {
    W65C02S_SET_P(cpu->p, W65C02S_P_Z, !(cpu->a & b));
}

/* TSB(set=1)/TRB(set=0) a, b. returns new value of b. updates Z. */
static uint8_t w65c02s_oper_tsb(struct w65c02s_cpu *cpu,
                                uint8_t a, uint8_t b, bool set) {
    W65C02S_SET_P(cpu->p, W65C02S_P_Z, !(a & b));
    return set ? b | a : b & ~a;
}

/* perform RMW op on value v and return new value. updates flags. */
W65C02S_INLINE uint8_t w65c02s_oper_rmw(struct w65c02s_cpu *cpu,
                                unsigned op, uint8_t v) {
    switch (op) {
        case W65C02S_OPER_ASL: return w65c02s_oper_asl(cpu, v);
        case W65C02S_OPER_DEC: return w65c02s_oper_dec(cpu, v);
        case W65C02S_OPER_INC: return w65c02s_oper_inc(cpu, v);
        case W65C02S_OPER_LSR: return w65c02s_oper_lsr(cpu, v);
        case W65C02S_OPER_ROL: return w65c02s_oper_rol(cpu, v);
        case W65C02S_OPER_ROR: return w65c02s_oper_ror(cpu, v);
    }
    W65C02S_UNREACHABLE();
    return v;
}

/* perform ALU op on values a, b and return new value. updates flags. */
W65C02S_INLINE uint8_t w65c02s_oper_alu(struct w65c02s_cpu *cpu,
                                        unsigned op, uint8_t a, uint8_t b) {
    switch (op) {
        case W65C02S_OPER_AND: return w65c02s_mark_nz(cpu, a & b);
        case W65C02S_OPER_EOR: return w65c02s_mark_nz(cpu, a ^ b);
        case W65C02S_OPER_ORA: return w65c02s_mark_nz(cpu, a | b);
        case W65C02S_OPER_ADC: return w65c02s_oper_adc(cpu, a, b);
        case W65C02S_OPER_SBC: return w65c02s_oper_sbc(cpu, a, b);
    }
    W65C02S_UNREACHABLE();
    return w65c02s_mark_nz(cpu, b);
}

/* returns whether to take the branch op with the current value of P */
W65C02S_INLINE bool w65c02s_oper_branch(unsigned op, uint8_t p) {
    /* whether to take the branch? */
    switch (op) {
        case W65C02S_OPER_BPL: return !(p & W65C02S_P_N); /* if N clear */
        case W65C02S_OPER_BMI: return  (p & W65C02S_P_N); /* if N set */
        case W65C02S_OPER_BVC: return !(p & W65C02S_P_V); /* if V clear */
        case W65C02S_OPER_BVS: return  (p & W65C02S_P_V); /* if V set */
        case W65C02S_OPER_BCC: return !(p & W65C02S_P_C); /* if C clear */
        case W65C02S_OPER_BCS: return  (p & W65C02S_P_C); /* if C set */
        case W65C02S_OPER_BNE: return !(p & W65C02S_P_Z); /* if Z clear */
        case W65C02S_OPER_BEQ: return  (p & W65C02S_P_Z); /* if Z set */
        case W65C02S_OPER_BRA: return true;               /* always */
    }
    W65C02S_UNREACHABLE();
    return false;
}

/* run a SMxB/RMBx instruction. oper&8 = set(1)/reset(0), oper&7 = bit (0-7)
   v = existing memory value, returns new value. */
static uint8_t w65c02s_oper_bitset(unsigned oper, uint8_t v) {
    uint8_t mask = 1 << (oper & 7);
    return (oper & 8) ? v | mask : v & ~mask;
}

/* whether to take a BBRx/BBSx branch.
   oper&8 = set(1)/reset(0), oper&7 = bit (0-7). v = existing memory value */
static bool w65c02s_oper_bitbranch(unsigned oper, uint8_t v) {
    uint8_t mask = 1 << (oper & 7);
    return (oper & 8) ? v & mask : !(v & mask);
}

/* push value onto stack. counts as a memory write */
W65C02S_INLINE void w65c02s_stack_push(struct w65c02s_cpu *cpu, uint8_t v) {
    uint16_t s = W65C02S_STACK_ADDR(cpu->s--);
    W65C02S_WRITE(s, v);
}

/* pop value off stack. counts as a memory read */
W65C02S_INLINE uint8_t w65c02s_stack_pull(struct w65c02s_cpu *cpu) {
    uint16_t s = W65C02S_STACK_ADDR(++cpu->s);
    return W65C02S_READ(s);
}

/* update interrupt mask, which is ~P_I if IRQs are disabled and else ~0. 
   to be called whenever the I flag changes in P. */
W65C02S_INLINE void w65c02s_irq_update_mask(struct w65c02s_cpu *cpu) {
    /* note: W65C02S_CPU_STATE_IRQ == P_I */
    cpu->int_mask = W65C02S_GET_P(cpu->p, W65C02S_P_I)
                        ? ~W65C02S_CPU_STATE_IRQ : ~0;
}

/* latch interrupts from int_trig based on int_mask.
   NMI is always latched, but IRQ may be masked away.
   interrupt latch happens on the end of the penultimate cycle of any
   instruction (i.e. before the last cycle).
   on 2-cycle ops, it happens after fetching the opcode.
   on 1-cycle NOPs, it does not happen at all! */
W65C02S_INLINE void w65c02s_irq_latch(struct w65c02s_cpu *cpu) {
    cpu->cpu_state |= cpu->int_trig & cpu->int_mask;
}

/* resets the IRQ flag and re-latches. used for extra cycles. */
W65C02S_INLINE void w65c02s_irq_latch_slow(struct w65c02s_cpu *cpu) {
    cpu->cpu_state = (cpu->cpu_state & ~W65C02S_CPU_STATE_IRQ)
                   | (cpu->int_trig & cpu->int_mask);
}

/* selects an interrupt vector. used by BRK. */
static uint16_t w65c02s_irq_select_vector(struct w65c02s_cpu *cpu) {
    if (cpu->in_rst) {
        cpu->in_rst = false;
        return W65C02S_VEC_RST;
    } else if (cpu->in_nmi) {
        cpu->in_nmi = false;
        return W65C02S_VEC_NMI;
    } else if (cpu->in_irq) {
        cpu->in_irq = false;
    }
    return W65C02S_VEC_IRQ;
}

W65C02S_INLINE uint16_t w65c02s_compute_branch(uint16_t pc, uint8_t offset) {
    return pc + offset - (offset & 0x80 ? 0x100 : 0);
}

/* most RMW abs,x instructions can avoid a penalty cycle, but INC, DEC cannot */
W65C02S_INLINE bool w65c02s_slow_rmw_absx(unsigned oper) {
    switch (oper) {
        case W65C02S_OPER_INC:
        case W65C02S_OPER_DEC:
            return true;
    }
    return false;
}

/* false = no penalty, true = decimal mode penalty */
W65C02S_INLINE bool w65c02s_oper_imm(struct w65c02s_cpu *cpu,
                                     unsigned oper, uint8_t v) {
    switch (oper) {
        case W65C02S_OPER_NOP:
            break;

        case W65C02S_OPER_AND:
        case W65C02S_OPER_EOR:
        case W65C02S_OPER_ORA:
            cpu->a = w65c02s_oper_alu(cpu, oper, cpu->a, v);
            return false;

        case W65C02S_OPER_ADC:
        case W65C02S_OPER_SBC:
            cpu->a = w65c02s_oper_alu(cpu, oper, cpu->a, v);
            /* decimal mode penalty */
            return W65C02S_GET_P(cpu->p, W65C02S_P_D);

        case W65C02S_OPER_CMP: w65c02s_oper_cmp(cpu, cpu->a, v);     break;
        case W65C02S_OPER_CPX: w65c02s_oper_cmp(cpu, cpu->x, v);     break;
        case W65C02S_OPER_CPY: w65c02s_oper_cmp(cpu, cpu->y, v);     break;
        case W65C02S_OPER_BIT: w65c02s_oper_bit_imm(cpu, v);         break;
        
        case W65C02S_OPER_LDA: cpu->a = w65c02s_mark_nz(cpu, v);     break;
        case W65C02S_OPER_LDX: cpu->x = w65c02s_mark_nz(cpu, v);     break;
        case W65C02S_OPER_LDY: cpu->y = w65c02s_mark_nz(cpu, v);     break;
        default: W65C02S_UNREACHABLE();
    }
    return false;
}

/* false = no penalty, true = decimal mode penalty */
W65C02S_INLINE bool w65c02s_oper_ea(struct w65c02s_cpu *cpu,
                                    unsigned oper, uint16_t ea) {
    switch (oper) {
        case W65C02S_OPER_NOP:
            W65C02S_READ(ea);
            break;

        case W65C02S_OPER_AND:
        case W65C02S_OPER_EOR:
        case W65C02S_OPER_ORA:
            cpu->a = w65c02s_oper_alu(cpu, oper, cpu->a, W65C02S_READ(ea));
            return false;

        case W65C02S_OPER_ADC:
        case W65C02S_OPER_SBC:
            cpu->a = w65c02s_oper_alu(cpu, oper, cpu->a, W65C02S_READ(ea));
            /* decimal mode penalty */
            return W65C02S_GET_P(cpu->p, W65C02S_P_D);

        case W65C02S_OPER_CMP:
            w65c02s_oper_cmp(cpu, cpu->a, W65C02S_READ(ea));
            break;
        case W65C02S_OPER_CPX:
            w65c02s_oper_cmp(cpu, cpu->x, W65C02S_READ(ea));
            break;
        case W65C02S_OPER_CPY:
            w65c02s_oper_cmp(cpu, cpu->y, W65C02S_READ(ea));
            break;
        case W65C02S_OPER_BIT:
            w65c02s_oper_bit(cpu, cpu->a, W65C02S_READ(ea));
            break;

        case W65C02S_OPER_LDA:
            cpu->a = w65c02s_mark_nz(cpu, W65C02S_READ(ea));
            break;
        case W65C02S_OPER_LDX:
            cpu->x = w65c02s_mark_nz(cpu, W65C02S_READ(ea));
            break;
        case W65C02S_OPER_LDY:
            cpu->y = w65c02s_mark_nz(cpu, W65C02S_READ(ea));
            break;

        case W65C02S_OPER_STA: W65C02S_WRITE(ea, cpu->a); break;
        case W65C02S_OPER_STX: W65C02S_WRITE(ea, cpu->x); break;
        case W65C02S_OPER_STY: W65C02S_WRITE(ea, cpu->y); break;
        case W65C02S_OPER_STZ: W65C02S_WRITE(ea, 0);      break;
        default: W65C02S_UNREACHABLE();
    }
    return false;
}



/* +------------------------------------------------------------------------+ */
/* |                                                                        | */
/* |                            ADDRESSING MODES                            | */
/* |                                                                        | */
/* +------------------------------------------------------------------------+ */



#if !W65C02S_COARSE
/* current CPU, whether we are continuing an instruction */
#define W65C02S_PARAMS_MODE struct w65c02s_cpu *cpu, unsigned oper, bool cont
#define W65C02S_GO_MODE(mode, oper) return w65c02s_mode_##mode(cpu, oper, cont)
#else
#define W65C02S_PARAMS_MODE struct w65c02s_cpu *cpu, unsigned oper
#define W65C02S_GO_MODE(mode, oper) return w65c02s_mode_##mode(cpu, oper)
#endif

/* the extra cycle done on ADC/SBC if D is set. copies p_adj to p.
   skipped if flag is false (use the return value of
      w65c02s_oper_imm or w65c02s_oper_ea) */
#define W65C02S_P_ADJ_MASK (W65C02S_P_N | W65C02S_P_Z | W65C02S_P_C)
#define W65C02S_ADC_D_SPURIOUS(ea)                                             \
    cpu->p = (cpu->p & ~W65C02S_P_ADJ_MASK) | cpu->p_adj;                      \
    W65C02S_READ(ea);
    /* no need to update mask - p_adj never changes I */

W65C02S_INLINE unsigned w65c02s_mode_IMPLIED(W65C02S_PARAMS_MODE) {
    W65C02S_BEGIN_INSTRUCTION
        /* 0: w65c02s_irq_latch(cpu); (prerun) */
        W65C02S_CYCLE(1)
            w65c02s_irq_latch(cpu);
            switch (oper) {
                case W65C02S_OPER_NOP:
                    break;

                case W65C02S_OPER_DEC:
                case W65C02S_OPER_INC:
                case W65C02S_OPER_ASL:
                case W65C02S_OPER_ROL:
                case W65C02S_OPER_LSR:
                case W65C02S_OPER_ROR:
                    cpu->a = w65c02s_oper_rmw(cpu, oper, cpu->a);
                    break;

                case W65C02S_OPER_CLV:
                    W65C02S_SET_P(cpu->p, W65C02S_P_V, 0);
                    break;
                case W65C02S_OPER_CLC:
                    W65C02S_SET_P(cpu->p, W65C02S_P_C, 0);
                    break;
                case W65C02S_OPER_SEC:
                    W65C02S_SET_P(cpu->p, W65C02S_P_C, 1);
                    break;
                case W65C02S_OPER_CLD:
                    W65C02S_SET_P(cpu->p, W65C02S_P_D, 0);
                    break;
                case W65C02S_OPER_SED:
                    W65C02S_SET_P(cpu->p, W65C02S_P_D, 1);
                    break;
                case W65C02S_OPER_CLI:
                    W65C02S_SET_P(cpu->p, W65C02S_P_I, 0);
                    w65c02s_irq_update_mask(cpu);
                    break;
                case W65C02S_OPER_SEI:
                    W65C02S_SET_P(cpu->p, W65C02S_P_I, 1);
                    w65c02s_irq_update_mask(cpu);
                    break;

/* transfer register src to dst and update N, Z flags. */
#define W65C02S_TRANSFER(src, dst) cpu->dst = w65c02s_mark_nz(cpu, cpu->src)
                case W65C02S_OPER_TAX: W65C02S_TRANSFER(a, x);  break;
                case W65C02S_OPER_TXA: W65C02S_TRANSFER(x, a);  break;
                case W65C02S_OPER_TAY: W65C02S_TRANSFER(a, y);  break;
                case W65C02S_OPER_TYA: W65C02S_TRANSFER(y, a);  break;
                case W65C02S_OPER_TSX: W65C02S_TRANSFER(s, x);  break;
/* TXS does not touch N or Z! */
                case W65C02S_OPER_TXS: cpu->s = cpu->x; break;
                default: W65C02S_UNREACHABLE();
            }
            W65C02S_READ(cpu->pc);
    W65C02S_END_INSTRUCTION
}

W65C02S_INLINE unsigned w65c02s_mode_IMPLIED_X(W65C02S_PARAMS_MODE) {
    W65C02S_BEGIN_INSTRUCTION
        /* 0: w65c02s_irq_latch(cpu); (prerun) */
        W65C02S_CYCLE(1)
            w65c02s_irq_latch(cpu);
            cpu->x = w65c02s_oper_rmw(cpu, oper, cpu->x);
            W65C02S_READ(cpu->pc);
    W65C02S_END_INSTRUCTION
}

W65C02S_INLINE unsigned w65c02s_mode_IMPLIED_Y(W65C02S_PARAMS_MODE) {
    W65C02S_BEGIN_INSTRUCTION
        /* 0: w65c02s_irq_latch(cpu); (prerun) */
        W65C02S_CYCLE(1)
            w65c02s_irq_latch(cpu);
            cpu->y = w65c02s_oper_rmw(cpu, oper, cpu->y);
            W65C02S_READ(cpu->pc);
    W65C02S_END_INSTRUCTION
}

W65C02S_INLINE unsigned w65c02s_mode_IMMEDIATE(W65C02S_PARAMS_MODE) {
    W65C02S_BEGIN_INSTRUCTION
        /* 0: w65c02s_irq_latch(cpu); (prerun) */
        W65C02S_CYCLE(1)
            w65c02s_irq_latch(cpu);
            /* penalty cycle? */
            if (W65C02S_LIKELY(!w65c02s_oper_imm(cpu, oper,
                                W65C02S_READ(cpu->pc++))))
                W65C02S_SKIP_REST_AFTER; /* no penalty. */
            else w65c02s_irq_latch_slow(cpu);
        W65C02S_CYCLE(2)
            W65C02S_ADC_D_SPURIOUS(cpu->pc);
    W65C02S_END_INSTRUCTION
}

W65C02S_INLINE unsigned w65c02s_mode_ZEROPAGE(W65C02S_PARAMS_MODE) {
    W65C02S_USE_TR(ea)
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            W65C02S_TR.ea = W65C02S_READ(cpu->pc++);
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(2)
            /* penalty cycle? */
            if (W65C02S_LIKELY(!w65c02s_oper_ea(cpu, oper, W65C02S_TR.ea)))
                W65C02S_SKIP_REST_AFTER; /* no penalty. */
            else w65c02s_irq_latch_slow(cpu);
        W65C02S_CYCLE(3)
            W65C02S_ADC_D_SPURIOUS(W65C02S_TR.ea);
    W65C02S_END_INSTRUCTION
}

W65C02S_INLINE unsigned w65c02s_mode_ZEROPAGE_X(W65C02S_PARAMS_MODE) {
    W65C02S_USE_TR(ea8)
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            W65C02S_TR.ea = W65C02S_READ(cpu->pc);
        W65C02S_CYCLE(2)
            W65C02S_READ(cpu->pc++);
            W65C02S_TR.ea += cpu->x;
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(3)
            /* penalty cycle? */
            if (W65C02S_LIKELY(!w65c02s_oper_ea(cpu, oper, W65C02S_TR.ea)))
                W65C02S_SKIP_REST_AFTER; /* no penalty. */
            else w65c02s_irq_latch_slow(cpu);
        W65C02S_CYCLE(4)
            W65C02S_ADC_D_SPURIOUS(W65C02S_TR.ea);
    W65C02S_END_INSTRUCTION
}

W65C02S_INLINE unsigned w65c02s_mode_ZEROPAGE_Y(W65C02S_PARAMS_MODE) {
    W65C02S_USE_TR(ea8)
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            W65C02S_TR.ea = W65C02S_READ(cpu->pc);
        W65C02S_CYCLE(2)
            W65C02S_READ(cpu->pc++);
            W65C02S_TR.ea += cpu->y;
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(3)
            /* penalty cycle? */
            if (W65C02S_LIKELY(!w65c02s_oper_ea(cpu, oper, W65C02S_TR.ea)))
                W65C02S_SKIP_REST_AFTER; /* no penalty. */
            else w65c02s_irq_latch_slow(cpu);
        W65C02S_CYCLE(4)
            W65C02S_ADC_D_SPURIOUS(W65C02S_TR.ea);
    W65C02S_END_INSTRUCTION
}

W65C02S_INLINE unsigned w65c02s_mode_ABSOLUTE(W65C02S_PARAMS_MODE) {
    W65C02S_USE_TR(ea)
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            W65C02S_TR.ea = W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(2)
            W65C02S_SET_HI(W65C02S_TR.ea, W65C02S_READ(cpu->pc++));
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(3)
            /* penalty cycle? */
            if (W65C02S_LIKELY(!w65c02s_oper_ea(cpu, oper, W65C02S_TR.ea)))
                W65C02S_SKIP_REST_AFTER; /* no penalty. */
            else w65c02s_irq_latch_slow(cpu);
        W65C02S_CYCLE(4)
            W65C02S_ADC_D_SPURIOUS(W65C02S_TR.ea);
    W65C02S_END_INSTRUCTION
}

W65C02S_INLINE unsigned w65c02s_mode_ABSOLUTE_X(W65C02S_PARAMS_MODE) {
    W65C02S_USE_TR(ea_page)
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            W65C02S_TR.ea = W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(2)
        {
            uint16_t page = W65C02S_READ(cpu->pc++) << 8;
            /* page wrap cycle? */
            W65C02S_TR.page_penalty = W65C02S_OVERFLOW8(W65C02S_TR.ea, cpu->x);
            W65C02S_TR.ea += page | cpu->x;
            if (!W65C02S_TR.page_penalty) w65c02s_irq_latch(cpu);
        }
        W65C02S_CYCLE(3)
            /* if we did not cross the page boundary, skip this cycle. */
            if (!W65C02S_TR.page_penalty) W65C02S_SKIP_TO_NEXT(4);
            W65C02S_READ(cpu->pc - 1);
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(4)
            /* penalty cycle? */
            if (W65C02S_LIKELY(!w65c02s_oper_ea(cpu, oper, W65C02S_TR.ea)))
                W65C02S_SKIP_REST_AFTER; /* no penalty. */
            else w65c02s_irq_latch_slow(cpu);
        W65C02S_CYCLE(5)
            W65C02S_ADC_D_SPURIOUS(W65C02S_TR.ea);
    W65C02S_END_INSTRUCTION
}

W65C02S_INLINE unsigned w65c02s_mode_ABSOLUTE_X_STORE(W65C02S_PARAMS_MODE) {
    W65C02S_USE_TR(ea_page)
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            W65C02S_TR.ea = W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(2)
        {
            uint16_t page = W65C02S_READ(cpu->pc++) << 8;
            bool page_crossed = W65C02S_OVERFLOW8(W65C02S_TR.ea, cpu->x);
            W65C02S_TR.ea += page | cpu->x;
            W65C02S_TR.ea_wrong = page_crossed ? cpu->pc - 1 : W65C02S_TR.ea;
        }
        W65C02S_CYCLE(3)
            W65C02S_READ(W65C02S_TR.ea_wrong);
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(4)
            /* stores never have penalty cycles */
            w65c02s_oper_ea(cpu, oper, W65C02S_TR.ea);
    W65C02S_END_INSTRUCTION
}

W65C02S_INLINE unsigned w65c02s_mode_ABSOLUTE_Y(W65C02S_PARAMS_MODE) {
    W65C02S_USE_TR(ea_page)
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            W65C02S_TR.ea = W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(2)
        {
            uint16_t page = W65C02S_READ(cpu->pc++) << 8;
            /* page wrap cycle? */
            W65C02S_TR.page_penalty = W65C02S_OVERFLOW8(W65C02S_TR.ea, cpu->y);
            W65C02S_TR.ea += page | cpu->y;
            if (!W65C02S_TR.page_penalty) w65c02s_irq_latch(cpu);
        }
        W65C02S_CYCLE(3)
            /* if we did not cross the page boundary, skip this cycle. */
            if (!W65C02S_TR.page_penalty) W65C02S_SKIP_TO_NEXT(4);
            W65C02S_READ(cpu->pc - 1);
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(4)
            /* penalty cycle? */
            if (W65C02S_LIKELY(!w65c02s_oper_ea(cpu, oper, W65C02S_TR.ea)))
                W65C02S_SKIP_REST_AFTER; /* no penalty. */
            else w65c02s_irq_latch_slow(cpu);
        W65C02S_CYCLE(5)
            W65C02S_ADC_D_SPURIOUS(W65C02S_TR.ea);
    W65C02S_END_INSTRUCTION
}

W65C02S_INLINE unsigned w65c02s_mode_ABSOLUTE_Y_STORE(W65C02S_PARAMS_MODE) {
    W65C02S_USE_TR(ea_page)
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            W65C02S_TR.ea = W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(2)
        {
            uint16_t page = W65C02S_READ(cpu->pc++) << 8;
            bool page_crossed = W65C02S_OVERFLOW8(W65C02S_TR.ea, cpu->y);
            W65C02S_TR.ea += page | cpu->y;
            W65C02S_TR.ea_wrong = page_crossed ? cpu->pc - 1 : W65C02S_TR.ea;
        }
        W65C02S_CYCLE(3)
            W65C02S_READ(W65C02S_TR.ea_wrong);
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(4)
            /* stores never have penalty cycles */
            w65c02s_oper_ea(cpu, oper, W65C02S_TR.ea);
    W65C02S_END_INSTRUCTION
}

W65C02S_INLINE unsigned w65c02s_mode_ZEROPAGE_INDIRECT(W65C02S_PARAMS_MODE) {
    W65C02S_USE_TR(ea_zp)
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            W65C02S_TR.zp = W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(2)
            W65C02S_TR.ea = W65C02S_READ(W65C02S_TR.zp++);
        W65C02S_CYCLE(3)
            W65C02S_SET_HI(W65C02S_TR.ea, W65C02S_READ(W65C02S_TR.zp));
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(4)
            /* penalty cycle? */
            if (W65C02S_LIKELY(!w65c02s_oper_ea(cpu, oper, W65C02S_TR.ea)))
                W65C02S_SKIP_REST_AFTER; /* no penalty. */
            else w65c02s_irq_latch_slow(cpu);
        W65C02S_CYCLE(5)
            W65C02S_ADC_D_SPURIOUS(W65C02S_TR.ea);
    W65C02S_END_INSTRUCTION
}

W65C02S_INLINE unsigned w65c02s_mode_ZEROPAGE_INDIRECT_X(W65C02S_PARAMS_MODE) {
    W65C02S_USE_TR(ea_zp)
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            W65C02S_TR.zp = W65C02S_READ(cpu->pc);
        W65C02S_CYCLE(2)
            W65C02S_READ(cpu->pc++);
            W65C02S_TR.zp += cpu->x;
        W65C02S_CYCLE(3)
            W65C02S_TR.ea = W65C02S_READ(W65C02S_TR.zp++);
        W65C02S_CYCLE(4)
            W65C02S_SET_HI(W65C02S_TR.ea, W65C02S_READ(W65C02S_TR.zp));
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(5)
            /* penalty cycle? */
            if (W65C02S_LIKELY(!w65c02s_oper_ea(cpu, oper, W65C02S_TR.ea)))
                W65C02S_SKIP_REST_AFTER; /* no penalty. */
            else w65c02s_irq_latch_slow(cpu);
        W65C02S_CYCLE(6)
            W65C02S_ADC_D_SPURIOUS(W65C02S_TR.ea);
    W65C02S_END_INSTRUCTION
}

W65C02S_INLINE unsigned w65c02s_mode_ZEROPAGE_INDIRECT_Y(W65C02S_PARAMS_MODE) {
    W65C02S_USE_TR(ea_zp)
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            W65C02S_TR.zp = W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(2)
            W65C02S_TR.ea = W65C02S_READ(W65C02S_TR.zp++);
        W65C02S_CYCLE(3)
        {
            uint16_t page = W65C02S_READ(W65C02S_TR.zp) << 8;
            /* page wrap cycle? */
            W65C02S_TR.page_penalty = W65C02S_OVERFLOW8(W65C02S_TR.ea, cpu->y);
            W65C02S_TR.ea += page | cpu->y;
            if (!W65C02S_TR.page_penalty) w65c02s_irq_latch(cpu);
        }
        W65C02S_CYCLE(4)
            /* if we did not cross the page boundary, skip this cycle */
            if (!W65C02S_TR.page_penalty) W65C02S_SKIP_TO_NEXT(5);
            W65C02S_READ(W65C02S_TR.zp);
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(5)
            /* penalty cycle? */
            if (W65C02S_LIKELY(!w65c02s_oper_ea(cpu, oper, W65C02S_TR.ea)))
                W65C02S_SKIP_REST_AFTER; /* no penalty. */
            else w65c02s_irq_latch_slow(cpu);
        W65C02S_CYCLE(6)
            W65C02S_ADC_D_SPURIOUS(W65C02S_TR.ea);
    W65C02S_END_INSTRUCTION
}

W65C02S_INLINE unsigned w65c02s_mode_ZEROPAGE_INDIRECT_Y_STORE(
                                                        W65C02S_PARAMS_MODE) {
    W65C02S_USE_TR(ea_zp)
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            W65C02S_TR.zp = W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(2)
            W65C02S_TR.ea = W65C02S_READ(W65C02S_TR.zp++);
        W65C02S_CYCLE(3)
            W65C02S_TR.ea += (W65C02S_READ(W65C02S_TR.zp) << 8) | cpu->y;
        W65C02S_CYCLE(4)
            W65C02S_READ(W65C02S_TR.zp);
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(5)
            /* stores never have penalty cycles */
            w65c02s_oper_ea(cpu, oper, W65C02S_TR.ea);
    W65C02S_END_INSTRUCTION
}

W65C02S_INLINE unsigned w65c02s_mode_ABSOLUTE_JUMP(W65C02S_PARAMS_MODE) {
    /* oper = JMP */
    W65C02S_USE_TR(ea)
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            W65C02S_TR.ea = W65C02S_READ(cpu->pc++);
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(2)
            cpu->pc = (W65C02S_READ(cpu->pc) << 8) | W65C02S_TR.ea;
    W65C02S_END_INSTRUCTION
}

W65C02S_INLINE unsigned w65c02s_mode_ABSOLUTE_INDIRECT(W65C02S_PARAMS_MODE) {
    /* oper = JMP */
    W65C02S_USE_TR(jump)
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            W65C02S_TR.ta = W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(2)
            W65C02S_SET_HI(W65C02S_TR.ta, W65C02S_READ(cpu->pc));
        W65C02S_CYCLE(3)
            W65C02S_READ(cpu->pc);
        W65C02S_CYCLE(4)
            W65C02S_TR.ea = W65C02S_READ(W65C02S_TR.ta++);
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(5)
            cpu->pc = (W65C02S_READ(W65C02S_TR.ta) << 8) | W65C02S_TR.ea;
    W65C02S_END_INSTRUCTION
}

W65C02S_INLINE unsigned w65c02s_mode_ABSOLUTE_INDIRECT_X(W65C02S_PARAMS_MODE) {
    /* oper = JMP */
    W65C02S_USE_TR(jump)
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            W65C02S_TR.ta = W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(2)
            W65C02S_TR.ta += W65C02S_READ(cpu->pc) << 8;
        W65C02S_CYCLE(3)
            W65C02S_READ(cpu->pc);
            W65C02S_TR.ta += cpu->x;
        W65C02S_CYCLE(4)
            W65C02S_TR.ea = W65C02S_READ(W65C02S_TR.ta++);
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(5)
            cpu->pc = (W65C02S_READ(W65C02S_TR.ta) << 8) | W65C02S_TR.ea;
    W65C02S_END_INSTRUCTION
}

static unsigned w65c02s_mode_ZEROPAGE_BIT(W65C02S_PARAMS_MODE) {
    W65C02S_USE_TR(rmw)
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            W65C02S_TR.ea = W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(2)
            W65C02S_TR.data = W65C02S_READ(W65C02S_TR.ea);
        W65C02S_CYCLE(3)
            W65C02S_TR.data = w65c02s_oper_bitset(oper, W65C02S_TR.data);
            W65C02S_READ(W65C02S_TR.ea);
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(4)
            W65C02S_WRITE(W65C02S_TR.ea, W65C02S_TR.data);
    W65C02S_END_INSTRUCTION
}

W65C02S_INLINE unsigned w65c02s_mode_RELATIVE(W65C02S_PARAMS_MODE) {
    W65C02S_USE_TR(branch)
    W65C02S_BEGIN_INSTRUCTION
        /* 0: w65c02s_irq_latch(cpu); (prerun) */
        W65C02S_CYCLE(1)
            w65c02s_irq_latch(cpu);
        {
            /* note the post-increment of PC here! */
            uint8_t offset = W65C02S_READ(cpu->pc++);
            /* copy PC to old_pc and compute new_pc with offset */
            W65C02S_TR.new_pc = w65c02s_compute_branch(
                        W65C02S_TR.old_pc = cpu->pc, offset);
        }
        W65C02S_CYCLE(2)
            /* skip the rest of the instruction if branch is not taken */
            if (!w65c02s_oper_branch(oper, cpu->p)) W65C02S_SKIP_REST;
            /* skip one read cycle if no page boundary crossed */
            if (W65C02S_GET_HI(cpu->pc = W65C02S_TR.new_pc)
                       == W65C02S_GET_HI(W65C02S_TR.old_pc))
                W65C02S_SKIP_TO_NEXT(3);
            W65C02S_READ(W65C02S_TR.old_pc);
            w65c02s_irq_latch_slow(cpu);
        W65C02S_CYCLE(3)
            W65C02S_READ(W65C02S_TR.old_pc);
    W65C02S_END_INSTRUCTION
}

static unsigned w65c02s_mode_RELATIVE_BIT(W65C02S_PARAMS_MODE) {
    W65C02S_USE_TR(branch_bit)
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            W65C02S_TR.new_pc = W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(2)
            W65C02S_READ(W65C02S_TR.new_pc);
        W65C02S_CYCLE(3)
            W65C02S_TR.data = W65C02S_READ(W65C02S_TR.new_pc);
        W65C02S_CYCLE(4)
        {
            /* note the post-increment of PC here! */
            uint8_t offset = W65C02S_READ(cpu->pc++);
            /* copy PC to old_pc and compute new_pc with offset */
            W65C02S_TR.new_pc = w65c02s_compute_branch(
                        W65C02S_TR.old_pc = cpu->pc, offset);
            w65c02s_irq_latch(cpu);
        }
        W65C02S_CYCLE(5)
            /* skip the rest of the instruction if branch is not taken */
            if (!w65c02s_oper_bitbranch(oper, W65C02S_TR.data))
                W65C02S_SKIP_REST;
            /* skip one read cycle if no page boundary crossed */
            if (W65C02S_GET_HI(cpu->pc = W65C02S_TR.new_pc)
                       == W65C02S_GET_HI(W65C02S_TR.old_pc))
                W65C02S_SKIP_TO_NEXT(6);
            W65C02S_READ(W65C02S_TR.old_pc);
            w65c02s_irq_latch_slow(cpu);
        W65C02S_CYCLE(6)
            W65C02S_READ(W65C02S_TR.old_pc);
    W65C02S_END_INSTRUCTION
}

W65C02S_INLINE unsigned w65c02s_mode_RMW_ZEROPAGE(W65C02S_PARAMS_MODE) {
    W65C02S_USE_TR(rmw)
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            W65C02S_TR.ea = W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(2)
            W65C02S_TR.data = W65C02S_READ(W65C02S_TR.ea);
        W65C02S_CYCLE(3)
        {
            uint8_t data = W65C02S_TR.data;
            W65C02S_READ(W65C02S_TR.ea);
            switch (oper) {
                case W65C02S_OPER_DEC:
                case W65C02S_OPER_INC:
                case W65C02S_OPER_ASL:
                case W65C02S_OPER_ROL:
                case W65C02S_OPER_LSR:
                case W65C02S_OPER_ROR:
                    data = w65c02s_oper_rmw(cpu, oper, data);
                    break;
                case W65C02S_OPER_TSB:
                    data = w65c02s_oper_tsb(cpu, cpu->a, data, 1);
                    break;
                case W65C02S_OPER_TRB:
                    data = w65c02s_oper_tsb(cpu, cpu->a, data, 0);
                    break;
                default: W65C02S_UNREACHABLE();
            }
            W65C02S_TR.data = data;
            w65c02s_irq_latch(cpu);
        }
        W65C02S_CYCLE(4)
            W65C02S_WRITE(W65C02S_TR.ea, W65C02S_TR.data);
    W65C02S_END_INSTRUCTION
}

W65C02S_INLINE unsigned w65c02s_mode_RMW_ZEROPAGE_X(W65C02S_PARAMS_MODE) {
    W65C02S_USE_TR(rmw8)
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            W65C02S_TR.ea = W65C02S_READ(cpu->pc) + cpu->x;
        W65C02S_CYCLE(2)
            W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(3)
            W65C02S_TR.data = W65C02S_READ(W65C02S_TR.ea);
        W65C02S_CYCLE(4)
            W65C02S_READ(W65C02S_TR.ea);
            W65C02S_TR.data = w65c02s_oper_rmw(cpu, oper, W65C02S_TR.data);
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(5)
            W65C02S_WRITE(W65C02S_TR.ea, W65C02S_TR.data);
    W65C02S_END_INSTRUCTION
}

W65C02S_INLINE unsigned w65c02s_mode_RMW_ABSOLUTE(W65C02S_PARAMS_MODE) {
    W65C02S_USE_TR(rmw)
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            W65C02S_TR.ea = W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(2)
            W65C02S_SET_HI(W65C02S_TR.ea, W65C02S_READ(cpu->pc++));
        W65C02S_CYCLE(3)
            W65C02S_TR.data = W65C02S_READ(W65C02S_TR.ea);
        W65C02S_CYCLE(4)
        {
            uint8_t data = W65C02S_TR.data;
            W65C02S_READ(W65C02S_TR.ea);
            switch (oper) {
                case W65C02S_OPER_DEC:
                case W65C02S_OPER_INC:
                case W65C02S_OPER_ASL:
                case W65C02S_OPER_ROL:
                case W65C02S_OPER_LSR:
                case W65C02S_OPER_ROR:
                    data = w65c02s_oper_rmw(cpu, oper, data);
                    break;
                case W65C02S_OPER_TSB:
                    data = w65c02s_oper_tsb(cpu, cpu->a, data, 1);
                    break;
                case W65C02S_OPER_TRB:
                    data = w65c02s_oper_tsb(cpu, cpu->a, data, 0);
                    break;
                default: W65C02S_UNREACHABLE();
            }
            W65C02S_TR.data = data;
            w65c02s_irq_latch(cpu);
        }
        W65C02S_CYCLE(5)
            W65C02S_WRITE(W65C02S_TR.ea, W65C02S_TR.data);
    W65C02S_END_INSTRUCTION
}

W65C02S_INLINE unsigned w65c02s_mode_RMW_ABSOLUTE_X(W65C02S_PARAMS_MODE) {
    W65C02S_USE_TR(rmw)
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            W65C02S_TR.ea = W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(2)
            W65C02S_SET_HI(W65C02S_TR.ea, W65C02S_READ(cpu->pc++));
        W65C02S_CYCLE(3)
        {
            bool penalty = w65c02s_slow_rmw_absx(oper) ||
                    W65C02S_OVERFLOW8(cpu->x, W65C02S_GET_LO(W65C02S_TR.ea));
            W65C02S_TR.ea += cpu->x;
            /* if not INC and DEC and we did not cross a page, skip cycle. */
            if (!penalty) W65C02S_SKIP_TO_NEXT(4);
            W65C02S_READ(W65C02S_TR.ea);
        }
        W65C02S_CYCLE(4)
            W65C02S_TR.data = W65C02S_READ(W65C02S_TR.ea);
        W65C02S_CYCLE(5)
            W65C02S_READ(W65C02S_TR.ea);
            W65C02S_TR.data = w65c02s_oper_rmw(cpu, oper, W65C02S_TR.data);
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(6)
            W65C02S_WRITE(W65C02S_TR.ea, W65C02S_TR.data);
    W65C02S_END_INSTRUCTION
}

W65C02S_INLINE unsigned w65c02s_mode_STACK_PUSH(W65C02S_PARAMS_MODE) {
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            W65C02S_READ(cpu->pc);
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(2)
        {
            uint8_t tmp;
            switch (oper) {
                case W65C02S_OPER_PHP:
                    /* PHP always pushes P with bits 5 and 4 set. */
                    tmp = cpu->p | W65C02S_P_A1 | W65C02S_P_B;
                    break;
                case W65C02S_OPER_PHA: tmp = cpu->a; break;
                case W65C02S_OPER_PHX: tmp = cpu->x; break;
                case W65C02S_OPER_PHY: tmp = cpu->y; break;
                default: tmp = 0; W65C02S_UNREACHABLE();
            }
            w65c02s_stack_push(cpu, tmp);
        }
    W65C02S_END_INSTRUCTION
}

W65C02S_INLINE unsigned w65c02s_mode_STACK_PULL(W65C02S_PARAMS_MODE) {
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            W65C02S_READ(cpu->pc);
        W65C02S_CYCLE(2)
            W65C02S_READ(W65C02S_STACK_ADDR(cpu->s));
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(3)
        {
            uint8_t tmp = w65c02s_mark_nz(cpu, w65c02s_stack_pull(cpu));
            switch (oper) {
                case W65C02S_OPER_PLP:
                    cpu->p = tmp;
                    w65c02s_irq_update_mask(cpu);
                    break;
                case W65C02S_OPER_PLA:
                    cpu->a = tmp;
                    break;
                case W65C02S_OPER_PLX:
                    cpu->x = tmp;
                    break;
                case W65C02S_OPER_PLY:
                    cpu->y = tmp;
                    break;
                default: W65C02S_UNREACHABLE();
            }
        }
    W65C02S_END_INSTRUCTION
}

W65C02S_INLINE unsigned w65c02s_mode_SUBROUTINE(W65C02S_PARAMS_MODE) {
    W65C02S_USE_TR(ea)
    /* op = JSR abs */
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            W65C02S_TR.ea = W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(2)
            W65C02S_READ(W65C02S_STACK_ADDR(cpu->s));
        W65C02S_CYCLE(3)
            w65c02s_stack_push(cpu, W65C02S_GET_HI(cpu->pc));
        W65C02S_CYCLE(4)
            w65c02s_stack_push(cpu, W65C02S_GET_LO(cpu->pc));
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(5)
            cpu->pc = (W65C02S_READ(cpu->pc) << 8) | W65C02S_TR.ea;
    W65C02S_END_INSTRUCTION
}

W65C02S_INLINE unsigned w65c02s_mode_RETURN_SUB(W65C02S_PARAMS_MODE) {
    /* op = RTS */
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            W65C02S_READ(cpu->pc);
        W65C02S_CYCLE(2)
            W65C02S_READ(W65C02S_STACK_ADDR(cpu->s));
        W65C02S_CYCLE(3)
            W65C02S_SET_LO(cpu->pc, w65c02s_stack_pull(cpu));
        W65C02S_CYCLE(4)
            W65C02S_SET_HI(cpu->pc, w65c02s_stack_pull(cpu));
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(5)
            W65C02S_READ(cpu->pc++);
    W65C02S_END_INSTRUCTION
}

W65C02S_INLINE unsigned w65c02s_mode_STACK_RTI(W65C02S_PARAMS_MODE) {
    /* op = RTI */
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            W65C02S_READ(cpu->pc);
        W65C02S_CYCLE(2)
            W65C02S_READ(W65C02S_STACK_ADDR(cpu->s));
        W65C02S_CYCLE(3)
            cpu->p = w65c02s_stack_pull(cpu);
            w65c02s_irq_update_mask(cpu);
        W65C02S_CYCLE(4)
            W65C02S_SET_LO(cpu->pc, w65c02s_stack_pull(cpu));
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(5)
            W65C02S_SET_HI(cpu->pc, w65c02s_stack_pull(cpu));
    W65C02S_END_INSTRUCTION
}

W65C02S_INLINE unsigned w65c02s_mode_STACK_BRK(W65C02S_PARAMS_MODE) {
    W65C02S_USE_TR(brk)
    /* op = BRK (possibly via NMI, IRQ or RESET) */
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
        {
            uint8_t tmp = W65C02S_READ(cpu->pc);
            /* is this instruction a true BRK or a hardware interrupt? */
            W65C02S_TR.is_brk = !(cpu->in_nmi || cpu->in_irq || cpu->in_rst);
            if (W65C02S_TR.is_brk) {
                ++cpu->pc;
#if W65C02S_HOOK_BRK
                if (cpu->hook_brk && (*cpu->hook_brk)(tmp)) W65C02S_SKIP_REST;
#else
                (void)tmp;
#endif
            }
        }
        W65C02S_CYCLE(2)
            if (cpu->in_rst) W65C02S_READ(W65C02S_STACK_ADDR(cpu->s--));
            else             w65c02s_stack_push(cpu, W65C02S_GET_HI(cpu->pc));
        W65C02S_CYCLE(3)
            if (cpu->in_rst) W65C02S_READ(W65C02S_STACK_ADDR(cpu->s--));
            else             w65c02s_stack_push(cpu, W65C02S_GET_LO(cpu->pc));
        W65C02S_CYCLE(4)
            if (cpu->in_rst) W65C02S_READ(W65C02S_STACK_ADDR(cpu->s--));
            else { /* B flag: 0 for NMI/IRQ, 1 for BRK */
                uint8_t p = cpu->p | W65C02S_P_A1 | W65C02S_P_B;
                if (!W65C02S_TR.is_brk) p &= ~W65C02S_P_B;
                w65c02s_stack_push(cpu, p);
            }
            /* the I flag is automatically set on any interrupt or BRK. */
            W65C02S_SET_P(cpu->p, W65C02S_P_I, 1);
            /* the D flag is too, even if not on NMOS 6502. */
            W65C02S_SET_P(cpu->p, W65C02S_P_D, 0);
            w65c02s_irq_update_mask(cpu);
        W65C02S_CYCLE(5)
            /* NMI replaces IRQ if one is triggered before this cycle.
               BRKs are apparently not affected by this */
            if ((cpu->int_trig & W65C02S_CPU_STATE_NMI) && cpu->in_irq) {
                W65C02S_CPU_STATE_CLEAR_IRQ(cpu);
                W65C02S_CPU_STATE_CLEAR_NMI(cpu);
                cpu->int_trig &= ~W65C02S_CPU_STATE_NMI;
                cpu->in_irq = false;
                cpu->in_nmi = true;
            }
            W65C02S_TR.ea = w65c02s_irq_select_vector(cpu);
            W65C02S_SET_LO(cpu->pc, W65C02S_READ(W65C02S_TR.ea++));
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(6)
            W65C02S_SET_HI(cpu->pc, W65C02S_READ(W65C02S_TR.ea));
        W65C02S_CYCLE(7)
            /* end instantly! this is a "ghost" cycle in a way. */
            /* HW interrupts do not increment the instruction counter */
            if (!W65C02S_TR.is_brk) --cpu->total_instructions;
#if !W65C02S_COARSE
            cpu->cycl = 0;
#endif
            W65C02S_SKIP_REST;
    W65C02S_END_INSTRUCTION
}

W65C02S_INLINE unsigned w65c02s_mode_NOP_5C(W65C02S_PARAMS_MODE) {
    W65C02S_USE_TR(ea)
    /* op = NOP $5C, which behaves oddly */
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            W65C02S_TR.ea = W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(2)
            W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(3)
            W65C02S_READ(0xFF00U | W65C02S_TR.ea);
        W65C02S_CYCLE(4)
            W65C02S_READ(0xFFFFU);
        W65C02S_CYCLE(5)
            W65C02S_READ(0xFFFFU);
        W65C02S_CYCLE(6)
            W65C02S_READ(0xFFFFU);
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(7)
            W65C02S_READ(0xFFFFU);
    W65C02S_END_INSTRUCTION
}

W65C02S_INLINE unsigned w65c02s_mode_INT_WAIT_STOP(W65C02S_PARAMS_MODE) {
    W65C02S_USE_TR(wai)
    /* op = WAI/STP */
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            W65C02S_READ(cpu->pc);
            /* STP (1) or WAI (0) */
            W65C02S_TR.is_stp = oper != W65C02S_OPER_WAI;
#if W65C02S_HOOK_STP
            if (W65C02S_TR.is_stp && cpu->hook_stp && (*cpu->hook_stp)())
                W65C02S_SKIP_REST;
#endif
        W65C02S_CYCLE(2)
            W65C02S_READ(cpu->pc);
        W65C02S_CYCLE(3)
            W65C02S_READ(cpu->pc);
            if (W65C02S_TR.is_stp) {
                W65C02S_CPU_STATE_INSERT(cpu->cpu_state,
                                         W65C02S_CPU_STATE_STOP);
            /* don't enter WAI if *any* interrupts are lined up */
            } else if (W65C02S_CPU_STATE_EXTRACT_WITH_INTS(cpu->cpu_state)
                        == W65C02S_CPU_STATE_RUN && !cpu->int_trig) {
                W65C02S_CPU_STATE_INSERT(cpu->cpu_state,
                                         W65C02S_CPU_STATE_WAIT);
            }
#if !W65C02S_COARSE
            /* end instruction immediately */
            cpu->cycl = 0;
#endif
    W65C02S_END_INSTRUCTION
}

W65C02S_INLINE unsigned w65c02s_mode_IMPLIED_1C(W65C02S_PARAMS_MODE) {
    (void)cpu;
    (void)oper;
#if W65C02S_COARSE
    return 1; /* spent 1 cycle doing nothing */
#else
    (void)cont;
    return 0; /* return immediately */
#endif
}



/* +------------------------------------------------------------------------+ */
/* |                                                                        | */
/* |                        OPCODE TABLE & EXECUTION                        | */
/* |                                                                        | */
/* +------------------------------------------------------------------------+ */



#define W65C02S_OPCODE_TABLE()                                                 \
W65C02S_OPCODE(0x00, STACK_BRK,                 W65C02S_OPER_BRK) /* BRK brk */\
W65C02S_OPCODE(0x01, ZEROPAGE_INDIRECT_X,       W65C02S_OPER_ORA) /* ORA zix */\
W65C02S_OPCODE(0x02, IMMEDIATE,                 W65C02S_OPER_NOP) /* NOP imm */\
W65C02S_OPCODE(0x03, IMPLIED_1C,                W65C02S_OPER_NOP) /* NOP im1 */\
W65C02S_OPCODE(0x04, RMW_ZEROPAGE,              W65C02S_OPER_TSB) /* TSB wzp */\
W65C02S_OPCODE(0x05, ZEROPAGE,                  W65C02S_OPER_ORA) /* ORA zpg */\
W65C02S_OPCODE(0x06, RMW_ZEROPAGE,              W65C02S_OPER_ASL) /* ASL wzp */\
W65C02S_OPCODE(0x07, ZEROPAGE_BIT,              000)              /* 000 zpb */\
W65C02S_OPCODE(0x08, STACK_PUSH,                W65C02S_OPER_PHP) /* PHP phv */\
W65C02S_OPCODE(0x09, IMMEDIATE,                 W65C02S_OPER_ORA) /* ORA imm */\
W65C02S_OPCODE(0x0A, IMPLIED,                   W65C02S_OPER_ASL) /* ASL imp */\
W65C02S_OPCODE(0x0B, IMPLIED_1C,                W65C02S_OPER_NOP) /* NOP im1 */\
W65C02S_OPCODE(0x0C, RMW_ABSOLUTE,              W65C02S_OPER_TSB) /* TSB wab */\
W65C02S_OPCODE(0x0D, ABSOLUTE,                  W65C02S_OPER_ORA) /* ORA abs */\
W65C02S_OPCODE(0x0E, RMW_ABSOLUTE,              W65C02S_OPER_ASL) /* ASL wab */\
W65C02S_OPCODE(0x0F, RELATIVE_BIT,              000)              /* 000 rlb */\
W65C02S_OPCODE(0x10, RELATIVE,                  W65C02S_OPER_BPL) /* BPL rel */\
W65C02S_OPCODE(0x11, ZEROPAGE_INDIRECT_Y,       W65C02S_OPER_ORA) /* ORA ziy */\
W65C02S_OPCODE(0x12, ZEROPAGE_INDIRECT,         W65C02S_OPER_ORA) /* ORA zpi */\
W65C02S_OPCODE(0x13, IMPLIED_1C,                W65C02S_OPER_NOP) /* NOP im1 */\
W65C02S_OPCODE(0x14, RMW_ZEROPAGE,              W65C02S_OPER_TRB) /* TRB wzp */\
W65C02S_OPCODE(0x15, ZEROPAGE_X,                W65C02S_OPER_ORA) /* ORA zpx */\
W65C02S_OPCODE(0x16, RMW_ZEROPAGE_X,            W65C02S_OPER_ASL) /* ASL wzx */\
W65C02S_OPCODE(0x17, ZEROPAGE_BIT,              001)              /* 001 zpb */\
W65C02S_OPCODE(0x18, IMPLIED,                   W65C02S_OPER_CLC) /* CLC imp */\
W65C02S_OPCODE(0x19, ABSOLUTE_Y,                W65C02S_OPER_ORA) /* ORA aby */\
W65C02S_OPCODE(0x1A, IMPLIED,                   W65C02S_OPER_INC) /* INC imp */\
W65C02S_OPCODE(0x1B, IMPLIED_1C,                W65C02S_OPER_NOP) /* NOP im1 */\
W65C02S_OPCODE(0x1C, RMW_ABSOLUTE,              W65C02S_OPER_TRB) /* TRB wab */\
W65C02S_OPCODE(0x1D, ABSOLUTE_X,                W65C02S_OPER_ORA) /* ORA abx */\
W65C02S_OPCODE(0x1E, RMW_ABSOLUTE_X,            W65C02S_OPER_ASL) /* ASL wax */\
W65C02S_OPCODE(0x1F, RELATIVE_BIT,              001)              /* 001 rlb */\
W65C02S_OPCODE(0x20, SUBROUTINE,                W65C02S_OPER_JSR) /* JSR sbj */\
W65C02S_OPCODE(0x21, ZEROPAGE_INDIRECT_X,       W65C02S_OPER_AND) /* AND zix */\
W65C02S_OPCODE(0x22, IMMEDIATE,                 W65C02S_OPER_NOP) /* NOP imm */\
W65C02S_OPCODE(0x23, IMPLIED_1C,                W65C02S_OPER_NOP) /* NOP im1 */\
W65C02S_OPCODE(0x24, ZEROPAGE,                  W65C02S_OPER_BIT) /* BIT zpg */\
W65C02S_OPCODE(0x25, ZEROPAGE,                  W65C02S_OPER_AND) /* AND zpg */\
W65C02S_OPCODE(0x26, RMW_ZEROPAGE,              W65C02S_OPER_ROL) /* ROL wzp */\
W65C02S_OPCODE(0x27, ZEROPAGE_BIT,              002)              /* 002 zpb */\
W65C02S_OPCODE(0x28, STACK_PULL,                W65C02S_OPER_PLP) /* PLP plv */\
W65C02S_OPCODE(0x29, IMMEDIATE,                 W65C02S_OPER_AND) /* AND imm */\
W65C02S_OPCODE(0x2A, IMPLIED,                   W65C02S_OPER_ROL) /* ROL imp */\
W65C02S_OPCODE(0x2B, IMPLIED_1C,                W65C02S_OPER_NOP) /* NOP im1 */\
W65C02S_OPCODE(0x2C, ABSOLUTE,                  W65C02S_OPER_BIT) /* BIT abs */\
W65C02S_OPCODE(0x2D, ABSOLUTE,                  W65C02S_OPER_AND) /* AND abs */\
W65C02S_OPCODE(0x2E, RMW_ABSOLUTE,              W65C02S_OPER_ROL) /* ROL wab */\
W65C02S_OPCODE(0x2F, RELATIVE_BIT,              002)              /* 002 rlb */\
W65C02S_OPCODE(0x30, RELATIVE,                  W65C02S_OPER_BMI) /* BMI rel */\
W65C02S_OPCODE(0x31, ZEROPAGE_INDIRECT_Y,       W65C02S_OPER_AND) /* AND ziy */\
W65C02S_OPCODE(0x32, ZEROPAGE_INDIRECT,         W65C02S_OPER_AND) /* AND zpi */\
W65C02S_OPCODE(0x33, IMPLIED_1C,                W65C02S_OPER_NOP) /* NOP im1 */\
W65C02S_OPCODE(0x34, ZEROPAGE_X,                W65C02S_OPER_BIT) /* BIT zpx */\
W65C02S_OPCODE(0x35, ZEROPAGE_X,                W65C02S_OPER_AND) /* AND zpx */\
W65C02S_OPCODE(0x36, RMW_ZEROPAGE_X,            W65C02S_OPER_ROL) /* ROL wzx */\
W65C02S_OPCODE(0x37, ZEROPAGE_BIT,              003)              /* 003 zpb */\
W65C02S_OPCODE(0x38, IMPLIED,                   W65C02S_OPER_SEC) /* SEC imp */\
W65C02S_OPCODE(0x39, ABSOLUTE_Y,                W65C02S_OPER_AND) /* AND aby */\
W65C02S_OPCODE(0x3A, IMPLIED,                   W65C02S_OPER_DEC) /* DEC imp */\
W65C02S_OPCODE(0x3B, IMPLIED_1C,                W65C02S_OPER_NOP) /* NOP im1 */\
W65C02S_OPCODE(0x3C, ABSOLUTE_X,                W65C02S_OPER_BIT) /* BIT abx */\
W65C02S_OPCODE(0x3D, ABSOLUTE_X,                W65C02S_OPER_AND) /* AND abx */\
W65C02S_OPCODE(0x3E, RMW_ABSOLUTE_X,            W65C02S_OPER_ROL) /* ROL wax */\
W65C02S_OPCODE(0x3F, RELATIVE_BIT,              003)              /* 003 rlb */\
W65C02S_OPCODE(0x40, STACK_RTI,                 W65C02S_OPER_RTI) /* RTI rti */\
W65C02S_OPCODE(0x41, ZEROPAGE_INDIRECT_X,       W65C02S_OPER_EOR) /* EOR zix */\
W65C02S_OPCODE(0x42, IMMEDIATE,                 W65C02S_OPER_NOP) /* NOP imm */\
W65C02S_OPCODE(0x43, IMPLIED_1C,                W65C02S_OPER_NOP) /* NOP im1 */\
W65C02S_OPCODE(0x44, ZEROPAGE,                  W65C02S_OPER_NOP) /* NOP zpg */\
W65C02S_OPCODE(0x45, ZEROPAGE,                  W65C02S_OPER_EOR) /* EOR zpg */\
W65C02S_OPCODE(0x46, RMW_ZEROPAGE,              W65C02S_OPER_LSR) /* LSR wzp */\
W65C02S_OPCODE(0x47, ZEROPAGE_BIT,              004)              /* 004 zpb */\
W65C02S_OPCODE(0x48, STACK_PUSH,                W65C02S_OPER_PHA) /* PHA phv */\
W65C02S_OPCODE(0x49, IMMEDIATE,                 W65C02S_OPER_EOR) /* EOR imm */\
W65C02S_OPCODE(0x4A, IMPLIED,                   W65C02S_OPER_LSR) /* LSR imp */\
W65C02S_OPCODE(0x4B, IMPLIED_1C,                W65C02S_OPER_NOP) /* NOP im1 */\
W65C02S_OPCODE(0x4C, ABSOLUTE_JUMP,             W65C02S_OPER_JMP) /* JMP abj */\
W65C02S_OPCODE(0x4D, ABSOLUTE,                  W65C02S_OPER_EOR) /* EOR abs */\
W65C02S_OPCODE(0x4E, RMW_ABSOLUTE,              W65C02S_OPER_LSR) /* LSR wab */\
W65C02S_OPCODE(0x4F, RELATIVE_BIT,              004)              /* 004 rlb */\
W65C02S_OPCODE(0x50, RELATIVE,                  W65C02S_OPER_BVC) /* BVC rel */\
W65C02S_OPCODE(0x51, ZEROPAGE_INDIRECT_Y,       W65C02S_OPER_EOR) /* EOR ziy */\
W65C02S_OPCODE(0x52, ZEROPAGE_INDIRECT,         W65C02S_OPER_EOR) /* EOR zpi */\
W65C02S_OPCODE(0x53, IMPLIED_1C,                W65C02S_OPER_NOP) /* NOP im1 */\
W65C02S_OPCODE(0x54, ZEROPAGE_X,                W65C02S_OPER_NOP) /* NOP zpx */\
W65C02S_OPCODE(0x55, ZEROPAGE_X,                W65C02S_OPER_EOR) /* EOR zpx */\
W65C02S_OPCODE(0x56, RMW_ZEROPAGE_X,            W65C02S_OPER_LSR) /* LSR wzx */\
W65C02S_OPCODE(0x57, ZEROPAGE_BIT,              005)              /* 005 zpb */\
W65C02S_OPCODE(0x58, IMPLIED,                   W65C02S_OPER_CLI) /* CLI imp */\
W65C02S_OPCODE(0x59, ABSOLUTE_Y,                W65C02S_OPER_EOR) /* EOR aby */\
W65C02S_OPCODE(0x5A, STACK_PUSH,                W65C02S_OPER_PHY) /* PHY phv */\
W65C02S_OPCODE(0x5B, IMPLIED_1C,                W65C02S_OPER_NOP) /* NOP im1 */\
W65C02S_OPCODE(0x5C, NOP_5C,                    W65C02S_OPER_NOP) /* NOP x5c */\
W65C02S_OPCODE(0x5D, ABSOLUTE_X,                W65C02S_OPER_EOR) /* EOR abx */\
W65C02S_OPCODE(0x5E, RMW_ABSOLUTE_X,            W65C02S_OPER_LSR) /* LSR wax */\
W65C02S_OPCODE(0x5F, RELATIVE_BIT,              005)              /* 005 rlb */\
W65C02S_OPCODE(0x60, RETURN_SUB,                W65C02S_OPER_RTS) /* RTS sbr */\
W65C02S_OPCODE(0x61, ZEROPAGE_INDIRECT_X,       W65C02S_OPER_ADC) /* ADC zix */\
W65C02S_OPCODE(0x62, IMMEDIATE,                 W65C02S_OPER_NOP) /* NOP imm */\
W65C02S_OPCODE(0x63, IMPLIED_1C,                W65C02S_OPER_NOP) /* NOP im1 */\
W65C02S_OPCODE(0x64, ZEROPAGE,                  W65C02S_OPER_STZ) /* STZ zpg */\
W65C02S_OPCODE(0x65, ZEROPAGE,                  W65C02S_OPER_ADC) /* ADC zpg */\
W65C02S_OPCODE(0x66, RMW_ZEROPAGE,              W65C02S_OPER_ROR) /* ROR wzp */\
W65C02S_OPCODE(0x67, ZEROPAGE_BIT,              006)              /* 006 zpb */\
W65C02S_OPCODE(0x68, STACK_PULL,                W65C02S_OPER_PLA) /* PLA plv */\
W65C02S_OPCODE(0x69, IMMEDIATE,                 W65C02S_OPER_ADC) /* ADC imm */\
W65C02S_OPCODE(0x6A, IMPLIED,                   W65C02S_OPER_ROR) /* ROR imp */\
W65C02S_OPCODE(0x6B, IMPLIED_1C,                W65C02S_OPER_NOP) /* NOP im1 */\
W65C02S_OPCODE(0x6C, ABSOLUTE_INDIRECT,         W65C02S_OPER_JMP) /* JMP ind */\
W65C02S_OPCODE(0x6D, ABSOLUTE,                  W65C02S_OPER_ADC) /* ADC abs */\
W65C02S_OPCODE(0x6E, RMW_ABSOLUTE,              W65C02S_OPER_ROR) /* ROR wab */\
W65C02S_OPCODE(0x6F, RELATIVE_BIT,              006)              /* 006 rlb */\
W65C02S_OPCODE(0x70, RELATIVE,                  W65C02S_OPER_BVS) /* BVS rel */\
W65C02S_OPCODE(0x71, ZEROPAGE_INDIRECT_Y,       W65C02S_OPER_ADC) /* ADC ziy */\
W65C02S_OPCODE(0x72, ZEROPAGE_INDIRECT,         W65C02S_OPER_ADC) /* ADC zpi */\
W65C02S_OPCODE(0x73, IMPLIED_1C,                W65C02S_OPER_NOP) /* NOP im1 */\
W65C02S_OPCODE(0x74, ZEROPAGE_X,                W65C02S_OPER_STZ) /* STZ zpx */\
W65C02S_OPCODE(0x75, ZEROPAGE_X,                W65C02S_OPER_ADC) /* ADC zpx */\
W65C02S_OPCODE(0x76, RMW_ZEROPAGE_X,            W65C02S_OPER_ROR) /* ROR wzx */\
W65C02S_OPCODE(0x77, ZEROPAGE_BIT,              007)              /* 007 zpb */\
W65C02S_OPCODE(0x78, IMPLIED,                   W65C02S_OPER_SEI) /* SEI imp */\
W65C02S_OPCODE(0x79, ABSOLUTE_Y,                W65C02S_OPER_ADC) /* ADC aby */\
W65C02S_OPCODE(0x7A, STACK_PULL,                W65C02S_OPER_PLY) /* PLY plv */\
W65C02S_OPCODE(0x7B, IMPLIED_1C,                W65C02S_OPER_NOP) /* NOP im1 */\
W65C02S_OPCODE(0x7C, ABSOLUTE_INDIRECT_X,       W65C02S_OPER_JMP) /* JMP idx */\
W65C02S_OPCODE(0x7D, ABSOLUTE_X,                W65C02S_OPER_ADC) /* ADC abx */\
W65C02S_OPCODE(0x7E, RMW_ABSOLUTE_X,            W65C02S_OPER_ROR) /* ROR wax */\
W65C02S_OPCODE(0x7F, RELATIVE_BIT,              007)              /* 007 rlb */\
W65C02S_OPCODE(0x80, RELATIVE,                  W65C02S_OPER_BRA) /* BRA rel */\
W65C02S_OPCODE(0x81, ZEROPAGE_INDIRECT_X,       W65C02S_OPER_STA) /* STA zix */\
W65C02S_OPCODE(0x82, IMMEDIATE,                 W65C02S_OPER_NOP) /* NOP imm */\
W65C02S_OPCODE(0x83, IMPLIED_1C,                W65C02S_OPER_NOP) /* NOP im1 */\
W65C02S_OPCODE(0x84, ZEROPAGE,                  W65C02S_OPER_STY) /* STY zpg */\
W65C02S_OPCODE(0x85, ZEROPAGE,                  W65C02S_OPER_STA) /* STA zpg */\
W65C02S_OPCODE(0x86, ZEROPAGE,                  W65C02S_OPER_STX) /* STX zpg */\
W65C02S_OPCODE(0x87, ZEROPAGE_BIT,              010)              /* 010 zpb */\
W65C02S_OPCODE(0x88, IMPLIED_Y,                 W65C02S_OPER_DEC) /* DEC imy */\
W65C02S_OPCODE(0x89, IMMEDIATE,                 W65C02S_OPER_BIT) /* BIT imm */\
W65C02S_OPCODE(0x8A, IMPLIED,                   W65C02S_OPER_TXA) /* TXA imp */\
W65C02S_OPCODE(0x8B, IMPLIED_1C,                W65C02S_OPER_NOP) /* NOP im1 */\
W65C02S_OPCODE(0x8C, ABSOLUTE,                  W65C02S_OPER_STY) /* STY abs */\
W65C02S_OPCODE(0x8D, ABSOLUTE,                  W65C02S_OPER_STA) /* STA abs */\
W65C02S_OPCODE(0x8E, ABSOLUTE,                  W65C02S_OPER_STX) /* STX abs */\
W65C02S_OPCODE(0x8F, RELATIVE_BIT,              010)              /* 010 rlb */\
W65C02S_OPCODE(0x90, RELATIVE,                  W65C02S_OPER_BCC) /* BCC rel */\
W65C02S_OPCODE(0x91, ZEROPAGE_INDIRECT_Y_STORE, W65C02S_OPER_STA) /* STA ziy */\
W65C02S_OPCODE(0x92, ZEROPAGE_INDIRECT,         W65C02S_OPER_STA) /* STA zpi */\
W65C02S_OPCODE(0x93, IMPLIED_1C,                W65C02S_OPER_NOP) /* NOP im1 */\
W65C02S_OPCODE(0x94, ZEROPAGE_X,                W65C02S_OPER_STY) /* STY zpx */\
W65C02S_OPCODE(0x95, ZEROPAGE_X,                W65C02S_OPER_STA) /* STA zpx */\
W65C02S_OPCODE(0x96, ZEROPAGE_Y,                W65C02S_OPER_STX) /* STX zpy */\
W65C02S_OPCODE(0x97, ZEROPAGE_BIT,              011)              /* 011 zpb */\
W65C02S_OPCODE(0x98, IMPLIED,                   W65C02S_OPER_TYA) /* TYA imp */\
W65C02S_OPCODE(0x99, ABSOLUTE_Y_STORE,          W65C02S_OPER_STA) /* STA aby */\
W65C02S_OPCODE(0x9A, IMPLIED,                   W65C02S_OPER_TXS) /* TXS imp */\
W65C02S_OPCODE(0x9B, IMPLIED_1C,                W65C02S_OPER_NOP) /* NOP im1 */\
W65C02S_OPCODE(0x9C, ABSOLUTE,                  W65C02S_OPER_STZ) /* STZ abs */\
W65C02S_OPCODE(0x9D, ABSOLUTE_X_STORE,          W65C02S_OPER_STA) /* STA abx */\
W65C02S_OPCODE(0x9E, ABSOLUTE_X_STORE,          W65C02S_OPER_STZ) /* STZ abx */\
W65C02S_OPCODE(0x9F, RELATIVE_BIT,              011)              /* 011 rlb */\
W65C02S_OPCODE(0xA0, IMMEDIATE,                 W65C02S_OPER_LDY) /* LDY imm */\
W65C02S_OPCODE(0xA1, ZEROPAGE_INDIRECT_X,       W65C02S_OPER_LDA) /* LDA zix */\
W65C02S_OPCODE(0xA2, IMMEDIATE,                 W65C02S_OPER_LDX) /* LDX imm */\
W65C02S_OPCODE(0xA3, IMPLIED_1C,                W65C02S_OPER_NOP) /* NOP im1 */\
W65C02S_OPCODE(0xA4, ZEROPAGE,                  W65C02S_OPER_LDY) /* LDY zpg */\
W65C02S_OPCODE(0xA5, ZEROPAGE,                  W65C02S_OPER_LDA) /* LDA zpg */\
W65C02S_OPCODE(0xA6, ZEROPAGE,                  W65C02S_OPER_LDX) /* LDX zpg */\
W65C02S_OPCODE(0xA7, ZEROPAGE_BIT,              012)              /* 012 zpb */\
W65C02S_OPCODE(0xA8, IMPLIED,                   W65C02S_OPER_TAY) /* TAY imp */\
W65C02S_OPCODE(0xA9, IMMEDIATE,                 W65C02S_OPER_LDA) /* LDA imm */\
W65C02S_OPCODE(0xAA, IMPLIED,                   W65C02S_OPER_TAX) /* TAX imp */\
W65C02S_OPCODE(0xAB, IMPLIED_1C,                W65C02S_OPER_NOP) /* NOP im1 */\
W65C02S_OPCODE(0xAC, ABSOLUTE,                  W65C02S_OPER_LDY) /* LDY abs */\
W65C02S_OPCODE(0xAD, ABSOLUTE,                  W65C02S_OPER_LDA) /* LDA abs */\
W65C02S_OPCODE(0xAE, ABSOLUTE,                  W65C02S_OPER_LDX) /* LDX abs */\
W65C02S_OPCODE(0xAF, RELATIVE_BIT,              012)              /* 012 rlb */\
W65C02S_OPCODE(0xB0, RELATIVE,                  W65C02S_OPER_BCS) /* BCS rel */\
W65C02S_OPCODE(0xB1, ZEROPAGE_INDIRECT_Y,       W65C02S_OPER_LDA) /* LDA ziy */\
W65C02S_OPCODE(0xB2, ZEROPAGE_INDIRECT,         W65C02S_OPER_LDA) /* LDA zpi */\
W65C02S_OPCODE(0xB3, IMPLIED_1C,                W65C02S_OPER_NOP) /* NOP im1 */\
W65C02S_OPCODE(0xB4, ZEROPAGE_X,                W65C02S_OPER_LDY) /* LDY zpx */\
W65C02S_OPCODE(0xB5, ZEROPAGE_X,                W65C02S_OPER_LDA) /* LDA zpx */\
W65C02S_OPCODE(0xB6, ZEROPAGE_Y,                W65C02S_OPER_LDX) /* LDX zpy */\
W65C02S_OPCODE(0xB7, ZEROPAGE_BIT,              013)              /* 013 zpb */\
W65C02S_OPCODE(0xB8, IMPLIED,                   W65C02S_OPER_CLV) /* CLV imp */\
W65C02S_OPCODE(0xB9, ABSOLUTE_Y,                W65C02S_OPER_LDA) /* LDA aby */\
W65C02S_OPCODE(0xBA, IMPLIED,                   W65C02S_OPER_TSX) /* TSX imp */\
W65C02S_OPCODE(0xBB, IMPLIED_1C,                W65C02S_OPER_NOP) /* NOP im1 */\
W65C02S_OPCODE(0xBC, ABSOLUTE_X,                W65C02S_OPER_LDY) /* LDY abx */\
W65C02S_OPCODE(0xBD, ABSOLUTE_X,                W65C02S_OPER_LDA) /* LDA abx */\
W65C02S_OPCODE(0xBE, ABSOLUTE_Y,                W65C02S_OPER_LDX) /* LDX aby */\
W65C02S_OPCODE(0xBF, RELATIVE_BIT,              013)              /* 013 rlb */\
W65C02S_OPCODE(0xC0, IMMEDIATE,                 W65C02S_OPER_CPY) /* CPY imm */\
W65C02S_OPCODE(0xC1, ZEROPAGE_INDIRECT_X,       W65C02S_OPER_CMP) /* CMP zix */\
W65C02S_OPCODE(0xC2, IMMEDIATE,                 W65C02S_OPER_NOP) /* NOP imm */\
W65C02S_OPCODE(0xC3, IMPLIED_1C,                W65C02S_OPER_NOP) /* NOP im1 */\
W65C02S_OPCODE(0xC4, ZEROPAGE,                  W65C02S_OPER_CPY) /* CPY zpg */\
W65C02S_OPCODE(0xC5, ZEROPAGE,                  W65C02S_OPER_CMP) /* CMP zpg */\
W65C02S_OPCODE(0xC6, RMW_ZEROPAGE,              W65C02S_OPER_DEC) /* DEC wzp */\
W65C02S_OPCODE(0xC7, ZEROPAGE_BIT,              014)              /* 014 zpb */\
W65C02S_OPCODE(0xC8, IMPLIED_Y,                 W65C02S_OPER_INC) /* INC imy */\
W65C02S_OPCODE(0xC9, IMMEDIATE,                 W65C02S_OPER_CMP) /* CMP imm */\
W65C02S_OPCODE(0xCA, IMPLIED_X,                 W65C02S_OPER_DEC) /* DEC imx */\
W65C02S_OPCODE(0xCB, INT_WAIT_STOP,             W65C02S_OPER_WAI) /* WAI wai */\
W65C02S_OPCODE(0xCC, ABSOLUTE,                  W65C02S_OPER_CPY) /* CPY abs */\
W65C02S_OPCODE(0xCD, ABSOLUTE,                  W65C02S_OPER_CMP) /* CMP abs */\
W65C02S_OPCODE(0xCE, RMW_ABSOLUTE,              W65C02S_OPER_DEC) /* DEC wab */\
W65C02S_OPCODE(0xCF, RELATIVE_BIT,              014)              /* 014 rlb */\
W65C02S_OPCODE(0xD0, RELATIVE,                  W65C02S_OPER_BNE) /* BNE rel */\
W65C02S_OPCODE(0xD1, ZEROPAGE_INDIRECT_Y,       W65C02S_OPER_CMP) /* CMP ziy */\
W65C02S_OPCODE(0xD2, ZEROPAGE_INDIRECT,         W65C02S_OPER_CMP) /* CMP zpi */\
W65C02S_OPCODE(0xD3, IMPLIED_1C,                W65C02S_OPER_NOP) /* NOP im1 */\
W65C02S_OPCODE(0xD4, ZEROPAGE_X,                W65C02S_OPER_NOP) /* NOP zpx */\
W65C02S_OPCODE(0xD5, ZEROPAGE_X,                W65C02S_OPER_CMP) /* CMP zpx */\
W65C02S_OPCODE(0xD6, RMW_ZEROPAGE_X,            W65C02S_OPER_DEC) /* DEC wzx */\
W65C02S_OPCODE(0xD7, ZEROPAGE_BIT,              015)              /* 015 zpb */\
W65C02S_OPCODE(0xD8, IMPLIED,                   W65C02S_OPER_CLD) /* CLD imp */\
W65C02S_OPCODE(0xD9, ABSOLUTE_Y,                W65C02S_OPER_CMP) /* CMP aby */\
W65C02S_OPCODE(0xDA, STACK_PUSH,                W65C02S_OPER_PHX) /* PHX phv */\
W65C02S_OPCODE(0xDB, INT_WAIT_STOP,             W65C02S_OPER_STP) /* STP wai */\
W65C02S_OPCODE(0xDC, ABSOLUTE,                  W65C02S_OPER_NOP) /* NOP abs */\
W65C02S_OPCODE(0xDD, ABSOLUTE_X,                W65C02S_OPER_CMP) /* CMP abx */\
W65C02S_OPCODE(0xDE, RMW_ABSOLUTE_X,            W65C02S_OPER_DEC) /* DEC wax */\
W65C02S_OPCODE(0xDF, RELATIVE_BIT,              015)              /* 015 rlb */\
W65C02S_OPCODE(0xE0, IMMEDIATE,                 W65C02S_OPER_CPX) /* CPX imm */\
W65C02S_OPCODE(0xE1, ZEROPAGE_INDIRECT_X,       W65C02S_OPER_SBC) /* SBC zix */\
W65C02S_OPCODE(0xE2, IMMEDIATE,                 W65C02S_OPER_NOP) /* NOP imm */\
W65C02S_OPCODE(0xE3, IMPLIED_1C,                W65C02S_OPER_NOP) /* NOP im1 */\
W65C02S_OPCODE(0xE4, ZEROPAGE,                  W65C02S_OPER_CPX) /* CPX zpg */\
W65C02S_OPCODE(0xE5, ZEROPAGE,                  W65C02S_OPER_SBC) /* SBC zpg */\
W65C02S_OPCODE(0xE6, RMW_ZEROPAGE,              W65C02S_OPER_INC) /* INC wzp */\
W65C02S_OPCODE(0xE7, ZEROPAGE_BIT,              016)              /* 016 zpb */\
W65C02S_OPCODE(0xE8, IMPLIED_X,                 W65C02S_OPER_INC) /* INC imx */\
W65C02S_OPCODE(0xE9, IMMEDIATE,                 W65C02S_OPER_SBC) /* SBC imm */\
W65C02S_OPCODE(0xEA, IMPLIED,                   W65C02S_OPER_NOP) /* NOP imp */\
W65C02S_OPCODE(0xEB, IMPLIED_1C,                W65C02S_OPER_NOP) /* NOP im1 */\
W65C02S_OPCODE(0xEC, ABSOLUTE,                  W65C02S_OPER_CPX) /* CPX abs */\
W65C02S_OPCODE(0xED, ABSOLUTE,                  W65C02S_OPER_SBC) /* SBC abs */\
W65C02S_OPCODE(0xEE, RMW_ABSOLUTE,              W65C02S_OPER_INC) /* INC wab */\
W65C02S_OPCODE(0xEF, RELATIVE_BIT,              016)              /* 016 rlb */\
W65C02S_OPCODE(0xF0, RELATIVE,                  W65C02S_OPER_BEQ) /* BEQ rel */\
W65C02S_OPCODE(0xF1, ZEROPAGE_INDIRECT_Y,       W65C02S_OPER_SBC) /* SBC ziy */\
W65C02S_OPCODE(0xF2, ZEROPAGE_INDIRECT,         W65C02S_OPER_SBC) /* SBC zpi */\
W65C02S_OPCODE(0xF3, IMPLIED_1C,                W65C02S_OPER_NOP) /* NOP im1 */\
W65C02S_OPCODE(0xF4, ZEROPAGE_X,                W65C02S_OPER_NOP) /* NOP zpx */\
W65C02S_OPCODE(0xF5, ZEROPAGE_X,                W65C02S_OPER_SBC) /* SBC zpx */\
W65C02S_OPCODE(0xF6, RMW_ZEROPAGE_X,            W65C02S_OPER_INC) /* INC wzx */\
W65C02S_OPCODE(0xF7, ZEROPAGE_BIT,              017)              /* 017 zpb */\
W65C02S_OPCODE(0xF8, IMPLIED,                   W65C02S_OPER_SED) /* SED imp */\
W65C02S_OPCODE(0xF9, ABSOLUTE_Y,                W65C02S_OPER_SBC) /* SBC aby */\
W65C02S_OPCODE(0xFA, STACK_PULL,                W65C02S_OPER_PLX) /* PLX plv */\
W65C02S_OPCODE(0xFB, IMPLIED_1C,                W65C02S_OPER_NOP) /* NOP im1 */\
W65C02S_OPCODE(0xFC, ABSOLUTE,                  W65C02S_OPER_NOP) /* NOP abs */\
W65C02S_OPCODE(0xFD, ABSOLUTE_X,                W65C02S_OPER_SBC) /* SBC abx */\
W65C02S_OPCODE(0xFE, RMW_ABSOLUTE_X,            W65C02S_OPER_INC) /* INC wax */\
W65C02S_OPCODE(0xFF, RELATIVE_BIT,              017)              /* 017 rlb */


#if !W65C02S_COARSE
static void w65c02s_prerun_mode(struct w65c02s_cpu *cpu, uint8_t ir) {
    unsigned mode;

    switch (ir) {
        default: W65C02S_UNREACHABLE();
#define W65C02S_OPCODE(opcode, o_mode, o_oper)                                 \
        case opcode: mode = W65C02S_MODE_ ## o_mode; break;
W65C02S_OPCODE_TABLE()
#undef W65C02S_OPCODE
    }

    switch (mode) {
    /* these are all the two-cycle addressing modes.
       latch interrupts on the cycle after reading the opcode */
    case W65C02S_MODE_IMPLIED:
    case W65C02S_MODE_IMPLIED_X:
    case W65C02S_MODE_IMPLIED_Y:
    case W65C02S_MODE_IMMEDIATE:
    case W65C02S_MODE_RELATIVE:
        w65c02s_irq_latch(cpu);
    }
}
#endif

/* COARSE=0: return true if there is still more to run, false if not */
/* COARSE=1: return number of cycles */
#if W65C02S_COARSE
static unsigned w65c02s_run_op(struct w65c02s_cpu *cpu, uint8_t ir)
#else
static unsigned w65c02s_run_op(struct w65c02s_cpu *cpu, uint8_t ir, bool cont)
#endif
                                                                     {
    switch (ir) {
#define W65C02S_OPCODE(opcode, o_mode, o_oper)                                 \
        case opcode: W65C02S_GO_MODE(o_mode, o_oper);
W65C02S_OPCODE_TABLE()
#undef W65C02S_OPCODE
    }
    W65C02S_UNREACHABLE();
    return 0;
}



/* +------------------------------------------------------------------------+ */
/* |                                                                        | */
/* |                             EXECUTION LOOP                             | */
/* |                                                                        | */
/* +------------------------------------------------------------------------+ */



static void w65c02s_handle_reset(struct w65c02s_cpu *cpu) {
    uint8_t p = cpu->p;
    /* do RESET */
    cpu->in_rst = true;
    cpu->in_nmi = cpu->in_irq = false;
    W65C02S_CPU_STATE_INSERT(cpu->cpu_state, W65C02S_CPU_STATE_RUN);
    W65C02S_CPU_STATE_CLEAR_RESET(cpu);
    W65C02S_CPU_STATE_CLEAR_NMI(cpu);
    W65C02S_CPU_STATE_CLEAR_IRQ(cpu);
    W65C02S_SET_P(p, W65C02S_P_A1, 1);
    W65C02S_SET_P(p, W65C02S_P_B, 1);
    cpu->p = p;
}

static void w65c02s_handle_nmi(struct w65c02s_cpu *cpu) {
    cpu->in_nmi = true;
    cpu->int_trig &= ~W65C02S_CPU_STATE_NMI;
    W65C02S_CPU_STATE_CLEAR_NMI(cpu);
}

static void w65c02s_handle_irq(struct w65c02s_cpu *cpu) {
    cpu->in_irq = true;
    W65C02S_CPU_STATE_CLEAR_IRQ(cpu);
}

W65C02S_INLINE bool w65c02s_handle_interrupt(struct w65c02s_cpu *cpu) {
    if (W65C02S_CPU_STATE_HAS_RESET(cpu))
        w65c02s_handle_reset(cpu);
    else if (W65C02S_CPU_STATE_HAS_NMI(cpu))
        w65c02s_handle_nmi(cpu);
    else if (W65C02S_CPU_STATE_HAS_IRQ(cpu))
        w65c02s_handle_irq(cpu);
    else
        return false;
    W65C02S_READ(cpu->pc); /* stall for a cycle */
    return true;
}

W65C02S_INLINE void w65c02s_handle_end_of_instruction(struct w65c02s_cpu *cpu) {
    /* increment instruction tally */
    ++cpu->total_instructions;
#if W65C02S_HOOK_EOI
    if (cpu->hook_eoi) (cpu->hook_eoi)();
#endif
}

#define W65C02S_SPENT_CYCLE             ++cpu->total_cycles

#define W65C02S_STARTING_INSTRUCTION false
#define W65C02S_CONTINUE_INSTRUCTION true

static bool w65c02s_handle_break(struct w65c02s_cpu *cpu) {
    return W65C02S_CPU_STATE_HAS_BREAK(cpu);
}

static bool w65c02s_handle_stp_wai_i(struct w65c02s_cpu *cpu) {
    switch (W65C02S_CPU_STATE_EXTRACT(cpu->cpu_state)) {
        case W65C02S_CPU_STATE_WAIT:
            /* if there is an IRQ or NMI, latch it immediately and continue */
            if (cpu->int_trig) {
                w65c02s_irq_latch_slow(cpu);
                W65C02S_CPU_STATE_INSERT(cpu->cpu_state, W65C02S_CPU_STATE_RUN);
                return false;
            }
        case W65C02S_CPU_STATE_STOP:
            if (W65C02S_CPU_STATE_HAS_RESET(cpu))
                return false;
            /* spurious read to waste a cycle */
            W65C02S_READ(cpu->pc); /* stall for a cycle */
            W65C02S_SPENT_CYCLE;
            return true;
    }
    return false;
}

#if !W65C02S_COARSE

static bool w65c02s_handle_stp_wai_c(struct w65c02s_cpu *cpu) {
    switch (W65C02S_CPU_STATE_EXTRACT(cpu->cpu_state)) {
        case W65C02S_CPU_STATE_WAIT:
            for (;;) {
                if (W65C02S_CPU_STATE_HAS_BREAK(cpu)) {
                    return true;
                } else if (W65C02S_CPU_STATE_HAS_RESET(cpu)) {
                    return false;
                } else if (cpu->int_trig) {
                    w65c02s_irq_latch_slow(cpu);
                    W65C02S_CPU_STATE_INSERT(cpu->cpu_state,
                                             W65C02S_CPU_STATE_RUN);
                    return false;
                }
                W65C02S_READ(cpu->pc); /* stall for a cycle */
                if (W65C02S_CYCLE_CONDITION) return true;
            }
        case W65C02S_CPU_STATE_STOP:
            for (;;) {
                if (W65C02S_CPU_STATE_HAS_BREAK(cpu)) {
                    return true;
                } else if (W65C02S_CPU_STATE_HAS_RESET(cpu))
                    return false;
                W65C02S_READ(cpu->pc); /* stall for a cycle */
                if (W65C02S_CYCLE_CONDITION) return true;
            }
    }
    return false;
}

static unsigned long w65c02s_execute_c(struct w65c02s_cpu *cpu,
                                       unsigned long maximum_cycles) {
    uint8_t ir;
    cpu->maximum_cycles = maximum_cycles;
    cpu->target_cycles = cpu->total_cycles + maximum_cycles;
    if (cpu->cycl) {
        /* continue running instruction */
        ir = cpu->ir;
        if (w65c02s_run_op(cpu, ir, W65C02S_CONTINUE_INSTRUCTION)) {
            maximum_cycles = cpu->maximum_cycles;
            if (cpu->cycl) cpu->cycl += maximum_cycles;
            return maximum_cycles;
        }
        goto end_of_instruction;
    }

    /* new instruction, handle special states now */
    if (W65C02S_UNLIKELY(cpu->cpu_state != W65C02S_CPU_STATE_RUN)) {
        ir = cpu->ir;
        goto check_special_state;
    }

    for (;;) {
        unsigned long cyclecount;
        ir = W65C02S_READ(cpu->pc++);

decoded:
        cpu->cycl = 0;
        cyclecount = cpu->total_cycles;
        if (W65C02S_UNLIKELY(W65C02S_CYCLE_CONDITION)) {
            cpu->cycl = 1; /* stopped after decoding, continue from there */
            w65c02s_prerun_mode(cpu, ir);
            cpu->ir = ir;
            return cpu->maximum_cycles;
        }

        if (W65C02S_UNLIKELY(w65c02s_run_op(cpu, ir,
                             W65C02S_STARTING_INSTRUCTION))) {
            if (cpu->cycl) {
                cpu->cycl += cpu->total_cycles - cyclecount;
                cpu->ir = ir;
            } else {
                w65c02s_handle_end_of_instruction(cpu);
            }
            return cpu->maximum_cycles;
        }

#define W65C02S_CYCLES_NOW \
    (cpu->maximum_cycles - (cpu->target_cycles - cpu->total_cycles))
end_of_instruction:
        w65c02s_handle_end_of_instruction(cpu);
        if (W65C02S_UNLIKELY(cpu->cpu_state != W65C02S_CPU_STATE_RUN)) {
check_special_state:
            if (w65c02s_handle_break(cpu) || w65c02s_handle_stp_wai_c(cpu)) {
                cpu->ir = ir;
                return W65C02S_CYCLES_NOW;
            }
            if (w65c02s_handle_interrupt(cpu)) {
                ir = 0; /* force BRK ($00) to handle interrupt */
                goto decoded;
            }
        }
    }
}

W65C02S_INLINE unsigned w65c02s_run_op_c(struct w65c02s_cpu *cpu, uint8_t ir,
                                           bool cont) {
    cpu->target_cycles = cpu->total_cycles - (cont ? 0 : 1);
    w65c02s_run_op(cpu, ir, cont);
    return cpu->total_cycles - cpu->target_cycles;
}

static unsigned long w65c02s_execute_ic(struct w65c02s_cpu *cpu) {
    unsigned cycles;
    cycles = w65c02s_run_op_c(cpu, cpu->ir, W65C02S_CONTINUE_INSTRUCTION);
    w65c02s_handle_end_of_instruction(cpu);
    return cycles;
}

#endif /* W65C02S_COARSE */

static unsigned long w65c02s_execute_i(struct w65c02s_cpu *cpu) {
    unsigned cycles;
    uint8_t ir;

    if (W65C02S_UNLIKELY(cpu->cpu_state != W65C02S_CPU_STATE_RUN)) {
        if (w65c02s_handle_break(cpu)) return 0;
        if (w65c02s_handle_stp_wai_i(cpu)) return 1;
        if (w65c02s_handle_interrupt(cpu)) {
            ir = 0; /* force BRK ($00) to handle interrupt */
            goto decoded;
        }
    }

    ir = W65C02S_READ(cpu->pc++);
decoded:
    W65C02S_SPENT_CYCLE;

#if !W65C02S_COARSE
    cpu->cycl = 1; /* make sure we start at the first cycle */
    cycles = w65c02s_run_op_c(cpu, ir, W65C02S_STARTING_INSTRUCTION);
#else
    cycles = w65c02s_run_op(cpu, ir);
#endif
    w65c02s_handle_end_of_instruction(cpu);
    W65C02S_ASSUME(cycles > 0);
    return cycles;
}

#if W65C02S_COARSE

static unsigned long w65c02s_execute_ix(struct w65c02s_cpu *cpu,
                                        unsigned long cycles) {
    unsigned long c = 0;
    /* we may overflow otherwise */
    if (cycles > ULONG_MAX - 8) cycles = ULONG_MAX - 8;
    while (c < cycles) {
        unsigned ic = w65c02s_execute_i(cpu);
        if (W65C02S_UNLIKELY(!ic)) break; /* w65c02s_break() */
        c += ic;
    }
    return c;
}

#endif /* W65C02S_COARSE */

static unsigned long w65c02s_execute_im(struct w65c02s_cpu *cpu,
                                        unsigned long instructions) {
    unsigned long c = 0;
    while (instructions--) {
        c += w65c02s_execute_i(cpu);
    }
    return c;
}



/* +------------------------------------------------------------------------+ */
/* |                                                                        | */
/* |                            MAIN  INTERFACE                             | */
/* |                                                                        | */
/* +------------------------------------------------------------------------+ */



#if !W65C02S_LINK
static uint8_t w65c02s_openbus_read(struct w65c02s_cpu *cpu,
                                    uint16_t addr) {
    return 0xFF;
}

static void w65c02s_openbus_write(struct w65c02s_cpu *cpu,
                                  uint16_t addr, uint8_t value) { }
#endif

void w65c02s_init(struct w65c02s_cpu *cpu,
                  uint8_t (*mem_read)(struct w65c02s_cpu *, uint16_t),
                  void (*mem_write)(struct w65c02s_cpu *, uint16_t, uint8_t),
                  void *cpu_data) {
    cpu->total_cycles = cpu->total_instructions = 0;
#if !W65C02S_COARSE
    cpu->cycl = 0;
#endif
    cpu->int_trig = 0;
    cpu->in_nmi = cpu->in_rst = cpu->in_irq = 0;

#if !W65C02S_LINK
    cpu->mem_read = mem_read ? mem_read : &w65c02s_openbus_read;
    cpu->mem_write = mem_write ? mem_write : &w65c02s_openbus_write;
#else
    (void)mem_read;
    (void)mem_write;
#endif
    cpu->hook_brk = NULL;
    cpu->hook_stp = NULL;
    cpu->hook_eoi = NULL;
    cpu->cpu_data = cpu_data;

    cpu->pc = 0xFFFFU;
    cpu->a = cpu->x = cpu->y = cpu->s = cpu->p = 0xFF;
    cpu->cpu_state = W65C02S_CPU_STATE_RESET;
    cpu->stall_cycles = 0;
}

unsigned long w65c02s_run_cycles(struct w65c02s_cpu *cpu,
                                 unsigned long cycles) {
    unsigned long c = 0;
    if (W65C02S_UNLIKELY(cpu->stall_cycles)) {
        if (cpu->stall_cycles > cycles) {
            cpu->total_cycles += cycles;
            cpu->stall_cycles -= cycles;
            return cycles;
        } else {
            cpu->total_cycles += cpu->stall_cycles;
            cycles -= cpu->stall_cycles;
            cpu->stall_cycles = 0;
        }
    }
    if (W65C02S_UNLIKELY(!cycles)) return 0;
    W65C02S_CPU_STATE_RST_FLAG(cpu, W65C02S_CPU_STATE_BREAK);
#if W65C02S_COARSE
    c = w65c02s_execute_ix(cpu, cycles);
#else
    c = w65c02s_execute_c(cpu, cycles);
#endif
    return c;
}

unsigned long w65c02s_step_instruction(struct w65c02s_cpu *cpu) {
    unsigned cycles;
    W65C02S_CPU_STATE_RST_FLAG(cpu, W65C02S_CPU_STATE_BREAK);
#if !W65C02S_COARSE
    if (W65C02S_UNLIKELY(cpu->cycl))
        cycles = w65c02s_execute_ic(cpu);
    else
#endif
        cycles = w65c02s_execute_i(cpu);
#if !W65C02S_COARSE
    cpu->cycl = 0;
#endif
    return cycles;
}

unsigned long w65c02s_run_instructions(struct w65c02s_cpu *cpu,
                                       unsigned long instructions,
                                       bool finish_existing) {
    unsigned long total_cycles, stalled = 0;
    if (W65C02S_UNLIKELY(!instructions)) return 0;
    W65C02S_CPU_STATE_RST_FLAG(cpu, W65C02S_CPU_STATE_BREAK);
    if (W65C02S_UNLIKELY(cpu->stall_cycles)) {
        stalled = cpu->stall_cycles;
        cpu->total_cycles += stalled;
        cpu->stall_cycles = 0;
    }
#if !W65C02S_COARSE
    if (W65C02S_UNLIKELY(cpu->cycl)) {
        total_cycles = w65c02s_execute_ic(cpu);
        if (!finish_existing && !--instructions)
            return total_cycles;
        total_cycles += w65c02s_execute_im(cpu, instructions);
    } else
        total_cycles = w65c02s_execute_im(cpu, instructions);
#else
    (void)finish_existing;
    total_cycles = w65c02s_execute_im(cpu, instructions);
#endif
#if !W65C02S_COARSE
    cpu->cycl = 0;
#endif
    return total_cycles + stalled;
}

void w65c02s_break(struct w65c02s_cpu *cpu) {
#if !W65C02S_COARSE
    /* also adjust the cycle counters so we stop right away. */
    unsigned long next_cycle = cpu->total_cycles + 1;
    cpu->maximum_cycles -= cpu->target_cycles - next_cycle;
    cpu->target_cycles = next_cycle;
#endif
    W65C02S_CPU_STATE_SET_FLAG(cpu, W65C02S_CPU_STATE_BREAK);
}

void w65c02s_stall(struct w65c02s_cpu *cpu, unsigned long cycles) {
    w65c02s_break(cpu);
    cpu->stall_cycles += cycles;
}

void w65c02s_nmi(struct w65c02s_cpu *cpu) {
    cpu->int_trig |= W65C02S_CPU_STATE_NMI;
    if (W65C02S_CPU_STATE_EXTRACT(cpu->cpu_state) == W65C02S_CPU_STATE_WAIT) {
        W65C02S_CPU_STATE_INSERT(cpu->cpu_state, W65C02S_CPU_STATE_RUN);
        W65C02S_CPU_STATE_ASSERT_NMI(cpu);
    }
}

void w65c02s_reset(struct w65c02s_cpu *cpu) {
    W65C02S_CPU_STATE_ASSERT_RESET(cpu);
    W65C02S_CPU_STATE_CLEAR_IRQ(cpu);
    W65C02S_CPU_STATE_CLEAR_NMI(cpu);
}

void w65c02s_irq(struct w65c02s_cpu *cpu) {
    cpu->int_trig |= W65C02S_CPU_STATE_IRQ;
    if (W65C02S_CPU_STATE_EXTRACT(cpu->cpu_state) == W65C02S_CPU_STATE_WAIT) {
        W65C02S_CPU_STATE_INSERT(cpu->cpu_state, W65C02S_CPU_STATE_RUN);
        cpu->cpu_state |= W65C02S_CPU_STATE_IRQ & cpu->int_mask;
    }
}

void w65c02s_irq_cancel(struct w65c02s_cpu *cpu) {
    cpu->int_trig &= ~W65C02S_CPU_STATE_IRQ;
}

/* brk_hook: 0 = treat BRK as normal, <>0 = treat it as NOP */
bool w65c02s_hook_brk(struct w65c02s_cpu *cpu, bool (*brk_hook)(uint8_t)) {
#if W65C02S_HOOK_BRK
    cpu->hook_brk = brk_hook;
    return true;
#else
    (void)cpu;
    (void)brk_hook;
    return false;
#endif
}

/* stp_hook: 0 = treat STP as normal, <>0 = treat it as NOP */
bool w65c02s_hook_stp(struct w65c02s_cpu *cpu, bool (*stp_hook)(void)) {
#if W65C02S_HOOK_STP
    cpu->hook_stp = stp_hook;
    return true;
#else
    (void)cpu;
    (void)stp_hook;
    return false;
#endif
}

bool w65c02s_hook_end_of_instruction(struct w65c02s_cpu *cpu,
                                     void (*instruction_hook)(void)) {
#if W65C02S_HOOK_EOI
    cpu->hook_eoi = instruction_hook;
    return true;
#else
    (void)cpu;
    (void)instruction_hook;
    return false;
#endif
}

unsigned long w65c02s_get_cycle_count(const struct w65c02s_cpu *cpu) {
    return cpu->total_cycles;
}

unsigned long w65c02s_get_instruction_count(const struct w65c02s_cpu *cpu) {
    return cpu->total_instructions;
}

void w65c02s_reset_cycle_count(struct w65c02s_cpu *cpu) {
    cpu->total_cycles = 0;
}

void w65c02s_reset_instruction_count(struct w65c02s_cpu *cpu) {
    cpu->total_instructions = 0;
}

void *w65c02s_get_cpu_data(const struct w65c02s_cpu *cpu) {
    return cpu->cpu_data;
}

bool w65c02s_is_cpu_waiting(const struct w65c02s_cpu *cpu) {
    return W65C02S_CPU_STATE_EXTRACT(cpu->cpu_state) == W65C02S_CPU_STATE_WAIT;
}

bool w65c02s_is_cpu_stopped(const struct w65c02s_cpu *cpu) {
    return W65C02S_CPU_STATE_EXTRACT(cpu->cpu_state) == W65C02S_CPU_STATE_STOP;
}

void w65c02s_set_overflow(struct w65c02s_cpu *cpu) {
    cpu->p |= W65C02S_P_V;
}

uint8_t w65c02s_reg_get_a(const struct w65c02s_cpu *cpu) { return cpu->a; }
uint8_t w65c02s_reg_get_x(const struct w65c02s_cpu *cpu) { return cpu->x; }
uint8_t w65c02s_reg_get_y(const struct w65c02s_cpu *cpu) { return cpu->y; }
uint8_t w65c02s_reg_get_s(const struct w65c02s_cpu *cpu) { return cpu->s; }
uint16_t w65c02s_reg_get_pc(const struct w65c02s_cpu *cpu) { return cpu->pc; }

uint8_t w65c02s_reg_get_p(const struct w65c02s_cpu *cpu) {
    return cpu->p | W65C02S_P_A1 | W65C02S_P_B;
}

void w65c02s_reg_set_a(struct w65c02s_cpu *cpu, uint8_t v) { cpu->a = v; }
void w65c02s_reg_set_x(struct w65c02s_cpu *cpu, uint8_t v) { cpu->x = v; }
void w65c02s_reg_set_y(struct w65c02s_cpu *cpu, uint8_t v) { cpu->y = v; }
void w65c02s_reg_set_s(struct w65c02s_cpu *cpu, uint8_t v) { cpu->s = v; }
void w65c02s_reg_set_pc(struct w65c02s_cpu *cpu, uint16_t v) { cpu->pc = v; }

void w65c02s_reg_set_p(struct w65c02s_cpu *cpu, uint8_t v) {
    cpu->p = v | W65C02S_P_A1 | W65C02S_P_B;
    w65c02s_irq_update_mask(cpu);
}

#endif /* W65C02S_IMPL */

#ifdef __cplusplus
}
#endif
