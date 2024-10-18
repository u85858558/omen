#include <generic/config.h>
#include <omen/managers/mem/pmm.h>
#include <omen/libraries/std/error.h>
#include <omen/libraries/std/math.h>
#include <omen/libraries/std/string.h>
#include <omen/libraries/std/stdint.h>
#include <omen/apps/debug/debug.h>
#include <omen/managers/boot/bootloaders/bootloader.h>
#include <omen/libraries/allocators/buddy_allocator.h>

struct pmm_block main_memory;
buddy_allocator_t * buddy;
uint8_t ready = 0;

void pmm_init() {
    if (ready) {
        kprintf("PMM already initialized\n");
        return;
    }
    uint64_t entries = get_memory_map_entries();
    uint64_t biggest = 0;
    int64_t biggest_index = -1;
    for (uint64_t i = 0; i < entries; i++) {
        kprintf("Entry %d: base: 0x%x, length: 0x%x, type: %d\n", i, get_memory_map_base(i), get_memory_map_length(i), get_memory_map_type(i));
        if (get_memory_map_type(i) == BOOTLOADER_MEMMAP_USABLE && get_memory_map_length(i) > biggest) {
            biggest = get_memory_map_length(i);
            biggest_index = i;
        }
    }

    if (biggest_index == -1) {
        kprintf("No usable memory found\n");
        return;
    }

    main_memory.base = get_memory_map_base(biggest_index);
    main_memory.size = get_memory_map_length(biggest_index);
    main_memory.type = get_memory_map_type(biggest_index);

    //TODO: Jonbardo, xq?
    uint64_t prev = 0;
    for (uint64_t i = 0; i < 64; i++) {
        if ((1 << i) >= main_memory.size) {
            break;
        }
        prev = 1 << i;
    }

    kprintf("Real: %llx Used: %llx Lost: %llx\n", main_memory.size, prev, main_memory.size - prev);
    main_memory.size = prev;

    buddy = buddy_create((void*)main_memory.base, main_memory.size, PAGE_SIZE);

    if (buddy == NULL) {
        kprintf("Failed to create buddy allocator\n");
        ready = 0;
    } else {
        ready = 1;
    }
}

struct pmm_block * get_main_memory() {
    return &main_memory;
}

void pmm_list_map() {
    if (!ready) {
        return;
    }
    uint64_t entries = get_memory_map_entries();
    kprintf("Memory map entries: %d\n", entries);
    for (uint64_t i = 0; i < entries; i++) {
        kprintf("Entry %d: base: 0x%x, length: 0x%x, type: %d\n", i, get_memory_map_base(i), get_memory_map_length(i), get_memory_map_type(i));
    }
}

void * pmm_alloc(uint64_t size) {
    if (!ready) {
        return NULL;
    }
    return buddy_alloc(buddy, size);
}

void pmm_free(void * ptr) {
    if (!ready) {
        return;
    }
    buddy_free(buddy, ptr);
}