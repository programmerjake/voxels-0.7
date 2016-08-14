/*
 * Copyright (C) 2012-2016 Jacob R. Lifshay
 * This file is part of Voxels.
 *
 * Voxels is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Voxels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Voxels; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#include "stream/network.h"
#include "util/password.h"
#include "util/util.h"
#include "util/logging.h"
#ifndef __ANDROID__
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/conf.h>
#include <openssl/rand.h>
#include <iostream>
#include <mutex>
#include <thread>
#include <cstdlib>
#include <new> // for std::bad_alloc
#include <random>
#include <exception>
#include <csignal>
#include <cerrno>
#include <sstream>

#if defined(_WIN64) || defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef min
#undef max
#elif defined(__ANDROID__) || defined(__APPLE__) || defined(__linux) || defined(__unix) \
    || defined(__posix)
#else
#error unrecognized platform
#endif

#ifndef OPENSSL_THREADS
#error OpenSSL must be configured to support threads
#endif // OPENSSL_THREADS

namespace programmerjake
{
namespace voxels
{
namespace stream
{
namespace
{
#if defined(_WIN64) || defined(_WIN32)
#elif defined(__ANDROID__) || defined(__APPLE__) || defined(__linux) || defined(__unix) \
    || defined(__posix)
initializer clearSigPipe([]()
                         {
                             std::signal(SIGPIPE, SIG_IGN);
                         });
#else
#error unrecognized platform
#endif

struct OpenSSLCallbacks final
{
    OpenSSLCallbacks() = delete;
    OpenSSLCallbacks(OpenSSLCallbacks &) = delete;
    OpenSSLCallbacks &operator=(OpenSSLCallbacks &) = delete;
    ~OpenSSLCallbacks() = delete;
    struct dynlock final
    {
        std::mutex lock;
        dynlock() : lock()
        {
        }
    };
    static struct CRYPTO_dynlock_value *dyn_create_function(const char *, int)
    {
        return reinterpret_cast<struct CRYPTO_dynlock_value *>(new dynlock);
    }
    static void dyn_destroy_function(struct CRYPTO_dynlock_value *lockIn, const char *, int)
    {
        if(lockIn == nullptr)
            return;
        delete reinterpret_cast<dynlock *>(lockIn);
    }
    static void dyn_lock_function(int mode, struct CRYPTO_dynlock_value *lockIn, const char *, int)
    {
        assert(lockIn != nullptr);
        dynlock *lock = reinterpret_cast<dynlock *>(lockIn);
        if(mode & CRYPTO_LOCK)
            lock->lock.lock();
        else
            lock->lock.unlock();
    }
    struct MutexArray final
    {
        std::mutex *const array;
        MutexArray(const MutexArray &) = delete;
        MutexArray &operator=(const MutexArray &) = delete;
        MutexArray(std::size_t count) : array(new std::mutex[count])
        {
        }
        ~MutexArray()
        {
            delete[] array;
        }
    };
    static void locking_function(int mode, int n, const char *, int)
    {
        static const std::size_t limit = CRYPTO_num_locks();
        std::size_t index = n;
        assert(index < limit);
        static MutexArray mutexArray(limit);
        if(mode & CRYPTO_LOCK)
            mutexArray.array[index].lock();
        else
            mutexArray.array[index].unlock();
    }
};

const std::size_t maxSingleReadWriteSize = 0x10000;

void throwOpenSSLError(unsigned long errorCode)
{
    std::string msg;
    msg.resize(0x100, '\0');
    ERR_error_string_n(errorCode, &msg[0], static_cast<int>(msg.size()));
    std::size_t index = msg.find_first_of('\0');
    if(index != std::string::npos)
        msg.erase(index);
    throw NetworkException("OpenSSL error: " + msg);
}

void throwOpenSSLError()
{
    unsigned long errorCode = ERR_get_error();
    if(errorCode == 0)
    {
#if defined(_WIN64) || defined(_WIN32)
        DWORD errnoValue = WSAGetLastError();
#elif defined(__ANDROID__) || defined(__APPLE__) || defined(__linux) || defined(__unix) \
    || defined(__posix)
        int errnoValue = errno;
#else
#error unrecognized platform
#endif
        if(errnoValue != 0)
        {
#if defined(_WIN64) || defined(_WIN32)
            if(errnoValue == WSAECONNRESET || errnoValue == WSAECONNABORTED || WSAESHUTDOWN)
            {
                throw NetworkDisconnectedException();
            }
            std::ostringstream ss;
            ss << "io error : socket error : " << errnoValue;
            throw NetworkException(ss.str());
#elif defined(__ANDROID) || defined(__APPLE__) || defined(__linux) || defined(__unix) \
    || defined(__posix)
            if(errnoValue == EPIPE)
            {
                throw NetworkDisconnectedException();
            }
            throw NetworkException("io error : " + std::string(std::strerror(errnoValue)));
#else
#error unrecognized platform
#endif
        }
    }
    throwOpenSSLError(ERR_get_error());
}

SSL_CTX *startOpenSSLLibAndClient()
{
    OpenSSL_add_all_ciphers();
    SSL_library_init();
    ERR_load_BIO_strings();
    SSL_load_error_strings();
    ERR_load_crypto_strings();
    ERR_load_X509_strings();
    OPENSSL_config(nullptr);
    OPENSSL_add_all_algorithms_conf();
    CRYPTO_set_dynlock_create_callback(OpenSSLCallbacks::dyn_create_function);
    CRYPTO_set_dynlock_destroy_callback(OpenSSLCallbacks::dyn_destroy_function);
    CRYPTO_set_dynlock_lock_callback(OpenSSLCallbacks::dyn_lock_function);
    CRYPTO_set_locking_callback(OpenSSLCallbacks::locking_function);
    static unsigned buf[0x1000 / sizeof(unsigned)] = {0};
    double addedEntropy;
    {
        std::random_device engine;
        addedEntropy = engine.entropy() * (sizeof(buf) / sizeof(unsigned));
        for(unsigned &v : buf)
        {
            v = engine();
        }
    }
    RAND_add(static_cast<const void *>(buf), sizeof(buf), addedEntropy);
    const SSL_METHOD *sslMethods;
#if(OPENSSL_VERSION_NUMBER >= 0x10100000L)
    sslMethods = TLS_client_method();
#else
    sslMethods = SSLv23_client_method();
#endif
    SSL_CTX *retval = SSL_CTX_new((SSL_METHOD *)sslMethods);
    if(retval == nullptr)
        throwOpenSSLError();
#ifndef SSL_OP_NO_COMPRESSION
#define SSL_OP_NO_COMPRESSION 0
#endif // SSL_OP_NO_COMPRESSION
    SSL_CTX_set_options(retval,
                        SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION);
    if(!SSL_CTX_set_default_verify_paths(retval))
        throwOpenSSLError();
    return retval;
}

SSL_CTX *initOpenSSLLibAndClient()
{
    static SSL_CTX *retval = startOpenSSLLibAndClient();
    return retval;
}

void initOpenSSL()
{
    initOpenSSLLibAndClient();
}

struct BIOReader final : public Reader
{
    std::shared_ptr<BIO> bio;
    std::shared_ptr<SSL_CTX> sslContext;
    explicit BIOReader(std::shared_ptr<BIO> bio, std::shared_ptr<SSL_CTX> sslContext = nullptr)
        : bio(bio), sslContext(nullptr)
    {
    }
    virtual ~BIOReader()
    {
        bio = nullptr; // must be first
        sslContext = nullptr;
    }
    virtual std::size_t readBytes(std::uint8_t *array, std::size_t maxCount) override
    {
        std::size_t readByteCount = 0;
        for(;;)
        {
            int currentReadLen = (int)maxCount;
            if(maxCount > maxSingleReadWriteSize)
                currentReadLen = maxSingleReadWriteSize;
            int readRetVal = BIO_read(bio.get(), (void *)array, currentReadLen);
            if(readRetVal > 0)
            {
                maxCount -= readRetVal;
                array += readRetVal;
                readByteCount += readRetVal;
                if(maxCount == 0)
                    break;
                continue;
            }
            else if(readRetVal == 0)
            {
                break;
            }
            if(BIO_should_retry(bio.get()))
                continue;
            throwOpenSSLError();
        }
        return readByteCount;
    }
    virtual std::uint8_t readByte() override
    {
        std::uint8_t retval;
        readAllBytes(&retval, 1);
        return retval;
    }
};

struct BIOWriter final : public Writer
{
    std::shared_ptr<BIO> bio;
    std::shared_ptr<SSL_CTX> sslContext;
    explicit BIOWriter(std::shared_ptr<BIO> bio, std::shared_ptr<SSL_CTX> sslContext = nullptr)
        : bio(bio), sslContext(nullptr)
    {
    }
    virtual ~BIOWriter()
    {
        bio = nullptr; // must be first
        sslContext = nullptr;
    }
    virtual void writeByte(std::uint8_t byte) override
    {
        writeBytes(&byte, 1);
    }
    virtual void writeBytes(const std::uint8_t *array, std::size_t count) override
    {
        for(;;)
        {
            int currentWriteLen = (int)count;
            if(count > maxSingleReadWriteSize)
                currentWriteLen = maxSingleReadWriteSize;
            int writeRetVal = BIO_write(bio.get(), (const void *)array, currentWriteLen);
            if(writeRetVal > 0)
            {
                count -= writeRetVal;
                array += writeRetVal;
                if(count == 0)
                    return;
                continue;
            }
            if(BIO_should_retry(bio.get()))
                continue;
            throwOpenSSLError();
        }
    }
    virtual void flush() override
    {
        while(1 != BIO_flush(bio.get()))
        {
            if(BIO_should_retry(bio.get()))
                continue;
            throwOpenSSLError();
        }
    }
};

std::wstring handleAddressDefaultPort(std::wstring address, std::uint16_t defaultPort)
{
    if(address.find_last_of(L':') != std::wstring::npos)
        return std::move(address);
    std::wostringstream ss;
    ss << address << L":" << (unsigned)defaultPort;
    return std::move(ss).str();
}

struct MyStreamServer final : public StreamServer
{
    std::shared_ptr<BIO> bio;
    std::shared_ptr<SSL_CTX> sslContext;
    explicit MyStreamServer(std::shared_ptr<BIO> bio, std::shared_ptr<SSL_CTX> sslContext = nullptr)
        : bio(bio), sslContext(sslContext)
    {
    }
    virtual ~MyStreamServer()
    {
        bio = nullptr; // must be first
        sslContext = nullptr;
    }
    virtual std::shared_ptr<StreamRW> accept() override
    {
        while(BIO_do_accept(bio.get()) <= 0)
        {
            if(BIO_should_retry(bio.get()))
                continue;
            throwOpenSSLError();
        }
        std::shared_ptr<BIO> retval = std::shared_ptr<BIO>(BIO_pop(bio.get()),
                                                           [](BIO *pbio)
                                                           {
                                                               BIO_free_all(pbio);
                                                           });
        return std::make_shared<StreamRWWrapper>(std::make_shared<BIOReader>(retval, sslContext),
                                                 std::make_shared<BIOWriter>(retval, sslContext));
    }
};

SSLCertificateValidationError translateOpenSSLX509ErrorCode(long errorCode)
{
    switch(errorCode)
    {
    case X509_V_OK:
        return SSLCertificateValidationError::NoError;
    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
        return SSLCertificateValidationError::IssuerCertificateNotFound;
    case X509_V_ERR_UNABLE_TO_GET_CRL:
        return SSLCertificateValidationError::CRLNotFound;
    case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE:
        return SSLCertificateValidationError::UnableToDecryptCertificateSignature;
    case X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE:
        return SSLCertificateValidationError::UnableToDecryptCRLSignature;
    case X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY:
        return SSLCertificateValidationError::UnableToDecodeIssuerPublicKey;
    case X509_V_ERR_CERT_SIGNATURE_FAILURE:
        return SSLCertificateValidationError::CertificateSignatureInvalid;
    case X509_V_ERR_CRL_SIGNATURE_FAILURE:
        return SSLCertificateValidationError::CRLSignatureInvalid;
    case X509_V_ERR_CERT_NOT_YET_VALID:
        return SSLCertificateValidationError::CertificateNotYetValid;
    case X509_V_ERR_CERT_HAS_EXPIRED:
        return SSLCertificateValidationError::CertificateHasExpired;
    case X509_V_ERR_CRL_NOT_YET_VALID:
        return SSLCertificateValidationError::CRLNotYetValid;
    case X509_V_ERR_CRL_HAS_EXPIRED:
        return SSLCertificateValidationError::CRLHasExpired;
    case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
        return SSLCertificateValidationError::FormatErrorInCertificateNotBeforeField;
    case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
        return SSLCertificateValidationError::FormatErrorInCertificateNotAfterField;
    case X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD:
        return SSLCertificateValidationError::FormatErrorInCRLLastUpdateField;
    case X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD:
        return SSLCertificateValidationError::FormatErrorInCRLNextUpdateField;
    case X509_V_ERR_OUT_OF_MEM:
        UNREACHABLE(); // should be caught before
    case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
        return SSLCertificateValidationError::UntrustedSelfSignedCertificate;
    case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
        return SSLCertificateValidationError::UntrustedRootCertificate;
    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
        return SSLCertificateValidationError::LocalIssuerCertificateNotFound;
    case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
        return SSLCertificateValidationError::LeafCertificateVerificationFailed;
    case X509_V_ERR_CERT_CHAIN_TOO_LONG:
        return SSLCertificateValidationError::CertificateChainTooLong;
    case X509_V_ERR_CERT_REVOKED:
        return SSLCertificateValidationError::CertificateRevoked;
    case X509_V_ERR_INVALID_CA:
        return SSLCertificateValidationError::InvalidCACertificate;
    case X509_V_ERR_PATH_LENGTH_EXCEEDED:
        return SSLCertificateValidationError::PathTooLong;
    case X509_V_ERR_INVALID_PURPOSE:
        return SSLCertificateValidationError::UnsupportedCertificatePurpose;
    case X509_V_ERR_CERT_UNTRUSTED:
        return SSLCertificateValidationError::RootCACertificateUntrustedForPurpose;
    case X509_V_ERR_CERT_REJECTED:
        return SSLCertificateValidationError::RootCACertificateRejectsPurpose;
    case X509_V_ERR_SUBJECT_ISSUER_MISMATCH:
        return SSLCertificateValidationError::NoError;
    case X509_V_ERR_AKID_SKID_MISMATCH:
        return SSLCertificateValidationError::AuthorityKeyIdentifierDoesNotMatchSubjectKey;
    case X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH:
        return SSLCertificateValidationError::AuthorityKeySerialNumberDoesNotMatchSubjectKey;
    case X509_V_ERR_KEYUSAGE_NO_CERTSIGN:
        return SSLCertificateValidationError::NoError;
    case X509_V_ERR_INVALID_EXTENSION:
        return SSLCertificateValidationError::InvalidExtension;
    case X509_V_ERR_INVALID_POLICY_EXTENSION:
        return SSLCertificateValidationError::InvalidPolicyExtension;
    case X509_V_ERR_NO_EXPLICIT_POLICY:
        return SSLCertificateValidationError::MissingExplicitPolicy;
#ifdef X509_V_ERR_DIFFERENT_CRL_SCOPE
    case X509_V_ERR_DIFFERENT_CRL_SCOPE:
        return SSLCertificateValidationError::DifferentCRLScope;
#endif
#ifdef X509_V_ERR_UNSUPPORTED_EXTENSION_FEATURE
    case X509_V_ERR_UNSUPPORTED_EXTENSION_FEATURE:
        return SSLCertificateValidationError::UnsupportedExtensionFeature;
#endif
#ifdef X509_V_ERR_PERMITTED_VIOLATION
    case X509_V_ERR_PERMITTED_VIOLATION:
        return SSLCertificateValidationError::NameConstraintViolationInPermittedSubtrees;
#endif
#ifdef X509_V_ERR_EXCLUDED_VIOLATION
    case X509_V_ERR_EXCLUDED_VIOLATION:
        return SSLCertificateValidationError::NameConstraintViolationInExcludedSubtrees;
#endif
#ifdef X509_V_ERR_SUBTREE_MINMAX
    case X509_V_ERR_SUBTREE_MINMAX:
        return SSLCertificateValidationError::UnsupportedNameConstraintsMinimumOrMaximum;
#endif
#ifdef X509_V_ERR_UNSUPPORTED_CONSTRAINT_TYPE
    case X509_V_ERR_UNSUPPORTED_CONSTRAINT_TYPE:
        return SSLCertificateValidationError::UnsupportedNameConstraintsType;
#endif
#ifdef X509_V_ERR_UNSUPPORTED_CONSTRAINT_SYNTAX
    case X509_V_ERR_UNSUPPORTED_CONSTRAINT_SYNTAX:
        return SSLCertificateValidationError::UnsupportedNameConstraintsSyntax;
#endif
#ifdef X509_V_ERR_CRL_PATH_VALIDATION_ERROR
    case X509_V_ERR_CRL_PATH_VALIDATION_ERROR:
        return SSLCertificateValidationError::CRLPathValidationError;
#endif
    case X509_V_ERR_APPLICATION_VERIFICATION:
        UNREACHABLE();
    }
    UNREACHABLE();
}
}

std::shared_ptr<StreamRW> Network::makeTCPConnection(std::wstring address,
                                                     std::uint16_t defaultPort)
{
    initOpenSSL();
    address = handleAddressDefaultPort(std::move(address), defaultPort);
    std::shared_ptr<BIO> bio =
        std::shared_ptr<BIO>(BIO_new_connect((char *)string_cast<std::string>(address).c_str()),
                             [](BIO *pbio)
                             {
                                 BIO_free_all(pbio);
                             });
    if(bio == nullptr)
        throwOpenSSLError();
    if(BIO_do_connect(bio.get()) <= 0)
        throwOpenSSLError();
    return std::make_shared<StreamRWWrapper>(std::make_shared<BIOReader>(bio),
                                             std::make_shared<BIOWriter>(bio));
}

std::shared_ptr<StreamServer> Network::makeTCPServer(std::uint16_t listenPort)
{
    initOpenSSL();
    std::ostringstream ss;
    ss << "*:" << listenPort;
    std::shared_ptr<BIO> bio = std::shared_ptr<BIO>(BIO_new_accept((char *)ss.str().c_str()),
                                                    [](BIO *pbio)
                                                    {
                                                        BIO_free_all(pbio);
                                                    });
    if(bio == nullptr)
        throwOpenSSLError();
    if(BIO_do_accept(bio.get()) <= 0)
        throwOpenSSLError();
    return std::make_shared<MyStreamServer>(bio);
}

std::shared_ptr<StreamRW> Network::makeSSLConnection(
    std::wstring address,
    std::uint16_t defaultPort,
    std::function<void(SSLCertificateValidationError errorCode)> handleCertificateValidationErrorFn)
{
    if(handleCertificateValidationErrorFn == nullptr)
    {
        handleCertificateValidationErrorFn = [](SSLCertificateValidationError errorCode)
        {
            throw SSLCertificateValidationException(errorCode);
        };
    }
    SSL_CTX *ctx = initOpenSSLLibAndClient();
    address = handleAddressDefaultPort(std::move(address), defaultPort);
    std::shared_ptr<BIO> bio = std::shared_ptr<BIO>(BIO_new_ssl_connect(ctx),
                                                    [](BIO *pbio)
                                                    {
                                                        BIO_free_all(pbio);
                                                    });
    if(bio == nullptr)
        throwOpenSSLError();
    SSL *ssl;
    if(BIO_get_ssl(bio.get(), &ssl) <= 0)
        throwOpenSSLError();
    SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
    if(BIO_set_conn_hostname(bio.get(), (char *)string_cast<std::string>(address).c_str()) <= 0)
        throwOpenSSLError();
    std::size_t colonLocation = address.find_last_of(L':');
    assert(colonLocation != std::wstring::npos);
#ifdef __ANDROID__
// SSL_set_tlsext_host_name(ssl, (char *)string_cast<std::string>(address.substr(0,
// colonLocation)).c_str());
#else
    SSL_set_tlsext_host_name(
        ssl, (char *)string_cast<std::string>(address.substr(0, colonLocation)).c_str());
#endif
    SSL_set_cipher_list(ssl, "ALL:!EXPORT:!EXPORT40:!EXPORT56:!aNULL:!LOW:!RC4:@STRENGTH");
    if(BIO_do_connect(bio.get()) <= 0)
        throwOpenSSLError();
    std::shared_ptr<X509> peerCertificate = std::shared_ptr<X509>(SSL_get_peer_certificate(ssl),
                                                                  [](X509 *v)
                                                                  {
                                                                      X509_free(v);
                                                                  });
    if(peerCertificate == nullptr)
    {
        handleCertificateValidationErrorFn(SSLCertificateValidationError::NoCertificateSupplied);
    }
    else
    {
        long verifyResult = SSL_get_verify_result(ssl);
        if(verifyResult == X509_V_ERR_OUT_OF_MEM)
        {
            throw std::bad_alloc();
        }
        bool handledError = false;
        if(verifyResult != X509_V_OK)
        {
            SSLCertificateValidationError errorCode = translateOpenSSLX509ErrorCode(verifyResult);
            if(errorCode != SSLCertificateValidationError::NoError)
            {
                handleCertificateValidationErrorFn(errorCode);
                handledError = true;
            }
        }
        if(!handledError)
        {
        }
    }
    return std::make_shared<StreamRWWrapper>(std::make_shared<BIOReader>(bio),
                                             std::make_shared<BIOWriter>(bio));
}

std::shared_ptr<StreamServer> Network::makeSSLServer(
    std::uint16_t listenPort,
    std::wstring certificateFile,
    std::wstring privateKeyFile,
    std::function<std::wstring()> getDecryptionPasswordFn)
{
    if(getDecryptionPasswordFn == nullptr)
    {
        getDecryptionPasswordFn = []() -> std::wstring
        {
            throw NetworkException("decryption password needed");
        };
    }
    initOpenSSL();
    struct PasswordFunctionUserData final
    {
        std::function<std::wstring()> getDecryptionPasswordFn;
        std::exception_ptr thrownException;
        PasswordFunctionUserData() : getDecryptionPasswordFn(), thrownException()
        {
        }
    };
    PasswordFunctionUserData *passwordUserData = new PasswordFunctionUserData();
    const SSL_METHOD *sslMethods;
#if(OPENSSL_VERSION_NUMBER >= 0x10100000L)
    sslMethods = TLS_server_method();
#else
    sslMethods = SSLv23_server_method();
#endif
    std::shared_ptr<SSL_CTX> ctx = std::shared_ptr<SSL_CTX>(SSL_CTX_new((SSL_METHOD *)sslMethods),
                                                            [passwordUserData](SSL_CTX *ctx)
                                                            {
                                                                SSL_CTX_free(ctx);
                                                                delete passwordUserData;
                                                            });
    if(ctx == nullptr)
    {
        delete passwordUserData;
        throwOpenSSLError();
    }
    SSL_CTX_set_options(ctx.get(),
                        SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION);
    SSL_CTX_set_default_passwd_cb(
        ctx.get(),
        [](char *buf, int size, int rwFlag, void *userDataIn) -> int
        {
            assert(!rwFlag);
            PasswordFunctionUserData *userData = (PasswordFunctionUserData *)userDataIn;
            try
            {
                assert(userData->getDecryptionPasswordFn);
                std::string password =
                    string_cast<std::string>(userData->getDecryptionPasswordFn());
                if(password.size() > (std::size_t)size)
                {
                    return -1;
                }
                for(std::size_t i = 0; i < password.size(); i++)
                    buf[i] = password[i];
                return password.size();
            }
            catch(...)
            {
                userData->thrownException = std::current_exception();
            }
            return -1;
        });
    SSL_CTX_set_default_passwd_cb_userdata(ctx.get(), (void *)passwordUserData);

    passwordUserData->getDecryptionPasswordFn = []() -> std::wstring
    {
        throw NetworkException("decryption of public certificate not supported");
    };
    int SSL_CTX_use_certificate_file_retval = SSL_CTX_use_certificate_file(
        ctx.get(), string_cast<std::string>(certificateFile).c_str(), SSL_FILETYPE_PEM);
    if(passwordUserData->thrownException != nullptr)
        std::rethrow_exception(passwordUserData->thrownException);
    passwordUserData->getDecryptionPasswordFn = nullptr;
    if(!SSL_CTX_use_certificate_file_retval)
    {
        throwOpenSSLError();
    }
    for(;;)
    {
        passwordUserData->getDecryptionPasswordFn = getDecryptionPasswordFn;
        int SSL_CTX_use_PrivateKey_file_retval = SSL_CTX_use_PrivateKey_file(
            ctx.get(), string_cast<std::string>(privateKeyFile).c_str(), SSL_FILETYPE_PEM);
        if(passwordUserData->thrownException != nullptr)
            std::rethrow_exception(passwordUserData->thrownException);
        passwordUserData->getDecryptionPasswordFn = nullptr;
        if(!SSL_CTX_use_PrivateKey_file_retval)
        {
            unsigned long errorCode = ERR_get_error();
            if(ERR_GET_LIB(errorCode) == ERR_LIB_PEM
               && (ERR_GET_REASON(errorCode) == PEM_R_BAD_DECRYPT
                   || ERR_GET_REASON(errorCode) == PEM_R_BAD_PASSWORD_READ))
                continue;
            throwOpenSSLError(errorCode);
        }
        else
        {
            break;
        }
    }
    if(!SSL_CTX_check_private_key(ctx.get()))
    {
        throwOpenSSLError();
    }
    BIO *sslBio = BIO_new_ssl(ctx.get(), 0);
    if(sslBio == nullptr)
    {
        throwOpenSSLError();
    }
    SSL *ssl;
    if(BIO_get_ssl(sslBio, &ssl) <= 0)
    {
        unsigned long errorCode = ERR_get_error();
        BIO_free_all(sslBio);
        throwOpenSSLError(errorCode);
    }
    SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
    SSL_set_cipher_list(ssl, "ALL:!EXPORT:!EXPORT40:!EXPORT56:!aNULL:!LOW:!RC4:@STRENGTH");
    std::shared_ptr<BIO> retval;
    try
    {
        std::ostringstream ss;
        ss << "*:" << listenPort;
        retval = std::shared_ptr<BIO>(BIO_new_accept((char *)ss.str().c_str()),
                                      [](BIO *p)
                                      {
                                          BIO_free_all(p);
                                      });
        if(retval == nullptr)
            throwOpenSSLError();
        BIO_set_accept_bios(retval.get(), sslBio);
    }
    catch(...)
    {
        BIO_free_all(sslBio);
        throw;
    }
    if(BIO_do_accept(retval.get()) <= 0)
        throwOpenSSLError();
    return std::make_shared<MyStreamServer>(retval, ctx);
}

#if 0
namespace
{
initializer init1([]()
{
    try
    {
        bool gotPassword = false;
        std::shared_ptr<StreamRW> connection = Network::makeSSLServer(4433, L"ssl_test_cert.pem", L"ssl_test_key.pem", [&]()
        {
            if(gotPassword)
                throw IOException("password decode failed");
            gotPassword = true;
            return L"password";
        })->accept();
        for(;;)
        {
            const std::size_t bufferSize = 0x1000;
            static char buffer[bufferSize];
            std::size_t readSize = connection->preader()->readBytes((std::uint8_t *)buffer, bufferSize);
            if(readSize == 0)
                break;
            std::cout.write(buffer, readSize);
        }
    }
    catch(IOException &e)
    {
        std::cerr << "error: " << e.what() << std::endl;
    }
    std::exit(0);
});
}
#endif
}

std::string Password::sha256HashString(std::string str)
{
    stream::initOpenSSL();
    unsigned char outputBuffer[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char *)str.data(), str.size(), outputBuffer);
    std::ostringstream ss;
    ss.fill('0');
    ss << std::hex << std::nouppercase;
    for(unsigned char byte : outputBuffer)
    {
        ss.width(2);
        ss << (unsigned)byte;
    }
    return ss.str();
}
}
}
#else // ifndef __ANDROID__
#include <stdexcept>
namespace programmerjake
{
namespace voxels
{
std::string Password::sha256HashString(std::string str)
{
    FIXME_MESSAGE(finish implementing SHA256)
    assert(!"finish implementing SHA256");
    throw std::runtime_error("finish implementing SHA256");
}

namespace stream
{
std::shared_ptr<StreamServer> Network::makeSSLServer(
    std::uint16_t listenPort,
    std::wstring certificateFile,
    std::wstring privateKeyFile,
    std::function<std::wstring()> getDecryptionPasswordFn)
{
    FIXME_MESSAGE(finish implementing makeSSLServer)
    throw NetworkException("finish implementing makeSSLServer");
}

std::shared_ptr<StreamRW> Network::makeSSLConnection(
    std::wstring address,
    std::uint16_t defaultPort,
    std::function<void(SSLCertificateValidationError errorCode)> handleCertificateValidationErrorFn)
{
    FIXME_MESSAGE(finish implementing makeSSLConnection)
    throw NetworkException("finish implementing makeSSLConnection");
}

std::shared_ptr<StreamServer> Network::makeTCPServer(std::uint16_t listenPort)
{
    FIXME_MESSAGE(finish implementing makeTCPServer)
    throw NetworkException("finish implementing makeTCPServer");
}

std::shared_ptr<StreamRW> Network::makeTCPConnection(std::wstring address,
                                                     std::uint16_t defaultPort)
{
    FIXME_MESSAGE(finish implementing makeTCPConnection)
    throw NetworkException("finish implementing makeTCPConnection");
}
}
}
}
#endif
