/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"

#define DEBUG 0

#if DEBUG==1
#include <stdio.h>
static void aesd_buffer_print(struct aesd_circular_buffer *buffer);

static void aesd_buffer_print(struct aesd_circular_buffer *buffer){
    printf("Printing Buffer with in_offs=%u and out_offs=%u and full=%u\n", buffer->in_offs, buffer->out_offs, buffer->full);
    for(int i = 0; i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++){
        printf("\tEntry %u: ",i);
        if(buffer->entry[i].buffptr == NULL){
            printf("Empty");
        } else {
            printf("%s",buffer->entry[i].buffptr);
        }
        printf(" \tSize: %lu",buffer->entry[i].size);
        printf("\n");
    }    
    printf("\n");
}
#endif

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    #if DEBUG==1
    printf("\nCalling find_entry_offset with char_offset=%lu\n",char_offset);
    aesd_buffer_print(buffer);
    #endif
    
    size_t char_difference = char_offset;

    //for each buffer entry
    for(int i = 0; i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++){
        //transform count variable to index
        int buffer_index = (buffer->out_offs + i) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

        //if buffer is not full and crawler reaches input pointer
        if(buffer_index == buffer->in_offs && !buffer->full){
            //End of buffer reached. Return without finding char
            return NULL;
        }

        //If this string does not contain the offsetted char, go to next string, subtract offset of current string
        if(char_difference >= buffer->entry[buffer_index].size){
            char_difference -= buffer->entry[buffer_index].size;

        //Char is contained in current string
        } else {
            *entry_offset_byte_rtn = char_difference;
            #if DEBUG==1
            printf("Returning from entry number %u with entry index %u and char index %lu\n", i, buffer_index, char_difference);
            #endif
            return &(buffer->entry[buffer_index]);
        }

    }
    
    //This is only reached if all entries are searched, buffer is full, and the buffer does not contain enough chars for char_offset
    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
* @return NULL, unless if an entry was replaced, then the entry is returned so caller may free it.
*/
const char* aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    #if DEBUG==1
    //aesd_buffer_print(buffer);
    #endif

    size_t returnValue = NULL;

    if(buffer->full){
        returnValue = buffer->entry[buffer->in_offs].buffptr;
    }

    //copy param to buffer
    memcpy(&(buffer->entry[buffer->in_offs]), add_entry, sizeof(struct aesd_buffer_entry));

    //always increment input pointer
    buffer->in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

    //if already full and was just overwritten, increment out pointer
    if(buffer->full == true){
        buffer->out_offs = (buffer->out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }

    //if both pointers are equal after increments, mark as full
    if(buffer->out_offs == buffer->in_offs){
        buffer->full = true;
    }

    return returnValue;
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
    
    #if DEBUG==1
    //aesd_buffer_print(buffer);
    #endif
}

