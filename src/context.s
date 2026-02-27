.syntax unified
.cpu cortex-m0
.fpu softvfp

.thumb

.extern scheduler_is_started
.extern set_scheduler_started
.extern get_current_task
.extern get_next_task

.global _cpsid
.type _cpsid, %function

_cpsid:
    cpsid i
    bx lr

.global _cpsie
.type _cpsie, %function

_cpsie:
    cpsie i
    bx lr

.global set_spsel
.type set_spsel, %function

set_spsel: // Do not run in interrupt, or it will break!
    mov r1, sp
    msr control, r0
    msr psp, r1
    isb
    bx lr

.global is_privileged
.type is_privileged, %function

is_privileged:
    mrs r0, msp
    mov r1, sp

    cmp r0, r1
    beq priviliged_true

    movs r0, #0
    bx lr

priviliged_true:
    movs r0, #1
    bx lr

.global isr_pendsv
.type isr_pendsv, %function

isr_pendsv:
    cpsid i                 // disable interrupts

    blx scheduler_is_started // get wither this core's scheduler is started or not

    cmp r0, #0              // compare r0 with 0
    beq first_context_switch // this is the first time running

    mov r1, sp              // get the msp (through sp) and save it for later

    mrs r2, psp             // get the process stack pointer
    mov sp, r2              // and use it for now

    push {r4-r7}            // push r4-r7
    mov r4, r8              // shuffle high registers to r4-r7
    mov r5, r9
    mov r6, r10
    mov r7, r11
    push {r4-r7}            // push r8-r11

    mov r4, sp              // copy stack pointer
    mov sp, r1              // restore the original msp

    blx get_current_task    // get the current task of this core's scheduler

    str r4, [r0]            // and save the stack pointer there

first_context_switch:       // skip here if this is the first time running, as there is nothing to save
    blx get_next_task       // run `get_next_task` to get the next task

    mov r1, sp              // get the msp (through sp) and save it for later

    blx get_current_task    // get the current task of this core's scheduler

    ldr r2, [r0]            // get the current task's stack pointer
    mov sp, r2              // and use it for now

    pop {r4-r7}             // pop r8-r11
    mov r8, r4              // shuffle high values from low rigisters to high registers
    mov r9, r5
    mov r10, r6
    mov r11, r7
    pop {r4-r7}             // pop r4-r7

    mov r2, sp              // copy the stack pointer to r2
    msr psp, r2             // and save it to psp for the task to use

    mov sp, r1              // restore the original msp

    movs r0, #1             // load a 1
    blx set_scheduler_started // run `set_scheduler_started` with a one to say its started

    isb

    cpsie i                 // enable interrupts again

    ldr r0, =0xFFFFFFFD     // load `0xFFFFFFFD` (special address to tell cpu to go back to process mode)
    bx r0                   // and go there, exiting

.end
