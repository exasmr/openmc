#ifndef OPENMC_SHARED_ARRAY_H
#define OPENMC_SHARED_ARRAY_H

//! \file shared_array.h
//! \brief Shared array data structure

#include <memory>

#include "openmc/device_alloc.h"

namespace openmc {

//==============================================================================
// Class declarations
//==============================================================================

// This container is an array that is capable of being appended to in an
// thread safe manner by use of atomics. It only provides protection for the
// use cases currently present in OpenMC. Namely, it covers the scenario where
// multiple threads are appending to an array, but no threads are reading from
// or operating on it in any other way at the same time. Multiple threads can
// call the thread_safe_append() function concurrently and store data to the
// object at the index returned from thread_safe_append() safely, but no other
// operations are protected.
template <typename T> 
class SharedArray { 

public: 
  //==========================================================================
  // Constructors

  //! Default constructor.
  SharedArray() = default;

  // Note: the destructor is not defined due to OpenMP offloading
  // restrictions.  The problem is that since the SharedArray is a
  // global variable that is accessed beyond the local scope of a target
  // construct (i.e., from within a function in a target region), the
  // variable must be marked as "declare target". When doing so with a
  // global variable a copy is constructed in place on device and will
  // therefore also be destructed in place on device (which involves
  // dynamic memory usage as well as calling delete on a mapped host
  // pointer).
  /*
  ~SharedArray()
  {
    this->clear();
  }
  */

  //! Construct a zero size container with space to hold capacity number of
  //! elements.
  //
  //! \param capacity The number of elements for the container to allocate
  //! space for
  SharedArray(int capacity) : capacity_(capacity)
  {
    data_ = new T[capacity];
  }

  //==========================================================================
  // Methods and Accessors

  //! Return a reference to the element at specified location i. No bounds
  //! checking is performed.
  T& operator[](int i) {return data_[i];}
  const T& operator[](int i) const { return data_[i]; }

  //! Allocate space in the container for the specified number of elements.
  //! reserve() does not change the size of the container.
  //
  //! \param capacity The number of elements to allocate in the container
  void reserve(int capacity)
  {
    if (capacity <= capacity_) {
      return;
    }
    data_ = new T[capacity];
    capacity_ = capacity;
  }

  //! Increase the size of the container by one and append value to the 
  //! array. Returns an index to the element of the array written to. Also
  //! tests to enforce that the append operation does not read off the end
  //! of the array. In the event that this does happen, set the size to be
  //! equal to the capacity and return -1.
  //
  //! \value The value of the element to append
  //! \return The index in the array written to. In the event that this
  //! index would be greater than what was allocated for the container,
  //! return -1.
  int thread_safe_append(const T& value)
  {
    // Atomically capture the index we want to write to
    int idx;
    // NOTE: The seq_cst is required for correctness but is not yet
    // well supported on device
    #pragma omp atomic capture //seq_cst
    idx = size_++;

    // Check that we haven't written off the end of the array
    if (idx >= capacity_) {
      // NOTE: The seq_cst is required for correctness but is not yet
      // well supported on device
      #pragma omp atomic write //seq_cst
      size_ = capacity_;
      return -1;
    }

    // Copy element value to the array
    data_[idx] = value;

    return idx;
  }

  //! Return the number of elements in the container
  int size() {return size_;}
  
  void sync_size_host_to_device()
  {
    #pragma omp target update to(size_)
  }
  
  void sync_size_device_to_host()
  {
    #pragma omp target update from(size_)
  }

  //! Resize the container to contain a specified number of elements. This is
  //! useful in cases where the container is written to in a non-thread safe manner,
  //! where the internal size of the array needs to be manually updated.
  //
  //! \param size The new size of the container
  void resize(int size)
  {
    size_ = size;
    sync_size_host_to_device();
  }

  //! Return the number of elements that the container has currently allocated
  //! space for.
  int capacity() {return capacity_;}

  //! Return pointer to the underlying array serving as element storage.
  T* data() {return data_;}
  const T* data() const {return data_;}
  
  //! Return data pointer to the underlying array serving as element storage.
  T* device_data() {return device_data_;}
  const T* device_data() const {return device_data_;}

  //! Map data pointer to device but do not copy data from host to device
  void allocate_on_device()
  {
    #pragma omp target update to(size_)
    #pragma omp target update to(capacity_)
    #pragma omp target enter data map(alloc: data_[:capacity_])
    
    // Determine mapped device pointer
    // May be useful for device library interop (e.g., sorting via Thrust)
    #pragma omp target data use_device_ptr(data_)
    {
      device_data_ = data_;
    }
    
    // If OpenMP 5.1 is fully supported, we can more simply just do:
    //device_data_ = static_cast<T*>(omp_get_mapped_ptr(data_, omp_get_default_device()));
  }

  //! Free allocated memory on device
  void free_on_device()
  {
    #pragma omp target exit data map(delete: data_[:capacity_])
  }

  void copy_host_to_device()
  {
    #pragma omp target update to(size_)
    #pragma omp target update to(data_[:capacity_])
  }
  
  void copy_device_to_host()
  {
    #pragma omp target update from(data_[:capacity_])
    #pragma omp target update from(size_)
  }

  //! Free any space that was allocated for the container. Set the
  //! container's size and capacity to 0.
  void clear()
  {
    if( data_ != nullptr )
    {
      free_on_device();
      delete[] data_;
      data_ = nullptr;
    }
    size_ = 0;
    capacity_ = 0;
  }
  
  //==========================================================================
  // Data members

  //private: 

  T* data_ {nullptr}; //!< An RAII handle to the elements
  T* device_data_ {nullptr}; //!< Device pointer for interop with device libraries
  int size_ {0}; //!< The current number of elements 
  int capacity_ {0}; //!< The total space allocated for elements
}; 

} // namespace openmc

#endif // OPENMC_SHARED_ARRAY_H
