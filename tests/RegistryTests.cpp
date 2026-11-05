#include "ParcelRegistry.hpp"
#include <cstdlib>
#include <iostream>

static void check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

static Parcel sample() {
    Parcel p;
    p.recipientName = "Test Student";
    p.recipientID = "RC012345";
    p.deliveryAddress = "Campus Block A";
    p.type = ParcelType::Small;
    return p;
}

int main() {
    ParcelRegistry registry;
    check(registry.routeNextParcel(RouteAction::Default) == ResultCode::Empty,
          "processing an empty registry is handled");
    check(registry.viewParcel("UNKNOWN") == ResultCode::NotFound,
          "viewing a non-existent parcel is handled");
    check(registry.removeParcel("UNKNOWN") == ResultCode::NotFound,
          "removing a non-existent parcel is handled");
    check(registry.updateParcelStatus("UNKNOWN", Missing) == ResultCode::NotFound,
          "updating a non-existent parcel is handled");
    check(registry.viewParcel(" \t") == ResultCode::InvalidInput,
          "blank parcel ID is rejected");
    const char* invalidIDs[] = {"", " ", "RC", "RC12345", "RC1234567",
        "TP012345", "rc012345", "RC12345A", "RC12 345", " RC012345", "RC012345 "};
    for (const char* invalidID : invalidIDs) {
        Parcel p = sample();
        p.recipientID = invalidID;
        p.parcelID = "MANUAL1";
        std::string assignedID = "unchanged";
        check(registry.registerParcel(p, assignedID) == ResultCode::InvalidInput,
              "auto-ID registration rejects malformed recipient ID");
        check(assignedID.empty(), "failed registration clears output ID");
        check(registry.registerParcel(p) == ResultCode::InvalidInput,
              "explicit-ID registration rejects malformed recipient ID");
        check(registry.getSize() == 0 && registry.waitingCount() == 0,
              "failed registration leaves registry and queues unchanged");
    }
    Parcel unsetType;
    unsetType.recipientName = "Test Student";
    unsetType.recipientID = "RC012345";
    unsetType.deliveryAddress = "Campus Block A";
    std::string id;
    check(registry.registerParcel(unsetType, id) == ResultCode::InvalidInput,
          "missing parcel type is rejected");
    Parcel invalidType = sample();
    invalidType.type = static_cast<ParcelType>(99);
    invalidType.parcelID = "MANUAL1";
    check(registry.registerParcel(invalidType, id) == ResultCode::InvalidInput,
          "auto-ID registration rejects invalid parcel type");
    check(registry.registerParcel(invalidType) == ResultCode::InvalidInput,
          "explicit-ID registration rejects invalid parcel type");

    check(registry.registerParcel(sample(), id) == ResultCode::Success, "valid registration");
    check(id == "PCL0001", "invalid registration does not consume generated IDs");
    check(registry.registerParcel(*registry.findParcel(id)) == ResultCode::Duplicate,
          "duplicate Parcel ID is rejected");
    check(registry.getSize() == 1 && registry.waitingCount() == 1,
          "duplicate registration does not change queues");
    check(registry.updateParcelStatus(id, Delivered) == ResultCode::InvalidStatus,
          "invalid completion from registered is rejected");
    for (const char* invalidID : invalidIDs) {
        if (std::string(invalidID).empty()) continue; // Empty means keep existing ID.
        check(registry.updateParcelDetails(id, "Changed Name", invalidID, "Changed Address")
                  == ResultCode::InvalidInput, "edit rejects malformed recipient ID");
        const Parcel* p = registry.findParcel(id);
        check(p->recipientName == "Test Student" && p->recipientID == "RC012345"
                  && p->deliveryAddress == "Campus Block A", "failed edit changes no fields");
    }
    check(registry.updateParcelDetails(id, "Changed Name", "", "") == ResultCode::Success,
          "empty ID preserves current ID during other edits");
    check(registry.updateParcelDetails(id, "", "RC000000", "") == ResultCode::Success,
          "valid replacement ID including leading zeros");

    const ParcelStatus active[] = {Registered, ReadyForCollection, OutForDelivery,
        SpecialHandling, ReturnToSender, OnHold, Missing};
    for (ParcelStatus status : active) {
        ParcelRegistry r;
        check(r.registerParcel(sample(), id) == ResultCode::Success, "register cancellation sample");
        if (status != Registered) {
            check(r.updateParcelStatus(id, status) == ResultCode::Success, "prepare active status");
        }
        check(r.updateParcelStatus(id, Cancelled) == ResultCode::Success, "cancel any active status");
        check(r.waitingCount() + r.collectionCount() + r.deliveryCount() + r.specialCount()
                  + r.returnCount() + r.holdCount() == 0, "cancellation removes queued parcel");
        check(r.updateParcelStatus(id, Missing) == ResultCode::AlreadyCompleted,
              "cancelled parcel remains locked");
    }
    const ParcelStatus missingFrom[] = {SpecialHandling, ReturnToSender};
    for (ParcelStatus status : missingFrom) {
        ParcelRegistry r;
        check(r.registerParcel(sample(), id) == ResultCode::Success, "register missing sample");
        check(r.updateParcelStatus(id, status) == ResultCode::Success, "prepare missing transition");
        check(r.updateParcelStatus(id, Missing) == ResultCode::Success, "report active parcel missing");
        check(r.specialCount() == 0 && r.returnCount() == 0, "missing parcel removed from old queue");
    }
    const ParcelType types[] = {Document, Small, Large, Fragile};
    const ParcelStatus destinations[] = {ReadyForCollection, ReadyForCollection, OutForDelivery, SpecialHandling};
    for (int i = 0; i < 4; ++i) {
        ParcelRegistry r;
        Parcel p = sample();
        p.type = types[i];
        check(r.registerParcel(p, id) == ResultCode::Success, "accept valid parcel type");
        check(r.routeNextParcel(RouteAction::Default) == ResultCode::Success, "route valid parcel type");
        check(r.findParcel(id)->status == destinations[i], "correct destination status");
    }
    std::cout << "All registry validation and queue consistency tests passed.\n";
}
