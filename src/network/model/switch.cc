#include "ns3/log.h"
#include "switch.h"
#include "iostream"

/*Buffer Management Algorithms*/
# define DT 0
# define EDT 1
# define TDT 2
# define AASDT 3

/*EDT port state*/
# define CONTROL 10
# define NONCONTROL 11

/*TDT port state*/
# define TDTNORMAL 20
# define ABSORPTION 21
# define EVACUATION 22

/*AASDT port state*/
# define AASDTNORMAL 30
# define INCAST 31
# define CONGESTION 32
# define COEXIST_I 33
# define COEXIST_C 34

/*AASDT packet drop num limit*/
# define PACKETDROPNUMLIMIT 10

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
    m_packetDropNum = 0;
    m_packetEnqueueNum = 0;
}

Switch::~Switch(){
    NS_LOG_FUNCTION (this);
}

int Switch::GetThreshold(){
    Calculate();
    return int(m_threshold);
}

int Switch::GetPacketDropNum(){
    return m_packetDropNum;
}

int Switch::GetSharedBufferSize(){
    return m_sharedBufferSize;
}

int Switch::GetAvailBufferSize(){
    return m_sharedBufferSize - m_usedBufferPtr->Get();
}

int Switch::GetPacketEnqueueNum(){
    return m_packetEnqueueNum;
}

void Switch::Calculate(){
    uint64_t availBuffer = m_sharedBufferSize - m_usedBufferPtr->Get();
    if(m_strategy == DT){
        m_threshold = (m_dtAlphaExp >= 0) ? (availBuffer << m_dtAlphaExp) : (availBuffer >> (-m_dtAlphaExp));
    }else if(m_strategy == EDT){
        int n = m_EDTNCPortNum->Get();
        if(m_EDTstate == CONTROL){
            m_threshold = (m_dtAlphaExp >= 0) ? (availBuffer << m_dtAlphaExp) : (availBuffer >> (-m_dtAlphaExp));
        }else{
            m_threshold = m_sharedBufferSize / n;
        }
    }else if(m_strategy == TDT){
        int n = m_PortNum->Get();
        int n_a = m_TDTAPortNum->Get();
        if(m_TDTstate == ABSORPTION){
            m_threshold = m_sharedBufferSize / n_a;
        }else if(m_TDTstate == EVACUATION){
            m_threshold = m_sharedBufferSize / n;
        }else{
            m_threshold = (m_dtAlphaExp >= 0) ? (availBuffer << m_dtAlphaExp) : (availBuffer >> (-m_dtAlphaExp));
        }
    }else if(m_strategy == AASDT){
        int n = m_PortNum->Get();
        int n_n = m_AASDTNPortNum->Get();
        int n_i = m_AASDTIPortNum->Get();
        int n_c = m_AASDTCPortNum->Get();
        int n_ci = m_AASDTCIPortNum->Get();
        int n_cc = m_AASDTCCPortNum->Get();
        if(n_n == n){//all normal
            m_threshold = (m_dtAlphaExp >= 0) ? (availBuffer << m_dtAlphaExp) : (availBuffer >> (-m_dtAlphaExp));
        }else if(n_n + n_i == n){
            //only have incast port
            if(m_AASDTstate == INCAST){
                if(m_packetDropNum <= PACKETDROPNUMLIMIT){
                    int alpha = m_dtAlphaExp + 1 + n - n_i;
                    m_threshold = (alpha >= 0) ? (availBuffer << alpha) : (availBuffer >> (-alpha));
                }else{
                    int alpha = m_dtAlphaExp + 1 + n;
                    m_threshold = (alpha >= 0) ? (availBuffer << alpha) : (availBuffer >> (-alpha));
                }
            }else{
                m_threshold = (m_dtAlphaExp >= 0) ? (availBuffer << m_dtAlphaExp) : (availBuffer >> (-m_dtAlphaExp));
            }
        }else if(n_n + n_c == n){
            //only have CONGESTION port
            if(m_AASDTstate == CONGESTION){
                int alpha = m_dtAlphaExp + n_c;
                m_threshold = (alpha >= 0) ? (availBuffer << alpha) : (availBuffer >> (-alpha));
            }else{
                m_threshold = (m_dtAlphaExp >= 0) ? (availBuffer << m_dtAlphaExp) : (availBuffer >> (-m_dtAlphaExp));
            }
        }else{
            //COEXIST
            if(m_AASDTstate == COEXIST_I){
                int alpha = m_dtAlphaExp + n + 1 + n_cc - n_ci;
                m_threshold = (alpha >= 0) ? (availBuffer << alpha) : (availBuffer >> (-alpha));
            }else if(m_AASDTstate == COEXIST_C){
                int alpha = m_dtAlphaExp + n_cc - n_ci;
                m_threshold = (alpha >= 0) ? (availBuffer << alpha) : (availBuffer >> (-alpha));
            }else{
                m_threshold = (m_dtAlphaExp >= 0) ? (availBuffer << m_dtAlphaExp) : (availBuffer >> (-m_dtAlphaExp));
            }
        }
    }else{
        std::cout << "Error in threshold calculate" << std::endl;
    }   
}

void Switch::Setstrategy(int strategy){
    m_strategy = strategy;
    Calculate();              //calculate the threshold ;
}

void Switch::SetSharedBufferSize(int sharedBufferSize){
    m_sharedBufferSize = sharedBufferSize;
    Calculate();              //calculate the threshold ;
}

void Switch::SetdtAlphaExp(int alphaExp){
    //std::cout<<"111111111111111"<<std::endl;
    m_dtAlphaExp = alphaExp;
    Calculate();              //calculate the threshold ;
}

void Switch::SetUsedBufferPtr(Ptr<UintegerValue> usedBufferPtr)
{
    m_usedBufferPtr = usedBufferPtr;
    Calculate();              //calculate the threshold ;
}

void Switch::AddPacketDropNum(){
    ++m_packetDropNum;
}

void Switch::AddPacketEnqueueNum(){
    ++m_packetEnqueueNum;
}

void Switch::AddUsed(int size){      //this means a packet enqueue
    m_usedBufferPtr->Set(m_usedBufferPtr->Get() + size);
    Calculate();
}

void Switch::DeleteUsed(int size){
    m_usedBufferPtr->Set(m_usedBufferPtr->Get() - size);
    Calculate();
}



} // namespace ns3