#include "RsaManager.h"
#include "LogMacro.h"

using namespace std;


static const char rnd_seed[] = "string to make the random number generator think it has entropy";


string rsaPubEncrypt(cs_t srtPublicKey, cs_t srtData)
{
    RsaManager rsaManager;

    if (!rsaManager.initByRSAPublicKey(srtPublicKey))
    {
        LogError("Call initByRSAPublicKey failed.");
        return INVALID_STR;
    }

    string strAfterEncrypt;
    if (!rsaManager.encrypt(srtData, strAfterEncrypt))
    {
        LogError("Call encrypt failed.");
        return INVALID_STR;
    }

    return strAfterEncrypt;
}

RsaManager::RsaManager()
{
    rsa_ = NULL;
    bn_ = NULL;
    bp_public_ = NULL;
    bp_private_ = NULL;

    rsa_bits_ = 1024;
    rsa_len_ = 0;
}

RsaManager::~RsaManager()
{
    if (rsa_)
        RSA_free(rsa_);

    if (bn_)
        BN_free(bn_);

    if (bp_public_)
        BIO_free_all(bp_public_);

    if (bp_private_)
        BIO_free_all(bp_private_);
}

bool RsaManager::generateRSAKey()
{
    rsa_ = RSA_new();
    bn_ = BN_new();
    bp_public_ = BIO_new(BIO_s_mem());
    bp_private_ = BIO_new(BIO_s_mem());

    CHK_NULL_RETURN_FALSE(rsa_);
    CHK_NULL_RETURN_FALSE(bn_);
    CHK_NULL_RETURN_FALSE(bp_public_);
    CHK_NULL_RETURN_FALSE(bp_private_);

    int ret = 0;

    ret = BN_set_word(bn_, RSA_F4);
    if (ret != 1)
    {
        LogError("Call BN_set_word failed.");
        return false;
    }

    ret = RSA_generate_key_ex(rsa_, rsa_bits_, bn_, NULL);
    if (ret != 1)
    {
        LogError("Call RSA_generate_key_ex failed.");
        return false;
    }

    ret = PEM_write_bio_RSAPublicKey(bp_public_, rsa_);
    if (ret != 1)
    {
        LogError("Call PEM_write_bio_RSAPublicKey failed.");
        return false;
    }

    ret = PEM_write_bio_RSAPrivateKey(bp_private_, rsa_, NULL, NULL, 0, NULL, NULL);
    if (ret != 1)
    {
        LogError("Call PEM_write_bio_RSAPrivateKey failed.");
        return false;
    }

    // 对于1024bit的密钥,block length = 1024/8 = 128 字节
    rsa_len_ = RSA_size(rsa_);

    return true;
}

void RsaManager::printRSAKey()
{
    {
        char buf[2048] = {0};
        int ret = BIO_read(bp_public_, buf, sizeof(buf));
        if (ret < 0)
        {
            LogError("Call BIO_read failed. code[%d].", ret);
            return;
        }

        LogNotice("\n%s", buf);
    }

    {
        char buf[2048] = {0};
        int ret = BIO_read(bp_private_, buf, sizeof(buf));
        if (ret < 0)
        {
            LogError("Call BIO_read failed. code[%d].", ret);
            return;
        }

        LogNotice("\n%s", buf);
    }
}

bool RsaManager::getRSAPublicKey(string& strPubKey)
{
    char buf[2048] = {0};
    int ret = BIO_read(bp_public_, buf, sizeof(buf));
    if (ret < 0)
    {
        LogError("Call BIO_gets failed. code[%d].", ret);
        return false;
    }

    strPubKey.assign(buf);
    return true;
}

bool RsaManager::initByRSAPublicKey(cs_t strPublicKey)
{
    RAND_seed(rnd_seed, sizeof(rnd_seed));

    bp_public_ = BIO_new_mem_buf((void*)strPublicKey.data(), -1);
    if (NULL == bp_public_)
    {
        LogError("Call BIO_new_mem_buf failed.");
        return false;
    }

    rsa_ = PEM_read_bio_RSAPublicKey(bp_public_, NULL, NULL, NULL);
    if (NULL == rsa_)
    {
        ERR_load_crypto_strings();

        char err[512];
        ERR_error_string_n(ERR_get_error(), err, sizeof(err));

        LogError("Call PEM_read_bio_RSAPublicKey failed. reason[%s].", err);
        return false;
    }

    // 对于1024bit的密钥,block length = 1024/8 = 128 字节
    rsa_len_ = RSA_size(rsa_);

    return true;
}

bool RsaManager::initByRSAPrivateKey(cs_t strPrivateKey)
{
    RAND_seed(rnd_seed, sizeof(rnd_seed));

    bp_private_ = BIO_new_mem_buf((void*)strPrivateKey.data(), -1);
    if (NULL == bp_private_)
    {
        LogError("Call BIO_new_mem_buf failed.");
        return false;
    }

    rsa_ = PEM_read_bio_RSAPrivateKey(bp_private_, NULL, NULL, NULL);
    if (NULL == rsa_)
    {
        ERR_load_crypto_strings();

        char err[512];
        ERR_error_string_n(ERR_get_error(), err, sizeof(err));

        LogError("Call PEM_read_bio_RSAPrivateKey failed. reason[%s].", err);
        return false;
    }

    // 对于1024bit的密钥,block length = 1024/8 = 128 字节
    rsa_len_ = RSA_size(rsa_);

    return true;
}

bool RsaManager::encrypt(cs_t strData, string& strAfterEncrypt)
{
    CHK_NULL_RETURN_FALSE(rsa_);

    strAfterEncrypt.clear();

    // 对于RSA_PKCS1_PADDING 填充模式,要求输入的明文必须比RSA钥模长(modulus) 短至少11个字节,即128-11=117
    int p = rsa_len_ - 11;

    int n = strData.length() / p + 1;
    LogDebug("RsaLen[%d], DataLength[%u], Need encrypt %d times.", rsa_len_, strData.length(), n);

    int start = 0;

    for (int i = 0; i < n; ++i)
    {
        string s = strData.substr(start, p);
        start = start + p;

        if (!encryptEx(s, strAfterEncrypt))
        {
            LogError("Call encryptEx failed.");
            return false;
        }
    }

    LogDebug("Encrypt success. Length[%u]\n%s.",
        strAfterEncrypt.length(), strAfterEncrypt.data());
    return true;
}

bool RsaManager::encryptEx(cs_t strData, string& strAfterEncrypt)
{
    CHK_NULL_RETURN_FALSE(rsa_);

    ERR_clear_error();

    int flen = strData.length();
    const unsigned char* from = (unsigned char*)strData.data();
    unsigned char* to = new unsigned char[rsa_len_]();

    int len = RSA_public_encrypt(flen, from, to, rsa_, RSA_PKCS1_PADDING);

    LogDebug("  -->rsa_len[%d], flen[%d], len[%d].", rsa_len_, flen, len);

    if (len != rsa_len_)
    {
        ERR_load_crypto_strings();

        char err[512] = {0};
        ERR_error_string_n(ERR_get_error(), err, sizeof(err));

        LogError("Call RSA_public_encrypt failed. reason[%s].", err);
        delete [] to;
        return false;
    }

    strAfterEncrypt.append((const char*)to, rsa_len_);

    delete [] to;
    return true;
}

bool RsaManager::decrypt(cs_t strData, string& strAfterDecrypt)
{
    CHK_NULL_RETURN_FALSE(rsa_);

    if (0 == rsa_len_)
    {
        LogError("Internal Error");
        return false;
    }

    strAfterDecrypt.clear();

    int n = strData.length() / rsa_len_;
    LogDebug("RsaLen[%d], DataLength[%u], Need decrypt %d times.", rsa_len_, strData.length(), n);

    int start = 0;

    for (int i = 0; i < n; ++i)
    {
        string s = strData.substr(start, rsa_len_);
        start = start + rsa_len_;

        if (!decryptEx(s, strAfterDecrypt))
        {
            LogError("Call decryptEx failed.");
            return false;
        }
    }

    LogDebug("Decrypt success. Length[%u].\n%s",
        strAfterDecrypt.length(), strAfterDecrypt.data());
    return true;

}

bool RsaManager::decryptEx(cs_t strData, string& strAfterDecrypt)
{
    CHK_NULL_RETURN_FALSE(rsa_);

    ERR_clear_error();

    int flen = strData.length();
    const unsigned char* from = (unsigned char*)strData.data();
    unsigned char* to = new unsigned char[rsa_len_]();

    int len = RSA_private_decrypt(flen, from, to, rsa_, RSA_PKCS1_PADDING);

    LogDebug("  -->rsa_len[%d], flen[%d], len[%d].", rsa_len_, flen, len);

    if (len == -1)
    {
        ERR_load_crypto_strings();

        char errBuf[512];
        ERR_error_string_n(ERR_get_error(), errBuf, sizeof(errBuf));

        LogError("Call RSA_private_decrypt failed. reason[%s].", errBuf);
        delete [] to;
        return false;
    }

    strAfterDecrypt.append((const char*)to, rsa_len_ - 11);
    LogDebug("strAfterDecrypt.length[%u, %s].", strAfterDecrypt.length(), (const char*)to);

    delete [] to;
    return true;
}


