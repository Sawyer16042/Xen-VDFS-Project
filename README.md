Xen VDFS Patch
Dynamically change the readout in /proc/cpuinfo to match the affective speed based on resource allocation
Added the set_freq syscal to the guest system (code for dom0 given, but it can run on all guest types) to change how much of the guests resources it can use.
