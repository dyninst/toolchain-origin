#ifndef FEATURE_QUEUE_H
#define FEATURE_QUEUE_H

#include <mutex>
#include <condition_variable>
#include <queue>

class FeatureQueue {

    std::mutex m;
    std::condition_variable cv;
    std::queue<void*> q;
    bool done;

public:
    void enqueue(void *item);
    void* dequeue();
    void finish();
    FeatureQueue() : done(false) {}
};

#endif
