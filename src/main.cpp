// src/main.cpp
// Console menu for the Smart Campus Parcel System (Task 1 + integration shell).
//
// Rules for this file:
//  - It only reads input, prints messages and calls the registry.
//  - It never touches head, tail or the queues directly.
//  - Every ResultCode is turned into a message in ONE place (describe).
//
// Needs these public members in ParcelRegistry:
//   registerParcel(parcel, assignedID), findParcel, viewParcel, displayAllParcels,
//   routeNextParcel, updateParcelStatus,
//   getSize, peekWaiting, waitingCount, collectionCount, deliveryCount,
//   specialCount, returnCount, holdCount

#include <iostream>
#include <iomanip>
#include <limits>
#include <string>
#include "ParcelRegistry.hpp"

namespace {

// ======================= Input helpers =======================

// Prints a prompt and reads a whole line (names and addresses contain spaces)
std::string readLine(const std::string& prompt) {
    std::string text;
    std::cout << prompt;
    std::getline(std::cin, text);
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
    std::getline(std::cin, ignored);
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

// ======================= Records =======================

void viewParcelMenu(const ParcelRegistry& registry) {
    printTitle("VIEW A PARCEL");

    std::string id = readLine("Parcel ID: ");
    ResultCode result = registry.viewParcel(id);
    if (result != ResultCode::Success) {
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

// ======================= Update =======================

// Lists ONLY the statuses this parcel may move to (checked with canTransition),
// and returns the one chosen; returns false if there is nothing to choose or
// the user cancels.
bool readStatus(ParcelStatus current, ParcelStatus& chosen) {
    const ParcelStatus all[] = {
        ParcelStatus::Registered,     ParcelStatus::ReadyForCollection,
        ParcelStatus::OutForDelivery, ParcelStatus::SpecialHandling,
        ParcelStatus::ReturnToSender, ParcelStatus::OnHold,
        ParcelStatus::Missing,        ParcelStatus::Collected,
        ParcelStatus::Delivered,      ParcelStatus::Returned,
        ParcelStatus::Cancelled
    };
    const int total = 11;

    ParcelStatus allowed[11];
    int count = 0;
    for (int i = 0; i < total; ++i) {
        if (canTransition(current, all[i])) {
            allowed[count++] = all[i];
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
    if (!readStatus(current, newStatus)) {
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

// ======================= Main menu =======================

void printMenu() {
    std::cout << "REGISTRATION\n"
              << "  1. Register a parcel\n"
              << "RECORDS\n"
              << "  2. Display all parcels\n"
              << "  3. View a parcel by ID\n"
              << "PROCESSING\n"
              << "  4. Process next parcel\n"
              << "  5. Process all waiting parcels\n"
              << "UPDATES\n"
              << "  6. Update a parcel's status\n"
              << "  0. Exit\n";
}

}  // namespace

int main() {
    ParcelRegistry registry;
    bool running = true;

    while (running) {
        showDashboard(registry);
        printMenu();

        switch (readInt("Enter your choice (0-6): ", 0, 6)) {
            case 1: registerParcelMenu(registry);   pressEnter(); break;
            case 2: registry.displayAllParcels();   pressEnter(); break;
            case 3: viewParcelMenu(registry);       pressEnter(); break;
            case 4: processNextMenu(registry);      pressEnter(); break;
            case 5: processAllMenu(registry);       pressEnter(); break;
            case 6: updateStatusMenu(registry);     pressEnter(); break;
            case 0:
                std::cout << "Goodbye.\n";
                running = false;
                break;
        }
    }
    return 0;
}