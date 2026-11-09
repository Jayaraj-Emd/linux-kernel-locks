# Linux Kernel Symbol Export and Resolution

## 1. Introduction

Linux supports Loadable Kernel Modules (LKMs). These modules can be independently compiled and loaded into the running kernel.

Sometimes, one kernel module needs to use a function or variable implemented in another kernel module.

Example:

- Module A implements a function.
- Module A exports the function.
- Module B uses the exported function.
- The kernel resolves the function reference when Module B is loaded.

Linux provides `EXPORT_SYMBOL()` and `EXPORT_SYMBOL_GPL()` to make symbols available to other kernel modules.

This README explains kernel symbol sharing, symbol resolution, module dependencies, and the responsibilities of the kernel build system and module loader.

---

# 2. What Is a Kernel Symbol?

A kernel symbol is a named entity that can be referenced by other parts of the kernel.

Examples:

- Functions
- Global variables

Example:

```c
int calculate_sum(int a, int b)
{
    return a + b;
}
```

Here, `calculate_sum` is a symbol.

A symbol has a name and is associated with an implementation or memory location.

The kernel uses symbol information to resolve references between kernel components.

## Important

Not every function inside a kernel module is automatically available to other modules.

A symbol must be exported using an appropriate export mechanism before another module can normally resolve it as an exported module symbol.

---

# 3. What Is `extern`?

`extern` is a C language keyword.

It is not part of the C standard library.

It tells the compiler that a function or variable is declared here but defined elsewhere.

Example:

```c
extern int calculate_sum(int a, int b);
```

This declaration tells the compiler:

- A function named `calculate_sum` exists.
- It returns an integer.
- It accepts two integer arguments.
- Its implementation is located elsewhere.

## What `extern` Does

`extern` provides a declaration.

It does not:

- Export the function from a kernel module.
- Make the function available to other modules.
- Load the module containing the function.
- Resolve the runtime function address.

### Declaration vs Export

| Concept | Responsibility |
|---|---|
| `extern` | Declares that a function or variable exists elsewhere |
| `EXPORT_SYMBOL()` | Makes a symbol available for other kernel modules |
| Symbol resolution | Finds the actual symbol address during module loading |

---

# 4. What Is `EXPORT_SYMBOL()`?

`EXPORT_SYMBOL()` exports a function or variable so that other kernel modules can use it.

Syntax:

```c
EXPORT_SYMBOL(symbol_name);
```

Example:

```c
int calculate_sum(int a, int b)
{
    return a + b;
}

EXPORT_SYMBOL(calculate_sum);
```

The function `calculate_sum()` is now an exported symbol.

The kernel build system records exported-symbol information, and the module loader can resolve references to this symbol when the exporting module is loaded.

## Important

Exporting a symbol does not automatically:

- Call the function.
- Load another module.
- Create a dependency-loading mechanism.

It makes the symbol available for resolution.

---

# 5. What Is `EXPORT_SYMBOL_GPL()`?

Linux provides another export macro:

```c
EXPORT_SYMBOL_GPL(calculate_sum);
```

This exports the symbol only to modules whose license is compatible with the kernel's GPL-only symbol restrictions.

## Difference

| Macro | Meaning |
|---|---|
| `EXPORT_SYMBOL()` | Exports a symbol subject to kernel symbol licensing rules |
| `EXPORT_SYMBOL_GPL()` | Exports a symbol with GPL-compatible module restrictions |

The module license is declared using:

```c
MODULE_LICENSE("GPL");
```

The license declaration affects whether a module can use GPL-only exported symbols.

---

# 6. Example: Module A and Module B

We will use two modules.

- Module A provides a function.
- Module B uses the function.

## Module A — Provider

File: `module_a.c`

```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

int my_function(void)
{
    pr_info("Module A: my_function called\n");

    return 100;
}

EXPORT_SYMBOL(my_function);

static int __init module_a_init(void)
{
    pr_info("Module A loaded\n");

    return 0;
}

static void __exit module_a_exit(void)
{
    pr_info("Module A unloaded\n");
}

module_init(module_a_init);
module_exit(module_a_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module A exports a function");
MODULE_AUTHOR("Jayaraj");
```

### Explanation

```c
int my_function(void)
```

Implements the function.

```c
EXPORT_SYMBOL(my_function);
```

Exports the function so another kernel module can reference it.

Module A is the provider of the symbol.

## Module B — Consumer

File: `module_b.c`

```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

extern int my_function(void);

static int __init module_b_init(void)
{
    int value;

    value = my_function();

    pr_info("Module B: Value = %d\n", value);

    return 0;
}

static void __exit module_b_exit(void)
{
    pr_info("Module B unloaded\n");
}

module_init(module_b_init);
module_exit(module_b_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module B uses an exported symbol");
MODULE_AUTHOR("Jayaraj");
```

### Explanation

```c
extern int my_function(void);
```

Declares the function.

```c
value = my_function();
```

Calls the function provided by Module A.

Module B is the consumer of the symbol.

---

# 7. Why Is `extern` Not Enough?

Consider:

```c
extern int my_function(void);
```

The compiler knows the function declaration.

However, the actual implementation is located in Module A.

During compilation, Module B can contain an unresolved reference to `my_function`.

Conceptually:

```text
Module B Object File

module_b_init()
      |
      |
      +---- unresolved reference ----> my_function
```

The function declaration does not provide the implementation.

The kernel module loader must resolve the reference during module loading.

For this to happen:

1. Module A must export the symbol.
2. Module A must be loaded, or the symbol must be provided by another loaded kernel component.
3. Module B must reference the correct symbol.
4. The kernel module loader must successfully resolve the symbol.

---

# 8. What Is Symbol Resolution?

Symbol resolution is the process of finding the actual address of a symbol referenced by a kernel module.

Suppose Module B references:

```c
my_function();
```

The compiled Module B contains a reference to `my_function`.

When Module B is loaded, the kernel must determine where `my_function` is located in memory.

Conceptually:

```text
Module B references:

my_function
      |
      v
Kernel symbol lookup
      |
      v
Find exported symbol
      |
      v
Get runtime address
      |
      v
Apply relocation
      |
      v
Module B can call my_function()
```

The kernel module loader resolves symbols using exported symbols available from the kernel and loaded modules.

---

# 9. Build Time vs Load Time

This distinction is extremely important.

## Build Time

During the build process:

- The compiler compiles source code.
- The object file may contain unresolved symbol references.
- The kernel build system checks exported-symbol information.
- `Module.symvers` provides exported symbol information.
- Symbol CRC information may be checked when symbol versioning is enabled.

The build system does not normally determine the final runtime address of another module's function.

## Load Time

During module loading:

- The kernel allocates memory for the module.
- The module's sections are loaded.
- Relocations are processed.
- Exported symbols are registered.
- Unresolved symbol references are resolved.
- The module becomes available for execution.

### Comparison

| Build Time | Load Time |
|---|---|
| Compiler and kernel build system | Kernel module loader |
| Uses symbol declarations and export metadata | Uses symbols available in the running kernel |
| `Module.symvers` provides symbol information | Kernel resolves actual runtime addresses |
| Generates `.ko` file | Loads `.ko` into kernel memory |

---

# 10. What Is `Module.symvers`?

`Module.symvers` is a file generated by the kernel build system.

It contains information about exported symbols.

A typical entry is conceptually:

```text
0x12345678    my_function    module_a    EXPORT_SYMBOL
```

## Fields

| Field | Meaning |
|---|---|
| `0x12345678` | Symbol CRC, when symbol versioning is enabled |
| `my_function` | Exported symbol name |
| `module_a` | Module that exports the symbol |
| `EXPORT_SYMBOL` | Export type |

The actual format depends on the kernel configuration and build system.

## Why Does the Build System Need It?

Suppose Module B uses:

```c
extern int my_function(void);
```

The kernel build system needs to know whether `my_function` is exported.

`Module.symvers` provides this information.

It also provides symbol version CRCs when configured.

### Important

`Module.symvers` does not contain the final runtime function address.

The kernel module loader resolves the actual address when the module is loaded.

## Inspect Module.symvers

```bash
cat /lib/modules/$(uname -r)/build/Module.symvers
```

Search for a symbol:

```bash
grep "my_function" /lib/modules/$(uname -r)/build/Module.symvers
```

---

# 11. What Is `depmod`?

`depmod` analyzes kernel modules and generates dependency information.

Command:

```bash
sudo depmod
```

It generates dependency metadata such as:

```text
/lib/modules/<kernel-version>/modules.dep
/lib/modules/<kernel-version>/modules.dep.bin
```

## Responsibility of depmod

`depmod`:

1. Examines installed kernel modules.
2. Identifies module dependencies.
3. Generates dependency information.
4. Makes the information available to `modprobe`.

Conceptual dependency:

```text
module_b.ko: module_a.ko
```

This indicates that Module B depends on Module A.

---

# 12. What Is `modprobe`?

`modprobe` is a Linux utility for loading and unloading kernel modules.

It uses module dependency information generated by `depmod`.

Command:

```bash
sudo modprobe module_b
```

If Module B depends on Module A, `modprobe` attempts to load Module A first.

## Responsibility of modprobe

`modprobe`:

1. Reads module dependency information.
2. Identifies required modules.
3. Loads prerequisite modules.
4. Requests loading of the target module.

The kernel module loader performs the actual symbol resolution.

## Loading Flow

```text
sudo modprobe module_b
          |
          v
modprobe reads dependency metadata
          |
          v
Module B depends on Module A
          |
          v
Load Module A
          |
          v
Module A exports my_function
          |
          v
Load Module B
          |
          v
Kernel resolves my_function
          |
          v
Module B loads successfully
```

---

# 13. Difference Between insmod and modprobe

| `insmod` | `modprobe` |
|---|---|
| Loads a specified module file | Loads a module using its name |
| Does not automatically load dependencies | Handles dependencies using metadata |
| Uses a `.ko` file path | Uses module dependency database |
| Direct module loading | Dependency-aware module loading |

Example:

```bash
sudo insmod module_a.ko
```

Loads Module A directly.

```bash
sudo modprobe module_b
```

Loads Module B and its dependencies.

## Important

`modprobe` depends on correct dependency metadata.

If modules are built externally and installed manually, dependency metadata may need to be regenerated using `depmod`.

---

# 14. What Happens If Module A Is Not Loaded?

Suppose:

- Module A exports `my_function`.
- Module B uses `my_function`.
- Module A is not loaded.
- Module B is loaded using `insmod`.

Command:

```bash
sudo insmod module_b.ko
```

The kernel attempts to resolve `my_function`.

However, the symbol is not available.

The load fails with an error similar to:

```text
Unknown symbol my_function (err -2)
```

## Why?

`insmod` does not automatically load Module A.

The kernel module loader cannot resolve the symbol because the provider module is not loaded.

## With modprobe

If Module B's dependency metadata correctly identifies Module A:

```bash
sudo modprobe module_b
```

`modprobe` loads Module A first.

Then the kernel can resolve the symbol when Module B is loaded.

---

# 15. Module Dependency and Symbol Dependency

A symbol dependency exists when one module references a symbol exported by another module.

Example:

```text
Module A exports:
    my_function()

Module B references:
    my_function()
```

Therefore:

```text
Module B depends on Module A
```

This dependency is important for:

- Module loading order
- Module unloading
- Dependency tracking
- Symbol resolution

The dependency information allows `modprobe` to load modules in the correct order.

---

# 16. What Happens During Module A Loading?

When Module A is loaded:

```bash
sudo insmod module_a.ko
```

The kernel module loader performs operations conceptually similar to:

```text
1. Read module ELF file
2. Allocate memory for module sections
3. Load machine code and data
4. Perform relocations
5. Process exported symbols
6. Register exported symbols
7. Execute module initialization function
```

After Module A is loaded, its exported symbols can be available to other modules.

Example:

```text
my_function → runtime function address
```

The actual implementation and data structures vary with the kernel version and configuration.

---

# 17. What Happens During Module B Loading?

When Module B is loaded:

```bash
sudo insmod module_b.ko
```

The kernel module loader:

```text
1. Read Module B ELF file
2. Allocate memory for Module B
3. Load Module B sections
4. Find unresolved symbols
5. Search exported symbols
6. Resolve symbol addresses
7. Apply relocations
8. Execute Module B initialization function
```

After successful symbol resolution:

```c
value = my_function();
```

can execute using the resolved address.

---

# 18. Important Kernel APIs and Macros

| API / Macro | Purpose |
|---|---|
| `EXPORT_SYMBOL()` | Exports a symbol |
| `EXPORT_SYMBOL_GPL()` | Exports a GPL-restricted symbol |
| `extern` | Declares an external function or variable |
| `MODULE_LICENSE()` | Declares the module license |
| `module_init()` | Specifies module initialization function |
| `module_exit()` | Specifies module cleanup function |

---

# 19. Common Errors

## Unknown Symbol

```text
Unknown symbol my_function
```

Possible reasons:

- Module A is not loaded.
- Module A did not export the symbol.
- Module B references the wrong symbol name.
- The required dependency is missing.

## Module Dependency Missing

Module B cannot load because Module A is not loaded.

Possible solution:

```bash
sudo modprobe module_b
```

provided that dependency metadata is available.

## Symbol Version Mismatch

A module may fail to load if symbol CRCs do not match the running kernel.

Possible causes:

- Module built against incompatible kernel headers.
- Kernel configuration mismatch.
- Module built against a different kernel version.

## GPL-only Symbol

A module may fail to use a GPL-only exported symbol if its license is incompatible.

---

# 20. Complete Symbol Sharing Flow

```text
                  MODULE A
                  --------

Implements function
       |
       v
EXPORT_SYMBOL(my_function)
       |
       v
Build Module A
       |
       v
Exported symbol information


                  MODULE B
                  --------

extern int my_function(void);
       |
       v
Calls my_function()
       |
       v
Build Module B
       |
       v
Unresolved symbol reference


                BUILD SYSTEM
                ------------

Module.symvers
       |
       v
Provides exported-symbol information
       |
       v
Build validation and symbol version checks


                DEPENDENCY SYSTEM
                ----------------

depmod
       |
       v
Generates module dependency metadata
       |
       v
modprobe reads dependency metadata


                LOAD TIME
                ---------

modprobe module_b
       |
       v
Load Module A
       |
       v
Register Module A's exported symbols
       |
       v
Load Module B
       |
       v
Kernel module loader resolves my_function
       |
       v
Apply relocations
       |
       v
Module B calls my_function()
```

---

# 21. Interview Questions

### Q1. What is `EXPORT_SYMBOL()`?

It exports a kernel function or variable so that other kernel modules can use it.

### Q2. Is `extern` sufficient for sharing a function between modules?

No. `extern` declares the function, but the function must be available as an exported symbol for module symbol resolution.

### Q3. What is `Module.symvers`?

It is generated by the kernel build system and contains exported symbol information, including symbol names, exporting modules, export types, and CRCs when configured.

### Q4. Does `Module.symvers` contain runtime addresses?

No. Runtime symbol addresses are resolved by the kernel module loader.

### Q5. What is the difference between `insmod` and `modprobe`?

`insmod` loads a specified module file directly. `modprobe` handles module dependencies using metadata generated by `depmod`.

### Q6. What does `depmod` do?

It analyzes installed modules and generates dependency information.

### Q7. What happens if Module B uses a symbol from Module A but Module A is not loaded?

Module B's loading fails if the symbol is unavailable. Using `modprobe` with valid dependency metadata can load Module A first.

### Q8. Who resolves the actual function address?

The kernel module loader resolves the symbol address and applies relocations during module loading.

### Q9. What is the difference between build-time and load-time symbol resolution?

Build-time processing uses symbol declarations and exported-symbol metadata. Load-time processing resolves references to actual addresses in the running kernel.

### Q10. What is the difference between `EXPORT_SYMBOL()` and `EXPORT_SYMBOL_GPL()`?

`EXPORT_SYMBOL_GPL()` imposes GPL-compatible license restrictions on modules using the symbol.

---

# 22. Summary

The Linux kernel uses exported symbols to share functions and variables between modules.

The key concepts are:

- `extern` declares a function or variable.
- `EXPORT_SYMBOL()` makes a symbol available to other modules.
- `EXPORT_SYMBOL_GPL()` exports a GPL-restricted symbol.
- `Module.symvers` contains exported-symbol information used during builds.
- `depmod` generates module dependency information.
- `modprobe` loads modules and their dependencies.
- The kernel module loader resolves actual symbol addresses and applies relocations.

The essential flow is:

```text
Export symbol
    |
Build modules
    |
Generate dependency metadata
    |
Load provider module
    |
Load consumer module
    |
Resolve symbol
    |
Execute function
```