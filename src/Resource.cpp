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
