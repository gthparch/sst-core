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

#ifndef SST_CORE_STOPACTION_H
#define SST_CORE_STOPACTION_H

#include <sst/core/serialization.h>

#include <iostream>

#include <sst/core/action.h>
#include <sst/core/output.h>

namespace SST {

/**
 * Action which causes the Simulation to end
 */
class StopAction : public Action
{
private:

    std::string message;
    bool print_message;
    
public:
    StopAction() {
        setPriority(STOPACTIONPRIORITY);
        print_message = false;
    }

    /** Create a new StopAction which includes a message to be printed when it fires
     */
    StopAction(std::string msg) {
        setPriority(STOPACTIONPRIORITY);
        print_message = true;
        message = msg;
    }

    void execute() {
        if ( print_message ) {
            Output::getDefaultObject().output("%s\n", message.c_str());
        }
        endSimulation();
    }

    void print(const std::string &header, Output &out) const {
        out.output("%s StopAction to be delivered at %" PRIu64 "\n", header.c_str(), getDeliveryTime());
    }

private:
    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Action);
        ar & BOOST_SERIALIZATION_NVP(message);
        ar & BOOST_SERIALIZATION_NVP(print_message);
    }
};

} // namespace SST

#endif //SST_CORE_STOPACTION_H
