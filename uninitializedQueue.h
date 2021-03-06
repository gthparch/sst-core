// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_CORE_UNINITIALIZEDQUEUE_H
#define SST_CORE_UNINITIALIZEDQUEUE_H

#include <sst/core/serialization.h>

#include <sst/core/activityQueue.h>

namespace SST {

/** Always unitialized queue
 * @brief Used for debugging, and preventing accidentally sending messages
 * into an incorrect queue
 */
class UninitializedQueue : public ActivityQueue {
public:
    /** Create a new Queue
     * @param message - Message to print when something attempts to use this Queue
     */
    UninitializedQueue(std::string message);
    UninitializedQueue(); // Only used for serialization
    ~UninitializedQueue();

    bool empty();
    int size();
    void insert(Activity* activity);
    Activity* pop();
    Activity* front();


private:
    std::string message;

    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(ActivityQueue);
        ar & BOOST_SERIALIZATION_NVP(message);
    }
};

} //namespace SST

BOOST_CLASS_EXPORT_KEY(SST::UninitializedQueue)

#endif // SST_CORE_UNINITIALIZEDQUEUE_H
