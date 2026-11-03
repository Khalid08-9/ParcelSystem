// Tests for Task 4: ActivityStack (linked-list stack) and ActivityHistory.
#include <iostream>
#include <sstream>
#include <string>

#include "ActivityHistory.hpp"

namespace {

int failures = 0;

void check(bool condition, const std::string& name) {
    std::cout << (condition ? "  PASS  " : "  FAIL  ") << name << "\n";
    if (!condition) failures++;
}

// Runs a display function and returns what it printed instead of showing it
template <typename Function>
std::string captureOutput(Function display) {
    std::ostringstream sink;
    std::streambuf* original = std::cout.rdbuf(sink.rdbuf());
    display();
    std::cout.rdbuf(original);
    return sink.str();
}

// Registers a parcel and records it, like the menu does
std::string addParcel(ParcelRegistry& registry, ActivityHistory& history, ParcelType type) {
    Parcel p{};
    p.recipientName = "Test Recipient";
    p.recipientID = "TP000001";
    p.deliveryAddress = "Block A";
    p.type = type;

    std::string id;
    registry.registerParcel(p, id);
    history.recordRegistration(*registry.findParcel(id));
    return id;
}

// Routes the next waiting parcel with the default rule and records it
void routeNext(ParcelRegistry& registry, ActivityHistory& history) {
    Parcel* p = registry.peekWaiting();
    ParcelStatus before = p->status;
    registry.routeNextParcel(RouteAction::Default);
    history.recordStatusChange(*p, before, ActivityType::Routed);
}

// Changes a status through Task 1 and records it
void changeStatus(ParcelRegistry& registry, ActivityHistory& history,
                  const std::string& id, ParcelStatus newStatus) {
    ParcelStatus before = registry.findParcel(id)->status;
    registry.updateParcelStatus(id, newStatus);
    history.recordStatusChange(*registry.findParcel(id), before);
}

Activity makeActivity(int sequenceNo, const std::string& parcelID) {
    Activity a;
    a.sequenceNo = sequenceNo;
    a.parcelID = parcelID;
    a.type = ActivityType::StatusChanged;
    return a;
}

void testStack() {
    std::cout << "Stack\n";
    ActivityStack s;
    Activity out;

    check(s.isEmpty() && s.peek() == nullptr, "peek on empty returns nullptr");
    check(!s.pop(out), "pop on empty returns false");

    s.push(makeActivity(1, "PCL0001"));
    s.push(makeActivity(2, "PCL0002"));
    s.push(makeActivity(3, "PCL0001"));
    check(s.getSize() == 3 && s.peek()->sequenceNo == 3, "newest activity is on top");
    check(s.findLatestFor("PCL0001")->sequenceNo == 3, "search finds the newest match first");
    check(s.findBySequence(2) != nullptr && s.findBySequence(9) == nullptr, "find by sequence");

    s.peek()->reversed = true;
    check(s.findLatestUndoable("PCL0001")->sequenceNo == 1, "undoable search skips reversed");
    check(s.findLatestUndoable("")->sequenceNo == 2, "empty parcel ID matches any parcel");

    check(s.pop(out) && out.sequenceNo == 3 && s.getSize() == 2, "pop returns the top (LIFO)");
    s.clear();
    check(s.isEmpty() && s.getSize() == 0, "clear empties the stack");
}

void testRecordAndDisplay() {
    std::cout << "Record and display\n";
    ParcelRegistry registry;
    ActivityHistory history(registry);

    check(history.showRecent(5) == ResultCode::Empty, "show recent on empty history");

    std::string a = addParcel(registry, history, ParcelType::Small);  // #1
    std::string b = addParcel(registry, history, ParcelType::Large);  // #2
    routeNext(registry, history);                                     // #3 (parcel a)
    check(history.getSize() == 3, "three activities recorded");

    std::string recent = captureOutput([&] { history.showRecent(2); });
    check(recent.find("#3") < recent.find("#2") && recent.find("#1") == std::string::npos,
          "recent list is newest first and limited");
    check(history.getSize() == 3, "display leaves the stack unchanged");

    std::string parcelA = captureOutput([&] { history.showParcelHistory(a); });
    check(parcelA.find("#1") < parcelA.find("#3") && parcelA.find(b) == std::string::npos,
          "parcel history is oldest first and only that parcel");

    check(history.showRecent(0) == ResultCode::InvalidInput, "count 0 rejected");
    check(history.showParcelHistory("  ") == ResultCode::InvalidInput, "blank parcel ID rejected");
    check(history.showParcelHistory("PCL9999") == ResultCode::NotFound, "unknown parcel ID");

    Parcel unrecorded{};
    unrecorded.recipientName = "No History";
    unrecorded.recipientID = "TP000002";
    unrecorded.deliveryAddress = "Block B";
    std::string quiet;
    registry.registerParcel(unrecorded, quiet);   // not recorded on purpose
    check(history.showParcelHistory(quiet) == ResultCode::Empty, "parcel with no history");
    check(history.reviewParcel(quiet) == ResultCode::Empty, "review parcel with no history");

    std::string review = captureOutput([&] { history.reviewParcel(a); });
    check(review.find("Ready for Collection") != std::string::npos &&
          review.find("#3 Routed") != std::string::npos, "review shows status and latest activity");
}

void testUndoStatus() {
    std::cout << "Undo status changes\n";
    ParcelRegistry registry;
    ActivityHistory history(registry);
    int undone = 0;

    check(history.undoLastActivity(undone) == ResultCode::Empty, "undo on empty history");

    std::string id = addParcel(registry, history, ParcelType::Small);    // #1
    routeNext(registry, history);                                        // #2
    changeStatus(registry, history, id, ParcelStatus::Missing);         // #3

    check(history.undoActivity(0) == ResultCode::InvalidInput, "activity number 0 rejected");
    check(history.undoActivity(99) == ResultCode::NotFound, "unknown activity number");
    check(history.undoActivity(1) == ResultCode::NotReversible, "registration cannot be undone");
    check(history.undoActivity(2) == ResultCode::NotReversible, "older activity blocked by newer one");

    check(history.undoLastActivity(undone) == ResultCode::Success && undone == 3 &&
          registry.findParcel(id)->status == ParcelStatus::ReadyForCollection &&
          registry.collectionCount() == 1, "undo Missing restores status and queue");
    check(history.undoActivity(3) == ResultCode::AlreadyReversed, "same activity twice rejected");
    check(history.undoActivity(4) == ResultCode::NotReversible, "undo entry cannot be undone");

    check(history.undoActivity(2) == ResultCode::Success &&
          registry.findParcel(id)->status == ParcelStatus::Registered &&
          registry.waitingCount() == 1 && registry.collectionCount() == 0,
          "undo route puts parcel back in the waiting queue");
    check(history.undoLastActivity(undone) == ResultCode::NotReversible && undone == 1,
          "only the registration is left");
}

void testUndoBlocked() {
    std::cout << "Undo blocked\n";
    ParcelRegistry registry;
    ActivityHistory history(registry);

    std::string done = addParcel(registry, history, ParcelType::Small);  // #1
    routeNext(registry, history);                                        // #2
    changeStatus(registry, history, done, ParcelStatus::Collected);     // #3
    check(history.undoActivity(3) == ResultCode::NotReversible, "final status cannot be undone");
    check(history.undoActivity(2) == ResultCode::NotReversible, "nothing before a final status");

    std::string drifted = addParcel(registry, history, ParcelType::Large); // #4
    routeNext(registry, history);                                          // #5
    registry.updateParcelStatus(drifted, ParcelStatus::Missing);          // not recorded
    check(history.undoActivity(5) == ResultCode::InvalidStatus,
          "parcel changed since the activity");

    std::string removed = addParcel(registry, history, ParcelType::Small); // #6
    Parcel copy = *registry.findParcel(removed);
    registry.removeParcel(removed);
    history.recordRemoval(copy);                                          // #7
    int undone = 0;
    check(history.undoLastActivity(undone) == ResultCode::NotReversible && undone == 7,
          "removal cannot be undone");
}

void testUndoDetails() {
    std::cout << "Undo details edit\n";
    ParcelRegistry registry;
    ActivityHistory history(registry);

    std::string id = addParcel(registry, history, ParcelType::Document);
    Parcel before = *registry.findParcel(id);
    registry.updateParcelDetails(id, "New Name", "", "Block Z");
    history.recordDetailsEdit(*registry.findParcel(id), before);

    int undone = 0;
    Parcel* p = registry.findParcel(id);
    check(history.undoLastActivity(undone) == ResultCode::Success &&
          p->recipientName == "Test Recipient" && p->deliveryAddress == "Block A",
          "undo restores old recipient details");
}

void testUndoResources() {
    std::cout << "Undo resource activities\n";
    ParcelRegistry registry;
    ResourceManager manager(registry);
    ActivityHistory history(registry, &manager);
    std::string counter, assigned;
    manager.registerResource("Counter A", ResourceType::CollectionCounter, counter);

    std::string id = addParcel(registry, history, ParcelType::Small);    // #1
    routeNext(registry, history);                                        // #2
    manager.assignNextAvailable(id, "", assigned);
    history.recordResourceAssigned(*registry.findParcel(id));            // #3

    check(history.undoActivity(2) == ResultCode::NotReversible,
          "route blocked while a resource holds the parcel");
    check(history.undoActivity(3) == ResultCode::Success &&
          manager.findResource(counter)->status == ResourceStatus::Available &&
          registry.findParcel(id)->resourceID.empty(), "undo assignment frees the counter");

    manager.assignNextAvailable(id, "", assigned);
    history.recordResourceAssigned(*registry.findParcel(id));            // #5
    manager.releaseResource(counter);
    history.recordResourceReleased(*registry.findParcel(id), counter);   // #6
    check(history.undoActivity(6) == ResultCode::Success &&
          registry.findParcel(id)->resourceID == counter &&
          manager.findResource(counter)->status == ResourceStatus::Occupied,
          "undo release gives the parcel back to the same counter");

    ActivityHistory noResources(registry);   // Task 3 not connected
    noResources.recordResourceAssigned(*registry.findParcel(id));
    int undone = 0;
    check(noResources.undoLastActivity(undone) == ResultCode::NotReversible,
          "resource undo needs the resource manager");
}

}  // namespace

int main() {
    testStack();
    testRecordAndDisplay();
    testUndoStatus();
    testUndoBlocked();
    testUndoDetails();
    testUndoResources();

    std::cout << "\n" << (failures == 0 ? "All tests passed." : "Some tests FAILED.") << "\n";
    return failures == 0 ? 0 : 1;
}
