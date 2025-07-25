.syntax unified
.cpu cortex-m0
.fpu softvfp

.thumb

.extern scheduler_started
.extern current_task
.extern current_task_index
.extern task_list
.extern getNextTask

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

.global isr_pendsv
.type isr_pendsv, %function

isr_pendsv:
    cpsid i                 // disable interrupts

    ldr r0, =scheduler_started // get wither or not the scheduler has run before
    ldr r1, [r0]            // load contents at [r0] to r1
    cmp r1, #0              // compare r1 with 0
    beq first_context_switch // this is the first time running

    mov r3, sp              // get the msp (through sp) and save it for later

    mrs r1, psp             // get the process stack pointer
    mov sp, r1              // and use it for now

    push {r4-r7}            // push r4-r7
    mov r4, r8              // shuffle high registers to r4-r7
    mov r5, r9
    mov r6, r10
    mov r7, r11
    push {r4-r7}            // push r8-r11

    mov r1, sp              // copy stack pointer
    ldr r2, =current_task   // get the address of the current task stack pointer
    ldr r2, [r2]            // shuffle that address around
    str r1, [r2]            // and save the stack pointer there

    mov sp, r3              // restore the original msp

first_context_switch:       // skip here if this is the first time running, as there is nothing to save
    blx getNextTask         // run `getNextTask` to get the next task

    mov r3, sp              // get the msp (through sp) and save it for later

    ldr r1, =current_task   // get the address of the current task stack pointer
    ldr r1, [r1]            // shuffle that address around
    ldr r2, [r1]            // shuffle that address around
    mov sp, r2              // and use it for now

    pop {r4-r7}             // pop r8-r11
    mov r8, r4              // shuffle high values from low rigisters to high registers
    mov r9, r5
    mov r10, r6
    mov r11, r7
    pop {r4-r7}             // pop r4-r7

    mov r1, sp              // copy the stack pointer to r1
    msr psp, r1             // and save it to psp for the task to use

    mov sp, r3              // restore the original msp

    ldr r0, =scheduler_started // get the location of the scheduler_started variable
    movs r1, #1             // load a 1
    str r1, [r0]            // and save it to scheduler_started to say we've started

    cpsie i                 // enable interrupts again

    ldr r0, =0xFFFFFFFD     // load `0xFFFFFFFD` (special address to tell cpu to go back to process mode)
    bx r0                   // and go there, exiting

.end
