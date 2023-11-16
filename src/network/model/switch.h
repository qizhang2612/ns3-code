#ifndef SWITCH_H
#define SWITCH_H

#include "ns3/object.h"
#include "ns3/ptr.h"
#include "ns3/uinteger.h"
#include "iostream"

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
            void SetdtAlphaExp(int alphaExp);
            void Setstrategy(int strategy);
            void SetUsedBufferPtr(Ptr<UintegerValue> usedBufferPtr);
            void SetSharedBufferSize(int sharedBufferSize);
            void AddPacketDropNum();
            void AddPacketEnqueueNum();
            void AddUsed(int size);
            void DeleteUsed(int size);
            void Calculate();
            void StatusJudgment();

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

            //EVERY STATE PORT NUM POINTER
            Ptr<UintegerValue> m_PortNum;
            Ptr<UintegerValue> m_EDTCPortNum;
            Ptr<UintegerValue> m_EDTNCPortNum;
            Ptr<UintegerValue> m_TDTNPortNum;
            Ptr<UintegerValue> m_TDTAPortNum;
            Ptr<UintegerValue> m_TDTEPortNum;
            Ptr<UintegerValue> m_AASDTNPortNum;
            Ptr<UintegerValue> m_AASDTIPortNum;
            Ptr<UintegerValue> m_AASDTCPortNum;
            Ptr<UintegerValue> m_AASDTCIPortNum;
            Ptr<UintegerValue> m_AASDTCCPortNum;

            Ptr<UintegerValue> m_usedBufferPtr; // point to used buffer size in a node
            
    };

}
#endif
