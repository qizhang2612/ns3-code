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
            void SetdtAlphaExp(int32_t AlphaExp);
            void SetUsedBufferPtr(Ptr<UintegerValue> usedBufferPtr);
            void SetSharedBufferSize(uint64_t sharedBufferSize);
            void DeleteUsed(int size);
            void AddUsed(int size);
            void Calculate();

            int EDTstate = 0;            //0:控制；1:非控制;
            int TDTstate = 0;            //0:正常；1:吸收;2:疏散
            int AASDTstate = 0;          //0:正常；1:突发;2:共存1;3:共存2
            int strategy = 0;            //0:DT;1:EDT;2:TDT;3:AASDT

        private:
        	double m_threshold;
            int32_t m_dtAlphaExp;        //alpha = 2^(dtAlphaExp) 
            uint64_t m_sharedBufferSize; //bytes

            Ptr<UintegerValue> m_usedBufferPtr; // point to used buffer size in a node
            
    };

}
#endif
