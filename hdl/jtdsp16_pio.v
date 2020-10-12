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

// Only output mode is supported, i.e. pods_n and pids_n are both output signals
// In active mode the data strobe signals are output from the chips, forcing a write or a read
// to/from the external device

module jtdsp16_pio(
    input             rst,
    input             clk,
    input      [15:0] pbus_in,
    output reg [15:0] pbus_out,
    output            pods_n,        // parallel output data strobe
    output            pids_n,        // parallel input  data strobe
    output reg        psel,          // peripheral select
                                     // Unused by QSound firmware:
    input             irq,           // external interrupt request
    // interface with CPU
    input      [15:0] cpu_dout,
    output     [15:0] pio_dout,
    input             pio_we,
    input             pio_rd,
    input      [ 1:0] cpu_addr,
    // Interrupts
    input             serrd_full,
    input             serwr_empty,
    output            ext_irq
);

reg  [14:5] pioc; // bit 15 is a copy of bit 4
reg  [ 3:0] pocnt, picnt;
reg  [15:0] pdx0_rd, pdx1_rd;
wire [ 4:0] status;

wire [ 1:0] stlen   = pioc[14:13];
wire        po_mode = pioc[12];
wire        pi_mode = pioc[11];
wire        scmode  = pioc[10]; // when high, pbus_out[15:8] should be ignored
wire [ 4:0] ien     = pioc[ 9: 5];
wire [ 3:0] ststart = 4'he << stlen;

assign pods_n = pocnt[0];
assign pids_n = picnt[0];

assign ext_irq   = (irq & pioc[5]) |
                   (serwr_empty & pioc[9]) |
                   (serrd_full  & pioc[8]); // passive mode interrupts are not supported

assign pio_dout = cpu_addr==2'd0 ? {status[4], pioc[14:5],status} : (
                  cpu_addr[0] ? pdx1_rd : pdx0_rd );
assign status   = {serwr_empty, serrd_full, 2'd0, irq&pioc[5]};

always @(posedge clk, posedge rst) begin
    if( rst ) begin
        pocnt   <= 4'hf;
        picnt   <= 4'hf;
        psel    <= 0;
        pdx0_rd <= 16'd0;
        pdx1_rd <= 16'd0;
    end else begin
        pocnt <= pio_we ? ststart : {1'b1,pocnt[3:1]};
        picnt <= pio_rd ? ststart : {1'b1,picnt[3:1]};
        if( (pio_we || pio_rd) && cpu_addr!=2'd0 ) begin
            psel <= cpu_addr[0];
            if( pio_rd ) begin
                if( cpu_addr[0] )
                    pdx1_rd <= pbus_in;
                else
                    pdx0_rd <= pbus_in;
            end else begin
                pbus_out <= cpu_dout;
            end
        end
        if( pio_we && cpu_addr==2'd0 ) pioc[14:5] <= cpu_dout[14:5];
    end
end

endmodule