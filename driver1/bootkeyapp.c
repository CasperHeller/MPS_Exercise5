#include <linux/gpio.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <linux/module.h>

#define BOOTKEYK_MAJOR 64
#define BOOTKEY_MINOR 0
#define BOOTKEY_GPIO 7
#define BUFFER_LENGTH 2

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("BOOT_KEY Driver");
MODULE_AUTHOR("DETMPS");

ssize_t bootkey_read(struct file *filep, char __user *buf, size_t count, loff_t *f_pos);
int bootkey_open(struct inode *inode, struct file *filep);
int bootkey_release(struct inode *inode, struct file *filep);

struct file_operations bootkey_fops = {
  .owner = THIS_MODULE,
  .read = bootkey_read,
  .open = bootkey_open,
  .release = bootkey_release,
};

static struct cdev *cdevStruct;
static int devno;

static int __init bootkey_init(void)
{
  printk(KERN_ALERT "BOOT_KEY driver loaded\n");
  
  //Make device number
  devno = MKDEV(BOOTKEYK_MAJOR, BOOTKEY_MINOR);
  
  //Request GPIO
  int err = 0;
  err = gpio_request(BOOTKEY_GPIO, "BOOT_KEY");
  
  if(err < 0)
  {
    printk(KERN_ALERT "ERROR requesting GPIO: %d\n", err);
    goto err_exit;
  }
  
  //Set GPIO direction (in or out)
  err = gpio_direction_input(BOOTKEY_GPIO);
  
  if(err < 0)
  {
    printk(KERN_ALERT "ERROR changing GPIO direction: %d\n", err);
    goto err_freegpio;
  }
  
  //Register device
  err = register_chrdev_region(devno, 1, "BOOT_KEY");
  
  if(err < 0)
  {
    printk(KERN_ALERT "ERROR register cdev: %d\n", err);
    goto err_freegpio;
  }
  
  //Cdev init
  cdevStruct = cdev_alloc();
  cdev_init(cdevStruct, &bootkey_fops);
  
  //Add Cdev
  err = cdev_add(cdevStruct, devno, 1);
  if(err < 0)
  {
    printk(KERN_ALERT "ERROR adding cdev: %d\n", err);
    goto err_unregister_chardev;
  }
  
  //Succes
  printk(KERN_ALERT "BOOT_KEY driver loaded successfully\n");
  return 0;
  
  //Goto erros
  err_unregister_chardev:
    unregister_chrdev_region(devno, 1);
  err_freegpio:
    gpio_free(BOOTKEY_GPIO); 
  err_exit:
    return err;
  
}

static void __exit bootkey_exit(void)
{ 
  //Delete Cdev
  cdev_del(cdevStruct);

  //Unregister device
  unregister_chrdev_region(devno, 1);
  
  //Free GPIO
  gpio_free(BOOTKEY_GPIO);
  
  //Succes
  printk(KERN_ALERT "BOOT_KEY driver unloaded\n");
}

module_init(bootkey_init);
module_exit(bootkey_exit);

int bootkey_open(struct inode *inode, struct file *filep)
{
  //Reading Major and Minor
  int major, minor;
  major = MAJOR(inode->i_rdev);
  minor = MINOR(inode->i_rdev);
  
  //Printing
  printk("Opening MyGpio Device:\n- Major: %i\n- Minor: %i\n\n", major, minor);
  return 0; //??
}

int bootkey_release(struct inode *inode, struct file *filep)
{
  //Reading Major and Minor
  int minor, major;
  major = MAJOR(inode->i_rdev);
  minor = MINOR(inode->i_rdev);
  
  //Printing
  printk("Closing/Releasing MyGpio Device: \n- Major: %i\n- Minor: %i\n\n", major, minor);
  return 0; //??
}

ssize_t bootkey_read(struct file *filep, char __user *buf, size_t count, loff_t *f_pos)
{
  //Bufferen har en størrelse på 2, da readBootkey enten kan være 0 eller 1 og så plus NULL
  char buffer[BUFFER_LENGTH];
  int bufferLength = sizeof(buffer);
  
  int readBootkey = gpio_get_value(BOOTKEY_GPIO);
  
  bufferLength = sprintf(buffer, "%i", readBootkey);
  
  if(copy_to_user(buf, buffer, bufferLength) != 0)
  {
    printk(KERN_ALERT "Could not copy to user space..\n");
  }
  
  //Flytter fil position
  *f_pos += bufferLength;
  
  return bufferLength;
}