// -*- mode: c++ -*-
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
//

#ifndef CORE_INTERFACES_SIMPLEMEM_H_
#define CORE_INTERFACES_SIMPLEMEM_H_

#include <string>
#include <utility>
#include <map>
#include <atomic>

#include <sst/core/sst_types.h>
#include <sst/core/subcomponent.h>
#include <sst/core/params.h>
#include <sst/core/link.h>

namespace SST {

class Component;
class Event;

namespace Interfaces {

/**
 * Simplified, generic interface to Memory models
 */
class SimpleMem : public SubComponent {

public:
    /** All Addresses can be 64-bit */
    typedef uint64_t Addr;

    /**
     * Represents both memory requests and responses.
     */
    class Request {
    public:
        typedef uint64_t id_t;      /*!< Request ID type */
        typedef uint32_t flags_t;   /*!< Flag type */

        /**
         * Commands and responses possible with a Request object
         */
        typedef enum {
            Read,       /*!< Issue a Read from Memory */
            Write,      /*!< Issue a Write to Memory */
            ReadResp,   /*!< Response from Memory to a Read */
            WriteResp   /*!< Response from Memory to a Write */
        } Command;

        /**
         * Flags to specify conditions on a Request
         */
        typedef enum {
            F_NONCACHEABLE  = 1<<1,     /*!< This request should not be cached */
            F_LOCKED        = 1<<2,     /*!< This request should be locked.  A LOCKED read should be soon followed by a LOCKED write (to unlock) */
            F_LLSC          = 1<<3,
            F_LLSC_RESP     = 1<<4
        } Flags;

        /** Type of the payload or data */
        typedef std::vector<uint8_t> dataVec;

        Command cmd;        /*!< Command to issue */
        Addr addr;          /*!< Target address */
        size_t size;        /*!< Size of this request or response */
        dataVec data;       /*!< Payload data (for Write, or ReadResp) */
        flags_t flags;      /*!< Flags associated with this request or response */
        flags_t memFlags;   /*!< Memory flags - ignored by caches except to be passed through with request to main memory */
        id_t id;            /*!< Unique ID to identify responses with requests */
        uint32_t groupId;   /* Group Id.  Used to maintain group-based stats in MH */
	Addr instrPtr;      /*!< Instruction pointer associated with the operation */
        Addr virtualAddr;   /*!< Virtual address associated with the operation */

        /** Constructor */
        Request(Command cmd, Addr addr, size_t size, dataVec &data, flags_t flags = 0, flags_t memFlags = 0) :
            cmd(cmd), addr(addr), size(size), data(data), flags(flags), memFlags(memFlags), groupId(0),
		instrPtr(0), virtualAddr(0)
        {
            id = main_id++;
        }

        /** Constructor */
        Request(Command cmd, Addr addr, size_t size, flags_t flags = 0, flags_t memFlags = 0) :
            cmd(cmd), addr(addr), size(size), flags(flags), memFlags(memFlags), groupId(0),
		instrPtr(0), virtualAddr(0)
        {
            id = main_id++;
        }

        /**
         * Set Stats Group Id
         */
        void setGroupId(uint32_t _groupId)
        {
            groupId = _groupId;
        }

        /**
         * Set the contents of the payload / data field.
         */
        void setPayload(const std::vector<uint8_t> & data_in )
        {
            data = data_in;
        }

        /**
         * Set the contents of the payload / data field.
         */
        void setPayload(uint8_t *data_in, size_t len)
        {
            data.resize(len);
            for ( size_t i = 0 ; i < len ; i++ ) {
                data[i] = data_in[i];
            }
        }

	/**
	* Set the virtual address associated with the operation
	*/
  	void setVirtualAddress(const Addr newVA) {
		virtualAddr = newVA;
	}

	/**
	* Get the virtual address associated with the operation
	*/
	uint64_t getVirtualAddress() {
		return (uint64_t) virtualAddr;
	}

	/*
	* Sets the instruction pointer associated with the operation
	*/
	void setInstructionPointer(const Addr newIP) {
		instrPtr = newIP;
	}

	/**
	* Sets the instruction pointer associated with the operation
	*/
	Addr getInstructionPointer() {
		return instrPtr;
	}

    private:
        static std::atomic<id_t> main_id;
    };

    /** Functor classes for Clock handling */
    class HandlerBase {
    public:
        /** Function called when Handler is invoked */
        virtual void operator()(Request*) = 0;
        virtual ~HandlerBase() {}
    };


    /** Event Handler class with user-data argument
     * @tparam classT Type of the Object
     * @tparam argT Type of the argument
     */
    template <typename classT, typename argT = void>
    class Handler : public HandlerBase {
    private:
        typedef void (classT::*PtrMember)(Request*, argT);
        classT* object;
        const PtrMember member;
        argT data;

    public:
        /** Constructor
         * @param object - Pointer to Object upon which to call the handler
         * @param member - Member function to call as the handler
         * @param data - Additional argument to pass to handler
         */
        Handler( classT* const object, PtrMember member, argT data ) :
            object(object),
            member(member),
            data(data)
        {}

        void operator()(Request* req) {
            return (object->*member)(req,data);
        }
    };

    /** Event Handler class without user-data
     * @tparam classT Type of the Object
     */
    template <typename classT>
    class Handler<classT, void> : public HandlerBase {
    private:
        typedef void (classT::*PtrMember)(Request*);
        classT* object;
        const PtrMember member;

    public:
        /** Constructor
         * @param object - Pointer to Object upon which to call the handler
         * @param member - Member function to call as the handler
         */
        Handler( classT* const object, PtrMember member ) :
            object(object),
            member(member)
        {}

        void operator()(Request* req) {
            return (object->*member)(req);
        }
    };


    /** Constructor, designed to be used via 'loadSubComponent'. */
    SimpleMem(SST::Component *comp, Params &params) :
        SubComponent(comp)
        { }

    /** Second half of building the interface.
     * Intialize with link name name, and handler, if any
     * @return true if the link was able to be configured.
     */
    virtual bool initialize(const std::string &linkName, HandlerBase *handler = NULL) = 0;

    /**
     * Sends a memory-based request during the init() phase
     */
    virtual void sendInitData(Request *req) = 0;

    /**
     * Sends a generic Event during the init() phase
     * (Mostly acts as a passthrough)
     * @see SST::Link::sendInitData()
     */
    virtual void sendInitData(SST::Event *ev) { getLink()->sendInitData(ev); }

    /**
     * Receive any data during the init() phase.
     * @see SST::Link::recvInitData()
     */
    virtual SST::Event* recvInitData() { return getLink()->recvInitData(); }

    /**
     * Returns a handle to the underlying SST::Link
     */
    virtual SST::Link* getLink(void) const = 0;

    /**
     * Send a Request to the other side of the link.
     */
    virtual void sendRequest(Request *req) = 0;

    /**
     * Receive a Request response from the side of the link.
     *
     * Use this method for polling-based applications.
     * Register a handler for push-based notification of responses.
     *
     * @return NULL if nothing is available.
     * @return Pointer to a Request response (that should be deleted)
     */
    virtual Request* recvResponse(void) = 0;


};

}
}

#endif
