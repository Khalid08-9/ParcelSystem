#include <iostream>
#include "Parcel.hpp"
#include "ParcelQueue.hpp"

// Prints PASS or FAIL for each check
void check(const char* name, bool condition) {
    std::cout << (condition ? "PASS: " : "FAIL: ") << name << "\n";
}

int main() {
    ParcelQueue q;

    Parcel p1, p2, p3;
    p1.parcelID = "P001";
    p2.parcelID = "P002";
    p3.parcelID = "P003";

    // 1. A new queue is empty
    check("new queue is empty", q.isEmpty());
    check("size is 0", q.getSize() == 0);
    check("peek on empty returns nullptr", q.peek() == nullptr);
    check("dequeue on empty returns nullptr", q.dequeue() == nullptr);

    // 2. Enqueue three parcels
    q.enqueue(&p1);
    q.enqueue(&p2);
    q.enqueue(&p3);
    check("not empty after enqueue", !q.isEmpty());
    check("size is 3", q.getSize() == 3);
    check("peek shows P001", q.peek() == &p1);
    check("peek does not remove", q.getSize() == 3);

    // 3. Dequeue keeps first-in, first-out order
    check("dequeue gives P001", q.dequeue() == &p1);
    check("size is 2", q.getSize() == 2);
    check("dequeue gives P002", q.dequeue() == &p2);
    check("dequeue gives P003", q.dequeue() == &p3);
    check("empty again", q.isEmpty());
    check("size is 0 again", q.getSize() == 0);

    // 4. Reuse after emptying (checks that rear was reset)
    q.enqueue(&p1);
    check("enqueue after empty works", q.peek() == &p1);
    q.enqueue(&p2);
    check("order still correct", q.dequeue() == &p1 && q.dequeue() == &p2);

    // 5. clear
    q.enqueue(&p1);
    q.enqueue(&p2);
    q.clear();
    check("clear empties the queue", q.isEmpty() && q.getSize() == 0);

    return 0;
};