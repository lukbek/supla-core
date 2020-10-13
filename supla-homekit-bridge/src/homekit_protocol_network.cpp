//
//  PHKNetworkIP.cpp
//  Workbench
//
//  Created by Wai Man Chan on 4/8/14.
//
//

#include "homekit_protocol_network.h"
#define PHKNetworkServiceType "_hap._tcp"

#include <errno.h>
#include <strings.h>
#include <fstream>
#include <iostream>

#include <netdb.h>

void (*newConnection)(connectionInfo *info) = 0;
void (*deadConnection)(connectionInfo *info) = 0;

//#include <byteswap.h>
static inline unsigned short bswap_16(unsigned short x) {
  return (x >> 8) | (x << 8);
}

static inline unsigned int bswap_32(unsigned int x) {
  return (bswap_16(x & 0xffff) << 16) | (bswap_16(x >> 16));
}
static inline unsigned long long bswap_64(unsigned long long x) {
  return x;  //(((unsigned long long)bswap_32(x&0xffffffffull))<<32) |
  //    (bswap_32(x>>32));
}

// Security
extern "C" {
// ChaCha20-Poly1035
#include "security/chacha20_simple.h"
#include "security/poly1305.h"
// HKDF-SHA512
#include "security/hkdf.h"
// ED25519
#include "security/ed25519.h"
// Curve25519
#include "security/curve25519-donna.h"
// SRP
#include "security/srp.h"
}

// Store controller public key
#include "homekit_controller_record.h"

// Handle Data
#include "homekit_protocol_accessory.h"

#include "Configuration.h"

#include <pthread.h>

using namespace std;

#define portNumber 0

connectionInfo connection[numberOfClient];

const unsigned char modulusStr[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC9, 0x0F, 0xDA, 0xA2,
    0x21, 0x68, 0xC2, 0x34, 0xC4, 0xC6, 0x62, 0x8B, 0x80, 0xDC, 0x1C, 0xD1,
    0x29, 0x02, 0x4E, 0x08, 0x8A, 0x67, 0xCC, 0x74, 0x02, 0x0B, 0xBE, 0xA6,
    0x3B, 0x13, 0x9B, 0x22, 0x51, 0x4A, 0x08, 0x79, 0x8E, 0x34, 0x04, 0xDD,
    0xEF, 0x95, 0x19, 0xB3, 0xCD, 0x3A, 0x43, 0x1B, 0x30, 0x2B, 0x0A, 0x6D,
    0xF2, 0x5F, 0x14, 0x37, 0x4F, 0xE1, 0x35, 0x6D, 0x6D, 0x51, 0xC2, 0x45,
    0xE4, 0x85, 0xB5, 0x76, 0x62, 0x5E, 0x7E, 0xC6, 0xF4, 0x4C, 0x42, 0xE9,
    0xA6, 0x37, 0xED, 0x6B, 0x0B, 0xFF, 0x5C, 0xB6, 0xF4, 0x06, 0xB7, 0xED,
    0xEE, 0x38, 0x6B, 0xFB, 0x5A, 0x89, 0x9F, 0xA5, 0xAE, 0x9F, 0x24, 0x11,
    0x7C, 0x4B, 0x1F, 0xE6, 0x49, 0x28, 0x66, 0x51, 0xEC, 0xE4, 0x5B, 0x3D,
    0xC2, 0x00, 0x7C, 0xB8, 0xA1, 0x63, 0xBF, 0x05, 0x98, 0xDA, 0x48, 0x36,
    0x1C, 0x55, 0xD3, 0x9A, 0x69, 0x16, 0x3F, 0xA8, 0xFD, 0x24, 0xCF, 0x5F,
    0x83, 0x65, 0x5D, 0x23, 0xDC, 0xA3, 0xAD, 0x96, 0x1C, 0x62, 0xF3, 0x56,
    0x20, 0x85, 0x52, 0xBB, 0x9E, 0xD5, 0x29, 0x07, 0x70, 0x96, 0x96, 0x6D,
    0x67, 0x0C, 0x35, 0x4E, 0x4A, 0xBC, 0x98, 0x04, 0xF1, 0x74, 0x6C, 0x08,
    0xCA, 0x18, 0x21, 0x7C, 0x32, 0x90, 0x5E, 0x46, 0x2E, 0x36, 0xCE, 0x3B,
    0xE3, 0x9E, 0x77, 0x2C, 0x18, 0x0E, 0x86, 0x03, 0x9B, 0x27, 0x83, 0xA2,
    0xEC, 0x07, 0xA2, 0x8F, 0xB5, 0xC5, 0x5D, 0xF0, 0x6F, 0x4C, 0x52, 0xC9,
    0xDE, 0x2B, 0xCB, 0xF6, 0x95, 0x58, 0x17, 0x18, 0x39, 0x95, 0x49, 0x7C,
    0xEA, 0x95, 0x6A, 0xE5, 0x15, 0xD2, 0x26, 0x18, 0x98, 0xFA, 0x05, 0x10,
    0x15, 0x72, 0x8E, 0x5A, 0x8A, 0xAA, 0xC4, 0x2D, 0xAD, 0x33, 0x17, 0x0D,
    0x04, 0x50, 0x7A, 0x33, 0xA8, 0x55, 0x21, 0xAB, 0xDF, 0x1C, 0xBA, 0x64,
    0xEC, 0xFB, 0x85, 0x04, 0x58, 0xDB, 0xEF, 0x0A, 0x8A, 0xEA, 0x71, 0x57,
    0x5D, 0x06, 0x0C, 0x7D, 0xB3, 0x97, 0x0F, 0x85, 0xA6, 0xE1, 0xE4, 0xC7,
    0xAB, 0xF5, 0xAE, 0x8C, 0xDB, 0x09, 0x33, 0xD7, 0x1E, 0x8C, 0x94, 0xE0,
    0x4A, 0x25, 0x61, 0x9D, 0xCE, 0xE3, 0xD2, 0x26, 0x1A, 0xD2, 0xEE, 0x6B,
    0xF1, 0x2F, 0xFA, 0x06, 0xD9, 0x8A, 0x08, 0x64, 0xD8, 0x76, 0x02, 0x73,
    0x3E, 0xC8, 0x6A, 0x64, 0x52, 0x1F, 0x2B, 0x18, 0x17, 0x7B, 0x20, 0x0C,
    0xBB, 0xE1, 0x17, 0x57, 0x7A, 0x61, 0x5D, 0x6C, 0x77, 0x09, 0x88, 0xC0,
    0xBA, 0xD9, 0x46, 0xE2, 0x08, 0xE2, 0x4F, 0xA0, 0x74, 0xE5, 0xAB, 0x31,
    0x43, 0xDB, 0x5B, 0xFC, 0xE0, 0xFD, 0x10, 0x8E, 0x4B, 0x82, 0xD1, 0x20,
    0xA9, 0x3A, 0xD2, 0xCA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
const unsigned char curveBasePoint[] = {
    0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const unsigned char generator[] = {0x05};

char tempStr[3073];

const unsigned char accessorySecretKey[32] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC9, 0x0F, 0xDA,
    0xA2, 0x21, 0x68, 0xC2, 0x34, 0xC4, 0xC6, 0x62, 0x8B, 0x80, 0xDC,
    0x1C, 0xD1, 0x29, 0x02, 0x4E, 0x08, 0x8A, 0x67, 0xCC, 0x74};

int _socket_v4, _socket_v6;

DNSServiceRef netServiceV4, netServiceV6;

deviceType currentDeviceType = deviceType_other;

int currentConfigurationNum = 1;

void close_socket() { close(_socket_v4); }

int is_big_endian(void) {
  union {
    uint32_t i;
    char c[4];
  } e = {0x01000000};

  return e.c[0];
}

// Network Setup
int setupSocketV4(unsigned int maximumConnection) {
  int _socket = socket(PF_INET, SOCK_STREAM, 0);
  sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_family = PF_INET;
  addr.sin_port = htons(portNumber);

  int optval = 1;
  socklen_t optlen = sizeof(optval);
  setsockopt(_socket, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen);

  bind(_socket, (const struct sockaddr *)&addr, sizeof(addr));
  int result = listen(_socket, maximumConnection);
  return _socket;
}

int setupSocketV6(unsigned int maximumConnection) {
  int _socket = socket(PF_INET6, SOCK_STREAM, 0);
  sockaddr_in6 addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin6_addr = in6addr_any;
  addr.sin6_family = PF_INET6;
  addr.sin6_port = htons(portNumber);
  bind(_socket, (const struct sockaddr *)&addr, sizeof(addr));
  listen(_socket, maximumConnection);
  return _socket;
}

unsigned short getSocketPortNumberV4(int _socket) {
  sockaddr_in addr;
  socklen_t len = sizeof(addr);
  getsockname(_socket, (struct sockaddr *)&addr, &len);
  return ntohs(addr.sin_port);
}

unsigned short getSocketPortNumberV6(int _socket) {
  sockaddr_in6 addr;
  socklen_t len = sizeof(addr);
  getsockname(_socket, (struct sockaddr *)&addr, &len);
  return ntohs(addr.sin6_port);
}

void registerFail(DNSServiceRef sdRef, DNSRecordRef RecordRef,
                  DNSServiceFlags flags, DNSServiceErrorType errorCode,
                  void *context) {
  supla_log(LOG_DEBUG, "homekit service can't register");
  exit(0);
}

TXTRecordRef buildTXTRecord() {
  TXTRecordRef txtRecord;
  TXTRecordCreate(&txtRecord, 0, NULL);
  TXTRecordSetValue(&txtRecord, "pv", 3, "1.1");  // Version
  TXTRecordSetValue(
      &txtRecord, "id", 17,
      Configuration::Instance().getDeviceIdentity().c_str());  // Device id
  char buf[3];
  sprintf(buf, "%d", 1);
  TXTRecordSetValue(&txtRecord, "c#", 1, buf);  // Configuration Number
  TXTRecordSetValue(&txtRecord, "s#", 1, "1");  // Number of service
  if (hasController())
    buf[0] = '0';
  else
    buf[0] = '1';
  TXTRecordSetValue(&txtRecord, "sf", 1,
                    buf);  // Discoverable: 0 if has been paired
  TXTRecordSetValue(&txtRecord, "ff", 1, "0");  // 1 for MFI product

  std::string deviceName = Configuration::Instance().getDeviceName();

  TXTRecordSetValue(&txtRecord, "md", deviceName.length(),
                    deviceName.c_str());  // Model Name
  int len = sprintf(buf, "%d", 2);
  TXTRecordSetValue(&txtRecord, "ci", len, buf);  // 1 for MFI product
  return txtRecord;
}

void updatePairable() {
  TXTRecordRef txtRecord = buildTXTRecord();
  DNSServiceUpdateRecord(netServiceV4, NULL, 0, TXTRecordGetLength(&txtRecord),
                         TXTRecordGetBytesPtr(&txtRecord), 0);
  TXTRecordDeallocate(&txtRecord);
}

void updateConfiguration() {
  currentConfigurationNum++;
  TXTRecordRef txtRecord = buildTXTRecord();
  DNSServiceUpdateRecord(netServiceV4, NULL, 0, TXTRecordGetLength(&txtRecord),
                         TXTRecordGetBytesPtr(&txtRecord), 0);
  TXTRecordDeallocate(&txtRecord);
}

void PHKNetworkIP::setupSocket() {
  if (netServiceV4) {
    supla_log(LOG_WARNING, "Already registered a service");
    return;
  }

  TXTRecordRef txtRecord = buildTXTRecord();
  _socket_v4 = setupSocketV4(numberOfClient);

  auto error = DNSServiceRegister(
      &netServiceV4, 0, 0, Configuration::Instance().getDeviceName().c_str(),
      PHKNetworkServiceType, "", NULL, htons(getSocketPortNumberV4(_socket_v4)),
      TXTRecordGetLength(&txtRecord), TXTRecordGetBytesPtr(&txtRecord), NULL,
      NULL);

  if (error != kDNSServiceErr_NoError) {
    supla_log(LOG_DEBUG, "DNSServiceRegister error %d", error);
  } else {
    supla_log(LOG_DEBUG, "DNSService registered");
  }

  if (!netServiceV4) {
    supla_log(LOG_DEBUG, "DNSService reference is null!");
  }

  TXTRecordDeallocate(&txtRecord);
}

PHKNetworkIP::PHKNetworkIP() {
  SRP_initialize_library();

  srand((unsigned int)time(NULL));

  for (int i = 0; i < numberOfClient; i++) {
    connection[i].subSocket = -1;
  }
  setupSocket();
}

// Broadcast a event message, use for notifcation or keep alive
void broadcastMessage(void *sender, char *resultData, size_t resultLen) {
  for (int i = 0; i < numberOfClient; i++) {
    int socketNumber = connection[i].subSocket;

    if (socketNumber >= 0 && connection[i].connected &&
        connection[i].numberOfMsgSend >= 0 &&
        (connection[i].notify(sender) || sender == NULL)) {
      supla_log(LOG_DEBUG, "broadcast with sender a %d message to client %d\n",
                resultLen, socketNumber);

      pthread_mutex_lock(&connection[i].mutex);

      chacha20_ctx chacha20;
      memset(&chacha20, 0, sizeof(chacha20));

      char temp[64] = {0};
      char temp2[64] = {0};
      unsigned char *reply = new unsigned char[resultLen + 18];
      reply[0] = resultLen % 256;
      reply[1] = (resultLen - (uint8_t)reply[0]) / 256;

      unsigned long long numberOfMsgSend = connection[i].numberOfMsgSend;
      if (!is_big_endian())
        numberOfMsgSend = bswap_64(connection[i].numberOfMsgSend);
      chacha20_setup(&chacha20,
                     (const uint8_t *)connection[i].accessoryToControllerKey,
                     32, (uint8_t *)&numberOfMsgSend);
      connection[i].numberOfMsgSend++;

      chacha20_encrypt(&chacha20, (const uint8_t *)temp, (uint8_t *)temp2, 64);
      chacha20_encrypt(&chacha20, (const uint8_t *)resultData,
                       (uint8_t *)&reply[2], resultLen);

      char verify[16];
      memset(verify, 0, 16);
      connection[i].Poly1305_GenKey((const unsigned char *)temp2,
                                    (uint8_t *)reply, resultLen,
                                    Type_Data_With_Length, verify);
      memcpy((unsigned char *)&reply[resultLen + 2], verify, 16);

      write(socketNumber, reply, resultLen + 18);

      delete[] reply;

      pthread_mutex_unlock(&connection[i].mutex);
    }
  }
}
void *connectionLoop(void *threadInfo) {
  connectionInfo *info = (connectionInfo *)threadInfo;
  int subSocket = info->subSocket;
  ssize_t len;
  if (subSocket >= 0) {
    supla_log(LOG_DEBUG, "start connect: %d", subSocket);

    do {
      len = read(subSocket, info->buffer, 4096);

      if (len > 0) {
        supla_log(LOG_DEBUG, "return len %d for socket %d\n", len, subSocket);
        supla_log(LOG_DEBUG, "message: %s\n", info->buffer);
      }

      PHKNetworkMessage msg(info->buffer);
      if (len > 0) {
        if (!strcmp(msg.directory, "pair-setup")) {
          info->handlePairSeup();
          updateConfiguration();
        } else if (!strcmp(msg.directory, "pair-verify")) {
          info->handlePairVerify();
          info->handleAccessoryRequest();
        } else if (!strcmp(msg.directory, "identify")) {
          close(subSocket);
        }
      }

    } while (len > 0);

    close(subSocket);
    supla_log(LOG_DEBUG, "stop connect %d", subSocket);
    info->subSocket = -1;
  }
  return NULL;
}

void PHKNetworkIP::handleConnection() const {
  struct sockaddr_in client_addr;
  socklen_t clen;

  clen = sizeof(client_addr);

  int subSocket = accept(_socket_v4, (struct sockaddr *)&client_addr, &clen);

  if (subSocket == -1) {
    supla_log(LOG_ERR, "Connection accept error %i", errno);
    return;
  };

  // Before anything start, get sniff the host name of the client
  string socketName = "";
  if (clen == sizeof(struct sockaddr_in)) {
    char buffer[1024];
    getnameinfo((struct sockaddr *)&client_addr, clen, buffer, 1024, NULL, 0,
                NI_NOFQDN);
    socketName = string(buffer);
  }

  int index = -1;
  for (int i = 0; i < numberOfClient; i++) {
    if (connection[i].subSocket == -1) {
      index = i;
      connection[index].subSocket = subSocket;
      connection[index].hostname = socketName;

      pthread_create(&connection[index].thread, NULL, connectionLoop,
                     &connection[index]);
      pthread_detach(connection[index].thread);
      break;
    }
  }

  // Too much connection?
  if (index < 0) close(subSocket);
}

// 2ByteS(datalen) + data_buf + 16Bytes(Verfied Key)
// Type_Data_Without_Length
// Passed-in buf is OMIT len and 16Bytes verfied key
// Type_Data_With_Length
// Passed-in buf is len and data_buf
void connectionInfo::Poly1305_GenKey(const unsigned char *key, uint8_t *buf,
                                     uint16_t len, Poly1305Type_t type,
                                     char *verify) {
  if (key == NULL || buf == NULL || len < 2 || verify == NULL) return;

  poly1305_context verifyContext;
  memset(&verifyContext, 0, sizeof(verifyContext));
  poly1305_init(&verifyContext, key);

  char waste[16] = {0};

  if (type == Type_Data_With_Length) {
    poly1305_update(&verifyContext, (const unsigned char *)&buf[0], 1);
    poly1305_update(&verifyContext, (const unsigned char *)&buf[1], 1);
    poly1305_update(&verifyContext, (const unsigned char *)waste, 14);

    poly1305_update(&verifyContext, (const unsigned char *)&buf[2], len);
  } else {
    poly1305_update(&verifyContext, (const unsigned char *)buf, len);
  }

  if (len % 16 > 0)
    poly1305_update(&verifyContext, (const unsigned char *)waste,
                    16 - (len % 16));
  unsigned char _len;
  if (type == Type_Data_With_Length) {
    _len = 2;
  } else {
    _len = 0;
  }

  poly1305_update(&verifyContext, (const unsigned char *)&_len, 1);
  poly1305_update(&verifyContext, (const unsigned char *)&waste, 7);
  _len = len;

  poly1305_update(&verifyContext, (const unsigned char *)&_len, 1);
  _len = len / 256;
  poly1305_update(&verifyContext, (const unsigned char *)&_len, 1);

  poly1305_update(&verifyContext, (const unsigned char *)&waste, 6);

  poly1305_finish(&verifyContext, (unsigned char *)verify);
}

void connectionInfo::handlePairSeup() {
  identity[36] = 0;
  PHKNetworkMessageDataRecord stateRecord;
  stateRecord.activate = true;
  stateRecord.data = new char[1];
  stateRecord.length = 1;
  stateRecord.index = 6;
  PairSetupState_t state = State_M1_SRPStartRequest;
  SRP *srp = SRP_new(SRP6a_server_method());

  cstr *secretKey = NULL, *publicKey = NULL, *response = NULL;
  char sessionKey[64];
  char *responseBuffer = 0;
  int responseLen = 0;

  std::string deviceIdentity = Configuration::Instance().getDeviceIdentity();

  do {
    PHKNetworkMessage msg = PHKNetworkMessage(buffer);
    PHKNetworkResponse mResponse(200);

    state = (PairSetupState_t)(*msg.data.dataPtrForIndex(6));

    *stateRecord.data = (char)state + 1;
    switch (state) {
      case State_M4_SRPVerifyRespond:
      case State_M6_ExchangeRespond:
      case State_M2_SRPStartRespond:
        break;
      case State_M1_SRPStartRequest: {
        supla_log(LOG_DEBUG, "pair-setup State_M1_SRPStartRequest");
    	PHKNetworkMessageDataRecord saltRec;
        PHKNetworkMessageDataRecord publicKeyRec;
        unsigned char saltChar[16];

        for (int i = 0; i < 16; i++) {
          saltChar[i] = rand();
        }

        SRP_set_username(srp, "Pair-Setup");
        int modulusSize = sizeof(modulusStr) / sizeof(modulusStr[0]);
        SRP_set_params(srp, (const unsigned char *)modulusStr, modulusSize,
                       (const unsigned char *)generator, 1, saltChar, 16);
        SRP_set_auth_password(
            srp, Configuration::Instance().getDevicePassword().c_str());
        SRP_gen_pub(srp, &publicKey);

        saltRec.index = 2;
        saltRec.activate = true;
        saltRec.length = 16;
        saltRec.data = new char[saltRec.length];
        bcopy(saltChar, saltRec.data, saltRec.length);
        publicKeyRec.index = 3;
        publicKeyRec.activate = true;
        publicKeyRec.length = publicKey->length;
        publicKeyRec.data = new char[publicKeyRec.length];
        bcopy(publicKey->data, publicKeyRec.data, publicKeyRec.length);

        mResponse.data.addRecord(stateRecord);
        mResponse.data.addRecord(publicKeyRec);
        mResponse.data.addRecord(saltRec);
      } break;
      case State_M3_SRPVerifyRequest: {
    	supla_log(LOG_DEBUG, "pair-setup State_M3_SRPVerifyRequest");
        const char *keyStr = 0;
        int keyLen = 0;
        const char *proofStr;
        int proofLen;
        keyStr = msg.data.dataPtrForIndex(3);
        keyLen = msg.data.lengthForIndex(3);
        char *temp = msg.data.dataPtrForIndex(4);
        if (temp != NULL) {
          proofStr = temp;
          proofLen = msg.data.lengthForIndex(4);
        }

        SRP_RESULT result = SRP_compute_key(
            srp, &secretKey, (const unsigned char *)keyStr, keyLen);
        result = SRP_verify(srp, (const unsigned char *)proofStr, proofLen);

        if (!SRP_OK(result)) {
          PHKNetworkMessageDataRecord responseRecord;
          responseRecord.activate = true;
          responseRecord.data = new char[1];
          responseRecord.data[0] = 2;
          responseRecord.index = 7;
          responseRecord.length = 1;
          mResponse.data.addRecord(responseRecord);
        } else {
          SRP_respond(srp, &response);
          PHKNetworkMessageDataRecord responseRecord;
          responseRecord.activate = true;
          responseRecord.index = 4;
          responseRecord.length = response->length;
          responseRecord.data = new char[responseRecord.length];
          bcopy(response->data, responseRecord.data, responseRecord.length);
          mResponse.data.addRecord(responseRecord);
        }

        const char salt[] = "Pair-Setup-Encrypt-Salt";
        const char info[] = "Pair-Setup-Encrypt-Info";
        int i = hkdf((const unsigned char *)salt, strlen(salt),
                     (const unsigned char *)secretKey->data, secretKey->length,
                     (const unsigned char *)info, strlen(info),
                     (uint8_t *)sessionKey, 32);
        if (i != 0) return;
      } break;
      case State_M5_ExchangeRequest: {
    	supla_log(LOG_DEBUG, "pair-setup State_M5_ExchangeRequest");
        const char *encryptedPackage = NULL;
        int packageLen = 0;
        encryptedPackage = msg.data.dataPtrForIndex(5);
        packageLen = msg.data.lengthForIndex(5);
        char *encryptedData = new char[packageLen];
        bcopy(encryptedPackage, encryptedData, packageLen - 16);
        char mac[16];
        bcopy(&encryptedPackage[packageLen - 16], mac, 16);

        chacha20_ctx chacha20;
        memset(&chacha20, 0, sizeof(chacha20));
        chacha20_setup(&chacha20, (const uint8_t *)sessionKey, 32,
                       (uint8_t *)"PS-Msg05");

        // Ploy1305 key
        char temp[64] = {0};
        char temp2[64] = {0};
        chacha20_encrypt(&chacha20, (const uint8_t *)temp, (uint8_t *)temp2,
                         64);

        char verify[16] = {0};
        Poly1305_GenKey((const unsigned char *)temp2,
                        (unsigned char *)encryptedData, packageLen - 16,
                        Type_Data_Without_Length, verify);

        char *decryptedData = new char[packageLen - 16];

        memset(decryptedData, 0, packageLen - 16);

        chacha20_decrypt(&chacha20, (const uint8_t *)encryptedData,
                         (uint8_t *)decryptedData, packageLen - 16);

        if (bcmp(verify, mac, 16)) {
          PHKNetworkMessageDataRecord responseRecord;
          responseRecord.activate = true;
          responseRecord.data = new char[1];
          responseRecord.data[0] = 1;
          responseRecord.index = 7;
          responseRecord.length = 1;
          mResponse.data.addRecord(responseRecord);
        } else {
          /*
           * HAK Pair Setup M6
           */
          PHKNetworkMessageData *subTLV8 =
              new PHKNetworkMessageData(decryptedData, packageLen - 16);
          char *controllerIdentifier = subTLV8->dataPtrForIndex(1);
          char *controllerPublicKey = subTLV8->dataPtrForIndex(3);
          char *controllerSignature = subTLV8->dataPtrForIndex(10);
          char controllerHash[100];

          PHKKeyRecord newRecord;
          bcopy(controllerIdentifier, newRecord.controllerID, 36);
          bcopy(controllerPublicKey, newRecord.publicKey, 32);
          addControllerKey(newRecord);

          const char salt[] = "Pair-Setup-Controller-Sign-Salt";
          const char info[] = "Pair-Setup-Controller-Sign-Info";
          int i = hkdf((const unsigned char *)salt, strlen(salt),
                       (const unsigned char *)secretKey->data,
                       secretKey->length, (const unsigned char *)info,
                       strlen(info), (uint8_t *)controllerHash, 32);
          if (i != 0) return;

          bcopy(controllerIdentifier, &controllerHash[32], 36);
          bcopy(controllerPublicKey, &controllerHash[68], 32);

          int ed25519_err =
              ed25519_sign_open((const unsigned char *)controllerHash, 100,
                                (const unsigned char *)controllerPublicKey,
                                (const unsigned char *)controllerSignature);
          delete subTLV8;

          if (ed25519_err) {
            if (srp) SRP_free(srp);
            delete publicKey;
            delete response;
            delete secretKey;
            return;
          } else {
            PHKNetworkMessageData *returnTLV8 = new PHKNetworkMessageData();
            {
              PHKNetworkMessageDataRecord usernameRecord;
              usernameRecord.activate = true;
              usernameRecord.index = 1;
              usernameRecord.length = deviceIdentity.length();
              usernameRecord.data = new char[usernameRecord.length];
              bcopy(deviceIdentity.c_str(), usernameRecord.data,
                    usernameRecord.length);
              returnTLV8->addRecord(usernameRecord);
            }

            {
              /*
               * Generate Signature
               */
              const char salt[] = "Pair-Setup-Accessory-Sign-Salt";
              const char info[] = "Pair-Setup-Accessory-Sign-Info";
              uint8_t output[150];
              hkdf((const unsigned char *)salt, strlen(salt),
                   (const unsigned char *)secretKey->data, secretKey->length,
                   (const unsigned char *)info, strlen(info), output, 32);

              bcopy(deviceIdentity.c_str(), &output[32],
                    deviceIdentity.length());

              char *signature = new char[64];
              ed25519_secret_key edSecret;
              bcopy(accessorySecretKey, edSecret, sizeof(edSecret));
              ed25519_public_key edPubKey;
              ed25519_publickey(edSecret, edPubKey);

              bcopy(edPubKey, &output[32 + deviceIdentity.length()], 32);
              ed25519_sign(output, 64 + deviceIdentity.length(),
                           (const unsigned char *)edSecret,
                           (const unsigned char *)edPubKey,
                           (unsigned char *)signature);
              PHKNetworkMessageDataRecord signatureRecord;
              signatureRecord.activate = true;
              signatureRecord.data = signature;
              signatureRecord.index = 10;
              signatureRecord.length = 64;
              returnTLV8->addRecord(signatureRecord);

              PHKNetworkMessageDataRecord publicKeyRecord;
              publicKeyRecord.activate = true;
              publicKeyRecord.index = 3;
              publicKeyRecord.length = 32;
              publicKeyRecord.data = new char[publicKeyRecord.length];
              bcopy(edPubKey, publicKeyRecord.data, publicKeyRecord.length);
              returnTLV8->addRecord(publicKeyRecord);
            }

            const char *tlv8Data;
            unsigned short tlv8Len;
            returnTLV8->rawData(&tlv8Data, &tlv8Len);

            PHKNetworkMessageDataRecord tlv8Record;
            tlv8Record.data = new char[tlv8Len + 16];
            tlv8Record.length = tlv8Len + 16;
            memset(tlv8Record.data, 0, tlv8Record.length);
            {
              chacha20_ctx ctx;
              memset(&ctx, 0, sizeof(ctx));

              chacha20_setup(&ctx, (const uint8_t *)sessionKey, 32,
                             (uint8_t *)"PS-Msg06");
              char buffer[64] = {0};
              char key[64] = {0};
              chacha20_encrypt(&ctx, (const uint8_t *)buffer, (uint8_t *)key,
                               64);
              chacha20_encrypt(&ctx, (const uint8_t *)tlv8Data,
                               (uint8_t *)tlv8Record.data, tlv8Len);

              char verify[16];
              memset(verify, 0, 16);
              Poly1305_GenKey((const unsigned char *)key,
                              (unsigned char *)tlv8Record.data, tlv8Len,
                              Type_Data_Without_Length, verify);
              memcpy((unsigned char *)&tlv8Record.data[tlv8Len], verify, 16);
            }

            tlv8Record.activate = true;
            tlv8Record.index = 5;
            mResponse.data.addRecord(tlv8Record);

            delete returnTLV8;
          }
        }

        mResponse.data.addRecord(stateRecord);
        mResponse.getBinaryPtr(&responseBuffer, &responseLen);
        if (responseBuffer) {
          write(subSocket, (const void *)responseBuffer, (size_t)responseLen);
          delete[] responseBuffer;
        }

        delete[] encryptedData;

        if (srp) SRP_free(srp);
        delete publicKey;
        delete response;
        delete secretKey;
        return;
      } break;
    }
    mResponse.data.addRecord(stateRecord);
    mResponse.getBinaryPtr(&responseBuffer, &responseLen);

    if (responseBuffer) {
          write(subSocket, (const void *)responseBuffer, (size_t)responseLen);
      delete[] responseBuffer;
    };

  } while (read(subSocket, (void *)buffer, 4096) > 0);

  delete publicKey;
  delete response;
  delete secretKey;
  SRP_free(srp);
}

void connectionInfo::handlePairVerify() {
  bool end = false;
  char state = State_Pair_Verify_M1;

  curved25519_key secretKey;
  curved25519_key publicKey;
  curved25519_key controllerPublicKey;
  curved25519_key sharedKey;

  uint8_t enKey[32];

  supla_log(LOG_DEBUG, "Pair-Verify Start");

  do {
    PHKNetworkMessage msg(buffer);
    PHKNetworkResponse response = PHKNetworkResponse(200);
    memcpy(&state, msg.data.dataPtrForIndex(6), 1);

    switch (state) {
      case State_Pair_Verify_M1: {
        supla_log(LOG_DEBUG, "Pair-Verify M1");

        memcpy(controllerPublicKey, msg.data.dataPtrForIndex(3), 32);

        for (unsigned short i = 0; i < sizeof(secretKey); i++) {
          secretKey[i] = rand();
        }
        curve25519_donna((u8 *)publicKey, (const u8 *)secretKey,
                         (const u8 *)curveBasePoint);
        curve25519_donna(sharedKey, secretKey, controllerPublicKey);

        char *temp = new char[100];
        memcpy(temp, publicKey, 32);

        std::string deviceIdentity =
            Configuration::Instance().getDeviceIdentity();

        memcpy(&temp[32], deviceIdentity.c_str(), deviceIdentity.length());
        memcpy(&temp[32 + deviceIdentity.length()], controllerPublicKey, 32);

        PHKNetworkMessageDataRecord signRecord;
        signRecord.activate = true;
        signRecord.data = new char[64];
        signRecord.index = 10;
        signRecord.length = 64;

        ed25519_secret_key edSecret;
        memcpy(edSecret, accessorySecretKey, sizeof(edSecret));

        ed25519_public_key edPubKey;
        ed25519_publickey(edSecret, edPubKey);

        ed25519_sign((const unsigned char *)temp, 64 + deviceIdentity.length(),
                     edSecret, edPubKey, (unsigned char *)signRecord.data);
        delete[] temp;

        PHKNetworkMessageDataRecord idRecord;
        idRecord.activate = true;
        idRecord.data = new char[17];

        memcpy(idRecord.data, deviceIdentity.c_str(), 17);

        idRecord.index = 1;
        idRecord.length = (unsigned int)17;

        PHKNetworkMessageDataRecord pubKeyRecord;
        pubKeyRecord.activate = true;
        pubKeyRecord.data = new char[32];

        memcpy(pubKeyRecord.data, publicKey, 32);

        pubKeyRecord.index = 3;
        pubKeyRecord.length = 32;

        PHKNetworkMessageData data;
        response.data.addRecord(pubKeyRecord);
        data.addRecord(signRecord);
        data.addRecord(idRecord);

        unsigned char salt[] = "Pair-Verify-Encrypt-Salt";
        unsigned char info[] = "Pair-Verify-Encrypt-Info";

        hkdf(salt, 24, sharedKey, 32, info, 24, enKey, 32);
        const char *plainMsg = 0;
        unsigned short msgLen = 0;
        data.rawData(&plainMsg, &msgLen);

        char *encryptMsg = new char[msgLen + 16];
        char *polyKey = new char[64];
        memset(polyKey, 0, 64);

        char zero[64];

        chacha20_ctx chacha;
        chacha20_setup(&chacha, enKey, 32, (uint8_t *)"PV-Msg02");
        chacha20_encrypt(&chacha, (uint8_t *)zero, (uint8_t *)polyKey, 64);
        chacha20_encrypt(&chacha, (uint8_t *)plainMsg, (uint8_t *)encryptMsg,
                         msgLen);

        delete[] plainMsg;

        char verify[16];
        memset(verify, 0, 16);
        Poly1305_GenKey((const unsigned char *)polyKey, (uint8_t *)encryptMsg,
                        msgLen, Type_Data_Without_Length, verify);
        memcpy((unsigned char *)&encryptMsg[msgLen], verify, 16);

        PHKNetworkMessageDataRecord encryptRecord;
        encryptRecord.activate = true;
        encryptRecord.index = 5;
        encryptRecord.length = msgLen + 16;
        encryptRecord.data = new char[encryptRecord.length];

        memcpy(encryptRecord.data, encryptMsg, encryptRecord.length);

        response.data.addRecord(encryptRecord);

        delete[] encryptMsg;
        delete[] polyKey;
      } break;
      case State_Pair_Verify_M3: {
        supla_log(LOG_DEBUG, "Pair-Verify M3");

        char *encryptedData = msg.data.dataPtrForIndex(5);
        short packageLen = msg.data.lengthForIndex(5);

        chacha20_ctx chacha20;
        memset(&chacha20, 0, sizeof(chacha20));

        chacha20_setup(&chacha20, (const uint8_t *)enKey, 32,
                       (uint8_t *)"PV-Msg03");

        // Ploy1305 key
        char temp[64] = {0};
        char temp2[64] = {0};

        chacha20_encrypt(&chacha20, (const uint8_t *)temp, (uint8_t *)temp2,
                         64);

        char verify[16] = {0};

        Poly1305_GenKey((const unsigned char *)temp2, (uint8_t *)encryptedData,
                        packageLen - 16, Type_Data_Without_Length, verify);

        if (!bcmp(verify, &encryptedData[packageLen - 16], 16)) {
          char *decryptData = new char[packageLen - 16];
          chacha20_decrypt(&chacha20, (const uint8_t *)encryptedData,
                           (uint8_t *)decryptData, packageLen - 16);
          PHKNetworkMessageData data =
              PHKNetworkMessageData(decryptData, packageLen - 16);
          PHKKeyRecord rec = getControllerKey(data.dataPtrForIndex(1));

          char tempMsg[100] = {0};
          memcpy(tempMsg, controllerPublicKey, 32);
          memcpy(&tempMsg[32], data.dataPtrForIndex(1), 36);
          memcpy(identity, data.dataPtrForIndex(1), 36);
          memcpy(&tempMsg[68], publicKey, 32);

          int err = ed25519_sign_open(
              (const unsigned char *)tempMsg, 100,
              (const unsigned char *)rec.publicKey,
              (const unsigned char *)data.dataPtrForIndex(10));

          if (err == 0) {
            end = true;

            hkdf((uint8_t *)"Control-Salt", 12, sharedKey, 32,
                 (uint8_t *)"Control-Read-Encryption-Key", 27,
                 accessoryToControllerKey, 32);
            hkdf((uint8_t *)"Control-Salt", 12, sharedKey, 32,
                 (uint8_t *)"Control-Write-Encryption-Key", 28,
                 controllerToAccessoryKey, 32);

            supla_log(LOG_DEBUG, "Pair-Verify Success!");

          } else {
            PHKNetworkMessageDataRecord error;
            error.activate = true;
            error.data = new char[1];
            error.data[0] = 2;
            error.index = 7;
            error.length = 1;
            response.data.addRecord(error);
            supla_log(LOG_DEBUG, "Pair-Verify Failure");
          }
          delete[] decryptData;
        }
      }
    }

    PHKNetworkMessageDataRecord stage;
    stage.activate = true;
    stage.data = new char;
    stage.data[0] = (char)state + 1;
    stage.index = 6;
    stage.length = 1;
    response.data.addRecord(stage);

    char *repBuffer = 0;
    int repLen = 0;
    response.getBinaryPtr(&repBuffer, &repLen);
    if (repBuffer) {
      write(subSocket, repBuffer, repLen);
      delete[] repBuffer;
    }
  } while (!end && read(subSocket, buffer, 4096) > 0);

  supla_log(LOG_DEBUG, "Pair-Verify End");
}

void connectionInfo::handleAccessoryRequest() {
  char *decryptData = new char[2048];

  int len;
  numberOfMsgRec = 0;
  numberOfMsgSend = 0;

  clearNotify();

  pthread_mutex_init(&mutex, NULL);
  connected = true;

  do {
    memset(buffer, 0, 4096);
    len = read(subSocket, buffer, 4096);

    pthread_mutex_lock(&mutex);

    if (len > 0) {
      uint16_t msgLen = (uint8_t)buffer[1] * 256 + (uint8_t)*buffer;

      chacha20_ctx chacha20;
      memset(&chacha20, 0, sizeof(chacha20));

      if (!is_big_endian()) numberOfMsgRec = bswap_64(numberOfMsgRec);
      chacha20_setup(&chacha20, (const uint8_t *)controllerToAccessoryKey, 32,
                     (uint8_t *)&numberOfMsgRec);
      if (!is_big_endian()) numberOfMsgRec = bswap_64(numberOfMsgRec);
      numberOfMsgRec++;

      char temp[64] = {0};
      char temp2[64] = {0};
      chacha20_encrypt(&chacha20, (const uint8_t *)temp, (uint8_t *)temp2, 64);

      // Ploy1305 key
      char verify[16] = {0};

      Poly1305_GenKey((const unsigned char *)temp2, (uint8_t *)buffer, msgLen,
                      Type_Data_With_Length, verify);

      memset(decryptData, 0, 2048);

      chacha20_encrypt(&chacha20, (const uint8_t *)&buffer[2],
                       (uint8_t *)decryptData, msgLen);

      if (len >= (2 + msgLen + 16) &&
          memcmp((void *)verify, (void *)&buffer[2 + msgLen], 16) == 0) {
      } else {
        continue;
      }

      // Output return
      char *resultData = 0;
      unsigned int resultLen = 0;
      handleAccessory(decryptData, msgLen, &resultData, &resultLen, this);

      // 18 = 2(resultLen) + 16(poly1305 verify key)
      char *reply = new char[resultLen + 18];
      reply[0] = resultLen % 256;
      reply[1] = (resultLen - (uint8_t)reply[0]) / 256;

      if (!is_big_endian()) numberOfMsgSend = bswap_64(numberOfMsgSend);
      chacha20_setup(&chacha20, (const uint8_t *)accessoryToControllerKey, 32,
                     (uint8_t *)&numberOfMsgSend);
      if (!is_big_endian()) numberOfMsgSend = bswap_64(numberOfMsgSend);
      numberOfMsgSend++;

      chacha20_encrypt(&chacha20, (const uint8_t *)temp, (uint8_t *)temp2, 64);
      chacha20_encrypt(&chacha20, (const uint8_t *)resultData,
                       (uint8_t *)&reply[2], resultLen);

      Poly1305_GenKey((const unsigned char *)temp2, (uint8_t *)reply, resultLen,
                      Type_Data_With_Length, verify);
      memcpy((unsigned char *)&reply[resultLen + 2], verify, 16);

      write(subSocket, reply, resultLen + 18);

      pthread_mutex_unlock(&mutex);

      delete[] reply;
      delete[] resultData;
    }
  } while (len > 0);

  if (deadConnection) deadConnection(this);

  pthread_mutex_destroy(&mutex);

  delete[] decryptData;
  connected = false;
}

// Object Logic
PHKNetworkIP::~PHKNetworkIP() {
  supla_log(LOG_DEBUG, "Freeing PHKNetworkIP...");
  if (netServiceV4) {
    supla_log(LOG_DEBUG, "Deallocating DNS Service...");
    DNSServiceRefDeallocate(netServiceV4);
    netServiceV4 = 0;
  }
}

const char *copyLine(const char *rawData, char *destination) {
  int i;
  for (i = 0; rawData[i] != '\r' && rawData[i] != 0; i++) {
    destination[i] = rawData[i];
  }
  i++;
  if (rawData[i] == '\n')
    return &rawData[i + 1];
  else
    return &rawData[i];
}

const char *skipTillChar(const char *ptr, const char target) {
  for (; (*ptr) != 0 && (*ptr) != target; ptr++)
    ;

  ptr++;
  return ptr;
}

PHKNetworkMessage::PHKNetworkMessage(const char *rawData) {
  strcpy(method, "POST");

  const char *ptr = rawData;
  for (int i = 0; (*ptr) != 0 && (*ptr) != ' '; ptr++, i++) {
    method[i] = (*ptr);
    method[i + 1] = 0;
  }
  ptr += 2;

  // Copy message directory
  for (int i = 0; (*ptr) != 0 && (*ptr) != ' '; ptr++, i++) {
    directory[i] = (*ptr);
    directory[i + 1] = 0;
  }

  ptr = skipTillChar(ptr, '\n');

  char buffer[1024];
  const char *dptr = ptr;
  for (int i = 0; i < 19; i++) {
    memset(buffer, 0, 1024);
    dptr = copyLine(dptr, buffer);
  }

  // Reject host
  if (strncmp(ptr, "Host", 4) == 0) {
    ptr = skipTillChar(ptr, '\n');
  }

  // Get the length of content
  // Skip to the content-type
  ptr = skipTillChar(ptr, ':');
  ptr++;
  unsigned int dataSize = (unsigned int)strtol(ptr, (char **)&ptr, 10);

  // Get the type of content
  // Skip to the content-length
  ptr = skipTillChar(ptr, ':');
  ptr += 2;
  for (int i = 0; (*ptr) != 0 && (*ptr) != '\r'; ptr++, i++) {
    type[i] = (*ptr);
    type[i + 1] = 0;
  }
  ptr += 4;

  // Data
  data = PHKNetworkMessageData(ptr, dataSize);
}

void PHKNetworkMessage::getBinaryPtr(char **buffer, int *contentLength) {
  const char *_data;
  unsigned short dataSize;
  data.rawData(&_data, &dataSize);
  (*buffer) = new char[1024];
  (*contentLength) = snprintf(
      (*buffer), 1024,
      "%s /%s HTTP/1.1\r\nContent-Length: %hu\r\nContent-Type: %s\r\n\r\n",
      method, directory, dataSize, type);
  for (int i = 0; i < dataSize; i++) {
    (*buffer)[*contentLength + i] = _data[i];
  }
  (*contentLength) += dataSize;
  (*buffer)[*contentLength] = 0;
}

PHKNetworkMessageData &PHKNetworkMessageData::operator=(
    const PHKNetworkMessageData &data) {
  count = data.count;
  for (int i = 0; i < 10; i++) {
    if (data.records[i].length) {
      records[i] = data.records[i];
      records[i].data =
          records[i].length > 0 ? new char[records[i].length] : NULL;
      if (data.records[i].length > 0)
        memcpy(records[i].data, data.records[i].data, data.records[i].length);
    }
  }
  return *this;
}

PHKNetworkMessageData::PHKNetworkMessageData(const char *rawData,
                                             unsigned short len) {
  unsigned short delta = 0;
  while (delta < len) {
    int index = recordIndex(rawData[delta + 0]);
    if (index < 0) {
      records[count].index = (rawData)[delta + 0];
      records[count].length = (unsigned char)(rawData)[delta + 1];
      records[count].data = new char[records[count].length];
      records[count].activate = true;
      memcpy(records[count].data, &rawData[delta + 2], records[count].length);
      delta += (records[count].length + 2);
      count++;
    } else {
      int newLen = ((unsigned char *)(rawData))[delta + 1];
      newLen += records[index].length;
      char *ptr = new char[newLen];
      memcpy(ptr, records[index].data, records[index].length);
      memcpy(&ptr[records[index].length], &rawData[delta + 2],
             newLen - records[index].length);
      delete[] records[index].data;
      records[index].data = ptr;

      delta += (newLen - records[index].length + 2);
      records[index].length = newLen;
    }
  }
}

void PHKNetworkMessageData::rawData(const char **dataPtr, unsigned short *len) {
  string buffer = "";
  for (int i = 0; i < 10; i++) {
    if (records[i].activate) {
      for (unsigned int j = 0; j != records[i].length;) {
        unsigned char len =
            records[i].length - j > 255 ? 255 : records[i].length - j;
        string temp(&records[i].data[j], len);
        temp = (char)records[i].index + ((char)len + temp);
        buffer += temp;
        j += (unsigned int)len;
      }
    }
  }
  *len = buffer.length();
  *dataPtr = new char[*len];
  memcpy((void *)*dataPtr, buffer.c_str(), *len);
}

int PHKNetworkMessageData::recordIndex(unsigned char index) {
  for (int i = 0; i < count; i++) {
    if (records[i].activate && records[i].index == index) return i;
  }
  return -1;
}

void PHKNetworkMessageData::addRecord(PHKNetworkMessageDataRecord &record) {
  records[count] = record;
  count++;
}

PHKNetworkResponse::PHKNetworkResponse(unsigned short _responseCode) {
  responseCode = _responseCode;
}

string PHKNetworkResponse::responseType() {
  switch (responseCode) {
    case 200:
      return "OK";
    default:
      return "";
  }
}

inline void int2str(int i, char *s) { sprintf(s, "%d", i); }

void PHKNetworkResponse::getBinaryPtr(char **buffer, int *contentLength) {
  char tempCstr[5];
  int2str(responseCode, tempCstr);
  string temp = "HTTP/1.1 ";
  temp += tempCstr + (" " + responseType());
  temp += "\r\nContent-Type: application/pairing+tlv8\r\nContent-Length: ";
  const char *dataPtr;
  unsigned short dataLen;
  data.rawData(&dataPtr, &dataLen);
  int2str(dataLen, tempCstr);
  temp += tempCstr;
  temp += "\r\n\r\n";
  *buffer = new char[temp.length() + dataLen];
  memcpy(*buffer, temp.c_str(), temp.length());

  memcpy(&((*buffer)[temp.length()]), dataPtr, dataLen);

  *contentLength = temp.length() + dataLen;
  delete[] dataPtr;
}

char *PHKNetworkMessageData::dataPtrForIndex(unsigned char index) {
  int _index = recordIndex(index);
  if (_index >= 0) return records[_index].data;
  return 0;
}

unsigned int PHKNetworkMessageData::lengthForIndex(unsigned char index) {
  int _index = recordIndex(index);
  if (_index >= 0) return records[_index].length;
  return 0;
}

PHKNetworkMessageDataRecord &PHKNetworkMessageDataRecord::operator=(
    const PHKNetworkMessageDataRecord &r) {
  index = r.index;
  activate = r.activate;
  length = r.length;

  if (data) delete[] data;

  data = new char[length];
  memcpy(data, r.data, length);
  return *this;
}

PHKNetworkMessageDataRecord::~PHKNetworkMessageDataRecord() {
  if (length) delete[] data;
}