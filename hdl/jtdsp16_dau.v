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

module jtdsp16_dau(
    input         rst,
    input         clk,
    input         cen,
    input  [ 4:0] t_field,
    input  [ 3:0] f1_field,
    input  [ 3:0] f2_field,
    input         s_field,  // source
    input         d_field,  // destination
    input  [ 4:0] c_field,  // condition
    // X load control
    input         up_xram,
    input         up_xrom,
    input         up_xext,
    input         up_xcache,
    // Data buses
    input  [15:0] ram_dout,
    input  [15:0] rom_dout,
    input  [15:0] cache_dout,
    input  [15:0] ext_dout,
    output [15:0] dau_dout
);

reg  [15:0] x, yh, yl;
reg  [31:0] p;
reg  [35:0] a1, a0;
reg  [35:0] alu_out, alu_arith, alu_special;
reg  [35:0] p_ext;
wire [35:0] alu_in;

// Control registers
reg  [ 7:0] c0, c1, c2;
reg  [ 6:0] auc;
reg         lmi, leq, llv, lmv;
wire [15:0] psw;
reg         ov1, ov0;   // overflow

wire [31:0] y;
wire [35:0] as;
wire [35:0] y_ext;
wire [35:0] ram_ext;
wire        up_p;
wire        up_y;
wire        ad_sel;
wire        as_sel;
wire        store;
wire        sel_special;
wire        clr_yl, clr_a1l, clr_a0l;
wire        sat_a1, sat_a0;
reg         load_ay1, load_ay0;
reg         load_y, load_yl;

// Conditions
wire        pl;     // nonnegative
wire        mi;     // negative
wire        peq;    // equal to zero
wire        ne;     // not equal to zero
wire        gt;     // greater than zero
wire        le;     // less than zero
wire        lvs;    // logical overflow set
wire        lvc;    // logical overflow clear
wire        mvs;    // mathematical overflow set
wire        mvc;    // mathematical overflow clear
wire        c0ge;   // counter0 >=0 (and counter gets incremented)
wire        c0lt;   // counter0 <0  (and counter gets incremented)
wire        c1ge;   // counter1 >=0 (and counter gets incremented)
wire        c1lt;   // counter1 <0  (and counter gets incremented)
wire        heads;  // pseudorandom sequence bit set
wire        tails;  // pseudorandom sequence bit clear

assign y           = {yh, yl};
assign up_p        = f1_field[3:2]==2'b0;
assign up_y        = load_ay0 | load_ay1 | load_y | load_yl;
assign st_0        = !d_field && store;
assign st_1        =  d_field && store;
assign store       = f1_field != 4'b10 && f1_field != 4'b110 && f1_field[3:1] != 3'b101;
assign as          = s_field ? a1 : a0;
assign y_ext       = { {4{y[31]}}, y };
assign sel_special = t_field == 5'h12 || t_field == 5'h13;
assign psw         = { flags, 2'b0, ov1, ov0, a1[35:32], a0[35:32] };
assign clr_yl      = aux[6];
assign clr_a1l     = aux[5];
assign clr_a0l     = aux[4];
assign sat_a1      = aux[3];
assign sat_a0      = aux[2];
assign ram_ext     = { {4{ram_dout[15]}}, ram_dout, 16'd0 };
assign alu_in      = alu_sel ? ram_ext : p_ext;

function [35:0] round;
    input [35:0] a;
    round = { a[35:16] + a[15] , 16'd0 };
endfunction

always @(posedge clk, posedge rst) begin
    if( rst ) begin
        p  <= 32'd0;
        x  <= 16'd0;
        yh <= 16'd0;
        yl <= 16'd0;
    end else if(cen) begin
        if( up_p ) p  <= x*yh;
        x <= up_xram   ? ram_dout   : (
             up_xrom   ? rom_dout   : (
             up_xcache ? cache_dout : (
             up_xext   ? ext_dout   : x )));
        if( up_y ) begin
            if( !load_yl ) begin
                yh <= load_ay1 ? a1[31:16] : (load_ay0 ? a1[15:0] : ram_dout);
                if( clr_yl ) yl <= 16'd0;
            end else begin
                yl <= ram_dout[7:0];
            end
        end
        if( st_a0h ) a0[35:16] <= alu_out[35:16];
        if( st_a1h ) a1[35:16] <= alu_out[35:16];
        a0[15: 0] <= st_a0h ? (clr_a0l ? 16'd0 : alu_out[15:0]) :
                    (st_a0l ? alu_out[15:0] : a0[15:0]);
        a1[15: 0] <= st_a1h ? (clr_a1l ? 16'd0 : alu_out[15:0]) :
                    (st_a1l ? alu_out[15:0] : a1[15:0]);
        // Flags
        lmi <= alu_out[35];
        leq <= ~|alu_out;
        // llv <= // ??
        lmv <= ^alu_out[35:31];
    end
end

always @(*) begin
    case( f1_field )
        4'd0, 4'd4: alu_arith = p_ext;
        4'd1, 4'd5: alu_arith = as+p_ext;
        4'd3, 4'd7, 4'd11: alu_arith = as-p_ext;
        4'd8:       alu_arith = as | p_ext;
        4'd9:       alu_arith = as ^ p_ext;
        4'd10:      alu_arith = as & p_ext;
        4'd12:      alu_arith = y_ext;
        4'd13:      alu_arith = as + y_ext;
        4'd14:      alu_arith = as & y_ext;
        4'd15:      alu_arith = as - y_ext;
        default: alu_arith = 36'd0;
    endcase
end

/////// F2 field
always @(*) begin
    case( f2_field )
        4'd0: alu_special = as >>> 1;
        4'd1: alu_special = { as[30], as[30:0], 1'd0 }; // shift by 1
        4'd2: alu_special = as >>> 4;
        4'd3: alu_special = { {4{as[27]}}, as[27:0], 4'd0 }; // shift by 4
        4'd4: alu_special = as >>> 8;
        4'd5: alu_special = { {8{as[23]}}, as[23:0], 8'd0 }; // shift by 8
        4'd6: alu_special = as >>> 16;
        4'd7: alu_special = { {16{as[15]}}, as[15:0], 16'd0 }; // shift by 16
        4'd8: alu_special = p_ext;
        4'd9: alu_special = as + 36'h10000;
        4'd11: alu_special = round(as);
        4'd12: alu_special = y_ext;
        4'd13: alu_special = as + 1'd1;
        4'd14: alu_special = as;
        4'd15: alu_special = -as;
        default: alu_special = 36'd0;
    endcase
end

always @(*) begin
    case( auc[1:0] )
        2'd0: p_ext = { 4{p[31]}, p };
        2'd1, 2'd3: p_ext = { 6{p[31]}, p[31:2] }; // Makes reserved case 3 same as 1
        2'd2: p_ext = { 2{p[31]}, p, 2'd0 };        
    endcase
end

always @(*) begin
    alu_out = sel_special ? alu_special : alu_arith;
end

endmodule