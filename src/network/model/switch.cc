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


/*state transition limit*/
# define PACKETDROPNUMLIMIT 9
# define AASDTPACKETDROPNUMLIMIT 5
# define TIMELIMIT 100
# define AASDTTIMELIMIT 200
# define QUEUELENGTHLIMIT 0
# define ENQUEUERATELIMIT 1000
# define DEQUEUERATELIMIT 40
# define PACKETNUM 1

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
    m_threshold = m_sharedBufferSize;
    m_packetDropNum = 0;
    m_packetEnqueueNum = 0;
    m_packetDequeueNum = 0;
    m_enqueueLength = 0;
    m_dequeueLength = 0;
    m_packetArriveSize = 0;
    m_enqueueClock = 0;
    m_dequeueClock = 0;
    m_startTime = 0;
    //ptr
    m_usedBufferPtr = Create<UintegerValue>(0);
    m_AASDTITimePtr = Create<UintegerValue>(0);
    m_AASDTCTimePtr = Create<UintegerValue>(0);
}

Switch::~Switch(){
    NS_LOG_FUNCTION (this);
}

int Switch::GetThreshold(){
    StatusJudgment();
    Calculate();
    //std::cout<<m_threshold<<std::endl;
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
    return m_enqueueLength;
}

int Switch::GetDeQueueLength(){
    return m_dequeueLength;
}

int Switch::GetAASDTITime(){
    return m_AASDTITimePtr->Get();
}

int Switch::GetAASDTCTime(){
    return m_AASDTCTimePtr->Get();
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
        if(n == 1){
            if(m_AASDTstate == AASDTNORMAL){
                SetdtAlphaExp(m_dtAlphaExp);
            }else if(m_AASDTstate == INCAST){
                //std::cout<<"11111111111111111111"<<std::endl;
                if(m_packetDropNum <= AASDTPACKETDROPNUMLIMIT){
                    SetdtAlphaExp(m_dtAlphaExp + 1);
                    //std::cout<<m_threshold<<std::endl;
                }else{
                    SetdtAlphaExp(m_dtAlphaExp + 1 + n);
                }
            }else if(m_AASDTstate == CONGESTION){
                SetdtAlphaExp(m_dtAlphaExp + 1);
            }
        }else if(n_n == n){//all normal
            SetdtAlphaExp(m_dtAlphaExp);
        }else if(n_n + n_i == n){
            //only have incast port
            if(m_AASDTstate == INCAST){
                if(m_packetDropNum <= AASDTPACKETDROPNUMLIMIT){
                    SetdtAlphaExp(m_dtAlphaExp + 1 + n - n_i);
                }else{
                    SetdtAlphaExp(m_dtAlphaExp + 1 + n);
                }
            }else{
                SetdtAlphaExp(m_dtAlphaExp);
            }
        }else if(n_n + n_c == n){
            //only have CONGESTION port
            if(m_AASDTstate == CONGESTION){
                SetdtAlphaExp(m_dtAlphaExp + n_c);
            }else{
                SetdtAlphaExp(m_dtAlphaExp);
            }
        }else{
            //COEXIST
            if(m_AASDTstate == COEXIST_I){
                SetdtAlphaExp(m_dtAlphaExp + n + 1 + n_cc - n_ci);
            }else if(m_AASDTstate == COEXIST_C){
                SetdtAlphaExp(m_dtAlphaExp + n_cc - n_ci);
            }else{
                SetdtAlphaExp(m_dtAlphaExp);
            }
        }
        m_threshold = (m_dtAlphaExp >= 0) ? (availBuffer << m_dtAlphaExp) : (availBuffer >> (-m_dtAlphaExp));
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
}

void Switch::AddPacketArriveSize(int size){
    m_packetArriveSize += size;
    //start time
    if(m_startTime == 0){
        m_startTime = GetNowTime();
        std::cout<<"-------start time is "<<m_startTime<<std::endl;
    }
    int64_t nowTime = GetNowTime();
    if(nowTime - m_startTime > ADJUSTCYCLE){
        AASDTReset();
    }
}

void Switch::AddPacketEnqueueNum(){
    ++m_packetEnqueueNum;
}

void Switch::AddPacketDequeueNum(){
    ++m_packetDequeueNum;
}

void Switch::TimeoutJudgment(){
    int64_t m_time_now = GetNowTime();
    if(m_strategy == EDT && m_EDTstate == NONCONTROL){
        int n_c = m_EDTCPortNumPtr->Get();
        int n_n = m_EDTNCPortNumPtr->Get();
        if(m_time_now - m_stateTransitionTimer > TIMELIMIT){
            m_EDTstate = CONTROL;
            //std::cout<<"22222222222";
            m_EDTCPortNumPtr->Set(n_c + 1);
            m_EDTNCPortNumPtr->Set(n_n - 1);
        }
    }else if(m_strategy == AASDT && m_AASDTstate == INCAST){
        int n_i = m_AASDTIPortNumPtr->Get();
        int n_c = m_AASDTCPortNumPtr->Get();
        if(m_time_now - m_stateTransitionTimer > AASDTTIMELIMIT){
            m_AASDTstate = CONGESTION;
            m_AASDTCPortNumPtr->Set(n_c - 1);
            m_AASDTIPortNumPtr->Set(n_i - 1);
            int AASDTCTime = m_AASDTCTimePtr->Get();
            m_AASDTCTimePtr->Set(AASDTCTime + 1);
        } 
    }
}


void Switch::AddEnQueueLength(int queuelength){
    m_enqueueLength += queuelength;
    //每几个包算一下入队速率
    if(m_packetEnqueueNum % PACKETNUM == 0){
        if(m_enqueueClock == 0){
            m_enqueueClock = GetNowTime();
        }else{
            int64_t nowClock = GetNowTime();
            m_enqueueInterval = nowClock - m_enqueueClock;
            //std::cout<<m_enqueueInterval<<std::endl;
            m_enqueueClock = nowClock;
            m_enqueueRate = m_enqueueLength / m_enqueueInterval;
            std::cout<<"-------enqueue rate is "<<m_enqueueRate<<std::endl;
        }
    }

    //cycle adjust
    int64_t nowTime = GetNowTime();
    if(nowTime - m_startTime > ADJUSTCYCLE){
        AASDTReset();
    }

    //judge queuelength short
    if(m_enqueueLength - m_dequeueLength < QUEUELENGTHLIMIT){
        m_isQueueShort = true;
    }
    
    //入队增速超过阈值，状态调整
    if(m_enqueueRate - m_lastEnqueueRate > ENQUEUERATELIMIT){
        //记录最开始流量到达的时间
        if(!m_isTrafficExist){
            m_isTrafficExist = true;
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
            //int n_c = m_AASDTCPortNumPtr->Get();
            if(m_AASDTstate == AASDTNORMAL){
                m_AASDTstate = INCAST;
                int AASDTITime = m_AASDTITimePtr->Get();
                m_AASDTNPortNumPtr->Set(n_n - 1);
                m_AASDTIPortNumPtr->Set(n_i + 1);
                m_AASDTITimePtr->Set(AASDTITime + 1);
            }
        }
    }
    m_lastEnqueueRate = m_enqueueRate;
    TimeoutJudgment();
}

void Switch::AddDeQueueLength(int queuelength){
    m_dequeueLength += queuelength;
    //每几个包算一下出队速率
    if(m_packetDequeueNum % PACKETNUM == 0){
        if(m_dequeueClock == 0){
            m_dequeueClock = GetNowTime();
        }else{
            int64_t nowClock = GetNowTime();
            m_dequeueInterval = nowClock - m_dequeueClock;
            //std::cout<<m_dequeueInterval<<std::endl;
            m_dequeueClock = nowClock;
            m_dequeueRate = m_dequeueLength / m_dequeueInterval;
            std::cout<<"-------dequeue rate is "<<m_dequeueRate<<std::endl;
        }

    }

    //cycle adjust
    int64_t nowTime = GetNowTime();
    if(nowTime - m_startTime > ADJUSTCYCLE){
        AASDTReset();
    }

    //出队速率小于阈值且队列长度小于阈值
    if((m_dequeueRate !=0 && m_dequeueRate <= DEQUEUERATELIMIT) && m_isQueueShort){
        std::cout<<"incast traffic leave "<<std::endl;
        //state change
        if(m_isTrafficExist){
            m_isTrafficExist = false;
        }
        if(m_strategy == EDT){
            //std::cout<<"11111111"<<std::endl;
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
}

void Switch::AddUsed(int size){      //this means a packet enqueue
    m_usedBufferPtr->Set(m_usedBufferPtr->Get() + size);
}

void Switch::DeleteUsed(int size){
    m_usedBufferPtr->Set(m_usedBufferPtr->Get() - size);
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

void Switch::AASDTReset(){
    m_AASDTstate = AASDTNORMAL;
    //α adjust
    int AASDTITime = m_AASDTITimePtr->Get();
    int AASDTCTime = m_AASDTCTimePtr->Get();
    int alphaExp = ADJUSTPARAMETER * AASDTCTime - AASDTITime;
    SetdtAlphaExp(alphaExp);
    m_AASDTITimePtr->Set(0);
    m_AASDTCTimePtr->Set(0);
    m_packetEnqueueNum = 0;
    m_startTime = GetNowTime();
    //port adjust
    int n = m_PortNumPtr->Get();
    m_AASDTNPortNumPtr->Set(n);
    m_AASDTIPortNumPtr->Set(0);
    m_AASDTCPortNumPtr->Set(0);
    m_AASDTCIPortNumPtr->Set(0);
    m_AASDTCCPortNumPtr->Set(0);
}


} // namespace ns3