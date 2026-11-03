#ifndef RESOURCE_MANAGER_HPP
#define RESOURCE_MANAGER_HPP
#include <string>
#include "Resource.hpp"
#include "ResourceQueue.hpp"
#include "ParcelRegistry.hpp"
#include "Result.hpp"

// Task 3: stores every resource in a fixed array and keeps the available
// ones in two circular queues, one per resource type.

class ResourceManager
{
    private:
      ParcelRegistry& registry;               // Task 1 parcels, used when assigning
      Resource resources[RESOURCE_CAPACITY];  // every registered resource
      int size;
      ResourceQueue availableCounters;
      ResourceQueue availableDelivery;
      int nextCounterNumber;
      int nextDeliveryNumber;
      ResourceQueue& queueFor(ResourceType t);
      std::string generateResourceID(ResourceType t);

    public:
      explicit ResourceManager(ParcelRegistry& registry);
      // Gives the resource a new ID (CTR01, DLV01, ...) and makes it available
      ResultCode registerResource(const std::string& name, ResourceType type,
                                  std::string& assignedID);
      Resource* findResource(const std::string& resourceID); // nullptr if not found
      int getSize() const;
};

#endif
