
#ifdef LOCAL_PMU_VECTOR
void inst_smp_apic_pmu_interrupt(struct pt_regs *regs);
#endif
int inst_schedule(struct kprobe *p, struct pt_regs *regs);
void inst_release_thread(struct task_struct *t);
void inst___switch_to(struct task_struct *from,
			     struct task_struct *to);
/* I am going to say this only once. If you do not want to use
 * pmu_intr, then why should I force you to patch your kernel?
 * That is completely uncalled for! And hence I am checking for
 * one define I have introduced into the kernel. And hence if you
 * have not patched the kernel, you do not get to use the feature,
 * and live happly ever after with the timer interrupt!
 */

#ifdef LOCAL_PMU_VECTOR
void configure_enable_interrupts(void);
void configure_disable_interrupts(void);
void enable_apic_pmu(void);
#endif

