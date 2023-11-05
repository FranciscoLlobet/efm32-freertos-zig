const std = @import("std");
const freertos = @import("freertos.zig");
const connection = @import("connection.zig");
pub const c = @cImport({
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
}); // pub in order to be able to import from client modules

const ciphersuites_psk = [_]c_int{
    c.MBEDTLS_TLS_PSK_WITH_AES_128_GCM_SHA256,
    c.MBEDTLS_TLS_PSK_WITH_AES_128_CCM_8,
    c.MBEDTLS_TLS_PSK_WITH_AES_128_CCM,
    0,
};

const ciphersuites_ec = [_]c_int{
    c.MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8,
    c.MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_CCM,
    0,
};

const sig_algorithms = [_]u16{
    c.MBEDTLS_TLS1_3_SIG_ECDSA_SECP256R1_SHA256,
    c.MBEDTLS_TLS1_3_SIG_NONE,
};

const groups = [_]u16{ c.MBEDTLS_SSL_IANA_TLS_GROUP_SECP256R1, c.MBEDTLS_SSL_IANA_TLS_GROUP_NONE };

pub const auth_error = error{
    no_callback,
    default_callback,
    generic_error,
    unsuported_mode,
};

pub const mbedtls_error = error{
    psk_conf_error,
    generic_error,
};

pub const init_error = error{};

const mbedtls_ok: i32 = 0;
const mbedtls_nok: i32 = -1;

pub const credential_callback_fn = *const fn (*@This(), connection.security_mode) auth_error!void;
pub const custom_init_callback_fn = *const fn (*@This(), connection.protocol, connection.security_mode) init_error!void;

/// Security mode
mode: connection.security_mode,

/// MbedTLS Context
context: c.mbedtls_ssl_context,

/// MbedTLS Configuration
config: c.mbedtls_ssl_config,

drbg: c.mbedtls_ctr_drbg_context,
entropy: c.mbedtls_entropy_context,
timer: c.miso_mbedtls_timing_delay_t,

/// Entropy Seed
entropy_seed: u32,

/// Authentication callback
auth_callback: ?credential_callback_fn = defaultAuth,

/// Custom init callback
custom_init_callback: ?custom_init_callback_fn = null,

const tls_read_timeout: u32 = 5000;

pub fn deinit(self: *@This()) i32 {
    var ret: i32 = 0;

    self.cleanup();

    return ret;
}

fn defaultAuth(self: *@This(), security_mode: connection.security_mode) auth_error!void {
    _ = self;
    _ = security_mode;

    return auth_error.default_callback;
}

pub fn cleanup(self: *@This()) void {
    c.miso_mbedtls_deinit_timer(&self.timer);
    c.mbedtls_ctr_drbg_free(&self.drbg);
    c.mbedtls_entropy_free(&self.entropy);
    c.mbedtls_ssl_free(&self.context);
    c.mbedtls_ssl_config_free(&self.config);
}

fn get_ctx(self: *@This()) *c.mbedtls_ssl_context {
    return &self.context;
}

pub fn create(auth_callback: ?credential_callback_fn, custom_init: ?custom_init_callback_fn) @This() {
    return @This(){ .auth_callback = auth_callback orelse defaultAuth, .custom_init_callback = custom_init, .context = undefined, .timer = undefined, .config = undefined, .drbg = undefined, .entropy = undefined, .mode = connection.security_mode.no_sec, .entropy_seed = 0x55555555 };
}

/// Helper function to get the credential callback function in custom init
pub fn getCredentialCallbackFn(self: *@This()) ?credential_callback_fn {
    return self.auth_callback;
}

pub fn init(self: *@This(), connection_ctx: *connection, protocol: connection.protocol, mode: connection.security_mode) !i32 {
    var ret: i32 = mbedtls_nok;

    if (!connection.protocol.isSecure(protocol))
        // Running init on a non secure protocol
        return mbedtls_nok; // should return a different error code

    self.mode = mode;

    // custom init callback
    if (self.custom_init_callback) |custom| {
        try custom(self, protocol, mode);
    } else {
        c.miso_mbedtls_init_timer(&self.timer);
        c.mbedtls_ssl_init(&self.context);
        c.mbedtls_ssl_config_init(&self.config);
        c.mbedtls_ctr_drbg_init(&self.drbg);
        c.mbedtls_entropy_init(&self.entropy);

        const transport: c_int = switch (protocol) {
            .dtls_ip4, .dtls_ip6 => c.MBEDTLS_SSL_TRANSPORT_DATAGRAM,
            else => c.MBEDTLS_SSL_TRANSPORT_STREAM,
        };

        ret = c.mbedtls_ssl_config_defaults(&self.config, c.MBEDTLS_SSL_IS_CLIENT, transport, c.MBEDTLS_SSL_PRESET_DEFAULT);
        if (ret == mbedtls_ok) {
            ret = c.mbedtls_ssl_conf_max_frag_len(&self.config, c.MBEDTLS_SSL_MAX_FRAG_LEN_1024);
        }
        if (ret == mbedtls_ok) {
            c.mbedtls_ssl_conf_authmode(&self.config, c.MBEDTLS_SSL_VERIFY_NONE); // None since using PSK
            c.mbedtls_ssl_conf_read_timeout(&self.config, tls_read_timeout);
            c.mbedtls_ssl_conf_rng(&self.config, c.mbedtls_ctr_drbg_random, &self.drbg);
            //mbedtls_entropy_add_source(&entropy_context, mbedtls_entropy_f_source_ptr f_source, void *p_source, size_t threshold, MBEDTLS_ENTROPY_SOURCE_STRONG );
            ret = c.mbedtls_ctr_drbg_seed(&self.drbg, c.mbedtls_entropy_func, &self.entropy, null, 0);
            if (ret == c.MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED) ret = mbedtls_ok;
        }

        if (ret == mbedtls_ok) {
            switch (mode) {
                .psk => c.mbedtls_ssl_conf_ciphersuites(&self.config, &ciphersuites_psk[0]),
                .certificate_ec => c.mbedtls_ssl_conf_ciphersuites(&self.config, &ciphersuites_ec[0]),
                else => {
                    // This authentication mode is not supported yet.
                    ret = mbedtls_nok;
                },
            }
        }
        if (ret == mbedtls_ok) {
            c.mbedtls_ssl_conf_min_tls_version(&self.config, c.MBEDTLS_SSL_VERSION_TLS1_2);
            c.mbedtls_ssl_conf_renegotiation(&self.config, c.MBEDTLS_SSL_RENEGOTIATION_ENABLED);

            // In case of DTLS set CID
            ret = switch (protocol) {
                .dtls_ip4, .dtls_ip6 => c.mbedtls_ssl_conf_cid(&self.config, 6, c.MBEDTLS_SSL_UNEXPECTED_CID_FAIL),
                .tls_ip4, .tls_ip6 => mbedtls_ok, // Probably not necessary
                else => mbedtls_nok,
            };
        }

        // Run the auth credentials callback
        if (ret == mbedtls_ok) {
            try self.auth_callback.?(self, self.mode);
        }

        if (ret == mbedtls_ok) {
            ret = c.mbedtls_ssl_setup(&self.context, &self.config);
        }

        if (ret == mbedtls_ok) {
            c.mbedtls_ssl_set_timer_cb(&self.context, &self.timer, c.miso_mbedtls_timing_set_delay, c.miso_mbedtls_timing_get_delay);
        }
    }

    // End the ssl initialization
    if (ret == mbedtls_ok) {
        ret = c.miso_network_register_ssl_context(@as(*c.struct_miso_sockets_s, @ptrCast(connection_ctx.ctx)), &self.context);
    }

    if (ret != mbedtls_ok) {
        // init failure
    }

    return ret;
}

pub fn confPsk(self: *@This(), psk: []u8, psk_id: [*:0]u8) !void {
    return if (mbedtls_ok != c.mbedtls_ssl_conf_psk(&self.config, psk.ptr, psk.len, &psk_id[0], c.strlen(psk_id))) mbedtls_error.psk_conf_error else {};
}

pub fn base64Decode(input: [*:0]u8, output: []u8) ![]u8 {
    var len: usize = 0;

    if (mbedtls_ok == c.mbedtls_base64_decode(output.ptr, output.len, &len, &input[0], c.strlen(input))) {
        return if (output.len >= len) output[0..len] else mbedtls_error.generic_error;
    }

    return mbedtls_error.generic_error;
}
