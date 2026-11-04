// Task 2: Parcel Service and Collection Management
// Owner: [Name, TP number]
// Helper functions for ServiceRequest: names, the priority rule and messages.

#include "ServiceRequest.hpp"
#include "Resource.hpp"
#include <string>

std::string serviceTypeToString(ServiceType t) {
    switch (t) {
        case ServiceType::Collection:      return "Collection";
        case ServiceType::CampusDelivery:  return "Campus Delivery";
        case ServiceType::SpecialHandling: return "Special Handling";
        case ServiceType::ReDelivery:      return "Re-delivery";
        case ServiceType::ReturnToSender:  return "Return to Sender";
        default:                           return "Unknown";
    }
}

std::string urgencyToString(Urgency u) {
    return u == Urgency::Urgent ? "Urgent" : "Normal";
}

std::string requestStatusToString(RequestStatus s) {
    switch (s) {
        case RequestStatus::Pending:    return "Pending";
        case RequestStatus::InProgress: return "In Progress";
        case RequestStatus::Completed:  return "Completed";
        case RequestStatus::Failed:     return "Failed";
        case RequestStatus::Returned:   return "Returned";
        case RequestStatus::Cancelled:  return "Cancelled";
        default:                        return "Unknown";
    }
}

bool isClosedRequest(RequestStatus s) {
    return s != RequestStatus::Pending && s != RequestStatus::InProgress;
}

// Why this order:
//  - Special handling: fragile parcels are at risk while they wait.
//  - Re-delivery: the recipient has already been let down once.
//  - Campus delivery: a courier trip has to be planned.
//  - Return to sender: no recipient on campus is waiting for it.
//  - Collection: the parcel sits safely at the counter until the recipient
//    comes; if someone is standing at the counter, staff mark it Urgent.
int serviceTypeRank(ServiceType t) {
    switch (t) {
        case ServiceType::SpecialHandling: return 1;
        case ServiceType::ReDelivery:      return 2;
        case ServiceType::CampusDelivery:  return 3;
        case ServiceType::ReturnToSender:  return 4;
        case ServiceType::Collection:      return 5;
        default:                           return 6;
    }
}

bool hasHigherPriority(const ServiceRequest& a, const ServiceRequest& b) {
    if (a.urgency != b.urgency) {
        return a.urgency == Urgency::Urgent;
    }
    int rankA = serviceTypeRank(a.type);
    int rankB = serviceTypeRank(b.type);
    if (rankA != rankB) {
        return rankA < rankB;
    }
    // Equivalent processing conditions: first come, first served.
    // The sequence number is used instead of the time because two requests
    // can be created in the same second, but never get the same number.
    return a.sequenceNo < b.sequenceNo;
}

bool serviceMatchesParcel(ServiceType t, ParcelStatus parcelStatus) {
    switch (t) {
        case ServiceType::Collection:      return parcelStatus == ParcelStatus::ReadyForCollection;
        case ServiceType::CampusDelivery:  return parcelStatus == ParcelStatus::OutForDelivery;
        case ServiceType::ReDelivery:      return parcelStatus == ParcelStatus::OutForDelivery;
        case ServiceType::SpecialHandling: return parcelStatus == ParcelStatus::SpecialHandling;
        case ServiceType::ReturnToSender:  return parcelStatus == ParcelStatus::ReturnToSender;
        default:                           return false;
    }
}

std::string serviceResultMessage(ResultCode code) {
    switch (code) {
        case ResultCode::Success:          return "Done.";
        case ResultCode::NotFound:         return "Error: request ID or parcel ID not found.";
        case ResultCode::Duplicate:        return "Error: this parcel already has an active service request.";
        case ResultCode::InvalidInput:     return "Error: required information is missing or invalid.";
        case ResultCode::InvalidStatus:    return "Error: that operation is not allowed for this request.";
        case ResultCode::Empty:            return "Error: there are no service requests waiting.";
        case ResultCode::AlreadyCompleted: return "Error: this request (or its parcel) is already completed.";
        case ResultCode::WrongRecipient:   return "Error: recipient / collection details do not match the parcel.";
        case ResultCode::RequestCancelled: return "Error: this request was cancelled and cannot be processed.";
        case ResultCode::ServiceMismatch:  return "Error: this service does not match the parcel's current route/status.";
        case ResultCode::NotAssigned:      return "Error: no resource is handling this request. Start it again to assign one.";
        default:
            // Codes coming from Task 3 while assigning or releasing a resource
            return resourceResultMessage(code);
    }
}
