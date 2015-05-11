Xen VDFS Patch
Dynamically change the readout in /proc/cpuinfo to match the effective speed based on resource allocation
Added the set_freq syscall to the guest system (code for dom0 given, but it can run on all guest types) to change how much CPU speed it can use.
