
#include "ParcelQueue.hpp"

ParcelQueue::ParcelQueue() {
    front = nullptr;
    rear = nullptr;
    size = 0;
}

bool ParcelQueue::isEmpty() const {
    return front == nullptr;
}



void ParcelQueue::enqueue(Parcel* p) {
Queuenode* node = new Queuenode;
node->parcel = p;
node->next = nullptr;

if (front == nullptr){
    front=node;
    rear=node;
}
else{  
    rear ->next=node;
    rear = node;
}
   size++;
}


Parcel* ParcelQueue::peek() const {
    if (front == nullptr) {
        return nullptr;
    }
    return front->parcel;
};


int ParcelQueue::getSize() const {
    return size;
}

Parcel* ParcelQueue::dequeue() {
    if (front == nullptr) {
        return nullptr;
    }

    Queuenode* temp = front;
    Parcel* parcelToReturn = temp->parcel;

    front = front->next;

    if (front == nullptr) {
        rear = nullptr;
    }

    delete temp;
    size--;

    return parcelToReturn;
}


void ParcelQueue::clear() {
    while (front != nullptr) {
        Queuenode* temp = front;
        front = front->next;
        delete temp;
    }
    rear = nullptr;
    size = 0;
}
ParcelQueue::~ParcelQueue() {
    clear();
}

std::string typeToString(ParcelType t);













