const std = @import("std");
const microzig = @import("microzig");

const include_path = [_][]const u8{
    "csrc/crypto/mbedtls/include",
    // "csrc/crypto/mbedtls/include/mbedtls",
};

const base_src_path = "csrc/crypto/mbedtls/library/";

const source_path = [_][]const u8{
    base_src_path ++ "aes.c",
    //base_src_path ++ "aesni.c",
    //base_src_path ++ "aria.c",
    base_src_path ++ "asn1parse.c",
    base_src_path ++ "asn1write.c",
    base_src_path ++ "base64.c",
    base_src_path ++ "bignum.c",
    base_src_path ++ "bignum_core.c",
    base_src_path ++ "bignum_mod.c",
    base_src_path ++ "bignum_mod_raw.c",
    //base_src_path ++ "camellia.c",
    base_src_path ++ "ccm.c",
    //base_src_path ++ "chacha20.c",
    //base_src_path ++ "chachapoly.c",
    base_src_path ++ "cipher.c",
    base_src_path ++ "cipher_wrap.c",
    base_src_path ++ "constant_time.c",
    base_src_path ++ "cmac.c",
    base_src_path ++ "ctr_drbg.c",
    base_src_path ++ "des.c",
    base_src_path ++ "dhm.c",
    base_src_path ++ "ecdh.c",
    base_src_path ++ "ecdsa.c",
    base_src_path ++ "ecjpake.c",
    base_src_path ++ "ecp.c",
    base_src_path ++ "ecp_curves.c",
    base_src_path ++ "entropy.c",
    base_src_path ++ "entropy_poll.c",
    base_src_path ++ "gcm.c",
    base_src_path ++ "hkdf.c",
    base_src_path ++ "hmac_drbg.c",
    base_src_path ++ "lmots.c",
    base_src_path ++ "lms.c",
    base_src_path ++ "md.c",
    base_src_path ++ "md5.c",
    base_src_path ++ "memory_buffer_alloc.c",
    base_src_path ++ "mps_reader.c",
    base_src_path ++ "mps_trace.c",
    base_src_path ++ "nist_kw.c",
    base_src_path ++ "oid.c",
    base_src_path ++ "padlock.c",
    base_src_path ++ "pem.c",
    base_src_path ++ "pk.c",
    base_src_path ++ "pk_wrap.c",
    base_src_path ++ "pkcs12.c",
    base_src_path ++ "pkcs5.c",
    base_src_path ++ "pkparse.c",
    base_src_path ++ "pkwrite.c",
    base_src_path ++ "platform.c",
    base_src_path ++ "platform_util.c",
    base_src_path ++ "poly1305.c",
    base_src_path ++ "psa_crypto.c",
    base_src_path ++ "psa_crypto_aead.c",
    base_src_path ++ "psa_crypto_cipher.c",
    base_src_path ++ "psa_crypto_client.c",
    base_src_path ++ "psa_crypto_ecp.c",
    base_src_path ++ "psa_crypto_hash.c",
    base_src_path ++ "psa_crypto_mac.c",
    base_src_path ++ "psa_crypto_pake.c",
    base_src_path ++ "psa_crypto_rsa.c",
    base_src_path ++ "psa_crypto_se.c",
    base_src_path ++ "psa_crypto_slot_management.c",
    base_src_path ++ "psa_crypto_storage.c",
    base_src_path ++ "psa_its_file.c",
    base_src_path ++ "rsa.c",
    base_src_path ++ "rsa_alt_helpers.c",
    base_src_path ++ "sha1.c",
    base_src_path ++ "sha256.c",
    base_src_path ++ "sha512.c",
    base_src_path ++ "threading.c",
    base_src_path ++ "timing.c",
    base_src_path ++ "version.c",
    base_src_path ++ "x509.c",
    base_src_path ++ "x509_create.c",
    base_src_path ++ "x509_crl.c",
    base_src_path ++ "x509_crt.c",
    base_src_path ++ "x509_csr.c",
    base_src_path ++ "x509write_crt.c",
    base_src_path ++ "x509write_csr.c",
    base_src_path ++ "ssl_cache.c",
    base_src_path ++ "ssl_ciphersuites.c",
    base_src_path ++ "ssl_client.c",
    base_src_path ++ "ssl_cookie.c",
    base_src_path ++ "ssl_msg.c",
    base_src_path ++ "ssl_ticket.c",
    base_src_path ++ "ssl_tls.c",
    base_src_path ++ "ssl_tls12_client.c",
    base_src_path ++ "ssl_tls12_server.c",
    base_src_path ++ "ssl_tls13_keys.c",
    base_src_path ++ "ssl_tls13_server.c",
    base_src_path ++ "ssl_tls13_client.c",
    base_src_path ++ "ssl_tls13_generic.c",
    "csrc/src/mbedtls_adapter/entropy.c",
    "csrc/src/mbedtls_adapter/gmttime.c",
    "csrc/src/mbedtls_adapter/timing.c",
    "csrc/src/mbedtls_adapter/treading.c",
};

const c_flags = [_][]const u8{ "-DMBEDTLS_CONFIG_FILE=\"miso_mbedtls_config.h\"", "-DEFM32GG390F1024", "-O2", "-fdata-sections", "-ffunction-sections" };

pub fn aggregate(exe: *microzig.Firmware) void {
    for (include_path) |path| {
        exe.addIncludePath(std.build.LazyPath{ .path = path });
    }

    for (source_path) |path| {
        exe.addCSourceFile(std.Build.Step.Compile.CSourceFile{ .file = std.build.LazyPath{ .path = path }, .flags = &c_flags });
    }
}
