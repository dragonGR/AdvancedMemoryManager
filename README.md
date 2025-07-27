# AdvancedMemoryManager

## Overview
AdvancedMemoryManager is a comprehensive C library designed to showcase advanced memory management techniques.
It includes custom memory allocation, deallocation, reallocation, and memory copying functions, demonstrating sophisticated use of pointers, dynamic memory handling, and additional features like alignment, reference counting, memory pooling, and a conceptual defragmentation mechanism in C.

## Features
- **Custom Memory Allocation and Deallocation:** Manage memory blocks manually.
- **Memory Reallocation:** Change the size of existing allocated blocks.
- **Memory Copying with Error Handling:** Safely duplicate memory blocks.
- **Alignment Support:** Allocate memory aligned to specific boundaries (e.g., for performance or hardware requirements).
- **Detailed Memory Block Management:** Track allocated blocks using a linked list.
- **Reference Counting:** Prevent premature deallocation of shared memory blocks.
- **Memory Pooling:** Efficiently allocate and deallocate fixed-size blocks, reducing fragmentation and allocation overhead for common sizes.
- **Memory Defragmentation (Conceptual):** Includes a function demonstrating a basic approach to merging adjacent free blocks (with known limitations).
- **Example Usage:** A `main` function demonstrating the core functionalities.

## Getting Started
### Prerequisites
- GCC or any standard C compiler

### Building and Running
1.  Save the provided C code (e.g., as `mem_manager.c`).
2.  Compile the code:
    ```bash
    gcc -o mem_manager mem_manager.c
    ```
3.  Run the executable:
    ```bash
    ./mem_manager
    ```

## Code Overview
### `mem_manager.c`
The main file containing the implementation of the memory management library.

#### Key Data Structures:
- `MemBlock`: Represents a single managed memory block, storing its size, aligned pointer, reference count, and link to the next block.
- `MemPool`: Manages a pool of fixed-size memory blocks, maintaining a free list for fast allocation.
- `MemoryManager`: The central structure holding pointers to the list of active `MemBlock`s and the list of `MemPool`s.

#### Key Functions:
- `MemoryManager* create_memory_manager()`: Initializes and returns a new memory manager instance.
- `void* allocate_memory(MemoryManager* manager, size_t size, size_t alignment)`: Allocates a block of memory of at least `size` bytes, aligned to the specified `alignment` boundary. It first attempts to use a suitable memory pool; if none is available, it allocates directly from the heap.
- `void increment_ref_count(MemoryManager* manager, void* ptr)`: Increases the reference count for the memory block associated with `ptr`.
- `void decrement_ref_count(MemoryManager* manager, void* ptr)`: Decreases the reference count for the memory block associated with `ptr`. If the count reaches zero, it triggers deallocation.
- `void deallocate_memory(MemoryManager* manager, void* ptr)`: Handles the deallocation of a memory block. If the block originated from a pool, it's returned to that pool's free list. Otherwise, it's freed back to the system heap (though current implementation has a limitation here).
- `void* reallocate_memory(MemoryManager* manager, void* ptr, size_t new_size, size_t alignment)`: Changes the size of the memory block pointed to by `ptr` to `new_size`, maintaining the specified `alignment`. (Note: Current implementation has a limitation regarding pointer alignment during reallocation).
- `void* copy_memory(MemoryManager* manager, void* src, size_t size)`: Allocates a new block of memory and copies `size` bytes from `src` into it.
- `void free_memory_manager(MemoryManager* manager)`: Frees all memory associated with the manager, including active blocks, pool blocks, and the manager structure itself. (Note: Current implementation has a limitation that might lead to memory leaks due to how raw pointers are tracked).
- `void print_memory_blocks(MemoryManager* manager)`: Prints a list of currently allocated memory blocks and the status of memory pools.
- `void defragment_memory(MemoryManager* manager)`: A conceptual implementation attempting to merge adjacent free blocks in the active list. (Note: The current logic is flawed for true defragmentation).
- `void create_memory_pool(MemoryManager* manager, size_t block_size, size_t block_count, size_t alignment)`: Creates a new memory pool capable of holding `block_count` blocks, each of `block_size` bytes, with internal alignment.
- `void* allocate_from_pool(MemoryManager* manager, size_t size, size_t alignment)`: Internal function used by `allocate_memory` to try and satisfy an allocation request from an existing memory pool.

## Example
The `main` function demonstrates usage by:
1.  Creating two memory pools with different block sizes and alignments.
2.  Allocating an array of integers with alignment.
3.  Incrementing the reference count of the array.
4.  Reallocating the array to a larger size with alignment.
5.  Copying the (reallocated) array to a new memory block.
6.  Printing the reallocated and copied arrays.
7.  Printing details of managed memory blocks.
8.  Decrementing the reference count of the reallocated array twice to trigger its deallocation.
9.  Decrementing the reference count of the copied array to deallocate it.
10. Printing memory blocks after deallocation.
11. Attempting to defragment memory.
12. Printing memory blocks after the defragmentation attempt.
13. Freeing the entire memory manager.

## Important Notes and Limitations
- **Pointer Management:** The current implementation stores the *aligned* pointer (`block->ptr`) but often needs the *original raw pointer* returned by `malloc` to correctly `free` or `realloc` the entire block. This can lead to memory leaks or incorrect behavior in `deallocate_memory` and `reallocate_memory`. A robust implementation would store both the raw pointer and the aligned pointer.
- **Defragmentation:** The `defragment_memory` function attempts to merge blocks in the active list based on adjacency and reference count. True defragmentation is complex and usually operates on *free* blocks within a managed heap region, which this implementation does not fully replicate.
- **Pool Deallocation:** Returning blocks correctly to their originating pools relies on heuristics (like block size) which might not be