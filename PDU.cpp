#include "PDU.h"

using namespace std;
using namespace Forte;

boost::shared_array<char> PDU::CreateSendBuffer(const PDU &pdu)
{
    boost::shared_array<char> res;
    unsigned int len =
        sizeof(Forte::PDUHeader)
        + pdu.mHeader.payloadSize
        + pdu.mHeader.optionalDataSize;

    char *buf = new char[len];
    memset(buf, 0, len);
    // header
    memcpy(buf, &pdu.mHeader, sizeof(Forte::PDUHeader));

    // payload
    memcpy(buf+sizeof(Forte::PDUHeader),
           pdu.mPayload.get(),
           pdu.mHeader.payloadSize);

    if (pdu.mHeader.optionalDataSize > 0)
    {
        memcpy(buf+sizeof(Forte::PDUHeader)+pdu.mHeader.payloadSize,
               pdu.mOptionalData->mData,
               pdu.mHeader.optionalDataSize);
    }
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
    if (len > 0)
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

void PDU::SetOptionalData(
    const boost::shared_ptr<PDUOptionalData>& optionalData)
{
    mOptionalData = optionalData;
    if (optionalData)
    {
        mHeader.optionalDataSize = mOptionalData->mSize;
        mHeader.optionalDataAttributes = mOptionalData->mAttributes;
    }
    else
    {
        mHeader.optionalDataSize = 0;
        mHeader.optionalDataAttributes = 0;
    }
}


const boost::shared_ptr<PDUOptionalData>& PDU::GetOptionalData() const
{
    return mOptionalData;
}

bool PDU::operator==(const PDU &other) const
{
    bool res =
        mHeader.version == other.mHeader.version
        && mHeader.opcode == other.mHeader.opcode
        && mHeader.payloadSize == other.mHeader.payloadSize
        && mHeader.optionalDataSize == other.mHeader.optionalDataSize
        && mHeader.optionalDataAttributes == other.mHeader.optionalDataAttributes
        ;

    hlogstream(HLOG_DEBUG2, "A(" << this << ") vs B(" << &other << ")" <<
               endl << *this << endl << other << endl);

    // what happens here if only 1 payload is NULL? seems like we
    // would falsely reutrn true, but also that is a different error
    // where the size is gt 0 but the payload is NULL
    if (res
        && mHeader.payloadSize
        && mPayload.get() != NULL
        && other.mPayload.get() != NULL)
    {
        res = res
            && (memcmp(mPayload.get(),
                       other.mPayload.get(),
                       mHeader.payloadSize) == 0);
    }

    if (res
        && mHeader.optionalDataSize
        && mOptionalData.get() != NULL
        && other.mOptionalData.get() != NULL)
    {
        res = res
            && (memcmp(mOptionalData->mData,
                       other.mOptionalData->mData,
                       mHeader.optionalDataSize) == 0);
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
            str.assign(
                reinterpret_cast<const char*>(const_cast<const void*>(payload)),
                std::min(payloadSize, static_cast<unsigned int>(255)));
            os << "Payload = "
               << "[" << payloadSize << "]" << endl
               << "|" << str.HexDump().c_str() << "|" << endl;
        }
    }

    unsigned int optionalDataSize = obj.GetOptionalDataSize();
    if (optionalDataSize)
    {
        const boost::shared_ptr<PDUOptionalData>&
            optionalData = obj.GetOptionalData();

        if (optionalData && optionalData->GetData() != NULL)
        {
            FString str;
            str.assign(
                reinterpret_cast<const char*>(optionalData->GetData()),
                std::min(optionalDataSize, static_cast<unsigned int>(255)));

            os << "OptionalData = "
               << "[" << optionalDataSize << "]" << endl
               << "|" << str.HexDump().c_str() << "|" << endl;
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
    os << "\toptionalDataSize\t= " << obj.optionalDataSize << endl;
    os << "\toptionalDataAttributes\t= " << obj.optionalDataAttributes << endl;
    return os;
}

