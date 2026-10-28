#ifndef INTERFACES_HPP
#define INTERFACES_HPP

#include <string>
#include "Parcel.hpp"
#include "Result.hpp"

// ---- Task 1: Parcel intake ----
ResultCode registerParcel(const Parcel& newParcel);
Parcel*    findParcel(const std::string& parcelID);   // nullptr if not found
ResultCode updateParcelStatus(const std::string& parcelID, ParcelStatus newStatus);
ResultCode routeNextParcel();

// ---- Task 2: Service requests ----
ResultCode createRequest(const std::string& parcelID, const std::string& serviceType);
ResultCode processNextRequest();
ResultCode cancelRequest(const std::string& requestID);

// ---- Task 3: Resources ----
ResultCode assignResource(const std::string& requestID);
ResultCode releaseResource(const std::string& resourceID);
ResultCode setResourceStatus(const std::string& resourceID, bool available);

// ---- Task 4: Activity history ----
void       recordActivity(const std::string& parcelID, const std::string& action);
void       showHistory(const std::string& parcelID);
ResultCode undoLastActivity();

// ---- Task 5: Parcel tree ----
ResultCode treeInsert(const Parcel& p);
Parcel*    treeSearch(const std::string& key);
ResultCode treeRemove(const std::string& key);
void       treeDisplay();

#endif