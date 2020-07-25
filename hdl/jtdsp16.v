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
    input             ext_mode,     // EXM pin, when high internal ROM is disabled
    // ROM programming interface
    input      [11:0] prog_addr,
    input      [15:0] prog_data,
    input             prog_we
);

wire        cen2;   // cen divided by 2

wire [15:0] rom_data, ram_data, cache_data;
wire [15:0] rom_addr;

wire [ 2:0] r_field;
wire [ 1:0] inc_sel;

// X-AAU
wire        goto_ja;
wire        goto_b;
wire        call_ja;
wire        icall;
wire        post_inc;
wire [11:0] i_field;
wire        con_result;
wire        ext_irq;
wire        shadow;
wire        xaau_ram_load, xaau_imm_load;

// Y-AAU
wire [ 8:0] short_imm;
wire [15:0] long_imm;
wire [15:0] acc_dout;
wire [ 2:0] reg_sel_field;
wire        imm_type; // 0 for short, 1 for long
wire        imm_en;
wire        acc_en;
wire        pc_halt;

// DAU
wire [ 4:0] t_field;
wire [ 5:0] dau_op_fields;
wire [ 1:0] y_field;
wire        dau_acc_load, dau_imm_load, dau_ram_load;
wire        st_a0h, st_a1h;
wire        dau_dec_en, dau_con_en;

wire [15:0] cache_dout;
wire [15:0] dau_dout;

// X load control
wire        up_xram;
wire        up_xrom;
wire        up_xext;
wire        up_xcache;

// RAM
wire [10:0] ram_addr;
wire [15:0] ram_dout, rom_dout;
wire        ram_we;

// Register mux
wire [15:0] r_xaau, r_yaau, r_dau, rmux;
wire [ 2:0] rsel;

jtdsp16_div u_div(
    .clk            ( clk           ),
    .cen            ( cen           ),
    .cen2           ( cen2          )
);

jtdsp16_rsel u_rsel(
    .r_xaau  ( r_xaau    ),
    .r_yaau  ( r_yaau    ),
    .r_dau   ( r_dau     ),
    .r_if    ( 16'd0     ),
    .rsel    ( rsel      ),
    .rmux    ( rmux      )
);

jtdsp16_ctrl u_ctrl(
    .rst            ( rst           ),
    .clk            ( clk           ),
    .cen            ( cen2          ),
    // ROM AAU - XAAU
    .goto_ja        ( goto_ja       ),
    .goto_b         ( goto_b        ),
    .call_ja        ( call_ja       ),
    .icall          ( icall         ),
    .post_inc       ( post_inc      ),
    .i_field        ( i_field       ),
    .shadow         ( shadow        ),
    .pc_halt        ( pc_halt       ),
    .xaau_ram_load  ( xaau_ram_load ),
    .xaau_imm_load  ( xaau_imm_load ),
    // DAU
    .dau_dec_en     ( dau_dec_en    ),
    .dau_con_en     ( dau_con_en    ),
    .dau_op_fields  ( dau_op_fields ),
    .at_sel         ( at_sel        ),
    .dau_rmux_load  ( dau_rmux_load ),
    .dau_imm_load   ( dau_imm_load  ),
    .dau_ram_load   ( dau_ram_load  ),
    .st_a0h         ( st_a0h        ),
    .st_a1h         ( st_a1h        ),
    // X load control
    .up_xram        ( up_xram       ),
    .up_xrom        ( up_xrom       ),
    .up_xext        ( up_xext       ),
    .up_xcache      ( up_xcache     ),
    // Y load control
    .r_field        ( r_field       ),
    .rsel           ( rsel          ),
    .y_field        ( y_field       ),
    .ram_we         ( ram_we        ),
    // Increment selecction
    .inc_sel        ( inc_sel       ),
    .ksel           ( ksel          ),
    .step_sel       ( step_sel      ),
    // Load control
    .short_load     ( short_load    ),
    .long_load      ( long_load     ),
    .acc_load       ( acc_load      ),
    .ram_load       ( ram_load      ),
    .post_load      ( post_load     ),
    // register load inputs
    .short_imm      ( short_imm     ),
    .long_imm       ( long_imm      ),
    // Data buses
    .rom_dout       ( rom_dout      ),
    .cache_dout     ( cache_dout    ),
    .ext_dout       ( ext_data      )
);

jtdsp16_rom u_rom(
    .clk        ( clk             ),
    .addr       ( rom_addr        ),
    .dout       ( rom_dout        ),
    .ext_mode   ( ext_mode        ),
    .ext_data   ( ext_data        ),
    .ext_addr   ( ext_addr        ),
    // ROM programming interface
    .prog_addr  ( prog_addr       ),
    .prog_data  ( prog_data       ),
    .prog_we    ( prog_we         )
);

// ROM address arithmetic unit - XAAU
jtdsp16_rom_aau u_rom_aau(
    .rst        ( rst           ),
    .clk        ( clk           ),
    .cen        ( cen2          ),
    // instruction types
    .goto_ja    ( goto_ja       ),
    .goto_b     ( goto_b        ),
    .call_ja    ( call_ja       ),
    .icall      ( icall         ),
    .post_inc   ( post_inc      ),
    .pc_halt    ( pc_halt       ),
    .ram_load   ( xaau_ram_load ),
    .imm_load   ( xaau_imm_load ),
    // instruction fields
    .r_field    ( r_field       ),
    .i_field    ( i_field       ),
    .con_result ( con_result    ),
    // Interruption
    .ext_irq    ( 1'b0          ),
    .shadow     ( shadow        ),
    // Data buses
    .rom_dout   ( rom_dout      ),
    .ram_dout   ( ram_dout      ),
    // ROM request
    .rom_addr   ( rom_addr      ),
    .reg_dout   ( r_xaau        )
);

jtdsp16_ram u_ram(
    .clk        ( clk           ),
    .addr       ( ram_addr      ),
    .din        ( rmux          ),
    .dout       ( ram_dout      ),
    .we         ( ram_we        )
);

jtdsp16_ram_aau u_ram_aau(
    .rst        ( rst           ),
    .clk        ( clk           ),
    .cen        ( cen2          ),
    .r_field    ( r_field       ),
    .y_field    ( y_field       ),
    // Increment selecction
    .inc_sel    ( inc_sel       ),
    .ksel       ( ksel          ),
    .step_sel   ( step_sel      ),
    // Load control
    .short_load ( short_load    ),
    .long_load  ( long_load     ),
    .acc_load   ( acc_load      ),
    .ram_load   ( ram_load      ),
    .post_load  ( post_load     ),
    // register load inputs
    .short_imm  ( short_imm     ),
    .long_imm   ( long_imm      ),
    .acc        ( acc_dout      ),
    .ram_dout   ( ram_dout      ),
    // outputs
    .ram_addr   ( ram_addr      ),
    .reg_dout   ( r_yaau        )
);

jtdsp16_dau u_dau(
    .rst            ( rst           ),
    .clk            ( clk           ),
    .cen            ( cen2          ),
    // Decoder
    .dec_en         ( dau_dec_en    ),
    .con_en         ( dau_con_en    ),
    .r_field        ( r_field       ),
    .t_field        ( t_field       ),
    .op_fields      ( dau_op_fields ),
    // Acc control
    .rmux_load      ( dau_rmux_load ),
    .imm_load       ( dau_imm_load  ),
    .ram_load       ( dau_ram_load  ),
    .rmux           ( rmux          ),
    .st_a0h         ( st_a0h        ),
    .st_a1h         ( st_a1h        ),
    // Data buses
    .ram_dout       ( ram_dout      ),
    .rom_dout       ( rom_dout      ),
    .long_imm       ( long_imm      ),
    .cache_dout     ( cache_dout    ),
    .acc_dout       ( acc_dout      ),
    .reg_dout       ( r_dau         )
);

endmodule