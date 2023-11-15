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
 * Author: Qi Zhang
 */

#include "dt-queue-disc.h"
#include "ns3/log.h"
#include "ns3/object-factory.h"
#include "ns3/queue.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("DtFifoQueueDisc");

NS_OBJECT_ENSURE_REGISTERED(DtFifoQueueDisc);

TypeId
DtFifoQueueDisc::GetTypeId(void){
    static TypeId tid = 
        TypeId("ns3::DtFifoQueueDisc")
            .SetParent<QueueDisc>()
            .SetGroupName("TrafficControl")
            .AddConstructor<DtFifoQueueDisc>()
            .AddAttribute("Maxsize",
                          "The maximum number of packets accepted by each queue.",
                          QueueSizeValue(QueueSize("1024p")),
                          MakeQueueSizeAccessor(&QueueDisc::SetMaxSize,&QueueDisc::GetMaxSize),
                          MakeQueueSizeChecker())
            .AddAttribute("SharedBufferSize",
                          "Total bytes of shared buffer.",
                          UintegerValue(204800),
                          MakeUintegerAccessor(&DtFifoQueueDisc::m_sharedBufferSize),
                          MakeUintegerChecker<uint64_t>())
            .AddAttribute("DtAlphaExp",
                          "Alpha exp number of Dynamic Threshold.",
                          IntegerValue(0),
                          MakeIntegerAccessor(&DtFifoQueueDisc::m_dtAlphaExp),
                          MakeIntegerChecker<int32_t>());
//            .AddAttribute("UseEcn",
//                          "Flag to enable/disable ecn.",
//                          BooleanValue(false),
//                          MakeBooleanAccessor(&SbFifoQueueDisc::m_useEcn),
//                          MakeBooleanChecker())
//            .AddAttribute("EcnThreshold",
//                          "Threshold of ecn in bytes.",
//                          UintegerValue(102400),
//                          MakeUintegerAccessor(&SbFifoQueueDisc::m_ecnThreshold),
//                          MakeUintegerChecker<uint32_t>());
    return tid;
}

DtFifoQueueDisc::DtFifoQueueDisc()
    : QueueDisc(QueueDiscSizePolicy::SINGLE_INTERNAL_QUEUE)
{
    NS_LOG_FUNCTION(this);
}

DtFifoQueueDisc::~DtFifoQueueDisc()
{
    NS_LOG_FUNCTION(this);
}

void
DtFifoQueueDisc::SetUsedBufferPtr(Ptr<UintegerValue> usedBufferPtr)
{
    NS_LOG_FUNCTION(this << usedBufferPtr);
    m_usedBufferPtr = usedBufferPtr;
}

void
DtFifoQueueDisc::SetAlphaExp(int32_t AlphaExp){
    NS_LOG_FUNCTION(this << AlphaExp);
    m_dtAlphaExp = AlphaExp;
}

bool
DtFifoQueueDisc::DoEnqueue(Ptr<QueueDiscItem> item){
    NS_LOG_FUNCTION(this << item);
    bool retval;

    // Get threshold T
    uint64_t availBuffer = m_sharedBufferSize - m_usedBufferPtr->Get();
    uint64_t T = 
        (m_dtAlphaExp >= 0) ? (availBuffer << m_dtAlphaExp) : (availBuffer >> (-m_dtAlphaExp));
    uint64_t cursize = GetInternalQueue(0)->GetNBytes();// Get first queue length(bytes)
    if(cursize + item->GetSize() > T ||
        m_usedBufferPtr->Get() + item->GetSize() > m_sharedBufferSize){
        NS_LOG_LOGIC("Dynamic Threshold limit exceeded -- dropping packet");
        DropBeforeEnqueue(item,DT_EXCEEDED_DROP);
        return false;
    }else{
        retval = GetInternalQueue(0)->Enqueue(item);
        if(retval){
            m_usedBufferPtr->Set(m_usedBufferPtr->Get() + item->GetSize());
        }
    }

    return retval;
}

Ptr<QueueDiscItem>
DtFifoQueueDisc::DoDequeue(void){
    NS_LOG_FUNCTION(this);

    Ptr<QueueDiscItem> item;
    if((item = GetInternalQueue(0)->Dequeue()) != nullptr){
        // update used buffer
        m_usedBufferPtr->Set(m_usedBufferPtr->Get() - item->GetSize());
//        uint64_t curSize = GetInternalQueue(0)->GetNBytes();
//        if (m_useEcn && curSize > m_ecnThreshold)
//        {
//            bool marked = Mark(item, ECN_MARK);
//            if (!marked)
//            {
//                NS_LOG_WARN("SBFIFO Mark issue!");
//            }
//        }
        return item;
    }
    NS_LOG_LOGIC("Queue empty");
    return item;
}

Ptr<const QueueDiscItem>
DtFifoQueueDisc::DoPeek(void){
    NS_LOG_FUNCTION(this);

    Ptr<const QueueDiscItem> item;

    if ((item = GetInternalQueue(0)->Peek()) != nullptr){
        return item;
    }

    NS_LOG_LOGIC("Queue empty");
    return item;
}

bool
DtFifoQueueDisc::CheckConfig(void)
{
    NS_LOG_FUNCTION(this);
    if (GetNQueueDiscClasses() > 0)//QueueDisc NUM
    {
        NS_LOG_ERROR("DtFifoQueueDisc cannot have classes");
        std::cout << "DtFifoQueueDisc cannot have classes" << std::endl;
        return false;
    }

    if (GetNPacketFilters() != 0)//packet filter NUM
    {
        NS_LOG_ERROR("DtFifoQueueDisc needs no packet filter");
        std::cout << "DtFifoQueueDisc needs no packet filter" << std::endl;
        return false;
    }

    if (m_sharedBufferSize == 0)
    {
        NS_LOG_ERROR("DtFifoQueueDisc cannot have zero shared buffer size");
        std::cout << "DtFifoQueueDisc cannot have zero shared buffer size" << std::endl;
        return false;
    }

/*     if (m_usedBufferPtr == nullptr)
    {
        NS_LOG_ERROR("DtFifoQueueDisc needs a pointer to used buffer size");
        std::cout << "DtFifoQueueDisc needs a pointer to used buffer size" << std::endl;
        return false;
    } */

    if (GetMaxSize().GetValue() == 0)
    {
        NS_LOG_ERROR("The capacity of queue(s) cannot be zero");
        std::cout << "The capacity of queue(s) cannot be zero" << std::endl;
        return false;
    }

    if (GetNInternalQueues() == 0)
    {
        // create a queue with GetMaxSize() packets each
        ObjectFactory factory;
        factory.SetTypeId("ns3::DropTailQueue<QueueDiscItem>");
        factory.Set("MaxSize", QueueSizeValue(GetMaxSize()));
        AddInternalQueue(factory.Create<InternalQueue>());
    }

    if (GetNInternalQueues() != 1)
    {
        NS_LOG_ERROR("Number of queue in DtFifoQueueDisc mismatches 1");
        std::cout << "Number of queue in DtFifoQueueDisc mismatches 1" << std::endl;
        return false;
    }

    if (GetInternalQueue(0)->GetMaxSize().GetUnit() != QueueSizeUnit::PACKETS)//packets or bytes
    {
        NS_LOG_ERROR("DtFifoQueueDisc needs internal queue operating in packet mode");
        std::cout << "DtFifoQueueDisc needs internal queue operating in packet mode" << std::endl;
        return false;
    }

    if (GetInternalQueue(0)->GetMaxSize() != GetMaxSize())
    {
        NS_LOG_ERROR("The capacity of internal queue mismatches the queue disc capacity");
        std::cout << "The capacity of internal queue mismatches the queue disc capacity" << std::endl;
        return false;
    }

    return true;
}

void
DtFifoQueueDisc::InitializeParams(void)
{
    NS_LOG_FUNCTION(this);
}

}// namespace ns3