#include <linux/gpio.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <linux/module.h>

#define SYS4LED_MAJOR 20
#define SYS4LED_MINOR 0
#define SYS4LED_GPIO 164
#define BUFFER_LENGTH 16

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("SYS_LED4 Driver");
MODULE_AUTHOR("DETMPS");

int sys4led_open(struct inode *inode, struct file *filep);
int sys4led_release(struct inode *inode, struct file *filep);
ssize_t sys4led_read(struct file *filep, char __user *buf, size_t count, loff_t *f_pos);
ssize_t sys4led_write(struct file *filep, char __user *buf, size_t count, loff_t *f_pos);

struct file_operations sys4led_fops = {
  .owner = THIS_MODULE,
  .read = sys4led_read,
  .open = sys4led_open,
  .release = sys4led_release,
  .write = sys4led_write,
};

static struct cdev *cdevStruct;
static int devno;

static int __init sys4led_init(void)
{
  printk(KERN_ALERT "SYS_LED4 driver loaded\n");
  
  //Make device number
  devno = MKDEV(SYS4LED_MAJOR, SYS4LED_MINOR);
  
  //Request GPIO
  int err = 0;
  err = gpio_request(SYS4LED_GPIO, "SYS_LED4");
  
  if(err < 0)
  {
    printk(KERN_ALERT "ERROR requesting GPIO: %d\n", err);
    goto err_exit;
  }
  
  //Set GPIO direction (in or out)
  err = gpio_direction_output(SYS4LED_GPIO, 0);
  
  if(err < 0)
  {
    printk(KERN_ALERT "ERROR changing GPIO direction: %d\n", err);
    goto err_freegpio;
  }
  
  //Register device
  err = register_chrdev_region(devno, 1, "SYS_LED4");
  
  if(err < 0)
  {
    printk(KERN_ALERT "ERROR register cdev: %d\n", err);
    goto err_freegpio;
  }
  
  //Cdev init
  cdevStruct = cdev_alloc();
  cdev_init(cdevStruct, &sys4led_fops);
  
  //Add Cdev
  err = cdev_add(cdevStruct, devno, 1);
  if(err < 0)
  {
    printk(KERN_ALERT "ERROR adding cdev: %d\n", err);
    goto err_unregister_chardev;
  }
  
  //Succes
  printk(KERN_ALERT "SYS_LED4 driver loaded successfully\n");
  return 0;
  
  //Goto erros
  err_unregister_chardev:
    unregister_chrdev_region(devno, 1);
  err_freegpio:
    gpio_free(SYS4LED_GPIO); 
  err_exit:
    return err;
  
}

static void __exit sys4led_exit(void)
{ 
  //Delete Cdev
  cdev_del(cdevStruct);

  //Unregister device
  unregister_chrdev_region(devno, 1);
  
  //Free GPIO
  gpio_free(SYS4LED_GPIO);
  
  //Succes
  printk(KERN_ALERT "SYS_LED4 driver unloaded\n");
}

module_init(sys4led_init);
module_exit(sys4led_exit);

int sys4led_open(struct inode *inode, struct file *filep)
{
  //Reading Major and Minor
  int major, minor;
  major = MAJOR(inode->i_rdev);
  minor = MINOR(inode->i_rdev);
  
  //Printing
  printk("Opening MyGpio Device:\n- Major: %i\n- Minor: %i\n\n", major, minor);
  return 0; //??
}

int sys4led_release(struct inode *inode, struct file *filep)
{
  //Reading Major and Minor
  int minor, major;
  major = MAJOR(inode->i_rdev);
  minor = MINOR(inode->i_rdev);
  
  //Printing
  printk("Closing/Releasing MyGpio Device: \n- Major: %i\n- Minor: %i\n\n", major, minor);
  return 0; //??
}

ssize_t sys4led_read(struct file *filep, char __user *buf, size_t count, loff_t *f_pos)
{
  //Bufferen har en størrelse på 2, da readBootkey enten kan være 0 eller 1 og så plus NULL
  char buffer[BUFFER_LENGTH];
  int bufferLength = sizeof(buffer);
  
  int readLED = gpio_get_value(SYS4LED_GPIO);
  
  bufferLength = sprintf(buffer, "%i", readLED);
  
  if(copy_to_user(buf, buffer, bufferLength) != 0)
  {
    printk(KERN_ALERT "Could not copy to user space..\n");
  }
  
  //Flytter fil position
  *f_pos += bufferLength;
  
  return bufferLength;
}

ssize_t sys4led_write(struct file *filep, char __user *buf, size_t count, loff_t *f_pos)
{
  char buffer[BUFFER_LENGTH];

  if(copy_from_user(buffer, buf, count) != 0)
  {
    printk(KERN_ALERT "Could not copy from user space..\n");
  }  
  
  int value = 0;
  sscanf(buffer, "%d", &value);
  
  gpio_set_value(SYS4LED_GPIO, value);
  
  //Flytter fil position
  *f_pos += count;
  
  return count;
}


