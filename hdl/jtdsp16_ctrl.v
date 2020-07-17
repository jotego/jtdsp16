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
    output     [ 4:0] t_field,
    output     [ 3:0] f1_field,
    output     [ 3:0] f2_field,
    output            d_field,  // destination
    output            s_field,  // source
    output     [ 4:0] c_field,  // condition
    output     [ 2:0] r_field,
    output     [ 1:0] y_field,

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
    // instruction fields
    output reg [11:0] ifield,
    output reg        con_result,
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
    input      [15:0] ext_dout,
);

reg       x_field;
reg       double;
reg [3:0] y_field;

assign    long_imm = rom_dout;

// Decode instruction
always @(posedge clk, posedge rst) begin
    if(rst) begin
        short_load <= 0;
        long_load  <= 0;
        ram_load   <= 0;
        double     <= 0;
        post_load  <= 0;
    end else begin
        t_field   <= rom_dout[15:11];
        d_field   <= rom_dout[   10];
        s_field   <= rom_dout[    9];
        f1_field  <= rom_dout[ 8: 5];
        x_field   <= rom_dout[    4];
        y_field   <= rom_dout[ 3: 0];
        short_imm <= rom_dout[ 8: 0];

        // disable all control signals
        short_load <= 0;
        long_load  <= 0;
        ram_load   <= 0;
        double     <= 0;
        post_load  <= 0;
        // XAAU
        if(!double) begin
            casez( rom_dout[15:11] ) // T
                5'b0001?: begin // short imm j, k, rb, re
                    short_load <= 1;
                    r_field    <= { ~rom_dout[6], rom_dout[5:4] };
                end
                5'b01010: begin // long imm
                    long_load <= rom_dout[ 9:7]==3'b0; // YAAU register as destination
                    r_field   <= rom_dout[11:9];
                    double    <= 1;
                end
                5'b01111: begin
                    ram_load  <= rom_dout[ 9:7]==3'b0; // YAAU register as destination
                    r_field   <= rom_dout[11:9];
                    y_field   <= rom_dout[ 3:2];
                    post_load <= 1;
                    case( rom_dout[1:0] ) begin
                        default: begin
                            inc_sel <= 2'd1;
                            step_sel<= 0;
                            ksel    <= 0;
                        end
                        2'd2: begin
                            inc_sel  <= 2'd0;
                            step_sel <= 0;
                        end
                        2'd3: begin
                            step_sel <= 1;
                            ksel     <= 0;
                        end
                    end
                    double   <= 1;
            endcase
        end
    end
end

endmodule