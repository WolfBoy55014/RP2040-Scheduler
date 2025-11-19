# RP2040-Scheduler Library Usage

This document explains how to use the RP2040-Scheduler as a library in your own projects.

## Building as a Library

The project is structured to work in two modes:

### 1. Standalone Test Mode
When building this project directly, the test executable (`RP2040-Scheduler`) will be built automatically. This is useful for development and testing the kernel itself.

```bash
mkdir build
cd build
cmake ..
make
```

### 2. Library Mode
When included as a subdirectory in another CMake project, only the `rp2040_kernel` library target is exposed, and the test executable is **not** built.

## Using in Your Project

### Method 1: As a Git Submodule (Recommended)

1. Add this repository as a submodule to your project:
```bash
git submodule add https://github.com/WolfBoy55014/RP2040-Scheduler.git lib/rp2040_kernel
git submodule update --init --recursive
```

2. In your project's `CMakeLists.txt`:
```cmake
cmake_minimum_required(VERSION 3.13)

# ... your pico_sdk_import and project setup ...

# Add the kernel library
add_subdirectory(lib/rp2040_kernel)

# Create your executable
add_executable(my_project
    src/main.c
    # ... your other source files ...
)

# Link against the kernel
target_link_libraries(my_project
    rp2040_kernel
    # ... your other libraries ...
)

pico_add_extra_outputs(my_project)
```

### Method 2: Direct Copy

1. Copy the entire RP2040-Scheduler directory into your project (e.g., `lib/rp2040_kernel/`)

2. Follow the same CMakeLists.txt setup as Method 1

## Configuration

### Using Default Configuration
To use the default kernel configuration, simply include the necessary headers in your code:

```c
#include "scheduler.h"
#include "channel.h"
```

### Custom Configuration
To override the default kernel configuration:

1. Create your own `kernel_config.h` file in your project with `#define CUSTOM_KERNEL_CONFIG`
2. Define your custom settings using the template from `include/kernel_config.h`
3. Make sure your custom config is in an include path that comes **before** the kernel's include path

Example custom configuration:
```c
#ifndef MY_KERNEL_CONFIG_H
#define MY_KERNEL_CONFIG_H

#define CUSTOM_KERNEL_CONFIG

// Your custom configurations
#define CORE_COUNT 1
#define MAX_TASKS 8
#define LOOP_TIME 2
// ... etc ...

#endif
```

Then in your CMakeLists.txt:
```cmake
target_include_directories(my_project PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include  # Your config directory (first!)
)
```

## Example Project Structure

```
my_project/
├── CMakeLists.txt
├── pico_sdk_import.cmake
├── include/
│   └── kernel_config.h          # Optional: your custom config
├── src/
│   └── main.c
└── lib/
    └── rp2040_kernel/            # This kernel as a submodule/copy
        ├── CMakeLists.txt
        ├── include/
        ├── src/
        └── ...
```

## Minimal Example

Here's a minimal example showing how to use the kernel in your project:

```c
#include <stdio.h>
#include <pico/stdlib.h>
#include "scheduler.h"
#include "channel.h"

void my_task(uint32_t pid) {
    printf("Hello from task %lu!\n", pid);
    
    while (true) {
        // Your task logic here
        task_sleep_ms(1000);
    }
}

int main() {
    stdio_init_all();
    
    // Add your tasks
    task_add(my_task, 1, 5);
    
    // Start the kernel
    kernel_start();
    
    // Should never reach here
    while (true) {
        tight_loop_contents();
    }
}
```

## Available APIs

### Scheduler Functions
- `kernel_start()` - Initialize and start the scheduler
- `task_add(function, id, priority)` - Add a new task
- `task_yield()` - Yield control to other tasks
- `task_sleep_ms(ms)` - Sleep for milliseconds
- `task_sleep_us(us)` - Sleep for microseconds
- `task_end(code)` - End the current task
- `task_exists(pid)` - Check if a task exists

### Channel Functions (Inter-task Communication)
- `com_channel_request(pid)` - Request a channel with another task
- `com_channel_free(channel_id)` - Free a channel
- `com_channel_write(channel_id, data, size)` - Write data to a channel
- `com_channel_read(channel_id, buffer, size)` - Read data from a channel
- `is_channel_ready_to_write(channel_id)` - Check if channel is ready
- `is_channel_ready_to_read(channel_id)` - Check if channel has data
- `get_connected_channels(array, size)` - Get list of connected channels

## Testing Your Integration

After setting up the kernel in your project:

1. Build your project:
```bash
cd build
cmake ..
make
```

2. Flash to your RP2040:
```bash
# Copy the .uf2 file to your Pico in bootloader mode
cp my_project.uf2 /path/to/RPI-RP2/
```

## Troubleshooting

### Build Errors
- **Missing headers**: Ensure `add_subdirectory()` comes before `target_link_libraries()`
- **Linker errors**: Make sure you're linking against `rp2040_kernel` and required Pico SDK libraries
- **Multiple definitions**: Check that you're not including kernel source files directly in your executable

### Runtime Issues
- **Stack overflows**: Increase `MAX_STACK_SIZE` or `STARTING_STACK_SIZE` in your custom config
- **Task not running**: Check priority levels and that `kernel_start()` is called
- **Multicore issues**: Ensure `CORE_COUNT` matches your usage

## License

This kernel is subject to the license terms in the LICENSE file of the RP2040-Scheduler repository.
