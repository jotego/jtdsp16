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

// WE-DSP16A had an internal divider by 2
// This is also used to make RAM synchronous and ease synthesis

module jtdsp16_div(
    input      clk,
    input      cen,
    input      ext_rq,
    input      ext_ok,
    output reg cendiv
);

reg toggle;

`ifdef SIMULATION
initial begin
    cendiv = 0;
    toggle = 0;
end
`endif

always @(posedge clk) begin
    cendiv <= 0;
    if( ext_rq && !ext_ok ) begin
        cendiv <= 0;
    end else if( cen ) begin
        toggle <= ~toggle;
        cendiv <= toggle;
    end
end

endmodule
