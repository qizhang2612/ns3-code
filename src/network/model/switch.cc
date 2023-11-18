#include "ns3/log.h"
#include "switch.h"
#include "ns3/timer.h"
#include "iostream"
 #include <ns3/simulator.h>

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

/*state transition limit*/
# define PACKETDROPNUMLIMIT 9
# define TIMELIMIT 100
# define AASDTTIMELIMIT 200
# define QUEUELENGTHLIMIT 1000

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
    m_packetDequeueNum = 0;
    m_enQueueLength = 0;
    m_deQueueLength = 0;
    m_packetArriveSize = 0;
}

Switch::~Switch(){
    NS_LOG_FUNCTION (this);
}

int Switch::GetThreshold(){
    StatusJudgment();
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

int Switch::GetPacketDequeueNum(){
    return m_packetDequeueNum;
}

int Switch::GetEnQueueLength(){
    return m_enQueueLength;
}

int Switch::GetDeQueueLength(){
    return m_deQueueLength;
}

int Switch::GetAASDTITime(){
    return m_AASDTITime;
}

int Switch::GetAASDTCTime(){
    return m_AASDTCTime;
}

int64_t Switch::GetNowTime(){
    return Simulator::Now().GetMilliSeconds();
}

void Switch::Calculate(){
    uint64_t availBuffer = m_sharedBufferSize - m_usedBufferPtr->Get();
    if(m_strategy == DT){
        m_threshold = (m_dtAlphaExp >= 0) ? (availBuffer << m_dtAlphaExp) : (availBuffer >> (-m_dtAlphaExp));
    }else if(m_strategy == EDT){
        int n = m_EDTNCPortNumPtr->Get();
        if(m_EDTstate == CONTROL){
            m_threshold = (m_dtAlphaExp >= 0) ? (availBuffer << m_dtAlphaExp) : (availBuffer >> (-m_dtAlphaExp));
        }else{
            m_threshold = m_sharedBufferSize / n;
        }
    }else if(m_strategy == TDT){
        int n = m_PortNumPtr->Get();
        int n_a = m_TDTAPortNumPtr->Get();
        if(m_TDTstate == ABSORPTION){
            m_threshold = m_sharedBufferSize / n_a;
        }else if(m_TDTstate == EVACUATION){
            m_threshold = m_sharedBufferSize / n;
        }else{
            m_threshold = (m_dtAlphaExp >= 0) ? (availBuffer << m_dtAlphaExp) : (availBuffer >> (-m_dtAlphaExp));
        }
    }else if(m_strategy == AASDT){
        int n = m_PortNumPtr->Get();
        int n_n = m_AASDTNPortNumPtr->Get();
        int n_i = m_AASDTIPortNumPtr->Get();
        int n_c = m_AASDTCPortNumPtr->Get();
        int n_ci = m_AASDTCIPortNumPtr->Get();
        int n_cc = m_AASDTCCPortNumPtr->Get();
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
}

void Switch::SetSharedBufferSize(int sharedBufferSize){
    m_sharedBufferSize = sharedBufferSize;
}

void Switch::SetdtAlphaExp(int alphaExp){;
    m_dtAlphaExp = alphaExp;
}

void Switch::SetUsedBufferPtr(Ptr<UintegerValue> usedBufferPtr)
{
    m_usedBufferPtr = usedBufferPtr;
}

void Switch::SetPortNumPtr(Ptr<UintegerValue> PortNumPtr)
{
    m_PortNumPtr = PortNumPtr;
}

void Switch::SetEDTPortNumPtr(Ptr<UintegerValue> EDTCPortNumPtr,Ptr<UintegerValue> EDTNCPortNumPtr)
{
    m_EDTCPortNumPtr = EDTCPortNumPtr;
    m_EDTNCPortNumPtr = EDTNCPortNumPtr;
}


void Switch::SetTDTPortNumPtr(Ptr<UintegerValue> TDTNPortNumPtr,Ptr<UintegerValue> TDTAPortNumPtr,Ptr<UintegerValue> TDTEPortNumPtr)
{
    m_TDTNPortNumPtr = TDTNPortNumPtr;
    m_TDTAPortNumPtr = TDTAPortNumPtr;
    m_TDTEPortNumPtr = TDTEPortNumPtr;
}

void Switch::SetAASDTPortNumPtr(Ptr<UintegerValue> AASDTNPortNumPtr,Ptr<UintegerValue> AASDTIPortNumPtr,Ptr<UintegerValue> AASDTCPortNumPtr,Ptr<UintegerValue> AASDTCIPortNumPtr,Ptr<UintegerValue> AASDTCCPortNumPtr)
{
    m_AASDTNPortNumPtr = AASDTNPortNumPtr;
    m_AASDTIPortNumPtr = AASDTIPortNumPtr;
    m_AASDTCPortNumPtr = AASDTCPortNumPtr;
    m_AASDTCIPortNumPtr = AASDTCIPortNumPtr;
    m_AASDTCCPortNumPtr = AASDTCCPortNumPtr;
}

void Switch::AddPacketDropNum(){
    ++m_packetDropNum;
    if(m_packetDropNum > PACKETDROPNUMLIMIT){
        if(m_strategy == EDT){
            if(m_EDTstate == NONCONTROL){
                m_EDTstate = CONTROL;
                int n_nc = m_EDTNCPortNumPtr->Get();
                int n_c = m_EDTCPortNumPtr->Get();
                m_EDTNCPortNumPtr->Set(n_nc - 1);
                m_EDTCPortNumPtr->Set(n_c + 1);
            }
        }else if(m_strategy == TDT){
            if(m_TDTstate == TDTNORMAL){
                m_TDTstate = EVACUATION;
                int n_n = m_TDTNPortNumPtr->Get();
                int n_e = m_TDTEPortNumPtr->Get();
                m_TDTNPortNumPtr->Set(n_n - 1);
                m_TDTEPortNumPtr->Set(n_e + 1);
            }else if(m_TDTstate == ABSORPTION){
                m_TDTstate = TDTNORMAL;
                int n_n = m_TDTNPortNumPtr->Get();
                int n_a = m_TDTAPortNumPtr->Get();
                m_TDTNPortNumPtr->Set(n_n + 1);
                m_TDTEPortNumPtr->Set(n_a - 1);
            }
        }
    }
    //Calculate();
}

void Switch::AddPacketArriveSize(int size){
    m_packetArriveSize += size;
}

void Switch::AddPacketEnqueueNum(){
    ++m_packetEnqueueNum;
}

void Switch::AddPacketDequeueNum(){
    ++m_packetDequeueNum;
}

void Switch::AddEnQueueLength(int queuelength){
    m_enQueueLength += queuelength;
    if(m_enQueueLength - m_deQueueLength > QUEUELENGTHLIMIT){
        //state change
        if(m_isTrafficArrive){
            m_isTrafficArrive = false;
        }
        if(m_strategy == EDT){
            int n_c = m_EDTCPortNumPtr->Get();
            int n_n = m_EDTNCPortNumPtr->Get();
            if(m_EDTstate == NONCONTROL){
                m_EDTstate = CONTROL;
                m_EDTCPortNumPtr->Set(n_c + 1);
                m_EDTNCPortNumPtr->Set(n_n - 1);
            }
        }else if(m_strategy == TDT){
            int n_n = m_TDTNPortNumPtr->Get();
            int n_a = m_TDTAPortNumPtr->Get();
            if(m_TDTstate == ABSORPTION){
                m_TDTstate = TDTNORMAL;
                m_TDTAPortNumPtr->Set(n_a - 1);
                m_TDTNPortNumPtr->Set(n_n + 1);  
            }
        }else if(m_strategy == AASDT){
            int n_n = m_AASDTNPortNumPtr->Get();
            int n_i = m_AASDTIPortNumPtr->Get();
            int n_c = m_AASDTCPortNumPtr->Get();
            if(m_AASDTstate == INCAST){
                m_AASDTstate = AASDTNORMAL;
                m_AASDTNPortNumPtr->Set(n_n + 1);
                m_AASDTIPortNumPtr->Set(n_i - 1);
            }else if(m_AASDTstate == CONGESTION){
                m_AASDTstate = AASDTNORMAL;
                m_AASDTCPortNumPtr->Set(n_c - 1);
                m_AASDTNPortNumPtr->Set(n_n + 1); 
            }
        }
    }
    //Calculate();
}

void Switch::AddDeQueueLength(int queuelength){
    m_deQueueLength += queuelength;
    //m_stateTransitionTimer = GetNowTime();
    //std::cout<<m_stateTransitionTimer<<std::endl;
    if(m_packetArriveSize > m_deQueueLength){
        if(!m_isTrafficArrive){
            m_isTrafficArrive = true;
            //记录此时时间
            m_stateTransitionTimer = GetNowTime();
        }
        //state change
        if(m_strategy == EDT){
            int n_c = m_EDTCPortNumPtr->Get();
            int n_n = m_EDTNCPortNumPtr->Get();
            if(m_EDTstate == CONTROL){
                m_EDTstate = NONCONTROL;
                m_EDTCPortNumPtr->Set(n_c - 1);
                m_EDTNCPortNumPtr->Set(n_n + 1);
            }else{
                //判断是否超时
                int64_t m_time_now = GetNowTime();
                if(m_time_now - m_stateTransitionTimer > TIMELIMIT){
                    m_EDTstate = CONTROL;
                    m_EDTCPortNumPtr->Set(n_c + 1);
                    m_EDTNCPortNumPtr->Set(n_n - 1);
                }
            }
        }else if(m_strategy == TDT){
            int n_n = m_TDTNPortNumPtr->Get();
            int n_a = m_TDTAPortNumPtr->Get();
            if(m_TDTstate == TDTNORMAL){
                m_TDTstate = ABSORPTION;
                m_TDTAPortNumPtr->Set(n_a + 1);
                m_TDTNPortNumPtr->Set(n_n - 1);  
            }
        }else if(m_strategy == AASDT){
            int n_n = m_AASDTNPortNumPtr->Get();
            int n_i = m_AASDTIPortNumPtr->Get();
            int n_c = m_AASDTCPortNumPtr->Get();
            if(m_AASDTstate == AASDTNORMAL){
                m_AASDTstate = INCAST;
                m_AASDTNPortNumPtr->Set(n_n - 1);
                m_AASDTIPortNumPtr->Set(n_i + 1);
                ++m_AASDTITime;
            }else if(m_AASDTstate == INCAST){
                //判断是否超时
                int64_t m_time_now = GetNowTime();
                if(m_time_now - m_stateTransitionTimer > AASDTTIMELIMIT){
                    m_AASDTstate = CONGESTION;
                    m_AASDTCPortNumPtr->Set(n_c - 1);
                    m_AASDTIPortNumPtr->Set(n_i - 1);
                    ++m_AASDTCTime;
                } 
            }
        }
    }
    //Calculate();
}

void Switch::AddUsed(int size){      //this means a packet enqueue
    m_usedBufferPtr->Set(m_usedBufferPtr->Get() + size);
    //Calculate();
}

void Switch::DeleteUsed(int size){
    m_usedBufferPtr->Set(m_usedBufferPtr->Get() - size);
    //Calculate();
}

void Switch::StatusJudgment(){
    if(m_strategy == AASDT){
        int n_i = m_AASDTIPortNumPtr->Get();
        int n_c = m_AASDTCPortNumPtr->Get();
        int n_ci = m_AASDTCIPortNumPtr->Get();
        int n_cc = m_AASDTCCPortNumPtr->Get();
        if(m_AASDTstate == INCAST && n_c > 0){
            m_AASDTstate = COEXIST_I;
            m_AASDTIPortNumPtr->Set(n_i - 1);
            m_AASDTCIPortNumPtr->Set(n_ci + 1);
        }
        if(m_AASDTstate == CONGESTION && n_i > 0){
            m_AASDTstate = COEXIST_C;
            m_AASDTCPortNumPtr->Set(n_c - 1);
            m_AASDTCCPortNumPtr->Set(n_cc + 1);
        }
    }
}

void Switch::AASDTReset(int alphaExp){
    m_AASDTstate = AASDTNORMAL;
    SetdtAlphaExp(alphaExp);
    m_AASDTITime = 0;              
    m_AASDTCTime = 0; 
}


} // namespace ns3