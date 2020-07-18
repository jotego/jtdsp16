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
    Date: 12-7-2020 */

module jtdsp16_ctrl(
    input             rst,
    input             clk,
    input             cen,
    // Instruction fields
    output reg [ 4:0] t_field,
    output reg [ 3:0] f1_field,
    output     [ 3:0] f2_field,
    output reg        d_field,  // destination
    output reg        s_field,  // source
    output     [ 4:0] c_field,  // condition
    output reg [ 2:0] r_field,
    output reg [ 1:0] y_field,

    // YAAU control
    // Increment selecction
    output reg [ 1:0] inc_sel,
    output reg        ksel,
    output reg        step_sel,
    // Load control
    output reg        short_load,
    output reg        long_load,
    output reg        acc_load,
    output reg        ram_load,
    output reg        post_load,
    output reg        ram_we,
    // register load inputs
    output reg [ 8:0] short_imm,
    output     [15:0] long_imm,

    // XAAU control
    // instruction types
    output reg        goto_ja,
    output reg        goto_b,
    output reg        call_ja,
    output reg        icall,
    output reg        post_inc,
    output reg        pc_halt,
    // instruction fields
    output reg [11:0] i_field,
    // IRQ
    output reg        ext_irq,
    output reg        shadow,     // normal execution or inside IRQ

    // X load control
    output            up_xram,
    output            up_xrom,
    output            up_xext,
    output            up_xcache,
    // Data buses
    input      [15:0] rom_dout,
    output     [15:0] cache_dout,
    input      [15:0] ext_dout
);

reg       x_field;
reg       double;

assign    long_imm = rom_dout;

// Decode instruction
always @(posedge clk, posedge rst) begin
    if(rst) begin
        short_load <= 0;
        long_load  <= 0;
        ram_load   <= 0;
        double     <= 0;
        post_load  <= 0;
        acc_load   <= 0;
        // ROM AAU
        goto_ja    <= 0;
        goto_b     <= 0;
        call_ja    <= 0;
        icall      <= 0;
        post_inc   <= 0;
        ext_irq    <= 0;
        shadow     <= 1;
        ram_we     <= 0;
        pc_halt    <= 0;
        // *r++ control lines:
        y_field    <= 2'b0;
        step_sel   <= 0;
        ksel       <= 0;
        inc_sel    <= 2'b0;
    end else if(cen) begin
        t_field   <= rom_dout[15:11];
        d_field   <= rom_dout[   10];
        s_field   <= rom_dout[    9];
        f1_field  <= rom_dout[ 8: 5];
        i_field   <= rom_dout[10: 0];
        x_field   <= rom_dout[    4];
        short_imm <= rom_dout[ 8: 0];

        // disable all control signals
        short_load <= 0;
        long_load  <= 0;
        ram_load   <= 0;
        ram_we     <= 0;
        double     <= 0;
        post_load  <= 0;
        pc_halt    <= 0;
        // XAAU
        if(!double) begin
            casez( rom_dout[15:11] ) // T
                5'b0001?: begin // short imm j, k, rb, re
                    short_load <= 1;
                    r_field    <= rom_dout[11:9]^3'b100;
                end
                5'b01010: begin // long imm
                    long_load <= rom_dout[ 9:7]==3'b0; // YAAU register as destination
                    r_field   <= rom_dout[ 9:4];
                    double    <= 1;
                end
                5'b01111, // RAM load to r0-r3
                5'b01100  // r0-r3 storage to RAM
                : begin 
                    ram_load  <= 
                        rom_dout[15:11] == 5'b01111 &&
                        rom_dout[ 9:7]==3'b0; // YAAU register as destination
                    if( rom_dout[15:11] == 5'b01100 ) begin
                        ram_we  <= 1;
                        pc_halt <= 1;
                    end else begin
                        ram_we  <= 0;
                        pc_halt <= 0;
                    end
                    r_field   <= rom_dout[ 6:4];
                    y_field   <= rom_dout[ 3:2];
                    post_load <= 1;
                    case( rom_dout[1:0] )
                        2'd0: begin // *rN
                            inc_sel <= 2'd1;
                            step_sel<= 0;
                        end
                        2'd1: begin // *rN++
                            inc_sel <= 2'd2;
                            step_sel<= 0;
                        end
                        2'd2: begin // *rN--
                            inc_sel  <= 2'd0;
                            step_sel <= 0;
                        end
                        2'd3: begin // *rN++j
                            step_sel <= 1;
                            ksel     <= 0;
                        end
                    endcase
                    double   <= 1;
                end
            endcase
        end
    end
end

endmodule