#ifndef REQUEST_PRIORITY_LIST_HPP
#define REQUEST_PRIORITY_LIST_HPP
#include <string>
#include "ServiceRequest.hpp"

// Task 2: priority queue of pending service requests, built as a
// SORTED singly linked list. The head is always the next request to handle.
// Owner: [Name, TP number]
//
//   insert      O(n)  walk to the first node with LOWER priority, link in before it
//   peek        O(1)  the head
//   removeFront O(1)  unlink the head
//   remove(id)  O(n)  find and unlink (cancel / start a selected request)
//
// The list only holds pointers; the ServiceManager owns the requests.

struct RequestNode
{
    ServiceRequest* request;
    RequestNode* next;
};

class RequestPriorityList
{
    private:
      RequestNode* head;
      int size;

    public:
      RequestPriorityList();
      ~RequestPriorityList();
      // Copying would make two lists share (and twice delete) the same nodes
      RequestPriorityList(const RequestPriorityList&) = delete;
      RequestPriorityList& operator=(const RequestPriorityList&) = delete;
      bool isEmpty() const;
      int getSize() const;
      // Keeps the list sorted by hasHigherPriority(); equal requests keep
      // their arrival order because a new node goes AFTER its equals.
      void insert(ServiceRequest* r);
      ServiceRequest* peek() const;   // nullptr when empty
      ServiceRequest* removeFront();  // nullptr when empty
      ServiceRequest* remove(const std::string& requestID); // nullptr if not in list
      ServiceRequest* find(const std::string& requestID) const;
      int positionOf(const std::string& requestID) const;   // 1-based, 0 if absent
      // Calls visit(request, position) from the head (next to handle) to the tail
      template <typename Visitor>
      void forEach(Visitor visit) const {
          int position = 1;
          for (RequestNode* n = head; n != nullptr; n = n->next) {
              visit(*n->request, position++);
          }
      }
      void clear();
};

#endif
