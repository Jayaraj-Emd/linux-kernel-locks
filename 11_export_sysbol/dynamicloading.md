Append this section to your existing `README.md` under the **Dynamic Loading** section. It documents the experiment where `modprobe` automatically loads the provider module.

## 8. Automatic Dependency Loading Using `modprobe`

### Objective

Understand how `modprobe` automatically loads a provider module when a consumer module depends on its exported symbol.

In this experiment:

- `dep.ko` — Provider module. Exports `my_function()` using `EXPORT_SYMBOL()`.
- `ExSym.ko` — Consumer module. Calls `my_function()`.

### 8.1 Difference Between `insmod` and `modprobe`

| Command | Behavior |
|---|---|
| `insmod ExSym.ko` | Loads only the specified module. Does not automatically load dependencies. |
| `modprobe ExSym` | Loads the module and automatically loads its dependencies if dependency metadata is available. |

`modprobe` requires the modules to be installed in the kernel module directory and the dependency database to be generated.

### 8.2 Install the Modules

First, identify the running kernel version:

```bash
uname -r
```

Create the module installation directory:

```bash
KVER=$(uname -r)

sudo mkdir -p /lib/modules/$KVER/extra
```

Copy the modules:

```bash
sudo cp dep.ko ExSym.ko /lib/modules/$KVER/extra/
```

### 8.3 Generate Dependency Information

Execute:

```bash
sudo depmod -a
```

`depmod` scans installed kernel modules and generates dependency information.

This information is stored in files such as:

- `modules.dep`
- `modules.dep.bin`

These files allow `modprobe` to determine which modules must be loaded first.

### 8.4 Verify the Dependency

Check the dependency of the consumer module:

```bash
modinfo -F depends ExSym
```

Expected output:

```text
dep
```

This indicates that `ExSym` depends on `dep`.

### 8.5 Load the Consumer Module

Unload the modules if they are already loaded:

```bash
sudo rmmod ExSym
sudo rmmod dep
```

Now load only the consumer module:

```bash
sudo modprobe -v ExSym
```

Notice that we did not manually execute:

```bash
sudo insmod dep.ko
```

`modprobe` automatically loads the provider module before loading the consumer.

Verify:

```bash
lsmod | grep -E 'ExSym|dep'
```

Check the kernel log:

```bash
sudo dmesg | tail -10
```

Expected log sequence:

```text
Module A loaded
Module A: my_function called
Module B: Value = 100
```

### 8.6 Unload the Modules

Unload the consumer first:

```bash
sudo modprobe -r ExSym
```

Then unload the provider:

```bash
sudo modprobe -r dep
```

The consumer must be unloaded before the provider because the consumer depends on the provider's exported function.

### 8.7 Execution Flow

```text
                    depmod -a
                       |
                       v
            Dependency information generated
                       |
                       v
                 modprobe ExSym
                       |
                       v
            Check ExSym dependencies
                       |
                       v
                dep.ko required
                       |
                       v
                 Load dep.ko
                       |
                       v
             my_function() exported
                       |
                       v
                 Load ExSym.ko
                       |
                       v
          Resolve my_function() reference
                       |
                       v
              Consumer module loaded
                       |
                       v
              ExSym calls my_function()
```

### 8.8 Important Concepts

- `EXPORT_SYMBOL()` exposes a symbol to other kernel modules.
- `Module.symvers` provides build-time information about exported symbols.
- `depmod` generates module dependency metadata.
- `modprobe` uses dependency metadata to load required modules.
- `insmod` does not automatically load dependencies.
- `modprobe` uses installed module names, not `.ko` file paths.

**Key takeaway:** `modprobe` simplifies module loading by automatically handling dependencies. After the initial installation and dependency setup, loading the consumer requires only:

```bash
sudo modprobe ExSym
```

This section can be appended directly to your existing README.