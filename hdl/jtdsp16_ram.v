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

// RAM. Not clocked

module jtdsp16_ram(
    input      [10:0] addr,
    input      [15:0] din,
    output reg [15:0] dout,
    input             we
);

reg [15:0] ram[0:2047];

always @(*) begin
    if(we) ram[ addr ] = din;
end

always @(*) begin
    dout = ram[ addr ];
end

endmodule