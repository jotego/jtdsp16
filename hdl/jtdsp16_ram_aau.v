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

// RAM Address Arithmetic Unit
// This is caled YAAU in the block diagram

module jtdsp16_ram_aau(
    input           rst,
    input           clk,
    input           cen,
    input    [ 8:0] short_imm,
    input    [15:0] long_imm,
    input    [15:0] acc,
    input    [ 2:0] reg_sel_field,
    input           imm_type, // 0 for short, 1 for long
    input           imm_en,
    input           acc_en,
);

reg  [15:0] re, // end   - virtual shift register
            rb, // begin - virtual shift register
            j,
            k,
            r0, r1, r2, r3,
            post;
reg         post_sel;

wire        vse_en;
wire [ 2:0] reg_sel;
wire        short_sign;
wire [15:0] long_in;
wire        load_type;

assign      vse_en     = |re;   // virtual shift register enable
assign      reg_sel    = {imm_type,2'b0} ^ reg_sel_field;
assign      short_sign = short_imm[8];
assign      long_in    = imm_en ? long_imm : acc;
assign      load_type  = acc_en || (imm_en&&imm_type);

always @(*) begin
    load_j  = (imm_en || acc_en) && reg_sel==3'd0;
    load_k  = (imm_en || acc_en) && reg_sel==3'd1;
    load_rb = (imm_en || acc_en) && reg_sel==3'd2;
    load_re = (imm_en || acc_en) && reg_sel==3'd3;
    load_r0 = (imm_en || acc_en) && reg_sel==3'd4;
    load_r1 = (imm_en || acc_en) && reg_sel==3'd5;
    load_r2 = (imm_en || acc_en) && reg_sel==3'd6;
    load_r3 = (imm_en || acc_en) && reg_sel==3'd7;
end

function [15:0] load_reg;
    input    [15:0] old;
    input           load;
    input           imm_type;
    input           short;
    input           long;
    input           sign;
    load_reg = !load ? old : (
        imm_type ? long : { {7{sign}}, short});
endfunction

always @(posedge clk, posedge rst ) begin
    if( rst ) begin
        re <= 16'd0;
        rb <= 16'd0;
        j  <= 16'd0;
        k  <= 16'd0;
        r0 <= 16'd0;
        r1 <= 16'd0;
        r2 <= 16'd0;
        r3 <= 16'd0;
    end else begin
        j  <= load_reg(  j, load_j,  load_type, short_imm, long_in, short_sign );
        k  <= load_reg(  k, load_k,  load_type, short_imm, long_in, short_sign );
        rb <= load_reg( rb, load_rb, load_type, short_imm, long_in, 1'b0 );
        re <= load_reg( re, load_re, load_type, short_imm, long_in, 1'b0 );
        r0 <= load_reg( r0, load_r0, load_type, short_imm, long_in, 1'b0 );
        r1 <= load_reg( r1, load_r1, load_type, short_imm, long_in, 1'b0 );
        r2 <= load_reg( r2, load_r2, load_type, short_imm, long_in, 1'b0 );
        r3 <= load_reg( r3, load_r3, load_type, short_imm, long_in, 1'b0 );
    end
end

endmodule