#include "ResourceManager.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>

static bool isBlank(const std::string& text) {
    return text.find_first_not_of(" \t\r\n") == std::string::npos;
}

ResourceManager::ResourceManager(ParcelRegistry& registry)
    : registry(registry), size(0), nextCounterNumber(1), nextDeliveryNumber(1) {}

ResourceQueue& ResourceManager::queueFor(ResourceType t) {
    return t == ResourceType::CollectionCounter ? availableCounters : availableDelivery;
}

std::string ResourceManager::generateResourceID(ResourceType t) {
    std::ostringstream id;
    if (t == ResourceType::CollectionCounter) {
        id << "CTR" << std::setw(2) << std::setfill('0') << nextCounterNumber++;
    } else {
        id << "DLV" << std::setw(2) << std::setfill('0') << nextDeliveryNumber++;
    }
    return id.str();      // CTR01, CTR02, ... / DLV01, DLV02, ...
}

ResultCode ResourceManager::registerResource(const std::string& name, ResourceType type,
                                             std::string& assignedID) {
    assignedID = "";

    if (isBlank(name)) {
        return ResultCode::InvalidInput;
    }
    if (size == RESOURCE_CAPACITY) {
        return ResultCode::Full;
    }

    // IDs come from a counter per type, so a new ID can never repeat an old one
    Resource& r = resources[size];
    r = Resource{};
    r.resourceID = generateResourceID(type);
    r.name = name;
    r.type = type;
    r.status = ResourceStatus::Available;

    queueFor(type).enqueue(&r);
    size++;

    assignedID = r.resourceID;
    return ResultCode::Success;
}

ResultCode ResourceManager::checkParcel(const std::string& parcelID, const std::string& requestID,
                                        Parcel*& parcel, ResourceType& type) {
    parcel = registry.findParcel(parcelID);
    if (parcel == nullptr) {
        return ResultCode::NotFound;
    }
    if (isFinalStatus(parcel->status)) {
        return ResultCode::AlreadyCompleted;
    }
    if (!requiredResourceType(parcel->status, type)) {
        return ResultCode::InvalidStatus;   // not routed yet, on hold or missing
    }
    if (!parcel->resourceID.empty()) {
        return ResultCode::AlreadyAssigned;
    }
    // A request can only be linked to the parcel it was made for
    if (!requestID.empty() && !parcel->requestID.empty() && parcel->requestID != requestID) {
        return ResultCode::InvalidInput;
    }
    return ResultCode::Success;
}

void ResourceManager::occupy(Resource& r, Parcel& p, const std::string& requestID) {
    r.status = ResourceStatus::Occupied;
    r.assignedParcelID = p.parcelID;
    r.assignedRequestID = requestID;
    p.resourceID = r.resourceID;
}

Parcel* ResourceManager::clearAssignment(Resource& r) {
    Parcel* p = registry.findParcel(r.assignedParcelID);
    if (p != nullptr) {
        p->resourceID = "";
    }
    r.assignedParcelID = "";
    r.assignedRequestID = "";
    return p;
}

// Busy resources of this type will come back after a release, so say "occupied".
// Otherwise none is registered or all are unavailable: "no suitable resource".
ResultCode ResourceManager::whyNoneFree(ResourceType t) const {
    for (int i = 0; i < size; i++) {
        if (resources[i].type == t && resources[i].status == ResourceStatus::Occupied) {
            return ResultCode::AllResourcesOccupied;
        }
    }
    return ResultCode::NoResourceAvailable;
}

ResultCode ResourceManager::assignNextAvailable(const std::string& parcelID,
                                                const std::string& requestID,
                                                std::string& assignedID) {
    assignedID = "";
    Parcel* parcel;
    ResourceType type;
    ResultCode result = checkParcel(parcelID, requestID, parcel, type);
    if (result != ResultCode::Success) {
        return result;
    }

    Resource* r = queueFor(type).dequeue();
    if (r == nullptr) {
        return whyNoneFree(type);
    }
    occupy(*r, *parcel, requestID);
    assignedID = r->resourceID;
    return ResultCode::Success;
}

ResultCode ResourceManager::assignToResource(const std::string& resourceID,
                                             const std::string& parcelID,
                                             const std::string& requestID) {
    Resource* r = findResource(resourceID);
    if (r == nullptr) {
        return ResultCode::NotFound;
    }
    Parcel* parcel;
    ResourceType type;
    ResultCode result = checkParcel(parcelID, requestID, parcel, type);
    if (result != ResultCode::Success) {
        return result;
    }

    if (r->type != type) {
        return ResultCode::WrongResourceType;
    }
    if (r->status == ResourceStatus::Occupied) {
        return ResultCode::AlreadyAssigned;
    }
    if (r->status == ResourceStatus::Unavailable) {
        return ResultCode::ResourceUnavailable;
    }

    queueFor(type).remove(r);   // it is no longer free
    occupy(*r, *parcel, requestID);
    return ResultCode::Success;
}

ResultCode ResourceManager::releaseResource(const std::string& resourceID) {
    Resource* r = findResource(resourceID);
    if (r == nullptr) {
        return ResultCode::NotFound;
    }
    if (r->status != ResourceStatus::Occupied) {
        return ResultCode::NotAssigned;
    }
    clearAssignment(*r);
    r->status = ResourceStatus::Available;
    queueFor(r->type).enqueue(r);   // back of the queue, so the next one gets a turn first
    return ResultCode::Success;
}

ResultCode ResourceManager::setResourceStatus(const std::string& resourceID,
                                              ResourceStatus newStatus,
                                              std::string& handedToID) {
    handedToID = "";
    Resource* r = findResource(resourceID);
    if (r == nullptr) {
        return ResultCode::NotFound;
    }
    // Occupied is only reached by assigning a parcel
    if (newStatus == r->status || newStatus == ResourceStatus::Occupied) {
        return ResultCode::InvalidStatus;
    }

    if (r->status == ResourceStatus::Occupied) {
        if (newStatus == ResourceStatus::Available) {
            return releaseResource(resourceID);
        }
        // Became unavailable during processing: pass the parcel to the next free
        // resource. If none is free the parcel is left unassigned, so it can be
        // assigned again later and is never stuck on a resource that is off duty.
        std::string requestID = r->assignedRequestID;
        Parcel* parcel = clearAssignment(*r);
        Resource* next = parcel ? queueFor(r->type).dequeue() : nullptr;
        if (next != nullptr) {
            occupy(*next, *parcel, requestID);
            handedToID = next->resourceID;
        }
    } else if (newStatus == ResourceStatus::Unavailable) {
        queueFor(r->type).remove(r);    // Available -> Unavailable
    } else {
        queueFor(r->type).enqueue(r);   // Unavailable -> Available
    }

    r->status = newStatus;
    return ResultCode::Success;
}

Resource* ResourceManager::findResource(const std::string& resourceID) {
    for (int i = 0; i < size; i++) {
        if (resources[i].resourceID == resourceID) {
            return &resources[i];
        }
    }
    return nullptr;
}

int ResourceManager::getSize() const {
    return size;
}

int ResourceManager::countByStatus(ResourceStatus s) const {
    int count = 0;
    for (int i = 0; i < size; i++) {
        if (resources[i].status == s) {
            count++;
        }
    }
    return count;
}

void ResourceManager::displayResources() const {
    if (size == 0) {
        std::cout << "No resources registered.\n";
        return;
    }

    std::cout << std::left
              << std::setw(8)  << "ID"
              << std::setw(18) << "Name"
              << std::setw(21) << "Type"
              << std::setw(13) << "Status"
              << std::setw(10) << "Parcel"
              << "Request\n";
    std::cout << std::string(80, '-') << "\n";

    for (int i = 0; i < size; i++) {
        const Resource& r = resources[i];
        std::cout << std::left
                  << std::setw(8)  << r.resourceID
                  << std::setw(18) << r.name
                  << std::setw(21) << resourceTypeToString(r.type)
                  << std::setw(13) << resourceStatusToString(r.status)
                  << std::setw(10) << (r.assignedParcelID.empty() ? "-" : r.assignedParcelID)
                  << (r.assignedRequestID.empty() ? "-" : r.assignedRequestID) << "\n";
    }

    std::cout << std::string(80, '-') << "\n";
    std::cout << "Available: "   << countByStatus(ResourceStatus::Available)
              << "  Occupied: "  << countByStatus(ResourceStatus::Occupied)
              << "  Unavailable: " << countByStatus(ResourceStatus::Unavailable)
              << "  Total: "     << size << "\n";
}
