// #SCQAD TAG: forte.pdupeer
#ifndef __Forte__PDU_h_
#define __Forte__PDU_h_

#include "Exception.h"
#include "FString.h"
#include "LogManager.h"
#include <boost/make_shared.hpp>
#include <boost/shared_array.hpp>

/* PDU
 *
 * Usage:
 *
 * PDU is a general purpose data transport mechanism. For specific use
 * cases, you will generally create a set of opcodes and packed
 * structs to transfer acroos the wire. PDUPeer takes advantage of an
 * assumption that computers on both sides will have the same
 * endianess and architecture, so incoming PDUs can be
 * reinterpret_cast'ed based on their wire format, removing the need
 * serialize and deserialize on either end.
 *
 * PDUs can have a dynamic payload size. Applications can use this to
 * pack any structure into a PDU and send it over the wire. Dynamic
 * sized payloads can be used by any application, but packing and
 * unpacking of that structure is left to the application, and simply
 * reinterpret_casting the payload becomes more difficult. Due to
 * this, an optional dynamic size data chunk is available, as a common
 * use case will be to transfer some information with a single, large,
 * variable sized buffer. For that use case, the optionalData field is
 * provided.
 *
 * A specialization of the general optional data use case is writing
 * data to disk using libaio. In that situation, the data ultimately
 * needs to be memaligned to a particular buffer size. Alignment to
 * 512 bytes is provided, other alignments could be added easily.
 *
 */

EXCEPTION_CLASS(EPDU);
EXCEPTION_SUBCLASS2(EPDU, EPDUUnexpectedNullBuffer, "Expected non null buffer");

#define PDU_OPTIONAL_DATA_ATTRIBUTE_MEMALIGN_512  0x00000001

namespace Forte
{
    struct PDUHeader
    {
        PDUHeader()
            : version(0),
              opcode(0),
              payloadSize(0),
              optionalDataSize(0),
              optionalDataAttributes(0)
            {}

        unsigned int version;
        unsigned int opcode;
        unsigned int payloadSize;
        unsigned int optionalDataSize;
        unsigned int optionalDataAttributes;
        //TODO: checksum on header
    } __attribute__((__packed__));

    struct PDUOptionalData
    {
        PDUOptionalData(
            const unsigned int size,
            const unsigned int attributes,
            const void* data = NULL
            )
            : mSize(size),
              mAttributes(attributes),
              mData(NULL)
            {
                if (mSize > 0)
                {
                    if (attributes & PDU_OPTIONAL_DATA_ATTRIBUTE_MEMALIGN_512)
                    {
                        void* ptr(0);
                        if (posix_memalign(&ptr, 512, size) != 0)
                        {
                            hlog_and_throw(HLOG_WARN, std::bad_alloc());
                        }
                        mData = ptr;
                    }
                    else
                    {
                        mData = malloc(size);
                        if (mData == NULL)
                        {
                            hlog_and_throw(HLOG_WARN, std::bad_alloc());
                        }
                    }

                    CopyToDataBuffer(data);
                }
            }

        ~PDUOptionalData() {
            if (mData != NULL)
            {
                free(mData);
            }
        }

        void CopyToDataBuffer(const void* data) {
            if (data != NULL)
            {
                memcpy(mData, data, mSize);
            }
            else
            {
                memset(mData, 0, mSize);
            }
        }

        const void* GetData() const {
            return mData;
        }

        unsigned int GetSize() const {
            return mSize;
        }

        PDUOptionalData(const PDUOptionalData& other) = delete;
        const PDUOptionalData& operator=(const PDUOptionalData& rhs) = delete;

        unsigned int mSize;
        unsigned int mAttributes;
        void* mData;
    };

    class PDU
    {
      public:
        const static unsigned int PDU_VERSION = 2;

        PDU(int op = 0,
            size_t size = 0,
            const void *data = NULL,
            unsigned int payloadVersion = 0)
        {
            mHeader.version = CalculatePDUVersion(PDU_VERSION, payloadVersion);
            mHeader.opcode = op;
            mHeader.payloadSize = size;

            SetPayload(mHeader.payloadSize, data);
        }

        PDU(const PDU& pdu) {
            mHeader.version = pdu.mHeader.version;
            mHeader.opcode = pdu.mHeader.opcode;
            mHeader.payloadSize = pdu.mHeader.payloadSize;

            mPayload.reset();
            SetPayload(pdu.mHeader.payloadSize, pdu.mPayload.get());

            mHeader.optionalDataSize = pdu.mHeader.optionalDataSize;
            mHeader.optionalDataAttributes = pdu.mHeader.optionalDataAttributes;

            if (pdu.mOptionalData)
            {
                mOptionalData.reset(
                    new PDUOptionalData(pdu.mHeader.optionalDataSize,
                                        pdu.mHeader.optionalDataAttributes));

                if (pdu.mOptionalData->mData != NULL)
                {
                    memcpy(mOptionalData->mData,
                           pdu.mOptionalData->mData,
                           pdu.mHeader.optionalDataSize);
                }
            }
        }

        static size_t Size(const PDUHeader& header) {
            return sizeof(PDUHeader)
                + header.payloadSize
                + header.optionalDataSize;
        }

        static boost::shared_array<char> CreateSendBuffer(const Forte::PDU &pdu);
        static unsigned int CalculatePDUVersion(unsigned int baseVersion,
                                                unsigned int payloadVersion) {
            return baseVersion | (payloadVersion << 16);
        }
        static unsigned int GetBasePDUVersion(unsigned int version) {
            unsigned int v = version << 16;
            return (v >>= 16);
        }
        static unsigned int GetPayloadVersion(unsigned int version) {

            return version >> 16;
        }

        PDUHeader GetHeader(void) const { return mHeader; }
        unsigned int GetVersion(void) const { return mHeader.version; }
        unsigned int GetOpcode(void) const { return mHeader.opcode; }
        unsigned int GetPayloadSize(void) const { return mHeader.payloadSize; }
        unsigned int GetOptionalDataSize(void) const {
            return mHeader.optionalDataSize;
        }

        void SetHeader(const PDUHeader &header);
        void SetPayload(const unsigned int len, const void *data);
        void SetPayloadVersion(const unsigned int payloadVersion) {
            mHeader.version =
                CalculatePDUVersion(
                    GetBasePDUVersion(mHeader.version),
                    payloadVersion);
        }

        void SetOptionalData(
            const boost::shared_ptr<PDUOptionalData>& optionalData);

        const boost::shared_ptr<PDUOptionalData>& GetOptionalData() const;

        template <typename PayloadType>
            static bool PayloadIsEqual(Forte::PDU *a, Forte::PDU *b)
        {
            if (a == NULL || b == NULL)
                throw EPDU("Cannot pass NULL to ComparePayload");
            const PayloadType *aPayload = a->GetPayload<PayloadType>();
            const PayloadType *bPayload = b->GetPayload<PayloadType>();

            return (*aPayload == *bPayload);
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
        boost::shared_ptr<PDUOptionalData> mOptionalData;
    };

    std::ostream& operator<<(std::ostream& os, const PDU &obj);
    std::ostream& operator<<(std::ostream& os, const PDUHeader &obj);
};

#endif
