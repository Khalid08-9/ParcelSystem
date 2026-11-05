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
    ParcelQueue* queueFor(ParcelStatus s); 
    int nextParcelNumber;
    std::string generateParcelID();
    int size;

public:
    ParcelRegistry();
    ~ParcelRegistry();
    // This class owns its parcels and queues; sharing requires a reference, not a copy.
    ParcelRegistry(const ParcelRegistry&) = delete;
    ParcelRegistry& operator=(const ParcelRegistry&) = delete;

    ResultCode registerParcel(const Parcel& newParcel);
    ResultCode registerParcel(const Parcel& newParcel, std::string& assignedID);
 Parcel* findParcel(const std::string& parcelID) const;
    int  getSize() const;
    bool isEmpty() const;
    void clear();
    void displayAllParcels() const ;
    ResultCode routeNextParcel(RouteAction action);
    ResultCode viewParcel(const std::string& parcelID) const;
    ResultCode updateParcelStatus(const std::string& id, ParcelStatus newStatus);
    ResultCode updateParcelDetails(const std::string& id,
                               const std::string& newName,
                               const std::string& newRecipientID,
                               const std::string& newAddress);
    ResultCode removeParcel(const std::string& id);
    ResultCode searchByStatus(ParcelStatus status) const;
    ResultCode searchByRecipientID(const std::string& recipientID) const;
    Parcel* peekWaiting() const;
    // Read-only handoff: status updates, not consumers, remove parcels from queues.
    const Parcel* peekNextParcel(ParcelStatus status) const;
    int waitingCount() const;
    int collectionCount() const;
    int deliveryCount() const;
    int specialCount() const;
    int returnCount() const;
    int holdCount() const;
   
};

#endif 
