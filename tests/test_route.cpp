

#include <iostream>
#include <string>
#include "ParcelRegistry.hpp"

// Prints PASS or FAIL for each check
void check(const char* name, bool condition) {
    std::cout << (condition ? "PASS: " : "FAIL: ") << name << "\n";
}

// Builds a valid parcel so each test only changes the ID and the type
Parcel makeParcel(const std::string& id, ParcelType type) {
    Parcel p;
    p.parcelID = id;
    p.recipientName = "Ali Hassan";
    p.recipientID = "TP012345";
    p.deliveryAddress = "Block A, Room 101";
    p.type = type;
    return p;
}

int main() {
    // ---------- 1. Routing on an empty queue ----------
    {
        ParcelRegistry reg;
        check("route on empty queue returns Empty",
              reg.routeNextParcel(RouteAction::Default) == ResultCode::Empty);
    }

    // ---------- 2. Default routing by parcel type ----------
    {
        ParcelRegistry reg;
        reg.registerParcel(makeParcel("D1", ParcelType::Document));
        reg.registerParcel(makeParcel("L1", ParcelType::Large));
        reg.registerParcel(makeParcel("F1", ParcelType::Fragile));
        reg.registerParcel(makeParcel("S1", ParcelType::Small));
        check("4 parcels are waiting", reg.waitingCount() == 4);

        check("route 1 returns Success",
              reg.routeNextParcel(RouteAction::Default) == ResultCode::Success);
        check("Document -> ReadyForCollection",
              reg.findParcel("D1")->status == ParcelStatus::ReadyForCollection);
        check("collection queue has 1", reg.collectionCount() == 1);
        check("waiting dropped to 3", reg.waitingCount() == 3);

        reg.routeNextParcel(RouteAction::Default);
        check("Large -> OutForDelivery",
              reg.findParcel("L1")->status == ParcelStatus::OutForDelivery);
        check("delivery queue has 1", reg.deliveryCount() == 1);

        reg.routeNextParcel(RouteAction::Default);
        check("Fragile -> SpecialHandling",
              reg.findParcel("F1")->status == ParcelStatus::SpecialHandling);
        check("special queue has 1", reg.specialCount() == 1);

        reg.routeNextParcel(RouteAction::Default);
        check("Small -> ReadyForCollection",
              reg.findParcel("S1")->status == ParcelStatus::ReadyForCollection);
        check("collection queue has 2", reg.collectionCount() == 2);
        check("waiting is empty", reg.waitingCount() == 0);

        check("route after all processed returns Empty",
              reg.routeNextParcel(RouteAction::Default) == ResultCode::Empty);
    }

    // ---------- 3. First in, first out ----------
    {
        ParcelRegistry reg;
        reg.registerParcel(makeParcel("A", ParcelType::Document));
        reg.registerParcel(makeParcel("B", ParcelType::Document));
        check("front of the queue is A", reg.peekWaiting()->parcelID == "A");
        reg.routeNextParcel(RouteAction::Default);
        check("after routing A, front is B", reg.peekWaiting()->parcelID == "B");
        check("A was routed (not B)",
              reg.findParcel("A")->status == ParcelStatus::ReadyForCollection &&
              reg.findParcel("B")->status == ParcelStatus::Registered);
    }

    // ---------- 4. Staff overrides ----------
    {
        ParcelRegistry reg;
        reg.registerParcel(makeParcel("H1", ParcelType::Small));
        reg.registerParcel(makeParcel("R1", ParcelType::Small));
        reg.registerParcel(makeParcel("M1", ParcelType::Small));

        check("override OnHold returns Success",
              reg.routeNextParcel(RouteAction::OnHold) == ResultCode::Success);
        check("status is OnHold",
              reg.findParcel("H1")->status == ParcelStatus::OnHold);
        check("hold queue has 1", reg.holdCount() == 1);

        reg.routeNextParcel(RouteAction::ReturnToSender);
        check("status is ReturnToSender",
              reg.findParcel("R1")->status == ParcelStatus::ReturnToSender);
        check("return queue has 1", reg.returnCount() == 1);

        reg.routeNextParcel(RouteAction::Missing);
        check("status is Missing",
              reg.findParcel("M1")->status == ParcelStatus::Missing);
        check("Missing does not use the special queue", reg.specialCount() == 0);
        check("waiting is empty after 3 overrides", reg.waitingCount() == 0);
        check("registry still owns all 3 parcels", reg.getSize() == 3);
    }

    // ---------- 5. Status rules (canTransition / isFinalStatus) ----------
    check("Registered -> OutForDelivery",
          canTransition(ParcelStatus::Registered, ParcelStatus::OutForDelivery));
    check("Registered -> Collected is NOT allowed",
          !canTransition(ParcelStatus::Registered, ParcelStatus::Collected));
    check("SpecialHandling -> ReadyForCollection",
          canTransition(ParcelStatus::SpecialHandling, ParcelStatus::ReadyForCollection));
    check("OnHold -> Registered",
          canTransition(ParcelStatus::OnHold, ParcelStatus::Registered));
    check("ReadyForCollection -> Collected",
          canTransition(ParcelStatus::ReadyForCollection, ParcelStatus::Collected));
    check("OutForDelivery -> Delivered",
          canTransition(ParcelStatus::OutForDelivery, ParcelStatus::Delivered));
    check("ReturnToSender -> Returned",
          canTransition(ParcelStatus::ReturnToSender, ParcelStatus::Returned));
    check("Missing -> Cancelled",
          canTransition(ParcelStatus::Missing, ParcelStatus::Cancelled));
    check("Collected -> OnHold is NOT allowed (final)",
          !canTransition(ParcelStatus::Collected, ParcelStatus::OnHold));
    check("Cancelled -> Registered is NOT allowed (final)",
          !canTransition(ParcelStatus::Cancelled, ParcelStatus::Registered));
    check("Collected is final", isFinalStatus(ParcelStatus::Collected));
    check("Delivered is final", isFinalStatus(ParcelStatus::Delivered));
    check("Returned is final", isFinalStatus(ParcelStatus::Returned));
    check("Cancelled is final", isFinalStatus(ParcelStatus::Cancelled));
    check("OnHold is not final", !isFinalStatus(ParcelStatus::OnHold));

    return 0;
}