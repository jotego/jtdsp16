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
    input         rst,
    input         clk,
    input         cen,
    // Instruction fields
    output [ 4:0] t_field,
    output [ 3:0] f1_field,
    output [ 3:0] f2_field,
    output        d_field,  // destination
    output        s_field,  // source
    output [ 4:0] c_field,  // condition
    // X load control
    output        up_xram,
    output        up_xrom,
    output        up_xext,
    output        up_xcache,
    // Data buses
    input  [15:0] rom_dout,
    output [15:0] cache_dout,
    input  [15:0] ext_dout,
);

reg       x_field;
reg [3:0] y_field;

// Decode instruction
always @(posedge clk, posedge rst) begin
    if(rst) begin
    end else begin
        t_field <= rom_dout[15:11];
        d_field <= rom_dout[10];
        s_field <= rom_dout[9];
        f1_field<= rom_dout[8:5];
        x_field <= rom_dout[4];
        y_field <= rom_dout[3:0];
    end
end

endmodule