// #SCQAD TAG: forte.pdupeer
#ifndef __Forte__PDU_h_
#define __Forte__PDU_h_

#include "Exception.h"
#include "FString.h"
#include "LogManager.h"
#include <boost/make_shared.hpp>
#include <boost/shared_array.hpp>

EXCEPTION_CLASS(EPDU);

namespace Forte
{
    struct PDUHeader
    {
        unsigned int version;
        unsigned int opcode;
        unsigned int payloadSize;
    } __attribute__((__packed__));

    class PDU
    {
      public:
        const static unsigned int PDU_VERSION = 1;

        PDU(int op = 0, size_t size = 0, const void *data = NULL)
        {
            mHeader.version = PDU_VERSION;
            mHeader.opcode = op;
            mHeader.payloadSize = size;
            mPayload.reset();
            SetPayload(mHeader.payloadSize, data);
        }

        PDU(const PDU& pdu) {
            mHeader.version = pdu.mHeader.version;
            mHeader.opcode = pdu.mHeader.opcode;
            mHeader.payloadSize = pdu.mHeader.payloadSize;
            mPayload.reset();
            SetPayload(pdu.mHeader.payloadSize, pdu.mPayload.get());
        }

        static boost::shared_array<char> CreateSendBuffer(const Forte::PDU &pdu);
        unsigned int GetVersion(void) const { return mHeader.version; }
        unsigned int GetOpcode(void) const { return mHeader.opcode; }
        unsigned int GetPayloadSize(void) const { return mHeader.payloadSize; }

        void SetHeader(const PDUHeader &header);
        void SetPayload(const unsigned int len, const void *data = NULL);

        template <typename PayloadType>
            static bool PayloadIsEqual(Forte::PDU *a, Forte::PDU *b)
        {
            if (a == NULL || b == NULL)
                throw EPDU("Cannot pass NULL to ComparePayload");
            const PayloadType *aPayload = a->GetPayload<PayloadType>();
            const PayloadType *bPayload = b->GetPayload<PayloadType>();
            return *aPayload == *bPayload;
        }

        template <typename PayloadType>
            PayloadType *GetPayload(void) const
        {
            return reinterpret_cast<PayloadType*>(mPayload.get());
        }

        bool operator==(const PDU &other) const;

      protected:
        PDUHeader mHeader;
        boost::shared_array<char> mPayload;
    };
    std::ostream& operator<<(std::ostream& os, const PDU &obj);
};

#endif
