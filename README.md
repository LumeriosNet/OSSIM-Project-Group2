# demacOprx - OS Simulation for CO2018

Operating System Simulation with DMA (Verilator RTL) for HCMC University of Technology Course CO2018.

## Build full OS simulation - The output file ./os

```bash
make os    # Build full OS simulation. The output file ./os
```
Run os simulation
```bash
# OS simulation
./os [configuration_file]
```

## Build only DMA module - The output file dma_testbench

```bash
make dma 
```
Run DMA module:
```bash
./dma_testbench
./dma_testbench 0x1000 0x2000 1024
```
## demacOprx environment
Ubuntu Virtual Machine image for Intel-x86 and AARCH64 (required HCMUT.SSO system)
[Course CO2018 Documentation](https://docs.google.com/document/d/1vCxl0sBotFjONqRJBAlnkTVYtlL1XZQrTyVl_dO0zxg/edit?usp=sharing "Course CO2018 Documentation - HCMC University of Technology")

## License

See [LICENSE](LICENSE) — educational use only for CO2018.

## Group 2 Implementation

- **Multi-Level Queue (MLQ) Scheduler**: Priority-based scheduling (`0` to `MAX_PRIO - 1`) with slot quotas and automatic replenishment, tracked via `running_list` under thread-safe synchronization (`queue_lock`).
- **Memory Management & Paging (32-bit & 64-bit)**:
  - 32-bit paging with dynamic VMA limit expansion (`inc_vma_limit`), FIFO page replacement, and swap handling.
  - 64-bit multi-level paging with 5-level directory walk (`pt_walk` for PGD, P4D, PUD, PMD, PT), canonical address validation, and on-demand allocation with rollback.
- **Memory Isolation**: User/kernel separation using `mode_bit` in `struct vm_rg_struct`, kernel memory allocation (`__kmalloc`), and safe user-kernel data transfers (`libkmem_copy_from_user` / `libkmem_copy_to_user`).
- **Syscall Handling**: Dual-space system call dispatching, thread-safe caller PCB retrieval in `__sys_memmap`, and custom syscall `sys_xxxhandler`.
- **Asynchronous DMA Sysbus Integration**: Asynchronous DMA mailbox mechanism between CPU and co-processor thread, DPI memory bank & physical RAM redirection (`sysbus_set_mram`), and simulation lifecycle synchronization.

## Team Members

- **Nguyễn Trần Lan Châu** (2452164): MLQ Scheduler (P1)
- **Phạm Trương Hoàng Long** (2452700): 32-bit Paging (P2)
- **Nguyễn Việt Tùng** (2453391): System Calls & Memory Isolation (P3)
- **Belda Jérémie Philippe** (2660010): 64-bit Paging (P4)
- **Morel Pierre Louis** (2660035): Asynchronous DMA Sysbus (P5 & P6), Git Master

