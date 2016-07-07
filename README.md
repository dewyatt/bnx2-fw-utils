# bnx2-fw-utils

These are utilities I wrote for GSoC years ago for dealing with bnx2 firmware files ([like these](http://git.kernel.org/cgit/linux/kernel/git/firmware/linux-firmware.git/tree/bnx2?id=HEAD)).

I'm adding them here for archival purposes.

## fw_info
This is a simple tool to display information about a bnx2 firmware file.

The firmware filename must contain “rv2p” or “mips” to identify the firmware type.

Example:
```
./fw_info ./bnx2_bnx2-rv2p-06-6.0.15.fw
./fw_info ./bnx2_bnx2-mips-06-6.2.3.fw
```
## fw_elf

This is a tool to create an ELF executable from a bnx2 firmware file.
```
Usage: ./fw_elf <mips.fw> <com|cp|rxp|tpat|txp> <out.elf>
```
Example:
```
[daniel@daniel-pc gsoc]$ ./fw_elf bnx2_bnx2-mips-06-6.2.3.fw txp txp.elf
Done
```
This produces the file txp.elf with appropriate .text, .data, and .rodata sections.
This file can then be easily loaded into a disassembler/debugger.
