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

// ROM Address Arithmetic Unit
// This is caled XAAU in the block diagram

module jtdsp16_rom_aau(
    input             rst,
    input             clk,
    input             cen,
    // instruction types
    input             goto_ja,
    input             goto_b,
    input             call_ja,
    input             icall,
    input             post_inc,
    input             pc_halt,
    input             ram_load,
    input             imm_load,
    // do loop
    input             do_start,
    input      [10:0] do_data,
    // instruction fields
    input      [ 2:0] r_field,
    input      [11:0] i_field,
    // IRQ
    input             ext_irq,
    input             no_int,
    output reg        iack,
    // Data buses
    input      [15:0] rom_dout,
    input      [15:0] ram_dout,
    // ROM request
    output reg [15:0] reg_dout,
    output     [15:0] rom_addr
);

reg  [11:0] i;
reg  [15:0] pc,     // Program Counter
            pr,     // Program Return
            pi,     // Program Interrupt
            pt,     // Table Pointer
            rnext,
            do_head, redo_out, do_end;
reg         shadow;     // normal execution or inside IRQ
reg         do_en, redo_en;
reg  [ 6:0] do_left;

wire [15:0] next_pc;
wire [15:0] i_ext;
wire [ 2:0] b_field;
wire        copy_pc;
wire        load_pt, load_pi, load_pr ,load_i;
wire        any_load;

wire        ret, iret, goto_pt, call_pt;
wire        do_endhit, redo;
wire        enter_int;

assign      next_pc  = pc+1'd1;
assign      i_ext    = { {4{i[11]}}, i };
assign      b_field  = i_field[10:8];

assign      ret      = goto_b && b_field==3'b00;
assign      iret     = goto_b && b_field==3'b01;
assign      goto_pt  = goto_b && b_field==3'b10;
assign      call_pt  = goto_b && b_field==3'b11;
assign      copy_pc  = call_pt || call_ja;
assign      any_load = ram_load || imm_load;
assign      load_pt  =  any_load && r_field==3'd0;
assign      load_pr  = (any_load && r_field==3'd1) || copy_pc;
assign      load_pi  =  any_load && r_field==3'd2;
assign      load_i   =  any_load && r_field==3'd3;

assign      rom_addr = pc;
assign      do_endhit= do_en && next_pc==do_end;
assign      redo     = do_start && do_data[10:7]==4'd0;
assign      enter_int = ext_irq && shadow && !pc_halt && !no_int;

always @(*) begin
    rnext =
        imm_load ? rom_dout : (
        ram_load ? ram_dout : (
        copy_pc  ? pc       : (
                   pt+i_ext    )));
end

always @(*) begin
    case( r_field[1:0] )
        2'd0: reg_dout = pt;
        2'd1: reg_dout = pr;
        2'd2: reg_dout = pi;
        2'd3: reg_dout = i;
    endcase
end

always @(posedge clk, posedge rst ) begin
    if( rst ) begin
        pc      <= 16'd0;
        pr      <= 16'd0;
        pi      <= 16'd0;
        pt      <= 16'd0;
        i       <= 12'd0;
        do_en   <= 0;
        redo_en <= 0;
        redo_out<= 16'd0;
        shadow  <= 1;
        iack    <= 1;
    end else if(cen) begin
        if( load_pt  ) pt <= rnext;
        if( shadow  || load_pi ) pi <= load_pi ? rnext : next_pc;
        if( load_pr ) pr <= rnext;
        if( load_i  ) i  <= rnext[11:0];

        // Interrupt processing
        if( enter_int || icall ) begin
            shadow <= 0;
        end else if( iret ) shadow <= 1;
        iack <= enter_int;

        pc <=
            enter_int ? 16'd1 : (
            icall     ? 16'd2 : (
            (do_endhit || redo ) ? do_head : (
            (redo_en && next_pc==do_end ) ? redo_out :(
            (goto_ja || call_ja) ? { pc[15:12], i_field } : (
            (goto_pt || call_pt) ? pt : (
            ret                  ? pr : (
            iret                 ? pi : (
            pc_halt              ? pc : next_pc ))))))));
        if( do_start ) begin
            if(do_data[10:7]!=4'd0) begin
                do_head  <= pc;
                do_end   <= pc + {12'd0,do_data[10:7]};
                redo_en  <= 0;
            end else begin
                redo_out <= pc;
                redo_en  <= 1;
            end
            do_left  <= do_data[6:0];
            do_en    <= 1;
        end
        if( do_endhit ) begin
            do_left <= do_left-7'd1;
            do_en   <= do_left>7'd2;
            redo_en <= do_left>7'd2;
        end
    end
end

endmodule