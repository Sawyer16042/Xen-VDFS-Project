/*
 * vdfs-cpufreq.c - VDFS Processor P-States Driver
 *
 *  Copyright (C) 2001, 2002 Andy Grover <andrew.grover@intel.com>
 *  Copyright (C) 2001, 2002 Paul Diefenbaugh <paul.s.diefenbaugh@intel.com>
 *  Copyright (C) 2002 - 2004 Dominik Brodowski <linux@brodo.de>
 *  Copyright (C) 2006       Denis Sadykov <denis.m.sadykov@intel.com>
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or (at
 *  your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/smp.h>
#include <linux/sched.h>
#include <linux/cpufreq.h>
#include <linux/compiler.h>
#include <linux/dmi.h>
#include <linux/slab.h>

#include <linux/vdfs.h> //delete me if still does't work
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/uaccess.h>

#include <vdfs/processor.h>

#include <asm/msr.h>
#include <asm/processor.h>
#include <asm/cpufeature.h>

//MODULE_AUTHOR("Paul Diefenbaugh, Dominik Brodowski");
MODULE_DESCRIPTION("VDFS Processor P-States Driver");
//MODULE_LICENSE("GPL");

#define PFX "vdfs-cpufreq: "

/*enum {
	UNDEFINED_CAPABLE = 0,
	SYSTEM_INTEL_MSR_CAPABLE,
	SYSTEM_AMD_MSR_CAPABLE,
	SYSTEM_IO_CAPABLE,
};*/

#define INTEL_MSR_RANGE		(0xffff)
#define AMD_MSR_RANGE		(0x7)

#define MSR_K7_HWCR_CPB_DIS	(1ULL << 25)

struct vdfs_cpufreq_data {
	struct vdfs_processor_performance *vdfs_data;
	unsigned int resume;
	unsigned int cpu_feature;
	cpumask_var_t freqdomain_cpus;
};

static DEFINE_PER_CPU(struct vdfs_cpufreq_data *, acfreq_data);

/* vdfs_perf_data is a pointer to percpu data. */
static struct vdfs_processor_performance __percpu *vdfs_perf_data;

static struct cpufreq_driver vdfs_cpufreq_driver;

static unsigned int vdfs_pstate_strict;

static ssize_t show_freqdomain_cpus(struct cpufreq_policy *policy, char *buf)
{
	struct vdfs_cpufreq_data *data = per_cpu(acfreq_data, policy->cpu);

	return cpufreq_show_cpus(data->freqdomain_cpus, buf);
}

cpufreq_freq_attr_ro(freqdomain_cpus);


static unsigned int get_cur_freq_on_cpu(unsigned int cpu)
{
	struct vdfs_cpufreq_data *data = per_cpu(acfreq_data, cpu);
	unsigned int freq;
	//unsigned int cached_freq;

	pr_debug("get_cur_freq_on_cpu (%d)\n", cpu);

	if (unlikely(data == NULL || data->vdfs_data == NULL )) {
		return 0;
	
	}

	//checks if it was changed outside of target. If it was, set resume
	//cached_freq = data->freq_table[data->vdfs_data->state].frequency;
	freq = 413; //xen_get_freq();
	/*if (freq != cached_freq) {
		data->resume = 1;
	}*/

	pr_debug("cur freq = %u\n", freq);

	printk(KERN_EMERG "Gamzee says the value is %d\n", freq);
	return freq;
}

static int vdfs_cpufreq_target(struct cpufreq_policy *policy,
			       unsigned int index)
{
	//Gamzee
	struct vdfs_cpufreq_data *data = per_cpu(acfreq_data, policy->cpu);
	struct vdfs_processor_performance *perf;
	//unsigned int next_perf_state = 0; /* Index into perf table */
	int result = 0;

	//checks if it found stuff - keep?
	/*if (unlikely(data == NULL ||
	     data->vdfs_data == NULL || data->freq_table == NULL)) {
		return -ENODEV;
	}*/

	perf = data->vdfs_data;
	//next_perf_state = index;
	//redudancy check     CHANGE ME
	if (perf->state == index) {
		if (unlikely(data->resume)) {
			pr_debug("Called after resume, resetting to P%d\n",
				index);
			data->resume = 0;
		} else {
			pr_debug("Already at target state (P%d)\n",
				index);
			goto out;
		}
	}

	//xen_write_freq();

	
	//check if it got stuff - probs useless     CHANGE ME
	/*if (vdfs_pstate_strict) {
		//create new checky thing - killed check_freqs
		if (!check_freqs(data->freq_table[index].frequency,
					data)) {
			pr_debug("vdfs_cpufreq_target failed (%d)\n",
				policy->cpu);
			result = -EAGAIN;
		}
	}*/

	if (!result)
		perf->state = index;

out:
	return result;
}

static unsigned long
vdfs_cpufreq_guess_freq(struct vdfs_cpufreq_data *data, unsigned int cpu)
{
	struct vdfs_processor_performance *perf = data->vdfs_data;
	if (cpu_khz) {
		/* search the closest match to cpu_khz */
		unsigned int i;
		unsigned long freq;
		unsigned long freqn = perf->states[0].core_frequency * 1000;

		for (i = 0; i < (perf->state_count-1); i++) {
			freq = freqn;
			freqn = perf->states[i+1].core_frequency * 1000;
			if ((2 * cpu_khz) > (freqn + freq)) {
				perf->state = i;
				return freq;
			}
		}
		perf->state = perf->state_count-1;
		return freqn;
	} else {
		/* assume CPU is at P0... */
		perf->state = 0;
		return perf->states[0].core_frequency * 1000;
	}
}

static void free_vdfs_perf_data(void)
{
	unsigned int i;

	/* Freeing a NULL pointer is OK, and alloc_percpu zeroes. */
	for_each_possible_cpu(i)
		free_cpumask_var(per_cpu_ptr(vdfs_perf_data, i)
				 ->shared_cpu_map);
	free_percpu(vdfs_perf_data);
}

/*
 * vdfs_cpufreq_early_init - initialize VDFS P-States library
 *
 * Initialize the VDFS P-States library (drivers/vdfs/processor_perflib.c)
 * in order to determine correct frequency and voltage pairings. We can
 * do _PDC and _PSD and find out the processor dependency for the
 * actual init that will happen later...
 */
static int __init vdfs_cpufreq_early_init(void)
{
	unsigned int i;
	pr_debug("vdfs_cpufreq_early_init\n");

	vdfs_perf_data = alloc_percpu(struct vdfs_processor_performance);
	if (!vdfs_perf_data) {
		pr_debug("Memory allocation error for vdfs_perf_data.\n");
		return -ENOMEM;
	}
	for_each_possible_cpu(i) {
		if (!zalloc_cpumask_var_node(
			&per_cpu_ptr(vdfs_perf_data, i)->shared_cpu_map,
			GFP_KERNEL, cpu_to_node(i))) {

			/* Freeing a NULL pointer is OK: alloc_percpu zeroes. */
			free_vdfs_perf_data();
			return -ENOMEM;
		}
	}

	/* Do initialization in VDFS core */
	vdfs_processor_preregister_performance(vdfs_perf_data);
	return 0;
}

/*
 * Some BIOSes do SW_ANY coordination internally, either set it up in hw
 * or do it in BIOS firmware and won't inform about it to OS. If not
 * detected, this has a side effect of making CPU run at a different speed
 * than OS intended it to run at. Detect it and handle it cleanly.
 */
static int bios_with_sw_any_bug;

static int sw_any_bug_found(const struct dmi_system_id *d)
{
	bios_with_sw_any_bug = 1;
	return 0;
}


static int vdfs_cpufreq_cpu_init(struct cpufreq_policy *policy)
{
	unsigned int i;
//	unsigned int valid_states = 0;
	unsigned int cpu = policy->cpu;
	struct vdfs_cpufreq_data *data;
	unsigned int result = 0;
//	struct cpuinfo_x86 *c = &cpu_data(policy->cpu);
	struct vdfs_processor_performance *perf;
/*#ifdef CONFIG_SMP
	static int blacklisted;
#endif*/

	printk(KERN_EMERG "Gamzee says hello");
	pr_debug("vdfs_cpufreq_cpu_init\n");

/*#ifdef CONFIG_SMP
	if (blacklisted)
		return blacklisted;
	blacklisted = vdfs_cpufreq_blacklist(c);
	if (blacklisted)
		return blacklisted;
#endif*/

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;
	//look at mystery variable and cpumasks
	if (!zalloc_cpumask_var(&data->freqdomain_cpus, GFP_KERNEL)) {
		result = -ENOMEM;
		goto err_free;
	}

	data->vdfs_data = per_cpu_ptr(vdfs_perf_data, cpu);
	per_cpu(acfreq_data, cpu) = data;//fix me with Xen info

	//Don't delete this, figure out if the loop thing actually causes the program to loop and update frequently
	/*if (cpu_has(c, X86_FEATURE_CONSTANT_TSC))
		vdfs_cpufreq_driver.flags |= CPUFREQ_CONST_LOOPS;*/

	result = vdfs_processor_register_performance(data->vdfs_data, cpu);
	if (result)
		goto err_free_mask;

	perf = data->vdfs_data;
	policy->shared_type = perf->shared_type;

	/*
	 * Will let policy->cpus know about dependency only when software
	 * coordination is required.
	 */
	if (policy->shared_type == CPUFREQ_SHARED_TYPE_ALL ||
	    policy->shared_type == CPUFREQ_SHARED_TYPE_ANY) {
		cpumask_copy(policy->cpus, perf->shared_cpu_map);
	}
	cpumask_copy(data->freqdomain_cpus, perf->shared_cpu_map);

//check if hardware
/*#ifdef CONFIG_SMP
	dmi_check_system(sw_any_bug_dmi_table);
	if (bios_with_sw_any_bug && !policy_is_shared(policy)) {
		policy->shared_type = CPUFREQ_SHARED_TYPE_ALL;
		cpumask_copy(policy->cpus, cpu_core_mask(cpu));
	}

	if (check_amd_hwpstate_cpu(cpu) && !vdfs_pstate_strict) {
		cpumask_clear(policy->cpus);
		cpumask_set_cpu(cpu, policy->cpus);
		cpumask_copy(data->freqdomain_cpus, cpu_sibling_mask(cpu));
		policy->shared_type = CPUFREQ_SHARED_TYPE_HW;
		pr_info_once(PFX "overriding BIOS provided _PSD data\n");
	}
#endif*/

	/* capability check */
	if (perf->state_count <= 1) {
		pr_debug("No P-States\n");
		result = -ENODEV;
		goto err_unreg;
	}

	/*if (perf->control_register.space_id != perf->status_register.space_id) {
		result = -ENODEV;
		goto err_unreg;
	}

	/*switch (perf->control_register.space_id) {
	case VDFS_ADR_SPACE_SYSTEM_IO:
		if (boot_cpu_data.x86_vendor == X86_VENDOR_AMD &&
		    boot_cpu_data.x86 == 0xf) {
			pr_debug("AMD K8 systems must use native drivers.\n");
			result = -ENODEV;
			goto err_unreg;
		}
		pr_debug("SYSTEM IO addr space\n");
		data->cpu_feature = SYSTEM_IO_CAPABLE;
		break;
	case VDFS_ADR_SPACE_FIXED_HARDWARE:
		pr_debug("HARDWARE addr space\n");
		if (check_est_cpu(cpu)) {
			data->cpu_feature = SYSTEM_INTEL_MSR_CAPABLE;
			break;
		}
		if (check_amd_hwpstate_cpu(cpu)) {
			data->cpu_feature = SYSTEM_AMD_MSR_CAPABLE;
			break;
		}
		result = -ENODEV;
		goto err_unreg;
	default:
		pr_debug("Unknown addr space %d\n",
			(u32) (perf->control_register.space_id));
		result = -ENODEV;
		goto err_unreg;
	}

	data->freq_table = kmalloc(sizeof(*data->freq_table) *
		    (perf->state_count+1), GFP_KERNEL);
	if (!data->freq_table) {
		result = -ENOMEM;
		goto err_unreg;
	}*/

	/* detect transition latency */
	policy->cpuinfo.transition_latency = 0;
	for (i = 0; i < perf->state_count; i++) {
		if ((perf->states[i].transition_latency * 1000) >
		    policy->cpuinfo.transition_latency)
			policy->cpuinfo.transition_latency =
			    perf->states[i].transition_latency * 1000;
	}

	/* Check for high latency (>20uS) from buggy BIOSes, like on T42
	if (perf->control_register.space_id == VDFS_ADR_SPACE_FIXED_HARDWARE &&
	    policy->cpuinfo.transition_latency > 20 * 1000) {
		policy->cpuinfo.transition_latency = 20 * 1000;
		printk_once(KERN_INFO
			    "P-state transition latency capped at 20 uS\n");
	}*/

	/* table init 
	for (i = 0; i < perf->state_count; i++) {
		//sets up frequency table in descending order, with 0 being the max freq
		if (i > 0 && perf->states[i].core_frequency >=
		    data->freq_table[valid_states-1].frequency / 1000)
			continue;

		data->freq_table[valid_states].driver_data = i;
		data->freq_table[valid_states].frequency =
		    perf->states[i].core_frequency * 1000;
		valid_states++;
	}
	data->freq_table[valid_states].frequency = CPUFREQ_TABLE_END;
	perf->state = 0;

	result = cpufreq_table_validate_and_show(policy, data->freq_table);
	if (result)
		goto err_freqfree;*/
/*
	if (perf->states[0].core_frequency * 1000 != policy->cpuinfo.max_freq)
		printk(KERN_WARNING FW_WARN "P-state 0 is not max freq\n");
*/
	//sets the initial values, I think
	/*switch (perf->control_register.space_id) {
	case ACPI_ADR_SPACE_SYSTEM_IO:
		/*
		 * The core will not set policy->cur, because
		 * cpufreq_driver->get is NULL, so we need to set it here.
		 * However, we have to guess it, because the current speed is
		 * unknown and not detectable via IO ports.
		 *
		policy->cur = vdfs_cpufreq_guess_freq(data, policy->cpu);
		break;
	case ACPI_ADR_SPACE_FIXED_HARDWARE:
	*/	vdfs_cpufreq_driver.get = get_cur_freq_on_cpu;
	/*	break;
	default:
		break;
	}*/

	//BIOS STUFF, hadware probably
	/*
	/* notify BIOS that we exist /
	vdfs_processor_notify_smm(THIS_MODULE);
	*
	pr_debug("CPU%u - VDFS performance management activated.\n", cpu);
	for (i = 0; i < perf->state_count; i++)
		pr_debug("     %cP%d: %d MHz, %d mW, %d uS\n",
			(i == perf->state ? '*' : ' '), i,
			(u32) perf->states[i].core_frequency,
			(u32) perf->states[i].power,
			(u32) perf->states[i].transition_latency);

	*
	 * the first call to ->target() should result in us actually
	 * writing something to the appropriate registers.
	 */
	data->resume = 1;

	return result;

//err_freqfree:
//	kfree(data->freq_table);
err_unreg:
	vdfs_processor_unregister_performance(perf, cpu);
err_free_mask:
	free_cpumask_var(data->freqdomain_cpus);
err_free:
	kfree(data);
	per_cpu(acfreq_data, cpu) = NULL;

	return result;
}

static int vdfs_cpufreq_cpu_exit(struct cpufreq_policy *policy)
{
	struct vdfs_cpufreq_data *data = per_cpu(acfreq_data, policy->cpu);

	pr_debug("vdfs_cpufreq_cpu_exit\n");

	if (data) {
		cpufreq_frequency_table_put_attr(policy->cpu);
		per_cpu(acfreq_data, policy->cpu) = NULL;
		vdfs_processor_unregister_performance(data->vdfs_data,
						      policy->cpu);
		free_cpumask_var(data->freqdomain_cpus);
//		kfree(data->freq_table);
		kfree(data);
	}

	return 0;
}

static int vdfs_cpufreq_resume(struct cpufreq_policy *policy)
{
	struct vdfs_cpufreq_data *data = per_cpu(acfreq_data, policy->cpu);

	pr_debug("vdfs_cpufreq_resume\n");

	data->resume = 1;

	return 0;
}

static struct freq_attr *vdfs_cpufreq_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	&freqdomain_cpus,
	NULL,	/* this is a placeholder for cpb, do not remove */
	NULL,
};

static struct cpufreq_driver vdfs_cpufreq_driver = {
	.verify		= cpufreq_generic_frequency_table_verify,
	.target_index	= vdfs_cpufreq_target,
	.bios_limit	= vdfs_processor_get_bios_limit,
	.init		= vdfs_cpufreq_cpu_init,
	.exit		= vdfs_cpufreq_cpu_exit,
	.resume		= vdfs_cpufreq_resume,
	.name		= "vdfs-cpufreq",
	.attr		= vdfs_cpufreq_attr,
};

static int __init vdfs_cpufreq_init(void)
{
	int ret;

//	if (vdfs_disabled)
//		return -ENODEV;

	/* don't keep reloading if cpufreq_driver exists */
	if (cpufreq_get_current_driver())
		return -EEXIST;

	pr_debug("vdfs_cpufreq_init\n");

	ret = vdfs_cpufreq_early_init();
	if (ret)
		return ret;

//	vdfs_cpufreq_boost_init();
	ret = cpufreq_register_driver(&vdfs_cpufreq_driver);
	if (ret) {
		free_vdfs_perf_data();
//		vdfs_cpufreq_boost_exit();
	}
	return ret;
}

static void __exit vdfs_cpufreq_exit(void)
{
	pr_debug("vdfs_cpufreq_exit\n");

//	vdfs_cpufreq_boost_exit();

	cpufreq_unregister_driver(&vdfs_cpufreq_driver);

	free_vdfs_perf_data();
}

//module_param(vdfs_pstate_strict, uint, 0644);
//MODULE_PARM_DESC(vdfs_pstate_strict,
//	"value 0 or non-zero. non-zero -> strict VDFS checks are "
//	"performed during frequency changes.");

late_initcall(vdfs_cpufreq_init);
module_exit(vdfs_cpufreq_exit);

/*
static const struct x86_cpu_id vdfs_cpufreq_ids[] = {
	X86_FEATURE_MATCH(X86_FEATURE_VDFS),
	X86_FEATURE_MATCH(X86_FEATURE_HW_PSTATE),
	{}
};
MODULE_DEVICE_TABLE(x86cpu, vdfs_cpufreq_ids);

static const struct vdfs_device_id processor_device_ids[] = {
	{VDFS_PROCESSOR_OBJECT_HID, },
	{VDFS_PROCESSOR_DEVICE_HID, },
	{},
};
MODULE_DEVICE_TABLE(vdfs, processor_device_ids);
*/
MODULE_ALIAS("vdfs");






