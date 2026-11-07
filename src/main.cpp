

#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <limits>
#include <string>
#include "ParcelRegistry.hpp"
#include "ParcelTree.hpp"
#include "ActivityHistory.hpp"
#include "ResourceManager.hpp"
#include "ServiceManager.hpp"

namespace {

// All statuses in one list, so every menu uses the same order
const ParcelStatus kAllStatuses[] = {
    ParcelStatus::Registered,     ParcelStatus::ReadyForCollection,
    ParcelStatus::OutForDelivery, ParcelStatus::SpecialHandling,
    ParcelStatus::ReturnToSender, ParcelStatus::OnHold,
    ParcelStatus::Missing,        ParcelStatus::Collected,
    ParcelStatus::Delivered,      ParcelStatus::Returned,
    ParcelStatus::Cancelled
};
const int kStatusCount = 11;

// ======================= Input helpers =======================

// If the input stream has ended (Ctrl+D, or a piped file ran out), stop cleanly
// instead of looping forever.
void exitIfInputEnded() {
    if (std::cin.eof()) {
        std::cout << "\nInput ended. Exiting.\n";
        std::exit(0);
    }
}

// Prints a prompt and reads a whole line (names and addresses contain spaces)
std::string readLine(const std::string& prompt) {
    std::string text;
    std::cout << prompt;
    if (!std::getline(std::cin, text)) {
        exitIfInputEnded();
    }
    return text;
}

// Reads an integer between minValue and maxValue; asks again until valid
int readInt(const std::string& prompt, int minValue, int maxValue) {
    int value = 0;
    while (true) {
        std::cout << prompt;
        if (std::cin >> value) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            if (value >= minValue && value <= maxValue) {
                return value;
            }
            std::cout << "Please enter a number from " << minValue
                      << " to " << maxValue << ".\n";
        } else {
            exitIfInputEnded();
            std::cin.clear();   // not a number: reset the stream, drop the line
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid input. Please enter a number.\n";
        }
    }
}

// Asks a yes/no question; returns true for y or Y
bool confirm(const std::string& question) {
    std::string answer = readLine(question + " (y/n): ");
    return !answer.empty() && (answer[0] == 'y' || answer[0] == 'Y');
}

void pressEnter() {
    std::cout << "\nPress Enter to continue...";
    std::string ignored;
    if (!std::getline(std::cin, ignored)) {
        exitIfInputEnded();
    }
}

// ======================= Output helpers =======================

void printTitle(const std::string& title) {
    std::cout << "\n" << title << "\n" << std::string(title.size(), '=') << "\n";
}

// One place that turns a ResultCode into a message for the user
std::string describe(ResultCode code) {
    switch (code) {
        case ResultCode::Success:          return "Done.";
        case ResultCode::NotFound:         return "Error: parcel not found.";
        case ResultCode::Duplicate:        return "Error: a parcel with this ID already exists.";
        case ResultCode::InvalidInput:     return "Error: required information is missing or invalid.";
        case ResultCode::InvalidStatus:    return "Error: that status change is not allowed.";
        case ResultCode::Empty:            return "Error: there are no parcels waiting to be processed.";
        case ResultCode::AlreadyCompleted: return "Error: this parcel is already completed.";
        default:                           return "Error: the operation failed.";
    }
}

// The registry is authoritative; the tree stores independent copies.
bool synchronizeParcelTree(const ParcelRegistry& registry, treeNode*& root) {
    ResultCode result = ResultCode::Success;
    registry.forEachParcel([&](const Parcel& p) {
        if (result != ResultCode::Success) return;
        result = updateParcelRecord(root, p.parcelID, p);
        if (result == ResultCode::NotFound) {
            result = insertParcelRecord(root, p);
        }
    });
    if (result != ResultCode::Success) {
        std::cout << "Error: could not synchronize the parcel tree. " << describe(result) << "\n";
        return false;
    }
    return true;
}

// Summary shown above the main menu
void showDashboard(const ParcelRegistry& registry, const ServiceManager& services,
                   const ResourceManager& resources) {
    std::cout << "\n" << std::string(46, '=') << "\n"
              << "  SMART CAMPUS PARCEL SYSTEM\n"
              << std::string(46, '=') << "\n"
              << std::left
              << "  Registered parcels : " << registry.getSize() << "\n"
              << "  Waiting to process : " << registry.waitingCount() << "\n"
              << "  Collection / Delivery / Special / Return / Hold : "
              << registry.collectionCount() << " / "
              << registry.deliveryCount() << " / "
              << registry.specialCount() << " / "
              << registry.returnCount() << " / "
              << registry.holdCount() << "\n"
              << "  Service requests  : " << services.pendingCount() << " pending, "
              << services.countByStatus(RequestStatus::InProgress) << " in progress\n"
              << "  Resources         : "
              << resources.countByStatus(ResourceStatus::Available) << " available, "
              << resources.countByStatus(ResourceStatus::Occupied) << " occupied, "
              << resources.countByStatus(ResourceStatus::Unavailable) << " unavailable\n"
              << std::string(46, '-') << "\n";
}

// ======================= Registration =======================

ParcelType readParcelType() {
    std::cout << "Parcel type:\n"
              << "  1. Document\n"
              << "  2. Small\n"
              << "  3. Large\n"
              << "  4. Fragile\n";
    switch (readInt("Choose (1-4): ", 1, 4)) {
        case 1:  return ParcelType::Document;
        case 2:  return ParcelType::Small;
        case 3:  return ParcelType::Large;
        default: return ParcelType::Fragile;
    }
}

void registerParcelMenu(ParcelRegistry& registry, ActivityHistory& history, treeNode*& root) {
    printTitle("REGISTER A NEW PARCEL");
    std::cout << "The Parcel ID is created automatically.\n";

    Parcel p;
    p.recipientName   = readLine("Recipient name: ");
    while (true) {
        p.recipientID = readLine("Recipient ID (RC + 6 digits, e.g. RC012345): ");
        if (p.recipientID.size() == 8 &&
            p.recipientID.compare(0, 2, "RC") == 0 &&
            p.recipientID.find_first_not_of("0123456789", 2) == std::string::npos) {
            break;
        }
        std::cout << "Recipient ID must be RC followed by exactly 6 digits. Please try again.\n";
    }
    p.deliveryAddress = readLine("Delivery address: ");
    p.type            = readParcelType();

    std::string assignedID;                       // filled in by the registry
    ResultCode result = registry.registerParcel(p, assignedID);
    if (result == ResultCode::Success) {
        ResultCode inserted = insertParcelRecord(root, *registry.findParcel(assignedID));
        if (inserted != ResultCode::Success) {
            registry.removeParcel(assignedID);
            std::cout << "Registration cancelled: could not insert the parcel into the tree. "
                      << describe(inserted) << "\n";
            return;
        }
        history.recordRegistration(*registry.findParcel(assignedID));
        std::cout << "Parcel registered with ID " << assignedID
                  << " (status Registered, added to the waiting queue).\n";
    } else {
        std::cout << describe(result) << "\n";
        if (result == ResultCode::InvalidInput) {
            std::cout << "Name and address must not be blank. Recipient ID must be RC followed by exactly 6 digits.\n";
        }
    }
}

// ======================= Records and searches =======================

void viewParcelMenu(const ParcelRegistry& registry) {
    printTitle("VIEW A PARCEL");

    std::string id = readLine("Parcel ID: ");
    ResultCode result = registry.viewParcel(id);
    if (result != ResultCode::Success) {
        std::cout << describe(result) << "\n";
    }
}

// Lists every status; returns false if the user cancels
bool readAnyStatus(ParcelStatus& chosen) {
    std::cout << "\nChoose a status:\n";
    for (int i = 0; i < kStatusCount; ++i) {
        std::cout << "  " << std::setw(2) << (i + 1) << ". "
                  << statusToString(kAllStatuses[i]) << "\n";
    }
    std::cout << "   0. Cancel\n";

    int choice = readInt("Choice (0-11): ", 0, kStatusCount);
    if (choice == 0) {
        return false;
    }
    chosen = kAllStatuses[choice - 1];
    return true;
}

void searchByStatusMenu(const ParcelRegistry& registry) {
    printTitle("SEARCH BY STATUS");

    ParcelStatus status = ParcelStatus::Registered;
    if (!readAnyStatus(status)) {
        std::cout << "Cancelled.\n";
        return;
    }

    ResultCode result = registry.searchByStatus(status);
    if (result == ResultCode::NotFound) {
        std::cout << "No parcels have the status "
                  << statusToString(status) << ".\n";
    } else if (result != ResultCode::Success) {
        std::cout << describe(result) << "\n";
    }
}

void searchByRecipientMenu(const ParcelRegistry& registry) {
    printTitle("SEARCH BY RECIPIENT ID");

    std::string recipientID = readLine("Recipient ID: ");
    ResultCode result = registry.searchByRecipientID(recipientID);
    if (result == ResultCode::NotFound) {
        std::cout << "No parcels match this recipient ID.\n";
    } else if (result != ResultCode::Success) {
        std::cout << describe(result) << "\n";
    }
}

// ======================= Processing =======================

// Shows the next waiting parcel, lets staff choose the route, then routes it
void processNextMenu(ParcelRegistry& registry, ActivityHistory& history) {
    printTitle("PROCESS NEXT PARCEL");

    Parcel* next = registry.peekWaiting();
    if (next == nullptr) {
        std::cout << describe(ResultCode::Empty) << "\n";
        return;
    }

    std::cout << "Next parcel in the queue:\n";
    registry.viewParcel(next->parcelID);

    std::cout << "\nChoose how to route it:\n"
              << "  1. Default route (by parcel type)\n"
              << "  2. Return to sender\n"
              << "  3. Hold for investigation\n"
              << "  4. Report as missing\n"
              << "  0. Cancel\n";
    int choice = readInt("Choice (0-4): ", 0, 4);
    if (choice == 0) {
        std::cout << "Cancelled. The parcel stays in the queue.\n";
        return;
    }

    RouteAction action = RouteAction::Default;
    if (choice == 2) action = RouteAction::ReturnToSender;
    else if (choice == 3) action = RouteAction::OnHold;
    else if (choice == 4) action = RouteAction::Missing;

    // Parcels are owned by the registry, so this pointer stays valid
    std::string id = next->parcelID;
    ParcelStatus before = next->status;
    ResultCode result = registry.routeNextParcel(action);

    if (result == ResultCode::Success) {
        history.recordStatusChange(*next, before, ActivityType::Routed);
        std::cout << "Parcel " << id << " is now: "
                  << statusToString(next->status) << "\n";
    } else {
        std::cout << describe(result) << "\n";
    }
}

// Routes every waiting parcel using the default rule
void processAllMenu(ParcelRegistry& registry, ActivityHistory& history) {
    printTitle("PROCESS ALL WAITING PARCELS");

    if (registry.waitingCount() == 0) {
        std::cout << describe(ResultCode::Empty) << "\n";
        return;
    }

    std::cout << registry.waitingCount()
              << " parcel(s) will be routed with the default rule (by parcel type).\n";
    if (!confirm("Continue?")) {
        std::cout << "Cancelled.\n";
        return;
    }

    int routed = 0;
    while (registry.waitingCount() > 0) {
        Parcel* next = registry.peekWaiting();
        ParcelStatus before = next->status;
        ResultCode result = registry.routeNextParcel(RouteAction::Default);
        if (result != ResultCode::Success) {
            std::cout << describe(result) << " Stopping.\n";
            break;
        }
        history.recordStatusChange(*next, before, ActivityType::Routed);
        ++routed;
    }
    std::cout << routed << " parcel(s) routed.\n";
}

// ======================= Update and remove =======================

// Lists ONLY the statuses this parcel may move to (checked with canTransition),
// and returns the one chosen; returns false if there is nothing to choose or
// the user cancels.
bool readAllowedStatus(ParcelStatus current, ParcelStatus& chosen) {
    ParcelStatus allowed[kStatusCount];
    int count = 0;
    for (int i = 0; i < kStatusCount; ++i) {
        if (canTransition(current, kAllStatuses[i])) {
            allowed[count++] = kAllStatuses[i];
        }
    }

    if (count == 0) {
        std::cout << "This parcel is in a final status. No change is possible.\n";
        return false;
    }

    std::cout << "\nAllowed new statuses (current: " << statusToString(current) << "):\n";
    for (int i = 0; i < count; ++i) {
        std::cout << "  " << (i + 1) << ". " << statusToString(allowed[i]) << "\n";
    }
    std::cout << "  0. Cancel\n";

    int choice = readInt("Choice: ", 0, count);
    if (choice == 0) {
        return false;
    }
    chosen = allowed[choice - 1];
    return true;
}

void updateStatusMenu(ParcelRegistry& registry, ActivityHistory& history) {
    printTitle("UPDATE PARCEL STATUS");

    std::string id = readLine("Parcel ID: ");
    ResultCode found = registry.viewParcel(id);   // shows the current details
    if (found != ResultCode::Success) {
        std::cout << describe(found) << "\n";
        return;
    }

    Parcel* p = registry.findParcel(id);
    if (!p->requestID.empty() || !p->resourceID.empty()) {
        std::cout << "Complete or cancel the active request and release its resource "
                     "before changing the parcel's status.\n";
        return;
    }

    ParcelStatus current = p->status;
    ParcelStatus newStatus = current;
    if (!readAllowedStatus(current, newStatus)) {
        std::cout << "Nothing was changed.\n";
        return;
    }

    ResultCode result = registry.updateParcelStatus(id, newStatus);
    if (result == ResultCode::Success) {
        history.recordStatusChange(*registry.findParcel(id), current);
        std::cout << "Parcel " << id << " is now: "
                  << statusToString(newStatus) << "\n";
    } else {
        std::cout << describe(result) << "\n";
    }
}

// Edits recipient name, recipient ID and address; Enter keeps the old value
void editDetailsMenu(ParcelRegistry& registry, ActivityHistory& history) {
    printTitle("EDIT PARCEL DETAILS");

    std::string id = readLine("Parcel ID: ");
    ResultCode found = registry.viewParcel(id);   // shows the current details
    if (found != ResultCode::Success) {
        std::cout << describe(found) << "\n";
        return;
    }

    std::cout << "\nPress Enter to keep the current value.\n";
    std::string name    = readLine("New recipient name: ");
    std::string recID   = readLine("New recipient ID (RC + 6 digits): ");
    std::string address = readLine("New delivery address: ");

    Parcel before = *registry.findParcel(id);   // old values, so the edit can be undone
    ResultCode result = registry.updateParcelDetails(id, name, recID, address);
    if (result == ResultCode::Success) {
        history.recordDetailsEdit(*registry.findParcel(id), before);
        std::cout << "Details updated.\n";
        registry.viewParcel(id);
    } else if (result == ResultCode::InvalidInput) {
        std::cout << "No changes made. Enter at least one new value; a supplied recipient ID must be RC followed by exactly 6 digits.\n";
    } else {
        std::cout << describe(result) << "\n";
    }
}

// Permanently removes a parcel after showing it and asking for confirmation
void removeParcelMenu(ParcelRegistry& registry, ActivityHistory& history, treeNode*& root) {
    printTitle("REMOVE A PARCEL");

    std::string id = readLine("Parcel ID: ");
    ResultCode found = registry.viewParcel(id);
    if (found != ResultCode::Success) {
        std::cout << describe(found) << "\n";
        return;
    }

    std::cout << "\nTip: to keep a record, update the status to Cancelled instead.\n";
    if (!confirm("Remove this parcel permanently?")) {
        std::cout << "Cancelled. The parcel was not removed.\n";
        return;
    }

    Parcel removed = *registry.findParcel(id);   // copy kept for the history
    ResultCode result = registry.removeParcel(id);
    if (result == ResultCode::Success) {
        ResultCode removedFromTree = removeParcelRecord(root, id);
        history.recordRemoval(removed);
        if (removedFromTree != ResultCode::Success) {
            std::cout << "Error: parcel removed from the registry, but tree removal failed. "
                      << describe(removedFromTree) << "\n";
            return;
        }
        std::cout << "Parcel " << id << " was removed.\n";
    } else if (result == ResultCode::InvalidStatus) {
        std::cout << "Error: this parcel is linked to a service request or resource "
                     "and cannot be removed. Cancel it instead.\n";
    } else {
        std::cout << describe(result) << "\n";
    }
}

// ======================= Activity history (Task 4) =======================

void showResult(ResultCode result) {
    if (result != ResultCode::Success) {
        std::cout << activityResultMessage(result) << "\n";
    }
}

void recentActivitiesMenu(ActivityHistory& history) {
    printTitle("RECENT ACTIVITIES");
    showResult(history.showRecent(readInt("How many to show (1-100): ", 1, 100)));
}

void parcelHistoryMenu(ActivityHistory& history) {
    printTitle("PARCEL ACTIVITY HISTORY");
    showResult(history.showParcelHistory(readLine("Parcel ID: ")));
}

void reviewParcelMenu(ActivityHistory& history) {
    printTitle("REVIEW PARCEL ACTIVITIES");
    showResult(history.reviewParcel(readLine("Parcel ID: ")));
}

// Shows recent activities, lets staff pick one, confirms, then undoes it
void undoMenu(ActivityHistory& history) {
    printTitle("UNDO AN ACTIVITY");

    ResultCode shown = history.showRecent(10);
    if (shown != ResultCode::Success) {
        showResult(shown);
        return;
    }

    std::cout << "\nEnter an activity number, or 0 for the most recent one.\n";
    int number = readInt("Activity number: ", 0, std::numeric_limits<int>::max());
    if (!confirm("Undo this activity?")) {
        std::cout << "Cancelled. Nothing was changed.\n";
        return;
    }

    int undone = number;
    ResultCode result = (number == 0) ? history.undoLastActivity(undone)
                                      : history.undoActivity(number);
    if (result == ResultCode::Success) {
        std::cout << "Activity #" << undone << " was reversed.\n";
    } else {
        showResult(result);
    }
}

// ======================= Service requests (Task 2) =======================

void showService(ResultCode result) {
    std::cout << serviceResultMessage(result) << "\n";
}

ServiceType readServiceType() {
    std::cout << "Service type:\n"
              << "  1. Collection        (parcel must be Ready for Collection)\n"
              << "  2. Campus Delivery   (parcel must be Out for Delivery)\n"
              << "  3. Special Handling  (parcel must be in Special Handling)\n"
              << "  4. Re-delivery       (after a failed delivery attempt)\n"
              << "  5. Return to Sender  (parcel must be Return to Sender)\n";
    switch (readInt("Choose (1-5): ", 1, 5)) {
        case 1:  return ServiceType::Collection;
        case 2:  return ServiceType::CampusDelivery;
        case 3:  return ServiceType::SpecialHandling;
        case 4:  return ServiceType::ReDelivery;
        default: return ServiceType::ReturnToSender;
    }
}

void createRequestMenu(ServiceManager& services, const ParcelRegistry& registry) {
    printTitle("CREATE A SERVICE REQUEST");

    std::string parcelID = readLine("Parcel ID: ");
    if (registry.viewParcel(parcelID) != ResultCode::Success) {
        showService(ResultCode::NotFound);
        return;
    }
    std::cout << "\n";
    std::string recipientID = readLine("Recipient ID (to confirm the recipient): ");
    ServiceType type = readServiceType();
    Urgency urgency = confirm("Is this request urgent?") ? Urgency::Urgent : Urgency::Normal;
    std::string notes = readLine("Notes / recipient requirements (optional): ");

    std::string requestID;
    ResultCode result = services.createRequest(parcelID, recipientID, type, urgency,
                                               notes, requestID);
    if (result == ResultCode::Success) {
        std::cout << "Request " << requestID << " created (status Pending).\n";
    } else {
        showService(result);
    }
}

void viewRequestMenu(const ServiceManager& services) {
    printTitle("VIEW A SERVICE REQUEST");
    ResultCode result = services.viewRequest(readLine("Request ID: "));
    if (result != ResultCode::Success) {
        showService(result);
    }
}

void startNextRequestMenu(ServiceManager& services) {
    printTitle("START NEXT REQUEST (HIGHEST PRIORITY)");

    ServiceRequest* next = services.peekNext();
    if (next == nullptr) {
        showService(ResultCode::Empty);
        return;
    }
    std::cout << "Next request by priority:";
    services.viewRequest(next->requestID);
    if (!confirm("\nStart this request now?")) {
        std::cout << "Cancelled. The request stays at the front.\n";
        return;
    }
    std::string startedID, resourceID;
    ResultCode result = services.startNextRequest(startedID, resourceID);
    if (result == ResultCode::Success) {
        std::cout << "Request " << startedID << " is In Progress, handled by "
                  << resourceID << ".\n";
    } else {
        showService(result);
    }
}

void startSelectedRequestMenu(ServiceManager& services) {
    printTitle("START A SELECTED REQUEST");
    std::cout << "Use this to serve a request out of priority order, or to resume an\n"
                 "in-progress request that lost its resource.\n";

    std::string requestID = readLine("Request ID: ");
    std::string resourceID;
    ResultCode result = services.startRequest(requestID, resourceID);
    if (result == ResultCode::Success) {
        std::cout << "Request " << requestID << " is In Progress, handled by "
                  << resourceID << ".\n";
    } else {
        showService(result);
    }
}

void completeRequestMenu(ServiceManager& services) {
    printTitle("COMPLETE A SERVICE REQUEST");

    std::string requestID = readLine("Request ID: ");
    ResultCode found = services.viewRequest(requestID);
    if (found != ResultCode::Success) {
        showService(found);
        return;
    }
    ServiceRequest* r = services.findRequest(requestID);
    bool delivery = r->type == ServiceType::CampusDelivery || r->type == ServiceType::ReDelivery;

    std::cout << "\nOutcome:\n"
              << "  1. Service completed\n"
              << "  2. Delivery attempt failed (recipient not reached)\n"
              << "  3. Return parcel to sender (refused / wrong details)\n"
              << "  0. Cancel\n";
    int choice = readInt("Choice (0-3): ", 0, 3);
    if (choice == 0) {
        std::cout << "Nothing was changed.\n";
        return;
    }

    RequestOutcome outcome = RequestOutcome::Completed;
    if (choice == 2) outcome = RequestOutcome::FailedAttempt;
    else if (choice == 3) outcome = RequestOutcome::ReturnParcel;

    std::string receiverID;
    ParcelStatus afterSpecial = ParcelStatus::ReadyForCollection;
    if (outcome == RequestOutcome::Completed) {
        if (r->type == ServiceType::Collection || delivery) {
            receiverID = readLine("Recipient ID shown by the person receiving it: ");
        } else if (r->type == ServiceType::SpecialHandling) {
            std::cout << "Where should the parcel go next?\n"
                      << "  1. Ready for Collection\n"
                      << "  2. Out for Delivery\n";
            afterSpecial = readInt("Choose (1-2): ", 1, 2) == 1 ? ParcelStatus::ReadyForCollection
                                                                : ParcelStatus::OutForDelivery;
        }
    }

    std::string followUpID;
    ResultCode result = services.completeRequest(requestID, outcome, receiverID,
                                                 afterSpecial, followUpID);
    if (result == ResultCode::Success) {
        std::cout << "Request " << requestID << " closed: "
                  << requestStatusToString(r->status) << ". " << r->outcomeNote << ".\n";
        if (!followUpID.empty()) {
            ServiceRequest* follow = services.findRequest(followUpID);
            std::cout << "Follow-up request " << followUpID << " ("
                      << serviceTypeToString(follow->type) << ") was created automatically.\n";
        }
    } else {
        showService(result);
    }
}

void cancelRequestMenu(ServiceManager& services) {
    printTitle("CANCEL A SERVICE REQUEST");

    std::string requestID = readLine("Request ID: ");
    ResultCode found = services.viewRequest(requestID);
    if (found != ResultCode::Success) {
        showService(found);
        return;
    }
    std::string reason = readLine("\nReason (optional): ");
    if (!confirm("Cancel this request?")) {
        std::cout << "Nothing was changed.\n";
        return;
    }
    ResultCode result = services.cancelRequest(requestID, reason);
    if (result == ResultCode::Success) {
        std::cout << "Request " << requestID << " was cancelled. The parcel can get a new request.\n";
    } else {
        showService(result);
    }
}

void serviceMenu(ServiceManager& services, const ParcelRegistry& registry, treeNode*& root) {
    while (true) {
        printTitle("SERVICE REQUESTS (Task 2)");
        std::cout << "  1. Create a service request\n"
                  << "  2. Show active requests (priority order)\n"
                  << "  3. Show all requests\n"
                  << "  4. View a request\n"
                  << "  5. Start next request (highest priority)\n"
                  << "  6. Start / resume a selected request\n"
                  << "  7. Complete a request\n"
                  << "  8. Cancel a request\n"
                  << "  0. Back\n";
        switch (readInt("Enter your choice (0-8): ", 0, 8)) {
            case 1: createRequestMenu(services, registry); break;
            case 2: {
                ResultCode result = services.displayActive();
                if (result != ResultCode::Success) showService(result);
                break;
            }
            case 3: services.displayAll();               break;
            case 4: viewRequestMenu(services);           break;
            case 5: startNextRequestMenu(services);      break;
            case 6: startSelectedRequestMenu(services);  break;
            case 7: completeRequestMenu(services);       break;
            case 8: cancelRequestMenu(services);         break;
            case 0: return;
        }
        if (!synchronizeParcelTree(registry, root)) return;
        pressEnter();
    }
}

// ======================= Resources (Task 3) =======================

void showResource(ResultCode result) {
    std::cout << resourceResultMessage(result) << "\n";
}

void registerResourceMenu(ResourceManager& resources) {
    printTitle("REGISTER A RESOURCE");
    std::string name = readLine("Name (counter name or staff name): ");
    std::cout << "Type:\n  1. Collection Counter\n  2. Delivery Personnel\n";
    ResourceType type = readInt("Choose (1-2): ", 1, 2) == 1 ? ResourceType::CollectionCounter
                                                             : ResourceType::DeliveryPersonnel;
    std::string id;
    ResultCode result = resources.registerResource(name, type, id);
    if (result == ResultCode::Success) {
        std::cout << "Resource registered with ID " << id << " (Available).\n";
    } else {
        showResource(result);
    }
}

void assignResourceMenu(ResourceManager& resources, ParcelRegistry& registry,
                        ActivityHistory& history) {
    printTitle("ASSIGN A RESOURCE TO A PARCEL");
    std::string parcelID = readLine("Parcel ID: ");
    Parcel* p = registry.findParcel(parcelID);
    if (p == nullptr) {
        showResource(ResultCode::NotFound);
        return;
    }
    std::string resourceID = readLine("Resource ID (Enter = next available): ");
    ResultCode result;
    if (resourceID.empty()) {
        result = resources.assignNextAvailable(parcelID, p->requestID, resourceID);
    } else {
        result = resources.assignToResource(resourceID, parcelID, p->requestID);
    }
    if (result == ResultCode::Success) {
        history.recordResourceAssigned(*p);
        std::cout << "Parcel " << parcelID << " assigned to " << p->resourceID << ".\n";
    } else {
        showResource(result);
    }
}

void releaseResourceMenu(ResourceManager& resources, ParcelRegistry& registry,
                         ActivityHistory& history) {
    printTitle("RELEASE A RESOURCE");
    std::string resourceID = readLine("Resource ID: ");
    Resource* r = resources.findResource(resourceID);
    std::string parcelID = (r != nullptr) ? r->assignedParcelID : "";
    ResultCode result = resources.releaseResource(resourceID);
    if (result == ResultCode::Success) {
        Parcel* p = registry.findParcel(parcelID);
        if (p != nullptr) {
            history.recordResourceReleased(*p, resourceID);
        }
        std::cout << "Resource " << resourceID << " is Available again.\n";
    } else {
        showResource(result);
    }
}

void resourceStatusMenu(ResourceManager& resources, ParcelRegistry& registry,
                        ActivityHistory& history) {
    printTitle("CHANGE RESOURCE STATUS");
    std::string resourceID = readLine("Resource ID: ");
    Resource* r = resources.findResource(resourceID);
    if (r == nullptr) {
        showResource(ResultCode::NotFound);
        return;
    }
    std::cout << "Current status: " << resourceStatusToString(r->status) << "\n"
              << "  1. Available\n  2. Unavailable\n";
    ResourceStatus newStatus = readInt("Choose (1-2): ", 1, 2) == 1 ? ResourceStatus::Available
                                                                    : ResourceStatus::Unavailable;
    std::string parcelID = r->assignedParcelID;
    std::string handedTo;
    ResultCode result = resources.setResourceStatus(resourceID, newStatus, handedTo);
    if (result != ResultCode::Success) {
        showResource(result);
        return;
    }
    std::cout << "Resource " << resourceID << " is now "
              << resourceStatusToString(newStatus) << ".\n";
    Parcel* p = registry.findParcel(parcelID);
    if (p != nullptr) {
        history.recordResourceReleased(*p, resourceID);
        if (!handedTo.empty()) {
            history.recordResourceAssigned(*p);
            std::cout << "Its parcel " << parcelID << " was handed to " << handedTo << ".\n";
        } else if (newStatus == ResourceStatus::Unavailable) {
            std::cout << "No free resource could take parcel " << parcelID
                      << ". Resume its request from the Service Requests menu.\n";
        }
    }
}

void resourceMenu(ResourceManager& resources, ParcelRegistry& registry,
                  ActivityHistory& history, treeNode*& root) {
    while (true) {
        printTitle("RESOURCES (Task 3)");
        std::cout << "  1. Register a resource\n"
                  << "  2. Show resources\n"
                  << "  3. Assign a resource to a parcel\n"
                  << "  4. Release a resource\n"
                  << "  5. Change resource status\n"
                  << "  0. Back\n";
        switch (readInt("Enter your choice (0-5): ", 0, 5)) {
            case 1: registerResourceMenu(resources);                     break;
            case 2: resources.displayResources();                        break;
            case 3: assignResourceMenu(resources, registry, history);    break;
            case 4: releaseResourceMenu(resources, registry, history);   break;
            case 5: resourceStatusMenu(resources, registry, history);    break;
            case 0: return;
        }
        if (!synchronizeParcelTree(registry, root)) return;
        pressEnter();
    }
}

// ======================= Parcel organisation (Task 5) =======================

void searchParcelTreeMenu(const treeNode* root) {
    printTitle("SEARCH THE PARCEL TREE");

    const Parcel* p = nullptr;
    ResultCode result = findParcelRecord(root, readLine("Parcel ID: "), p);
    if (result != ResultCode::Success) {
        std::cout << describe(result) << "\n";
        return;
    }

    std::cout << "\nTREE PARCEL DETAILS\n"
              << std::string(40, '-') << "\n"
              << "Parcel ID:        " << p->parcelID << "\n"
              << "Recipient name:   " << p->recipientName << "\n"
              << "Recipient ID:     " << p->recipientID << "\n"
              << "Delivery address: " << p->deliveryAddress << "\n"
              << "Type:             " << typeToString(p->type) << "\n"
              << "Status:           " << statusToString(p->status) << "\n"
              << "Request ID:       " << (p->requestID.empty() ? "None" : p->requestID) << "\n"
              << "Resource ID:      " << (p->resourceID.empty() ? "None" : p->resourceID) << "\n"
              << std::string(40, '-') << "\n";
}

void parcelTreeMenu(ParcelRegistry& registry, ActivityHistory& history, treeNode*& root) {
    while (true) {
        printTitle("PARCEL ORGANISATION (Task 5)");
        std::cout << "Records are organised by Parcel ID.\n"
                  << "  1. Display parcel IDs in sorted order\n"
                  << "  2. Search for a parcel in the tree\n"
                  << "  3. Register and insert a parcel\n"
                  << "  4. Edit parcel details\n"
                  << "  5. Update parcel status\n"
                  << "  6. Remove a parcel\n"
                  << "  0. Back\n";
        switch (readInt("Enter your choice (0-6): ", 0, 6)) {
            case 1: {
                printTitle("PARCEL TREE (SORTED BY PARCEL ID)");
                ResultCode result = displayParcelRecords(root);
                if (result == ResultCode::Empty) {
                    std::cout << "The parcel tree is empty.\n";
                } else if (result != ResultCode::Success) {
                    std::cout << describe(result) << "\n";
                } else {
                    std::cout << "\n";
                }
                break;
            }
            case 2: searchParcelTreeMenu(root);                  break;
            case 3: registerParcelMenu(registry, history, root); break;
            case 4: editDetailsMenu(registry, history);          break;
            case 5: updateStatusMenu(registry, history);         break;
            case 6: removeParcelMenu(registry, history, root);   break;
            case 0: return;
        }
        if (!synchronizeParcelTree(registry, root)) return;
        pressEnter();
    }
}

// ======================= Optional test data =======================

ResultCode seedDemoData(ParcelRegistry& registry, ResourceManager& resources,
                        ActivityHistory& history) {
    const struct {
        const char* name;
        const char* recipientID;
        const char* address;
        ParcelType type;
    } samples[] = {
        {"Demo Alice", "RC100001", "Block A", ParcelType::Document},
        {"Demo Bob", "RC100002", "Block B", ParcelType::Small},
        {"Demo Carol", "RC100003", "Block C", ParcelType::Large},
        {"Demo David", "RC100004", "Block D", ParcelType::Fragile},
        {"Demo Emma", "RC100005", "Block E", ParcelType::Small},
        {"Demo Farid", "RC100006", "Block F", ParcelType::Large},
        {"Demo Grace", "RC100007", "Library", ParcelType::Document},
        {"Demo Hana", "RC100008", "Block H", ParcelType::Small},
        {"Demo Ivan", "RC100009", "Block I", ParcelType::Fragile}
    };

    for (const auto& sample : samples) {
        Parcel p;
        p.recipientName = sample.name;
        p.recipientID = sample.recipientID;
        p.deliveryAddress = sample.address;
        p.type = sample.type;
        std::string id;
        ResultCode result = registry.registerParcel(p, id);
        if (result != ResultCode::Success) return result;
        history.recordRegistration(*registry.findParcel(id));
    }

    // Route the first seven in FIFO order; leave two waiting for processing.
    const RouteAction routes[] = {
        RouteAction::Default, RouteAction::Default, RouteAction::Default,
        RouteAction::Default, RouteAction::ReturnToSender,
        RouteAction::OnHold, RouteAction::Missing
    };
    for (RouteAction action : routes) {
        Parcel* p = registry.peekWaiting();
        if (p == nullptr) return ResultCode::Empty;
        ParcelStatus before = p->status;
        ResultCode result = registry.routeNextParcel(action);
        if (result != ResultCode::Success) return result;
        history.recordStatusChange(*p, before, ActivityType::Routed);
    }

    const struct {
        const char* name;
        ResourceType type;
    } demoResources[] = {
        {"Demo Counter A", ResourceType::CollectionCounter},
        {"Demo Counter B", ResourceType::CollectionCounter},
        {"Demo Courier A", ResourceType::DeliveryPersonnel},
        {"Demo Courier B", ResourceType::DeliveryPersonnel}
    };
    for (const auto& resource : demoResources) {
        std::string id;
        ResultCode result = resources.registerResource(resource.name, resource.type, id);
        if (result != ResultCode::Success) return result;
    }
    return ResultCode::Success;
}

// ======================= Main menu =======================

void printMenu() {
    std::cout << "REGISTRATION\n"
              << "   1. Register a parcel\n"
              << "RECORDS\n"
              << "   2. Display all parcels\n"
              << "   3. View a parcel by ID\n"
              << "   4. Search by status\n"
              << "   5. Search by recipient ID\n"
              << "PROCESSING\n"
              << "   6. Process next parcel\n"
              << "   7. Process all waiting parcels\n"
              << "UPDATES\n"
              << "   8. Update a parcel's status\n"
              << "   9. Edit parcel details\n"
              << "  10. Remove a parcel\n"
              << "ACTIVITY HISTORY\n"
              << "  11. Show recent activities\n"
              << "  12. Show a parcel's history\n"
              << "  13. Review a parcel's activities\n"
              << "  14. Undo an activity\n"
              << "SERVICES AND RESOURCES\n"
              << "  15. Service requests...\n"
              << "  16. Collection counters and couriers...\n"
              << "PARCEL ORGANISATION\n"
              << "  17. Parcel tree...\n"
              << "   0. Exit\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    bool seed = false;
    for (int i = 1; i < argc; ++i) {
        std::string option = argv[i];
        if (option == "--seed") {
            seed = true;
        } else if (option == "--help" || option == "-h") {
            std::cout << "Usage: " << argv[0] << " [--seed]\n"
                      << "  --seed  Start with demo parcels and resources for testing.\n"
                      << "          Demo data is recreated each run and is not saved.\n"
                      << "  --help  Show this help.\n";
            return 0;
        } else {
            std::cerr << "Unknown option: " << option << "\nUse --help for usage.\n";
            return 1;
        }
    }

    ParcelRegistry registry;
    static treeNode* root = nullptr;
    // Input exhaustion uses std::exit, so clean up the tree on that path too.
    std::atexit([] { clearParcelRecords(root); });
    ResourceManager resources(registry);
    // Resources are passed in so resource assignments can be undone too
    ActivityHistory history(registry, &resources);
    ServiceManager services(registry, resources, &history);
    if (seed) {
        ResultCode result = seedDemoData(registry, resources, history);
        if (result != ResultCode::Success) {
            std::cerr << "Could not load demo data. " << describe(result) << "\n";
            return 1;
        }
        if (!synchronizeParcelTree(registry, root)) {
            clearParcelRecords(root);
            return 1;
        }
        printTitle("TEST DATA LOADED");
        std::cout << "9 parcels and 4 available resources. No service requests created.\n"
                  << "Data is recreated on each --seed run; changes are not saved.\n"
                  << std::left << std::setw(12) << "Parcel ID"
                  << std::setw(12) << "Recipient ID" << "Status\n";
        registry.forEachParcel([](const Parcel& p) {
            std::cout << std::left << std::setw(12) << p.parcelID
                      << std::setw(12) << p.recipientID << statusToString(p.status) << "\n";
        });
    }
    bool running = true;

    while (running) {
        showDashboard(registry, services, resources);
        printMenu();

        switch (readInt("Enter your choice (0-17): ", 0, 17)) {
            case 1:  registerParcelMenu(registry, history, root); pressEnter(); break;
            case 2:  registry.displayAllParcels();          pressEnter(); break;
            case 3:  viewParcelMenu(registry);              pressEnter(); break;
            case 4:  searchByStatusMenu(registry);          pressEnter(); break;
            case 5:  searchByRecipientMenu(registry);       pressEnter(); break;
            case 6:  processNextMenu(registry, history);    pressEnter(); break;
            case 7:  processAllMenu(registry, history);     pressEnter(); break;
            case 8:  updateStatusMenu(registry, history);   pressEnter(); break;
            case 9:  editDetailsMenu(registry, history);    pressEnter(); break;
            case 10: removeParcelMenu(registry, history, root);   pressEnter(); break;
            case 11: recentActivitiesMenu(history);         pressEnter(); break;
            case 12: parcelHistoryMenu(history);            pressEnter(); break;
            case 13: reviewParcelMenu(history);             pressEnter(); break;
            case 14: undoMenu(history);                     pressEnter(); break;
            case 15: serviceMenu(services, registry, root);                     break;
            case 16: resourceMenu(resources, registry, history, root);          break;
            case 17: parcelTreeMenu(registry, history, root);                   break;
            case 0:
                std::cout << "Goodbye.\n";
                running = false;
                break;
        }
        if (!synchronizeParcelTree(registry, root)) {
            clearParcelRecords(root);
            return 1;
        }
    }
    clearParcelRecords(root);
    return 0;
}
