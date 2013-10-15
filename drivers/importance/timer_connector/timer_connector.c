#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/init.h>
#include <linux/uaccess.h>
#include <linux/timer.h>
#include <linux/cn_proc.h>
#define MODULE_NAME "timer_connector"
#define MAX_NUM_TIMER 4 // util_sample, temp_high, aging, temp_mid
#define BUFF_SIZE 16

struct timer_data_t
{
	int timer_no;
};

extern void timer_tick_connector(int timer_no);

static struct timer_list timer[MAX_NUM_TIMER];
static int num_of_timer;
static int timer_enable[MAX_NUM_TIMER];
static int timer_time[MAX_NUM_TIMER];
static int timer_repeat[MAX_NUM_TIMER];
static struct proc_dir_entry *entry_root;
static struct proc_dir_entry *entry_enable[MAX_NUM_TIMER];
static struct proc_dir_entry *entry_time[MAX_NUM_TIMER];
static struct proc_dir_entry *entry_repeat[MAX_NUM_TIMER];
static struct timer_data_t timer_data_list[MAX_NUM_TIMER];

static int __init timer_init(void);
static void __exit timer_exit(void);
static int timer_enable_read(char *page, char **start, off_t off, int count, int *eof, void *data);
static int timer_enable_write(struct file *file, const char __user *buffer, unsigned long count, void *data);
static int timer_time_read(char *page, char **start, off_t off, int count, int *eof, void *data);
static int timer_time_write(struct file *file, const char __user *buffer, unsigned long count, void *data);
static int timer_repeat_read(char *page, char **start, off_t off, int count, int *eof, void *data);
static int timer_repeat_write(struct file *file, const char __user *buffer, unsigned long count, void *data);
static void timer_func(unsigned long timer_no);

static int __init timer_init(void)
{
	int i, ret = 0;
	char buff[BUFF_SIZE];

	num_of_timer = 0;
	memset(timer_enable, 0, sizeof(timer_enable));
	memset(timer_time, 0, sizeof(timer_time));
	memset(timer_repeat, 0, sizeof(timer_repeat));

	// initialize procfs
	
	// make a root in procfs for this module
	entry_root = proc_mkdir(MODULE_NAME, NULL);
	if(entry_root == NULL)
	{
		ret = -ENOMEM;
		printk(KERN_INFO "%s: Cannot create proc entry\n", MODULE_NAME);
		return ret;
	}

	for(i = 0; i < MAX_NUM_TIMER; i++)
	{
		// initialize data
		timer_data_list[i].timer_no = i;

		// create entries
		sprintf(buff, "timer%d_enable", i);
		entry_enable[i] = create_proc_entry(buff, 0644, entry_root);
		sprintf(buff, "timer%d_time", i);
		entry_time[i] = create_proc_entry(buff, 0644, entry_root);
		sprintf(buff, "timer%d_repeat", i);
		entry_repeat[i] = create_proc_entry(buff, 0644, entry_root);

		if(entry_enable[i] == NULL || entry_time[i] == NULL)
		{
			ret = -ENOMEM;
			printk(KERN_INFO "%s: Cannot create proc entry\n", MODULE_NAME);
			break;
		}
		else
		{
			entry_enable[i]->read_proc = timer_enable_read;
			entry_enable[i]->write_proc = timer_enable_write;
			entry_enable[i]->data = &timer_data_list[i]; // this column is used in read_proc and write_proc to distinguish which timer is called
			entry_time[i]->read_proc = timer_time_read;
			entry_time[i]->write_proc = timer_time_write;
			entry_time[i]->data = &timer_data_list[i]; // this column is used in read_proc and write_proc to distinguish which timer is called
			entry_repeat[i]->read_proc = timer_repeat_read;
			entry_repeat[i]->write_proc = timer_repeat_write;
			entry_repeat[i]->data = &timer_data_list[i]; // this column is used in read_proc and write_proc to distinguish which timer is called
		}
	}

	if(!ret)
		printk(KERN_INFO "%s: Module loaded\n", MODULE_NAME);
	return ret;
}

static void __exit timer_exit(void)
{
	int i = 0;
	char buff[BUFF_SIZE];

	for(i = 0; i < MAX_NUM_TIMER; i++)
	{
		if(timer_enable[i])
		{
			del_timer(&timer[i]); // delete timer
		}
		// remove procfs entries
		sprintf(buff, "timer%d_enable", i);
		remove_proc_entry(buff, entry_root);
		sprintf(buff, "timer%d_time", i);
		remove_proc_entry(buff, entry_root);
		sprintf(buff, "timer%d_repeat", i);
		remove_proc_entry(buff, entry_root);
	}
	remove_proc_entry(MODULE_NAME, NULL);
	printk(KERN_INFO "%s: Module unloaded\n", MODULE_NAME);
}

static int timer_enable_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int len;
	struct timer_data_t *timer_data = (struct timer_data_t *)data;
	len = sprintf(page, "%d\n", timer_enable[timer_data->timer_no]);
	return len;
}

static int timer_enable_write(struct file *file, const char __user *buffer, unsigned long count, void *data)
{
	unsigned long num;
	char buff[BUFF_SIZE];
	struct timer_data_t *timer_data = (struct timer_data_t *)data;
	int timer_no = timer_data->timer_no;

	// get buffer size
	if(count >= BUFF_SIZE)
	{
		printk(KERN_INFO "%s: new count = %d\n", MODULE_NAME, BUFF_SIZE - 1);
		count = BUFF_SIZE - 1;
	}
	// write data to buffer
	if(copy_from_user(buff, buffer, count))
	{
		printk(KERN_INFO "%s: copy_from_user failed\n", MODULE_NAME);
		return -EFAULT;
	}
	buff[count] = '\0';
	// convert buffer to number
	if(kstrtoul(buff, 10, &num))
	{
		printk(KERN_INFO "%s: string to num failed\n", MODULE_NAME);
		return -EINVAL;
	}
	// assign number
	if(num)
	{
		timer_enable[timer_no] = 1;
		init_timer(&timer[timer_no]); // initialize timer
		timer[timer_no].expires = jiffies + timer_time[timer_no]; // expired time
		timer[timer_no].function = timer_func; // callback function
		timer[timer_no].data = timer_no; // callback function parameter
		add_timer(&timer[timer_no]);
	}
	else
	{
		timer_enable[timer_no] = 0;
		del_timer(&timer[timer_no]);
	}

	return count;
}

static int timer_time_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int len;
	struct timer_data_t *timer_data = (struct timer_data_t *)data;
	len = sprintf(page, "%d\n", timer_time[timer_data->timer_no]);
	return len;
}

static int timer_time_write(struct file *file, const char __user *buffer, unsigned long count, void *data)
{
	unsigned long num;
	char buff[BUFF_SIZE];
	struct timer_data_t *timer_data = (struct timer_data_t *)data;

	// get buffer size
	if(count >= BUFF_SIZE)
	{
		printk(KERN_INFO "%s: new count = %d\n", MODULE_NAME, BUFF_SIZE - 1);
		count = BUFF_SIZE - 1;
	}
	// write data to buffer
	if(copy_from_user(buff, buffer, count))
	{
		printk(KERN_INFO "%s: copy_from_user failed\n", MODULE_NAME);
		return -EFAULT;
	}
	buff[count] = '\0';
	// convert buffer to number, newline is acceptible and ignored in kstrtoul
	if(kstrtoul(buff, 10, &num))
	{
		printk(KERN_INFO "%s: string to num failed\n", MODULE_NAME);
		return -EINVAL;
	}
	// assign number
	timer_time[timer_data->timer_no] = num;

	return count;
}

static int timer_repeat_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int len;
	struct timer_data_t *timer_data = (struct timer_data_t *)data;
	len = sprintf(page, "%d\n", timer_repeat[timer_data->timer_no]);
	return len;
}

static int timer_repeat_write(struct file *file, const char __user *buffer, unsigned long count, void *data)
{
	unsigned long num;
	char buff[BUFF_SIZE];
	struct timer_data_t *timer_data = (struct timer_data_t *)data;
	int timer_no = timer_data->timer_no;

	// get buffer size
	if(count >= BUFF_SIZE)
	{
		printk(KERN_INFO "%s: new count = %d\n", MODULE_NAME, BUFF_SIZE - 1);
		count = BUFF_SIZE - 1;
	}
	// write data to buffer
	if(copy_from_user(buff, buffer, count))
	{
		printk(KERN_INFO "%s: copy_from_user failed\n", MODULE_NAME);
		return -EFAULT;
	}
	buff[count] = '\0';
	// convert buffer to number
	if(kstrtoul(buff, 10, &num))
	{
		printk(KERN_INFO "%s: string to num failed\n", MODULE_NAME);
		return -EINVAL;
	}
	// assign number
	if(num)
		timer_repeat[timer_no] = 1;
	else
		timer_repeat[timer_no] = 0;

	return count;
}

static void timer_func(unsigned long timer_no)
{
	timer_tick_connector(timer_no);
	if(timer_repeat[timer_no])
	{
		mod_timer(&timer[timer_no], jiffies + timer_time[timer_no]);
	}
	else
	{
		timer_enable[timer_no] = 0;
		del_timer(&timer[timer_no]);
	}
}

module_init(timer_init);
module_exit(timer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Po-Hsien Tseng <r00922073@csie.ntu.edu.tw>");
MODULE_DESCRIPTION("Timer interrupt to proc connector module");
