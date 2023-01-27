/*
 * Race-Averse Scheduling (RAS) Class (SCHED_RAS)
 */

#include "sched.h"

#include <linux/slab.h>

/*
 * Print some information to debug and show result.
 */
static void debug(char *name, struct rq *rq, struct task_struct *p)
{
    printk("%s:: pid: %d,wcounts: %d,total_wcounts: %d,time_slice: %u,nr: %lu\n", name,
           p->pid, p->wcounts, rq->ras.total_wcounts, p->ras.time_slice, rq->ras.ras_nr_running);
}

/*
 * Get the task_struct according to sched_ras_entity.
 */
static inline struct task_struct *ras_task_of(struct sched_ras_entity *ras_se)
{
    return container_of(ras_se, struct task_struct, ras);
}

static inline int on_ras_rq(struct sched_ras_entity *ras_se)
{
    /* Check if ras_se's run_list is empty. */
    return !list_empty(&ras_se->run_list);
}

/*
 * Update the current task's runtime statistics. Skip current tasks that
 * are not in our scheduling class.
 */
static void update_curr_ras(struct rq *rq)
{
    struct task_struct *curr = rq->curr;
    u64 delta_exec;

    if (curr->sched_class != &ras_sched_class)
        return;

    delta_exec = rq->clock_task - curr->se.exec_start;
    if (unlikely((s64)delta_exec < 0))
        delta_exec = 0;

    schedstat_set(curr->se.statistics.exec_max,
                  max(curr->se.statistics.exec_max, delta_exec));

    curr->se.sum_exec_runtime += delta_exec;
    account_group_exec_runtime(curr, delta_exec);

    curr->se.exec_start = rq->clock_task;
    cpuacct_charge(curr, delta_exec);
}

/*
 * Get task's group path.(from kernel/sched/debug.c)
 */
static char group_path[4096];
static char *task_group_path(struct task_group *tg)
{
    if (autogroup_path(tg, group_path, 4096))
        return group_path;

    /*
     * May be NULL if the underlying cgroup isn't fully-created yet
     */
    if (!tg->css.cgroup)
    {
        group_path[0] = '\0';
        return group_path;
    }
    cgroup_path(tg->css.cgroup, group_path, 4096);
    return group_path;
}

/*
 * Update the time_slice of given task.
 */
static void update_time_slice_ras(struct rq *rq, struct task_struct *p)
{
    struct sched_ras_entity *ras_se = &p->ras;
    char *group_path;
    int wcounts = p->wcounts;
    int prob;

    group_path = task_group_path(p->sched_task_group);

    if (group_path[1] == 'b') /* background task */
    {
        ras_se->time_slice = RAS_BG_TIMESLICE;
    }
    else /* foreground task */
    {
        /* calculate weight */
        if (wcounts == 0)
        {
            ras_se->weight = 10;
        }
        else
        {
            rq->ras.total_wcounts = rq->ras.total_wcounts - ras_se->old_wcounts + p->wcounts;
            prob = wcounts / DIV_ROUND_UP(rq->ras.total_wcounts, 10);
            ras_se->weight = prob == 10 ? 1 : 10 - prob;
        }

        ras_se->time_slice = RAS_TIMESLICE * ras_se->weight;
        ras_se->old_wcounts = p->wcounts;
    }
}

/*
 * Initialize the ras run queue.
 */
void init_ras_rq(struct ras_rq *ras_rq, struct rq *rq)
{
    INIT_LIST_HEAD(&ras_rq->run_list);
    ras_rq->ras_nr_running = 0;
    ras_rq->total_wcounts = 0;
}

/*
 * Adding a task to a ras run_list.
 */
static void enqueue_task_ras(struct rq *rq, struct task_struct *p, int flags)
{
    struct sched_ras_entity *ras_se = &p->ras;
    int head = flags & ENQUEUE_HEAD;

    ras_se->old_wcounts = 0;
    update_time_slice_ras(rq, p);

    if (head)
        list_add(&ras_se->run_list, &(rq->ras.run_list));
    else
        list_add_tail(&ras_se->run_list, &(rq->ras.run_list));

    ++rq->ras.ras_nr_running;
    inc_nr_running(rq);

    debug("enqueue_task_ras", rq, p);
}

/*
 * Removing a task from a ras run_list.
 */
static void dequeue_task_ras(struct rq *rq, struct task_struct *p, int flags)
{
    struct sched_ras_entity *ras_se = &p->ras;

    update_curr_ras(rq);

    list_del_init(&ras_se->run_list);
    rq->ras.total_wcounts -= ras_se->old_wcounts;
    --rq->ras.ras_nr_running;

    dec_nr_running(rq);

    debug("dequeue_task_ras", rq, p);
}

/*
 * Put task to the head or the end of the run list without the overhead of
 * dequeue followed by enqueue.
 */
static void requeue_task_ras(struct rq *rq, struct task_struct *p, int head)
{

    struct sched_ras_entity *ras_se = &p->ras;
    struct list_head *queue = &(rq->ras.run_list);

    if (on_ras_rq(ras_se))
    {
        if (head)
            list_move(&ras_se->run_list, queue);
        else
            list_move_tail(&ras_se->run_list, queue);
    }

    debug("requeue_task_ras", rq, p);
}

/*
 * Put task to the end of the run list without the overhead of
 * dequeue followed by enqueue.
 */
static void yield_task_ras(struct rq *rq)
{
    requeue_task_ras(rq, rq->curr, 0);
}

/*
 * Preempt the current task with a newly woken task if needed.
 */
static void check_preempt_curr_ras(struct rq *rq, struct task_struct *p, int flags)
{
    /*
     * There is nood need to implement this function.
     */
}

/*
 * Pick the next task to run.
 */
static struct task_struct *pick_next_task_ras(struct rq *rq)
{
    struct sched_ras_entity *ras_se;
    struct task_struct *p;
    struct ras_rq *ras_rq;

    ras_rq = &rq->ras;

    if (ras_rq->ras_nr_running == 0)
        return NULL;

    ras_se = list_first_entry(&((rq->ras)).run_list, struct sched_ras_entity, run_list);

    p = ras_task_of(ras_se);
    p->se.exec_start = rq->clock_task;

    return p;
}

static void put_prev_task_ras(struct rq *rq, struct task_struct *p)
{
    update_curr_ras(rq);
}

#ifdef CONFIG_SMP

static int select_task_rq_ras(struct task_struct *p, int sd_flag, int flags)
{
    struct task_struct *curr;
    struct rq *rq;
    int new_cpu, cpu, min, total_wcounts;

    new_cpu = task_cpu(p);

    if (p->rt.nr_cpus_allowed == 1)
        goto out;

    /* For anything but wake ups, just return the task_cpu */
    if (sd_flag != SD_BALANCE_WAKE && sd_flag != SD_BALANCE_FORK)
        goto out;

    rq = cpu_rq(cpu);
    min = rq->ras.total_wcounts;

    rcu_read_lock();

    for_each_online_cpu(cpu)
    {
        if (!cpumask_test_cpu(cpu, tsk_cpus_allowed(p)))
            continue;
        rq = cpu_rq(cpu);
        total_wcounts = rq->ras.total_wcounts;
        if (total_wcounts < min)
        {
            min = total_wcounts;
            new_cpu = cpu;
        }
    }

    rcu_read_unlock();
    
    printk("select_task_rq_ras:: pid: %d,pre_cpu: %d,new_cpu: %d\n",p->pid,task_cpu(p),new_cpu);
out:
    return new_cpu;
}

static void set_cpus_allowed_ras(struct task_struct *p,
                                 const struct cpumask *new_mask)
{
}

static void rq_online_ras(struct rq *rq)
{
}

static void rq_offline_ras(struct rq *rq)
{
}

static void pre_schedule_ras(struct rq *rq, struct task_struct *prev)
{
}

static void post_schedule_ras(struct rq *rq)
{
}

static void task_woken_ras(struct rq *rq, struct task_struct *p)
{
}

static void switched_from_ras(struct rq *rq, struct task_struct *p)
{
}
#endif

static void set_curr_task_ras(struct rq *rq)
{
    struct task_struct *p = rq->curr;

    /* set the start time of execution */
    p->se.exec_start = rq->clock_task;
}

/*
 * Update the running task's timeslice every 1ms.
 */
static void task_tick_ras(struct rq *rq, struct task_struct *p, int queued)
{
    struct sched_ras_entity *ras_se = &p->ras;

    update_curr_ras(rq);

    debug("task_tick_ras", rq, p);

    /* Timeslice has not used up. */
    if (--ras_se->time_slice)
        return;

    update_time_slice_ras(rq, p);

    if (ras_se->run_list.prev != ras_se->run_list.next)
    {
        requeue_task_ras(rq, p, 0);
        set_tsk_need_resched(p);
        return;
    }
}

/*
 * Return the timeslice of a task.
 */
static unsigned int get_rr_interval_ras(struct rq *rq, struct task_struct *task)
{
    return task->ras.weight * RAS_TIMESLICE;
}

static void prio_changed_ras(struct rq *rq, struct task_struct *p, int oldprio)
{
}

/*
 * We switched to the sched_ras class.
 */
static void switched_to_ras(struct rq *rq, struct task_struct *p)
{
    if (!p->se.on_rq)
        return;

    if (rq->curr == p)
        resched_task(rq->curr);
    else
        check_preempt_curr(rq, p, 0);
}

const struct sched_class ras_sched_class = {
    .next = &idle_sched_class,        /*Required*/
    .enqueue_task = enqueue_task_ras, /*Required*/
    .dequeue_task = dequeue_task_ras, /*Required*/
    .yield_task = yield_task_ras,     /*Required*/

    .check_preempt_curr = check_preempt_curr_ras, /*Required*/

    .pick_next_task = pick_next_task_ras, /*Required*/
    .put_prev_task = put_prev_task_ras,   /*Required*/

#ifdef CONFIG_SMP
    .select_task_rq = select_task_rq_ras, /*Never need impl */

    .set_cpus_allowed = set_cpus_allowed_ras, /*Never need impl */
    .rq_online = rq_online_ras,               /*Never need impl */
    .rq_offline = rq_offline_ras,             /*Never need impl */
    .pre_schedule = pre_schedule_ras,         /*Never need impl */
    .post_schedule = post_schedule_ras,       /*Never need impl */
    .task_woken = task_woken_ras,             /*Never need impl */
    .switched_from = switched_from_ras,       /*Never need impl */
#endif

    .set_curr_task = set_curr_task_ras, /*Required*/
    .task_tick = task_tick_ras,         /*Required*/

    .get_rr_interval = get_rr_interval_ras, /*Required*/

    .prio_changed = prio_changed_ras, /*Never need impl */
    .switched_to = switched_to_ras,   /*Required*/
};
