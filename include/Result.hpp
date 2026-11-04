#ifndef RESULT_HPP
#define RESULT_HPP



enum  ResultCode {
    Success, NotFound, Duplicate, InvalidInput,
    InvalidStatus, Empty, AlreadyCompleted,
    // Task 3: resources
    Full, AllResourcesOccupied, NoResourceAvailable, AlreadyAssigned,
    WrongResourceType, ResourceUnavailable, NotAssigned,
    // Task 4: activity history
    NotReversible, AlreadyReversed,
    // Task 2: service requests
    WrongRecipient, RequestCancelled, ServiceMismatch
};

#endif