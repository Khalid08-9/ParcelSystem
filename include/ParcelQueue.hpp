#ifndef PARCEL_QUEUE_HPP
#define PARCEL_QUEUE_HPP
#include "Parcel.hpp"


struct Queuenode 
{
    Parcel* parcel;
    Queuenode* next;   
};

class ParcelQueue
{
    private:
      Queuenode* front;
      Queuenode* rear;
      int size ;


    public:
      ParcelQueue();
      ~ParcelQueue();
      bool isEmpty() const;
      int getSize() const;
      void enqueue(Parcel* p);
      Parcel* dequeue(); 
      Parcel* peek() const; 
      void clear(); 
      bool remove(Parcel* p);   
    
};

#endif