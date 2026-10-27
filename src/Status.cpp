#include "Status.hpp"
#include <Parcel.hpp>
#include <string>
std::string statusToString(ParcelStatus s) {
    switch (s) {
        case ParcelStatus::Registered:
            return "Registered";
        case ParcelStatus::ReadyForCollection:
            return "Ready for Collection";
        case ParcelStatus::OutForDelivery:
            return "Out for Delivery";
        case ParcelStatus::SpecialHandling:
            return "Special Handling";
        case ParcelStatus::ReturnToSender:
            return "Return to Sender";
        case ParcelStatus::OnHold:
            return "On Hold";
        case ParcelStatus::Missing:
            return "Missing";
        case ParcelStatus::Collected:
            return "Collected";
        case ParcelStatus::Delivered:
            return "Delivered";
        case ParcelStatus::Returned:
            return "Returned";
        case ParcelStatus::Cancelled:
            return "Cancelled";
        default:
            return "Unknown";
    }
}

std::string typeToString(ParcelType t) {
    switch (t) {
        case ParcelType::Document:
            return "Document";
        case ParcelType::Small:
            return "Small";
        case ParcelType::Large:
            return "Large";
        case ParcelType::Fragile:
            return "Fragile";
        default:
            return "Unknown";
    }
}
