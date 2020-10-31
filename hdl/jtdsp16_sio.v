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
    input             cen,
    // DSP16 pins
    output reg        ock,  // serial output clock
    output            sio_do,   // serial data output
    output            sadd,
    output reg        old,  // output load
    output            ose,  // output shift register empty
    input             doen, // enable data output (ignored)
    // interface with CPU - only writes command are implemented
    input      [15:0] long_imm,
    input             sio_imm_load,
    input      [ 2:0] r_field,
    // status
    output            obe,      // output buffer empty
    output            ibf       // input buffer full - unused
);

reg  [15:0] ibuf, obuf;
reg  [16:0] ocnt;
reg  [ 9:0] sioc;
reg  [15:0] srta, addr_obuf;
reg         ifsr, ofsr;
reg         last_ock;
wire        sdx_load, srta_load, sioc_load;

reg  [ 3:0] clkdiv;
wire        posedge_ock;

assign sio_do      = obuf[15];
assign posedge_ock = ock && !last_ock;
assign obe         = ocnt[16];
assign sadd        = addr_obuf[7] && !obe;
assign sdx_load    = sio_imm_load && r_field==3'b010;
assign srta_load   = sio_imm_load && r_field==3'b001;
assign sioc_load   = sio_imm_load && r_field==3'b000;

// serial input related registers. Not supported
assign sio_dout = 16'd0;
assign ibf      = 0;
// Other unsupported signals

always @(posedge clk, posedge rst) begin
    if( rst ) begin
        clkdiv    <= 4'd0;
        ocnt      <= ~17'h0;
        old       <= 1;
        last_ock  <= 0;
        ock       <= 0;
        addr_obuf <= ~8'h0;
        srta      <= 16'h0;
        obuf      <= 16'h0;
    end else if(cen) begin
        clkdiv   <= clkdiv==4'd11 ? 4'd0: clkdiv+4'd1;
        last_ock <= ock;
        if( clkdiv==4'd5  ) ock <= ~obe;
        if( clkdiv==4'd11 ) ock <= 0;
        if( sio_imm_load ) begin
            if( sdx_load ) begin
                obuf      <= long_imm;
                addr_obuf <= srta[7:0];
                ocnt      <= 16'h1;
            end
            if( sioc_load ) sioc <= long_imm[9:0]; // contents ignored as config is fixed
            if( srta_load ) srta <= long_imm;
        end else begin
            if( posedge_ock && !obe ) begin
                old  <= 0;
                if( !old ) begin
                    obuf <= obuf<<1;
                    ocnt <= ocnt<<1;
                    addr_obuf <= { addr_obuf[6:0], 1'b1 };
                end
            end else if( obe ) begin
                old <= 1;
            end
        end
    end
end

endmodule
