#include "FeatureQueue.h"

void FeatureQueue::enqueue(void *new_item) {
    {
        std::unique_lock<std::mutex> lk(m);
        q.push(new_item);
    }
    cv.notify_all();
}

void* FeatureQueue::dequeue() {
    void* ret = NULL;
    {
        std::unique_lock<std::mutex> lk(m);
        while (q.empty() && !done)
            cv.wait(lk);
        if (!done) {
            ret = q.front();
            q.pop();
        }
    }
    return ret;
}

void FeatureQueue::finish() {
    {
        std::unique_lock<std::mutex> lk(m);
        done = true;
    }
    cv.notify_all();
}
