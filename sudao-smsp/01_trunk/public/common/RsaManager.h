#ifndef _RsaManager_h_
#define _RsaManager_h_

#include "platform.h"
#include "ssl/rsa.h"
#include "ssl/rand.h"
#include "ssl/pem.h"
#include "ssl/err.h"

#include <string>



std::string rsaPubEncrypt(cs_t srtPublicKey, cs_t srtData);

class RsaManager
{
public:
    RsaManager();
    ~RsaManager();

    bool generateRSAKey();
    void printRSAKey();

    bool getRSAPublicKey(std::string& strPubKey);

    bool initByRSAPublicKey(cs_t strPublicKey);
    bool initByRSAPrivateKey(cs_t strPrivateKey);

    bool encrypt(cs_t strData, std::string& strAfterEncrypt);
    bool decrypt(cs_t strData, std::string& strAfterDecrypt);

private:
    bool encryptEx(cs_t strData, std::string& strAfterEncrypt);
    bool decryptEx(cs_t strData, std::string& strAfterDecrypt);

public:
    RSA* rsa_;
    BIGNUM* bn_;
    BIO* bp_public_;
    BIO* bp_private_;

    int rsa_bits_;
    int rsa_len_;
};

#endif
