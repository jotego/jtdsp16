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

module jtdsp16_ram_dau(
    input         rst,
    input         clk,
    input         cen,
);

reg  [15:0] re, // end   - virtual shift register
            rb, // begin - virtual shift register
            j,
            k,
            r0, r1, r2, r3,
            post;
reg         post_sel;

wire        vse_en = |re;   // virtual shift register enable

always @(*) begin

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
    end else begin
    end
end

endmodule