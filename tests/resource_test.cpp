// Tests for Task 3: ResourceQueue (circular queue) and ResourceManager.
#include <iostream>
#include <string>

#include "ResourceManager.hpp"

namespace {

int failures = 0;

void check(bool condition, const std::string& name) {
    std::cout << (condition ? "  PASS  " : "  FAIL  ") << name << "\n";
    if (!condition) failures++;
}

// Registers a parcel and routes it, so it reaches the given status
std::string addRoutedParcel(ParcelRegistry& registry, ParcelType type,
                            RouteAction action = RouteAction::Default) {
    Parcel p{};
    p.recipientName = "Test Recipient";
    p.recipientID = "TP000001";
    p.deliveryAddress = "Block A";
    p.type = type;

    std::string id;
    registry.registerParcel(p, id);
    registry.routeNextParcel(action);
    return id;
}

void testCircularQueue() {
    std::cout << "Circular queue\n";
    ResourceQueue q;
    Resource items[RESOURCE_CAPACITY];

    check(q.isEmpty() && q.dequeue() == nullptr, "dequeue on empty returns nullptr");

    for (int i = 0; i < RESOURCE_CAPACITY; i++) q.enqueue(&items[i]);
    check(q.isFull(), "full after CAPACITY enqueues");
    check(!q.enqueue(&items[0]), "enqueue on full is rejected");

    // Remove 5, add 5: rear must wrap around past the end of the array
    for (int i = 0; i < 5; i++) q.dequeue();
    for (int i = 0; i < 5; i++) q.enqueue(&items[i]);
    check(q.peek() == &items[5], "wrap-around keeps FIFO order");

    check(q.remove(&items[10]) && q.getSize() == RESOURCE_CAPACITY - 1,
          "remove from the middle");
    check(!q.remove(&items[10]), "remove of missing item returns false");
    check(q.peek() == &items[5], "remove keeps the order of the rest");
}

void testRegisterAndDisplay() {
    std::cout << "Register\n";
    ParcelRegistry registry;
    ResourceManager manager(registry);
    std::string id;

    check(manager.registerResource("  ", ResourceType::CollectionCounter, id)
              == ResultCode::InvalidInput, "blank name rejected");
    check(manager.registerResource("Counter A", ResourceType::CollectionCounter, id)
              == ResultCode::Success && id == "CTR01", "counter gets CTR01");
    check(manager.registerResource("Ahmad", ResourceType::DeliveryPersonnel, id)
              == ResultCode::Success && id == "DLV01", "courier gets DLV01");
    check(manager.findResource("XXX") == nullptr, "unknown ID not found");

    for (int i = manager.getSize(); i < RESOURCE_CAPACITY; i++) {
        manager.registerResource("Extra", ResourceType::DeliveryPersonnel, id);
    }
    check(manager.registerResource("One too many", ResourceType::DeliveryPersonnel, id)
              == ResultCode::Full, "register when full rejected");
}

void testAssignAndRelease() {
    std::cout << "Assign and release\n";
    ParcelRegistry registry;
    ResourceManager manager(registry);
    std::string ctr1, ctr2, dlv1, assigned;
    manager.registerResource("Counter A", ResourceType::CollectionCounter, ctr1);
    manager.registerResource("Counter B", ResourceType::CollectionCounter, ctr2);
    manager.registerResource("Ahmad", ResourceType::DeliveryPersonnel, dlv1);

    std::string small1 = addRoutedParcel(registry, ParcelType::Small);   // collection
    std::string small2 = addRoutedParcel(registry, ParcelType::Small);
    std::string small3 = addRoutedParcel(registry, ParcelType::Small);
    std::string large  = addRoutedParcel(registry, ParcelType::Large);   // delivery

    check(manager.assignNextAvailable("PCL9999", "", assigned) == ResultCode::NotFound,
          "unknown parcel rejected");
    check(manager.assignNextAvailable(small1, "", assigned) == ResultCode::Success
              && assigned == ctr1, "collection parcel goes to first counter");
    check(registry.findParcel(small1)->resourceID == ctr1, "parcel records its resource");
    check(manager.assignNextAvailable(small1, "", assigned) == ResultCode::AlreadyAssigned,
          "duplicate active assignment rejected");
    check(manager.assignNextAvailable(small2, "", assigned) == ResultCode::Success
              && assigned == ctr2, "round-robin picks the next counter");
    check(manager.assignNextAvailable(small3, "", assigned) == ResultCode::NoResourceAvailable,
          "all counters occupied");

    check(manager.assignToResource(dlv1, small3, "") == ResultCode::WrongResourceType,
          "collection parcel cannot go to a courier");
    check(manager.assignToResource(ctr1, small3, "") == ResultCode::AlreadyAssigned,
          "selecting an occupied resource rejected");
    check(manager.assignToResource(dlv1, large, "") == ResultCode::Success,
          "delivery parcel to the chosen courier");

    check(manager.releaseResource(ctr1) == ResultCode::Success, "release works");
    check(registry.findParcel(small1)->resourceID.empty(), "release clears the parcel link");
    check(manager.releaseResource(ctr1) == ResultCode::NotAssigned,
          "releasing a free resource rejected");
    check(manager.assignNextAvailable(small3, "", assigned) == ResultCode::Success
              && assigned == ctr1, "released counter is reused");
}

void testParcelStatusRules() {
    std::cout << "Routing rules\n";
    ParcelRegistry registry;
    ResourceManager manager(registry);
    std::string id, assigned;
    manager.registerResource("Counter A", ResourceType::CollectionCounter, id);
    manager.registerResource("Ahmad", ResourceType::DeliveryPersonnel, id);

    Parcel p{};
    p.recipientName = "Test";
    p.recipientID = "TP1";
    p.deliveryAddress = "Block A";
    p.type = ParcelType::Small;
    std::string notRouted;
    registry.registerParcel(p, notRouted);
    check(manager.assignNextAvailable(notRouted, "", assigned) == ResultCode::InvalidStatus,
          "parcel not routed yet rejected");
    registry.routeNextParcel(RouteAction::Default);

    std::string onHold = addRoutedParcel(registry, ParcelType::Small, RouteAction::OnHold);
    check(manager.assignNextAvailable(onHold, "", assigned) == ResultCode::InvalidStatus,
          "parcel on hold rejected");

    std::string fragile = addRoutedParcel(registry, ParcelType::Fragile);
    check(manager.assignNextAvailable(fragile, "", assigned) == ResultCode::Success
              && assigned == "DLV01", "special handling goes to delivery personnel");

    registry.updateParcelStatus(notRouted, ParcelStatus::Collected);
    check(manager.assignNextAvailable(notRouted, "", assigned) == ResultCode::AlreadyCompleted,
          "completed parcel rejected");

    std::string withRequest = addRoutedParcel(registry, ParcelType::Small);
    registry.findParcel(withRequest)->requestID = "REQ001";
    check(manager.assignNextAvailable(withRequest, "REQ999", assigned) == ResultCode::InvalidInput,
          "request belonging to another parcel rejected");
}

void testStatusChanges() {
    std::cout << "Status changes\n";
    ParcelRegistry registry;
    ResourceManager manager(registry);
    std::string dlv1, dlv2, handedTo, assigned;
    manager.registerResource("Ahmad", ResourceType::DeliveryPersonnel, dlv1);
    manager.registerResource("Sara", ResourceType::DeliveryPersonnel, dlv2);
    std::string large1 = addRoutedParcel(registry, ParcelType::Large);
    std::string large2 = addRoutedParcel(registry, ParcelType::Large);

    check(manager.setResourceStatus("XXX", ResourceStatus::Unavailable, handedTo)
              == ResultCode::NotFound, "invalid resource ID");
    check(manager.setResourceStatus(dlv1, ResourceStatus::Occupied, handedTo)
              == ResultCode::InvalidStatus, "cannot mark occupied directly");
    check(manager.setResourceStatus(dlv1, ResourceStatus::Available, handedTo)
              == ResultCode::InvalidStatus, "same status rejected");

    check(manager.setResourceStatus(dlv2, ResourceStatus::Unavailable, handedTo)
              == ResultCode::Success, "available -> unavailable");
    check(manager.assignToResource(dlv2, large1, "") == ResultCode::ResourceUnavailable,
          "selecting an unavailable resource rejected");
    check(manager.setResourceStatus(dlv2, ResourceStatus::Available, handedTo)
              == ResultCode::Success, "unavailable -> available");

    // dlv1 drops out while busy: its parcel moves to dlv2
    manager.assignToResource(dlv1, large1, "");
    check(manager.setResourceStatus(dlv1, ResourceStatus::Unavailable, handedTo)
              == ResultCode::Success && handedTo == dlv2, "busy resource hands over its parcel");
    check(registry.findParcel(large1)->resourceID == dlv2, "parcel follows the handover");
    check(manager.findResource(dlv1)->assignedParcelID.empty(), "old resource is cleared");

    // dlv2 drops out with nobody free: parcel is left waiting
    check(manager.setResourceStatus(dlv2, ResourceStatus::Unavailable, handedTo)
              == ResultCode::Success && handedTo.empty(), "no replacement available");
    check(registry.findParcel(large1)->resourceID.empty(), "parcel waits for a new resource");
    check(manager.assignNextAvailable(large2, "", assigned) == ResultCode::NoResourceAvailable,
          "no suitable resource available");
    check(manager.countByStatus(ResourceStatus::Unavailable) == 2, "status counts are correct");
}

}  // namespace

int main() {
    testCircularQueue();
    testRegisterAndDisplay();
    testAssignAndRelease();
    testParcelStatusRules();
    testStatusChanges();

    std::cout << (failures == 0 ? "\nAll tests passed.\n" : "\nSome tests FAILED.\n");
    return failures == 0 ? 0 : 1;
}
