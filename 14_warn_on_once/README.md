## WARN_ON_ONCE() 

### 1. What is WARN_ON_ONCE()?

`WARN_ON_ONCE()` is a Linux kernel macro used to detect unexpected conditions and generate a warning **only once per call site**.

Header file:
```c
#include <linux/bug.h>
```

### 2. Syntax

```c
WARN_ON_ONCE(condition);
```

- If the condition is `true`, the kernel prints a warning and stack trace.
- If the condition is `false`, nothing happens.
- The warning is printed only the first time the condition becomes true at that call site.

### 3. Return Value

| Condition | Return value |
|---|---|
| True | Non-zero (1) |
| False | 0 |

### 4. How Does It Work?

1. The kernel evaluates the condition every time the macro executes.
2. If the condition is true for the first time, it generates a warning.
3. The kernel records that the warning has occurred.
4. Subsequent true conditions at the same call site do not generate another warning.
5. Normal execution continues.

### 5. WARN_ON() vs WARN_ON_ONCE()

| Feature | WARN_ON() | WARN_ON_ONCE() |
|---|---|---|
| Condition true | Generates warning | Generates warning |
| Repeated calls | Warning every time | Warning only once |
| Stack trace | Yes | Yes |
| Execution continues | Yes | Yes |
| Return value | Non-zero if true | Non-zero if true |

### 6. Important Points

- Warning state belongs to the **call site**, not the function invocation.
- Different `WARN_ON_ONCE()` locations maintain independent warning states.
- A false condition does not reset the warning state.
- Reloading the module resets its static warning state.
- It helps prevent repeated warning messages from flooding kernel logs.

### 7. When Should We Use It?

Use `WARN_ON_ONCE()` when:
- An unexpected internal kernel condition occurs.
- You want to identify a problem without repeatedly printing warnings.
- A warning at the same code location does not need to be printed repeatedly.

**Interview takeaway:** `WARN_ON_ONCE()` evaluates the condition every time but generates a warning and stack trace only once per call site.
