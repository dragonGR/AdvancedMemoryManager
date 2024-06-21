#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Custom memory block structure
typedef struct MemoryBlock {
    size_t size;
    void* ptr;
    int ref_count; // Reference count for the block
    struct MemoryBlock* next;
} MemoryBlock;

// Memory manager structure
typedef struct {
    MemoryBlock* head;
} MemoryManager;

// Function prototypes
MemoryManager* create_memory_manager();
void* allocate_memory(MemoryManager* manager, size_t size);
void increment_ref_count(MemoryManager* manager, void* ptr);
void decrement_ref_count(MemoryManager* manager, void* ptr);
void deallocate_memory(MemoryManager* manager, void* ptr);
void* reallocate_memory(MemoryManager* manager, void* ptr, size_t new_size);
void* copy_memory(MemoryManager* manager, void* src, size_t size);
void free_memory_manager(MemoryManager* manager);
void print_memory_blocks(MemoryManager* manager);

// Main function
int main() {
    MemoryManager* manager = create_memory_manager();
    if (!manager) {
        fprintf(stderr, "Failed to create memory manager\n");
        return 1;
    }

    // Allocate memory
    int* array = (int*)allocate_memory(manager, 10 * sizeof(int));
    if (!array) {
        fprintf(stderr, "Failed to allocate memory\n");
        free_memory_manager(manager);
        return 1;
    }
    for (int i = 0; i < 10; i++) {
        array[i] = i + 1;
    }

    // Increment reference count
    increment_ref_count(manager, array);

    // Reallocate memory
    array = (int*)reallocate_memory(manager, array, 20 * sizeof(int));
    if (!array) {
        fprintf(stderr, "Failed to reallocate memory\n");
        free_memory_manager(manager);
        return 1;
    }
    for (int i = 10; i < 20; i++) {
        array[i] = i + 1;
    }

    // Print reallocated array
    printf("Reallocated array: ");
    for (int i = 0; i < 20; i++) {
        printf("%d ", array[i]);
    }
    printf("\n");

    // Copy memory
    int* copy = (int*)copy_memory(manager, array, 20 * sizeof(int));
    if (!copy) {
        fprintf(stderr, "Failed to copy memory\n");
        free_memory_manager(manager);
        return 1;
    }

    // Print copied array
    printf("Copied array: ");
    for (int i = 0; i < 20; i++) {
        printf("%d ", copy[i]);
    }
    printf("\n");

    // Print memory blocks
    print_memory_blocks(manager);

    // Decrement reference count
    decrement_ref_count(manager, array);
    decrement_ref_count(manager, array); // This should trigger deallocation

    // Deallocate copied memory
    decrement_ref_count(manager, copy);

    // Print memory blocks after deallocation
    print_memory_blocks(manager);

    // Free memory manager
    free_memory_manager(manager);

    return 0;
}

// Create memory manager
MemoryManager* create_memory_manager() {
    MemoryManager* manager = (MemoryManager*)malloc(sizeof(MemoryManager));
    if (!manager) {
        return NULL;
    }
    manager->head = NULL;
    return manager;
}

// Allocate memory
void* allocate_memory(MemoryManager* manager, size_t size) {
    MemoryBlock* block = (MemoryBlock*)malloc(sizeof(MemoryBlock));
    if (!block) {
        return NULL;
    }
    block->size = size;
    block->ptr = malloc(size);
    if (!block->ptr) {
        free(block);
        return NULL;
    }
    block->ref_count = 1; // Initial reference count is 1
    block->next = manager->head;
    manager->head = block;
    return block->ptr;
}

// Increment reference count
void increment_ref_count(MemoryManager* manager, void* ptr) {
    MemoryBlock* current = manager->head;

    while (current != NULL) {
        if (current->ptr == ptr) {
            current->ref_count++;
            return;
        }
        current = current->next;
    }
}

// Decrement reference count
void decrement_ref_count(MemoryManager* manager, void* ptr) {
    MemoryBlock* current = manager->head;
    MemoryBlock* prev = NULL;

    while (current != NULL) {
        if (current->ptr == ptr) {
            current->ref_count--;
            if (current->ref_count == 0) {
                if (prev != NULL) {
                    prev->next = current->next;
                } else {
                    manager->head = current->next;
                }
                free(current->ptr);
                free(current);
            }
            return;
        }
        prev = current;
        current = current->next;
    }
}

// Reallocate memory
void* reallocate_memory(MemoryManager* manager, void* ptr, size_t new_size) {
    MemoryBlock* current = manager->head;

    while (current != NULL) {
        if (current->ptr == ptr) {
            void* new_ptr = realloc(current->ptr, new_size);
            if (new_ptr == NULL) {
                return NULL; // realloc failed
            }
            current->ptr = new_ptr;
            current->size = new_size;
            return new_ptr;
        }
        current = current->next;
    }

    return NULL; // ptr not found
}

// Copy memory
void* copy_memory(MemoryManager* manager, void* src, size_t size) {
    void* dest = allocate_memory(manager, size);
    if (!dest) {
        return NULL;
    }
    memcpy(dest, src, size);
    return dest;
}

// Free memory manager
void free_memory_manager(MemoryManager* manager) {
    MemoryBlock* current = manager->head;

    while (current != NULL) {
        MemoryBlock* next = current->next;
        free(current->ptr);
        free(current);
        current = next;
    }

    free(manager);
}

// Print memory blocks
void print_memory_blocks(MemoryManager* manager) {
    MemoryBlock* current = manager->head;
    printf("Memory blocks:\n");

    while (current != NULL) {
        printf("Block at %p, size: %zu bytes, ref_count: %d\n", current->ptr, current->size, current->ref_count);
        current = current->next;
    }
    printf("\n");
}
