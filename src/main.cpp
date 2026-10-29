

#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <limits>
#include <string>
#include "ParcelRegistry.hpp"

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

// Summary shown above the main menu
void showDashboard(const ParcelRegistry& registry) {
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

void registerParcelMenu(ParcelRegistry& registry) {
    printTitle("REGISTER A NEW PARCEL");
    std::cout << "The Parcel ID is created automatically.\n";

    Parcel p;
    p.recipientName   = readLine("Recipient name: ");
    p.recipientID     = readLine("Recipient ID: ");
    p.deliveryAddress = readLine("Delivery address: ");
    p.type            = readParcelType();

    std::string assignedID;                       // filled in by the registry
    ResultCode result = registry.registerParcel(p, assignedID);
    if (result == ResultCode::Success) {
        std::cout << "Parcel registered with ID " << assignedID
                  << " (status Registered, added to the waiting queue).\n";
    } else {
        std::cout << describe(result) << "\n";
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
void processNextMenu(ParcelRegistry& registry) {
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
    ResultCode result = registry.routeNextParcel(action);

    if (result == ResultCode::Success) {
        std::cout << "Parcel " << id << " is now: "
                  << statusToString(next->status) << "\n";
    } else {
        std::cout << describe(result) << "\n";
    }
}

// Routes every waiting parcel using the default rule
void processAllMenu(ParcelRegistry& registry) {
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
        ResultCode result = registry.routeNextParcel(RouteAction::Default);
        if (result != ResultCode::Success) {
            std::cout << describe(result) << " Stopping.\n";
            break;
        }
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

void updateStatusMenu(ParcelRegistry& registry) {
    printTitle("UPDATE PARCEL STATUS");

    std::string id = readLine("Parcel ID: ");
    ResultCode found = registry.viewParcel(id);   // shows the current details
    if (found != ResultCode::Success) {
        std::cout << describe(found) << "\n";
        return;
    }

    ParcelStatus current = registry.findParcel(id)->status;
    ParcelStatus newStatus = current;
    if (!readAllowedStatus(current, newStatus)) {
        std::cout << "Nothing was changed.\n";
        return;
    }

    ResultCode result = registry.updateParcelStatus(id, newStatus);
    if (result == ResultCode::Success) {
        std::cout << "Parcel " << id << " is now: "
                  << statusToString(newStatus) << "\n";
    } else {
        std::cout << describe(result) << "\n";
    }
}

// Edits recipient name, recipient ID and address; Enter keeps the old value
void editDetailsMenu(ParcelRegistry& registry) {
    printTitle("EDIT PARCEL DETAILS");

    std::string id = readLine("Parcel ID: ");
    ResultCode found = registry.viewParcel(id);   // shows the current details
    if (found != ResultCode::Success) {
        std::cout << describe(found) << "\n";
        return;
    }

    std::cout << "\nPress Enter to keep the current value.\n";
    std::string name    = readLine("New recipient name: ");
    std::string recID   = readLine("New recipient ID: ");
    std::string address = readLine("New delivery address: ");

    ResultCode result = registry.updateParcelDetails(id, name, recID, address);
    if (result == ResultCode::Success) {
        std::cout << "Details updated.\n";
        registry.viewParcel(id);
    } else if (result == ResultCode::InvalidInput) {
        std::cout << "Nothing to update: no new value was entered.\n";
    } else {
        std::cout << describe(result) << "\n";
    }
}

// Permanently removes a parcel after showing it and asking for confirmation
void removeParcelMenu(ParcelRegistry& registry) {
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

    ResultCode result = registry.removeParcel(id);
    if (result == ResultCode::Success) {
        std::cout << "Parcel " << id << " was removed.\n";
    } else if (result == ResultCode::InvalidStatus) {
        std::cout << "Error: this parcel is linked to a service request or resource "
                     "and cannot be removed. Cancel it instead.\n";
    } else {
        std::cout << describe(result) << "\n";
    }
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
              << "   0. Exit\n";
}

}  // namespace

int main() {
    ParcelRegistry registry;
    bool running = true;

    while (running) {
        showDashboard(registry);
        printMenu();

        switch (readInt("Enter your choice (0-10): ", 0, 10)) {
            case 1:  registerParcelMenu(registry);      pressEnter(); break;
            case 2:  registry.displayAllParcels();      pressEnter(); break;
            case 3:  viewParcelMenu(registry);          pressEnter(); break;
            case 4:  searchByStatusMenu(registry);      pressEnter(); break;
            case 5:  searchByRecipientMenu(registry);   pressEnter(); break;
            case 6:  processNextMenu(registry);         pressEnter(); break;
            case 7:  processAllMenu(registry);          pressEnter(); break;
            case 8:  updateStatusMenu(registry);        pressEnter(); break;
            case 9:  editDetailsMenu(registry);         pressEnter(); break;
            case 10: removeParcelMenu(registry);        pressEnter(); break;
            case 0:
                std::cout << "Goodbye.\n";
                running = false;
                break;
        }
    }
    return 0;
}