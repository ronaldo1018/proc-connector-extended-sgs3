#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#define MODULE_NAME "allstat"

static int __init allstat_init(void);
static void __exit allstat_exit(void);
static int allstat_open(struct inode *inode, struct file *file);
static int allstat_show(struct seq_file *m, void *v);

static const struct file_operations allstat_fops = {
	.open		= allstat_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init allstat_init(void)
{
	proc_create(MODULE_NAME, 0, NULL, &allstat_fops);
	printk(KERN_INFO "%s: Module loaded\n", MODULE_NAME);
	return 0;
}

static void __exit allstat_exit(void)
{
	remove_proc_entry(MODULE_NAME, NULL);
	printk(KERN_INFO "%s: Module unloaded\n", MODULE_NAME);
}

static int allstat_open(struct inode *inode, struct file *file)
{
	return single_open(file, allstat_show, NULL);
}

static int allstat_show(struct seq_file *m, void *v)
{
	struct task_struct *p, *t;
	long long exec_runtime;
	long long remainder;

	rcu_read_lock();
	do_each_thread(p, t) // cannot use break in do_each_thread macro!!! because it's a double loop
	{
		exec_runtime = t->se.sum_exec_runtime;
		remainder = do_div(exec_runtime, 1000000000);
		seq_printf(m, "%d %lld.%lld\n", t->pid, exec_runtime, remainder);
	} while_each_thread(p, t);
	rcu_read_unlock();

	return 0;
}

module_init(allstat_init);
module_exit(allstat_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Po-Hsien Tseng <r00922073@csie.ntu.edu.tw>");
MODULE_DESCRIPTION("Some stats of all processes in one file");
