#include "ActivityStack.hpp"

ActivityStack::ActivityStack() {
    top = nullptr;
    size = 0;
}

ActivityStack::~ActivityStack() {
    clear();
}

bool ActivityStack::isEmpty() const {
    return top == nullptr;
}

int ActivityStack::getSize() const {
    return size;
}

void ActivityStack::push(const Activity& a) {
    StackNode* node = new StackNode;
    node->activity = a;
    node->next = top;   // new node sits above the old top
    top = node;
    size++;
}

bool ActivityStack::pop(Activity& out) {
    if (top == nullptr) {
        return false;
    }
    StackNode* temp = top;
    out = temp->activity;
    top = top->next;
    delete temp;
    size--;
    return true;
}

Activity* ActivityStack::peek() {
    if (top == nullptr) {
        return nullptr;
    }
    return &top->activity;
}

void ActivityStack::clear() {
    while (top != nullptr) {
        StackNode* temp = top;
        top = top->next;
        delete temp;
    }
    size = 0;
}

Activity* ActivityStack::findBySequence(int sequenceNo) {
    for (StackNode* current = top; current != nullptr; current = current->next) {
        if (current->activity.sequenceNo == sequenceNo) {
            return &current->activity;
        }
    }
    return nullptr;
}

Activity* ActivityStack::findLatestFor(const std::string& parcelID) {
    for (StackNode* current = top; current != nullptr; current = current->next) {
        if (current->activity.parcelID == parcelID) {
            return &current->activity;
        }
    }
    return nullptr;
}

Activity* ActivityStack::findLatestUndoable(const std::string& parcelID) {
    for (StackNode* current = top; current != nullptr; current = current->next) {
        Activity& a = current->activity;
        bool parcelMatches = parcelID.empty() || a.parcelID == parcelID;
        if (parcelMatches && a.type != ActivityType::Undo && !a.reversed) {
            return &a;
        }
    }
    return nullptr;
}
