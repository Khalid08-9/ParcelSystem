#include "ParcelRegistry.hpp"
#include <Parcel.hpp>
#include <iostream>
#include <string>
#include <iomanip>
#include <ctime>

ParcelRegistry::ParcelRegistry() : head(nullptr), tail(nullptr), size(0) {}

static bool isBlank(const std::string& text) {
    return text.find_first_not_of(" \t\r\n") == std::string::npos;
}

ParcelRegistry::~ParcelRegistry() {
    clear();
}

bool ParcelRegistry::isEmpty() const {
    return head == nullptr;

}

int ParcelRegistry::getSize() const {
    return size;
}

Parcel* ParcelRegistry::findParcel(const std::string& parcelID) const {
    Parcel* current = head;
    while (current != nullptr) {
        if (current->parcelID == parcelID)  {
            return current;
        }
        current = current->next;
    }
    return nullptr;
}



void ParcelRegistry::clear() {
    waiting.clear();
    collectionQueue.clear();
    deliveryQueue.clear();
    specialQueue.clear();
    returnQueue.clear();
    holdQueue.clear();
    while (head != nullptr) {
        Parcel* temp = head;
        head = head->next;
        delete temp;
    }
    tail = nullptr;
    size = 0;
}


ResultCode ParcelRegistry::registerParcel(const Parcel& newParcel) {
    if (isBlank(newParcel.parcelID) ||
        isBlank(newParcel.recipientName) ||
        isBlank(newParcel.recipientID) ||
        isBlank(newParcel.deliveryAddress)) {
        return ResultCode::InvalidInput;
    }

    if (findParcel(newParcel.parcelID) != nullptr) {
        return ResultCode::Duplicate;
    }

    Parcel* parcelCopy = new Parcel(newParcel);
    parcelCopy->status = ParcelStatus::Registered;
    parcelCopy->arrivalTime = std::time(nullptr);
    parcelCopy->requestID = "";
    parcelCopy->resourceID = "";
    parcelCopy->next = nullptr;


    if (head == nullptr) {
        head = parcelCopy;
        tail = parcelCopy;
    } else {
        tail->next = parcelCopy;
        tail = parcelCopy;
    }

    size++;
    waiting.enqueue(parcelCopy);
    return ResultCode::Success;
}

static std::string formatTime(std::time_t t) {
    char buffer[20];
    std::tm* local = std::localtime(&t);
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", local);
    return std::string(buffer);
}

void ParcelRegistry::displayAllParcels() const {
    if (head == nullptr) {
        std::cout << "No parcels registered.\n";
        return;
    }

    std::cout << std::left
              << std::setw(12) << "Parcel ID"
              << std::setw(20) << "Recipient Name"
              << std::setw(12) << "Recipient ID"
              << std::setw(22) << "Status"
              << "Arrival Time\n";
    std::cout << std::string(80, '-') << "\n";

    Parcel* current = head;
    while (current != nullptr) {
        std::cout << std::left
                  << std::setw(12) << current->parcelID
                  << std::setw(20) << current->recipientName
                  << std::setw(12) << current->recipientID
                  << std::setw(22) << statusToString(current->status)
                  << formatTime(current->arrivalTime) << "\n";
        current = current->next;
    }

    std::cout << std::string(80, '-') << "\n";
    std::cout << "Total parcels: " << size << "\n";
}

ResultCode ParcelRegistry::routeNextParcel(RouteAction action) {
    if (waiting.isEmpty()) {
        return ResultCode::Empty;
    }

    Parcel* p = waiting.peek();            // look only, do not remove yet

    if (isFinalStatus(p->status)) {
        return ResultCode::AlreadyCompleted;
    }

    // Work out the new status
    ParcelStatus newStatus = p->status;
    switch (action) {
        case RouteAction::ReturnToSender:
            newStatus = ParcelStatus::ReturnToSender;
            break;
        case RouteAction::OnHold:
            newStatus = ParcelStatus::OnHold;
            break;
        case RouteAction::Missing:
            newStatus = ParcelStatus::Missing;
            break;
        case RouteAction::Default:
        default:
            switch (p->type) {
                case ParcelType::Document:
                case ParcelType::Small:
                    newStatus = ParcelStatus::ReadyForCollection;
                    break;
                case ParcelType::Large:
                    newStatus = ParcelStatus::OutForDelivery;
                    break;
                case ParcelType::Fragile:
                    newStatus = ParcelStatus::SpecialHandling;
                    break;
            }
            break;
    }

    // Check the rule. If it fails, nothing has changed.
    if (!canTransition(p->status, newStatus)) {
        return ResultCode::InvalidStatus;
    }

    // Everything is valid, so now change things
    waiting.dequeue();
    p->status = newStatus;

    switch (newStatus) {
        case ParcelStatus::ReadyForCollection: collectionQueue.enqueue(p); break;
        case ParcelStatus::OutForDelivery:     deliveryQueue.enqueue(p);   break;
        case ParcelStatus::SpecialHandling:    specialQueue.enqueue(p);    break;
        case ParcelStatus::ReturnToSender:     returnQueue.enqueue(p);     break;
        case ParcelStatus::OnHold:             holdQueue.enqueue(p);       break;
        default: break;   // Missing: stays in the registry only
    }
// Missing: stays in the registry only
    return ResultCode::Success;
}


ResultCode ParcelRegistry::viewParcel(const std::string& parcelID) const {
    if (isBlank(parcelID)) {
        return ResultCode::InvalidInput;
    }

    Parcel* p = findParcel(parcelID);
    if (p == nullptr) {
        return ResultCode::NotFound;
    }

    std::cout << "\nPARCEL DETAILS\n"
              << std::string(40, '-') << "\n"
              << "Parcel ID:        " << p->parcelID << "\n"
              << "Recipient name:   " << p->recipientName << "\n"
              << "Recipient ID:     " << p->recipientID << "\n"
              << "Delivery address: " << p->deliveryAddress << "\n"
              << "Type:             " << typeToString(p->type) << "\n"
              << "Status:           " << statusToString(p->status) << "\n"
              << "Arrival time:     " << formatTime(p->arrivalTime) << "\n"
              << "Request ID:       " << (p->requestID.empty() ? "None" : p->requestID) << "\n"
              << "Resource ID:      " << (p->resourceID.empty() ? "None" : p->resourceID) << "\n"
              << std::string(40, '-') << "\n";

    return ResultCode::Success;
}

Parcel* ParcelRegistry::peekWaiting() const {
    return waiting.peek();
}

int ParcelRegistry::waitingCount() const {
    return waiting.getSize();
}

int ParcelRegistry::collectionCount() const {
    return collectionQueue.getSize();
}

int ParcelRegistry::deliveryCount() const {
    return deliveryQueue.getSize();
}

int ParcelRegistry::specialCount() const {
    return specialQueue.getSize();
}

int ParcelRegistry::returnCount() const {
    return returnQueue.getSize();
}

int ParcelRegistry::holdCount() const {
    return holdQueue.getSize();
}