#ifndef STATUS_HPP
#define STATUS_HPP


enum class ParcelStatus {
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
    Returned,
    Cancelled
};

bool canTransition(ParcelStatus from, ParcelStatus to);

bool isFinalStatus(ParcelStatus s);

#endif



