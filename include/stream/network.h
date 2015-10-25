/*
 * Copyright (C) 2012-2015 Jacob R. Lifshay
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
#ifndef NETWORK_H_INCLUDED
#define NETWORK_H_INCLUDED

#include "stream/stream.h"
#include <memory>
#include <string>

namespace programmerjake
{
namespace voxels
{
namespace stream
{

class NetworkException : public IOException
{
public:
    explicit NetworkException(std::string msg)
        : IOException(msg)
    {
    }
};

struct NetworkDisconnectedException final : public NetworkException
{
    NetworkDisconnectedException()
        : NetworkException("network disconnected")
    {
    }
};

enum class SSLCertificateValidationError
{
    /** no error
     */
    NoError,
    /** no certificate was supplied
     */
    NoCertificateSupplied,
    /** the issuer certificate could not be found
     */
    IssuerCertificateNotFound,
    /** the CRL of a certificate could not be found
     */
    CRLNotFound,
    /**
     */
    UnableToDecryptCertificateSignature,
    /**
     */
    UnableToDecryptCRLSignature,
    /**
     */
    UnableToDecodeIssuerPublicKey,
    /**
     */
    CertificateSignatureInvalid,
    /**
     */
    CRLSignatureInvalid,
    /**
     */
    CertificateNotYetValid,
    /**
     */
    CertificateHasExpired,
    /**
     */
    CRLNotYetValid,
    /**
     */
    CRLHasExpired,
    /**
     */
    FormatErrorInCertificateNotBeforeField,
    /**
     */
    FormatErrorInCertificateNotAfterField,
    /**
     */
    FormatErrorInCRLLastUpdateField,
    /**
     */
    FormatErrorInCRLNextUpdateField,
    /** the passed certificate is self-signed and is not found in the list of trusted certificates
     */
    UntrustedSelfSignedCertificate,
    /** the passed certificate's root certificate is not found in the list of trusted certificates
     */
    UntrustedRootCertificate,
    /** the issuer certificate of a local certificate could not be found
     */
    LocalIssuerCertificateNotFound,
    /**
     */
    LeafCertificateVerificationFailed,
    /**
     */
    CertificateChainTooLong,
    /**
     */
    CertificateRevoked,
    /**
     */
    InvalidCACertificate,
    /**
     */
    PathTooLong,
    /**
     */
    UnsupportedCertificatePurpose,
    /**
     */
    RootCACertificateUntrustedForPurpose,
    /**
     */
    RootCACertificateRejectsPurpose,
    /**
     */
    AuthorityKeyIdentifierDoesNotMatchSubjectKey,
    /**
     */
    AuthorityKeySerialNumberDoesNotMatchSubjectKey,
    /**
     */
    InvalidExtension,
    /**
     */
    InvalidPolicyExtension,
    /**
     */
    MissingExplicitPolicy,
    /**
     */
    DifferentCRLScope,
    /**
     */
    UnsupportedExtensionFeature,
    /**
     */
    NameConstraintViolationInPermittedSubtrees,
    /**
     */
    NameConstraintViolationInExcludedSubtrees,
    /**
     */
    UnsupportedNameConstraintsMinimumOrMaximum,
    /**
     */
    UnsupportedNameConstraintsType,
    /**
     */
    UnsupportedNameConstraintsSyntax,
    /**
     */
    CRLPathValidationError,
};

class SSLCertificateValidationException : public NetworkException
{
public:
    static std::string toMessage(SSLCertificateValidationError errorCode)
    {
        switch(errorCode)
        {
        case SSLCertificateValidationError::NoError:
            return "no error";
        case SSLCertificateValidationError::NoCertificateSupplied:
            return "no certificate was supplied";
        case SSLCertificateValidationError::IssuerCertificateNotFound:
            return "issuer certificate not found";
        case SSLCertificateValidationError::CRLNotFound:
            return "CRL not found";
        case SSLCertificateValidationError::UnableToDecryptCertificateSignature:
            return "unable to decrypt certificate signature";
        case SSLCertificateValidationError::UnableToDecryptCRLSignature:
            return "unable to decrypt CRL signature";
        case SSLCertificateValidationError::UnableToDecodeIssuerPublicKey:
            return "unable to decode issuer public key";
        case SSLCertificateValidationError::CertificateSignatureInvalid:
            return "certificate signature invalid";
        case SSLCertificateValidationError::CRLSignatureInvalid:
            return "CRL signature invalid";
        case SSLCertificateValidationError::CertificateNotYetValid:
            return "certificate not yet valid";
        case SSLCertificateValidationError::CertificateHasExpired:
            return "certificate has expired";
        case SSLCertificateValidationError::CRLNotYetValid:
            return "CRL not yet valid";
        case SSLCertificateValidationError::CRLHasExpired:
            return "CRL has expired";
        case SSLCertificateValidationError::FormatErrorInCertificateNotBeforeField:
            return "format error in certificate notBefore field";
        case SSLCertificateValidationError::FormatErrorInCertificateNotAfterField:
            return "format error in certificate notAfter field";
        case SSLCertificateValidationError::FormatErrorInCRLLastUpdateField:
            return "format error in CRL lastUpdate field";
        case SSLCertificateValidationError::FormatErrorInCRLNextUpdateField:
            return "format error in CRL nextUpdate field";
        case SSLCertificateValidationError::UntrustedSelfSignedCertificate:
            return "untrusted self-signed certificate";
        case SSLCertificateValidationError::UntrustedRootCertificate:
            return "untrusted root certificate";
        case SSLCertificateValidationError::LocalIssuerCertificateNotFound:
            return "certificate's issuer not found locally";
        case SSLCertificateValidationError::LeafCertificateVerificationFailed:
            return "leaf certificate verification failed";
        case SSLCertificateValidationError::CertificateChainTooLong:
            return "certificate chain too long";
        case SSLCertificateValidationError::CertificateRevoked:
            return "certificate revoked";
        case SSLCertificateValidationError::InvalidCACertificate:
            return "invalid CA certificate";
        case SSLCertificateValidationError::PathTooLong:
            return "path too long";
        case SSLCertificateValidationError::UnsupportedCertificatePurpose:
            return "unsupported certificate purpose";
        case SSLCertificateValidationError::RootCACertificateUntrustedForPurpose:
            return "root CA certificate untrusted for purpose";
        case SSLCertificateValidationError::RootCACertificateRejectsPurpose:
            return "root CA certificate rejects purpose";
        case SSLCertificateValidationError::AuthorityKeyIdentifierDoesNotMatchSubjectKey:
            return "authority key identifier does not match subject key";
        case SSLCertificateValidationError::AuthorityKeySerialNumberDoesNotMatchSubjectKey:
            return "authority key serial number does not match subject key";
        case SSLCertificateValidationError::InvalidExtension:
            return "invalid extension";
        case SSLCertificateValidationError::InvalidPolicyExtension:
            return "invalid policy extension";
        case SSLCertificateValidationError::MissingExplicitPolicy:
            return "missing explicit policy";
        case SSLCertificateValidationError::DifferentCRLScope:
            return "different CRL scope";
        case SSLCertificateValidationError::UnsupportedExtensionFeature:
            return "unsupported extension feature";
        case SSLCertificateValidationError::NameConstraintViolationInPermittedSubtrees:
            return "name constraint violation occurred in permitted subtrees";
        case SSLCertificateValidationError::NameConstraintViolationInExcludedSubtrees:
            return "name constraint violation occurred in excluded subtrees";
        case SSLCertificateValidationError::UnsupportedNameConstraintsMinimumOrMaximum:
            return "certificate name constraints field not supported: minimum or maximum";
        case SSLCertificateValidationError::UnsupportedNameConstraintsType:
            return "certificate name constraints type not supported";
        case SSLCertificateValidationError::UnsupportedNameConstraintsSyntax:
            return "certificate name constraints syntax invalid or not supported";
        case SSLCertificateValidationError::CRLPathValidationError:
            return "CRL path validation error";
        }
        UNREACHABLE();
    }
    const SSLCertificateValidationError errorCode;
    explicit SSLCertificateValidationException(SSLCertificateValidationError errorCode)
        : NetworkException("SSL Certificate Validation Error: " + toMessage(errorCode)),
        errorCode(errorCode)
    {
    }
};

namespace Network
{
std::shared_ptr<StreamRW> makeTCPConnection(std::wstring address, std::uint16_t defaultPort);
std::shared_ptr<StreamServer> makeTCPServer(std::uint16_t listenPort);
std::shared_ptr<StreamRW> makeSSLConnection(std::wstring address, std::uint16_t defaultPort, std::function<void (SSLCertificateValidationError errorCode)> handleCertificateValidationErrorFn = nullptr);
/**
 * @param listenPort the TCP port number to listen on
 * @param certificateFile the certificate file name
 * @param privateKeyFile the private key file name
 * @param getDecryptionPasswordFn the function called to get a decryption password.
 * If decryption fails, then the function is called again.
 * The function should throw when it can't return the password.
 * @return the new StreamServer
 */
std::shared_ptr<StreamServer> makeSSLServer(std::uint16_t listenPort,
                                            std::wstring certificateFile,
                                            std::wstring privateKeyFile,
                                            std::function<std::wstring()> getDecryptionPasswordFn = nullptr);
}
}
}
}

#endif // NETWORK_H_INCLUDED
