const std = @import("std");
const connection = @import("connection.zig");
const c = @cImport({
    @cDefine("MBEDTLS_CONFIG_FILE", "\"miso_mbedtls_config.h\"");
    @cInclude("mbedtls/ctr_drbg.h");
    @cInclude("mbedtls/timing.h");
    @cInclude("mbedtls/aes.h");
    @cInclude("mbedtls/base64.h");
    @cInclude("mbedtls/net_sockets.h");
    @cInclude("mbedtls/entropy.h");
});

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

pub const mbedtls_error = error{ psk_conf_error, generic_error, init_error, no_sec };

pub const init_error = error{};

const mbedtls_ok: i32 = 0;
const mbedtls_nok: i32 = -1;
const tls_read_timeout: u32 = 5000;
pub const mbedtls_ssl_context = c.mbedtls_ssl_context;
pub const mbedtls_ssl_config = c.mbedtls_ssl_config;

/// mbedTLS context
pub fn TlsContext(comptime T: type, comptime connType: type, comptime mode: connection.security_mode) type {
    // Mbedtls context
    if (connType != void) {
        if (!@hasDecl(connType, "open"))
            @compileError("Connection type must have an open method");
        if (!@hasDecl(connType, "close"))
            @compileError("Connection type must have a close method");
        if (!@hasDecl(connType, "recieve_c"))
            @compileError("Connection type must have a recieve_c method");
        if (!@hasDecl(connType, "send_c"))
            @compileError("Connection type must have a send_c method");
    }

    return struct {
        /// Credential callback function
        const credential_callback_fn = *const fn (*T, connection.security_mode) auth_error!void;

        const custom_init_callback_fn = *const fn (*@This(), connection.security_mode) void;
        const custom_cleanup_callback_fn = *const fn (*@This()) void;

        /// Parent type
        parent: *T,

        /// Connection
        conn: connType,

        /// MbedTLS Context
        context: mbedtls_ssl_context,
        /// MbedTLS Configuration
        config: mbedtls_ssl_config,
        /// MbedTLS DRBG
        drbg: c.mbedtls_ctr_drbg_context,
        /// MbedTLS Entropy context
        entropy: c.mbedtls_entropy_context,
        /// MbedTLS Timer
        timer: c.miso_mbedtls_timing_delay_t,

        auth_callback: credential_callback_fn,

        // Certificate mode
        ec: if (mode == connection.security_mode.certificate_ec) struct {
            /// Own CRT Context
            own_crt: c.mbedtls_x509_crt,
            /// Peer certificate context
            peer_crt: c.mbedtls_x509_crt,
            /// Peer CRL context
            peer_crl: c.mbedtls_x509_crl,
            /// Own PK
            own_pk: c.mbedtls_pk_context,
        } else struct {},
        /// Entropy Seed
        entropy_seed: u32,

        /// DTLS CID
        cid: [c.MBEDTLS_SSL_CID_IN_LEN_MAX]u8,

        /// Custom init callback
        custom_init_callback: ?custom_init_callback_fn = null,

        /// Custom cleanup callback
        custom_cleanup_callback: ?custom_cleanup_callback_fn = null,

        // Default auth callback
        pub fn create(parent: *T, comptime auth_callback: credential_callback_fn, custom_init: ?custom_init_callback_fn, custom_cleanup: ?custom_cleanup_callback_fn) @This() {
            return @This(){ .parent = parent, .conn = undefined, .auth_callback = auth_callback, .custom_init_callback = custom_init, .custom_cleanup_callback = custom_cleanup, .context = undefined, .timer = undefined, .config = undefined, .drbg = undefined, .entropy = undefined, .entropy_seed = 0x55555555, .ec = undefined, .cid = undefined };
        }
        /// Initialize the SSL context
        pub fn messup(self: *@This()) void {
            //self.entropy_seed = 0x55555555;

            c.miso_mbedtls_init_timer(&self.timer);
            c.mbedtls_ssl_init(&self.context);
            c.mbedtls_ssl_config_init(&self.config);
            c.mbedtls_ctr_drbg_init(&self.drbg);
            c.mbedtls_entropy_init(&self.entropy);

            if (mode == connection.security_mode.certificate_ec) {
                // Initialize the certificate chains
                c.mbedtls_x509_crt_init(&self.ec.own_crt);
                c.mbedtls_x509_crt_init(&self.ec.peer_crt);
                c.mbedtls_x509_crl_init(&self.ec.peer_crl);
                c.mbedtls_pk_init(&self.ec.own_pk);
            }
        }

        pub fn cleanup(self: *@This()) void {
            if (self.custom_cleanup_callback) |custom| {
                custom(self);
            } else {
                // Free Own PK
                if (mode == connection.security_mode.certificate_ec) {
                    c.mbedtls_pk_free(&self.own_pk);

                    // Free Certificate Chains
                    c.mbedtls_x509_crt_free(&self.ec.own_crt);
                    c.mbedtls_x509_crt_free(&self.ec.peer_crt);
                    c.mbedtls_x509_crl_free(&self.ec.peer_crl);
                }

                c.miso_mbedtls_deinit_timer(&self.timer);
                c.mbedtls_ctr_drbg_free(&self.drbg);
                c.mbedtls_entropy_free(&self.entropy);
                c.mbedtls_ssl_free(&self.context);
                c.mbedtls_ssl_config_free(&self.config);
            }
        }
        /// Open a connection to peer
        pub fn open(self: *@This(), uri: std.Uri, local_port: ?u16) !void {
            const proto = connection.schemes.match(uri.scheme).?.getProtocol();
            var ret: i32 = 0;

            try self.init(proto);
            errdefer {
                _ = self.deinit();
            }

            try self.conn.open(uri, local_port);
            errdefer {
                self.conn.close() catch {};
            }

            if (proto.isTls()) {
                //ret = c.mbedtls_ssl_set_hostname(&self.context, @ptrCast(uri.host.?));
            }

            ret = c.MBEDTLS_ERR_SSL_WANT_WRITE;
            while ((ret != 0) and ((ret == c.MBEDTLS_ERR_SSL_WANT_WRITE) or (ret == c.MBEDTLS_ERR_SSL_WANT_READ))) {
                ret = c.mbedtls_ssl_handshake(&self.context);
            }
        }
        /// Close connection to peer
        pub fn close(self: *@This()) !void {
            defer {
                _ = self.deinit();
            }
            try self.conn.close();
        }
        /// Send data to peer
        pub fn send(self: *@This(), buffer: []const u8) !usize {
            if (self.conn.getProto().isTls()) {
                return self.send_tls(buffer);
            } else {
                return self.send_dtls(buffer);
            }
        }
        /// Send data via DTLS
        fn send_dtls(self: *@This(), buffer: []const u8) !usize {
            var offset: usize = 0;
            var cid_enabled: c_int = c.MBEDTLS_SSL_CID_DISABLED;

            var ret: i32 = c.mbedtls_ssl_get_peer_cid(&self.context, &cid_enabled, null, 0);
            if (cid_enabled == c.MBEDTLS_SSL_CID_DISABLED) {
                //
            } else {
                ret = 0;
            }

            if (ret == 0) {
                while (offset != buffer.len) {
                    const slice = buffer[offset..];
                    const num_bytes = c.mbedtls_ssl_write(&self.context, @ptrCast(slice.ptr), @intCast(slice.len));
                    if ((c.MBEDTLS_ERR_SSL_WANT_READ == num_bytes) or (c.MBEDTLS_ERR_SSL_WANT_WRITE == num_bytes) or (c.MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS == num_bytes) or (c.MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS == num_bytes)) {
                        continue;
                    } else if (num_bytes < 0) {
                        ret = -1;
                        break;
                    } else {
                        offset += @as(usize, @intCast(num_bytes));
                    }
                }
            }

            return if (ret == 0) offset else connection.connection_error.send_error;
        }
        /// Read data via DTLS
        fn read_dtls(self: *@This(), buffer: []u8) ![]u8 {
            var numBytes: isize = c.MBEDTLS_ERR_SSL_WANT_READ;

            while ((numBytes == c.MBEDTLS_ERR_SSL_WANT_READ) or (numBytes == c.MBEDTLS_ERR_SSL_WANT_WRITE) or (numBytes == c.MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS) or (numBytes == c.MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS) or (numBytes == c.MBEDTLS_ERR_SSL_CLIENT_RECONNECT)) {
                numBytes = c.mbedtls_ssl_read(&self.context, @ptrCast(buffer.ptr), @intCast(buffer.len));
            }

            return if (numBytes < 0) connection.connection_error.recieve_error else buffer[0..@intCast(numBytes)];
        }
        /// Read data via TLS
        fn read_tls(self: *@This(), buffer: []u8) ![]u8 {
            var numBytes: i32 = c.MBEDTLS_ERR_SSL_WANT_READ;
            //var res: i32 = 0;

            while ((numBytes == c.MBEDTLS_ERR_SSL_WANT_READ) or (numBytes == c.MBEDTLS_ERR_SSL_WANT_WRITE) or (numBytes == c.MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS) or (numBytes == c.MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS) or (numBytes == c.MBEDTLS_ERR_SSL_CLIENT_RECONNECT)) {
                numBytes = c.mbedtls_ssl_read(&self.context, @ptrCast(buffer.ptr), @intCast(buffer.len));
            }

            return if (numBytes < 0) connection.connection_error.recieve_error else buffer[0..@intCast(numBytes)];
        }
        /// Send data via TLS
        fn send_tls(self: *@This(), buffer: []const u8) !usize {
            var offset: usize = 0;
            var ret: i32 = -1;
            while (offset < buffer.len) {
                const slice = buffer[offset..];
                const num_bytes = c.mbedtls_ssl_write(&self.context, @ptrCast(slice.ptr), @intCast(slice.len));
                if ((c.MBEDTLS_ERR_SSL_WANT_READ == num_bytes) or (c.MBEDTLS_ERR_SSL_WANT_WRITE == num_bytes) or (c.MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS == num_bytes) or (c.MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS == num_bytes)) {
                    continue;
                } else if (num_bytes >= 0) {
                    offset += @as(usize, @intCast(num_bytes));
                    ret = 0;
                } else {
                    ret = num_bytes;
                    break;
                }
            }
            return if (ret == 0) offset else connection.connection_error.send_error;
        }
        /// Recieve data from peer
        pub fn recieve(self: *@This(), buffer: []u8) ![]u8 {
            if (self.conn.getProto().isTls()) {
                return self.read_tls(buffer);
            } else {
                return self.read_dtls(buffer);
            }
        }
        /// Send data callback for MbedTLS
        fn send_c(ctx: ?*anyopaque, data: [*c]const u8, data_len: usize) callconv(.C) c_int {
            const self: *@This() = @as(*@This(), @ptrCast(@alignCast(ctx)));
            const len = self.conn.send_c(data[0..data_len]);
            return if (len == connection.EAGAIN) c.MBEDTLS_ERR_SSL_WANT_WRITE else len;
        }
        /// Recieve data callback for MbedTLS
        fn recv_c(ctx: ?*anyopaque, data: [*c]u8, data_len: usize) callconv(.C) c_int {
            const self: *@This() = @as(*@This(), @ptrCast(@alignCast(ctx)));
            const len = self.conn.recieve_c(data[0..data_len]);
            return if (len == connection.EAGAIN) c.MBEDTLS_ERR_SSL_WANT_READ else len;
        }

        pub fn waitRx(self: *@This(), timeout: u32) !bool {
            return self.conn.waitRx(timeout);
        }
        /// Initialize the MbedTLS context
        pub fn init(self: *@This(), protocol: connection.proto) !void {
            var ret: i32 = mbedtls_nok;

            if (!connection.proto.isSecure(protocol))
                // Running init on a non secure protocol
                return mbedtls_error.no_sec;

            // custom init callback
            if (self.custom_init_callback) |custom| {
                custom(self, mode);
            } else {
                self.messup();

                const transport: c_int = switch (protocol) {
                    .dtls_ip4, .dtls_ip6 => c.MBEDTLS_SSL_TRANSPORT_DATAGRAM,
                    else => c.MBEDTLS_SSL_TRANSPORT_STREAM,
                };

                ret = c.mbedtls_ssl_config_defaults(&self.config, c.MBEDTLS_SSL_IS_CLIENT, transport, c.MBEDTLS_SSL_PRESET_DEFAULT);
                if (ret == mbedtls_ok) {
                    ret = c.mbedtls_ssl_conf_max_frag_len(&self.config, c.MBEDTLS_SSL_MAX_FRAG_LEN_1024);
                }
                if (ret == mbedtls_ok) {
                    c.mbedtls_ssl_conf_authmode(&self.config, c.MBEDTLS_SSL_VERIFY_OPTIONAL); // None since using PSK
                    c.mbedtls_ssl_conf_read_timeout(&self.config, tls_read_timeout);
                    c.mbedtls_ssl_conf_rng(&self.config, c.mbedtls_ctr_drbg_random, &self.drbg);
                    //mbedtls_entropy_add_source(&entropy_context, mbedtls_entropy_f_source_ptr f_source, void *p_source, size_t threshold, MBEDTLS_ENTROPY_SOURCE_STRONG );
                    ret = c.mbedtls_ctr_drbg_seed(&self.drbg, c.mbedtls_entropy_func, &self.entropy, null, 0);
                    if (ret == c.MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED) ret = mbedtls_ok;
                }

                if (ret == mbedtls_ok) {
                    if (mode == connection.security_mode.certificate_ec) {
                        c.mbedtls_ssl_conf_groups(&self.config, &groups[0]);
                        c.mbedtls_ssl_conf_sig_algs(&self.config, &sig_algorithms[0]);
                    }
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
                }

                if (protocol.isDtls()) {
                    if (ret == mbedtls_ok) {
                        ret = c.mbedtls_ssl_conf_cid(&self.config, self.cid.len, c.MBEDTLS_SSL_UNEXPECTED_CID_FAIL);
                    }
                }

                // Run the auth credentials callback
                if (ret == mbedtls_ok) {
                    try self.auth_callback(self.parent, mode);
                }

                if (ret == mbedtls_ok) {
                    ret = c.mbedtls_ssl_setup(&self.context, &self.config);
                }

                if (protocol.isDtls()) {
                    if (ret == mbedtls_ok) {
                        @memset(&self.cid, 0);
                        ret = c.mbedtls_ctr_drbg_random(&self.drbg, &self.cid, self.cid.len);
                    }

                    if (ret == mbedtls_ok) {
                        ret = c.mbedtls_ssl_set_cid(&self.context, c.MBEDTLS_SSL_CID_ENABLED, &self.cid, self.cid.len);
                    }
                }

                if (ret == mbedtls_ok) {
                    c.mbedtls_ssl_set_timer_cb(&self.context, &self.timer, c.miso_mbedtls_timing_set_delay, c.miso_mbedtls_timing_get_delay);
                }

                if (ret == mbedtls_ok) {
                    c.mbedtls_ssl_set_bio(&self.context, @ptrCast(@alignCast(self)), send_c, recv_c, null);
                    c.mbedtls_ssl_set_mtu(&self.context, 1472);
                }
            } // End of standard init

            if (ret != mbedtls_ok) {
                return mbedtls_error.init_error;
            }
        }
        pub fn deinit(self: *@This()) i32 {
            var ret: i32 = 0;

            self.cleanup();

            return ret;
        }
        pub fn confPsk(self: *@This(), psk: []u8, psk_id: [*:0]u8) !void {
            return if (mbedtls_ok != c.mbedtls_ssl_conf_psk(&self.config, psk.ptr, psk.len, &psk_id[0], c.strlen(psk_id))) mbedtls_error.psk_conf_error else {};
        }
    };
}

pub fn base64Decode(input: [*:0]u8, output: []u8) ![]u8 {
    var len: usize = 0;

    if (mbedtls_ok == c.mbedtls_base64_decode(output.ptr, output.len, &len, &input[0], c.strlen(input))) {
        return if (output.len >= len) output[0..len] else mbedtls_error.generic_error;
    }

    return mbedtls_error.generic_error;
}
