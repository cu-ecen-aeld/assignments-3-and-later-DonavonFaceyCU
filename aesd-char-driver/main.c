/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <linux/uaccess.h>
#include <linux/semaphore.h>

#include "aesdchar.h"
#include "aesd_ioctl.h"

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Donavon Facey"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

static int __init aesd_init_module(void);
static void __exit aesd_cleanup_module(void);
static int aesd_open(struct inode *inode, struct file *filp);
static int aesd_release(struct inode *inode, struct file *filp);
static ssize_t aesd_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
static ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
long aesd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
loff_t aesd_llseek(struct file *filp, loff_t offset, int whence);

static void aesd_print(void);

struct aesd_dev aesd_device;

static int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    /**
     * TODO: handle open //DONE
     */

    struct aesd_dev *dev;
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev;
    
    return 0;
}

static int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release //DONE
     */
    return 0;
}

static ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    PDEBUG("\n\n\n\n\n\n\n");
    PDEBUG("Calling read:\tcount: %zu\tf_pos:%lld",count,*f_pos);
    /**
     * TODO: handle read //DONE
     */
    struct aesd_dev* device = (struct aesd_dev*) filp->private_data;
    size_t returnedByteOffset = -1;
    //acquire semaphore
    down(device->aesd_dev_lock);
    struct aesd_buffer_entry* selectedEntry = aesd_circular_buffer_find_entry_offset_for_fpos(device->circularBuffer, *f_pos, &returnedByteOffset);
    
    if(returnedByteOffset == -1){ //did not find a byte at offset f_pos
        //PDEBUG("Failed to find character at desired file position");
        retval = 0;
        goto endfunc;
    }

    size_t bytesLeftInEntry = selectedEntry->size - returnedByteOffset;
    
    if (copy_to_user(buf, selectedEntry->buffptr + returnedByteOffset, bytesLeftInEntry)){
        //PDEBUG("Failed to copy bytes");
        retval = -EFAULT;  // failed to copy some bytes
        goto endfunc;
    }  
    filp->f_pos += bytesLeftInEntry;
    retval = bytesLeftInEntry;

    for(int i = 0; i < bytesLeftInEntry; i++){
        PDEBUG("%c",selectedEntry->buffptr[i]);
    }
    endfunc:
    //release semaphore
    up(device->aesd_dev_lock);
    PDEBUG("Exiting read:\tf_pos:%lld\t\tret:%lu",*f_pos,retval);
    aesd_print();
    return retval;
}

static ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    PDEBUG("\n\n\n\n\n\n\n");
    PDEBUG("Calling write:\tcount: %zu\tf_pos:%lld",count,*f_pos);
    /**
     * TODO: handle write //DONE
     */

    struct aesd_dev* device = (struct aesd_dev*) filp->private_data;

    char* temp_buffer = kmalloc(count, GFP_KERNEL);
    if(!temp_buffer){
        return -ENOMEM;
    }
    

    if (copy_from_user(temp_buffer, buf, count)){
        return -EFAULT;  // failed to copy some bytes
    }  

    int CommandTerminatorIndex = 0;
    for(CommandTerminatorIndex = 0; CommandTerminatorIndex < count; CommandTerminatorIndex++){
        //PDEBUG("Checking for terminator at %i, found char:%c",CommandTerminatorIndex,temp_buffer[CommandTerminatorIndex]);
        if(temp_buffer[CommandTerminatorIndex] == '\n'){
            break;
        }
    }

    size_t bytesToAdd;
    bool foundTerminator = false;
    if(CommandTerminatorIndex == count){ //no terminator found
        bytesToAdd = count;
    } else { //terminator found
        bytesToAdd = CommandTerminatorIndex + 1; //Want to include \n in newTempBuffer
        foundTerminator = true;
    }
    //acquire semaphore
    down(device->aesd_dev_lock);
    
    char* newTempBuffer = krealloc(device->tempEntry->buffptr, device->tempEntry->size + bytesToAdd, GFP_KERNEL);
    if(!newTempBuffer){ //failed to allocate new temp buffer
        retval = -ENOMEM;
        goto endfunc;
    }
    device->tempEntry->buffptr = newTempBuffer;
    memcpy(((char*)device->tempEntry->buffptr) + device->tempEntry->size, temp_buffer, bytesToAdd);
    device->tempEntry->size += bytesToAdd;

    //if \n, krealloc temporary buffer to size to hold \n. Acquire Semaphore. Pass temporary buffer to circular buffer. Release Semaphore. return with partial write.
    if(foundTerminator){
        PDEBUG("Writing to CircularBuffer");
        //copy tempEntry into circularBuffer, circularBuffer takes ownership of entry's string
        char* overwrittenEntry = aesd_circular_buffer_add_entry(device->circularBuffer, device->tempEntry); //circular buffer takes ownership of entry
        
        //if old entry is overwritten, free old entry's string
        if(overwrittenEntry){
            kfree(overwrittenEntry);
        }
        
        //reset tempEntry to empty
        device->tempEntry->size = 0;
        device->tempEntry->buffptr = kmalloc(0, GFP_KERNEL);
        if(!device->tempEntry->buffptr){
            retval = -ENOMEM;
            goto endfunc;
        }
    }
    
    filp->f_pos += bytesToAdd;
    retval = bytesToAdd;
    

    endfunc:
    //release semaphore
    up(device->aesd_dev_lock);
    PDEBUG("Exiting write:\tf_pos:%lld\t\tret:%lu",*f_pos,retval);
    aesd_print();
    return retval;
}

loff_t aesd_llseek(struct file *filp, loff_t offset, int whence)
{
    PDEBUG("\n\n\n\n\n\n\n");
    PDEBUG("Calling llseek:\toffset: %lld\tf_pos:%lld",offset,filp->f_pos);
    
    struct aesd_dev* device = (struct aesd_dev*) filp->private_data;
    loff_t returnedOffset = 0;
    //acquire semaphore
    down(device->aesd_dev_lock);

    //Check circular buffer length
    loff_t bufferLength = 0;
    
    //this only works because we never actually remove anything from the buffer
    for (int i = 0; i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++){
        bufferLength += device->circularBuffer->entry[i].size;
    }

    //release semaphore
    up(device->aesd_dev_lock);

    returnedOffset = fixed_size_llseek(filp, offset, whence, bufferLength);

    PDEBUG("Exiting llseek:\tf_pos:%lld\t\tret:%lld",filp->f_pos,returnedOffset);
    return returnedOffset;
}

long aesd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
    case AESDCHAR_IOCSEEKTO: {
        struct aesd_dev *device = (struct aesd_dev *)filp->private_data;
        struct aesd_seekto seekto;
        loff_t byteCount = 0;

        if (copy_from_user(&seekto, (const void __user *)arg, sizeof(seekto)))
            return -EFAULT;

        if (seekto.write_cmd >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED) {
            // Circular Buffer not large enough to store that many commands
            return -EINVAL;
        }

        ssize_t startIndex = device->circularBuffer->out_offs;
        ssize_t endIndex = (device->circularBuffer->out_offs + seekto.write_cmd) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

        // Acquire semaphore
        down(device->aesd_dev_lock);

        for (int i = startIndex; i != (endIndex + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i = (i + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
        {
            if (i == endIndex) {
                byteCount += seekto.write_cmd_offset;
            } else {
                byteCount += device->circularBuffer->entry[i].size;
            }
        }

        up(device->aesd_dev_lock);

        return aesd_llseek(filp, byteCount, SEEK_SET);
    }

    default:
        return -EINVAL;
    }
}

struct file_operations aesd_fops = {
    .owner          = THIS_MODULE,
    .read           = aesd_read,
    .write          = aesd_write,
    .open           = aesd_open,
    .release        = aesd_release,
    .llseek         = aesd_llseek,
    .unlocked_ioctl = aesd_ioctl,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



static int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    /**
     * TODO: initialize the AESD specific portion of the device //DONE
     */
    
    aesd_device.circularBuffer = kmalloc(sizeof(struct aesd_circular_buffer),GFP_KERNEL);
    if(!aesd_device.circularBuffer) {
        result = -ENOMEM;
        goto free_endfunc;
    }
    memset(aesd_device.circularBuffer,0,sizeof(struct aesd_circular_buffer));

    aesd_device.tempEntry = kmalloc(sizeof(struct aesd_circular_buffer),GFP_KERNEL);
    if(!aesd_device.tempEntry) {
        result = -ENOMEM;
        goto free_circularBuffer;
    }
    memset(aesd_device.tempEntry,0,sizeof(struct aesd_buffer_entry));

    aesd_device.tempEntry->size = 0;
    aesd_device.tempEntry->buffptr = kmalloc(0, GFP_KERNEL);
    if(!aesd_device.tempEntry->buffptr){
        result = -ENOMEM;
        goto free_tempEntry;
    }

    aesd_device.aesd_dev_lock = kmalloc(sizeof(struct semaphore),GFP_KERNEL);
    if(!aesd_device.aesd_dev_lock) {
        result = -ENOMEM;
        goto free_tempEntryString;
    }
    memset(aesd_device.aesd_dev_lock,0,sizeof(struct aesd_buffer_entry));
    sema_init(aesd_device.aesd_dev_lock, 1);

    result = aesd_setup_cdev(&aesd_device);

    //initialized all correctly
    if(result == 0){
        return result;
    }

    //cleanup path
    //free_chrdev:
    unregister_chrdev_region(dev, 1);
    //free_semaphore:
    kfree(aesd_device.aesd_dev_lock);
    free_tempEntryString:
    kfree(aesd_device.tempEntry->buffptr);
    free_tempEntry:
    kfree(aesd_device.tempEntry);
    free_circularBuffer:
    kfree(aesd_device.circularBuffer);
    free_endfunc:
    return result;

}

static void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary //DONE
     */

    //free circular buffer contents
    uint8_t index;
    struct aesd_buffer_entry *entry;
    AESD_CIRCULAR_BUFFER_FOREACH(entry,aesd_device.circularBuffer,index) {
        kfree(entry->buffptr);
    }

    //free circular buffer struct
    kfree(aesd_device.aesd_dev_lock);
    kfree(aesd_device.tempEntry->buffptr);
    kfree(aesd_device.tempEntry);
    kfree(aesd_device.circularBuffer);

    unregister_chrdev_region(devno, 1);
}

static void aesd_print(void){
    down(aesd_device.aesd_dev_lock);

    PDEBUG("\n");
    PDEBUG("Semaphore : %p", aesd_device.aesd_dev_lock);
    PDEBUG("\tCount: %i",aesd_device.aesd_dev_lock->count);
    
    PDEBUG("\n");
    PDEBUG("Temp Entry : %p", aesd_device.tempEntry);
    PDEBUG("\tSize: %lu", aesd_device.tempEntry->size);
    if(aesd_device.tempEntry->size > 0) { 
        PDEBUG("\tString: %s", aesd_device.tempEntry->buffptr); 
        PDEBUG("\tChars:");
        for(int i = 0; i < aesd_device.tempEntry->size; i++){
            PDEBUG("\t%c", aesd_device.tempEntry->buffptr[i]);
        }
    }
    else {PDEBUG("\tString: {EMPTY}"); }

    PDEBUG("\n");
    PDEBUG("Circular Buffer : %p", aesd_device.circularBuffer);
    PDEBUG("\t Write Offset : %d", aesd_device.circularBuffer->in_offs);
    PDEBUG("\t Read Offset : %d", aesd_device.circularBuffer->out_offs);
    for(int i = 0; i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++){
        PDEBUG("\tEntry %i:", i);
        PDEBUG("\t\tSize: %lu", aesd_device.circularBuffer->entry[i].size);
        if(aesd_device.circularBuffer->entry[i].size > 0) { PDEBUG("\tString: %s", aesd_device.circularBuffer->entry[i].buffptr); }
        else {PDEBUG("\t\tString: {EMPTY}"); }
    }

    up(aesd_device.aesd_dev_lock);
}

module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
