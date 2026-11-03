#include "ResourceManager.hpp"
#include <iomanip>
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
