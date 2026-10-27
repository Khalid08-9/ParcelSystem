#ifndef STATUS_HPP
#define STATUS_HPP
#include <string>

enum  ParcelStatus {
    Registered,
    ReadyForCollection,
    OutForDelivery,
    SpecialHandling,
    ReturnToSender,
    OnHold,
    Missing,
    Collected,
    Delivered,
    Returned,
    Cancelled
};

bool canTransition(ParcelStatus from, ParcelStatus to);

bool isFinalStatus(ParcelStatus s);

std::string statusToString(ParcelStatus s);

#endif



