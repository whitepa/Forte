#include "PDU.h"

using namespace std;
using namespace Forte;

boost::shared_array<char> PDU::CreateSendBuffer(const PDU &pdu) {
    boost::shared_array<char> res;
    unsigned int len = sizeof(Forte::PDUHeader) + pdu.mHeader.payloadSize;
    char *buf = new char[len];
    memset(buf, 0, len);
    memcpy(buf, &pdu.mHeader, sizeof(Forte::PDUHeader));
    memcpy(buf+sizeof(Forte::PDUHeader), pdu.mPayload.get(),
           pdu.mHeader.payloadSize);
    res.reset(buf);
    return res;
}

void PDU::SetHeader(const PDUHeader &header)
{
    memcpy(&mHeader, &header, sizeof(PDUHeader));
}

void PDU::SetPayload(const unsigned int len, const void *data)
{
    mHeader.payloadSize = len;
    if (len)
    {
        char *newPayload = new char[mHeader.payloadSize];
        memset(newPayload, 0, mHeader.payloadSize);
        if (data)
            memcpy(newPayload, data, mHeader.payloadSize);
        mPayload.reset(newPayload);
    }
    else
    {
        mPayload.reset();
    }
}

bool PDU::operator==(const PDU &other) const
{
    bool res = (mHeader.version == other.mHeader.version &&
                mHeader.opcode == other.mHeader.opcode &&
                mHeader.payloadSize == other.mHeader.payloadSize);
    hlogstream(HLOG_DEBUG2, "A(" << this << ") vs B(" << &other << ")" <<
               endl << *this << endl << other << endl);
    if (res && mHeader.payloadSize &&
        mPayload.get() != NULL &&
        other.mPayload.get() != NULL)
    {
        res = res &&
            (memcmp(mPayload.get(), other.mPayload.get(),
                    mHeader.payloadSize) == 0);
    }
    return res;
}

std::ostream& Forte::operator<<(std::ostream& os, const PDU &obj)
{

    os << "PDU " << &obj << endl;
    os << obj.GetHeader();
    unsigned int payloadSize = obj.GetPayloadSize();
    if (payloadSize)
    {
        void *payload = obj.GetPayload<void>();
        if (payload != NULL)
        {
            FString str;
            str.assign((const char*)payload, payloadSize);
            os << "Payload = |" << str.HexDump().c_str() << "|" << endl;
        }
    }
    return os;
}

std::ostream& Forte::operator<<(std::ostream& os, const PDUHeader &obj)
{
    os << "PDUHeader" << endl;
    os << "\tversion\t\t= " << obj.version << endl;
    os << "\topcode\t\t= " << obj.opcode << endl;
    os << "\tpayloadSize\t= " << obj.payloadSize << endl;
    return os;
}

