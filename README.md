# AdvancedMemoryManager

## Overview
AdvancedMemoryManager is a comprehensive C library designed to showcase advanced memory management techniques.
It includes custom memory allocation, deallocation, and memory copying functions, demonstrating sophisticated use of pointers and dynamic memory handling in C.

## Features
- Custom memory allocation and deallocation
- Memory copying with error handling
- Detailed memory block management
- Example usage in the `main` function

## Getting Started
### Prerequisites
- GCC or any C compiler

## Code Overview
### `mem_manager.c`
The main file containing the implementation of the memory management library.

#### Key Functions:
- `MemoryManager* create_memory_manager()`: Initializes a memory manager.
- `void* allocate_memory(MemoryManager* manager, size_t size)`: Allocates memory and tracks it.
- `void deallocate_memory(MemoryManager* manager, void* ptr)`: Deallocates a specific memory block.
- `void* reallocate_memory(MemoryManager* manager, void* ptr, size_t new_size)`: Reallocates memory to a new size.
- `void* copy_memory(MemoryManager* manager, void* src, size_t size)`: Copies data to a new memory block.
- `void free_memory_manager(MemoryManager* manager)`: Frees all allocated memory and the manager.
- `void print_memory_blocks(MemoryManager* manager)`: Prints details of all managed memory blocks.

## Example
The `main` function demonstrates usage by:
1. Allocating an array of integers.
2. Reallocating the array to a larger size.
3. Copying the array.
4. Printing the copied array.
5. Printing memory blocks before and after deallocation.

## License
This project is licensed under the MIT License.
