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
    input             ph1,
    input      [15:0] pbus_in,
    output reg [15:0] pbus_out,
    output            pods_n,        // parallel output data strobe
    output            pids_n,        // parallel input  data strobe
    output reg        psel,          // peripheral select
                                     // Unused by QSound firmware:
    input             irq,           // external interrupt request
    // interface with CPU
    input             pdx_read,
    input             pio_imm_load,
    input             pio_ram_load,
    input             pio_acc_load,
    input      [ 2:0] r_field,
    output     [15:0] pio_dout,
    // Buses
    input      [15:0] long_imm,
    input      [15:0] ram_dout,
    input      [15:0] acc_dout,
    // Interrupts
    input             siord_full,
    input             siowr_empty,
    input             iack,
    output reg        irq_latch
);

reg  [14:5] pioc; // bit 15 is a copy of bit 4
reg  [15:0] pdx_buffer;
reg  [ 3:0] pocnt, picnt;
reg  [15:0] pdx0_rd, pdx1_rd;
wire [ 4:0] status;

wire [ 1:0] stlen   = pioc[14:13];
// wire        po_mode = pioc[12];
// wire        pi_mode = pioc[11];
// wire        scmode  = pioc[10]; // when high, pbus_out[15:8] should be ignored
// wire [ 4:0] ien     = pioc[ 9: 5];
wire [ 3:0] ststart = 4'he << stlen;

wire        pioc_load, pdx0_load, pdx1_load, pdx_load, pdx_access;
wire        any_load;
wire [15:0] load_data;

// interrupt latches
reg         last_irq, last_siowr_empty, last_siord_full, last_iack;
wire        iack_negedge, siowr_empty_posedge, siord_full_posedge, irq_posedge;

assign pods_n = pocnt[0];
assign pids_n = picnt[0];

assign load_data  = pio_imm_load ? long_imm : ( pio_ram_load ? ram_dout : acc_dout);
assign any_load   = pio_imm_load | pio_ram_load | pio_acc_load;
assign pioc_load  = any_load && r_field[1:0]==2'd0;
assign pdx0_load  = any_load && r_field[1:0]==2'd1;
assign pdx1_load  = any_load && r_field[1:0]==2'd2;
assign pdx_load   = pdx0_load | pdx1_load;
assign pdx_access = (any_load | pdx_read) && r_field[1:0]!=2'd0;

// interrupt signals
assign iack_negedge        = ~iack       &  last_iack;
assign siord_full_posedge  = siord_full  & ~last_siord_full;
assign siowr_empty_posedge = siowr_empty & ~last_siowr_empty;
assign irq_posedge         = irq & pioc[5] & ~last_irq;

assign pio_dout = r_field[1:0]==2'd0 ? {status[4], pioc[14:5],status} : (
                  r_field[1] ? pdx1_rd : pdx0_rd );
assign status   = {siowr_empty, siord_full, 2'd0, irq&pioc[5]};

// interrupt control
always @(posedge clk, posedge rst) begin
    if( rst ) begin
        last_irq         <= 0;
        last_siord_full  <= 0;
        last_siowr_empty <= 0;
        irq_latch        <= 0;
        last_iack        <= 0;
        irq_latch        <= 0;
    end else if(ph1) begin
        last_iack        <= iack;
        last_irq         <= ~iack_negedge & (irq&pioc[5]);
        last_siowr_empty <= siowr_empty;
        last_siord_full  <= siord_full;
        if( irq_posedge |
           (siowr_empty_posedge & pioc[9]) |
           (siord_full_posedge  & pioc[8]) ) // passive mode interrupts are not supported
            irq_latch <= 1;
        else if( iack_negedge )
            irq_latch <= 0;
    end
end

// parallel port control
always @(posedge clk, posedge rst) begin
    if( rst ) begin
        pocnt      <= 4'hf;
        picnt      <= 4'hf;
        psel       <= 0;
        pdx0_rd    <= 16'd0;
        pdx1_rd    <= 16'd0;
        pdx_buffer <= 16'd0;
        pioc       <= { 2'd0, 2'b11, 1'b0, 5'd0 };
        pbus_out   <= 16'd0;
    end else if(ph1) begin
        pocnt <= pdx_load ? ststart : {1'b1,pocnt[3:1]};
        picnt <= pdx_read ? ststart : {1'b1,picnt[3:1]};

        // Data is read in after the stablished period
        if( !picnt[0] && picnt[1] ) begin
            //pdx_buffer <= pbus_in;
            if( psel )
                pdx1_rd <= pbus_in;
            else
                pdx0_rd <= pbus_in;
        end

        if( pdx_access ) begin
            psel <= r_field[1];
            if( pdx0_load || pdx1_load ) begin
                pbus_out <= load_data;
            end
            // if(r_field[0] && pdx_read ) pdx0_rd <= pdx_buffer;
            // if(r_field[1] && pdx_read ) pdx1_rd <= pdx_buffer;
        end

        if( pioc_load ) pioc[14:5] <= long_imm[14:5];
    end
end

endmodule