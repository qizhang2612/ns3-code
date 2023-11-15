#include "ns3/log.h"
#include "switch.h"
#include "iostream"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Switch");

NS_OBJECT_ENSURE_REGISTERED (Switch);

TypeId Switch::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Switch")
    .SetParent<Object> ()
    .SetGroupName("Network")
  ;
  return tid;
}

Switch::Switch(){
    m_dtAlphaExp = 0;
    m_sharedBufferSize = 10000;//204800
    m_usedBufferPtr = Create<UintegerValue>(0);
    m_threshold = m_sharedBufferSize;
}

Switch::~Switch(){
    NS_LOG_FUNCTION (this);
}

int Switch::GetThreshold(){
    return int(m_threshold);
}

void Switch::Calculate(){
    uint64_t availBuffer = m_sharedBufferSize - m_usedBufferPtr->Get();
    m_threshold = (m_dtAlphaExp >= 0) ? (availBuffer << m_dtAlphaExp) : (availBuffer >> (-m_dtAlphaExp));
}

void Switch::SetSharedBufferSize(uint64_t sharedBufferSize){
    NS_LOG_FUNCTION(this << sharedBufferSize);
    m_sharedBufferSize = sharedBufferSize;
    Calculate();              //calculate the threshold ;
}

void Switch::SetAlphaExp(int32_t AlphaExp){
    NS_LOG_FUNCTION(this << AlphaExp);
    m_dtAlphaExp = AlphaExp;
    Calculate();              //calculate the threshold ;
}

void Switch::SetUsedBufferPtr(Ptr<UintegerValue> usedBufferPtr)
{
    NS_LOG_FUNCTION(this << usedBufferPtr);
    m_usedBufferPtr = usedBufferPtr;
    Calculate();              //calculate the threshold ;
}

void Switch::AddUsed(int size){      //this means a packet enqueue
    m_usedBufferPtr->Set(m_usedBufferPtr->Get() + size);
    Calculate();
}

void Switch::DeleteUsed(int size){
    m_usedBufferPtr->Set(m_usedBufferPtr->Get() - size);
    Calculate();
}













} 



// namespace ns3