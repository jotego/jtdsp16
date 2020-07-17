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
    input    [ 2:0] r_field,
    input    [ 1:0] y_field,
    // Increment selecction
    input    [ 1:0] inc_sel,
    input           ksel,
    input           step_sel,
    // Load control
    input           short_load,
    input           long_load,
    input           acc_load,
    input           ram_load,
    input           post_load,
    // register load inputs
    input    [ 8:0] short_imm,
    input    [15:0] long_imm,
    input    [15:0] acc,
    input    [15:0] ram_dout,
    // outputs
    output   [10:0] ram_addr,
    output   [15:0] reg_dout
);

reg  [15:0] re, // end   - virtual shift register
            rb, // begin - virtual shift register
            j,
            k,
            r0, r1, r2, r3, rin, rsum, rnext,
            jk_mux, unit_mux, step_mux, rmux,
            post;
reg         post_sel;
reg         load_j,
            load_k,
            load_rb,
            load_re,
            load_r0,
            load_r1,
            load_r2,
            load_r3;

wire        vsr_en;
wire        vsr_loop;
wire        short_sign;
wire [15:0] imm_ext;
wire        imm_load;

assign      vsr_en     = |re;   // virtual shift register enable
assign      vsr_loop   = rin==re && vsr_en;
assign      short_sign = short_imm[8];
assign      imm_ext    = long_load ? long_imm : { {7{short_imm[8]}}, short_imm };
assign      imm_load   = short_load || long_load;
assign      reg_dout   = rin;
assign      ram_addr   = rmux[10:0];

always @(*) begin
    load_j  = (imm_load || acc_load ) && r_field==3'd4;
    load_k  = (imm_load || acc_load ) && r_field==3'd5;
    load_rb = (imm_load || acc_load ) && r_field==3'd6;
    load_re = (imm_load || acc_load ) && r_field==3'd7;
    load_r0 = (imm_load || acc_load || ram_load || post_load ) && r_field==3'd0;
    load_r1 = (imm_load || acc_load || ram_load || post_load ) && r_field==3'd1;
    load_r2 = (imm_load || acc_load || ram_load || post_load ) && r_field==3'd2;
    load_r3 = (imm_load || acc_load || ram_load || post_load ) && r_field==3'd3;
end

function [15:0] load_reg;
    input           load_acc;
    input           load_short;
    input           load_long;
    input    [15:0] acc;
    input    [15:0] long;
    input           sign;
    input    [ 8:0] short;
    load_reg = load_acc   ? acc  : (
               load_long  ? long : { {7{sign}}, short});
endfunction        

always @(*) begin
    case( r_field[1:0] )
        2'd0: rin = r0;
        2'd1: rin = r1;
        2'd2: rin = r2;
        2'd3: rin = r3;
    endcase
    case( y_field )
        2'd0: rmux = r0;
        2'd1: rmux = r1;
        2'd2: rmux = r2;
        2'd3: rmux = r3;
    endcase
end

always @(*) begin
    // Increment can be -1, 0, 1, 2, j or k
    jk_mux = ksel ? k : j;
    case( inc_sel )
        2'd0: unit_mux = -16'd1;
        2'd1: unit_mux =  16'd0;
        2'd2: unit_mux =  16'd1;
        2'd3: unit_mux =  16'd2;
    endcase
    step_mux = step_sel ? jk_mux : unit_mux;
    rsum     = rin + step_mux;
    rnext    = imm_load ? imm_ext  : (
               acc_load ? acc      : (
               ram_load ? ram_dout : (
               vsr_loop ? rb       : rsum )));
end

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
    end else if(cen) begin
        if( load_j  ) j  <= load_reg( acc_load, short_load, long_load, acc, long_imm, short_sign, short_imm );
        if( load_k  ) k  <= load_reg( acc_load, short_load, long_load, acc, long_imm, short_sign, short_imm );
        if( load_rb ) rb <= load_reg( acc_load, short_load, long_load, acc, long_imm,       1'b0, short_imm );
        if( load_re ) re <= load_reg( acc_load, short_load, long_load, acc, long_imm,       1'b0, short_imm );
        if( load_r0 ) r0 <= rnext;
        if( load_r1 ) r1 <= rnext;
        if( load_r2 ) r2 <= rnext;
        if( load_r3 ) r3 <= rnext;
    end
end

endmodule