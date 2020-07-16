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

module jtdsp16(
    input             rst,
    input             clk,
    input             cen,

    output     [15:0] ext_addr,
    input      [15:0] ext_data,
    // ROM programming interface
    input      [11:0] prog_addr,
    input      [15:0] prog_data,
    input             prog_we
);

wire [15:0] rom_data, ram_data, cache_data;
wire [15:0] rom_addr;

// X-AAU
wire        goto_ja;
wire        goto_b;
wire        call_ja;
wire        icall;
wire [11:0] ifield;
wire        con_result;

// Y-AAU
wire [ 8:0] short_imm;
wire [15:0] long_imm;
wire [15:0] acc;
wire [ 2:0] reg_sel_field;
wire        imm_type; // 0 for short, 1 for long
wire        imm_en;
wire        acc_en;

// DAU
wire [ 4:0] t_field;
wire [ 3:0] f1_field;
wire [ 3:0] f2_field;
wire        s_field;  // source
wire        d_field;  // destination
wire [ 4:0] c_field;  // condition

wire [15:0] cache_dout;
wire [15:0] dau_dout;

// X load control
wire        up_xram;
wire        up_xrom;
wire        up_xext;
wire        up_xcache;

// ROM
wire [11:0] rom_addr;
wire [15:0] ram_dout;

// RAM
wire [10:0] ram_addr;
wire [15:0] ram_din;
wire [15:0] ram_dout;
wire        ram_we;

assign      ext_addr = rom_addr;


jtdsp16_ctrl u_ctrl(
    .rst            ( rst           ),
    .clk            ( clk           ),
    .cen            ( cen           ),
    // X load control
    .up_xram        ( up_xram       ),
    .up_xrom        ( up_xrom       ),
    .up_xext        ( up_xext       ),
    .up_xcache      ( up_xcache     ),
    // Data buses
    .rom_dout       ( rom_dout      ),
    .cache_dout     ( cache_dout    ),
    .ext_dout       ( ext_dout      ),
);

jtdsp16_rom u_rom(
    .addr       ( rom_addr  ),
    .dout       ( rom_dout  ),
    // ROM programming interface
    .prog_addr  ( prog_addr ),
    .prog_data  ( prog_data ),
    .prog_we    ( prog_we   )
);

jtdsp16_ram u_ram(
    .addr       ( ram_addr  ),
    .din        ( ram_din   ),
    .dout       ( ram_dout  ),
    .we         ( ram_we    )
);

// ROM address arithmetic unit - XAAU
jtdsp16_rom_aau u_rom_aau(
    .rst        ( rst       ),
    .clk        ( clk       ),
    .cen        ( cen       ),
    // instruction types
    .goto_ja    ( goto_ja   ),
    .goto_b     ( goto_b    ),
    .call_ja    ( call_ja   ),
    // instruction fields
    .icall      ( icall     ),
    .ifield     ( ifield    ),
    .con_result ( con_result),
    // ROM request
    .rom_req    ( rom_addr  )
);

jtdsp16_ram_aau u_ram_aau(
    .rst            ( rst           ),
    .clk            ( clk           ),
    .cen            ( cen           ),
    .short_imm      ( short_imm     ),
    .long_imm       ( long_imm      ),
    .acc            ( acc           ),
    .reg_sel_field  ( reg_sel_field ),
    .imm_type       ( imm_type      ), // 0 for short, 1 for long
    .imm_en         ( imm_en        ),
    .acc_en         ( acc_en        ),
    // X load control
    .up_xram        ( up_xram       ),
    .up_xrom        ( up_xrom       ),
    .up_xext        ( up_xext       ),
    .up_xcache      ( up_xcache     ),
    // Data buses
    .ram_dout       ( ram_dout      ),
    .rom_dout       ( rom_dout      ),
    .cache_dout     ( cache_dout    ),
    .ext_dout       ( ext_dout      ),
    .dau_dout       ( dau_dout      )
);

jtdsp16_dau u_dau(
    .rst        ( rst       ),
    .clk        ( clk       ),
    .cen        ( cen       ),
    .t_field    ( t_field   ),
    .f1_field   ( f1_field  ),
    .f2_field   ( f2_field  ),
    .s_field    ( s_field   ),  // source
    .d_field    ( d_field   ),  // destination
    .c_field    ( c_field   ),  // condition
    .dinX       ( dinX      ),
    .dinY       ( dinY      ),
    .dout       ( dau_dout  )
);

endmodule