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
# define TIMELIMIT 100
# define QUEUELENGTHLIMIT 100
# define ENQUEUERATELIMIT 100
# define DEQUEUERATELIMIT 40
# define PACKETNUM 3

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
    m_lastEnqueueLength = 0;
    m_lastDequeueLength = 0;
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
    /* if(m_stateChangePtr->Get() > 0){
        m_isDropPacket = false;
    } */
    Calculate();
    //std::cout<<m_threshold<<std::endl;
    if(m_threshold > m_sharedBufferSize){
        return m_sharedBufferSize;
    }
    return int64_t(m_threshold);
}

int Switch::GetPacketDropNum(){
    return m_packetDropNum;
}

int Switch::GetStrategy(){
    return m_strategy;
}

uint32_t Switch::GetPort(){
    return m_port;
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

uint32_t Switch::GetQueueLength(){
    return m_queueLength;
}

uint32_t Switch::GetQueuePacketNum(){
    return m_queuePacketNum;
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
            if(m_stateChangePtr->Get() > 0){
                //std::cout<<"zqzqzqzqzqzq"<<std::endl;
                //std::cout<<n<<std::endl;
                m_threshold = m_sharedBufferSize / n;
                m_stateChangePtr->Set(0);
            }
        }
    }else if(m_strategy == TDT){
        int n = m_PortNumPtr->Get();
        int n_a = m_TDTAPortNumPtr->Get();
        if(m_TDTstate == ABSORPTION){
            if(m_stateChangePtr->Get() > 0){
                m_threshold = m_sharedBufferSize / n_a;
                m_stateChangePtr->Set(0);
            }
        }else if(m_TDTstate == EVACUATION){
            if(m_stateChangePtr->Get() > 0){
                m_threshold = m_sharedBufferSize / n;
                m_stateChangePtr->Set(0);
            }
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
                //std::cout<<"-------this is AASDTNORMAL state-------"<<std::endl;
                if(m_stateChangePtr->Get() > 0){
                    SetdtAlphaExp(m_dtInitialAlpha);
                    m_stateChangePtr->Set(0);
                }
            }else if(m_AASDTstate == INCAST){
                //std::cout<<"11111111111111111111"<<std::endl;
                //std::cout<<"-------this is INCAST state-------"<<std::endl;
                if(m_stateChangePtr->Get() > 0){
                    SetdtAlphaExp(m_dtInitialAlpha + 1);
                    m_stateChangePtr->Set(0);
                }
                if(m_isDropPacket){
                    //std::cout<<"-------Drop Packet unlimited-------"<<std::endl;
                    SetdtAlphaExp(m_dtInitialAlpha + 1 + n);
                    m_isDropPacket = false;
                }
            }else if(m_AASDTstate == CONGESTION){
                //std::cout<<"-------this is CONGESTION state-------"<<std::endl;
                if(m_stateChangePtr->Get() > 0){
                    SetdtAlphaExp(m_dtInitialAlpha + 1);
                    m_stateChangePtr->Set(0);
                }
            }
        }else if(n_n == n){//all normal
            //std::cout<<"-------this is AASDTNORMAL state-------"<<std::endl;
            if(m_stateChangePtr->Get() > 0){
                SetdtAlphaExp(m_dtInitialAlpha);
                m_stateChangePtr->Set(0);
            }
        }else if(n_n + n_i == n){
            //only have incast port
            if(m_AASDTstate == INCAST){
                //std::cout<<"-------this is INCAST state-------"<<std::endl;
                if(m_stateChangePtr->Get() > 0){
                    SetdtAlphaExp(m_dtInitialAlpha + 1 + n - n_i);
                    m_stateChangePtr->Set(0);
                }
                if(m_isDropPacket){
                    //std::cout<<"-------Drop Packet unlimited-------"<<std::endl;
                    SetdtAlphaExp(m_dtInitialAlpha + 1 + n);
                    m_isDropPacket = false;
                }
            }else{
                //std::cout<<"-------this is AASDTNORMAL state-------"<<std::endl;
                if(m_stateChangePtr->Get() > 0){
                    SetdtAlphaExp(m_dtInitialAlpha);
                    m_stateChangePtr->Set(0);
                }
            }
        }else if(n_n + n_c == n){
            //only have CONGESTION port
            if(m_AASDTstate == CONGESTION){
                //std::cout<<"-------this is CONGESTION state-------"<<std::endl;
                if(m_stateChangePtr->Get() > 0){
                    SetdtAlphaExp(m_dtInitialAlpha + n_c);
                    m_stateChangePtr->Set(0);
                }
            }else{
                //std::cout<<"-------this is AASDTNORMAL state-------"<<std::endl;
                if(m_stateChangePtr->Get() > 0){
                    SetdtAlphaExp(m_dtInitialAlpha);
                    m_stateChangePtr->Set(0);
                }
            }
        }else{
            //COEXIST
            if(m_AASDTstate == COEXIST_I){
                //std::cout<<"-------this is COEXIST_I state-------"<<std::endl;
                if(m_stateChangePtr->Get() > 0){
                    SetdtAlphaExp(m_dtInitialAlpha + n + 1 + n_cc - n_ci);
                    m_stateChangePtr->Set(0);
                }
            }else if(m_AASDTstate == COEXIST_C){
                //std::cout<<"-------this is COEXIST_C state-------"<<std::endl;
                if(m_stateChangePtr->Get() > 0){
                    SetdtAlphaExp(m_dtInitialAlpha + n_cc - n_ci);
                    m_stateChangePtr->Set(0);
                }
            }else{
                //std::cout<<"-------this is AASDTNORMAL state-------"<<std::endl;
                if(m_stateChangePtr->Get() > 0){
                    SetdtAlphaExp(m_dtInitialAlpha);
                    m_stateChangePtr->Set(0);
                }
            }
        }
        m_threshold = (m_dtAlphaExp >= 0) ? (availBuffer << m_dtAlphaExp) : (availBuffer >> (-m_dtAlphaExp));
    }else{
        std::cout << "Error in threshold calculate" << std::endl;
    }   
}

void Switch::SetStrategy(int strategy){
    m_strategy = strategy;
}

void Switch::SetPort(uint32_t port){
    m_port = port;
}
void Switch::SetSharedBufferSize(int sharedBufferSize){
    m_sharedBufferSize = sharedBufferSize;
}

void Switch::SetdtAlphaExp(int alphaExp){;
    m_dtAlphaExp = alphaExp;
}

void Switch::SetdtInitialAlphaExp(int alphaExp){
    m_dtInitialAlpha = alphaExp;
    SetdtAlphaExp(alphaExp);
}

void Switch::SetQueueLength(uint32_t length){
    m_queueLength = length;
}

void Switch::SetQueuePacketNum(uint32_t num){
    m_queuePacketNum = num;
}

void Switch::SetUsedBufferPtr(Ptr<UintegerValue> usedBufferPtr)
{
    m_usedBufferPtr = usedBufferPtr;
}

void Switch::SetPortNumPtr(Ptr<UintegerValue> PortNumPtr)
{
    m_PortNumPtr = PortNumPtr;
}

void Switch::SetStateChangePtr(Ptr<UintegerValue> stateChangePtr)
{
    m_stateChangePtr = stateChangePtr;
}

void Switch::SetAASDTICNumPtr(Ptr<UintegerValue> AASDTITimePtr,Ptr<UintegerValue> AASDTCTimePtr){
    m_AASDTITimePtr = AASDTITimePtr;
    m_AASDTCTimePtr = AASDTCTimePtr;
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
    m_C2 = 0;
    if(m_strategy == AASDT && m_packetDropNum > AASDTPACKETDROPNUMLIMIT){
        m_isDropPacket = true;
        m_packetDropNum = 0;
    }
    if(m_strategy == TDT && m_packetDropNum > TDTPACKETDROPNUMLIMIT){
        if(m_TDTstate == TDTNORMAL){
            m_TDTstate = EVACUATION;
            m_stateChangePtr->Set(1);
            int n_n = m_TDTNPortNumPtr->Get();
            int n_e = m_TDTEPortNumPtr->Get();
            m_TDTNPortNumPtr->Set(n_n - 1);
            m_TDTEPortNumPtr->Set(n_e + 1);
        }else if(m_TDTstate == ABSORPTION){
            m_TDTstate = TDTNORMAL;
            m_stateChangePtr->Set(1);
            int n_n = m_TDTNPortNumPtr->Get();
            int n_a = m_TDTAPortNumPtr->Get();
            m_TDTNPortNumPtr->Set(n_n + 1);
            m_TDTEPortNumPtr->Set(n_a - 1);
            m_packetDropNum = 0;
            m_isTrafficExist = false;
        }
    }
}


void Switch::AddPacketArriveSize(int size){
    m_packetArriveSize += size;
    //start time
    if(m_startTime == 0){
        m_startTime = GetNowTime();
        m_T1 = GetNowTime();
        std::cout<<"-------start time in port "<<m_port<<" is "<<m_startTime<<std::endl;
    }
    int64_t nowTime = GetNowTime();
    if(nowTime - m_startTime > ADJUSTCYCLE){
        AASDTReset();
    }
    //EDT 计时器更新
    if(m_strategy == EDT && (nowTime - m_T1 > EDT_T1 && m_C2 <= EDT_C2)){
        m_T1 = GetNowTime();
        m_C2 = 0;
    }
}

void Switch::AddPacketEnqueueNum(){
    ++m_packetEnqueueNum;
}

void Switch::AddPacketDequeueNum(){
    ++m_packetDequeueNum;
    ++m_OC1;
    if(m_OC1 >= TDTOC1){
        m_OC1 = m_OC1 % TDTOC1;
        m_isTDTIncast = false;
    }
}

void Switch::TimeoutJudgment(){
    int64_t m_time_now = GetNowTime();
    if(m_strategy == EDT && m_EDTstate == NONCONTROL){
        int n_c = m_EDTCPortNumPtr->Get();
        int n_n = m_EDTNCPortNumPtr->Get();
        if(m_time_now - m_T2 > EDT_T2){
            m_EDTstate = CONTROL;
            m_stateChangePtr->Set(1);
            //std::cout<<"22222222222";
            m_EDTCPortNumPtr->Set(n_c + 1);
            m_EDTNCPortNumPtr->Set(n_n - 1);
            m_T1 = GetNowTime();
            m_C2 = 0;
            m_C1 = 0;
            m_isTrafficExist = false;
        }
    }else if(m_strategy == TDT && m_TDTstate == ABSORPTION){
        int n_n = m_TDTNPortNumPtr->Get();
        int n_a = m_TDTAPortNumPtr->Get();
        if(m_packetDequeueNum >= TDTOC2){
            m_TDTstate = TDTNORMAL;
            m_stateChangePtr->Set(1);
            m_TDTNPortNumPtr->Set(n_n + 1);
            m_TDTEPortNumPtr->Set(n_a - 1);
            m_packetDequeueNum = 0;
            m_isTrafficExist = false;
        }
    }else if(m_strategy == AASDT && m_AASDTstate == INCAST){
        int n_i = m_AASDTIPortNumPtr->Get();
        int n_c = m_AASDTCPortNumPtr->Get();
        if(m_time_now - m_stateTransitionTimer > AASDTTIMELIMIT){
            m_stateChangePtr->Set(1);
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
    if(m_packetEnqueueNum % PACKETNUM == 1){
        if(m_enqueueClock == 0){
            m_enqueueClock = GetNowTime();
            m_lastEnqueueLength = m_enqueueLength;
        }else{
            int64_t nowClock = GetNowTime();
            m_enqueueInterval = nowClock - m_enqueueClock;
            //std::cout<<m_enqueueInterval<<std::endl;
            m_enqueueClock = nowClock;
            if(m_enqueueInterval != 0){
                m_enqueueRate = (m_enqueueLength - m_lastEnqueueLength) / m_enqueueInterval;
                m_lastEnqueueLength = m_enqueueLength;
                std::cout<<"-------enqueue rate in port "<<m_port<<" is "<<m_enqueueRate<<std::endl;
            }else{
                std::cout<<"-------m_enqueueInterval in port "<<m_port<<" is "<<m_enqueueInterval<<std::endl;
            }
            /* m_enqueueRate = m_enqueueLength / m_enqueueInterval;
            std::cout<<"-------enqueue rate is "<<m_enqueueRate<<std::endl; */
        }
    }

    //cycle adjust
    int64_t nowTime = GetNowTime();
    if(nowTime - m_startTime > ADJUSTCYCLE){
        AASDTReset();
    }

    //judge queuelength short
    //std::cout<<m_enqueueLength - m_dequeueLength;
    if(m_queueLength < QUEUELENGTHLIMIT){
        m_isQueueShort = true;
        if(m_strategy == TDT && m_TDTstate == EVACUATION){
            m_TDTstate = TDTNORMAL;
            m_stateChangePtr->Set(1);
            int n_n = m_TDTNPortNumPtr->Get();
            int n_e = m_TDTEPortNumPtr->Get();
            m_TDTNPortNumPtr->Set(n_n + 1);
            m_TDTEPortNumPtr->Set(n_e - 1);
        }
    }else{
        m_isQueueShort = false;
    }
    
    //入队增速超过阈值，状态调整
    //std::cout<<m_enqueueRate - m_lastEnqueueRate;
    //if(m_enqueueRate - m_lastEnqueueRate > ENQUEUERATELIMIT && m_lastEnqueueRate != 0){
    if(m_enqueueRate > ENQUEUERATELIMIT && m_lastEnqueueRate != 0){
        //记录最开始流量到达的时间
        ++m_C2;
        //std::cout<<m_enqueueRate<<std::endl;
        //std::cout<<m_lastEnqueueRate<<std::endl;
        //state change
    }
    //std::cout<<m_C2<<std::endl;
    if(m_strategy == EDT && m_C2 >= EDT_C2){
        if(!m_isTrafficExist){
            std::cout<<"---------port "<<m_port<<" incast COME ----------"<<std::endl;
            m_isTrafficExist = true;
            //记录最开始流量到达的时间
            m_T2 = GetNowTime();
        }
        int n_c = m_EDTCPortNumPtr->Get();
        int n_n = m_EDTNCPortNumPtr->Get();
        if(m_EDTstate == CONTROL){
            m_stateChangePtr->Set(1);
            m_EDTstate = NONCONTROL;
            m_EDTCPortNumPtr->Set(n_c - 1);
            m_EDTNCPortNumPtr->Set(n_n + 1);
        }
    }
    if(m_strategy == TDT && (m_queuePacketNum >= TDTNEC && (m_packetDropNum == 0) && m_isTDTIncast)){
        if(!m_isTrafficExist){
            std::cout<<"---------port "<<m_port<<" incast COME ----------"<<std::endl;
            m_isTrafficExist = true;
        }
        int n_n = m_TDTNPortNumPtr->Get();
        int n_a = m_TDTAPortNumPtr->Get();
        if(m_TDTstate == TDTNORMAL){
            m_stateChangePtr->Set(1);
            m_TDTstate = ABSORPTION;
            m_TDTAPortNumPtr->Set(n_a + 1);
            m_TDTNPortNumPtr->Set(n_n - 1);  
        }
    }
    if(m_strategy == AASDT && (m_C2 >= EDT_C2 || m_queuePacketNum >= TDTNEC)){
        if(!m_isTrafficExist){
            std::cout<<"---------port "<<m_port<<" incast COME ----------"<<std::endl;
            m_isTrafficExist = true;
            //记录最开始流量到达的时间
            m_stateTransitionTimer = GetNowTime();
        }
        int n_n = m_AASDTNPortNumPtr->Get();
        int n_i = m_AASDTIPortNumPtr->Get();
        //int n_c = m_AASDTCPortNumPtr->Get();
        if(m_AASDTstate == AASDTNORMAL){
            m_stateChangePtr->Set(1);
            m_AASDTstate = INCAST;
            int AASDTITime = m_AASDTITimePtr->Get();
            m_AASDTNPortNumPtr->Set(n_n - 1);
            m_AASDTIPortNumPtr->Set(n_i + 1);
            m_AASDTITimePtr->Set(AASDTITime + 1);
        }
    }
    m_lastEnqueueRate = m_enqueueRate;
    TimeoutJudgment();
}

void Switch::AddDeQueueLength(int queuelength){
    m_dequeueLength += queuelength;
    //每几个包算一下出队速率
    if(m_packetDequeueNum % PACKETNUM == 1){
        if(m_dequeueClock == 0){
            m_dequeueClock = GetNowTime();
        }else{
            int64_t nowClock = GetNowTime();
            m_dequeueInterval = nowClock - m_dequeueClock;
            //std::cout<<m_dequeueInterval<<std::endl;
            m_dequeueClock = nowClock;
            if(m_dequeueInterval != 0){
                m_dequeueRate = (m_dequeueLength - m_lastDequeueLength) / m_dequeueInterval;
                m_lastDequeueLength = m_dequeueLength;
                std::cout<<"-------dequeue rate in port "<<m_port<<" is "<<m_dequeueRate<<std::endl;
            }else{
                std::cout<<"-------m_dequeueInterval in port "<<m_port<<" is "<<m_dequeueInterval<<std::endl;
            }
            /* m_dequeueRate = m_dequeueLength / m_dequeueInterval;
            std::cout<<"-------dequeue rate is "<<m_dequeueRate<<std::endl; */
        }

    }

    //cycle adjust
    int64_t nowTime = GetNowTime();
    if(nowTime - m_startTime > ADJUSTCYCLE){
        AASDTReset();
    }
    //std::cout<<m_isQueueShort<<std::endl;
    //出队速率小于阈值且队列长度小于阈值
    if((m_dequeueRate !=0 && m_dequeueRate <= DEQUEUERATELIMIT) && m_isQueueShort){
        //std::cout<<"----- incast traffic leave -----"<<std::endl;
        ++m_C1;
    }
    if(m_strategy == EDT && m_C1 > EDT_C1){
        if(m_isTrafficExist){
            std::cout<<"---------port "<<m_port<<" incast traffic leave ----------"<<std::endl;
            m_isTrafficExist = false;
        }
        //std::cout<<"11111111"<<std::endl;
        int n_c = m_EDTCPortNumPtr->Get();
        int n_n = m_EDTNCPortNumPtr->Get();
        if(m_EDTstate == NONCONTROL){
            m_stateChangePtr->Set(1);
            m_EDTstate = CONTROL;
            m_EDTCPortNumPtr->Set(n_c + 1);
            m_EDTNCPortNumPtr->Set(n_n - 1);
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
            m_stateChangePtr->Set(1);
            m_AASDTstate = COEXIST_I;
            m_AASDTIPortNumPtr->Set(n_i - 1);
            m_AASDTCIPortNumPtr->Set(n_ci + 1);
        }
        if(m_AASDTstate == CONGESTION && n_i > 0){
            m_stateChangePtr->Set(1);
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
    SetdtInitialAlphaExp(alphaExp);
    m_AASDTITimePtr->Set(0);
    m_AASDTCTimePtr->Set(0);
    m_packetEnqueueNum = 0;
    m_startTime = GetNowTime();
    m_stateChangePtr->Set(1);
    //port adjust
    int n = m_PortNumPtr->Get();
    m_AASDTNPortNumPtr->Set(n);
    m_AASDTIPortNumPtr->Set(0);
    m_AASDTCPortNumPtr->Set(0);
    m_AASDTCIPortNumPtr->Set(0);
    m_AASDTCCPortNumPtr->Set(0);
}


} // namespace ns3