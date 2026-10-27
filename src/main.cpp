#include <iostream>
#include <limits>
#include <string>
#include "ParcelRegistry.hpp"

// Prints a prompt and reads a whole line (names and addresses contain spaces)
static std::string readLine(const std::string& prompt) {
    std::string text;
    std::cout << prompt;
    std::getline(std::cin, text);
    return text;
}

// Shows the parcel types as numbers and asks until a valid one is chosen
static ParcelType readParcelType() {
    int choice = 0;
    while (true) {
        std::cout << "Parcel type:\n"
                  << "  1. Document\n"
                  << "  2. Small\n"
                  << "  3. Large\n"
                  << "  4. Fragile\n"
                  << "Choose (1-4): ";

        if (std::cin >> choice && choice >= 1 && choice <= 4) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            switch (choice) {
                case 1: return ParcelType::Document;
                case 2: return ParcelType::Small;
                case 3: return ParcelType::Large;
                default: return ParcelType::Fragile;
            }
        }
        std::cin.clear();   // bad input: reset the stream and discard the line
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Invalid choice. Please enter a number from 1 to 4.\n\n";
    }
}

// Asks for the parcel details, builds a Parcel and tries to register it
static void registerParcelMenu(ParcelRegistry& registry) {
    Parcel p;
    std::cout << "\nREGISTER A NEW PARCEL\n";
    p.parcelID        = readLine("Parcel ID: ");
    p.recipientName   = readLine("Recipient name: ");
    p.recipientID     = readLine("Recipient ID: ");
    p.deliveryAddress = readLine("Delivery address: ");
    p.type            = readParcelType();

    ResultCode result = registry.registerParcel(p);

    switch (result) {
        case ResultCode::Success:
            std::cout << "Parcel " << p.parcelID
                      << " registered with status Registered.\n";
            break;
        case ResultCode::Duplicate:
            std::cout << "Error: a parcel with ID " << p.parcelID
                      << " already exists.\n";
            break;
        case ResultCode::InvalidInput:
            std::cout << "Error: the ID, recipient name, recipient ID and "
                         "address cannot be empty.\n";
            break;
        default:
            std::cout << "Registration failed.\n";
    }
}

int main() {
    ParcelRegistry registry;
    int choice = -1;

    while (choice != 0) {
        std::cout << "\nSMART CAMPUS PARCEL SYSTEM\n";
        std::cout << "==========================\n";
        std::cout << "1. Register a parcel\n";
        std::cout << "2. Display all parcels\n";
        std::cout << "0. Exit\n";
        std::cout << "Enter your choice: ";

        if (!(std::cin >> choice)) {            // the user typed a non-number
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid input.\n";
            choice = -1;
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (choice) {
            case 1: registerParcelMenu(registry); break;
            case 2: registry.displayAllParcels(); break;
            case 0: std::cout << "Goodbye.\n"; break;
            default: std::cout << "Invalid choice. Please try again.\n";
        }
    }
    return 0;
}