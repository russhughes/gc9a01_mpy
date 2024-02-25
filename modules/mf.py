import gc

print("Allocating 1K bytes until memory is full... ", end='')
memory = []
i = 0
gc.collect()

try:
    while True:
        memory.append(bytearray(1024))
        i += 1

except MemoryError:
    gc.collect()
    print(f'{i}K bytes Allocated.')
