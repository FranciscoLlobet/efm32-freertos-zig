// Copyright (c) 2023-2024 Francisco Llobet-Blandino and the "Miso Project".
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the “Software”), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

/// This is a simple implementation of an XMODEM receiver in Zig.
const std = @cImport("std");

/// Control characters used in the XMODEM protocol
const ctrl_char = enum(u8) {
    /// Start of heading
    SOH = 0x01,

    /// Start of text
    STX = 0x02,

    /// End of text
    EOT = 0x04,

    /// Acknowledge
    ACK = 0x06,

    /// Negative acknowledge
    NAK = 0x15,

    /// Cancel
    CAN = 0x18,

    /// Checksum
    C = 0x43,
};

// Blocks are 132 bytes long, with the following structure:
// SOH (1Byte)
// Block number (1Byte)
// Block Number complement (1Byte)
// 128 bytes of data
// Checksum (1Byte) Sum of all data bytes

// Responses: ACK, NAK, CAN. 10s

const xmodem_package = packed struct {
    /// Start of heading
    soh: u8,
    block_number: u8,
    block_number_complement: u8,
    // Data block
    data: [128]u8,

    checksum: u8,
    checksum_2: u8,

    /// Returns the block number if the header is valid, otherwise null
    pub fn validate_header(self: *@This()) ?u8 {
        // Check if start of header
        if (self.soh != @intFromEnum(ctrl_char.SOH)) {
            return null;
        } else if (self.block_number != (0xFF - (self.block_number_complement & 0xFF))) {
            // block number is valid
            return null;
        }

        return self.block_number;
    }

    /// Returns true if the checksum is valid
    pub fn validate_checksum(self: *@This()) bool {
        var sum: u8 = 0;
        for (self.data) |byte| {
            sum += byte;
        }

        return (sum == self.checksum);
    }

    pub fn validate_crc(self: *@This()) bool {
        // Calulate CRC
        _ = self;
        return true;
    }
};

const xmodem_state_machine = struct {
    current_block: u8 = 0,

    pub fn calculate_next_block(self: *@This()) u8 {
        // Usually a simple increment, but I want to avoid issues with undefined behaviour due to overflow
        if (self.current_block == 0xFF) {
            return 0;
        } else {
            return self.current_block + 1;
        }
    }

    /// Validate incoming package
    /// Returns a slice to the data if the package is valid, otherwise null
    pub fn validate_package(self: *@This(), package: *xmodem_package) ?[]u8 {
        const block_number: ?u8 = package.validate_header();

        if (block_number) |bn| {
            if (false == package.validate_checksum()) {
                return null; // invalid checksum
            }

            if (bn == self.current_block) {
                // return package.data;
                // Retransmission of package
            } else if (bn == self.calculate_next_block()) {
                // return package.data;
                // Current package
            } else {
                // Out of order package
                return null; // Return NACK
            }
        }

        return package.data[0..];
    }
};
