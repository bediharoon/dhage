#include <pthread.h>
#include <stdlib.h>

#include "dhage.h"

typedef struct Channel {
    pthread_mutex_t lock;
    pthread_cond_t send_cond;
    pthread_cond_t recv_cond;
    void* data;
    void** recv_buffer;
    int closed;
    int has_sender;
    int has_receiver;
} Channel;

Channel* channel_create() {
    Channel* chan = malloc(sizeof(Channel));
    if (!chan) return NULL;

    pthread_mutex_init(&chan->lock, NULL);
    pthread_cond_init(&chan->send_cond, NULL);
    pthread_cond_init(&chan->recv_cond, NULL);
    chan->data = NULL;
    chan->recv_buffer = NULL;
    chan->closed = 0;
    chan->has_sender = 0;
    chan->has_receiver = 0;
    return chan;
}

int channel_send(Channel* chan, void* data) {
    if (!chan) return -1;

    pthread_mutex_lock(&chan->lock);
    if (chan->closed) {
        pthread_mutex_unlock(&chan->lock);
        return -1;
    }

    while (!chan->has_receiver && !chan->closed) {
        pthread_cond_wait(&chan->send_cond, &chan->lock);
    }

    if (chan->closed) {
        pthread_mutex_unlock(&chan->lock);
        return -1;
    }

    *chan->recv_buffer = data;
    chan->has_sender = 1;
    pthread_cond_signal(&chan->recv_cond);

    while (chan->has_sender) {
        pthread_cond_wait(&chan->send_cond, &chan->lock);
    }

    pthread_mutex_unlock(&chan->lock);
    return 0;
}

int channel_receive(Channel* chan, void** buffer) {
    if (!chan || !buffer) return -1;

    pthread_mutex_lock(&chan->lock);
    if (chan->closed && !chan->has_sender) {
        pthread_mutex_unlock(&chan->lock);
        return -1;
    }

    chan->has_receiver = 1;
    chan->recv_buffer = buffer;
    pthread_cond_signal(&chan->send_cond);

    while (!chan->has_sender && !chan->closed) {
        pthread_cond_wait(&chan->recv_cond, &chan->lock);
    }

    if (chan->closed && !chan->has_sender) {
        chan->has_receiver = 0;
        pthread_mutex_unlock(&chan->lock);
        return -1;
    }

    chan->has_sender = 0;
    chan->has_receiver = 0;
    pthread_cond_signal(&chan->send_cond);
    pthread_mutex_unlock(&chan->lock);
    return 0;
}

void channel_close(Channel* chan) {
    if (!chan) return;

    pthread_mutex_lock(&chan->lock);
    chan->closed = 1;
    pthread_cond_broadcast(&chan->send_cond);
    pthread_cond_broadcast(&chan->recv_cond);
    pthread_mutex_unlock(&chan->lock);
}

void channel_destroy(Channel* chan) {
    if (!chan) return;
    
    pthread_mutex_destroy(&chan->lock);
    pthread_cond_destroy(&chan->send_cond);
    pthread_cond_destroy(&chan->recv_cond);
    free(chan);
}
