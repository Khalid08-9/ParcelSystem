#include "Interface.hpp"
#include <cstdlib>
#include <iostream>
#include <type_traits>

static void check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

int main() {
    static_assert(!std::is_copy_constructible<ParcelRegistry>::value,
                  "Registry ownership must not be copied");
    static_assert(!std::is_copy_constructible<ParcelQueue>::value,
                  "Queue ownership must not be copied");
    ParcelRegistry registry;
    treeNode* tree = nullptr;
    Parcel incoming;
    incoming.recipientName = "Integration Recipient";
    incoming.recipientID = "RC012345";
    incoming.deliveryAddress = "Campus Block A";
    incoming.type = ParcelType::Large;
    std::string id;
    check(registerParcel(registry, incoming, id) == ResultCode::Success,
          "shared interface registers a parcel");
    check(insertParcelRecord(tree, *findParcel(registry, id)) == ResultCode::Success,
          "tree receives registered record");
    check(routeNextParcel(registry, static_cast<RouteAction>(99)) == ResultCode::InvalidInput
              && registry.waitingCount() == 1, "invalid route leaves waiting queue unchanged");
    check(routeNextParcel(registry) == ResultCode::Success, "route through shared interface");
    const Parcel* next = registry.peekNextParcel(OutForDelivery);
    check(next && next->parcelID == id && registry.waitingCount() == 0,
          "service module can inspect next delivery parcel");

    // Simulate the controller linking IDs created by Tasks 2 and 3.
    Parcel* shared = findParcel(registry, id);
    shared->requestID = "REQ0001";
    shared->resourceID = "RES0001";
    check(registry.removeParcel(id) == ResultCode::InvalidStatus,
          "active links protect parcel from deletion");

    // The controller captures this information for Task 4, then reports success.
    const ParcelStatus previousStatus = shared->status;
    check(updateParcelStatus(registry, id, Delivered) == ResultCode::Success,
          "service completion updates canonical parcel");
    check(previousStatus == OutForDelivery && shared->status == Delivered
              && registry.peekNextParcel(OutForDelivery) == nullptr,
          "completion preserves previous status for history and removes queue entry");
    check(updateParcelStatus(registry, id, Delivered) == ResultCode::AlreadyCompleted,
          "duplicate service completion is rejected");
    check(updateParcelRecord(tree, id, *shared) == ResultCode::Success,
          "controller synchronizes tree snapshot after completion");
    const Parcel* snapshot = nullptr;
    check(findParcelRecord(tree, id, snapshot) == ResultCode::Success
              && snapshot->status == Delivered && snapshot->requestID == "REQ0001",
          "tree snapshot reflects canonical record");
    check(registry.peekNextParcel(Missing) == nullptr
              && registry.peekNextParcel(static_cast<ParcelStatus>(99)) == nullptr,
          "statuses without queues return null");
    // Real controller releases resource/request records before clearing their links.
    shared->resourceID.clear();
    shared->requestID.clear();
    check(removeParcelRecord(tree, id) == ResultCode::Success, "remove tree snapshot");
    check(registry.removeParcel(id) == ResultCode::Success, "remove unlinked canonical record");
    clearParcelRecords(tree);
    check(registry.isEmpty(), "handoff lifecycle leaves registry consistent");
    std::cout << "Task 1 integration contract tests passed.\n";
}
