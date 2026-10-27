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

bool canTransition(ParcelStatus from, ParcelStatus to) {
    switch (from) {
        case ParcelStatus::Registered:
            return to == ParcelStatus::ReadyForCollection ||
                   to == ParcelStatus::OutForDelivery ||
                   to == ParcelStatus::SpecialHandling ||
                   to == ParcelStatus::ReturnToSender ||
                   to == ParcelStatus::OnHold ||
                   to == ParcelStatus::Missing;

        case ParcelStatus::SpecialHandling:
            return to == ParcelStatus::ReadyForCollection ||
                   to == ParcelStatus::OutForDelivery ||
                   to == ParcelStatus::ReturnToSender ||
                   to == ParcelStatus::OnHold;

        case ParcelStatus::OnHold:
            return to == ParcelStatus::Registered ||
                   to == ParcelStatus::ReturnToSender ||
                   to == ParcelStatus::Missing ||
                   to == ParcelStatus::Cancelled;

        case ParcelStatus::ReadyForCollection:
            return to == ParcelStatus::Collected ||
                   to == ParcelStatus::ReturnToSender ||
                   to == ParcelStatus::Missing;

        case ParcelStatus::OutForDelivery:
            return to == ParcelStatus::Delivered ||
                   to == ParcelStatus::ReturnToSender ||
                   to == ParcelStatus::Missing;

        case ParcelStatus::ReturnToSender:
            return to == ParcelStatus::Returned;

        case ParcelStatus::Missing:
            return to == ParcelStatus::OnHold ||
                   to == ParcelStatus::Cancelled;

        default:
            return false;    // Collected, Delivered, Returned, Cancelled: final
    }
}

bool isFinalStatus(ParcelStatus s) {
    return s == ParcelStatus::Collected ||
           s == ParcelStatus::Delivered ||
           s == ParcelStatus::Returned ||
           s == ParcelStatus::Cancelled;
}
