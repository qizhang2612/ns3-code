#ifndef SWITCH_H
#define SWITCH_H

#include "ns3/object.h"
#include "ns3/ptr.h"
#include "ns3/uinteger.h"
#include "iostream"
#include "ns3/nstime.h"
#include "ns3/core-module.h"

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

//EDT Threshold
# define EDT_C1 3
# define EDT_C2 3
# define EDT_T1 2.7
# define EDT_T2 10

//TDT Threshold
# define TDTPACKETDROPNUMLIMIT 333
# define TDTNEC 42
# define TDTDEC 3
# define TDTOC1 42
# define TDTOC2 500
//# define TDTOC2 1344


//AASDT Threshold
# define AASDTPACKETDROPNUMLIMIT 5
# define AASDTTIMELIMIT 200
# define AASDTOC 500

/*AASDT α Adjust the cycle : ms*/
# define ADJUSTCYCLE 1000000
# define ADJUSTPARAMETER 10

namespace ns3 {
    class Switch : public Object{
        public:
            static TypeId GetTypeId (void);
            Switch();
            virtual ~Switch();
            //get function
            int GetThreshold();
            int GetPacketDropNum();
            int GetStrategy();
            int GetSharedBufferSize();
            int GetAvailBufferSize();
            int GetPacketEnqueueNum();
            int GetPacketDequeueNum();
            int GetAASDTITime();
            int GetAASDTCTime();
            int GetEnQueueLength();
            int GetDeQueueLength();
            uint32_t GetPort();
            uint32_t GetQueuePacketNum();
            uint32_t GetQueueLength();
            int64_t GetNowTime(); //ms
            //set function
            void SetdtAlphaExp(int alphaExp);
            void SetdtInitialAlphaExp(int alphaExp);
            void SetStrategy(int strategy);
            void SetPort(uint32_t port);
            void SetQueueLength(uint32_t length);
            void SetQueuePacketNum(uint32_t num);
            void SetUsedBufferPtr(Ptr<UintegerValue> usedBufferPtr);
            void SetSharedBufferSize(int sharedBufferSize);
            void SetPortNumPtr(Ptr<UintegerValue> PortNumPtr);
            void SetStateChangePtr(Ptr<UintegerValue> stateChangePtr);
            void SetAASDTICNumPtr(Ptr<UintegerValue> AASDTITimePtr,Ptr<UintegerValue> AASDTCTimePtr);
            void SetEDTPortNumPtr(Ptr<UintegerValue> EDTCPortNumPtr,Ptr<UintegerValue> EDTNCPortNumPtr);
            void SetTDTPortNumPtr(Ptr<UintegerValue> TDTNPortNumPtr,Ptr<UintegerValue> TDTAPortNumPtr,Ptr<UintegerValue> TDTEPortNumPtr);
            void SetAASDTPortNumPtr(Ptr<UintegerValue> AASDTNPortNumPtr,Ptr<UintegerValue> AASDTIPortNumPtr,Ptr<UintegerValue> AASDTCPortNumPtr,Ptr<UintegerValue> AASDTCIPortNumPtr,Ptr<UintegerValue> AASDTCCPortNumPtr);
            //Calculate function
            void AddEnQueueLength(int queuelength);
            void AddDeQueueLength(int queuelength);
            void AddPacketDropNum();
            void AddPacketEnqueueNum();
            void AddPacketDequeueNum();
            void AddPacketArriveSize(int size);
            void AddUsed(int size);
            void DeleteUsed(int size);
            void Calculate();
            void StatusJudgment();
            void TimeoutJudgment();
            void AASDTReset();

            int m_EDTstate = CONTROL;                     //10:控制；11:非控制;
            int m_TDTstate = TDTNORMAL;                   //20:正常；21:吸收;22:疏散
            int m_AASDTstate = AASDTNORMAL;//INCAST;      //30:正常；31:突发;32:拥塞;33:共存1;34:共存2

        private:
            uint32_t m_port;                    //port i
            uint32_t m_queueLength;             //queue length
            uint32_t m_queuePacketNum;          //净排队包数量

            int m_strategy = 0;                 //0:DT;1:EDT;2:TDT;3:AASDT
            int m_dtAlphaExp;                   //alpha = 2^(dtAlphaExp)
            int m_dtInitialAlpha;               //Initial Alpha 
            int m_sharedBufferSize;             //bytes
            int m_packetDropNum;                //packet Drop Num
            int m_packetEnqueueNum;             //packet DoEnqueue Num
            int m_packetDequeueNum;             //packet DoEnqueue Num        
            int m_enqueueLength;                //enqueue length
            int m_lastEnqueueLength;            //temp enqueue length
            int m_dequeueLength;                //dequeue length
            int m_lastDequeueLength;            //temp dequeue length
            int m_packetArriveSize;             //packet arrive size
            int m_packetNum;                    //每几个算一个速率

            //EDT 
            int m_C1 = 0;                       //欠载计数器
            int m_C2 = 0;                       //过载计数器
            int64_t m_T1 = 0;                   //计时器1
            int64_t m_T2 = 0;                   //计时器2

            //TDT
            int m_OC1 = 0;                      //OC1
            int m_DEC = 0;                      //计算DEC
            bool m_isTDTIncast = true;          //OC1

            int64_t m_startTime;                //记录开始时间

            //时间变量：ms
            int64_t m_enqueueClock;             //enqueue clock
            int64_t m_dequeueClock;             //dequeue clock
            int64_t m_enqueueInterval;          //enqueue interval
            int64_t m_dequeueInterval;          //dequeue interval

            bool m_isTrafficExist = false;      //流量是否存在
            bool m_isQueueShort = false;        //队列是否很短  
            bool m_isDropPacket = false;        //AASDT是否丢包超过阈值       

            double m_threshold;                 //阈值
            double m_enqueueRate;               // enqueue rate B/MS
            double m_lastEnqueueRate = 0.0;     // enqueue rate B/MS
            double m_dequeueRate;               // dequeue rate B/MS

            //EVERY STATE PORT NUM POINTER
            Ptr<UintegerValue> m_PortNumPtr;
            //EDT
            Ptr<UintegerValue> m_EDTCPortNumPtr;
            Ptr<UintegerValue> m_EDTNCPortNumPtr;
            //TDT
            Ptr<UintegerValue> m_TDTNPortNumPtr;
            Ptr<UintegerValue> m_TDTAPortNumPtr;
            Ptr<UintegerValue> m_TDTEPortNumPtr;
            //AASDT
            Ptr<UintegerValue> m_AASDTNPortNumPtr;
            Ptr<UintegerValue> m_AASDTIPortNumPtr;
            Ptr<UintegerValue> m_AASDTCPortNumPtr;
            Ptr<UintegerValue> m_AASDTCIPortNumPtr;
            Ptr<UintegerValue> m_AASDTCCPortNumPtr;


            Ptr<UintegerValue> m_AASDTITimePtr; //突发次数
            Ptr<UintegerValue> m_AASDTCTimePtr; //拥塞次数 

            Ptr<UintegerValue> m_stateChangePtr;        //state Change

            Ptr<UintegerValue> m_usedBufferPtr;         // point to used buffer size in a node
            
    };

}
#endif
