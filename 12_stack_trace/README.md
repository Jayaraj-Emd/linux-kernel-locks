# Linux Kernel Stack Trace

## 1. Objective

Understand how Linux kernel stack traces work and how they help debug kernel modules and device drivers.

Topics covered:

- What is a stack trace?
- Difference between call stack and stack trace.
- `dump_stack()` API.
- Function-call sequence.
- Stack unwinding.
- Compiler optimization and function inlining.
- Understanding stack trace output.
- Practical applications in driver debugging.

## 2. What is a Stack Trace?

A stack trace represents the sequence of function calls leading to the current execution point.

When an error occurs inside a kernel function, a stack trace helps identify:

1. Which function encountered the problem?
2. Which function called it?
3. What was the execution path leading to the problem?

The stack trace helps developers reconstruct how execution reached a particular point.

## 3. Call Stack vs Stack Trace

| Call Stack | Stack Trace |
|---|---|
| Runtime memory structure used during function execution. | Representation of the active function-call sequence. |
| Contains return addresses, saved registers and local variables, depending on architecture and compiler. | Displays function names and instruction locations when symbols are available. |
| Changes as functions are called and return. | Captures the call chain at a particular point. |

**Important:** Function arguments are not necessarily stored on the stack. On x86-64 and ARM64, many function arguments are passed through registers.

## 4. Kernel API — `dump_stack()`

### Definition

`dump_stack()` prints the current kernel stack trace.

### Header

`<linux/kernel.h>`

### Purpose

- Prints the current kernel call stack.
- Helps identify the sequence of function calls.
- Useful for debugging kernel drivers.
- Helps investigate unexpected execution paths and kernel errors.

`dump_stack()` does not print the entire contents of stack memory. It prints a reconstructed representation of the active call chain.

## 5. Function Execution Flow

The experiment implemented three functions calling each other.

Execution order:

```text
Module Initialization
        |
        v
    straceInit()
        |
        v
     functionA()
        |
        v
     functionB()
        |
        v
     functionC()
        |
        v
    dump_stack()
        |
        v
 Kernel prints stack trace
```

### Execution explanation

1. `insmod` requests the kernel to load the module.
2. The kernel invokes the module initialization function.
3. The initialization function calls `functionA()`.
4. `functionA()` calls `functionB()`.
5. `functionB()` calls `functionC()`.
6. `functionC()` invokes `dump_stack()`.
7. The kernel reconstructs and prints the active call chain.

## 6. Understanding Stack Trace Output

Observed output:

```text
Call Trace:
 dump_stack_lvl+0x76/0xa0
 ? __pfx_straceInit+0x10/0x10 [trace]
 dump_stack+0x10/0x20
 functionC+0x1a/0x30 [trace]
 functionB+0x1a/0x30 [trace]
 functionA+0x1a/0x30 [trace]
 straceInit+0x1a/0xff0 [trace]
 do_one_initcall+0x5e/0x340
 do_init_module+0x97/0x290
 load_module+0xb48/0xc80
```

### Important entries

| Function | Purpose |
|---|---|
| `dump_stack_lvl()` | Internal kernel function involved in printing stack information. |
| `dump_stack()` | API called by our module. |
| `functionC()` | Calls `dump_stack()`. |
| `functionB()` | Calls `functionC()`. |
| `functionA()` | Calls `functionB()`. |
| `straceInit()` | Module initialization function. |
| `do_one_initcall()` | Invokes the module initialization function. |
| `do_init_module()` | Participates in module initialization. |
| `load_module()` | Handles loading the module into the kernel. |

### Why is the order reversed?

Actual execution order:

```text
straceInit()
functionA()
functionB()
functionC()
dump_stack()
```

Stack trace order:

```text
dump_stack()
functionC()
functionB()
functionA()
straceInit()
```

The stack trace starts from the current execution point and reconstructs the chain of callers.

It therefore displays the active call chain in reverse execution order.

## 7. Understanding Function Offsets

Consider:

```text
functionC+0x1a/0x30 [trace]
```

| Component | Meaning |
|---|---|
| `functionC` | Function name. |
| `+0x1a` | Instruction offset from the beginning of the function. |
| `/0x30` | Function size. |
| `[trace]` | Kernel module containing the function. |

This helps identify approximately where execution was inside a function when the stack trace was generated.

## 8. Stack Unwinding

### Definition

Stack unwinding is the process of reconstructing the active function-call chain by examining saved execution information.

The kernel uses stack unwinding to generate stack traces.

Depending on architecture and build configuration, the unwinder can use information such as:

- Frame pointers.
- Saved return addresses.
- Register and stack information.
- Compiler-generated unwind information, where supported.

### Why is it important?

Without stack unwinding, the kernel would have difficulty reconstructing which functions led to the current execution point.

It is particularly useful when investigating:

- Kernel NULL pointer dereferences.
- Kernel warnings.
- Kernel crashes.
- Unexpected function-call paths.
- Driver execution problems.

## 9. Compiler Inlining and Stack Traces

### What is function inlining?

Function inlining is a compiler optimization in which a function call is replaced with the function's body.

This can change the visible function-call structure in a stack trace.

### Observation from the experiment

Initially, the stack trace did not show all three functions.

After adding the compiler option:

`-fno-inline`

the stack trace showed:

```text
functionC
functionB
functionA
straceInit
```

This made the original function-call hierarchy visible.

**Important:** `-fno-inline` prevents ordinary function inlining. It is useful for debugging experiments but is not generally necessary in production kernel modules.

## 10. Understanding `?` in Stack Trace

Example:

```text
? srso_return_thunk+0x5/0x5f
```

The `?` indicates that the stack entry is uncertain or unreliable.

It should not automatically be interpreted as a normal function call in the active call chain.

## 11. Understanding CPU and PID Information

Example:

```text
CPU: 2 PID: 7590 Comm: insmod
```

| Field | Meaning |
|---|---|
| CPU: 2 | Logical CPU executing the code. |
| PID: 7590 | Process ID of the current task. |
| Comm: insmod | Name of the current task. |

In this experiment, the module initialization function executes in the context of the process requesting module loading.

## 12. Practical Applications in Driver Development

Stack traces are useful when debugging:

1. Character drivers.
2. Platform drivers.
3. I2C and SPI drivers.
4. PCIe drivers.
5. Interrupt handling.
6. Workqueues and kernel threads.
7. Kernel synchronization issues.
8. Unexpected execution paths.

For example, if a driver produces a NULL pointer dereference, the kernel stack trace can help identify the function where the fault occurred and the path that led to it.

## 13. Interview Questions

**Q1. What is a kernel stack trace?**

A stack trace represents the active function-call sequence at a particular execution point.

**Q2. What is `dump_stack()`?**

It is a Linux kernel API that prints the current kernel stack trace.

**Q3. What is the difference between a call stack and a stack trace?**

The call stack is a runtime memory structure used during function execution. A stack trace is a representation of the function-call sequence.

**Q4. What is stack unwinding?**

Stack unwinding is the process of reconstructing the active function-call chain using saved execution information.

**Q5. Why can compiler optimization affect stack traces?**

Inlining and other optimizations can change or remove separate function-call frames, affecting which functions appear in a stack trace.

**Q6. What does `functionC+0x1a/0x30` mean?**

It indicates that the instruction is approximately 0x1a bytes from the beginning of `functionC`, whose size is 0x30 bytes.

**Q7. Why does a stack trace show functions in reverse execution order?**

Because it starts at the current execution point and reconstructs the chain of callers.

**Q8. Why is a stack trace useful in Linux driver development?**

It helps identify the location of a problem and the execution path that led to it.

## 14. Experiment Summary

| Concept | Status |
|---|---|
| `dump_stack()` | Completed |
| Function-call hierarchy | Completed |
| Stack trace generation | Completed |
| Stack unwinding | Completed |
| Function offsets | Completed |
| Compiler inlining | Experimentally investigated |
| CPU and PID information | Identified in kernel logs |

## Conclusion

This experiment demonstrates how to generate and interpret Linux kernel stack traces using `dump_stack()`.

Understanding stack traces and stack unwinding is an essential debugging skill for Linux kernel and device driver development.
