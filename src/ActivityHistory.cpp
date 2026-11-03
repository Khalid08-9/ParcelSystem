#include "ActivityHistory.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>

static bool isBlank(const std::string& text) {
    return text.find_first_not_of(" \t\r\n") == std::string::npos;
}

static std::string formatTime(std::time_t t) {
    char buffer[20];
    std::tm* local = std::localtime(&t);
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", local);
    return std::string(buffer);
}

// The "Details" column: what changed in this activity
static std::string describeChange(const Activity& a) {
    std::string statusChange = statusToString(a.previousStatus) + " -> "
                             + statusToString(a.newStatus);
    std::ostringstream text;
    switch (a.type) {
        case ActivityType::Registered:
            text << "Status " << statusToString(a.newStatus);
            break;
        case ActivityType::Routed:
        case ActivityType::StatusChanged:
            text << statusChange;
            break;
        case ActivityType::DetailsEdited:
            text << "Recipient details updated";
            break;
        case ActivityType::ResourceAssigned:
            text << "Assigned to " << a.resourceID;
            break;
        case ActivityType::ResourceReleased:
            text << "Released from " << a.resourceID;
            break;
        case ActivityType::Removed:
            text << "Record deleted (was " << statusToString(a.previousStatus) << ")";
            break;
        case ActivityType::Undo:
            text << "Reversed #" << a.undoneSequenceNo;
            if (a.previousStatus != a.newStatus) {
                text << " (" << statusChange << ")";
            }
            break;
    }
    if (a.reversed) {
        text << "  [REVERSED]";
    }
    return text.str();
}

ActivityHistory::ActivityHistory(ParcelRegistry& registry, ResourceManager* resources)
    : registry(registry), resources(resources), nextSequenceNo(1) {}

int ActivityHistory::getSize() const {
    return history.getSize();
}

// ======================= Recording =======================

void ActivityHistory::record(Activity a) {
    a.sequenceNo = nextSequenceNo++;
    a.time = std::time(nullptr);
    history.push(a);
}

Activity ActivityHistory::fromParcel(const Parcel& p, ActivityType type) const {
    Activity a;
    a.parcelID = p.parcelID;
    a.type = type;
    a.previousStatus = p.status;
    a.newStatus = p.status;
    a.requestID = p.requestID;
    a.resourceID = p.resourceID;
    return a;
}

void ActivityHistory::recordRegistration(const Parcel& p) {
    record(fromParcel(p, ActivityType::Registered));
}

void ActivityHistory::recordStatusChange(const Parcel& p, ParcelStatus previousStatus,
                                         ActivityType type) {
    Activity a = fromParcel(p, type);
    a.previousStatus = previousStatus;
    record(a);
}

void ActivityHistory::recordDetailsEdit(const Parcel& p, const Parcel& before) {
    Activity a = fromParcel(p, ActivityType::DetailsEdited);
    a.oldName = before.recipientName;
    a.oldRecipientID = before.recipientID;
    a.oldAddress = before.deliveryAddress;
    record(a);
}

void ActivityHistory::recordResourceAssigned(const Parcel& p) {
    record(fromParcel(p, ActivityType::ResourceAssigned));
}

void ActivityHistory::recordResourceReleased(const Parcel& p, const std::string& resourceID) {
    Activity a = fromParcel(p, ActivityType::ResourceReleased);
    a.resourceID = resourceID;   // the parcel no longer holds it
    record(a);
}

void ActivityHistory::recordRemoval(const Parcel& removed) {
    record(fromParcel(removed, ActivityType::Removed));
}

// ======================= Display =======================

void ActivityHistory::printHeader() const {
    std::cout << std::left
              << std::setw(6)  << "No."
              << std::setw(18) << "Time"
              << std::setw(10) << "Parcel"
              << std::setw(19) << "Activity"
              << "Details\n";
    std::cout << std::string(90, '-') << "\n";
}

void ActivityHistory::printActivity(const Activity& a) const {
    std::ostringstream number;
    number << "#" << a.sequenceNo;

    std::cout << std::left
              << std::setw(6)  << number.str()
              << std::setw(18) << formatTime(a.time)
              << std::setw(10) << a.parcelID
              << std::setw(19) << activityTypeToString(a.type)
              << describeChange(a) << "\n";
}

ResultCode ActivityHistory::showRecent(int count) {
    if (history.isEmpty()) {
        return ResultCode::Empty;
    }
    if (count <= 0) {
        return ResultCode::InvalidInput;
    }

    printHeader();
    // Pop the newest entries (printing them newest first) onto a temp stack,
    // then pop them back so the history ends up exactly as before.
    ActivityStack temp;
    Activity a;
    int shown = 0;
    while (shown < count && history.pop(a)) {
        printActivity(a);
        temp.push(a);
        shown++;
    }
    while (temp.pop(a)) {
        history.push(a);
    }

    std::cout << std::string(90, '-') << "\n"
              << "Showing " << shown << " of " << history.getSize()
              << " activities, newest first.\n";
    return ResultCode::Success;
}

ResultCode ActivityHistory::showParcelHistory(const std::string& parcelID) {
    if (isBlank(parcelID)) {
        return ResultCode::InvalidInput;
    }
    if (history.findLatestFor(parcelID) == nullptr) {
        // A real parcel with no history is "empty"; an unknown ID is "not found"
        return registry.findParcel(parcelID) != nullptr ? ResultCode::Empty
                                                        : ResultCode::NotFound;
    }

    // Reverse the stack onto a temp stack so the OLDEST activity is on top.
    // Popping temp then gives the activities in the order they happened,
    // and pushing each one back rebuilds the history unchanged. O(n).
    ActivityStack temp;
    Activity a;
    while (history.pop(a)) {
        temp.push(a);
    }

    std::cout << "\nActivity history for " << parcelID << " (oldest first)\n";
    printHeader();
    int steps = 0;
    while (temp.pop(a)) {
        if (a.parcelID == parcelID) {
            printActivity(a);
            steps++;
        }
        history.push(a);
    }
    std::cout << std::string(90, '-') << "\n"
              << steps << " activities recorded for this parcel.\n";
    return ResultCode::Success;
}

ResultCode ActivityHistory::reviewParcel(const std::string& parcelID) {
    if (isBlank(parcelID)) {
        return ResultCode::InvalidInput;
    }
    Parcel* p = registry.findParcel(parcelID);
    Activity* latest = history.findLatestFor(parcelID);
    if (latest == nullptr) {
        return p != nullptr ? ResultCode::Empty : ResultCode::NotFound;
    }

    std::string requestID  = (p != nullptr && !p->requestID.empty())  ? p->requestID  : "None";
    std::string resourceID = (p != nullptr && !p->resourceID.empty()) ? p->resourceID : "None";

    std::cout << "\nACTIVITY REVIEW: " << parcelID << "\n"
              << std::string(50, '-') << "\n"
              << "Current status:          "
              << (p != nullptr ? statusToString(p->status) : "Record removed") << "\n"
              << "Status before last step: " << statusToString(latest->previousStatus) << "\n"
              << "Request ID:              " << requestID << "\n"
              << "Resource ID:             " << resourceID << "\n"
              << "Most recent activity:    #" << latest->sequenceNo << " "
              << activityTypeToString(latest->type) << " - " << describeChange(*latest)
              << " (" << formatTime(latest->time) << ")\n";

    // Say whether undo is possible right now, and if not, why
    Activity* undoable = history.findLatestUndoable(parcelID);
    std::cout << "Can be undone:           ";
    if (undoable == nullptr) {
        std::cout << "No, nothing left to undo.\n";
    } else {
        ResultCode check = checkUndo(*undoable);
        std::cout << (check == ResultCode::Success ? "Yes" : "No") << ", activity #"
                  << undoable->sequenceNo << " (" << activityTypeToString(undoable->type) << ")\n";
        if (check != ResultCode::Success) {
            std::cout << "  " << activityResultMessage(check) << "\n";
        }
    }

    return showParcelHistory(parcelID);
}

// ======================= Undo =======================

ResultCode ActivityHistory::checkUndo(const Activity& a) {
    if (a.type == ActivityType::Registered || a.type == ActivityType::Removed ||
        a.type == ActivityType::Undo) {
        return ResultCode::NotReversible;
    }
    if (a.reversed) {
        return ResultCode::AlreadyReversed;
    }
    // LIFO per parcel: newer activities of the same parcel must be undone first
    Activity* latest = history.findLatestUndoable(a.parcelID);
    if (latest == nullptr || latest->sequenceNo != a.sequenceNo) {
        return ResultCode::NotReversible;
    }

    Parcel* p = registry.findParcel(a.parcelID);
    if (p == nullptr) {
        return ResultCode::NotFound;
    }

    switch (a.type) {
        case ActivityType::Routed:
        case ActivityType::StatusChanged:
            // Collected / Delivered / Returned / Cancelled happened in the real world
            if (isFinalStatus(a.newStatus)) {
                return ResultCode::NotReversible;
            }
            if (p->status != a.newStatus) {
                return ResultCode::InvalidStatus;
            }
            // A resource is working on the parcel in this status: release it first
            if (!p->resourceID.empty()) {
                return ResultCode::InvalidStatus;
            }
            return ResultCode::Success;

        case ActivityType::DetailsEdited:
            return isFinalStatus(p->status) ? ResultCode::AlreadyCompleted : ResultCode::Success;

        case ActivityType::ResourceAssigned:
            if (resources == nullptr) {
                return ResultCode::NotReversible;
            }
            return p->resourceID == a.resourceID ? ResultCode::Success : ResultCode::InvalidStatus;

        case ActivityType::ResourceReleased:
            if (resources == nullptr) {
                return ResultCode::NotReversible;
            }
            if (!p->resourceID.empty()) {
                return ResultCode::AlreadyAssigned;
            }
            return p->status == a.newStatus ? ResultCode::Success : ResultCode::InvalidStatus;

        default:
            return ResultCode::NotReversible;
    }
}

ResultCode ActivityHistory::reverse(const Activity& a) {
    switch (a.type) {
        case ActivityType::Routed:
        case ActivityType::StatusChanged:
            return registry.restoreStatus(a.parcelID, a.previousStatus);
        case ActivityType::DetailsEdited:
            return registry.updateParcelDetails(a.parcelID, a.oldName,
                                                a.oldRecipientID, a.oldAddress);
        case ActivityType::ResourceAssigned:
            return resources->releaseResource(a.resourceID);
        case ActivityType::ResourceReleased:
            // Task 3 re-checks the resource is still free and the right type
            return resources->assignToResource(a.resourceID, a.parcelID, a.requestID);
        default:
            return ResultCode::NotReversible;
    }
}

ResultCode ActivityHistory::undoActivity(int sequenceNo) {
    if (history.isEmpty()) {
        return ResultCode::Empty;
    }
    if (sequenceNo <= 0) {
        return ResultCode::InvalidInput;
    }
    Activity* target = history.findBySequence(sequenceNo);
    if (target == nullptr) {
        return ResultCode::NotFound;
    }

    // Validate everything first, then change anything
    ResultCode result = checkUndo(*target);
    if (result != ResultCode::Success) {
        return result;
    }

    Parcel* p = registry.findParcel(target->parcelID);
    ParcelStatus statusBefore = p->status;
    result = reverse(*target);
    if (result != ResultCode::Success) {
        return result;
    }

    target->reversed = true;

    Activity undo = fromParcel(*p, ActivityType::Undo);
    undo.previousStatus = statusBefore;
    undo.resourceID = target->resourceID;
    undo.undoneSequenceNo = target->sequenceNo;
    record(undo);
    return ResultCode::Success;
}

ResultCode ActivityHistory::undoLastActivity(int& undoneSequenceNo) {
    undoneSequenceNo = 0;
    Activity* latest = history.findLatestUndoable("");
    if (latest == nullptr) {
        return ResultCode::Empty;
    }
    undoneSequenceNo = latest->sequenceNo;
    return undoActivity(latest->sequenceNo);
}
