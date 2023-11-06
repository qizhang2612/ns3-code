/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007, 2014 University of Washington
 *               2015 Universita' degli Studi di Napoli Federico II
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Guangyu Peng
 */

#ifndef SB_FIFO_QUEUE_DISC_H
#define SB_FIFO_QUEUE_DISC_H

#include "ns3/queue-disc.h"
#include "ns3/uinteger.h"

namespace ns3
{

/**
 * \ingroup traffic-control
 */
class SbFifoQueueDisc : public QueueDisc
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId(void);
    /**
     * \brief SbFifoQueueDisc constructor.
     *
     * Creates a FIFO queues with 1024 packets for each
     * by default.
     */
    SbFifoQueueDisc();

    virtual ~SbFifoQueueDisc();
    /**
     * \brief Set the pointer of used buffer size, the pointer must be the same
     * for all SbFifoQueueDiscs in a Node, so that SbFifoQueueDisc runs properly.
     * \param usedBufferPtr the pointer to be set
     */
    void SetUsedBufferPtr(Ptr<UintegerValue> usedBufferPtr);

    // Reasons for dropping packets
    static constexpr const char* DT_EXCEEDED_DROP = "Dynamic Threshold limit exceeded";
    // Reasons for mark
    static constexpr const char* ECN_MARK = "Ecn threshold exceeded";

  private:
    virtual bool DoEnqueue(Ptr<QueueDiscItem> item) override;
    virtual Ptr<QueueDiscItem> DoDequeue(void) override;
    virtual Ptr<const QueueDiscItem> DoPeek(void) override;
    virtual bool CheckConfig(void) override;
    virtual void InitializeParams(void) override;

    uint64_t m_sharedBufferSize; // bytes
    int32_t m_dtAlphaExp;        // alpha = 2^(dtAlphaExp)
    bool m_useEcn;
    uint32_t m_ecnThreshold;

    Ptr<UintegerValue> m_usedBufferPtr; // point to used buffer size in a node
};

} // namespace ns3

#endif // SB_FIFO_QUEUE_DISC_H
