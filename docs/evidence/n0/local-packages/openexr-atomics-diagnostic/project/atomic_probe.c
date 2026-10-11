#include <stdint.h>
#include <stdatomic.h>

int main(void)
{
    atomic_uintptr_t value;
    atomic_init(&value, (uintptr_t)0);
    atomic_store_explicit(&value, (uintptr_t)17, memory_order_release);
    return atomic_load_explicit(&value, memory_order_acquire) == (uintptr_t)17 ? 0 : 1;
}