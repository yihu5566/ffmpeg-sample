//
// Created by dongfang lu on 2022/11/15.
//
#pragma once
#include <queue>
using namespace std;

template<class T>
class SafeQueue {
public:
    SafeQueue() {
        pthread_mutex_init(&mutex, 0);
        pthread_cond_init(&cond, 0);
    }

    ~SafeQueue() {
        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&cond);
    }

    void enqueue(T t) {
        pthread_mutex_lock(&mutex);
        q.push(t);
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mutex);
    }

    int dequeue(T &t) {
        pthread_mutex_lock(&mutex);
        while (q.empty()) {
            pthread_cond_wait(&cond, &mutex);
        }
        t = q.front();
        q.pop();
        pthread_mutex_unlock(&mutex);
        return 1;
    }

private:
    queue<T> q;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};
