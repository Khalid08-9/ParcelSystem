#ifndef ACTIVITY_HPP
#define ACTIVITY_HPP
#include <string>
#include <ctime>
#include "Status.hpp"
#include "Result.hpp"

// Task 4: Parcel Activity and Record Management
// One Activity is one entry in the history stack.

enum class ActivityType
{
    Registered,       // parcel entered the system
    Routed,           // taken from the waiting queue and sent to its next stage
    StatusChanged,    // status updated by staff
    DetailsEdited,    // recipient name, recipient ID or address changed
    ResourceAssigned, // a collection counter or courier took the parcel
    ResourceReleased, // that resource was freed again
    Removed,          // parcel record deleted from the registry
    Undo              // an earlier activity was reversed
};

struct Activity
{
    int sequenceNo = 0;   // 1, 2, 3, ... gives the order activities happened in
    std::string parcelID;
    ActivityType type = ActivityType::Registered;
    ParcelStatus previousStatus = ParcelStatus::Registered;
    ParcelStatus newStatus = ParcelStatus::Registered;
    std::string requestID;  // Task 2 request linked at the time (may be empty)
    std::string resourceID; // Task 3 resource involved (may be empty)

    // Recipient details before an edit, so DetailsEdited can be undone
    std::string oldName;
    std::string oldRecipientID;
    std::string oldAddress;

    int undoneSequenceNo = 0; // Undo entries only: which activity was reversed
    std::time_t time = 0;
    bool reversed = false;    // true once this activity has been undone
};

std::string activityTypeToString(ActivityType t);

// Message for the user after an activity-history operation (requirement 4.5)
std::string activityResultMessage(ResultCode code);

#endif
