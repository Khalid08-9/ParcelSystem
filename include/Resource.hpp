#ifndef RESOURCE_HPP
#define RESOURCE_HPP
#include <string>
#include "Status.hpp"
#include "Result.hpp"

// Task 3: Collection Counter and Delivery Resource Management

enum class ResourceType
{
    CollectionCounter, // a counter where recipients collect parcels
    DeliveryPersonnel  // a staff member who carries parcels around campus
};

enum class ResourceStatus
{
    Available,  // free, waiting in the available queue
    Occupied,   // working on one parcel / request
    Unavailable // off duty or closed, cannot be assigned
};

struct Resource
{
    std::string resourceID;
    std::string name;
    ResourceType type = ResourceType::CollectionCounter;
    ResourceStatus status = ResourceStatus::Available;
    std::string assignedParcelID;  // empty when not occupied
    std::string assignedRequestID; // empty when no service request is linked
};

std::string resourceTypeToString(ResourceType t);
std::string resourceStatusToString(ResourceStatus s);

// Routing rule (see ROUTING_RULES.md): which resource a parcel needs,
// based on the queue Task 1 routed it to.
//   ReadyForCollection                            -> CollectionCounter
//   OutForDelivery, SpecialHandling, ReturnToSender -> DeliveryPersonnel
// Returns false when a parcel in this status must not get a resource
// (not routed yet, on hold, missing or already completed).
bool requiredResourceType(ParcelStatus status, ResourceType &required);

// Message for the user after a resource operation (requirement 3.5)
std::string resourceResultMessage(ResultCode code);

#endif
