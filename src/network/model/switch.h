#ifndef SWITCH_H
#define SWITCH_H

#include "ns3/object.h"
#include "ns3/ptr.h"
#include "ns3/uinteger.h"
#include "iostream"
#include "ns3/nstime.h"
#include "ns3/core-module.h"

namespace ns3 {
    class Switch : public Object{
        public:
            static TypeId GetTypeId (void);
            Switch();
            virtual ~Switch();
            int GetThreshold();
            int GetPacketDropNum();
            int GetSharedBufferSize();
            int GetAvailBufferSize();
            int GetPacketEnqueueNum();
            int GetPacketDequeueNum();
            int GetAASDTITime();
            int GetAASDTCTime();
            int GetEnQueueLength();
            int GetDeQueueLength();
            int64_t GetNowTime(); //ms
            void SetdtAlphaExp(int alphaExp);
            void Setstrategy(int strategy);
            void SetUsedBufferPtr(Ptr<UintegerValue> usedBufferPtr);
            void SetSharedBufferSize(int sharedBufferSize);
            void SetPortNumPtr(Ptr<UintegerValue> PortNumPtr);
            void SetEDTPortNumPtr(Ptr<UintegerValue> EDTCPortNumPtr,Ptr<UintegerValue> EDTNCPortNumPtr);
            void SetTDTPortNumPtr(Ptr<UintegerValue> TDTNPortNumPtr,Ptr<UintegerValue> TDTAPortNumPtr,Ptr<UintegerValue> TDTEPortNumPtr);
            void SetAASDTPortNumPtr(Ptr<UintegerValue> AASDTNPortNumPtr,Ptr<UintegerValue> AASDTIPortNumPtr,Ptr<UintegerValue> AASDTCPortNumPtr,Ptr<UintegerValue> AASDTCIPortNumPtr,Ptr<UintegerValue> AASDTCCPortNumPtr);
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
            void AASDTReset(int alphaExp);

            int m_EDTstate = 10;                  //10:控制；11:非控制;
            int m_TDTstate = 20;                  //20:正常；21:吸收;22:疏散
            int m_AASDTstate = 30;                //30:正常；31:突发;32:拥塞;33:共存1;34:共存2
            

        private:
            int m_strategy = 0;                 //0:DT;1:EDT;2:TDT;3:AASDT
        	double m_threshold;
            int m_dtAlphaExp;                   //alpha = 2^(dtAlphaExp) 
            int m_sharedBufferSize;             //bytes
            int m_packetDropNum;                //packet Drop Num
            int m_packetEnqueueNum;             //packet DoEnqueue Num
            int m_packetDequeueNum;             //packet DoEnqueue Num
            int m_enQueueLength;                //enqueuelength
            int m_deQueueLength;                //dequeuelength
            int m_packetArriveSize;             //packet arrive size

            bool m_isTrafficArrive = false;     //流量是否到达
            bool m_isTrafficLeave = false;      //流量是否离开

            int m_AASDTITime = 0;               //突发次数
            int m_AASDTCTime = 0;               //拥塞次数   

            int64_t m_stateTransitionTimer;     //记录超时时间

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

            Ptr<UintegerValue> m_usedBufferPtr; // point to used buffer size in a node
            
    };

}
#endif
