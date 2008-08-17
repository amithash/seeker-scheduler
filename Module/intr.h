
#ifdef LOCAL_PMU_VECTOR
void inst_smp_apic_pmu_interrupt(struct pt_regs *regs);
#endif
static int inst_schedule(struct kprobe *p, struct pt_regs *regs);
static void inst_release_thread(struct task_struct *t);
static void inst___switch_to(struct task_struct *from,
			     struct task_struct *to);
/* I am going to say this only once. If you do not want to use
 * pmu_intr, then why should I force you to patch your kernel?
 * That is completely uncalled for! And hence I am checking for
 * one define I have introduced into the kernel. And hence if you
 * have not patched the kernel, you do not get to use the feature,
 * and live happly ever after with the timer interrupt!
 */

static struct kprobe kp_schedule = {
	.pre_handler = inst_schedule,
	.post_handler = NULL,
	.fault_handler = NULL,
	.addr = (kprobe_opcode_t *) schedule,
};
#ifdef LOCAL_PMU_VECTOR
static struct jprobe jp_smp_pmu_interrupt = {
	.entry = (kprobe_opcode_t *)inst_smp_apic_pmu_interrupt,
	.kp.symbol_name = PMU_ISR,
};
#endif

static struct jprobe jp_release_thread = {
	.entry = (kprobe_opcode_t *)inst_release_thread,
#ifdef SCHED_EXIT_EXISTS
	.kp.symbol_name = "sched_exit",
#else
	.kp.symbol_name = "release_thread",
#endif
};

static struct jprobe jp___switch_to = {
	.entry = (kprobe_opcode_t *)inst___switch_to,
	.kp.symbol_name = "__switch_to",
};

#ifdef LOCAL_PMU_VECTOR
static void configure_enable_interrupts(void);
static void configure_disable_interrupts(void);
static void enable_apic_pmu(void);
#endif

