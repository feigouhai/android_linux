/*****************************************************************************
 *
 * Description:
 *
 *
 * Revision History:
 *    Date          Author         Modification
 * ---------------------------------------------------------------------
 * 1. 2006-10-16
 *
 *****************************************************************************/

#include <sys/time.h>

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#include "utils/Logger.h"
#include "common/hi_msg_queue.h"

//SVR_MODULE_DECLARE("COMMONMOD")

static HI_VOID message_half_add(message_half_t* MessageHalf, HI_VOID* UsrMessage)
{
    int status;
    message_hdr_t* Message = (message_hdr_t*)UsrMessage - 1;

    if (!MessageHalf) {
        HI_LOGD("FATAL! message_half_add: NULL MessageHalf\n");
        return;
    }

    if (!UsrMessage) {
        HI_LOGD("FATAL! message_half_add: NULL UsrMessage\n");
        return;
    }

    status = pthread_mutex_lock(&MessageHalf->message_half_mutex);
    if (status != 0) {
        HI_LOGD("FATAL! message_half_add: pthread_mutex_lock failure\n");
        return;
    }

    Message->message_hdr_next = NULL;
    if (NULL == MessageHalf->message_half_head  )
        MessageHalf->message_half_head = Message;
    else
        MessageHalf->message_half_tail->message_hdr_next = Message;
    MessageHalf->message_half_tail = Message;

    status = pthread_mutex_unlock(&MessageHalf->message_half_mutex);
    if (status != 0) {
        HI_LOGD("FATAL! message_half_add: pthread_mutex_unlock failure\n");
        return;
    }

    sem_post(&MessageHalf->message_half_sem);

    return;
}

static HI_VOID* message_half_remove(message_half_t* MessageHalf)
{
    int status;
    message_hdr_t* Message;

    if (!MessageHalf) {
        HI_LOGD("FATAL! message_half_remove: NULL MessageHalf\n");
        return NULL;
    }

    sem_wait(&MessageHalf->message_half_sem);

    status = pthread_mutex_lock(&MessageHalf->message_half_mutex);
    if (status != 0) {
        HI_LOGD("FATAL! message_half_remove: pthread_mutex_lock failure\n");
        return NULL;
    }

    Message                        = MessageHalf->message_half_head;
    MessageHalf->message_half_head = Message->message_hdr_next;

    status = pthread_mutex_unlock(&MessageHalf->message_half_mutex);
    if (status != 0) {
        HI_LOGD("FATAL! message_half_remove: pthread_mutex_unlock failure\n");
        return NULL;
    }

    return Message + 1;
}

HI_VOID message_send (message_queue_t*  MessageQueue, HI_VOID* Message)
{
    if (!MessageQueue) {
        HI_LOGD("FATAL! message_send: NULL MessageQueue\n");
        return;
    }

    /* other assertions defered to message_half_add */
    message_half_add(&MessageQueue->message_queue_queue, Message);
}

HI_VOID* message_receive (message_queue_t* MessageQueue)
{
    if (!MessageQueue) {
        HI_LOGD("FATAL! message_receive: NULL MessageQueue\n");
        return NULL;
    }

    /* other assertions defered to message_half_remove */
    return message_half_remove(&MessageQueue->message_queue_queue);
}

HI_VOID* message_claim (message_queue_t* MessageQueue)
{
    if (!MessageQueue) {
        HI_LOGD("FATAL! message_claim: NULL MessageQueue\n");
        return NULL;
    }

    /* other assertions defered to message_half_remove */
    return message_half_remove(&MessageQueue->message_queue_free);
}

HI_VOID message_release (message_queue_t* MessageQueue, HI_VOID* Message)
{
    if (!MessageQueue) {
        HI_LOGD("FATAL! message_release: NULL MessageQueue\n");
        return;
    }

    /* other assertions defered to message_half_add */
    message_half_add(&MessageQueue->message_queue_free, Message);
}

static HI_VOID message_uninit_queue(message_queue_t* MessageQueue)
{
    int status;

    if (!MessageQueue) {
        HI_LOGD("FATAL! message_uninit_queue: NULL MessageQueue\n");
        return;
    }

    status = sem_destroy(&MessageQueue->message_queue_free.message_half_sem);
    if (-1 ==  status ) {
        HI_LOGD("FATAL! message_uninit_queue: sem_destroy failure, message_queue_free\n");
        return;
    }

    status = sem_destroy(&MessageQueue->message_queue_queue.message_half_sem);
    if (-1 ==  status ) {
        HI_LOGD("FATAL! message_uninit_queue: sem_destroy failure, message_queue_queue\n");
        return;
    }

    status = pthread_mutex_destroy(&MessageQueue->message_queue_free.message_half_mutex);
    if (status != 0) {
        HI_LOGD("FATAL! message_uninit_queue: pthread_mutex_destroy failure, message_queue_free\n");
        return;
    }

    status = pthread_mutex_destroy(&MessageQueue->message_queue_queue.message_half_mutex);
    if (status != 0) {
        HI_LOGD("FATAL! message_uninit_queue: pthread_mutex_destroy failure, message_queue_queue\n");
        return;
    }
    return;
}

static int message_init_generic(message_queue_t* MessageQueue, HI_VOID* memory,
        size_t ElementSize, unsigned int NoElements)
{
    message_hdr_t**   PrevMsgP;
    message_hdr_t*    CurrMsg;
    int               i;
    size_t            Needed;

    if (!MessageQueue) {
        HI_LOGD("FATAL! message_init_generic: NULL MessageQueue\n");
        return FALSE;
    }

    if (!memory) {
        HI_LOGD("FATAL! message_init_generic: NULL memory\n");
        return FALSE;
    }

    ElementSize = (ElementSize + 3) & ~3;
    Needed = ElementSize + sizeof(message_hdr_t);

    CurrMsg = (message_hdr_t*)memory;
    PrevMsgP = &MessageQueue->message_queue_free.message_half_head;

    for (i = 1; i < NoElements; i++) {
        *PrevMsgP = CurrMsg;
        PrevMsgP = &CurrMsg->message_hdr_next;
        CurrMsg = (message_hdr_t*)((char*)CurrMsg + Needed);
    }
    *PrevMsgP = CurrMsg;
    MessageQueue->message_queue_free.message_half_tail = CurrMsg;
    CurrMsg->message_hdr_next = NULL;

    MessageQueue->message_queue_queue.message_half_head = NULL;
    MessageQueue->message_queue_queue.message_half_tail = NULL;

    sem_init(&MessageQueue->message_queue_free.message_half_sem, 0, NoElements);
    sem_init(&MessageQueue->message_queue_queue.message_half_sem, 0, 0);

    /* the following is the same as setting PTHREAD_MUTEX_INITIALIZER*/

    pthread_mutex_init(&MessageQueue->message_queue_free.message_half_mutex, NULL);
    pthread_mutex_init(&MessageQueue->message_queue_queue.message_half_mutex, NULL);

    MessageQueue->message_queue_memory = memory;

    return TRUE;
}

int message_init_queue(message_queue_t* MessageQueue, HI_VOID* memory,
        size_t ElementSize, unsigned int NoElements)
{
    /* assertions defered to message_init_queue */
    return message_init_generic(MessageQueue, memory, ElementSize, NoElements);
}

/*
 * Message queue creation functions
 */

static message_queue_t *message_create_generic(size_t ElementSize,
        unsigned int NoElements)
{
    message_queue_t* MessageQueue;
    HI_VOID* messages;
    size_t Needed;

    /* This calculation needs to be the same as the one performed in
     * message_queue_init() */
    ElementSize = (ElementSize + 3) & ~3;
    Needed = ElementSize + sizeof(message_hdr_t);

    messages = malloc(Needed * NoElements);
    if (NULL == messages  )
        return NULL;

    MessageQueue = malloc(sizeof(message_queue_t));
    if (NULL == MessageQueue  ) {
        free(messages);
    } else {
        message_init_generic(MessageQueue, messages, ElementSize, NoElements);
    }

    return MessageQueue;
}

message_queue_t* message_create_queue(size_t ElementSize,
        unsigned int NoElements)
{
    /* assertions defered to message_create_generic */
    return message_create_generic(ElementSize, NoElements);
}

/*
 * Message queue with timeout functions
 */

static int semaphore_wait_timeout(sem_t *sem, unsigned int time_out_ms)
{
    int status = 0, time_out = 0;

    while (time_out <= time_out_ms) {
        /*
         * sem_trywait immediately returns 0 if it is avaialble,
         * otherwise it immediately returns -1 with EAGAIN error code
         */
        status = sem_trywait(sem);
        if (!status)
            break;

        if (time_out == time_out_ms) {
            //HI_LOG_DEBUG("1 semaphore_wait_timeout: timed out\n");
            return -1;
        }
        /* sleep for 10ms */
        usleep(10000);
        time_out += 10;
    }

    if (time_out <= time_out_ms) {
        return 0;
    } else {
        return -1;
    }
}

static HI_VOID* message_half_remove_timeout(message_half_t* MessageHalf, unsigned int time_out)
{
    message_hdr_t* Message;
    int result;

    if (!MessageHalf) {
        HI_LOGD("FATAL! message_half_remove_timeout: NULL MessageHalf\n");
        return NULL;
    }

    result = semaphore_wait_timeout(&MessageHalf->message_half_sem, time_out);
    if (-1 == result  ) {
        //HI_LOG_DEBUG("FATAL! message_half_remove_timeout: semaphore_wait_timeout failure\n");
        return NULL;
    }

    result = pthread_mutex_lock(&MessageHalf->message_half_mutex);
    if (result != 0) {
        HI_LOGD("FATAL! message_half_remove_timeout: pthread_mutex_lock failure\n");
        return NULL;
    }

    Message                        = MessageHalf->message_half_head;
    MessageHalf->message_half_head = Message->message_hdr_next;

    result = pthread_mutex_unlock(&MessageHalf->message_half_mutex);
    if (result != 0) {
        HI_LOGD("FATAL! message_half_remove_timeout: pthread_mutex_unlock failure\n");
        return NULL;
    }

    return Message + 1;
}

HI_VOID* message_receive_timeout (message_queue_t* MessageQueue, unsigned int time_out)
{
    if (!MessageQueue) {
        HI_LOGD("FATAL! message_receive_timeout: NULL MessageQueue\n");
        return NULL;
    }
    /* other assertions defered to message_half_remove_timeout */
    return message_half_remove_timeout(&MessageQueue->message_queue_queue, time_out);
}

HI_VOID* message_claim_timeout (message_queue_t* MessageQueue, unsigned int time_out)
{
    if (!MessageQueue) {
        HI_LOGD("FATAL! message_claim_timeout: NULL MessageQueue\n");
        return NULL;
    }

    /* other assertions defered to message_half_remove_timeout */
    return message_half_remove_timeout(&MessageQueue->message_queue_free, time_out);
}

/*
 * Message queue vector functions
 */

HI_VOID message_delete_queue (message_queue_t* MessageQueue)
{
    if (!MessageQueue) {
        HI_LOGD("FATAL! message_delete_queue: NULL MessageQueue\n");
        return;
    }

    message_uninit_queue(MessageQueue);
    free(MessageQueue->message_queue_memory);
    free(MessageQueue);
}

HI_MsgQueue_p HI_MsgQueueCreate(size_t           MessageSize,
        unsigned int     Messages)
{
    return message_create_queue(MessageSize, Messages);
}

HI_VOID HI_MsgQueueDestroy(HI_MsgQueue_p Queue)
{
    message_delete_queue(Queue);
}

HI_VOID* HI_MsgClaim(HI_MsgQueue_p Queue, unsigned int Timeout_MS)
{
    if (!Timeout_MS) {
        return message_claim(Queue);
    } else {
        return message_claim_timeout(Queue, Timeout_MS);
    }
}

HI_VOID HI_MsgSend(HI_MsgQueue_p Queue, HI_VOID *Message)
{
    message_send(Queue, Message);
}

HI_VOID *HI_MsgReceive(HI_MsgQueue_p Queue, unsigned int Timeout_MS)
{
    if (!Timeout_MS) {
        return message_receive(Queue);
    } else {
        return message_receive_timeout(Queue, Timeout_MS);
    }
}

HI_VOID HI_MsgRelease(HI_MsgQueue_p Queue, HI_VOID *Message)
{
    message_release(Queue, Message);
}

int HI_MsgGetAmount(HI_MsgQueue_p Queue)
{
    int amount = 0;
    int ret = 0;
    ret = sem_getvalue(&Queue->message_queue_queue.message_half_sem, &amount);
    if (0 != ret)
    {
        return -1;
    }
    return amount;
}
