#ifndef RESOURCE_QUEUE_HPP
#define RESOURCE_QUEUE_HPP
#include "Resource.hpp"

// Task 3: circular queue of resources, built on a fixed-size array.
// Holds pointers only; the Resource records themselves live elsewhere.

const int RESOURCE_CAPACITY = 20;

class ResourceQueue
{
    private:
      Resource* items[RESOURCE_CAPACITY];
      int front; // index of the next item to dequeue
      int rear;  // index of the last item enqueued
      int count; // how many items are in the queue

    public:
      ResourceQueue();
      bool isEmpty() const;
      bool isFull() const;
      int getSize() const;
      bool enqueue(Resource* r); // false when full
      Resource* dequeue();       // nullptr when empty
      Resource* peek() const;    // nullptr when empty
      bool remove(Resource* r);  // takes r out from anywhere in the queue
};

#endif
