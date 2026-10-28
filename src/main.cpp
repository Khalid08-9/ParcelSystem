

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

    Parcel p;
    p.parcelID        = readLine("Parcel ID: ");
    p.recipientName   = readLine("Recipient name: ");
    p.recipientID     = readLine("Recipient ID: ");
    p.deliveryAddress = readLine("Delivery address: ");
    p.type            = readParcelType();

    ResultCode result = registry.registerParcel(p);
    if (result == ResultCode::Success) {
        std::cout << "Parcel " << p.parcelID
                  << " registered with status Registered and added to the waiting queue.\n";
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
              << "  0. Exit\n";
}

}  // namespace

int main() {
    ParcelRegistry registry;
    bool running = true;

    while (running) {
        showDashboard(registry);
        printMenu();

        switch (readInt("Enter your choice (0-5): ", 0, 5)) {
            case 1: registerParcelMenu(registry);   pressEnter(); break;
            case 2: registry.displayAllParcels();   pressEnter(); break;
            case 3: viewParcelMenu(registry);       pressEnter(); break;
            case 4: processNextMenu(registry);      pressEnter(); break;
            case 5: processAllMenu(registry);       pressEnter(); break;
            case 0:
                std::cout << "Goodbye.\n";
                running = false;
                break;
        }
    }
    return 0;
}