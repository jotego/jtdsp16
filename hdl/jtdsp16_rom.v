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
    Date: 15-7-2020 */

// ROM. Not clocked

module jtdsp16_rom(
    input             clk,
    input      [15:0] addr,
    output     [15:0] dout,     // first 4kB of memory comes from internal ROM
                                // the rest is read from the external memory
    // External ROM
    input             ext_mode,
    input      [15:0] ext_data,
    output     [15:0] ext_addr,
    // ROM programming interface
    input      [12:0] prog_addr,
    input      [ 7:0] prog_data,
    input             prog_we
);

reg [ 7:0] rom_lsb[0:4095];
reg [15:8] rom_msb[0:4095];
reg [15:0] rom_dout;

assign     ext_addr = addr;
assign     dout     = ext_mode ? ext_data : ((addr[15:12]==4'd0) ? rom_dout : ext_data);

always @(posedge clk) begin
    if(prog_we && !prog_addr[0]) rom_lsb[ prog_addr[12:1] ] <= prog_data;
    if(prog_we &&  prog_addr[0]) rom_msb[ prog_addr[12:1] ] <= prog_data;
    rom_dout <= { rom_msb[ addr[11:0] ], rom_lsb[ addr[11:0] ] };
end


endmodule