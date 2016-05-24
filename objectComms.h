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


#ifndef SST_CORE_OBJECTCOMMS_H
#define SST_CORE_OBJECTCOMMS_H

#ifdef SST_CONFIG_HAVE_MPI
#include <mpi.h>
#endif

#include <boost/archive/polymorphic_binary_iarchive.hpp>
#include <boost/archive/polymorphic_binary_oarchive.hpp>
#include <boost/serialization/vector.hpp>

#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/back_inserter.hpp>

namespace SST {

namespace Comms {

template <typename dataType>
std::vector<char> serialize(dataType *data)
{
    std::vector<char> buffer;

    boost::iostreams::back_insert_device<std::vector<char> > inserter(buffer);
    boost::iostreams::stream<boost::iostreams::back_insert_device<std::vector<char> > > output_stream(inserter);
    boost::archive::polymorphic_binary_oarchive oa(output_stream, boost::archive::no_header);

    oa << data;
    output_stream.flush();

    return buffer;
}


template <typename dataType>
std::vector<char> serialize(dataType &data)
{
    std::vector<char> buffer;

    boost::iostreams::back_insert_device<std::vector<char> > inserter(buffer);
    boost::iostreams::stream<boost::iostreams::back_insert_device<std::vector<char> > > output_stream(inserter);
    boost::archive::polymorphic_binary_oarchive oa(output_stream, boost::archive::no_header);

    oa << data;
    output_stream.flush();

    return buffer;
}


template <typename dataType>
dataType* deserialize(std::vector<char> &buffer)
{
    dataType *tgt;

    boost::iostreams::basic_array_source<char> source(buffer.data(), buffer.size());
    boost::iostreams::stream<boost::iostreams::basic_array_source <char> > input_stream(source);
    boost::archive::polymorphic_binary_iarchive ia(input_stream, boost::archive::no_header );

    ia >> tgt;

    return tgt;
}

template <typename dataType>
void deserialize(std::vector<char> &buffer, dataType &tgt)
{
    boost::iostreams::basic_array_source<char> source(buffer.data(), buffer.size());
    boost::iostreams::stream<boost::iostreams::basic_array_source <char> > input_stream(source);
    boost::archive::polymorphic_binary_iarchive ia(input_stream, boost::archive::no_header );

    ia >> tgt;
}

template <typename dataType>
void deserialize(char *buffer, int blen, dataType &tgt)
{
    boost::iostreams::basic_array_source<char> source(buffer, blen);
    boost::iostreams::stream<boost::iostreams::basic_array_source <char> > input_stream(source);
    boost::archive::polymorphic_binary_iarchive ia(input_stream, boost::archive::no_header );

    ia >> tgt;
}



#ifdef SST_CONFIG_HAVE_MPI
template <typename dataType>
void broadcast(dataType& data, int root) {
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if ( root == rank ) {
        // Serialize the data
        std::vector<char> buffer = Comms::serialize(data);

        // Now broadcast the size of the data
        int size = buffer.size();
        MPI_Bcast(&size, 1, MPI_INT, root, MPI_COMM_WORLD);

        // Now broadcast the data
        MPI_Bcast(buffer.data(), buffer.size(), MPI_BYTE, root, MPI_COMM_WORLD);
    }
    else {
        // Get the size of the broadcast
        int size = 0;
        MPI_Bcast(&size, 1, MPI_INT, root, MPI_COMM_WORLD);

        // Now get the data
        char* buffer = new char[size];
        MPI_Bcast(buffer, size, MPI_BYTE, root, MPI_COMM_WORLD);

        // Now deserialize data
        Comms::deserialize(buffer, size, data);
    }
}

template <typename dataType>
void send(int dest, int tag, dataType& data) {
    // Serialize the data
    std::vector<char> buffer = Comms::serialize(data);

    // Now send the data.  Send size first, then payload
    // std::cout<< sizeof(buffer.size()) << std::endl;
    int64_t size = buffer.size();
    MPI_Send(&size, 1, MPI_INT64_T, dest, tag, MPI_COMM_WORLD);

    int32_t fragment_size = 1000000000;
    int64_t offset = 0;

    while ( size >= fragment_size ) {
        MPI_Send(buffer.data() + offset, fragment_size, MPI_BYTE, dest, tag, MPI_COMM_WORLD);
        size -= fragment_size;
        offset += fragment_size;
    }
    MPI_Send(buffer.data() + offset, size, MPI_BYTE, dest, tag, MPI_COMM_WORLD);
}

template <typename dataType>
void recv(int src, int tag, dataType& data) {
    // Get the size of the broadcast
    int64_t size = 0;
    MPI_Status status;
    MPI_Recv(&size, 1, MPI_INT64_T, src, tag, MPI_COMM_WORLD, &status);

    // Now get the data
    char* buffer = new char[size];
    int64_t offset = 0;
    int32_t fragment_size = 1000000000;
    int64_t rem_size = size;

    while ( rem_size >= fragment_size ) {
        MPI_Recv(buffer + offset, fragment_size, MPI_BYTE, src, tag, MPI_COMM_WORLD, &status);
        rem_size -= fragment_size;
        offset += fragment_size;
    }
    MPI_Recv(buffer + offset, rem_size, MPI_BYTE, src, tag, MPI_COMM_WORLD, &status);


    // Now deserialize data
    Comms::deserialize(buffer, size, data);
}


template <typename dataType>
void all_gather(const dataType& data, std::vector<dataType> &out_data) {
    int rank = 0, world = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world);

    // Serialize the data
    std::vector<char> buffer = Comms::serialize(data);


    size_t sendSize = buffer.size();
    int allSizes[world];
    int displ[world];

    memset(allSizes, '\0', world * sizeof(int));
    memset(displ, '\0', world * sizeof(int));

    MPI_Allgather(&sendSize, sizeof(int), MPI_BYTE,
            &allSizes, sizeof(int), MPI_BYTE, MPI_COMM_WORLD);

    int totalBuf = 0;
    for ( int i = 0 ; i < world ; i++ ) {
        totalBuf += allSizes[i];
        if ( i > 0 )
            displ[i] = displ[i-1] + allSizes[i-1];
    }

    char *bigBuff = new char[totalBuf];

    MPI_Allgatherv(buffer.data(), buffer.size(), MPI_BYTE,
            bigBuff, allSizes, displ, MPI_BYTE, MPI_COMM_WORLD);

    out_data.resize(world);
    for ( int i = 0 ; i < world ; i++ ) {
        Comms::deserialize(&bigBuff[displ[i]], allSizes[i], out_data[i]);
    }

    delete [] bigBuff;

}




#endif

}

} //namespace SST

#endif // SST_CORE_OBJECTCOMMS_H
