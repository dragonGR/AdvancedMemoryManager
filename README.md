# AdvancedMemoryManager

## Overview
AdvancedMemoryManager is a comprehensive C library designed to showcase advanced memory management techniques.
It includes custom memory allocation, deallocation, and memory copying functions, demonstrating sophisticated use of pointers and dynamic memory handling in C.

## Features
- Custom memory allocation and deallocation
- Memory reallocation
- Memory copying with error handling
- Alignment support for memory allocation
- Detailed memory block management with reference counting
- Memory defragmentation to consolidate free blocks
- Memory pooling for efficient allocation of fixed-size blocks
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
- `void increment_ref_count(MemoryManager* manager, void* ptr)`: Increments the reference count for a memory block.
- `void decrement_ref_count(MemoryManager* manager, void* ptr)`: Decrements the reference count for a memory block and deallocates it if the count reaches zero.
- `void deallocate_memory(MemoryManager* manager, void* ptr)`: Deallocates a specific memory block.
- `void* reallocate_memory(MemoryManager* manager, void* ptr, size_t new_size)`: Reallocates memory to a new size.
- `void* copy_memory(MemoryManager* manager, void* src, size_t size)`: Copies data to a new memory block.
- `void free_memory_manager(MemoryManager* manager)`: Frees all allocated memory and the manager.
- `void print_memory_blocks(MemoryManager* manager)`: Prints details of all managed memory blocks.
- `void defragment_memory(MemoryManager* manager)`: Consolidates adjacent free memory blocks into larger contiguous blocks.
- `void create_memory_pool(MemoryManager* manager, size_t block_size, size_t block_count)`: Creates a memory pool for fixed-size blocks.
- `void* allocate_from_pool(MemoryManager* manager, size_t size)`: Allocates memory from a pool if available.

## Example
The `main` function demonstrates usage by:
1. Creating memory pools with alignment.
2. Allocating an array of integers with alignment.
3. Incrementing the reference count.
4. Reallocating the array to a larger size with alignment.
5. Copying the array with alignment.
6. Printing the reallocated and copied arrays.
7. Printing memory blocks before and after deallocation.
8. Defragmenting memory blocks.
9. Printing memory blocks after defragmentation.
10. Decrementing the reference count to trigger deallocation.

## License
This project is licensed under the MIT License.
