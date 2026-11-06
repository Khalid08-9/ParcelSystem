#ifndef INTERFACES_HPP
#define INTERFACES_HPP

#include <string>
#include "Parcel.hpp"
#include "ParcelTree.hpp"
#include "Result.hpp"
#include "ParcelRegistry.hpp"

// ---- Task 1: Parcel intake ----
// The controller creates ONE registry and passes it by reference to every module.
// Explicit registry arguments avoid hidden globals and separate copies of records.
inline ResultCode registerParcel(ParcelRegistry& registry, const Parcel& parcel,
                                 std::string& assignedID) {
    return registry.registerParcel(parcel, assignedID);
}
inline Parcel* findParcel(ParcelRegistry& registry, const std::string& parcelID) {
    return registry.findParcel(parcelID); // Borrowed pointer; never delete it.
}
inline ResultCode updateParcelStatus(ParcelRegistry& registry,
                                    const std::string& parcelID, ParcelStatus status) {
    return registry.updateParcelStatus(parcelID, status);
}
inline ResultCode routeNextParcel(ParcelRegistry& registry,
                                 RouteAction action = RouteAction::Default) {
    return registry.routeNextParcel(action);
}

// ---- Task 2: Service requests ----
// Tasks 2–4 below are proposed contracts, NOT implemented by Task 1.
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
// The tree functions are declared in ParcelTree.hpp, included above.

#endif
