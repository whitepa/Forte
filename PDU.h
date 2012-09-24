#ifndef __Forte__PDU_h_
#define __Forte__PDU_h_

namespace Forte
{
    struct PDU
    {
        const static unsigned int PDU_VERSION = 0;
        const static unsigned int PDU_MAX_PAYLOAD = 3968;
        const static unsigned int PDU_SIZE = 4096;

        PDU() :
            version(PDU_VERSION),
            opcode(0),
            payloadSize(0) { memset(payload, 0, sizeof(payload)); }
        PDU(int op, size_t size) :
            version(PDU_VERSION),
            opcode(op),
            payloadSize(size) { memset(payload, 0, sizeof(payload)); }
        PDU(int op, size_t size, void *data) :
            version(PDU_VERSION),
            opcode(op),
            payloadSize(size) {
            memset(payload, 0, sizeof(payload));
            memcpy(payload, data, size);
        }

        unsigned int version;
        unsigned int opcode;
        unsigned int payloadSize;
        char payload[PDU_MAX_PAYLOAD];
    } __attribute__((__packed__));
};

#endif
