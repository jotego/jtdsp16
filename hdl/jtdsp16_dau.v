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
    input             rst,
    input             clk,
    input             cen,
    input             dec_en,        // F1 decoder enable
    input             special,   // selects F2 output
    input      [ 2:0] r_field,
    input      [ 1:0] a_field,  // select acc output
    input      [ 4:0] t_field,
    input      [ 4:0] c_field,
    input      [ 5:0] op_fields,
    input             ram_load,
    input             rmux_load,
    input             imm_load,
    input             acc_load,
    input             acc_ram,
    input             fully_load,
    input             pt_load,
    // ALU control
    input             alu_sel,
    input             st_a0h,
    input             st_a1h,
    // Data buses
    input      [15:0] ram_dout,
    input      [15:0] rom_dout,
    input      [15:0] rmux,
    input      [15:0] long_imm,
    input      [15:0] cache_dout,
    input      [15:0] pt_dout,

    output     [15:0] acc_dout,
    output reg [15:0] reg_dout,
    output reg        con_result,
    // Debug
    output     [15:0] debug_x,
    output     [15:0] debug_y,
    output     [15:0] debug_yl,
    output     [ 7:0] debug_c0,
    output     [ 7:0] debug_c1,
    output     [ 7:0] debug_c2,
    output     [ 6:0] debug_auc,
    output     [35:0] debug_a0,
    output     [35:0] debug_a1,
    output     [15:0] debug_psw,
    output     [31:0] debug_p
);

reg  [15:0] x, yh, yl, z;
reg  [31:0] p;
reg  [35:0] a1, a0;
reg  [35:0] alu_out;
reg  [36:0] alu_arith, alu_special, p_ext;
wire [36:0] alu_in;
wire        st_a0l;
wire        st_a1l;
wire [35:0] alu_sat;

wire [ 3:0] f_field;
wire        s_field;  // source
wire        d_field;  // destination
wire        special_ok; // special operation F2 is ok to store as CON was true

// Control registers
reg  [ 7:0] c0, c1, c2;
reg  [ 6:0] auc;        // arithmetic unit control
reg         lmi, leq, llv, lmv, alu_llv;
reg         f1_nop;        // indicates that F1 should not alter the flags
wire [15:0] psw;        // processor status word
reg         ov1, ov0;   // overflow
reg         ah_only;    // used for F2 operation #9

// LFSR
reg  [31:0] lfsr;
reg         up_lfsr;

wire [31:0] y;
wire [36:0] as, y_ext;
wire [35:0] ram_ext, acc_mux;
wire [19:0] rmux_ext, acc_in;
wire [ 3:0] flags;
wire [15:0] load_data;
wire        pre_ov, pre_lmv;
wire        up_p;
wire        up_y;
wire        up_a0h, up_a1h;
wire        ad_sel;
wire        as_sel;
wire        clr_yl, clr_a1l, clr_a0l;
wire        sat_a1, sat_a0;
wire        load_y, load_yl;
wire        load_x, load_auc;
wire        load_c0, load_c1, load_c2;
wire        load_a0, load_a1;
wire        f1_st, f2_st;  // F1/2 store operation

assign flags       = { lmi, leq, llv, lmv };
assign y           = {yh, yl};
assign up_p        = dec_en && f_field[3:2]==2'b0 && !special;
assign up_y        = load_y | load_yl | fully_load;
assign st_a1l      = 0;
assign st_a0l      = 0;
assign as          = s_field ? {a1[35],a1} : {a0[35],a0};
assign y_ext       = { {5{y[31]}}, y };
assign psw         = { flags, 2'b0, ov1, ov0, a1[35:32], a0[35:32] };
assign clr_yl      = ~auc[6];
assign clr_a1l     = ~auc[5];
assign clr_a0l     = ~auc[4];
assign sat_a1      = ~auc[3];
assign sat_a0      = ~auc[2];
assign ram_ext     = { {4{ram_dout[15]}}, ram_dout, 16'd0 };
assign rmux_ext    = { {4{rmux[15]}}, rmux };
assign alu_in      = alu_sel ? { ram_ext[35], ram_ext} : p_ext;
assign acc_mux     = a_field[0] ? a1 : a0;
assign acc_dout    = a_field[1] ? acc_mux[31:16] : acc_mux[15:0]; // this is used in R=aT operations
assign acc_in      = acc_ram ? ram_ext[35:16] : (rmux_load ? rmux_ext : alu_out[35:16]);
assign pre_ov      = alu_llv ^ alu_out[35]; // number doesn't fit in 36-bit integer
assign pre_lmv     = |alu_out[35:31] ^ &alu_out[35:31]; // number doesn't fit in 32-bit integer
assign alu_sat     = { {5{alu_out[35]}}, {31{~alu_out[35]}}}; // saturate to 32-bit integer

assign f1_st       = dec_en && !special && (f_field!=4'd2 && f_field!=4'd6 && f_field!=4'd10 && f_field!=4'd11 );
assign special_ok  = special && con_result;
assign f2_st       = dec_en &&  special_ok; // reserved case 10 is not treated differently

assign load_x      = ((imm_load || ram_load || acc_load ) && r_field==3'd0) || pt_load;
assign load_y      = (imm_load || ram_load || acc_load ) && r_field==3'd1;
assign load_yl     = (imm_load || ram_load || acc_load ) && r_field==3'd2;
assign load_auc    = (imm_load || ram_load || acc_load ) && r_field==3'd3;
assign load_c0     = (imm_load || ram_load || acc_load ) && r_field==3'd5;
assign load_c1     = (imm_load || ram_load || acc_load ) && r_field==3'd6;
assign load_c2     = (imm_load || ram_load || acc_load ) && r_field==3'd7;
assign load_a0     = (f1_st || f2_st) && !d_field;
assign load_a1     = (f1_st || f2_st) &&  d_field;
assign load_data   = acc_load ? acc_dout : (imm_load ? long_imm : ram_dout);

assign { d_field, s_field, f_field } = op_fields;


// Debug
assign debug_x   = x;
assign debug_y   = yh;
assign debug_yl  = yl;
assign debug_c0  = c0;
assign debug_c1  = c1;
assign debug_c2  = c2;
assign debug_a0  = a0;
assign debug_a1  = a1;
assign debug_psw = psw;
assign debug_auc = auc;
assign debug_p   = p;

// Condition check
always @(*) begin
    up_lfsr = 0;
    case(c_field[4:1])
        4'd0: con_result =  lmi;
        4'd1: con_result =  leq;
        4'd2: con_result =  llv;
        4'd3: con_result =  lmv;
        4'd4: begin
            con_result = lfsr[31];
            up_lfsr    = 1;
        end
        4'd5: con_result = ~c0[7]; // >=0
        4'd6: con_result = ~c1[7]; // >=0
        4'd7: con_result = 1;
        4'd8: con_result = ~lmi & ~leq;
        default: con_result = 0; // should be 0?
    endcase
    if( c_field[0] ) con_result = ~con_result;
end

function [36:0] round;
    input [36:0] a;
    /* verilator lint_off WIDTH */
    round = { a[36:16] + a[15] , 16'd0 };
    /* verilator lint_on WIDTH */
endfunction

always @(posedge clk, posedge rst) begin
    if( rst ) begin
        lfsr <= 32'hcafe_cafe;
    end else if( cen ) begin
        if( up_lfsr && special)
            lfsr <= { lfsr[30:0], lfsr[31]^lfsr[21]^lfsr[1]^lfsr[0] };
    end
end

always @(posedge clk, posedge rst) begin
    if( rst ) begin
        p   <= 32'd0;
        x   <= 16'd0;
        yh  <= 16'd0;
        yl  <= 16'd0;
        z   <= 16'd0;
        a0  <= 36'd0;
        a1  <= 36'd0;
        auc <=  7'd0;
        c0  <=  8'd0;
        c1  <=  8'd0;
        c2  <=  8'd0;
        ov1 <=  0;
        ov0 <=  0;
        { lmi, leq, llv, lmv } <= 4'd0;
    end else if(cen) begin
        if( up_p   ) p <= x*yh;
        if( load_x ) x <= pt_load ? pt_dout : load_data;
        if( up_y ) begin
            if( fully_load ) begin
                {yh, yl} <= acc_mux[31:0];
            end else begin
                if( !load_yl ) begin
                    yh <= /*load_ay1 ? a1[31:16] : (load_ay0 ? a0[15:0] :*/
                                                 load_data;
                    if( clr_yl ) yl <= 16'd0;
                end else begin
                    yl <= load_data;
                end
            end
        end
        // z <= zyl_swap ? yl : yh; // keep a copy of y last value
        // a0
        if( st_a0h ) begin
            a0[35:16] <= acc_in;
            if( clr_a0l ) a0[15:0] <= 16'd0;
        end else if( load_a0 ) begin
            a0 <= (pre_lmv & sat_a0) ? alu_sat : alu_out;
            if( special && ah_only ) a0[15:0] <= 16'd0;
        end
        // a1
        if( st_a1h ) begin
            a1[35:16] <= acc_in;
            if( clr_a1l ) a1[15:0] <= 16'd0;
        end else if( load_a1 ) begin
            a1 <= (pre_lmv & sat_a1) ? alu_sat : alu_out;
            if( special && ah_only ) a1[15:0] <= 16'd0;
        end
        // Counters
        if( load_c0 ) c0 <= load_data[7:0];
        if( load_c1 ) c1 <= load_data[7:0];
        if( load_c2 ) c2 <= load_data[7:0];
        // special registers
        if( load_auc ) auc <= load_data[6:0];
        // Flags
        if(dec_en && ( (!special && !f1_nop) || f2_st)) begin
            lmi <= alu_out[35];
            leq <=  (special&&ah_only) ? ~|alu_out[35:16] // assume AL is all zeros
                                       : ~|alu_out;
            llv <= pre_ov;
            lmv <= pre_lmv;
            // Not sure whether these are always updated
            ov0 <= ~d_field & pre_ov;
            ov1 <=  d_field & pre_ov;
        end
    end
end

// F1 operations
always @(*) begin
    case( f_field )
        4'd0, 4'd4: alu_arith = p_ext;
        4'd1, 4'd5: alu_arith = as+p_ext;
        4'd3, 4'd7: alu_arith = as-p_ext;
        4'd8:       alu_arith = as | y_ext;
        4'd9:       alu_arith = as ^ y_ext;
        4'd10:      alu_arith = as & y_ext;
        4'd11,4'd15:alu_arith = as - y_ext;
        4'd12:      alu_arith = y_ext;
        4'd13:      alu_arith = as + y_ext;
        4'd14:      alu_arith = as & y_ext;
        default: alu_arith = 37'd0;
    endcase
    f1_nop = f_field==4'd2 || f_field==4'd6; // do not modify flags
end

// F2 operations
always @(*) begin
    ah_only = 0;
    case( f_field )
        4'd0:  alu_special = { as[36], as[36:1] };
        4'd1:  alu_special = { {5{as[30]}}, as[30:0], 1'd0 }; // shift << by 1
        4'd2:  alu_special = { {4{as[36]}}, as[36:4] };
        4'd3:  alu_special = { {5{as[27]}}, as[27:0], 4'd0 }; // shift by 4
        4'd4:  alu_special = { {8{as[36]}}, as[36:8] };
        4'd5:  alu_special = { {5{as[23]}}, as[23:0], 8'd0 }; // shift by 8
        4'd6:  alu_special = { {16{as[36]}}, as[36:16] }; // as >>> 16
        4'd7:  alu_special = { {5{as[15]}}, as[15:0], 16'd0 }; // shift by 16
        4'd8:  alu_special = p_ext;
        4'd9:  begin
            alu_special = as + 37'h1_0000;
            ah_only = 1;
        end
        4'd11: alu_special = round(as);
        4'd12: alu_special = y_ext;
        4'd13: alu_special = as + 37'd1;
        4'd14: alu_special = as;
        4'd15: alu_special = -as;
        default: alu_special = 37'd0; // case 10 is reserved
    endcase
end

always @(*) begin
    case( auc[1:0] )
        2'd0, 2'd3: p_ext = { {5{p[31]}}, p }; // Makes reserved case 3 same as 0
        2'd1: p_ext = { {7{p[31]}}, p[31:2] };
        2'd2: p_ext = { {3{p[31]}}, p, 2'd0 };
    endcase
end

always @(*) begin
    {alu_llv, alu_out} = special ? alu_special : alu_arith;
end

always @(*) begin
    case( r_field )
        3'd0: reg_dout = x;
        3'd1: reg_dout = y[31:16];
        3'd2: reg_dout = yl;
        3'd3: reg_dout = { 9'd0, auc };
        3'd4: reg_dout = psw;
        3'd5: reg_dout = {{8{c0[7]}}, c0};
        3'd6: reg_dout = {{8{c1[7]}}, c1};
        3'd7: reg_dout = {{8{c2[7]}}, c2};
    endcase
end

endmodule
