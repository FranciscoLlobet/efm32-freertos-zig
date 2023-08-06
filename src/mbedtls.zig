const std = @import("std");
const freertos = @import("freertos.zig");
const connection = @import("connection.zig");
const c = @cImport({
    @cDefine("MBEDTLS_CONFIG_FILE", "\"miso_mbedtls_config.h\"");
    @cInclude("miso_config.h");
    @cInclude("network.h");
    @cInclude("wifi_service.h");
    @cInclude("mbedtls/ctr_drbg.h");
    @cInclude("mbedtls/timing.h");
    @cInclude("mbedtls/aes.h");
    @cInclude("mbedtls/base64.h");
    @cInclude("mbedtls/net_sockets.h");
    @cInclude("mbedtls/entropy.h");
});

//const protocol = connection.protocol;

const ciphersuites_psk: [4]c_int = .{
    c.MBEDTLS_TLS_PSK_WITH_AES_128_GCM_SHA256,
    c.MBEDTLS_TLS_PSK_WITH_AES_128_CCM_8,
    c.MBEDTLS_TLS_PSK_WITH_AES_128_CCM,
    0,
};

const ciphersuites_ec: [1]c_int = .{
    0,
};

mode: connection.security_mode,
timer: c.miso_mbedtls_timing_delay_t,
context: c.mbedtls_ssl_context,
config: c.mbedtls_ssl_config,
drbg: c.mbedtls_ctr_drbg_context,
entropy: c.mbedtls_entropy_context,
entropy_seed: u32,
psk: [64]u8,
psk_len: usize,
//id: [128]u8,
//id_len: usize,

const tls_read_timeout: u32 = 5000;

pub fn deinit(self: *@This()) i32 {
    var ret: i32 = 0;

    self.cleanup();

    return ret;
}

fn cleanup(self: *@This()) void {
    c.miso_mbedtls_deinit_timer(&self.timer);
    c.mbedtls_ctr_drbg_free(&self.drbg);
    c.mbedtls_entropy_free(&self.entropy);
    c.mbedtls_ssl_free(&self.context);
    c.mbedtls_ssl_config_free(&self.config);
}

pub fn create() @This() {
    return @This(){ .context = undefined, .timer = undefined, .config = undefined, .drbg = undefined, .entropy = undefined, .psk = undefined, .psk_len = 0, .mode = connection.security_mode.no_sec, .entropy_seed = 0x55555555 };
}

pub fn init(self: *@This(), protocol: connection.protocol, mode: connection.security_mode) i32 {
    var ret: i32 = -1;

    ret = switch (protocol) {
        .dtls_ip4, .dtls_ip6, .tls_ip4, .tls_ip6 => 0,
        else => -1,
    };

    if (ret != 0) return -1; // Insupported protocol

    self.mode = mode;

    c.miso_mbedtls_init_timer(&self.timer);
    c.mbedtls_ssl_init(&self.context);
    c.mbedtls_ssl_config_init(&self.config);
    c.mbedtls_ctr_drbg_init(&self.drbg);
    c.mbedtls_entropy_init(&self.entropy);

    var transport: c_int = switch (protocol) {
        .dtls_ip4, .dtls_ip6 => c.MBEDTLS_SSL_TRANSPORT_DATAGRAM,
        else => c.MBEDTLS_SSL_TRANSPORT_STREAM,
    };

    ret = c.mbedtls_ssl_config_defaults(&self.config, c.MBEDTLS_SSL_IS_CLIENT, transport, c.MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret == 0) {
        ret = c.mbedtls_ssl_conf_max_frag_len(&self.config, c.MBEDTLS_SSL_MAX_FRAG_LEN_1024);
    }
    if (ret == 0) {
        c.mbedtls_ssl_conf_authmode(&self.config, c.MBEDTLS_SSL_VERIFY_NONE); // None since using PSK
        c.mbedtls_ssl_conf_read_timeout(&self.config, tls_read_timeout);
        c.mbedtls_ssl_conf_rng(&self.config, c.mbedtls_ctr_drbg_random, &self.drbg);
        //mbedtls_entropy_add_source(&entropy_context, mbedtls_entropy_f_source_ptr f_source, void *p_source, size_t threshold, MBEDTLS_ENTROPY_SOURCE_STRONG );
        ret = c.mbedtls_ctr_drbg_seed(&self.drbg, c.mbedtls_entropy_func, &self.entropy, null, 0);
        if (ret == c.MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED) ret = 0;
    }

    if (ret == 0) {
        switch (mode) {
            .psk => c.mbedtls_ssl_conf_ciphersuites(&self.config, &ciphersuites_psk[0]),
            .certificate_ec => c.mbedtls_ssl_conf_ciphersuites(&self.config, &ciphersuites_ec[0]),
            else => {},
        }
    }
    if (ret == 0) {
        c.mbedtls_ssl_conf_min_tls_version(&self.config, c.MBEDTLS_SSL_VERSION_TLS1_2);
        c.mbedtls_ssl_conf_renegotiation(&self.config, c.MBEDTLS_SSL_RENEGOTIATION_ENABLED);

        // In case of DTLS
        ret = switch (protocol) {
            .dtls_ip4, .dtls_ip6 => c.mbedtls_ssl_conf_cid(&self.config, 6, c.MBEDTLS_SSL_UNEXPECTED_CID_FAIL),
            else => 0,
        };
    }

    // This should be done by a callback
    if (ret == 0) {
        if (mode == .psk) {
            @memset(&self.psk, 0);

            ret = c.mbedtls_base64_decode(&self.psk, self.psk.len, &self.psk_len, c.config_get_mqtt_psk_key(), c.strlen(c.config_get_mqtt_psk_key()));
            if (ret == 0) {
                ret = c.mbedtls_ssl_conf_psk(&self.config, &self.psk, self.psk_len, c.config_get_mqtt_psk_id(), c.strlen(c.config_get_mqtt_psk_id()));
            }
        } else if (mode == .certificate_ec) {
            //
        }
    }

    if (ret == 0) {
        ret = c.mbedtls_ssl_setup(&self.context, &self.config);
    }

    if (ret == 0) {
        c.mbedtls_ssl_set_timer_cb(&self.context, &self.timer, c.miso_mbedtls_timing_set_delay, c.miso_mbedtls_timing_get_delay);
    }

    // register the context ?

    return ret;
}