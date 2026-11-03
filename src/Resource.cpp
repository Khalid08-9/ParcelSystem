#include "Resource.hpp"
#include <string>

std::string resourceTypeToString(ResourceType t) {
    switch (t) {
        case ResourceType::CollectionCounter:
            return "Collection Counter";
        case ResourceType::DeliveryPersonnel:
            return "Delivery Personnel";
        default:
            return "Unknown";
    }
}

std::string resourceStatusToString(ResourceStatus s) {
    switch (s) {
        case ResourceStatus::Available:
            return "Available";
        case ResourceStatus::Occupied:
            return "Occupied";
        case ResourceStatus::Unavailable:
            return "Unavailable";
        default:
            return "Unknown";
    }
}

bool requiredResourceType(ParcelStatus status, ResourceType &required) {
    switch (status) {
        case ParcelStatus::ReadyForCollection:
            required = ResourceType::CollectionCounter;
            return true;

        case ParcelStatus::OutForDelivery:
        case ParcelStatus::SpecialHandling:
        case ParcelStatus::ReturnToSender:
            required = ResourceType::DeliveryPersonnel;
            return true;

        default:
            return false;    // Registered, OnHold, Missing or final: no resource
    }
}

std::string resourceResultMessage(ResultCode code) {
    switch (code) {
        case ResultCode::Success:              return "Done.";
        case ResultCode::NotFound:             return "Error: resource or parcel ID not found.";
        case ResultCode::InvalidInput:         return "Error: name is blank, or the request belongs to another parcel.";
        case ResultCode::InvalidStatus:        return "Error: that parcel or status change cannot be handled by a resource.";
        case ResultCode::AlreadyCompleted:     return "Error: this parcel is already completed.";
        case ResultCode::Full:                 return "Error: no room to register another resource.";
        case ResultCode::AllResourcesOccupied: return "Error: all resources of this type are occupied. Release one and try again.";
        case ResultCode::NoResourceAvailable:  return "Error: no suitable resource is available (none registered or all unavailable).";
        case ResultCode::AlreadyAssigned:      return "Error: already assigned (the parcel has a resource, or the resource is occupied).";
        case ResultCode::WrongResourceType:    return "Error: wrong resource type for this parcel's route.";
        case ResultCode::ResourceUnavailable:  return "Error: that resource is unavailable.";
        case ResultCode::NotAssigned:          return "Error: that resource is not occupied, so there is nothing to release.";
        default:                               return "Error: the operation failed.";
    }
}
