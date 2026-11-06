#include "Status.hpp"

#include <iostream>

namespace {

struct Transition {
    ParcelStatus from;
    ParcelStatus to;
};

const ParcelStatus kStatuses[] = {
    ParcelStatus::Registered,
    ParcelStatus::ReadyForCollection,
    ParcelStatus::OutForDelivery,
    ParcelStatus::SpecialHandling,
    ParcelStatus::ReturnToSender,
    ParcelStatus::OnHold,
    ParcelStatus::Missing,
    ParcelStatus::Collected,
    ParcelStatus::Delivered,
    ParcelStatus::Returned,
    ParcelStatus::Cancelled
};

const Transition kAllowedTransitions[] = {
    {ParcelStatus::Registered, ParcelStatus::ReadyForCollection},
    {ParcelStatus::Registered, ParcelStatus::OutForDelivery},
    {ParcelStatus::Registered, ParcelStatus::SpecialHandling},
    {ParcelStatus::Registered, ParcelStatus::ReturnToSender},
    {ParcelStatus::Registered, ParcelStatus::OnHold},
    {ParcelStatus::Registered, ParcelStatus::Missing},
    {ParcelStatus::Registered, ParcelStatus::Cancelled},

    {ParcelStatus::SpecialHandling, ParcelStatus::ReadyForCollection},
    {ParcelStatus::SpecialHandling, ParcelStatus::OutForDelivery},
    {ParcelStatus::SpecialHandling, ParcelStatus::ReturnToSender},
    {ParcelStatus::SpecialHandling, ParcelStatus::OnHold},
    {ParcelStatus::SpecialHandling, ParcelStatus::Missing},
    {ParcelStatus::SpecialHandling, ParcelStatus::Cancelled},

    {ParcelStatus::OnHold, ParcelStatus::Registered},
    {ParcelStatus::OnHold, ParcelStatus::ReturnToSender},
    {ParcelStatus::OnHold, ParcelStatus::Missing},
    {ParcelStatus::OnHold, ParcelStatus::Cancelled},

    {ParcelStatus::ReadyForCollection, ParcelStatus::Collected},
    {ParcelStatus::ReadyForCollection, ParcelStatus::ReturnToSender},
    {ParcelStatus::ReadyForCollection, ParcelStatus::Missing},
    {ParcelStatus::ReadyForCollection, ParcelStatus::Cancelled},

    {ParcelStatus::OutForDelivery, ParcelStatus::Delivered},
    {ParcelStatus::OutForDelivery, ParcelStatus::ReturnToSender},
    {ParcelStatus::OutForDelivery, ParcelStatus::Missing},
    {ParcelStatus::OutForDelivery, ParcelStatus::Cancelled},

    {ParcelStatus::ReturnToSender, ParcelStatus::Returned},
    {ParcelStatus::ReturnToSender, ParcelStatus::Missing},
    {ParcelStatus::ReturnToSender, ParcelStatus::Cancelled},

    {ParcelStatus::Missing, ParcelStatus::OnHold},
    {ParcelStatus::Missing, ParcelStatus::Cancelled}
};

bool expectedTransition(ParcelStatus from, ParcelStatus to) {
    for (const Transition& transition : kAllowedTransitions) {
        if (transition.from == from && transition.to == to) {
            return true;
        }
    }
    return false;
}

bool checkTransition(ParcelStatus from, ParcelStatus to, bool expected) {
    const bool actual = canTransition(from, to);
    if (actual == expected) {
        return true;
    }

    std::cerr << "Unexpected transition result: "
              << statusToString(from) << " -> " << statusToString(to)
              << " (expected " << (expected ? "allowed" : "blocked")
              << ", got " << (actual ? "allowed" : "blocked") << ")\n";
    return false;
}

}  // namespace

int main() {
    for (ParcelStatus from : kStatuses) {
        for (ParcelStatus to : kStatuses) {
            if (!checkTransition(from, to, expectedTransition(from, to))) {
                return 1;
            }
        }
    }

    const ParcelStatus invalidStatuses[] = {
        static_cast<ParcelStatus>(-1),
        static_cast<ParcelStatus>(99)
    };
    for (ParcelStatus invalid : invalidStatuses) {
        for (ParcelStatus status : kStatuses) {
            if (!checkTransition(invalid, status, false) ||
                !checkTransition(status, invalid, false)) {
                return 1;
            }
        }
        if (!checkTransition(invalid, invalid, false)) {
            return 1;
        }
    }

    std::cout << "All status transition validations passed.\n";
    return 0;
}
