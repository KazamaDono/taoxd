/* vuln.c — DELIBERATELY VULNERABLE. Lab-only. Do NOT load on any host. */
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define VUL_MAGIC _IOW('v', 1, size_t)

static ssize_t vuln_write(struct file *f, const char __user *ubuf,
                          size_t len, loff_t *pos)
{
    unsigned char kbuf[64];                    /* ❶ 64-byte kernel stack buf */
    /* ❷ BUG: no length check; copy_from_user honors attacker len */
    if (copy_from_user(kbuf, ubuf, len))
        return -EFAULT;
    pr_info("vuln: took %zu bytes, first=0x%02x\n", len, kbuf[0]);
    return len;
}

static long vuln_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
    if (cmd != VUL_MAGIC)
        return -ENOTTY;
    /* ❸ leaks a kernel .text pointer to userland; disable KASLR quickly. */
    return (long)&vuln_ioctl;
}

static const struct file_operations vuln_fops = {
    .owner = THIS_MODULE,
    .write = vuln_write,
    .unlocked_ioctl = vuln_ioctl,
};

static struct miscdevice vuln_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "vuln",
    .fops  = &vuln_fops,
    .mode  = 0666,                             /* world-writable on purpose */
};

static int __init vuln_init(void) { return misc_register(&vuln_dev); }
static void __exit vuln_exit(void) { misc_deregister(&vuln_dev); }
module_init(vuln_init);
module_exit(vuln_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Modern Exploit Development, Chapter 24");
MODULE_DESCRIPTION("Deliberately vulnerable misc device (stack overflow + KASLR leak)");
