#ifndef __VDFS_PROCESSOR_H
#define __VDFS_PROCESSOR_H

#include <linux/kernel.h>
#include <linux/cpu.h>
#include <linux/thermal.h>
//#include <asm/vdfs.h>

#define VDFS_PROCESSOR_CLASS		"processor"
#define VDFS_PROCESSOR_DEVICE_NAME	"Processor"
#define VDFS_PROCESSOR_DEVICE_HID	"ACPI0007"

#define VDFS_PROCESSOR_BUSY_METRIC	10

#define VDFS_PROCESSOR_MAX_POWER	8
#define VDFS_PROCESSOR_MAX_C2_LATENCY	100
#define VDFS_PROCESSOR_MAX_C3_LATENCY	1000

#define VDFS_PROCESSOR_MAX_THROTTLING	16
#define VDFS_PROCESSOR_MAX_THROTTLE	250	/* 25% */
#define VDFS_PROCESSOR_MAX_DUTY_WIDTH	4

#define VDFS_PDC_REVISION_ID		0x1

#define VDFS_PSD_REV0_REVISION		0	/* Support for _PSD as in ACPI 3.0 */
#define VDFS_PSD_REV0_ENTRIES		5

#define VDFS_TSD_REV0_REVISION		0	/* Support for _PSD as in ACPI 3.0 */
#define VDFS_TSD_REV0_ENTRIES		5
/*
 * Types of coordination defined in VDFS 3.0. Same macros can be used across
 * P, C and T states
 */
#define DOMAIN_COORD_TYPE_SW_ALL	0xfc
#define DOMAIN_COORD_TYPE_SW_ANY	0xfd
#define DOMAIN_COORD_TYPE_HW_ALL	0xfe

#define VDFS_CSTATE_SYSTEMIO	0
#define VDFS_CSTATE_FFH		1
#define VDFS_CSTATE_HALT	2

#define VDFS_CX_DESC_LEN	32

/* Power Management *

struct vdfs_processor_cx;

struct vdfs_power_register {
	u8 descriptor;
	u16 length;
	u8 space_id;
	u8 bit_width;
	u8 bit_offset;
	u8 access_size;
	u64 address;
} __attribute__ ((packed));

struct vdfs_processor_cx {
	u8 valid;
	u8 type;
	u32 address;
	u8 entry_method;
	u8 index;
	u32 latency;
	u8 bm_sts_skip;
	char desc[VDFS_CX_DESC_LEN];
};

struct vdfs_processor_power {
	struct vdfs_processor_cx *state;
	unsigned long bm_check_timestamp;
	u32 default_state;
	int count;
	struct vdfs_processor_cx states[VDFS_PROCESSOR_MAX_POWER];
	int timer_broadcast_on_state;
};

/* Performance Management */

struct vdfs_psd_package {
	u64 num_entries;
	u64 revision;
	u64 domain;
	u64 coord_type;
	u64 num_processors;
} __attribute__ ((packed));

struct vdfs_pct_register {
	u8 descriptor;
	u16 length;
	u8 space_id;
	u8 bit_width;
	u8 bit_offset;
	u8 reserved;
	u64 address;
} __attribute__ ((packed));

struct vdfs_processor_px {
	u64 core_frequency;	/* megahertz */
	u64 power;	/* milliWatts */
	u64 transition_latency;	/* microseconds */
	u64 bus_master_latency;	/* microseconds */
	u64 control;	/* control value */
	u64 status;	/* success indicator */
};
//KEEP
struct vdfs_processor_performance {
	unsigned int state;
	unsigned int platform_limit;
	struct vdfs_pct_register control_register;
	struct vdfs_pct_register status_register;
	unsigned int state_count;
	struct vdfs_processor_px *states;
	struct vdfs_psd_package domain_info;
	cpumask_var_t shared_cpu_map;
	unsigned int shared_type;
};/*

/* Throttling Control *

struct vdfs_tsd_package {
	u64 num_entries;
	u64 revision;
	u64 domain;
	u64 coord_type;
	u64 num_processors;
} __attribute__ ((packed));

struct vdfs_ptc_register {
	u8 descriptor;
	u16 length;
	u8 space_id;
	u8 bit_width;
	u8 bit_offset;
	u8 reserved;
	u64 address;
} __attribute__ ((packed));

struct vdfs_processor_tx_tss {
	u64 freqpercentage;	/* *
	u64 power;	/* milliWatts *
	u64 transition_latency;	/* microseconds *
	u64 control;	/* control value *
	u64 status;	/* success indicator *
};
struct vdfs_processor_tx {
	u16 power;
	u16 performance;
};

struct vdfs_processor;
struct vdfs_processor_throttling {
	unsigned int state;
	unsigned int platform_limit;
	struct vdfs_pct_register control_register;
	struct vdfs_pct_register status_register;
	unsigned int state_count;
	struct vdfs_processor_tx_tss *states_tss;
	struct vdfs_tsd_package domain_info;
	cpumask_var_t shared_cpu_map;
	int (*vdfs_processor_get_throttling) (struct vdfs_processor * pr);
	int (*vdfs_processor_set_throttling) (struct vdfs_processor * pr,
					      int state, bool force);

	u32 address;
	u8 duty_offset;
	u8 duty_width;
	u8 tsd_valid_flag;
	unsigned int shared_type;
	struct vdfs_processor_tx states[VDFS_PROCESSOR_MAX_THROTTLING];
};

/* Limit Interface *

struct vdfs_processor_lx {
	int px;			/* performance state *
	int tx;			/* throttle level *
};

struct vdfs_processor_limit {
	struct vdfs_processor_lx state;	/* current limit *
	struct vdfs_processor_lx thermal;	/* thermal limit *
	struct vdfs_processor_lx user;	/* user limit *
};

struct vdfs_processor_flags {
	u8 power:1;
	u8 performance:1;
	u8 throttling:1;
	u8 limit:1;
	u8 bm_control:1;
	u8 bm_check:1;
	u8 has_cst:1;
	u8 power_setup_done:1;
	u8 bm_rld_set:1;
	u8 need_hotplug_init:1;
};

struct vdfs_processor {
	vdfs_handle handle;
	u32 vdfs_id;
	u32 apic_id;
	u32 id;
	u32 pblk;
	int performance_platform_limit;
	int throttling_platform_limit;
	/* 0 - states 0..n-th state available *

	struct vdfs_processor_flags flags;
	struct vdfs_processor_power power;
	struct vdfs_processor_performance *performance;
	struct vdfs_processor_throttling throttling;
	struct vdfs_processor_limit limit;
	struct thermal_cooling_device *cdev;
	struct device *dev; /* Processor device. /
};

struct vdfs_processor_errata {
	u8 smp;
	struct {
		u8 throttle:1;
		u8 fdma:1;
		u8 reserved:6;
		u32 bmisx;
	} piix4;
};
*/
//KEEP
extern int vdfs_processor_preregister_performance(struct
						  vdfs_processor_performance
						  __percpu *performance);

extern int vdfs_processor_register_performance(struct vdfs_processor_performance
					       *performance, unsigned int cpu);
extern void vdfs_processor_unregister_performance(struct
						  vdfs_processor_performance
						  *performance,
						  unsigned int cpu);

/* note: this locks both the calling module and the processor module
         if a _PPC object exists, rmmod is disallowed then *
int vdfs_processor_notify_smm(struct module *calling_module);

/* parsing the _P* objects. *
extern int vdfs_processor_get_performance_info(struct vdfs_processor *pr);

/* for communication between multiple parts of the processor kernel module *
DECLARE_PER_CPU(struct vdfs_processor *, processors);
extern struct vdfs_processor_errata errata;

#ifdef ARCH_HAS_POWER_INIT
void vdfs_processor_power_init_bm_check(struct vdfs_processor_flags *flags,
					unsigned int cpu);
int vdfs_processor_ffh_cstate_probe(unsigned int cpu,
				    struct vdfs_processor_cx *cx,
				    struct vdfs_power_register *reg);
void vdfs_processor_ffh_cstate_enter(struct vdfs_processor_cx *cstate);
#else
static inline void vdfs_processor_power_init_bm_check(struct
						      vdfs_processor_flags
						      *flags, unsigned int cpu)
{
	flags->bm_check = 1;
	return;
}
static inline int vdfs_processor_ffh_cstate_probe(unsigned int cpu,
						  struct vdfs_processor_cx *cx,
						  struct vdfs_power_register
						  *reg)
{
	return -1;
}
static inline void vdfs_processor_ffh_cstate_enter(struct vdfs_processor_cx
						   *cstate)
{
	return;
}
#endif

/* in processor_perflib.c *

#ifdef CONFIG_CPU_FREQ
void vdfs_processor_ppc_init(void);
void vdfs_processor_ppc_exit(void);
int vdfs_processor_ppc_has_changed(struct vdfs_processor *pr, int event_flag);
*/extern int vdfs_processor_get_bios_limit(int cpu, unsigned int *limit);
/*#else
static inline void vdfs_processor_ppc_init(void)
{
	return;
}
static inline void vdfs_processor_ppc_exit(void)
{
	return;
}
static inline int vdfs_processor_ppc_has_changed(struct vdfs_processor *pr,
								int event_flag)
{
	static unsigned int printout = 1;
	if (printout) {
		printk(KERN_WARNING
		       "Warning: Processor Platform Limit event detected, but not handled.\n");
		printk(KERN_WARNING
		       "Consider compiling CPUfreq support into your kernel.\n");
		printout = 0;
	}
	return 0;
}
static inline int vdfs_processor_get_bios_limit(int cpu, unsigned int *limit)
{
	return -ENODEV;
}

#endif				/* CONFIG_CPU_FREQ */

/*
 * Reevaluate whether the T-state is invalid after one cpu is
 * onlined/offlined. In such case the flags.throttling will be updated.
 *
extern void vdfs_processor_reevaluate_tstate(struct vdfs_processor *pr,
			unsigned long action);
extern const struct file_operations vdfs_processor_throttling_fops;
extern void vdfs_processor_throttling_init(void);
/* in processor_idle.c *
int vdfs_processor_power_init(struct vdfs_processor *pr);
int vdfs_processor_power_exit(struct vdfs_processor *pr);
int vdfs_processor_cst_has_changed(struct vdfs_processor *pr);
int vdfs_processor_hotplug(struct vdfs_processor *pr);
extern struct cpuidle_driver vdfs_idle_driver;

#ifdef CONFIG_PM_SLEEP
void vdfs_processor_syscore_init(void);
void vdfs_processor_syscore_exit(void);
#else
static inline void vdfs_processor_syscore_init(void) {}
static inline void vdfs_processor_syscore_exit(void) {}
#endif

/* in processor_thermal.c *
int vdfs_processor_get_limit_info(struct vdfs_processor *pr);
extern const struct thermal_cooling_device_ops processor_cooling_ops;
#ifdef CONFIG_CPU_FREQ
void vdfs_thermal_cpufreq_init(void);
void vdfs_thermal_cpufreq_exit(void);
#else
static inline void vdfs_thermal_cpufreq_init(void)
{
	return;
}
static inline void vdfs_thermal_cpufreq_exit(void)
{
	return;
}
#endif
*/
#endif
