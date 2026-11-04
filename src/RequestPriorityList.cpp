// Task 2: sorted linked list used as the priority queue of pending requests.
// Owner: [Name, TP number]

#include "RequestPriorityList.hpp"

RequestPriorityList::RequestPriorityList() : head(nullptr), size(0) {}

RequestPriorityList::~RequestPriorityList() {
    clear();
}

bool RequestPriorityList::isEmpty() const {
    return head == nullptr;
}

int RequestPriorityList::getSize() const {
    return size;
}

void RequestPriorityList::insert(ServiceRequest* r) {
    if (r == nullptr) {
        return;
    }
    RequestNode* node = new RequestNode{r, nullptr};

    // Case 1: empty list, or the new request beats the current head
    if (head == nullptr || hasHigherPriority(*r, *head->request)) {
        node->next = head;
        head = node;
        size++;
        return;
    }

    // Case 2: walk while the NEXT node is at least as important as the new one.
    // Stopping only at a strictly lower-priority node keeps equal requests in
    // first-come-first-served order (stable insert).
    RequestNode* current = head;
    while (current->next != nullptr && !hasHigherPriority(*r, *current->next->request)) {
        current = current->next;
    }
    node->next = current->next;
    current->next = node;
    size++;
}

ServiceRequest* RequestPriorityList::peek() const {
    return head == nullptr ? nullptr : head->request;
}

ServiceRequest* RequestPriorityList::removeFront() {
    if (head == nullptr) {
        return nullptr;
    }
    RequestNode* old = head;
    ServiceRequest* r = old->request;
    head = head->next;
    delete old;
    size--;
    return r;
}

ServiceRequest* RequestPriorityList::remove(const std::string& requestID) {
    RequestNode* previous = nullptr;
    RequestNode* current = head;
    while (current != nullptr && current->request->requestID != requestID) {
        previous = current;
        current = current->next;
    }
    if (current == nullptr) {
        return nullptr;
    }
    if (previous == nullptr) {
        head = current->next;       // removing the head
    } else {
        previous->next = current->next;
    }
    ServiceRequest* r = current->request;
    delete current;
    size--;
    return r;
}

ServiceRequest* RequestPriorityList::find(const std::string& requestID) const {
    for (RequestNode* n = head; n != nullptr; n = n->next) {
        if (n->request->requestID == requestID) {
            return n->request;
        }
    }
    return nullptr;
}

int RequestPriorityList::positionOf(const std::string& requestID) const {
    int position = 1;
    for (RequestNode* n = head; n != nullptr; n = n->next, position++) {
        if (n->request->requestID == requestID) {
            return position;
        }
    }
    return 0;
}

void RequestPriorityList::clear() {
    while (head != nullptr) {
        RequestNode* old = head;
        head = head->next;
        delete old;
    }
    size = 0;
}
