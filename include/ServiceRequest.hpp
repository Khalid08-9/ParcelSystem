#ifndef SERVICE_REQUEST_HPP
#define SERVICE_REQUEST_HPP
#include <string>
#include <ctime>
#include "Status.hpp"
#include "Result.hpp"

// Task 2: Parcel Service and Collection Management
// Owner: [Name, TP number]
//
// One ServiceRequest is one service a registered parcel needs
// (collection, delivery, special handling, re-delivery or return).
// See SERVICE_RULES.md for the full rules.

// enum class (not plain enum) because ParcelStatus already uses the names
// SpecialHandling, ReturnToSender, Cancelled and Returned.
enum class ServiceType
{
    Collection,      // recipient collects at a counter
    CampusDelivery,  // courier delivers on campus
    SpecialHandling, // fragile / special care before the parcel moves on
    ReDelivery,      // a later delivery attempt after a failed one
    ReturnToSender   // parcel is sent back
};

enum class Urgency
{
    Normal,
    Urgent
};

enum class RequestStatus
{
    Pending,    // waiting in the priority list
    InProgress, // a resource is working on it
    Completed,  // service done (parcel collected, delivered, returned or released)
    Failed,     // delivery attempt failed (a re-delivery or return follows)
    Returned,   // closed because the parcel is being returned to sender
    Cancelled   // cancelled by staff
};

// What staff report when they finish an in-progress request
enum class RequestOutcome
{
    Completed,     // the service was carried out
    FailedAttempt, // delivery only: recipient not reached
    ReturnParcel   // recipient refused / wrong address: send the parcel back
};

// A delivery may be tried this many times before the parcel is returned
const int MAX_DELIVERY_ATTEMPTS = 3;

struct ServiceRequest
{
    std::string requestID;     // REQ001, REQ002, ...
    std::string parcelID;      // the Task 1 parcel this request is for
    std::string recipientName; // copied from the parcel when created
    std::string recipientID;
    std::string notes;         // recipient requirements / special instructions
    ServiceType type = ServiceType::Collection;
    Urgency urgency = Urgency::Normal;
    RequestStatus status = RequestStatus::Pending;
    int sequenceNo = 0;        // creation order: breaks ties between equal requests
    int attemptNo = 1;         // delivery attempt number (re-deliveries count up)
    std::time_t createdTime = 0;
    std::time_t closedTime = 0;
    std::string handledBy;     // resource that worked on it (kept after release)
    std::string outcomeNote;   // what happened when it was closed
    ServiceRequest* next = nullptr; // link in the ServiceManager's record list
};

std::string serviceTypeToString(ServiceType t);
std::string urgencyToString(Urgency u);
std::string requestStatusToString(RequestStatus s);

// Completed, Failed, Returned and Cancelled requests can never be processed again
bool isClosedRequest(RequestStatus s);

// Rank used by the priority rule: smaller number = handled sooner
int serviceTypeRank(ServiceType t);

// The priority rule (requirement 2.3). True if a must be handled before b:
//   1. Urgent before Normal
//   2. then by service type rank (Special > Re-delivery > Delivery > Return > Collection)
//   3. then the earlier request first (lower sequence number)
bool hasHigherPriority(const ServiceRequest& a, const ServiceRequest& b);

// The parcel status a service type needs (from Task 1 routing).
bool serviceMatchesParcel(ServiceType t, ParcelStatus parcelStatus);

// Message for the user after a service-request operation (requirement 2.5)
std::string serviceResultMessage(ResultCode code);

#endif
