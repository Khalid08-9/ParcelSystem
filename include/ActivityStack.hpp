#ifndef ACTIVITY_STACK_HPP
#define ACTIVITY_STACK_HPP
#include <string>
#include "Activity.hpp"

// Task 4: linked-list stack of activities. The newest activity is always on
// top, so "most recent" is O(1) and undo works last-in, first-out.

struct StackNode
{
    Activity activity;
    StackNode* next; // the node below (older activity)
};

class ActivityStack
{
    private:
      StackNode* top;
      int size;

    public:
      ActivityStack();
      ~ActivityStack();
      // The stack owns its nodes, so copying it would double-delete them
      ActivityStack(const ActivityStack&) = delete;
      ActivityStack& operator=(const ActivityStack&) = delete;

      bool isEmpty() const;
      int getSize() const;
      void push(const Activity& a);
      bool pop(Activity& out); // false when empty
      Activity* peek();        // nullptr when empty
      void clear();

      // Linear searches from the top down, so the newest match is found first
      Activity* findBySequence(int sequenceNo);
      Activity* findLatestFor(const std::string& parcelID);
      // Newest activity of the parcel that is not an Undo entry and not yet
      // reversed: the only one that may be undone next.
      // An empty parcelID matches any parcel.
      Activity* findLatestUndoable(const std::string& parcelID);
};

#endif
