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

module jtdsp16_rom_aau(
    input         rst,
    input         clk,
    input         cen,
);

reg  [11:0] i;
reg  [15:0] pc,     // Program Counter
            pr,     // Program Return
            pi,     // Program Interrupt
            pt;     // Table Pointer

wire [15:0] next_pc;
wire [15:0] i_ext;

assign      next_pc = pc+1'd1;
assign      i_ext   = { {4{i[11]}}, i };

always @(posedge clk, posedge rst ) begin
    if( rst ) begin
        pc <= 16'd0; //?
        pr <= 16'd0;
        pi <= 16'd0;
        pt <= 16'd0;
    end else begin
        if( shadow ) pi <= next_pc;
        if( gosub  ) pr <= next_pc;
        if( posti  ) pt <= pt + i_ext;
        pc <= 
            ext_irq ? 16'd0 : (
            int_irq ? 16'd1 : (
            gosub ? din : (
            ret   ? pr  : (
            reti  ? pi  : next_pc ))));
    end
end

endmodule