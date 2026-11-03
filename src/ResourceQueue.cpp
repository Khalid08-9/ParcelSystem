#include "ResourceQueue.hpp"

ResourceQueue::ResourceQueue() {
    front = 0;
    rear = RESOURCE_CAPACITY - 1; // so the first enqueue lands on index 0
    count = 0;
}

bool ResourceQueue::isEmpty() const {
    return count == 0;
}

bool ResourceQueue::isFull() const {
    return count == RESOURCE_CAPACITY;
}

int ResourceQueue::getSize() const {
    return count;
}

bool ResourceQueue::enqueue(Resource* r) {
    if (isFull()) {
        return false;
    }
    // Step forward one slot; at the last index, % sends it back to 0
    rear = (rear + 1) % RESOURCE_CAPACITY;
    items[rear] = r;
    count++;
    return true;
}

Resource* ResourceQueue::dequeue() {
    if (isEmpty()) {
        return nullptr;
    }
    Resource* r = items[front];
    front = (front + 1) % RESOURCE_CAPACITY;
    count--;
    return r;
}

Resource* ResourceQueue::peek() const {
    if (isEmpty()) {
        return nullptr;
    }
    return items[front];
}

bool ResourceQueue::remove(Resource* r) {
    for (int i = 0; i < count; i++) {
        int pos = (front + i) % RESOURCE_CAPACITY;
        if (items[pos] == r) {
            // Shift everything behind it one slot forward to close the gap
            for (int j = i; j < count - 1; j++) {
                int cur = (front + j) % RESOURCE_CAPACITY;
                int next = (cur + 1) % RESOURCE_CAPACITY;
                items[cur] = items[next];
            }
            rear = (rear - 1 + RESOURCE_CAPACITY) % RESOURCE_CAPACITY;
            count--;
            return true;
        }
    }
    return false;
}
