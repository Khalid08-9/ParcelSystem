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
   
};

#endif 
