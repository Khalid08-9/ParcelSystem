

#include "ServiceManager.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>

namespace
{

    bool isBlank(const std::string &text)
    {
        return text.find_first_not_of(" \t\r\n") == std::string::npos;
    }

    std::string formatTime(std::time_t t)
    {
        if (t == 0)
            return "-";
        char buffer[20];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", std::localtime(&t));
        return buffer;
    }

    bool isDelivery(ServiceType t)
    {
        return t == ServiceType::CampusDelivery || t == ServiceType::ReDelivery;
    }

} // namespace

ServiceManager::ServiceManager(ParcelRegistry &registry, ResourceManager &resources,
                               ActivityHistory *history)
    : registry(registry), resources(resources), history(history),
      head(nullptr), tail(nullptr), size(0), nextRequestNumber(1), nextSequenceNo(1) {}

ServiceManager::~ServiceManager()
{
    pending.clear(); // list nodes only; the requests are deleted below
    while (head != nullptr)
    {
        ServiceRequest *old = head;
        head = head->next;
        delete old;
    }
}

// ======================= Internal helpers =======================

std::string ServiceManager::generateRequestID()
{
    std::ostringstream id;
    id << "REQ" << std::setw(3) << std::setfill('0') << nextRequestNumber++;
    return id.str(); // REQ001, REQ002, ...
}

// Creates a request, appends it to the records, puts it in the priority list
// and links it to the parcel. Callers validate first.
ServiceRequest *ServiceManager::addRequest(Parcel &p, ServiceType type, Urgency urgency,
                                           const std::string &notes, int attemptNo)
{
    ServiceRequest *r = new ServiceRequest();
    r->requestID = generateRequestID();
    r->parcelID = p.parcelID;
    r->recipientName = p.recipientName;
    r->recipientID = p.recipientID;
    r->notes = notes;
    r->type = type;
    r->urgency = urgency;
    r->sequenceNo = nextSequenceNo++;
    r->attemptNo = attemptNo;
    r->createdTime = std::time(nullptr);

    if (tail == nullptr)
        head = r;
    else
        tail->next = r; // O(1) append
    tail = r;
    size++;

    pending.insert(r); // sorted insert by priority
    p.requestID = r->requestID;
    return r;
}

int ServiceManager::failedDeliveries(const std::string &parcelID) const
{
    int count = 0;
    for (ServiceRequest *r = head; r != nullptr; r = r->next)
        if (r->parcelID == parcelID && isDelivery(r->type) && r->status == RequestStatus::Failed)
            count++;
    return count;
}

// The parcel's resourceID is the single source of truth: Task 3 may have
// handed the job to another resource, or Task 4 may have undone it.
void ServiceManager::releaseResourceOf(Parcel &p)
{
    if (p.resourceID.empty())
        return;
    std::string resourceID = p.resourceID;
    if (resources.releaseResource(resourceID) == ResultCode::Success && history != nullptr)
        history->recordResourceReleased(p, resourceID);
}

// Caller already checked canTransition, so this succeeds
void ServiceManager::changeParcelStatus(Parcel &p, ParcelStatus newStatus)
{
    ParcelStatus before = p.status;
    if (registry.updateParcelStatus(p.parcelID, newStatus) == ResultCode::Success && history != nullptr)
        history->recordStatusChange(p, before);
}

void ServiceManager::closeRequest(ServiceRequest &r, Parcel *p, RequestStatus status,
                                  const std::string &note)
{
    pending.remove(r.requestID); // no-op unless it was still pending
    r.status = status;
    r.closedTime = std::time(nullptr);
    r.outcomeNote = note;
    if (p != nullptr && p->requestID == r.requestID)
        p->requestID = ""; // parcel is free for a new request
}

// Shared checks: the request exists and is still open (Pending or InProgress)
ResultCode ServiceManager::checkOpen(const std::string &requestID, ServiceRequest *&r) const
{
    if (isBlank(requestID))
        return ResultCode::InvalidInput;
    r = findRequest(requestID);
    if (r == nullptr)
        return ResultCode::NotFound;
    if (r->status == RequestStatus::Cancelled)
        return ResultCode::RequestCancelled;
    if (isClosedRequest(r->status))
        return ResultCode::AlreadyCompleted;
    return ResultCode::Success;
}

// ======================= Requirement 1: create =======================

ResultCode ServiceManager::createRequest(const std::string &parcelID, const std::string &recipientID,
                                         ServiceType type, Urgency urgency,
                                         const std::string &notes, std::string &assignedID)
{
    if (isBlank(parcelID) || isBlank(recipientID))
        return ResultCode::InvalidInput;
    Parcel *p = registry.findParcel(parcelID);
    if (p == nullptr)
        return ResultCode::NotFound;
    if (isFinalStatus(p->status))
        return ResultCode::AlreadyCompleted;
    if (!p->requestID.empty())
        return ResultCode::Duplicate; // one active request per parcel
    if (recipientID != p->recipientID)
        return ResultCode::WrongRecipient;
    if (!serviceMatchesParcel(type, p->status))
        return ResultCode::ServiceMismatch;

    int attemptNo = 1;
    if (isDelivery(type))
    {
        int failed = failedDeliveries(parcelID);
        if (type == ServiceType::ReDelivery && failed == 0)
        {
            return ResultCode::ServiceMismatch; // re-delivery needs a failed attempt
        }
        attemptNo = failed + 1;
        if (attemptNo > MAX_DELIVERY_ATTEMPTS)
        {
            return ResultCode::InvalidStatus; // must be returned to sender now
        }
    }
    assignedID = addRequest(*p, type, urgency, notes, attemptNo)->requestID;
    return ResultCode::Success;
}

// ======================= Requirements 2 and 3: maintain and display =======================

ServiceRequest *ServiceManager::findRequest(const std::string &requestID) const
{
    for (ServiceRequest *r = head; r != nullptr; r = r->next)
    {
        if (r->requestID == requestID)
            return r;
    }
    return nullptr;
}

void ServiceManager::printTableHeader() const
{
    std::cout << std::left << std::setw(5) << "Pos" << std::setw(8) << "Request"
              << std::setw(9) << "Parcel" << std::setw(18) << "Service"
              << std::setw(8) << "Urgency" << std::setw(13) << "Status"
              << std::setw(5) << "Try" << std::setw(9) << "Resource" << "Recipient\n"
              << std::string(90, '-') << "\n";
}

void ServiceManager::printRow(const ServiceRequest &r, int position) const
{
    std::string resource = r.handledBy.empty() ? "-" : r.handledBy;
    if (r.status == RequestStatus::InProgress)
    { // show who has it right now
        Parcel *p = registry.findParcel(r.parcelID);
        resource = (p != nullptr && !p->resourceID.empty()) ? p->resourceID : "(none)";
    }
    std::cout << std::left
              << std::setw(5) << (position > 0 ? std::to_string(position) : "-")
              << std::setw(8) << r.requestID << std::setw(9) << r.parcelID
              << std::setw(18) << serviceTypeToString(r.type)
              << std::setw(8) << urgencyToString(r.urgency)
              << std::setw(13) << requestStatusToString(r.status)
              << std::setw(5) << r.attemptNo << std::setw(9) << resource
              << r.recipientName << "\n";
}

ResultCode ServiceManager::displayActive() const
{
    int inProgress = countByStatus(RequestStatus::InProgress);
    if (pending.isEmpty() && inProgress == 0)
        return ResultCode::Empty;

    std::cout << "\nPENDING (in the order they will be handled)\n";
    printTableHeader();
    if (pending.isEmpty())
        std::cout << "  (none)\n";
    pending.forEach([this](const ServiceRequest &r, int position)
                    { printRow(r, position); });

    std::cout << "\nIN PROGRESS\n";
    printTableHeader();
    if (inProgress == 0)
        std::cout << "  (none)\n";
    for (ServiceRequest *r = head; r != nullptr; r = r->next)
        if (r->status == RequestStatus::InProgress)
            printRow(*r, 0);
    return ResultCode::Success;
}

void ServiceManager::displayAll() const
{
    if (head == nullptr)
    {
        std::cout << "No service requests have been created yet.\n";
        return;
    }
    std::cout << "\nALL SERVICE REQUESTS (oldest first, Pos = place in pending list)\n";
    printTableHeader();
    for (ServiceRequest *r = head; r != nullptr; r = r->next)
        printRow(*r, pending.positionOf(r->requestID));
    std::cout << "Total: " << size << "\n";
}

ResultCode ServiceManager::viewRequest(const std::string &requestID) const
{
    if (isBlank(requestID))
        return ResultCode::InvalidInput;
    ServiceRequest *r = findRequest(requestID);
    if (r == nullptr)
        return ResultCode::NotFound;
    Parcel *p = registry.findParcel(r->parcelID);
    int position = pending.positionOf(r->requestID);

    std::cout << "\nRequest ID:       " << r->requestID << "\n"
              << "Parcel ID:        " << r->parcelID
              << (p ? " (parcel status: " + statusToString(p->status) + ")" : "") << "\n"
              << "Recipient:        " << r->recipientName << " (" << r->recipientID << ")\n"
              << "Service:          " << serviceTypeToString(r->type) << "\n"
              << "Urgency:          " << urgencyToString(r->urgency) << "\n"
              << "Status:           " << requestStatusToString(r->status) << "\n";
    if (position > 0)
        std::cout << "Queue position:   " << position << " of " << pending.getSize() << "\n";
    if (isDelivery(r->type))
        std::cout << "Delivery attempt: " << r->attemptNo << " of " << MAX_DELIVERY_ATTEMPTS << "\n";
    if (r->status == RequestStatus::InProgress && p != nullptr)
        std::cout << "Resource:         " << (p->resourceID.empty() ? "None" : p->resourceID) << "\n";
    else if (!r->handledBy.empty())
        std::cout << "Handled by:       " << r->handledBy << "\n";
    std::cout << "Notes:            " << (r->notes.empty() ? "-" : r->notes) << "\n"
              << "Created:          " << formatTime(r->createdTime) << "\n"
              << "Closed:           " << formatTime(r->closedTime) << "\n";
    if (!r->outcomeNote.empty())
        std::cout << "Outcome:          " << r->outcomeNote << "\n";
    return ResultCode::Success;
}

// ======================= Requirement 4: process =======================

ResultCode ServiceManager::startNextRequest(std::string &startedID, std::string &resourceID)
{
    ServiceRequest *next = pending.peek();
    if (next == nullptr)
        return ResultCode::Empty;
    startedID = next->requestID;
    return startRequest(startedID, resourceID);
}

// Starts a pending request, or resumes an in-progress one that lost its resource
ResultCode ServiceManager::startRequest(const std::string &requestID, std::string &resourceID)
{
    ServiceRequest *r = nullptr;
    ResultCode result = checkOpen(requestID, r);
    if (result != ResultCode::Success)
        return result;
    Parcel *p = registry.findParcel(r->parcelID);
    if (p == nullptr)
        return ResultCode::NotFound;
    if (r->status == RequestStatus::InProgress && !p->resourceID.empty())
    {
        return ResultCode::InvalidStatus; // already started
    }
    if (isFinalStatus(p->status))
        return ResultCode::AlreadyCompleted;
    // Re-check: the parcel may have been re-routed or undone since creation
    if (!serviceMatchesParcel(r->type, p->status))
        return ResultCode::ServiceMismatch;

    // No resource yet: Task 3 picks the next free one of the right type.
    // (If staff assigned one from the Task 3 menu, that one is used.)
    if (p->resourceID.empty())
    {
        std::string assigned;
        result = resources.assignNextAvailable(p->parcelID, r->requestID, assigned);
        if (result != ResultCode::Success)
            return result; // request keeps its place
        if (history != nullptr)
            history->recordResourceAssigned(*p);
    }

    pending.remove(r->requestID); // no-op when resuming
    r->status = RequestStatus::InProgress;
    r->handledBy = resourceID = p->resourceID;
    return ResultCode::Success;
}

ResultCode ServiceManager::completeRequest(const std::string &requestID, RequestOutcome outcome,
                                           const std::string &receiverID,
                                           ParcelStatus afterSpecial, std::string &followUpID)
{
    followUpID = "";
    ServiceRequest *r = nullptr;
    ResultCode result = checkOpen(requestID, r);
    if (result != ResultCode::Success)
        return result;
    if (r->status != RequestStatus::InProgress)
        return ResultCode::InvalidStatus; // start it first
    Parcel *p = registry.findParcel(r->parcelID);
    if (p == nullptr)
        return ResultCode::NotFound;
    if (isFinalStatus(p->status))
        return ResultCode::AlreadyCompleted;
    if (p->resourceID.empty())
        return ResultCode::NotAssigned; // nobody is handling it

    // ---- Step 1: work out the result, without changing anything ----
    bool changeStatus = true; // false only for a failed attempt with tries left
    ParcelStatus target = p->status;
    RequestStatus closeAs = RequestStatus::Completed;
    bool makeFollowUp = false;
    ServiceType followUpType = ServiceType::ReturnToSender;
    int followUpAttempt = 1;
    std::string note;

    if (outcome == RequestOutcome::Completed)
    {
        if (r->type == ServiceType::SpecialHandling)
        {
            if (afterSpecial != ParcelStatus::ReadyForCollection &&
                afterSpecial != ParcelStatus::OutForDelivery)
            {
                return ResultCode::InvalidInput;
            }
            target = afterSpecial;
            makeFollowUp = true;
            followUpType = (target == ParcelStatus::ReadyForCollection)
                               ? ServiceType::Collection
                               : ServiceType::CampusDelivery;
            note = "Special handling done; sent to " + statusToString(target);
        }
        else if (r->type == ServiceType::ReturnToSender)
        {
            target = ParcelStatus::Returned;
            note = "Returned to sender";
        }
        else
        { // Collection or delivery: check who receives it
            if (isBlank(receiverID))
                return ResultCode::InvalidInput;
            if (receiverID != p->recipientID)
                return ResultCode::WrongRecipient;
            bool collect = (r->type == ServiceType::Collection);
            target = collect ? ParcelStatus::Collected : ParcelStatus::Delivered;
            note = (collect ? "Collected by " : "Delivered to ") + receiverID;
        }
    }
    else if (outcome == RequestOutcome::FailedAttempt)
    {
        if (!isDelivery(r->type))
            return ResultCode::InvalidStatus; // only deliveries fail
        bool lastTry = r->attemptNo >= MAX_DELIVERY_ATTEMPTS;
        closeAs = RequestStatus::Failed;
        makeFollowUp = true;
        changeStatus = lastTry; // out of attempts: send the parcel back
        if (lastTry)
        {
            target = ParcelStatus::ReturnToSender;
        }
        else
        {
            followUpType = ServiceType::ReDelivery;
            followUpAttempt = r->attemptNo + 1;
        }
        note = "Attempt " + std::to_string(r->attemptNo) + " failed; " +
               (lastTry ? "maximum reached, parcel returned to sender" : "re-delivery scheduled");
    }
    else
    { // ReturnParcel: refused / wrong details
        if (r->type == ServiceType::ReturnToSender)
            return ResultCode::InvalidStatus;
        closeAs = RequestStatus::Returned;
        target = ParcelStatus::ReturnToSender;
        makeFollowUp = true;
        note = "Closed: parcel to be returned to sender";
    }

    // Task 1's rule decides whether the parcel may move
    if (changeStatus && !canTransition(p->status, target))
        return ResultCode::InvalidStatus;

    // ---- Step 2: all checks passed, now change things ----
    releaseResourceOf(*p);
    if (changeStatus)
        changeParcelStatus(*p, target);
    closeRequest(*r, p, closeAs, note);
    if (makeFollowUp)
    {
        followUpID = addRequest(*p, followUpType, r->urgency,
                                "Follow-up of " + r->requestID, followUpAttempt)
                         ->requestID;
    }
    return ResultCode::Success;
}

ResultCode ServiceManager::cancelRequest(const std::string &requestID, const std::string &reason)
{
    ServiceRequest *r = nullptr;
    ResultCode result = checkOpen(requestID, r);
    if (result != ResultCode::Success)
        return result;
    Parcel *p = registry.findParcel(r->parcelID);
    if (p != nullptr && r->status == RequestStatus::InProgress)
    {
        releaseResourceOf(*p);
    }
    // The parcel keeps its status, so staff can create a different request
    closeRequest(*r, p, RequestStatus::Cancelled,
                 isBlank(reason) ? "Cancelled" : "Cancelled: " + reason);
    return ResultCode::Success;
}

int ServiceManager::countByStatus(RequestStatus s) const
{
    int count = 0;
    for (ServiceRequest *r = head; r != nullptr; r = r->next)
        if (r->status == s)
            count++;
    return count;
}
