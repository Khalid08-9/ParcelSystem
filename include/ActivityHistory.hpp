#ifndef ACTIVITY_HISTORY_HPP
#define ACTIVITY_HISTORY_HPP
#include <string>
#include "Activity.hpp"
#include "ActivityStack.hpp"
#include "ParcelRegistry.hpp"
#include "ResourceManager.hpp"
#include "Result.hpp"

// Task 4: keeps every activity on one stack (newest on top), shows the
// history, and reverses the most recent activity of a parcel when allowed.
//
// Undo rules:
//  - Registration, removal, undo entries and moves into a final status
//    (Collected, Delivered, Returned, Cancelled) are permanent.
//  - Per parcel, undo is last-in first-out: only the newest activity of that
//    parcel that has not been reversed can be undone.
//  - The parcel must still be in the state the activity left it in.

class ActivityHistory
{
    private:
      ParcelRegistry& registry;
      ResourceManager* resources; // optional: only needed to undo resource activities
      ActivityStack history;
      int nextSequenceNo;

      void record(Activity a);    // stamps sequence number and time, then pushes
      Activity fromParcel(const Parcel& p, ActivityType type) const;
      ResultCode checkUndo(const Activity& a); // Success if a may be reversed now
      ResultCode reverse(const Activity& a);   // performs the reversal
      void printHeader() const;
      void printActivity(const Activity& a) const;

    public:
      explicit ActivityHistory(ParcelRegistry& registry, ResourceManager* resources = nullptr);

      // ---- Requirement 1: record (call after the operation succeeded) ----
      void recordRegistration(const Parcel& p);
      // type is Routed or StatusChanged; previousStatus is read before the change
      void recordStatusChange(const Parcel& p, ParcelStatus previousStatus,
                              ActivityType type = ActivityType::StatusChanged);
      // before is a copy of the parcel taken before the edit
      void recordDetailsEdit(const Parcel& p, const Parcel& before);
      void recordResourceAssigned(const Parcel& p);
      void recordResourceReleased(const Parcel& p, const std::string& resourceID);
      void recordRemoval(const Parcel& removed); // copy taken before removeParcel

      // ---- Requirements 2 and 3: display and review ----
      ResultCode showRecent(int count);                         // newest first
      ResultCode showParcelHistory(const std::string& parcelID); // oldest first
      ResultCode reviewParcel(const std::string& parcelID);

      // ---- Requirement 4: undo ----
      ResultCode undoActivity(int sequenceNo);
      // Undoes the newest activity that is not yet reversed; reports its number
      ResultCode undoLastActivity(int& undoneSequenceNo);

      int getSize() const;
};

#endif
