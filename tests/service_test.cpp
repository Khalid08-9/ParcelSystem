// Tests for Task 2: RequestPriorityList (sorted linked list) and ServiceManager.
// Owner: [Name, TP number]
#include <iostream>
#include <sstream>
#include <string>

#include "ServiceManager.hpp"

namespace {

int failures = 0;

void check(bool condition, const std::string& name) {
    std::cout << (condition ? "  PASS  " : "  FAIL  ") << name << "\n";
    if (!condition) failures++;
}

// Hides what a display function prints, so the test output stays readable
template <typename Function>
std::string captureOutput(Function display) {
    std::ostringstream sink;
    std::streambuf* original = std::cout.rdbuf(sink.rdbuf());
    display();
    std::cout.rdbuf(original);
    return sink.str();
}

const std::string RECIPIENT = "TP000001";

// Registers a parcel and routes it with the default rule (by type)
std::string addRoutedParcel(ParcelRegistry& registry, ParcelType type) {
    Parcel p{};
    p.recipientName = "Test Recipient";
    p.recipientID = RECIPIENT;
    p.deliveryAddress = "Block A";
    p.type = type;
    std::string id;
    registry.registerParcel(p, id);
    registry.routeNextParcel(RouteAction::Default);
    return id;
}

ServiceRequest makeRequest(const std::string& id, ServiceType type, Urgency urgency, int seq) {
    ServiceRequest r;
    r.requestID = id;
    r.type = type;
    r.urgency = urgency;
    r.sequenceNo = seq;
    return r;
}

// ---------------------------------------------------------------------

void testPriorityList() {
    std::cout << "\nRequestPriorityList (sorted linked list)\n";
    RequestPriorityList list;
    check(list.isEmpty() && list.peek() == nullptr, "new list is empty");
    check(list.removeFront() == nullptr, "removeFront on empty list returns nullptr");

    ServiceRequest collect1 = makeRequest("R1", ServiceType::Collection,      Urgency::Normal, 1);
    ServiceRequest delivery = makeRequest("R2", ServiceType::CampusDelivery,  Urgency::Normal, 2);
    ServiceRequest special  = makeRequest("R3", ServiceType::SpecialHandling, Urgency::Normal, 3);
    ServiceRequest urgentC  = makeRequest("R4", ServiceType::Collection,      Urgency::Urgent, 4);
    ServiceRequest collect2 = makeRequest("R5", ServiceType::Collection,      Urgency::Normal, 5);
    ServiceRequest redeliv  = makeRequest("R6", ServiceType::ReDelivery,      Urgency::Normal, 6);
    ServiceRequest ret      = makeRequest("R7", ServiceType::ReturnToSender,  Urgency::Normal, 7);

    list.insert(&collect1);
    list.insert(&delivery);
    list.insert(&special);
    list.insert(&urgentC);
    list.insert(&collect2);
    list.insert(&redeliv);
    list.insert(&ret);
    check(list.getSize() == 7, "size counts every insert");

    // Expected order: urgent first, then type rank, then sequence (FIFO)
    std::string order;
    list.forEach([&order](const ServiceRequest& r, int) { order += r.requestID + " "; });
    check(order == "R4 R3 R6 R2 R7 R1 R5 ", "order is urgency > type > arrival (got " + order + ")");
    check(list.positionOf("R5") == 7 && list.positionOf("X") == 0, "positionOf");

    check(list.remove("R6") == &redeliv && list.getSize() == 6, "remove from the middle");
    check(list.remove("R6") == nullptr, "removing a missing ID returns nullptr");
    check(list.removeFront() == &urgentC && list.peek() == &special, "removeFront takes the head");
    check(list.remove("R5") == &collect2, "remove the tail");
    list.clear();
    check(list.isEmpty() && list.getSize() == 0, "clear empties the list");
}

void testEqualConditions() {
    std::cout << "\nEquivalent processing conditions\n";
    RequestPriorityList list;
    ServiceRequest a = makeRequest("A", ServiceType::CampusDelivery, Urgency::Urgent, 10);
    ServiceRequest b = makeRequest("B", ServiceType::CampusDelivery, Urgency::Urgent, 11);
    ServiceRequest c = makeRequest("C", ServiceType::CampusDelivery, Urgency::Urgent, 12);
    list.insert(&b);   // inserted out of order on purpose
    list.insert(&c);
    list.insert(&a);
    std::string order;
    list.forEach([&order](const ServiceRequest& r, int) { order += r.requestID; });
    check(order == "ABC", "equal urgency and type: earliest request first");
}

void testCreate() {
    std::cout << "\nCreate a service request\n";
    ParcelRegistry registry;
    ResourceManager resources(registry);
    ServiceManager services(registry, resources);

    std::string small = addRoutedParcel(registry, ParcelType::Small);    // ReadyForCollection
    std::string id;

    check(services.createRequest("", RECIPIENT, ServiceType::Collection, Urgency::Normal, "", id)
              == ResultCode::InvalidInput, "blank parcel ID rejected");
    check(services.createRequest("P999", RECIPIENT, ServiceType::Collection, Urgency::Normal, "", id)
              == ResultCode::NotFound, "unknown parcel ID rejected");
    check(services.createRequest(small, "TP999999", ServiceType::Collection, Urgency::Normal, "", id)
              == ResultCode::WrongRecipient, "wrong recipient ID rejected");
    check(services.createRequest(small, RECIPIENT, ServiceType::CampusDelivery, Urgency::Normal, "", id)
              == ResultCode::ServiceMismatch, "service that does not match the route rejected");
    check(services.createRequest(small, RECIPIENT, ServiceType::ReDelivery, Urgency::Normal, "", id)
              == ResultCode::ServiceMismatch, "re-delivery without a failed attempt rejected");
    check(services.getSize() == 0, "failed creates leave nothing behind");

    check(services.createRequest(small, RECIPIENT, ServiceType::Collection, Urgency::Normal, "", id)
              == ResultCode::Success && id == "REQ001", "valid request created as REQ001");
    check(registry.findParcel(small)->requestID == "REQ001", "parcel is linked to the request");
    check(services.findRequest(id)->status == RequestStatus::Pending, "initial status is Pending");

    std::string id2;
    check(services.createRequest(small, RECIPIENT, ServiceType::Collection, Urgency::Urgent, "", id2)
              == ResultCode::Duplicate, "second active request for the same parcel rejected");
    check(registry.removeParcel(small) == ResultCode::InvalidStatus,
          "Task 1 refuses to remove a parcel with an active request");
}

void testProcessCollection() {
    std::cout << "\nProcess a collection request\n";
    ParcelRegistry registry;
    ResourceManager resources(registry);
    ActivityHistory history(registry, &resources);
    ServiceManager services(registry, resources, &history);

    std::string started, resource, follow;
    check(services.startNextRequest(started, resource) == ResultCode::Empty, "start with no requests: Empty");
    ResultCode shown = ResultCode::Success;
    std::string printed = captureOutput([&] { shown = services.displayActive(); });
    check(shown == ResultCode::Empty && printed.empty(),
          "displayActive with no requests: Empty, nothing printed");

    std::string parcel = addRoutedParcel(registry, ParcelType::Document);
    std::string req;
    services.createRequest(parcel, RECIPIENT, ServiceType::Collection, Urgency::Normal, "", req);

    check(services.completeRequest(req, RequestOutcome::Completed, RECIPIENT,
                                   ParcelStatus::ReadyForCollection, follow)
              == ResultCode::InvalidStatus, "cannot complete a request that was not started");

    check(services.startNextRequest(started, resource) == ResultCode::NoResourceAvailable,
          "no counter registered: NoResourceAvailable");
    check(services.findRequest(req)->status == RequestStatus::Pending && services.pendingCount() == 1,
          "failed start leaves the request pending");

    std::string ctr;
    resources.registerResource("Counter A", ResourceType::CollectionCounter, ctr);
    check(services.startNextRequest(started, resource) == ResultCode::Success && resource == ctr,
          "start assigns the free counter");
    check(services.findRequest(req)->status == RequestStatus::InProgress, "request is In Progress");
    check(resources.findResource(ctr)->status == ResourceStatus::Occupied &&
          resources.findResource(ctr)->assignedRequestID == req, "counter occupied by this request");
    check(services.startRequest(req, resource) == ResultCode::InvalidStatus,
          "starting an in-progress request again is rejected");

    check(services.completeRequest(req, RequestOutcome::Completed, "TP999999",
                                   ParcelStatus::ReadyForCollection, follow)
              == ResultCode::WrongRecipient, "wrong collector ID rejected");
    check(services.findRequest(req)->status == RequestStatus::InProgress,
          "request still in progress after wrong collector");

    check(services.completeRequest(req, RequestOutcome::FailedAttempt, "",
                                   ParcelStatus::ReadyForCollection, follow)
              == ResultCode::InvalidStatus, "a collection cannot 'fail delivery'");

    check(services.completeRequest(req, RequestOutcome::Completed, RECIPIENT,
                                   ParcelStatus::ReadyForCollection, follow)
              == ResultCode::Success, "correct collector completes the request");
    Parcel* p = registry.findParcel(parcel);
    check(p->status == ParcelStatus::Collected, "parcel is Collected (Task 1 updated)");
    check(p->requestID.empty() && p->resourceID.empty(), "parcel links cleared");
    check(resources.findResource(ctr)->status == ResourceStatus::Available, "counter released (Task 3)");
    check(services.findRequest(req)->status == RequestStatus::Completed, "request is Completed");
    check(history.getSize() == 3, "history recorded assign, release and status change (Task 4)");

    check(services.completeRequest(req, RequestOutcome::Completed, RECIPIENT,
                                   ParcelStatus::ReadyForCollection, follow)
              == ResultCode::AlreadyCompleted, "completed request cannot be processed again");
    check(services.startRequest(req, resource) == ResultCode::AlreadyCompleted,
          "completed request cannot be started again");
    check(services.cancelRequest(req, "") == ResultCode::AlreadyCompleted,
          "completed request cannot be cancelled");
    std::string again;
    check(services.createRequest(parcel, RECIPIENT, ServiceType::Collection, Urgency::Normal, "", again)
              == ResultCode::AlreadyCompleted, "no new request for a completed parcel");
}

void testDeliveryAttempts() {
    std::cout << "\nFailed deliveries, re-delivery and automatic return\n";
    ParcelRegistry registry;
    ResourceManager resources(registry);
    ServiceManager services(registry, resources);
    std::string dlv;
    resources.registerResource("Courier", ResourceType::DeliveryPersonnel, dlv);

    std::string parcel = addRoutedParcel(registry, ParcelType::Large);   // OutForDelivery
    std::string req, started, resource, follow;
    services.createRequest(parcel, RECIPIENT, ServiceType::CampusDelivery, Urgency::Normal, "", req);

    for (int attempt = 1; attempt <= MAX_DELIVERY_ATTEMPTS; attempt++) {
        services.startNextRequest(started, resource);
        ResultCode result = services.completeRequest(started, RequestOutcome::FailedAttempt, "",
                                                     ParcelStatus::ReadyForCollection, follow);
        check(result == ResultCode::Success && services.findRequest(started)->status == RequestStatus::Failed,
              "attempt " + std::to_string(attempt) + " recorded as Failed");
        ServiceRequest* next = services.findRequest(follow);
        if (attempt < MAX_DELIVERY_ATTEMPTS) {
            check(next->type == ServiceType::ReDelivery && next->attemptNo == attempt + 1,
                  "re-delivery attempt " + std::to_string(attempt + 1) + " created automatically");
        } else {
            check(next->type == ServiceType::ReturnToSender &&
                  registry.findParcel(parcel)->status == ParcelStatus::ReturnToSender,
                  "after the last attempt the parcel goes to Return to Sender");
        }
    }
    check(resources.findResource(dlv)->status == ResourceStatus::Available, "courier released each time");

    services.startNextRequest(started, resource);
    check(services.completeRequest(started, RequestOutcome::Completed, "",
                                   ParcelStatus::ReadyForCollection, follow) == ResultCode::Success &&
          registry.findParcel(parcel)->status == ParcelStatus::Returned,
          "return request completes: parcel Returned");
}

void testSpecialAndReturn() {
    std::cout << "\nSpecial handling and returning a parcel\n";
    ParcelRegistry registry;
    ResourceManager resources(registry);
    ServiceManager services(registry, resources);
    std::string dlv;
    resources.registerResource("Courier", ResourceType::DeliveryPersonnel, dlv);

    std::string fragile = addRoutedParcel(registry, ParcelType::Fragile);   // SpecialHandling
    std::string req, started, resource, follow;
    services.createRequest(fragile, RECIPIENT, ServiceType::SpecialHandling, Urgency::Normal, "", req);
    services.startNextRequest(started, resource);
    check(services.completeRequest(req, RequestOutcome::Completed, "",
                                   ParcelStatus::Collected, follow) == ResultCode::InvalidInput,
          "special handling must go to collection or delivery");
    check(services.completeRequest(req, RequestOutcome::Completed, "",
                                   ParcelStatus::OutForDelivery, follow) == ResultCode::Success,
          "special handling completed");
    check(registry.findParcel(fragile)->status == ParcelStatus::OutForDelivery &&
          services.findRequest(follow)->type == ServiceType::CampusDelivery,
          "parcel moved on and a delivery request follows");

    // Recipient refuses: close as Returned, parcel to Return to Sender
    services.startRequest(follow, resource);
    std::string ret;
    check(services.completeRequest(follow, RequestOutcome::ReturnParcel, "",
                                   ParcelStatus::ReadyForCollection, ret) == ResultCode::Success,
          "refused delivery closed with ReturnParcel");
    check(services.findRequest(follow)->status == RequestStatus::Returned &&
          registry.findParcel(fragile)->status == ParcelStatus::ReturnToSender &&
          services.findRequest(ret)->type == ServiceType::ReturnToSender,
          "request Returned, parcel Return to Sender, return request created");
}

void testCancel() {
    std::cout << "\nCancel requests\n";
    ParcelRegistry registry;
    ResourceManager resources(registry);
    ServiceManager services(registry, resources);
    std::string ctr;
    resources.registerResource("Counter A", ResourceType::CollectionCounter, ctr);

    std::string p1 = addRoutedParcel(registry, ParcelType::Small);
    std::string p2 = addRoutedParcel(registry, ParcelType::Small);
    std::string r1, r2, resource, follow;
    services.createRequest(p1, RECIPIENT, ServiceType::Collection, Urgency::Normal, "", r1);
    services.createRequest(p2, RECIPIENT, ServiceType::Collection, Urgency::Urgent, "", r2);
    check(services.peekNext()->requestID == r2, "urgent request is next even though it came later");

    check(services.cancelRequest("", "") == ResultCode::InvalidInput, "blank request ID rejected");
    check(services.cancelRequest("REQ999", "") == ResultCode::NotFound, "unknown request ID rejected");
    check(services.cancelRequest(r1, "recipient asked") == ResultCode::Success &&
          services.pendingCount() == 1, "pending request cancelled and removed from the list");
    check(registry.findParcel(p1)->requestID.empty() &&
          registry.findParcel(p1)->status == ParcelStatus::ReadyForCollection,
          "parcel unlinked and keeps its status");
    check(services.startRequest(r1, resource) == ResultCode::RequestCancelled,
          "cancelled request cannot be started");
    check(services.cancelRequest(r1, "") == ResultCode::RequestCancelled,
          "cancelled request cannot be cancelled again");

    services.startRequest(r2, resource);
    check(services.cancelRequest(r2, "") == ResultCode::Success &&
          resources.findResource(ctr)->status == ResourceStatus::Available,
          "cancelling an in-progress request releases its counter");

    std::string r3;
    check(services.createRequest(p1, RECIPIENT, ServiceType::Collection, Urgency::Normal, "", r3)
              == ResultCode::Success, "a new request can be made after cancelling");
}

void testIntegrationEdgeCases() {
    std::cout << "\nChanges made by other modules\n";
    ParcelRegistry registry;
    ResourceManager resources(registry);
    ServiceManager services(registry, resources);
    std::string d1, d2;
    resources.registerResource("Courier 1", ResourceType::DeliveryPersonnel, d1);

    std::string parcel = addRoutedParcel(registry, ParcelType::Large);
    std::string req, resource, follow, handed;
    services.createRequest(parcel, RECIPIENT, ServiceType::CampusDelivery, Urgency::Normal, "", req);

    // Task 1 marks the parcel missing while the request waits
    registry.updateParcelStatus(parcel, ParcelStatus::Missing);
    check(services.startRequest(req, resource) == ResultCode::ServiceMismatch,
          "start re-checks the parcel status (parcel went Missing)");
    services.cancelRequest(req, "parcel missing");

    // Courier goes unavailable mid-job with nobody free
    std::string parcel2 = addRoutedParcel(registry, ParcelType::Large);
    std::string req2;
    services.createRequest(parcel2, RECIPIENT, ServiceType::CampusDelivery, Urgency::Normal, "", req2);
    services.startRequest(req2, resource);
    resources.setResourceStatus(d1, ResourceStatus::Unavailable, handed);
    check(handed.empty() && registry.findParcel(parcel2)->resourceID.empty(),
          "courier unavailable, nobody free: parcel has no resource");
    check(services.completeRequest(req2, RequestOutcome::Completed, RECIPIENT,
                                   ParcelStatus::ReadyForCollection, follow) == ResultCode::NotAssigned,
          "cannot complete without a resource");
    resources.registerResource("Courier 2", ResourceType::DeliveryPersonnel, d2);
    check(services.startRequest(req2, resource) == ResultCode::Success && resource == d2,
          "resume assigns the new courier");
    check(services.completeRequest(req2, RequestOutcome::Completed, RECIPIENT,
                                   ParcelStatus::ReadyForCollection, follow) == ResultCode::Success &&
          registry.findParcel(parcel2)->status == ParcelStatus::Delivered,
          "resumed request completes: parcel Delivered");

    std::string out = captureOutput([&] { services.displayAll(); services.viewRequest(req2); });
    check(out.find("REQ002") != std::string::npos && out.find("Delivered to") != std::string::npos,
          "display and view show the request and its outcome");
}

}  // namespace

int main() {
    testPriorityList();
    testEqualConditions();
    testCreate();
    testProcessCollection();
    testDeliveryAttempts();
    testSpecialAndReturn();
    testCancel();
    testIntegrationEdgeCases();

    std::cout << "\n" << (failures == 0 ? "All tests passed." : "Some tests FAILED.") << "\n";
    return failures == 0 ? 0 : 1;
}
