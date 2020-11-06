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

module jtdsp16_rsel(
    input   [15:0] r_xaau,
    input   [15:0] r_yaau,
    input   [15:0] r_dau,
    input   [15:0] r_pio,
    input   [15:0] r_sio,
    input   [15:0] r_if,
    input   [15:0] r_acc,
    input   [ 2:0] rsel,
    input          acc_sel,
    output  [15:0] rmux
);

assign rmux = acc_sel             ? r_acc  : (
              rsel[2:1]== 2'b00   ? r_yaau : (
              rsel[2:1]== 2'b01   ? r_xaau : (
              rsel[2:1]== 2'b10   ? r_dau  : (
              rsel     == 3'b11_0 ? r_sio  : (
              rsel     == 3'b11_1 ? r_pio  : (
                                    r_if       ))))));

endmodule