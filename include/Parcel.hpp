#ifndef PARCEL_HPP
#define PARCEL_HPP
#include <string> 
#include <ctime>
#include "Status.hpp"

enum  ParcelType {
    Document,
    Small,
    Large,
    Fragile
};

struct Parcel{
std::string parcelID;
std::string recipientName;
std::string  recipientID;
std::string deliveryAddress;
ParcelType type;
ParcelStatus status ;
std::time_t arrivalTime = 0;
std::string requestID;
std::string resourceID;
Parcel* next=nullptr;
};


#endif
