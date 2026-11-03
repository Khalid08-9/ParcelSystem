#ifndef RESULT_HPP
#define RESULT_HPP



enum  ResultCode {
    Success, NotFound, Duplicate, InvalidInput,
    InvalidStatus, Empty, AlreadyCompleted,
    Full // Task 3: no room for another resource
};

#endif