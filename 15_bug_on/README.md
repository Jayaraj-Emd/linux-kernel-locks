# Linux Kernel BUG_ON()

## 1. Introduction

`BUG_ON()` is a Linux kernel macro used to detect a condition that represents a serious programming error or an impossible internal state.

When the condition becomes true, the kernel triggers a BUG event.

Unlike `WARN_ON()`, `BUG_ON()` does not allow normal execution to continue along the current execution path.

**Important:** `BUG_ON()` is generally discouraged in normal Linux driver development because it can cause a kernel Oops or panic, potentially affecting the entire system.

---

## 2. Header File

```c
#include <linux/bug.h>
```

The `BUG_ON()` macro is defined in the Linux kernel BUG handling infrastructure.

---

## 3. Syntax

```c
BUG_ON(condition);
```

### Behaviour

| Condition | Behaviour |
|---|---|
| False (0) | No BUG is triggered; execution continues |
| True (non-zero) | Kernel triggers a BUG event |

---

## 4. Working Principle

### Case 1: Condition is false

When the condition is false:

1. Kernel evaluates the condition.
2. The condition evaluates to false.
3. No BUG is triggered.
4. Execution continues normally.

### Case 2: Condition is true

When the condition is true:

1. Kernel evaluates the condition.
2. The condition evaluates to true.
3. Kernel triggers a BUG event.
4. Diagnostic information is generated.
5. The current execution path does not continue normally.

Depending on the architecture and kernel configuration, this may result in a kernel Oops or panic.

---

## 5. BUG_ON() and Kernel Oops

A BUG event indicates a serious kernel error.

### Kernel Oops

A kernel Oops is a diagnostic report generated when the kernel encounters a serious error.

It may contain:
- CPU number
- PID and process name
- Instruction pointer
- Register values
- Stack trace
- Module information

An Oops may terminate the current task or disrupt kernel execution.

### Kernel Panic

A kernel panic occurs when the kernel cannot safely continue operating.

The system may stop completely or require a reboot.

**A BUG event does not necessarily cause a panic in every configuration.**

---

## 6. WARN_ON() vs WARN_ON_ONCE() vs BUG_ON()

| Feature | WARN_ON() | WARN_ON_ONCE() | BUG_ON() |
|---|---|---|---|
| Purpose | Report unexpected condition | Report unexpected condition once | Trigger fatal BUG |
| Condition true | Warning generated | Warning generated once per call site | BUG event triggered |
| Stack trace | Yes | Yes | Diagnostic trace |
| Normal execution continues | Yes | Yes | No |
| Repeated warning | Every occurrence | Once per call site | Not applicable |
| Severity | Warning | Warning | Fatal |

### Key Difference

`WARN_ON()` and `WARN_ON_ONCE()` are diagnostic mechanisms.

`BUG_ON()` is a fatal error mechanism.

---

## 7. Why BUG_ON() Is Discouraged

Consider a device driver that reads a hardware register.

The device may return an unexpected value because:
- Hardware is not initialized.
- Communication failed.
- The device is disconnected.
- The register contains an invalid value.

Using `BUG_ON()` for such conditions may crash or disrupt the kernel.

Instead, the driver should:
1. Validate the value.
2. Handle the error.
3. Return an appropriate error code.
4. Allow the system to continue operating when possible.

### Recommended Error Handling

For expected runtime errors, use normal error handling such as returning an appropriate negative error code.

Examples:
- `-EINVAL` — Invalid argument
- `-EIO` — I/O error
- `-ENOMEM` — Memory allocation failure

`WARN_ON()` may be appropriate for unexpected internal invariant violations, but it should not replace normal error handling.

---

## 8. When Should BUG_ON() Be Used?

`BUG_ON()` should generally be avoided in ordinary device-driver code.

It may be considered only for situations where:
- The kernel has reached an impossible internal state.
- Continuing execution would be unsafe.
- The condition represents a severe programming error.

Even in these cases, developers should carefully consider whether a safer recovery mechanism is possible.

---

## 9. Important Concepts

### BUG
A fatal kernel event indicating a serious error.

### Oops
A kernel diagnostic report for a serious execution error.

### Panic
A condition where the kernel stops normal operation.

### Stack Trace
A record of the active function call chain that helps identify where an error occurred.

---

## 10. Interview Questions

### Q1. What is BUG_ON()?

`BUG_ON()` is a Linux kernel macro that triggers a BUG event when its condition evaluates to true.

### Q2. What happens when BUG_ON() condition is false?

No BUG is triggered, and execution continues normally.

### Q3. What is the difference between WARN_ON() and BUG_ON()?

`WARN_ON()` reports a warning and allows execution to continue. `BUG_ON()` triggers a fatal BUG event.

### Q4. Can BUG_ON() cause a kernel panic?

Yes. Depending on the architecture and kernel configuration, a BUG event can cause a kernel Oops or panic.

### Q5. Why is BUG_ON() discouraged in drivers?

Because an unexpected device or runtime error should not unnecessarily crash the entire kernel. Normal error handling is preferred.

### Q6. What should a driver do when a hardware register contains an invalid value?

Validate the value, handle the error, and return an appropriate error code instead of triggering a kernel BUG.

---

## 11. Summary

- `BUG_ON()` is defined in `<linux/bug.h>`.
- It triggers a BUG event when the condition is true.
- When the condition is false, execution continues normally.
- It can cause a kernel Oops or panic.
- It is generally discouraged in normal driver development.
- Use proper error handling for expected runtime failures.
- `WARN_ON()` is typically safer for reporting unexpected internal conditions.

**Key takeaway:** `BUG_ON()` is a fatal kernel debugging mechanism, not a normal device-driver error-handling API.