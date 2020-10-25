/*  This file is part of JTDSP16.
    JTDSP16 program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    JTDSP16 program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with JTDSP16.  If not, see <http://www.gnu.org/licenses/>.

    Author: Jose Tejada Gomez. Twitter: @topapate
    Version: 1.0
    Date: 12-10-2020 */

// Only serial output is supported. Serial input is not used by Q-Sound and thus ignored here
// Only supports the SIOC configuration used by Q-Sound
// SIOC (Serial I/O Control Register): always set to 0x02E8
//   bit   9  (0x0200) = 1: active ILD/OLD = OutCLK / 16, active SYNC = OutCLK/128 or OCK/256
//   bits 8-7 (0x0180) = 1: active clock = CKI / 12
//   bit   6  (0x0040) = 1; MSB first
//   bit   5  (0x0020) = 1: OutLoad is output
//   bit   4  (0x0010) = 0: InLoad is input
//   bit   3  (0x0008) = 1: OutCLK is output
//   bit   2  (0x0004) = 0: InCLK is input
//   bit   1  (0x0002) = 0: 16-bit output
//   bit   0  (0x0001) = 0: 16-bit input

module jtdsp16_sio(
    input             rst,
    input             clk,
    // DSP16 pins
    output reg        ock,  // serial output clock
    output            sio_do,   // serial data output
    output            sadd,
    output            old,  // output load
    output            ose,  // output shift register empty
    // interface with CPU
    input      [15:0] cpu_dout,
    output     [15:0] sio_dout,
    input             sio_we,
    input             sio_rd,
    input      [ 1:0] cpu_addr,
    // status
    output reg        obe,
    output            ibf,      // input buffer full - unused
    // Interrupts
    input             siord_full,
    input             siowr_empty,
    output            ext_irq
);

reg  [15:0] ibuf, obuf, ocnt;
reg  [ 9:0] sioc;
reg  [15:0] srta;
reg         ifsr, ofsr;
reg         obe;                // output buffer empty

reg  [ 3:0] clkdiv;

assign sio_do = obuf[15];
assign old    = clkdiv==4'd11;;

// serial input related registers. Not supported
assign sio_dout = 16'd0;
assign ibf      = 0;
// Other unsupported signals

always @(posedge clk, posedge rst) begin
    if( rst ) begin
        obe    <= 1;
        clkdiv <= 4'd0;
        ocnt   <= 16'hfffe;
    end else begin
        clkdiv <= clkdiv==4'd11 ? 4'd0: clkdiv+4'd1;
        ock    <= oevent;
        if( oevent ) begin
            obuf <= obuf<<1;
            ocnt <= ocnt<<1;
        end
    end
end

endmodule
