#include "ParcelRegistry.hpp"
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