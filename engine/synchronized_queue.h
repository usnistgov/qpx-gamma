/*******************************************************************************
 *
 * This software was developed at the National Institute of Standards and
 * Technology (NIST) by employees of the Federal Government in the course
 * of their official duties. Pursuant to title 17 Section 105 of the
 * United States Code, this software is not subject to copyright protection
 * and is in the public domain. NIST assumes no responsibility whatsoever for
 * its use by other parties, and makes no guarantees, expressed or implied,
 * about its quality, reliability, or any other characteristic.
 *
 * Author(s):
 *      Martin Shetty (NIST)
 *
 * Description:
 *      Thread-safe queue.
 *
 ******************************************************************************/

#pragma once

#include <queue>	
#include <boost/thread.hpp>

template <typename T>
class SynchronizedQueue
{
public:
  inline SynchronizedQueue() : end_queue_(false) {}
  
  inline void enqueue(const T& data)
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    
    queue_.push(data);
    
    cond_.notify_one();
  }
  
  inline T dequeue()
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    
    while (queue_.empty() && !end_queue_)
      cond_.wait(lock);
    
    if (end_queue_)
      return NULL;
    
    T result = queue_.front();
    queue_.pop();
    
    return result;
  }
  
  inline void stop()
  {
    end_queue_ = true;
    cond_.notify_all();        
  }

  inline uint32_t size()
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    return queue_.size();
  }
  
private:
  bool end_queue_;
  std::queue<T> queue_;
  boost::mutex mutex_;
  boost::condition_variable cond_;
};
