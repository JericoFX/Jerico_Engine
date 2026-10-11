// Copyright (c) Contributors to the Open 3D Engine Project.
// SPDX-License-Identifier: Apache-2.0 OR MIT
#include <winsock2.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/x509.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/crypto.h>
#include <openssl/dtls1.h>
#include <openssl/engine.h>
#include <array>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#ifndef OPENSSL_THREADS
#error AzNetworking requires OpenSSL thread support
#endif
#if !defined(OPENSSL_NO_DYNAMIC_ENGINE) || defined(OPENSSL_NO_STATIC_ENGINE) || defined(OPENSSL_NO_CAPIENG) || defined(OPENSSL_NO_ENGINE)
#error Exact Windows package requires static engines and CAPI with dynamic engines disabled
#endif
static_assert(OPENSSL_VERSION_NUMBER == 0x101010ffL, "Exact source version required");
template<class T, auto Free> using Owned = std::unique_ptr<T, decltype(Free)>;
using Context = Owned<SSL_CTX, SSL_CTX_free>;
using Session = Owned<SSL, SSL_free>;
using Certificate = Owned<X509, X509_free>;
using Key = Owned<EVP_PKEY, EVP_PKEY_free>;
using Cipher = Owned<EVP_CIPHER_CTX, EVP_CIPHER_CTX_free>;

void Require(bool value, const char* label)
{
    if (!value) { ERR_print_errors_fp(stderr); throw std::runtime_error(label); }
}
void ExactVersion()
{
    Require(OpenSSL_version_num() == 0x101010ffL, "linked library version");
    Require(std::string(OpenSSL_version(OPENSSL_VERSION)) == "OpenSSL 1.1.1o  3 May 2022", "linked version text");
}
void Crypto()
{
    const std::array<unsigned char, 32> expectedDigest{{
        0xba,0x78,0x16,0xbf,0x8f,0x01,0xcf,0xea,0x41,0x41,0x40,0xde,0x5d,0xae,0x22,0x23,
        0xb0,0x03,0x61,0xa3,0x96,0x17,0x7a,0x9c,0xb4,0x10,0xff,0x61,0xf2,0x00,0x15,0xad}};
    std::array<unsigned char, 32> digest{};
    unsigned int digestSize = 0;
    Require(EVP_Digest("abc", 3, digest.data(), &digestSize, EVP_sha256(), nullptr) == 1 && digestSize == digest.size() && digest == expectedDigest, "SHA256 known answer");
    // NIST AES-GCM zero key/IV/plaintext example, 128-bit key and 96-bit IV.
    const std::array<unsigned char, 16> expectedCipher{{0x03,0x88,0xda,0xce,0x60,0xb6,0xa3,0x92,0xf3,0x28,0xc2,0xb9,0x71,0xb2,0xfe,0x78}};
    const std::array<unsigned char, 16> expectedTag{{0xab,0x6e,0x47,0xd4,0x2c,0xec,0x13,0xbd,0xf5,0x3a,0x67,0xb2,0x12,0x57,0xbd,0xdf}};
    std::array<unsigned char, 16> key{}, plain{}, ciphertext{}, tag{};
    std::array<unsigned char, 12> iv{};
    Cipher encryption(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
    Require(encryption != nullptr, "cipher allocation");
    int count = 0, finalCount = 0;
    Require(EVP_EncryptInit_ex(encryption.get(), EVP_aes_128_gcm(), nullptr, key.data(), iv.data()) == 1, "GCM init");
    Require(EVP_EncryptUpdate(encryption.get(), ciphertext.data(), &count, plain.data(), static_cast<int>(plain.size())) == 1 && count == 16, "GCM encrypt");
    Require(EVP_EncryptFinal_ex(encryption.get(), ciphertext.data() + count, &finalCount) == 1 && finalCount == 0, "GCM finish");
    Require(EVP_CIPHER_CTX_ctrl(encryption.get(), EVP_CTRL_GCM_GET_TAG, 16, tag.data()) == 1 && ciphertext == expectedCipher && tag == expectedTag, "GCM known ciphertext/tag");
    for (int corrupt = 0; corrupt != 3; ++corrupt)
    {
        auto received = ciphertext;
        auto receivedTag = tag;
        if (corrupt == 1) { received[0] ^= 1; }
        if (corrupt == 2) { receivedTag[0] ^= 1; }
        std::array<unsigned char, 32> decoded{};
        Cipher decryption(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
        Require(decryption != nullptr, "decoder allocation");
        Require(EVP_DecryptInit_ex(decryption.get(), EVP_aes_128_gcm(), nullptr, key.data(), iv.data()) == 1, "GCM decoder init");
        Require(EVP_DecryptUpdate(decryption.get(), decoded.data(), &count, received.data(), 16) == 1, "GCM decode");
        Require(EVP_CIPHER_CTX_ctrl(decryption.get(), EVP_CTRL_GCM_SET_TAG, 16, receivedTag.data()) == 1, "GCM tag setup");
        const int authenticated = EVP_DecryptFinal_ex(decryption.get(), decoded.data() + count, &finalCount);
        Require(corrupt ? authenticated != 1 : authenticated == 1 && std::equal(plain.begin(), plain.end(), decoded.begin()), "GCM authentication rejects corrupted ciphertext/tag");
    }
    std::array<unsigned char, 32> random{};
    Require(RAND_bytes(random.data(), static_cast<int>(random.size())) == 1 && RAND_status() == 1, "RAND ready/write");
    Require(std::any_of(random.begin(), random.end(), [](unsigned char byte) { return byte != 0; }), "RAND overwrites zero sentinel");
}
void Ownership()
{
    for (int iteration = 0; iteration != 64; ++iteration)
    {
        auto* memory = static_cast<unsigned char*>(OPENSSL_malloc(128));
        Require(memory != nullptr, "OpenSSL allocation from MD consumer");
        std::memset(memory, 0x5a, 128);
        auto* resized = static_cast<unsigned char*>(OPENSSL_realloc(memory, 256));
        if (!resized) { OPENSSL_free(memory); Require(false, "OpenSSL realloc"); }
        Require(std::all_of(resized, resized + 128, [](unsigned char byte) { return byte == 0x5a; }), "OpenSSL realloc preserves bytes");
        OPENSSL_free(resized);
        Owned<BIO, BIO_free> bio(BIO_new(BIO_s_mem()), BIO_free);
        Require(bio != nullptr && BIO_write(bio.get(), "owned", 5) == 5, "BIO memory write");
        char read[6]{};
        Require(BIO_read(bio.get(), read, 5) == 5 && std::strcmp(read, "owned") == 0, "BIO memory read");
        Context context(SSL_CTX_new(TLS_method()), SSL_CTX_free);
        Require(context != nullptr, "SSL context ownership");
        Session session(SSL_new(context.get()), SSL_free);
        Require(session != nullptr, "SSL ownership");
        // SSL holds its own context reference; release the caller's reference first.
        context.reset();
        Require(SSL_get_SSL_CTX(session.get()) != nullptr, "SSL retained context");
    }
}
bool Progress(SSL* session)
{
    const int result = SSL_do_handshake(session);
    if (result == 1) { return true; }
    const int error = SSL_get_error(session, result);
    Require(error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE, "TLS handshake progress");
    return false;
}
void Tls()
{
    Owned<EVP_PKEY_CTX, EVP_PKEY_CTX_free> generation(EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr), EVP_PKEY_CTX_free);
    Require(generation != nullptr && EVP_PKEY_keygen_init(generation.get()) == 1 && EVP_PKEY_CTX_set_rsa_keygen_bits(generation.get(), 2048) == 1, "local certificate key init");
    EVP_PKEY* rawKey = nullptr;
    Require(EVP_PKEY_keygen(generation.get(), &rawKey) == 1, "local certificate key generation");
    Key key(rawKey, EVP_PKEY_free);
    Certificate certificate(X509_new(), X509_free);
    Require(certificate != nullptr && X509_set_version(certificate.get(), 2) == 1 && ASN1_INTEGER_set(X509_get_serialNumber(certificate.get()), 1) == 1, "local X509 allocation/version");
    Require(X509_gmtime_adj(X509_getm_notBefore(certificate.get()), -60) != nullptr && X509_gmtime_adj(X509_getm_notAfter(certificate.get()), 3600) != nullptr && X509_set_pubkey(certificate.get(), key.get()) == 1, "local certificate validity/key");
    X509_NAME* subject = X509_get_subject_name(certificate.get());
    Require(X509_NAME_add_entry_by_txt(subject, "CN", MBSTRING_ASC, reinterpret_cast<const unsigned char*>("JE offline fixture"), -1, -1, 0) == 1 && X509_set_issuer_name(certificate.get(), subject) == 1 && X509_sign(certificate.get(), key.get(), EVP_sha256()) > 0, "self signed local certificate");
    Context server(SSL_CTX_new(TLS_server_method()), SSL_CTX_free), client(SSL_CTX_new(TLS_client_method()), SSL_CTX_free);
    Require(server != nullptr && client != nullptr, "TLS contexts");
    for (SSL_CTX* context : {server.get(), client.get()})
    {
        Require(SSL_CTX_set_min_proto_version(context, TLS1_3_VERSION) == 1 && SSL_CTX_set_max_proto_version(context, TLS1_3_VERSION) == 1 && SSL_CTX_set_ciphersuites(context, "TLS_AES_128_GCM_SHA256") == 1, "TLS1.3 exact bounds/cipher");
    }
    Require(SSL_CTX_use_certificate(server.get(), certificate.get()) == 1 && SSL_CTX_use_PrivateKey(server.get(), key.get()) == 1 && SSL_CTX_check_private_key(server.get()) == 1, "server certificate ownership");
    Require(X509_STORE_add_cert(SSL_CTX_get_cert_store(client.get()), certificate.get()) == 1, "client trusts exact generated certificate");
    SSL_CTX_set_verify(client.get(), SSL_VERIFY_PEER, nullptr);
    Session accepting(SSL_new(server.get()), SSL_free), connecting(SSL_new(client.get()), SSL_free);
    Require(accepting != nullptr && connecting != nullptr, "TLS sessions");
    BIO* clientBio = nullptr;
    BIO* serverBio = nullptr;
    Require(BIO_new_bio_pair(&clientBio, 0, &serverBio, 0) == 1, "memory transport");
    // Each SSL takes ownership of one endpoint for both read/write; no sockets.
    SSL_set_bio(connecting.get(), clientBio, clientBio);
    SSL_set_bio(accepting.get(), serverBio, serverBio);
    SSL_set_connect_state(connecting.get());
    SSL_set_accept_state(accepting.get());
    bool clientDone = false, serverDone = false;
    for (int step = 0; step != 1024 && !(clientDone && serverDone); ++step)
    {
        if (!clientDone) { clientDone = Progress(connecting.get()); }
        if (!serverDone) { serverDone = Progress(accepting.get()); }
    }
    Require(clientDone && serverDone && SSL_get_verify_result(connecting.get()) == X509_V_OK, "authenticated memory handshake");
    Require(SSL_version(connecting.get()) == TLS1_3_VERSION && SSL_version(accepting.get()) == TLS1_3_VERSION && std::string(SSL_get_cipher_name(connecting.get())) == "TLS_AES_128_GCM_SHA256", "negotiated TLS1.3 cipher");
    Certificate peer(SSL_get_peer_certificate(connecting.get()), X509_free);
    Require(peer != nullptr && X509_cmp(peer.get(), certificate.get()) == 0, "verified peer identity");
    const char payload[] = "JE memory TLS ownership";
    std::array<char, sizeof(payload)> received{};
    Require(SSL_write(connecting.get(), payload, static_cast<int>(sizeof(payload))) == static_cast<int>(sizeof(payload)), "TLS write");
    Require(SSL_read(accepting.get(), received.data(), static_cast<int>(received.size())) == static_cast<int>(received.size()) && std::memcmp(payload, received.data(), sizeof(payload)) == 0, "TLS authenticated plaintext roundtrip");
}
int GenerateCookie(SSL*, unsigned char* cookie, unsigned int* size) { cookie[0] = 0x2a; *size = 1; return 1; }
int VerifyCookie(SSL*, const unsigned char* cookie, unsigned int size) { return size == 1 && cookie[0] == 0x2a; }
void AzApi()
{
    Require(SSL_library_init() == 1, "AzNetworking SSL initialization");
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    const int index = SSL_get_ex_new_index(0, nullptr, nullptr, nullptr, nullptr);
    Require(index >= 0, "AzNetworking trust metadata index");
    for (const SSL_METHOD* method : {TLS_method(), TLS_client_method(), TLS_server_method(), DTLS_method(), DTLS_client_method(), DTLS_server_method()})
    {
        Context context(SSL_CTX_new(method), SSL_CTX_free);
        Require(context != nullptr && SSL_CTX_set_ex_data(context.get(), index, context.get()) == 1 && SSL_CTX_get_ex_data(context.get(), index) == context.get(), "AzNetworking contexts/trust metadata");
        SSL_CTX_set_mode(context.get(), SSL_MODE_AUTO_RETRY);
        SSL_CTX_set_verify(context.get(), SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, nullptr);
        Require(SSL_CTX_set_cipher_list(context.get(), "ECDHE-RSA-AES256-GCM-SHA384") == 1, "AzNetworking cipher");
        SSL_CTX_set_cookie_generate_cb(context.get(), GenerateCookie);
        SSL_CTX_set_cookie_verify_cb(context.get(), VerifyCookie);
        Require(SSL_CTX_set_ecdh_auto(context.get(), 1) == 1, "AzNetworking automatic curve compatibility macro");
    }
    ERR_clear_error();
    Require(ERR_get_error() == 0, "AzNetworking error queue");
}
void StaticEngines()
{
    Require(OPENSSL_init_crypto(OPENSSL_INIT_NO_LOAD_CONFIG | OPENSSL_INIT_ENGINE_CAPI, nullptr) == 1, "static CAPI registration without config");
    Owned<ENGINE, ENGINE_free> engine(ENGINE_get_first(), ENGINE_free);
    while (engine)
    {
        const char* id = ENGINE_get_id(engine.get());
        Require(id != nullptr, "registered engine identifier");
        if (std::strcmp(id, "capi") == 0) { break; }
        // get_next releases the current structural reference and owns the next.
        // release() prevents the RAII wrapper from freeing the old reference twice.
        engine.reset(ENGINE_get_next(engine.release()));
    }
    Require(engine != nullptr, "static CAPI is present; reject empty SDK fallback");
    const char* name = ENGINE_get_name(engine.get());
    Require(name != nullptr && std::strcmp(name, "CryptoAPI ENGINE") == 0, "static CAPI identity");
    Require(ENGINE_get_RSA(engine.get()) != nullptr, "static CAPI RSA method");
    Require(ENGINE_get_init_function(engine.get()) != nullptr && ENGINE_get_finish_function(engine.get()) != nullptr && ENGINE_get_destroy_function(engine.get()) != nullptr, "static CAPI lifecycle callbacks");
    Require(ENGINE_get_load_privkey_function(engine.get()) != nullptr && ENGINE_get_ssl_client_cert_function(engine.get()) != nullptr, "static CAPI key/certificate callbacks");
    // Structural registration only: do not initialize CAPI or access stores/keys.
    // PadLock/RDRAND registration is CPU dependent and is not an acceptance check.
}
int main(int argc, char** argv)
{
    try
    {
        Require(argc == 2, "test case argument");
        ExactVersion();
        const std::string test = argv[1];
        if (test == "crypto") { Crypto(); }
        else if (test == "ownership") { Ownership(); }
        else if (test == "tls") { Tls(); }
        else if (test == "azapi") { AzApi(); }
        else if (test == "static-engines") { StaticEngines(); }
        else { Require(false, "unknown test case"); }
        std::cout << test << " passed with linked OpenSSL1.1.1o, MD consumer\n";
        return 0;
    }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
