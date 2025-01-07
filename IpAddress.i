%include "common-cxx/ip.i"

%nodefaultctor IpAddress;

%rename (IpAddressSwig) IpAddress;

class IpAddress {
public:
    IpAddress(const unsigned char ipAddress[], fiftyoneDegreesEvidenceIpType type);
    IpAddress(const char *ipAddressString);
    void getCopyOfIpAddress(unsigned char copy[], uint32_t size);
    fiftyoneDegreesEvidenceIpType getType();
};
