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
    input             pc_halt,
    input             ram_load,
    input             imm_load,
    input             acc_load,
    input             pt_load,
    // *pt++[i] reads
    input             pt_read,
    input             istep,
    output     [11:0] pt_addr,
    // do loop
    input             do_start,
    input      [10:0] do_data,
    output reg        do_flush,
    output reg        do_en,
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
    input      [15:0] acc_dout,
    // ROM request
    output reg [15:0] reg_dout,
    output     [15:0] rom_addr,
    // Registers - for debugging only
    output     [15:0] debug_pc,
    output     [15:0] debug_pr,
    output     [15:0] debug_pi,
    output     [15:0] debug_pt,
    output     [11:0] debug_i
);

reg  [11:0] i;
reg  [15:0] pc,     // Program Counter
            pr,     // Program Return
            pi,     // Program Interrupt
            pt,     // Table Pointer
            rnext;
reg         shadow;     // normal execution or inside IRQ

// Do loops
reg  [15:0] do_head, redo_out;
wire [15:0] do_addr;
reg  [ 4:0] do_pc, next_do_pc;
reg  [ 3:0] do_ni;
reg         redo_en, last_do_en, redo_aux, do_record;
reg  [ 6:0] do_left;

wire [15:0] sequ_pc;
reg  [15:0] next_pc, next_pt;
wire [15:0] i_ext;
wire [ 2:0] b_field;
wire        copy_pc;
wire        load_pt, load_pi, load_pr ,load_i;
wire        any_load;

wire        ret, iret, goto_pt, call_pt;
wire        enter_int, dis_shadow;

wire        do_endhit, do_prehit, redo, do_loop;

assign      sequ_pc  = pc+1'd1;
assign      i_ext    = { {4{i[11]}}, i };
assign      b_field  = i_field[10:8];

assign      ret      = goto_b && b_field==3'b00;
assign      iret     = goto_b && b_field==3'b01;
assign      goto_pt  = goto_b && b_field==3'b10;
assign      call_pt  = goto_b && b_field==3'b11;
assign      copy_pc  = call_pt || call_ja;
assign      any_load = ram_load || imm_load || acc_load;
assign      load_pt  = (any_load && r_field==3'd0) || pt_load;
assign      load_pr  = (any_load && r_field==3'd1) || copy_pc;
assign      load_pi  =  any_load && r_field==3'd2;
assign      load_i   =  any_load && r_field==3'd3;

assign      rom_addr = do_en ? do_addr : pc;

assign      do_endhit= do_pc == {1'b0, do_ni};
assign      do_prehit= do_pc == {1'b0, do_ni} && do_left==7'd1;
assign      enter_int = ext_irq && shadow && !pc_halt && !no_int && !do_en;
assign      dis_shadow= enter_int || icall || redo || do_start;
assign      pt_addr  = pt[11:0];

// Do loop
assign      do_addr  = do_head + { 12'd0, do_pc[3:0] };
assign      do_loop  = do_endhit && do_left>7'd1;
assign      redo     = do_start && do_data[10:7]==4'd0;
//assign      do_flush = do_prehit;


// Debugging
assign      debug_pc = pc;
assign      debug_pr = pr;
assign      debug_pi = pi;
assign      debug_pt = pt;
assign      debug_i  = i;

always @(*) begin
    rnext =
        imm_load ? rom_dout : (
        ram_load ? ram_dout : (
        acc_load ? acc_dout : pc ));
    next_pt = pt + (istep ? i_ext : 16'd1);
end

always @(*) begin
    case( r_field[1:0] )
        2'd0: reg_dout = pt;
        2'd1: reg_dout = pr;
        2'd2: reg_dout = pi;
        2'd3: reg_dout = { {4{i[11]}}, i };
    endcase

    if( do_en ) begin
        next_pc = pc;
    end else begin
        next_pc =
            enter_int ? 16'd1 : (
            icall     ? 16'd2 : (
            (goto_ja || call_ja) ? { pc[15:12], i_field } : (
            (goto_pt || call_pt) ? pt : (
            ret                  ? pr : (
            iret                 ? pi : (
            pc_halt              ? pc : sequ_pc ))))));
    end

    next_do_pc = pc_halt ? do_pc : ( do_pc[3:0]==do_ni ? 5'd0 : (do_pc + 5'd1));
end

always @(posedge clk, posedge rst ) begin
    if( rst ) begin
        pc      <= 16'd0;
        pr      <= 16'd0;
        pi      <= 16'd0;
        pt      <= 16'd0;
        i       <= 12'd0;
        shadow  <= 1;
        iack    <= 1;
        // Do registers
        do_en   <= 0;
        do_ni   <= 4'd0;
        redo_en <= 0;
        redo_out<= 16'd0;
        do_left <= 7'd0;
        last_do_en <= 0;
        do_record  <= 0;
        do_head    <= 16'd0;
        do_pc      <= 5'd0;
    end else if(cen) begin
        last_do_en <= do_en;
        do_flush <= do_en & do_prehit;
        if( load_pt  ) pt <= pt_load ? next_pt : rnext;
        if( load_pr  ) pr <= rnext;
        if( load_i   ) i  <= rnext[11:0];

        // Interrupt processing
        if( dis_shadow ) begin
            shadow <= 0;
        end else if( iret || (last_do_en && !do_en) ) shadow <= 1;
        iack <= enter_int;

        // Update PC
        pc    <= next_pc;
        do_pc <= next_do_pc;
        if( load_pi )
            pi <= rnext;
        else if( shadow && !do_start)
            pi <= sequ_pc;

        if( do_start ) begin
            if(do_data[10:7]!=4'd0) begin
                do_head  <= pc;
                do_ni    <= do_data[10:7]-4'd1;
                redo_aux <= 0;
            end else begin
                redo_out <= pc;
                pc       <= do_head;
                redo_aux <= 1;
            end
            do_pc    <= do_data[10:7] == 4'd0 ? 5'd0 : 5'd1;
            do_left  <= do_data[6:0];// - 7'd1;// (do_data[10:7] == 4'd0 ? 7'd1 : 7'd0);
            do_record<= 1;
            // do_en    <= 1;
        end else begin
            if( do_endhit ) begin
                if( do_record ) begin
                    do_record <= 0;
                    do_en <= 1; // execution from cache will start in next iteration
                end
                if( do_en && do_left > 7'd0 ) begin
                    do_left <= do_left-7'd1;
                end
            end
            if( do_prehit ) begin
                do_en    <= 0;
            end
            // redo_aux <= 0;
        end
    end
end

endmodule