#include <linux/init.h>           // Macros used to mark up functions e.g. __init __exit
#include <linux/module.h>         // Core header for loading LKMs into the kernel
#include <linux/device.h>         // Header to support the kernel Driver Model
#include <linux/kernel.h>         // Contains types, macros, functions for the kernel
#include <linux/fs.h>             // Header for the Linux file system support
#include <linux/signal.h>

#include <asm/siginfo.h>
#include <linux/pid_namespace.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/rcupdate.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/random.h>
// #include <linux/mutex.h>
// #include<linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/string.h>

#define  DEVICE_NAME "sysmon"        ///< The device will appear at /dev/sysmon using this value
#define  CLASS_NAME  "sysmon"        ///< The device class -- this is a character device driver

MODULE_LICENSE("GPL");              ///< The license type -- this affects available functionality
MODULE_AUTHOR("Gautam Verma");      ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("Linux char driver for Sysmon");  ///< The description -- see modinfo
MODULE_VERSION("1.0");              ///< A version number to inform users

static int    majorNumber;          ///< Stores the device number -- determined automatically

static int TOTAL_ENGINES;

static bool *firstFlipOnEngines;

static unsigned long long * lastFlipTime;

static bool isConnectedToProcess = false;

static char basePath[] = "/sys/ibmuftian/fpga/";
static char totalCardsPath[] = "/sys/ibmuftian/fpga/fpga_count";

static DECLARE_WAIT_QUEUE_HEAD(wait_queue);
static bool wait_queue_flag;

// static DEFINE_MUTEX(dp_list_mutex);
// static DEFINE_SEMAPHORE(dp_list_semaphore);
static DEFINE_SPINLOCK(dp_list_lock);

static struct class*  SYSMONClass  = NULL; ///< The device-driver class struct pointer
static struct device* SYSMONDevice = NULL; ///< The device-driver device struct pointer

// The prototype functions for the character driver -- must come before the struct definition
void           set_flipped_engine(int, int);
static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

void init_function(void(*func)(int, int));

struct file_operations fops =
{
   .open = dev_open,
   .write = dev_write,
   .read = dev_read,
   .release = dev_release,
};

typedef struct data_packet
{
    int engine;
    int deltaTime;
} data_packet;

typedef struct data_packet_list_node
{
    data_packet dp;
    struct data_packet_list_node *next;
} data_packet_list_node;

static data_packet_list_node *storage_dp_list_head, *storage_dp_list_tail;
static data_packet_list_node *active_dp_list_head, *active_dp_list_tail;
static int const INITIAL_STORAGE_LIST_NODES = 1000;

static struct file * file_open(char * path, int flags, int rights)
{
    struct file *filp = NULL;
    mm_segment_t oldfs;
    int err = 0;

    printk("\nOpening at: %s", path);

    oldfs = get_fs();
    set_fs(get_ds());
    filp = filp_open(path, flags, rights);
    set_fs(oldfs);
    if (IS_ERR(filp))
    {
        err = PTR_ERR(filp);
        return NULL;
    }

    return filp;
}

static int file_read(struct file * filep, char * place_holder)
{
    mm_segment_t oldfs;
    int ret = 0;
    unsigned long long offset = 0;

    oldfs = get_fs();
    set_fs(get_ds());
    
    ret = vfs_read(filep, place_holder, 1, &offset);
  
    set_fs(oldfs);
    return ret;
}

data_packet_list_node * getNodeFromStorageList(bool shouldReturnNewNode)
{
    data_packet_list_node * nodeToReturn = storage_dp_list_head;
    if(nodeToReturn == NULL)
    {
        if(shouldReturnNewNode)
        {
            nodeToReturn = (data_packet_list_node *)kmalloc(sizeof(*nodeToReturn), GFP_KERNEL);
            nodeToReturn->next = NULL;
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        if(storage_dp_list_head == storage_dp_list_tail)
        {
            storage_dp_list_tail = storage_dp_list_tail->next;
        }
        storage_dp_list_head = storage_dp_list_head->next;
    }

    return nodeToReturn;
}

data_packet_list_node * getNodeFromActiveList(void)
{
    data_packet_list_node * nodeToReturn = active_dp_list_head;
    if(nodeToReturn == NULL)
    {
        return NULL;
    }
    else
    {
        if (active_dp_list_head == active_dp_list_tail)
        {
            active_dp_list_tail = active_dp_list_tail->next;
        }
        active_dp_list_head = active_dp_list_head->next;
    }
  
    return nodeToReturn;
}

void addNodeToStorageList(data_packet_list_node * node)
{
    if(storage_dp_list_tail == NULL)
    {
        storage_dp_list_tail = storage_dp_list_head = node;
        node->next = NULL;
    }
    else
    {
        storage_dp_list_tail->next = node;
        node->next = NULL;
        storage_dp_list_tail = node;
    }
}

void addNodeToActiveList(data_packet_list_node * node)
{
    if(active_dp_list_tail == NULL)
    {
        active_dp_list_tail = active_dp_list_head = node;
        node->next = NULL;
    }
    else
    {
        active_dp_list_tail->next = node;
        node->next = NULL;
        active_dp_list_tail = node;
    }
}

/** @brief The LKM initialization function
 *  The static keyword restricts the visibility of the function to within this C file. The __init
 *  macro means that for a built-in driver (not a LKM) the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point.
 *  @return returns 0 if successful
 */
static int __init sysmon_init(void)
{
    int TOTAL_CARDS = 0, card = 0;
    char cardsBuffer[2];
    struct file *f = file_open(totalCardsPath, O_RDONLY, 0);

    printk(KERN_INFO "SysMon: Initializing the SYSMON LKM\n");
 
    if (f == NULL)
    {
        printk(KERN_ERR "can't find total cards count file.");
        return -1;
    }
    else
    {
        cardsBuffer[1] = '\0';
        file_read(f, cardsBuffer);
        TOTAL_CARDS = cardsBuffer[0] - '0';
        filp_close(f, NULL);
        
        for(card = 0; card < TOTAL_CARDS; ++card)
        {
            char cardAsString[2];
            char * totalEnginesPath = kmalloc((strlen(basePath) + 15) * sizeof(char), GFP_KERNEL);
            int enginesOnThisCard;
            strcpy(totalEnginesPath, basePath);
            
            cardAsString[0] = '0' + card;
            cardAsString[1] = '\0';
            strcat(totalEnginesPath, cardAsString);
            strcat(totalEnginesPath, "/engn_count");
            f = file_open(totalEnginesPath, O_RDONLY, 0);
            if(f == NULL)
            {
                printk(KERN_ERR "cound not find total engines on card %d.", card);
                return -1;
            }
            else
            {
                char enginesBuffer[2];
                file_read(f, enginesBuffer);
                enginesOnThisCard = enginesBuffer[0] - '0';
                TOTAL_ENGINES += enginesOnThisCard;
            }
        }
        firstFlipOnEngines = (bool *)kmalloc(TOTAL_ENGINES * sizeof(bool), GFP_KERNEL);
        lastFlipTime = (unsigned long long *) kmalloc(TOTAL_ENGINES * sizeof(unsigned long), GFP_KERNEL);
    }
 
    // Try to dynamically allocate a major number for the device -- more difficult but worth it
    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber<0)
    {
        printk(KERN_ALERT "SYSMON failed to register a major number\n");
        return majorNumber;
    }
    printk(KERN_INFO "SYSMON: registered correctly with major number %d\n", majorNumber);
 
    // Register the device class
    SYSMONClass = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(SYSMONClass)) // Check for error and clean up if there is
    {                
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to register device class\n");
        return PTR_ERR(SYSMONClass);          // Correct way to return an error on a pointer
    }
    printk(KERN_INFO "SYSMON: device class registered correctly\n");
 
    // Register the device driver
    SYSMONDevice = device_create(SYSMONClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if (IS_ERR(SYSMONDevice))               // Clean up if there is an error
    {
         class_destroy(SYSMONClass);           // Repeated code but the alternative is goto statements
         unregister_chrdev(majorNumber, DEVICE_NAME);
         printk(KERN_ALERT "Failed to create the device\n");
         return PTR_ERR(SYSMONDevice);
    }
 
    printk(KERN_INFO "SYSMON: device class created correctly\n"); // Made it! device was initialized
 
    return 0;
}

/** @brief The LKM cleanup function
 *  Similar to the initialization function, it is static. The __exit macro notifies that if this
 *  code is used for a built-in driver (not a LKM) that this function is not required.
 */
static void __exit sysmon_exit(void)
{
    data_packet_list_node *dp = NULL;
    unsigned long flags;

    device_destroy(SYSMONClass, MKDEV(majorNumber, 0));     // remove the device
    class_unregister(SYSMONClass);                          // unregister the device class
    class_destroy(SYSMONClass);                             // remove the device class
    unregister_chrdev(majorNumber, DEVICE_NAME);             // unregister the major number

    dp = getNodeFromActiveList();
    
    spin_lock_irqsave(&dp_list_lock, flags);

    while(dp != NULL)
    {
        kfree(dp);
        dp = getNodeFromActiveList();
    }

    dp = getNodeFromStorageList(false);

    while(dp != NULL)
    {
        kfree(dp);
        dp = getNodeFromStorageList(false);
    }
  
    spin_unlock_irqrestore(&dp_list_lock, flags);
  
    printk(KERN_INFO "SYSMON: Goodbye from the LKM!\n");
}

unsigned long long current_timestamp(void)
{
    unsigned long long milliseconds;
    struct timeval te;

    do_gettimeofday(&te); // get current time
    milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // caculate milliseconds

    return milliseconds;
}

void set_flipped_engine(int engine, int value)
{
    unsigned long long currentTime;
    unsigned long deltaTime, flags;
    data_packet_list_node* storage_dp_list_element = NULL;

    if(!firstFlipOnEngines[engine])
    {
        if(value == 1)// first flip should switch to 1
        { 
            firstFlipOnEngines[engine] = true;
            lastFlipTime[engine] = current_timestamp();
        }
        else
        {
            return;
        }
    }

    currentTime = current_timestamp();
    deltaTime = currentTime - lastFlipTime[engine];
    if(currentTime < lastFlipTime[engine])
    {
        deltaTime = 86400000 + currentTime - lastFlipTime[engine];
    }

    if(deltaTime == 0) return;

    storage_dp_list_element = getNodeFromStorageList(true);
    storage_dp_list_element->dp.engine = engine;
    storage_dp_list_element->dp.deltaTime = deltaTime;
    lastFlipTime[engine] = currentTime;
    
    spin_lock_irqsave(&dp_list_lock, flags);

    addNodeToActiveList(storage_dp_list_element);
    if(wait_queue_flag == true)
    {
        wait_queue_flag = false;
        wake_up_interruptible(&wait_queue);
    }

    spin_unlock_irqrestore(&dp_list_lock, flags);
}

static int dev_open(struct inode *inodep, struct file *filep)
{
    int engine = 0, storage_list_index = 0;
    data_packet_list_node * storage_dp_node;

    if (isConnectedToProcess)
    {
        printk(KERN_ERR "SysMon: Already connected with a process. Exiting...");
        return -EBUSY;
    }
    else
    {
        isConnectedToProcess = true;
    }

    wait_queue_flag = false;
    
    for(engine = 0; engine < TOTAL_ENGINES; ++engine)
    {
        firstFlipOnEngines[engine] = false;
    }
  
    active_dp_list_head = active_dp_list_tail = NULL;
  
    while(storage_list_index < INITIAL_STORAGE_LIST_NODES)
    {
        storage_dp_node = (data_packet_list_node *)kmalloc(sizeof(*storage_dp_node), GFP_KERNEL);
        storage_dp_node->next = NULL;
        addNodeToStorageList(storage_dp_node);
        storage_list_index++;
    }
  
    init_function(set_flipped_engine);
  
    printk("SysMon: DEVICE successfully opened\n");
    return 0;
}

int getTotalDigitsInNumber(int number)
{
    int digits = 1;
    int numberCopy = number / 10;

    while (numberCopy > 0)
    {
        digits++;
        numberCopy /= 10;
    }

    return digits;
}

static ssize_t dev_read(struct file *filep, char * buffer, size_t len, loff_t *offset)
{
    int error_count;
    unsigned long flags;
    char msg[len];
    char *cur = msg, * const end = msg + sizeof msg;
    data_packet_list_node *active_dp_list_element;
    data_packet *dp;
  
    if(active_dp_list_head == NULL){
      // Put the process to sleep
      wait_queue_flag = true;
      wait_event_interruptible(wait_queue, wait_queue_flag == false);
    }
    
    active_dp_list_element = getNodeFromActiveList();
        
    spin_lock_irqsave(&dp_list_lock, flags);

    while(active_dp_list_element != NULL)
    {
        int digitsInEngine, digitsInDeltaTime;
        dp = &(active_dp_list_element->dp);
        digitsInEngine = getTotalDigitsInNumber(dp->engine);
        digitsInDeltaTime = getTotalDigitsInNumber(dp->deltaTime);
        if(cur + digitsInEngine + digitsInDeltaTime + 3 >= end) break;
        cur += snprintf(cur, end - cur, "%d,%d.", dp->engine, dp->deltaTime);
        addNodeToStorageList(active_dp_list_element);
        active_dp_list_element = getNodeFromActiveList();
    }

    spin_unlock_irqrestore(&dp_list_lock, flags);

    error_count = copy_to_user(buffer, msg, strlen(msg));
    if (error_count==0)// if true then have success
    {            
        return 0;
    }
    else
    {
        printk(KERN_INFO "SysMon: Failed to send %d characters to the user\n", error_count);
        return -EFAULT;              // Failed -- return a bad address message (i.e. -14)
    }
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset)
{
    if(wait_queue_flag == true)
    {
        wait_queue_flag = false;
        wake_up_interruptible(&wait_queue);
    }

    return len;
}

static int dev_release(struct inode *inodep, struct file *filep)
{
    unsigned long flags;
    data_packet_list_node *dp = getNodeFromActiveList();
    isConnectedToProcess = false;

    if(wait_queue_flag == true)
    {
        wait_queue_flag = false;
        wake_up_interruptible(&wait_queue);
    }

    spin_lock_irqsave(&dp_list_lock, flags);

    while(dp != NULL)
    {
        kfree(dp);
        dp = getNodeFromActiveList();
    }

    dp = getNodeFromStorageList(false);

    while(dp != NULL)
    {
        kfree(dp);
        dp = getNodeFromStorageList(false);
    }

    spin_unlock_irqrestore(&dp_list_lock, flags);

    init_function(NULL);
    printk(KERN_INFO "SYSMON: Device successfully closed\n");

    return 0;
}

module_init(sysmon_init);
module_exit(sysmon_exit);
