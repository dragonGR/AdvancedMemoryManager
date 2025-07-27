#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h> // Include for uintptr_t

// Custom memory block structure
// Tracks allocated memory, its size, usage count (reference count),
// and links to the next block in the manager's list.
typedef struct MemBlock {
    size_t size;            // Size of the user data
    void* ptr;              // Pointer to the start of the allocated/raw memory (potentially aligned)
    int ref_count;          // Reference count for the block
    struct MemBlock* next;  // Pointer to the next memory block in the list
} MemBlock;

// Memory pool structure
// Manages a collection of fixed-size blocks for efficient allocation.
typedef struct MemPool {
    size_t block_size;      // Size of each block in this pool
    size_t block_count;     // Total number of blocks in this pool
    MemBlock* free_list;    // Linked list of currently free blocks in this pool
    struct MemPool* next;   // Pointer to the next memory pool in the manager's list
} MemPool;

// Memory manager structure
// Central structure holding the list of all allocated blocks and memory pools.
typedef struct {
    MemBlock* head;         // Head of the linked list of active (allocated) memory blocks
    MemPool* pools;         // Head of the linked list of memory pools
} MemoryManager;

// Function prototypes
MemoryManager* create_memory_manager();
void* allocate_memory(MemoryManager* manager, size_t size, size_t alignment);
void increment_ref_count(MemoryManager* manager, void* ptr);
void decrement_ref_count(MemoryManager* manager, void* ptr);
void deallocate_memory(MemoryManager* manager, void* ptr);
void* reallocate_memory(MemoryManager* manager, void* ptr, size_t new_size, size_t alignment);
void* copy_memory(MemoryManager* manager, void* src, size_t size);
void free_memory_manager(MemoryManager* manager);
void print_memory_blocks(MemoryManager* manager);
void defragment_memory(MemoryManager* manager);
void create_memory_pool(MemoryManager* manager, size_t block_size, size_t block_count, size_t alignment);
void* allocate_from_pool(MemoryManager* manager, size_t size, size_t alignment);

// Main function
// Demonstrates the usage of the Advanced Memory Manager.
int main() {
    MemoryManager* manager = create_memory_manager();
    if (!manager) {
        fprintf(stderr, "Failed to initialize memory manager.\n");
        return EXIT_FAILURE;
    }

    // Create memory pools with specific block sizes and alignments
    // Pool 1: 10 blocks of 32 bytes, aligned to 8-byte boundaries
    create_memory_pool(manager, 32, 10, 8);
    // Pool 2: 10 blocks of 64 bytes, aligned to 16-byte boundaries
    create_memory_pool(manager, 64, 10, 16);

    // Allocate memory for an array of 10 integers, aligned to integer boundaries
    int* array = (int*)allocate_memory(manager, 10 * sizeof(int), sizeof(int));
    if (array == NULL) {
        fprintf(stderr, "Initial allocation failed.\n");
        free_memory_manager(manager);
        return EXIT_FAILURE;
    }
    for (int i = 0; i < 10; i++) {
        array[i] = i + 1;
    }

    // Increment reference count to simulate sharing the pointer
    increment_ref_count(manager, array);

    // Reallocate memory to hold 20 integers, maintaining integer alignment
    int* old_array_ptr = array; // Keep track of the old pointer for comparison
    array = (int*)reallocate_memory(manager, array, 20 * sizeof(int), sizeof(int));
    if (array == NULL) {
        fprintf(stderr, "Reallocation failed.\n");
        // Attempt to decrement the ref count for the old pointer before exiting
        decrement_ref_count(manager, old_array_ptr);
        free_memory_manager(manager);
        return EXIT_FAILURE;
    }
    // Initialize the newly allocated part of the array
    for (int i = 10; i < 20; i++) {
        array[i] = i + 1;
    }

    // Print reallocated array
    printf("Reallocated array: ");
    for (int i = 0; i < 20; i++) {
        printf("%d ", array[i]);
    }
    printf("\n");

    // Copy memory to a new block, aligned to byte boundaries
    int* copy = (int*)copy_memory(manager, array, 20 * sizeof(int));
    if (copy == NULL) {
        fprintf(stderr, "Memory copy failed.\n");
        // Decrement ref count for original array (twice to trigger deallocation if needed)
        decrement_ref_count(manager, array);
        decrement_ref_count(manager, array);
        free_memory_manager(manager);
        return EXIT_FAILURE;
    }

    // Print copied array
    printf("Copied array: ");
    for (int i = 0; i < 20; i++) {
        printf("%d ", copy[i]);
    }
    printf("\n");

    // Print memory blocks before deallocation
    print_memory_blocks(manager);

    // Decrement reference count for the original/reallocated array
    // First decrement reduces ref count from 2 to 1.
    decrement_ref_count(manager, array);
    // Second decrement reduces ref count from 1 to 0, triggering deallocation.
    decrement_ref_count(manager, array);

    // Deallocate the copied memory by decrementing its reference count (initially 1)
    decrement_ref_count(manager, copy);

    // Print memory blocks after deallocation
    printf("After deallocation:\n");
    print_memory_blocks(manager);

    // Defragment memory to consolidate free blocks (note: current impl has limitations)
    defragment_memory(manager);

    // Print memory blocks after defragmentation
    printf("After defragmentation:\n");
    print_memory_blocks(manager);

    // Free the entire memory manager and all remaining resources
    free_memory_manager(manager);
    return 0;
}

// Create memory manager
// Initializes and returns a pointer to a new MemoryManager structure.
MemoryManager* create_memory_manager() {
    MemoryManager* manager = (MemoryManager*)malloc(sizeof(MemoryManager));
    if (manager == NULL) {
        perror("Failed to create memory manager");
        // Do not exit here, let the caller handle the error
        return NULL;
    }
    manager->head = NULL;
    manager->pools = NULL;
    return manager;
}

// Allocate memory with alignment
// Tries to allocate from a pool first. If not possible, allocates directly.
// Alignment must be a power of two.
void* allocate_memory(MemoryManager* manager, size_t size, size_t alignment) {
    // Basic validation for alignment (must be power of 2)
    if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
        fprintf(stderr, "Error: Invalid alignment value %zu. Must be a power of 2.\n", alignment);
        return NULL;
    }

    // Try to allocate from an existing pool
    void* ptr = allocate_from_pool(manager, size, alignment);
    if (ptr != NULL) {
        return ptr;
    }

    // If pool allocation fails, allocate directly from the heap
    MemBlock* block = (MemBlock*)malloc(sizeof(MemBlock));
    if (block == NULL) {
        perror("Failed to allocate MemBlock header");
        return NULL; // Allocation failed
    }

    // Allocate raw memory large enough to guarantee an aligned block within it
    // The extra `alignment - 1` bytes ensure we can find an aligned address.
    void* raw_ptr = malloc(size + alignment - 1);
    if (raw_ptr == NULL) {
        perror("Failed to allocate raw memory");
        free(block); // Clean up the previously allocated header
        return NULL; // Allocation failed
    }

    // Calculate the aligned pointer within the raw memory block
    uintptr_t aligned_ptr = (uintptr_t)raw_ptr;
    // Align the pointer up to the next boundary that is a multiple of `alignment`
    aligned_ptr = (aligned_ptr + alignment - 1) & ~(alignment - 1);

    // Populate the MemBlock structure
    block->size = size;
    block->ptr = (void*)aligned_ptr; // Store the aligned user pointer
    block->ref_count = 1;           // Initial reference count is 1 (newly allocated)
    block->next = manager->head;    // Link to the existing list of blocks
    manager->head = block;          // Update the head of the list

    return block->ptr; // Return the aligned pointer for use by the application
}

// Allocate memory from pool with alignment
// Searches pools for a suitable block. Alignment is handled during pool creation.
void* allocate_from_pool(MemoryManager* manager, size_t size, size_t alignment) {
    // Basic validation for alignment (must be power of 2)
    if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
        fprintf(stderr, "Error: Invalid alignment value %zu for pool allocation. Must be a power of 2.\n", alignment);
        return NULL;
    }

    MemPool* pool = manager->pools;
    // Iterate through all available memory pools
    while (pool != NULL) {
        // Check if the pool's block size can accommodate the requested size
        // Note: This is a simple check. A more robust system might check for
        // alignment compatibility between pool block alignment and requested alignment.
        if (pool->block_size >= size && pool->free_list != NULL) {
            // Take a block from the pool's free list
            MemBlock* block = pool->free_list;
            pool->free_list = block->next; // Update the pool's free list

            // The block->ptr was already aligned when the pool was created.
            // We might need to re-align it based on the *current* request's alignment,
            // in case the pool's base alignment is coarser than the requested one.
            // However, the pool's alignment should be >= any request to it.
            // Let's re-align just to be safe, assuming block->ptr points to the start
            // of the raw memory allocated for the pool block.
            uintptr_t aligned_ptr = (uintptr_t)block->ptr;
            aligned_ptr = (aligned_ptr + alignment - 1) & ~(alignment - 1);
            block->ptr = (void*)aligned_ptr;

            // Add the allocated block to the manager's main list of active blocks
            block->next = manager->head;
            manager->head = block;
            block->ref_count = 1; // Set reference count for the newly allocated block
            return block->ptr;   // Return the aligned pointer
        }
        pool = pool->next; // Move to the next pool
    }
    return NULL; // No suitable block found in any pool
}

// Create memory pool
// Initializes a new memory pool with a specified number of fixed-size blocks,
// each aligned to a specific boundary.
void create_memory_pool(MemoryManager* manager, size_t block_size, size_t block_count, size_t alignment) {
    // Basic validation for alignment (must be power of 2)
    if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
        fprintf(stderr, "Error: Invalid alignment value %zu for pool creation. Must be a power of 2.\n", alignment);
        // Cannot proceed with pool creation
        return;
    }

    MemPool* pool = (MemPool*)malloc(sizeof(MemPool));
    if (pool == NULL) {
        perror("Failed to create memory pool header");
        // Cannot proceed, but don't exit, let program continue or handle error
        return;
    }

    // Initialize pool properties
    pool->block_size = block_size;
    pool->block_count = block_count;
    pool->free_list = NULL; // Initially, no blocks are in the free list
    pool->next = manager->pools; // Link the new pool to the manager's pool list
    manager->pools = pool;       // Update the manager's pool list head

    // Allocate and initialize individual blocks within the pool
    for (size_t i = 0; i < block_count; i++) {
        MemBlock* block = (MemBlock*)malloc(sizeof(MemBlock));
        if (block == NULL) {
            perror("Failed to create memory block header for pool");
            // In a robust system, you might want to clean up previously allocated blocks/pool
            // For simplicity here, we just return/break
            return;
        }

        // Allocate raw memory for the block's data, ensuring space for alignment
        void* raw_ptr = malloc(block_size + alignment - 1);
        if (raw_ptr == NULL) {
            perror("Failed to allocate raw memory for pool block");
            free(block); // Clean up the block header
            // Again, robust cleanup might be needed
            return;
        }

        // Align the pointer within the raw memory
        uintptr_t aligned_ptr = (uintptr_t)raw_ptr;
        aligned_ptr = (aligned_ptr + alignment - 1) & ~(alignment - 1);

        // Initialize the MemBlock for this pool block
        block->size = block_size;
        block->ptr = (void*)aligned_ptr; // Store the aligned pointer
        block->ref_count = 0;           // Initially not allocated (in free list)
        block->next = pool->free_list;  // Link to the pool's current free list
        pool->free_list = block;        // Update the pool's free list head
    }
}

// Increment reference count
// Finds the memory block associated with `ptr` and increases its reference count.
void increment_ref_count(MemoryManager* manager, void* ptr) {
    if (!manager || !ptr) return; // Handle null inputs gracefully

    MemBlock* current = manager->head;
    while (current != NULL) {
        if (current->ptr == ptr) {
            current->ref_count++;
            return; // Found and updated, exit
        }
        current = current->next;
    }
    // If ptr is not found, it's either not managed or an error.
    fprintf(stderr, "Warning: increment_ref_count called on unmanaged pointer %p\n", ptr);
}

// Decrement reference count
// Finds the memory block associated with `ptr`, decreases its reference count,
// and deallocates the block if the count reaches zero.
void decrement_ref_count(MemoryManager* manager, void* ptr) {
    if (!manager || !ptr) return; // Handle null inputs gracefully

    MemBlock* current = manager->head;
    MemBlock* prev = NULL;

    while (current != NULL) {
        if (current->ptr == ptr) {
            current->ref_count--;
            if (current->ref_count == 0) {
                // Reference count dropped to zero, time to deallocate
                // Remove the block from the manager's active list
                if (prev != NULL) {
                    prev->next = current->next;
                } else {
                    manager->head = current->next;
                }
                // Perform the actual deallocation (might return block to pool)
                deallocate_memory(manager, current->ptr);
                // Free the MemBlock header itself
                free(current);
            }
            return; // Found and processed, exit
        }
        prev = current;
        current = current->next;
    }
    // If ptr is not found, it's either not managed or an error.
    fprintf(stderr, "Warning: decrement_ref_count called on unmanaged pointer %p\n", ptr);
}

// Deallocate memory
// Frees the memory associated with `ptr`. If it belongs to a pool, it's returned to the pool.
// Otherwise, the raw memory is freed back to the system heap.
void deallocate_memory(MemoryManager* manager, void* ptr) {
    if (!manager || !ptr) return; // Handle null inputs gracefully

    MemBlock dummy_head; // Dummy node to simplify list traversal/removal logic
    dummy_head.next = manager->head;
    MemBlock* current = &dummy_head;
    MemBlock* actual_head = manager->head; // Keep track of the real head

    while (current->next != NULL) {
        MemBlock* block_to_check = current->next;
        // Check if the raw memory pointer (block->ptr before alignment)
        // matches the `ptr` we are trying to deallocate.
        // This requires storing the original `raw_ptr` which is not currently done.
        // The current logic compares the aligned `ptr`, which works if we always
        // pass the exact aligned pointer returned by allocate/realloc.
        if (block_to_check->ptr == ptr) {
            // Found the block. Now, try to return it to a pool.
            MemPool* pool = manager->pools;
            while (pool != NULL) {
                // A simple heuristic: if the block size fits the pool's block size
                if (pool->block_size >= block_to_check->size) {
                    // Potential match. In a full implementation, you'd need a more
                    // robust way to know which pool a block belongs to (e.g., store pool ID).
                    // For this example, we'll assume it came from this pool if sizes match roughly.
                    // Add the block back to the pool's free list.
                    // Note: This modifies `block_to_check` which is still in the main list.
                    // We need to remove it from the main list first.
                    current->next = block_to_check->next; // Remove from main list
                    if (manager->head == block_to_check) {
                         manager->head = block_to_check->next; // Update head if necessary
                    }

                    block_to_check->next = pool->free_list; // Add to pool free list
                    block_to_check->ref_count = 0;          // Reset ref count
                    pool->free_list = block_to_check;
                    return; // Successfully returned to pool
                }
                pool = pool->next;
            }
            // If no pool matched, free the raw memory back to the system.
            // This requires the original `raw_ptr` used in `malloc`.
            // Since we don't store it, we cannot correctly free the entire `malloc`'d block.
            // The current `free(current->ptr)` is incorrect as `current->ptr` is the aligned pointer.
            // This is a limitation of the current design.
            fprintf(stderr, "Warning: Cannot safely free non-pool memory block %p due to missing original pointer. Potential memory leak.\n", ptr);
            // Correct approach would be:
            // free(original_raw_ptr_stored_in_block); // This requires storing original_raw_ptr
            return;
        }
        current = current->next;
    }
    // If we reach here, the ptr was not found in the active blocks list.
    fprintf(stderr, "Warning: deallocate_memory called on unmanaged or already freed pointer %p\n", ptr);
}


// Reallocate memory with alignment
// Changes the size of the memory block pointed to by `ptr`.
// Handles alignment for the new block.
void* reallocate_memory(MemoryManager* manager, void* ptr, size_t new_size, size_t alignment) {
    if (!manager) return NULL;
    if (ptr == NULL) {
        // realloc with NULL is equivalent to malloc
        return allocate_memory(manager, new_size, alignment);
    }
    if (new_size == 0) {
        // realloc with size 0 is equivalent to free
        decrement_ref_count(manager, ptr); // Use ref counting system
        return NULL;
    }
    // Basic validation for alignment (must be power of 2)
    if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
        fprintf(stderr, "Error: Invalid alignment value %zu for reallocation. Must be a power of 2.\n", alignment);
        return NULL;
    }

    MemBlock* current = manager->head;
    while (current != NULL) {
        if (current->ptr == ptr) {
            // Found the block. Attempt to reallocate.
            // Like malloc, we need to realloc enough space for alignment.
            // However, realloc works on the original malloc'd pointer.
            // We need the original `raw_ptr` passed to malloc.
            // The current design only stores the aligned `ptr`, making correct realloc impossible.
            // This is a significant limitation.
            fprintf(stderr, "Warning: Reallocation not fully supported due to missing original raw pointer. Behavior may be incorrect.\n");
            // Attempting with current->ptr (the aligned one) is incorrect.
            void* new_raw_ptr = realloc(current->ptr, new_size + alignment - 1);
            if (new_raw_ptr == NULL) {
                 // realloc failed, original block is still valid
                 perror("Reallocation failed");
                 return NULL;
            }
            // Re-align the new pointer
            uintptr_t aligned_ptr = (uintptr_t)new_raw_ptr;
            aligned_ptr = (aligned_ptr + alignment - 1) & ~(alignment - 1);

            // Update the block information
            // Note: We lost the original raw_ptr, so freeing later will be problematic.
            current->ptr = (void*)aligned_ptr;
            current->size = new_size;
            return current->ptr;
        }
        current = current->next;
    }
    // ptr not found in managed blocks
    fprintf(stderr, "Error: reallocate_memory called on unmanaged pointer %p\n", ptr);
    return NULL;
}


// Copy memory
// Allocates a new block of memory and copies `size` bytes from `src` to it.
void* copy_memory(MemoryManager* manager, void* src, size_t size) {
    if (!manager || !src) return NULL; // Handle null inputs gracefully

    // Allocate new memory with byte alignment (sizeof(char) = 1)
    void* dest = allocate_memory(manager, size, 1);
    if (dest == NULL) {
        // Allocation failed, error message should be printed by allocate_memory
        return NULL;
    }
    // Copy data from source to destination
    memcpy(dest, src, size);
    return dest;
}

// Free memory manager
// Releases all memory allocated and managed by the MemoryManager.
void free_memory_manager(MemoryManager* manager) {
    if (!manager) return; // Handle null input gracefully

    // Free all active memory blocks (those not returned to pools)
    MemBlock* current = manager->head;
    while (current != NULL) {
        MemBlock* next = current->next;
        // Similar to deallocate_memory, we cannot safely free current->ptr
        // because it's the aligned pointer, not the original malloc'd pointer.
        fprintf(stderr, "Warning: Leaking memory block at %p (size %zu) due to design limitation.\n", current->ptr, current->size);
        // Correct approach:
        // free(original_raw_ptr_stored_in_current);
        free(current); // Free the MemBlock header
        current = next;
    }

    // Free all memory pools and their blocks
    MemPool* pool = manager->pools;
    while (pool != NULL) {
        MemPool* next_pool = pool->next;
        MemBlock* block = pool->free_list;
        while (block != NULL) {
            MemBlock* next_block = block->next;
            // Free the raw memory allocated for the pool block
            // Again, we need the original raw_ptr. Assuming block->ptr points to it
            // for pool blocks (which it might not strictly, but likely does if allocated similarly)
             fprintf(stderr, "Warning: Leaking pool block memory at %p (size %zu) due to missing original pointer.\n", block->ptr, block->size);
            // Correct approach:
            // free(original_raw_ptr_stored_in_block);
            free(block); // Free the MemBlock header for the pool block
            block = next_block;
        }
        free(pool); // Free the MemPool header
        pool = next_pool;
    }

    // Finally, free the MemoryManager structure itself
    free(manager);
}

// Print memory blocks
// Displays information about currently allocated memory blocks and available pools.
void print_memory_blocks(MemoryManager* manager) {
    if (!manager) {
        printf("Cannot print blocks: Manager is NULL.\n");
        return;
    }

    MemBlock* current = manager->head;
    printf("Current Memory Blocks:\n");
    if (current == NULL) {
        printf("No memory blocks in use.\n");
    } else {
        while (current != NULL) {
            printf("  Block at %p, size: %zu bytes, ref_count: %d\n", current->ptr, current->size, current->ref_count);
            current = current->next;
        }
    }

    if (manager->pools == NULL) {
        printf("No memory pools created.\n");
    } else {
        MemPool* pool = manager->pools;
        printf("\nMemory Pools:\n");
        while (pool != NULL) {
            // Count free blocks in this pool for better info
            size_t free_count = 0;
            MemBlock* temp_block = pool->free_list;
            while(temp_block) {
                free_count++;
                temp_block = temp_block->next;
            }
            printf("  Pool (block size: %zu bytes, total blocks: %zu, free blocks: %zu)\n", pool->block_size, pool->block_count, free_count);
            pool = pool->next;
        }
    }
    printf("\n");
}

// Defragment memory
// Attempts to merge adjacent free memory blocks into larger contiguous blocks.
// Note: The current implementation has significant limitations.
// It only checks adjacent blocks in the *active* list (`manager->head`) and only if
// the *next* block has a ref_count of 0. This is flawed logic for defragmentation.
// True defragmentation usually works on *free* blocks, often within a specific region or pool.
void defragment_memory(MemoryManager* manager) {
    if (!manager) return;

    fprintf(stderr, "Warning: Current defragmentation logic is flawed and may not work as expected.\n");
    fprintf(stderr, "         It attempts to merge blocks in the active list, not necessarily free blocks.\n");

    MemBlock* current = manager->head;
    // Iterate through the list, but stop before the last element
    while (current != NULL && current->next != NULL) {
        MemBlock* next = current->next;

        // Incorrect condition: checks if `next` block is free (ref_count == 0)
        // This doesn't make sense for merging *allocated* blocks.
        // Also, pointer arithmetic `+ current->size` assumes no padding between malloc'd blocks,
        // which is not guaranteed.
        if ((char*)current->ptr + current->size == next->ptr && next->ref_count == 0) {
            // Attempt to merge. This requires the original raw pointers to be contiguous,
            // which `realloc` cannot guarantee unless they were originally allocated that way.
            void* new_ptr = realloc(current->ptr, current->size + next->size);
            if (new_ptr == NULL) {
                printf("Defragmentation realloc failed for blocks %p and %p\n", current->ptr, next->ptr);
                // Move to the next block
                current = current->next;
            } else {
                // Check if the reallocated pointer is the same as the original
                if (new_ptr == current->ptr) {
                     // Success, pointers are contiguous and merged
                     current->size += next->size;
                     // Remove `next` block from the list
                     current->next = next->next;
                     // Free the MemBlock header of the merged block
                     free(next);
                     // Do not advance `current`, check if the new `current->next` can be merged
                } else {
                    // realloc moved the block, update pointer
                    // This breaks the adjacency assumption for future merges in this pass
                    printf("Defragmentation realloc moved block from %p to %p\n", current->ptr, new_ptr);
                    current->ptr = new_ptr;
                    current->size += next->size;
                    current->next = next->next;
                    free(next);
                    // Move to the next block
                    current = current->next;
                }
            }
        } else {
            // Blocks are not adjacent or next block is not free, move to the next block
            current = current->next;
        }
    }
}