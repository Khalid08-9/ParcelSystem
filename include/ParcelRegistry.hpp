#ifndef PARCELREGISTRY_HPP
#define PARCELREGISTRY_HPP
#include <string>
#include "Parcel.hpp"
#include "ParcelQueue.hpp"
#include "Result.hpp"  


class ParcelRegistry{
private:
    Parcel* head;
    Parcel* tail;
    ParcelQueue waiting;
    ParcelQueue collectionQueue;
    ParcelQueue deliveryQueue;
    ParcelQueue specialQueue;
    ParcelQueue returnQueue;
    ParcelQueue holdQueue;
    int size;

public:
    ParcelRegistry();
    ~ParcelRegistry();

    ResultCode registerParcel(const Parcel& newParcel);
    Parcel* findParcel(const std::string& parcelID) const;
    int  getSize() const;
    bool isEmpty() const;
    void clear();
    void displayAllParcels() const ;
    ResultCode routeNextParcel(RouteAction action);
    ResultCode viewParcel(const std::string& parcelID) const;
    Parcel* peekWaiting() const;
    int waitingCount() const;
    int collectionCount() const;
    int deliveryCount() const;
    int specialCount() const;
    int returnCount() const;
    int holdCount() const;
   
};

#endif 
