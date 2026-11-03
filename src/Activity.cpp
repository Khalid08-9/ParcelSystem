#include "Activity.hpp"
#include <string>

std::string activityTypeToString(ActivityType t) {
    switch (t) {
        case ActivityType::Registered:       return "Registered";
        case ActivityType::Routed:           return "Routed";
        case ActivityType::StatusChanged:    return "Status Changed";
        case ActivityType::DetailsEdited:    return "Details Edited";
        case ActivityType::ResourceAssigned: return "Resource Assigned";
        case ActivityType::ResourceReleased: return "Resource Released";
        case ActivityType::Removed:          return "Removed";
        case ActivityType::Undo:             return "Undo";
        default:                             return "Unknown";
    }
}

std::string activityResultMessage(ResultCode code) {
    switch (code) {
        case ResultCode::Success:          return "Done.";
        case ResultCode::Empty:            return "Error: there is no activity history to show or undo.";
        case ResultCode::NotFound:         return "Error: parcel ID or activity number not found.";
        case ResultCode::InvalidInput:     return "Error: the parcel ID or activity number is invalid.";
        case ResultCode::NotReversible:    return "Error: this activity cannot be reversed. Registration, removal, "
                                                  "final statuses and undo entries are permanent, and only the most "
                                                  "recent activity of a parcel can be undone.";
        case ResultCode::AlreadyReversed:  return "Error: this activity has already been reversed.";
        case ResultCode::InvalidStatus:    return "Error: the parcel has changed since this activity, so undoing it "
                                                  "would leave an invalid status.";
        case ResultCode::AlreadyCompleted: return "Error: this parcel is already completed.";
        case ResultCode::AlreadyAssigned:  return "Error: the parcel or the resource is already assigned elsewhere.";
        case ResultCode::ResourceUnavailable: return "Error: the original resource is now unavailable.";
        default:                           return "Error: the operation failed.";
    }
}
