/* This file is part of the Palabos library.
 *
 * Copyright (C) 2011-2015 FlowKit Sarl
 * Route d'Oron 2
 * 1010 Lausanne, Switzerland
 * E-mail contact: contact@flowkit.com
 *
 * The most recent release of Palabos can be downloaded at 
 * <http://www.palabos.org/>
 *
 * The library Palabos is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * The library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/** \file
 * Wrapper functions that simplify the use of MPI
 */

#ifndef SEND_RECV_POOL_H
#define SEND_RECV_POOL_H

#include "core/globalDefs.h"
#include "core/util.h"
#include "parallelism/mpiManager.h"
#include <map>
#include <vector>
#include <sstream>

namespace plb {

#ifdef PLB_MPI_PARALLEL

/// This is a "write-only" storage for the communication between a pair of processors.
struct PoolEntry {
    PoolEntry()
        : cumDataLength(0), lengths()
    { }
    int cumDataLength;
    std::vector<int> lengths;
};

/// This is a "write-only" storage for the full communication pattern on a processor.
class SendRecvPool {
public:
    typedef std::map<int, PoolEntry> SubsT;
public:
    void subscribeMessage(int proc, int numData) {
        PoolEntry& entry = subscriptions[proc];
        entry.lengths.push_back(numData);
        entry.cumDataLength += numData;
    }
    void clear() {
        subscriptions.clear();
    }
    SubsT::const_iterator begin() const {
        //PLB_PRECONDITION( !subscriptions.empty() );
        return subscriptions.begin();
    }
    SubsT::const_iterator end() const {
        //PLB_PRECONDITION( !subscriptions.empty() );
        return subscriptions.end();
    }
    bool empty() const { return subscriptions.empty(); }
private:
    SubsT subscriptions;
};

/// This is a storage device for the communication between a pair of processors,
///   to be used in action.
struct CommunicatorEntry {
    CommunicatorEntry() 
        : lengths(),
          cumDataLength(0),
          messages(),
          data(),
          currentMessage(0)
    { } 
    CommunicatorEntry(PoolEntry const& poolEntry)
        : lengths(poolEntry.lengths),
          cumDataLength(poolEntry.cumDataLength),
          messages(lengths.size()),
          currentMessage(0)
    {
        for (pluint iMessage=0; iMessage<messages.size(); ++iMessage) {
            messages[iMessage].resize(lengths[iMessage]);
        }
    }
    void reset() {
        currentMessage=0;
    }
    std::string info() {
        std::stringstream infostr;
        for (pluint iL=0; iL<lengths.size(); ++iL) {
            int length = lengths[iL];
            PLB_ASSERT(length==(int)messages[iL].size());
            infostr << length << " ";
        }
        return infostr.str();
    }
    std::vector<int> lengths;
    int              cumDataLength;
    std::vector<std::vector<char> > messages;
    /// The variable data holds the message which in the end is being sent.
    ///   Having data here guarantees its persistence throughout the non-
    ///   blocking communication pattern and avoids unnecessery de- and re-
    ///   allocations.
    std::vector<char> data;
    /// If the data is dynamic, it must be sent and received piecewise,
    ///   and the individual sizes must be known. 
    ///   Having data here guarantees its persistence throughout the non-
    ///   blocking communication pattern and avoids unnecessery de- and re-
    ///   allocations.
    std::vector<int> dynamicDataSizes;
    int currentMessage;
    MPI_Request sizeRequest, messageRequest;
    MPI_Status  sizeStatus, messageStatus;
};

/// The "in-action" device for all messages sent from a processor.
class SendPoolCommunicator {
public:
    SendPoolCommunicator() { }
    SendPoolCommunicator(SendRecvPool const& pool);
    std::vector<char>& getSendBuffer(int toProc);
    void acceptMessage(int toProc, bool staticMessage);
    void finalize(bool staticMessage);
private:
    void startCommunication(int toProc, bool staticMessage);
private:
    std::map<int, CommunicatorEntry > subscriptions;
};

/// The "in-action" device for all messages received on a processor.
class RecvPoolCommunicator {
public:
    RecvPoolCommunicator() { }
    RecvPoolCommunicator(SendRecvPool const& pool);
    /// Initiate non-blocking communication.
    void startBeingReceptive(bool staticMessage);
    std::vector<char> const& receiveMessage(int fromProc, bool staticMessage);
private:
    void finalizeStatic(int fromProc);
    void receiveDynamic(int fromProc);
private:
    std::map<int, CommunicatorEntry > subscriptions;
};

#endif  // PLB_MPI_PARALLEL

}  // namespace plb

#endif  // SEND_RECV_POOL_H
