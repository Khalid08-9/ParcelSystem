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
      // Checks the parcel can take a resource and works out which type it needs
      ResultCode checkParcel(const std::string& parcelID, const std::string& requestID,
                             Parcel*& parcel, ResourceType& type);
      void occupy(Resource& r, Parcel& p, const std::string& requestID);
      Parcel* clearAssignment(Resource& r); // undoes occupy(), returns the parcel
      ResultCode whyNoneFree(ResourceType t) const; // called when a queue is empty

    public:
      explicit ResourceManager(ParcelRegistry& registry);
      // Gives the resource a new ID (CTR01, DLV01, ...) and makes it available
      ResultCode registerResource(const std::string& name, ResourceType type,
                                  std::string& assignedID);
      // Round-robin: takes the first resource in the matching available queue
      ResultCode assignNextAvailable(const std::string& parcelID, const std::string& requestID,
                                     std::string& assignedID);
      // Staff picks the resource; it must match the parcel's route and be free
      ResultCode assignToResource(const std::string& resourceID, const std::string& parcelID,
                                  const std::string& requestID);
      // Frees an occupied resource and puts it at the back of its queue
      ResultCode releaseResource(const std::string& resourceID);
      // Available <-> Unavailable. A busy resource that becomes unavailable hands
      // its parcel to the next free one (handedToID, empty if none was free).
      ResultCode setResourceStatus(const std::string& resourceID, ResourceStatus newStatus,
                                   std::string& handedToID);
      Resource* findResource(const std::string& resourceID); // nullptr if not found
      int getSize() const;
      int countByStatus(ResourceStatus s) const;
      void displayResources() const;
};

#endif
