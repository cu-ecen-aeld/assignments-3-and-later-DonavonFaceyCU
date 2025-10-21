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

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Donavon Facey"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
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

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release //DONE
     */
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle read //DONE
     */
    struct aesd_dev* device = (aesd_dev*) filp->private_data;
    size_t* returnedByteOffset = -1;
    //acquire semaphore
    down(device.aesd_dev_lock);
    aesd_buffer_entry* selectedEntry = aesd_circular_buffer_find_entry_offset_for_fpos(device->circularBuffer, *f_pos, returnedByteOffset);
    
    if(returnedByteOffset == -1){ //did not find a byte at offset f_pos
        retval = 0;
        goto endfunc;
    }

    size_t bytesLeftInEntry = selectedEntry.size - returnedByteOffset;
    
    if (copy_to_user(buf, aesd_buffer_entry.buffptr[returnedByteOffset], bytesLeftInEntry)){
        retval = -EFAULT;  // failed to copy some bytes
        goto endfunc;
    }  
    *f_pos += bytesLeftInEntry;
    retval = bytesLeftInEntry;

    endfunc:
    //release semaphore
    up(device.aesd_dev_lock);
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle write //DONE
     */

    struct aesd_dev* device = (aesd_dev*) filp->private_data;

    char* temp_buffer = kmalloc(count, GFP_KERNEL);
    if(!temp_buffer){
        return -ENOMEM;
    }
    copy_from_user(temp_buffer, buf, count);

    int CommandTerminatorIndex = 0;
    for(CommandTerminatorIndex = 0; CommandTerminatorIndex < count; CommandTerminatorIndex++){
        if(temp_buffer[CommandTerminatorIndex] == '\n'){
            break;
        }
    }

    size_t bytesToAdd;
    bool foundTerminator = false;
    if(CommandTerminatorIndex == count){ //no terminator found
        bytesToAdd = count;
        foundTerminator = true;
    } else { //terminator found
        bytesToAdd = CommandTerminatorIndex + 1; //Want to include \n in newTempBuffer
    }
    //acquire semaphore
    down(device.aesd_dev_lock);
    
    char* newTempBuffer = krealloc(device.tempEntry->buffPtr, device.tempEntry->size + bytesToAdd, GFP_KERNEL);
    if(!newTempBuffer){ //failed to allocate new temp buffer
        retval = -ENOMEM;
        goto endfunc;
    }
    device.tempEntry->buffPtr = newTempBuffer;
    memcpy(device.tempEntry->buffPtr + device.tempEntry->size, newTempBuffer, bytesToAdd);
    device.tempEntry->size += bytesToAdd;

    //if \n, krealloc temporary buffer to size to hold \n. Acquire Semaphore. Pass temporary buffer to circular buffer. Release Semaphore. return with partial write.
    if(foundTerminator){
        //copy tempEntry into circularBuffer, circularBuffer takes ownership of entry's string
        char* overwrittenEntry = aesd_circular_buffer_add_entry(device.circularBuffer, device.tempEntry); //circular buffer takes ownership of entry
        
        //if old entry is overwritten, free old entry's string
        if(overwrittenEntry){
            kfree(overwrittenEntry);
        }
        
        //reset tempEntry to empty
        device.tempEntry.size = 0;
        device.tempEntry.buffptr = kmalloc(0, GFP_KERNEL);
        if(!device.tempEntry.buffptr){
            retval = -ENOMEM;
            goto endfunc;
        }
    }
    
    retval = bytesToAdd;

    endfunc:
    //release semaphore
    up(device.aesd_dev_lock);
    return retval;
}

struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
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



int aesd_init_module(void)
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
    
    aesd_dev.circularBuffer = kmalloc(sizeof(struct aesd_circular_buffer),GFP_KERNEL);
    if(!aesd_dev.circularBuffer) {
        result = -ENOMEM;
        goto free_endfunc;
    }
    memset(aesd_dev.circularBuffer,0,sizeof(struct aesd_circular_buffer));

    aesd_dev.tempEntry = kmalloc(sizeof(struct aesd_circular_buffer),GFP_KERNEL);
    if(!aesd_dev.tempEntry) {
        result = -ENOMEM;
        goto free_circularBuffer;
    }
    memset(aesd_dev.tempEntry,0,sizeof(struct aesd_buffer_entry));

    device.tempEntry->size = 0;
    device.tempEntry->buffptr = kmalloc(0, GFP_KERNEL);
    if(!device.tempEntry->buffptr){
        result = -ENOMEM;
        goto free_tempEntry;
    }

    aesd_dev.aesd_dev_lock = kmalloc(sizeof(struct semaphore),GFP_KERNEL);
    if(!aesd_dev.aesd_dev_lock) {
        result = -ENOMEM;
        goto free_tempEntryString;
    }
    memset(aesd_dev.aesd_dev_lock,0,sizeof(struct aesd_buffer_entry));
    sema_init(aesd_dev.aesd_dev_lock, 1);

    result = aesd_setup_cdev(&aesd_device);

    //initialized all correctly
    if(result == 0){
        return result;
    }

    //cleanup path
    free_chrdev:
    unregister_chrdev_region(dev, 1);
    free_semaphore:
    kfree(aesd_dev.aesd_dev_lock);
    free_tempEntryString:
    kfree(aesd_dev.tempEntry->buffPtr);
    free_tempEntry:
    kfree(aesd_dev.tempEntry);
    free_circularBuffer:
    kfree(aesd_dev.circularBuffer);
    free_endfunc:
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary //DONE
     */

    //free circular buffer contents
    uint8_t index;
    struct aesd_buffer_entry *entry;
    AESD_CIRCULAR_BUFFER_FOREACH(entry,aesd_dev.circularBuffer,index) {
        kfree(entry->buffptr);
    }

    //free circular buffer struct
    kfree(aesd_dev.aesd_dev_lock);
    kfree(aesd_dev.tempEntry->buffPtr);
    kfree(aesd_dev.tempEntry);
    kfree(aesd_dev.circularBuffer);

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
