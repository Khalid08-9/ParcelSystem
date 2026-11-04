#ifndef SERVICE_MANAGER_HPP
#define SERVICE_MANAGER_HPP
#include <string>
#include "ServiceRequest.hpp"
#include "RequestPriorityList.hpp"
#include "ParcelRegistry.hpp"
#include "ResourceManager.hpp"
#include "ActivityHistory.hpp"
#include "Result.hpp"

// Task 2: Parcel Service and Collection Management
// Owner: [Name, TP number]
//
// Storage:
//  - every request ever created lives in a singly linked RECORD list
//    (head/tail, oldest first) and is owned by this class
//  - PENDING requests are also in a sorted linked list (RequestPriorityList),
//    so the head is always the next request to handle
//
// Life cycle:
//   createRequest -> Pending -> startRequest -> InProgress -> completeRequest
//                                                      -> Completed / Failed / Returned
//   cancelRequest works on Pending or InProgress requests.
//
// Integration:
//  - Task 1: the parcel must exist and match the service; parcel.requestID is
//    set while a request is active; parcel status changes go through
//    ParcelRegistry::updateParcelStatus (which checks canTransition)
//  - Task 3: starting a request takes the next available resource of the right
//    type; finishing or cancelling releases it
//  - Task 4: every resource and parcel-status change is recorded in the history
//
// Every operation validates first and only then changes anything, so a
// failed operation leaves the system unchanged.

class ServiceManager
{
    private:
      ParcelRegistry& registry;
      ResourceManager& resources;
      ActivityHistory* history;   // optional; nullptr = do not record

      ServiceRequest* head;       // record list, oldest first
      ServiceRequest* tail;
      int size;
      RequestPriorityList pending;
      int nextRequestNumber;
      int nextSequenceNo;

      std::string generateRequestID();
      // Builds a request, adds it to the records and the pending list and
      // links it to the parcel. No validation: callers check first.
      ServiceRequest* addRequest(Parcel& p, ServiceType type, Urgency urgency,
                                 const std::string& notes, int attemptNo);
      int failedDeliveries(const std::string& parcelID) const;
      void releaseResourceOf(Parcel& p);   // frees the parcel's resource, if any
      void changeParcelStatus(Parcel& p, ParcelStatus newStatus);
      void closeRequest(ServiceRequest& r, Parcel* p, RequestStatus status,
                        const std::string& note);
      // Common checks for start / complete / cancel
      ResultCode checkOpen(const std::string& requestID, ServiceRequest*& r) const;
      void printRow(const ServiceRequest& r, int position) const;
      void printTableHeader() const;

    public:
      ServiceManager(ParcelRegistry& registry, ResourceManager& resources,
                     ActivityHistory* history = nullptr);
      ~ServiceManager();
      ServiceManager(const ServiceManager&) = delete;
      ServiceManager& operator=(const ServiceManager&) = delete;

      // ---- Requirement 1: create ----
      // recipientID is what staff were given; it must match the parcel.
      ResultCode createRequest(const std::string& parcelID, const std::string& recipientID,
                               ServiceType type, Urgency urgency, const std::string& notes,
                               std::string& assignedID);

      // ---- Requirements 2 and 3: maintain and choose the next request ----
      ServiceRequest* peekNext() const { return pending.peek(); } // nullptr when none pending
      ResultCode displayActive() const;            // Empty when nothing is active
      void displayAll() const;                     // every request incl. closed ones
      ResultCode viewRequest(const std::string& requestID) const;
      ServiceRequest* findRequest(const std::string& requestID) const;

      // ---- Requirement 4: process and update ----
      // Starts the highest-priority pending request; reports which and who took it
      ResultCode startNextRequest(std::string& startedID, std::string& resourceID);
      // Starts a request chosen by staff (may skip the queue order)
      ResultCode startRequest(const std::string& requestID, std::string& resourceID);
      // Finishes an in-progress request.
      //  receiverID   : ID shown by the person collecting / receiving (Collection,
      //                 Campus Delivery, Re-delivery); checked against the parcel
      //  afterSpecial : ReadyForCollection or OutForDelivery (Special Handling only)
      //  followUpID   : filled when a follow-up request is created automatically
      ResultCode completeRequest(const std::string& requestID, RequestOutcome outcome,
                                 const std::string& receiverID, ParcelStatus afterSpecial,
                                 std::string& followUpID);
      ResultCode cancelRequest(const std::string& requestID, const std::string& reason);

      // ---- Counters for the dashboard ----
      int getSize() const { return size; }
      int pendingCount() const { return pending.getSize(); }
      int countByStatus(RequestStatus s) const;
};

#endif
