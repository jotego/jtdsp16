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
    input             rst,
    input             clk,
    input             cen,
    // Instruction fields
    output reg        dau_dec_en,
    output reg        dau_con_en,
    output reg [ 4:0] t_field,
    output reg [ 4:0] c_field,
    output reg [ 2:0] r_field,
    output reg [ 1:0] y_field,
    output reg [ 1:0] a_field,
    output reg [ 5:0] dau_op_fields,
    output reg [ 2:0] rsel,

    // YAAU control
    // Increment selecction
    output reg [ 1:0] inc_sel,
    output reg        ksel,
    output reg        step_sel,
    // DAU
    output reg        at_sel,
    output reg        dau_rmux_load,
    output reg        dau_imm_load,
    output reg        dau_ram_load,
    output reg        st_a0h,
    output reg        st_a1h,
    output reg        acc_sel,
    input             con_result,
    // Load control
    output reg        short_load,
    output reg        long_load,
    output reg        acc_load,
    output reg        ram_load,
    output reg        post_load,
    output reg        ram_we,
    // register load inputs
    output reg [ 8:0] short_imm,
    output     [15:0] long_imm,

    // XAAU control
    // instruction types
    output reg        goto_ja,
    output reg        goto_b,
    output reg        call_ja,
    output reg        icall,
    output reg        post_inc,
    output reg        pc_halt,
    output reg        xaau_ram_load,
    output reg        xaau_imm_load,
    // instruction fields
    output reg [11:0] i_field,
    // IRQ
    output            no_int,       // do not accept an irq now
    // cache
    output reg        do_start,
    output reg [10:0] do_data,
    // X load control
    output            up_xram,
    output            up_xrom,
    output            up_xext,
    output            up_xcache,

    // Parallel port
    output reg        pio_imm_load,
    output reg        pdx_read,

    // Serial port
    output reg        sio_imm_load,

    // Data buses
    input      [15:0] rom_dout,
    output     [15:0] cache_dout,
    input      [15:0] ext_dout,

    // Debug
    output reg        fault
);

reg       x_field;
reg       double;
reg       con_check;
wire      con_ok;

assign    long_imm = rom_dout;
assign    con_ok   = ~dau_con_en | con_result;
assign    no_int   = ~double;

// Decode instruction
always @(posedge clk, posedge rst) begin
    if(rst) begin
        short_load    <= 0;
        long_load     <= 0;
        ram_load      <= 0;
        double        <= 0;
        post_load     <= 0;
        acc_load      <= 0;
        // ROM AAU
        goto_ja       <= 0;
        goto_b        <= 0;
        call_ja       <= 0;
        icall         <= 0;
        post_inc      <= 0;
        ram_we        <= 0;
        pc_halt       <= 0;
        xaau_ram_load <= 0;
        xaau_imm_load <= 0;
        do_data       <= 11'd0;
        do_start      <= 0;
        // *r++ control lines:
        y_field       <= 2'b0;
        step_sel      <= 0;
        ksel          <= 0;
        inc_sel       <= 2'b0;
        // DAU
        a_field       <= 2'd0;
        c_field       <= 5'd0;
        dau_dec_en    <= 0;
        dau_con_en    <= 0;
        at_sel        <= 0;
        dau_rmux_load <= 0;
        dau_imm_load  <= 0;
        dau_ram_load  <= 0;
        rsel          <= 3'd0;
        st_a0h        <= 0;
        st_a1h        <= 0;
        con_check     <= 0;
        acc_sel       <= 0;
        // Parallel port
        pio_imm_load  <= 0;
        pdx_read      <= 0;
        // Serial port
        sio_imm_load  <= 0;
        fault         <= 0;
    end else if(cen) begin
        t_field   <= rom_dout[15:11];
        i_field   <= rom_dout[11: 0];
        x_field   <= rom_dout[    4];
        c_field   <= rom_dout[ 4: 0];
        a_field   <= 2'd0;
        short_imm <= rom_dout[ 8: 0];

        con_check <= dau_con_en;

        // disable all control signals
        short_load    <= 0;
        long_load     <= 0;
        ram_load      <= 0;
        ram_we        <= 0;
        double        <= 0;
        post_load     <= 0;
        pc_halt       <= 0;

        // XAAU
        goto_ja       <= 0;
        goto_b        <= 0;
        call_ja       <= 0;
        xaau_ram_load <= 0;
        xaau_imm_load <= 0;
        do_start      <= 0;

        // DAU
        dau_op_fields <= 6'd0;
        dau_dec_en    <= 0;
        dau_con_en    <= 0;
        dau_rmux_load <= 0;
        dau_imm_load  <= 0;
        dau_ram_load  <= 0;
        st_a0h        <= 0;
        st_a1h        <= 0;
        acc_sel       <= 0;

        // Parallel port
        pio_imm_load  <= 0;
        pdx_read      <= 0;

        // Serial port
        sio_imm_load  <= 0;

        if(!double) begin
            casez( rom_dout[15:11] ) // T
                5'b0000?: begin // goto JA
                    goto_ja <= con_ok;
                    pc_halt <= ~con_ok;
                    double  <= 1;
                end
                5'b1000?: begin // call JA
                    call_ja <= con_ok;
                    pc_halt <= ~con_ok;
                    double  <= 1;
                end
                5'b11000: begin // goto B (ret, iret, goto pt, call pt)
                    goto_b  <= con_ok || (rom_dout[10:8]==3'b1); // iret is always executed
                    pc_halt <= ~con_ok;
                    double  <= 1;
                end
                5'b0001?: begin // short imm j, k, rb, re
                    short_load <= 1;
                    r_field    <= rom_dout[11:9]^3'b100;
                end
                5'b01000: begin // aT=R
                    r_field      <=  rom_dout[6:4];
                    rsel         <=  rom_dout[8:6];
                    dau_rmux_load<= 1;
                    pdx_read     <= 1;
                    at_sel       <=  rom_dout[10];
                    st_a0h       <=  rom_dout[10];
                    st_a1h       <= ~rom_dout[10];
                    double       <= 1;
                    pc_halt      <= 1;
                end
                5'b01010: begin // R=imm (long imm)
                    long_load     <= rom_dout[9:7]==3'b000; // YAAU register as destination
                    xaau_imm_load <= rom_dout[9:7]==3'b001; // XAAU register as destination
                    dau_imm_load  <= rom_dout[9:7]==3'b010; // DAU register as destination
                    sio_imm_load  <= rom_dout[9:6]==4'b0110; // Serial I/O - tdms register is not implemented
                    pio_imm_load  <= rom_dout[9:6]==4'b0111; // Parallel I/O
                    r_field       <= rom_dout[6:4];
                    double        <= 1;
                end
                5'b01111, // R=Y RAM load to r0-r3
                5'b01100  // Y=R r0-r3 storage to RAM
                : begin
                    ram_load  <=
                        rom_dout[15:10] == 6'b011110 &&
                        rom_dout[ 9:7]==3'b000; // YAAU register as destination
                    xaau_ram_load <=
                        rom_dout[15:10] == 6'b011110 &&
                        rom_dout[ 9:7]==3'b001; // YAAU register as destination
                    dau_ram_load <=
                        rom_dout[15:10] == 6'b011110 &&
                        rom_dout[ 9:7]==3'b010; // DAU register as destination
                    pdx_read <= rom_dout[15:11] == 5'b01111;
                    pc_halt <= 1;
                    if( rom_dout[15:11] == 5'b01100 ) begin
                        ram_we  <= 1; // RAM write
                    end else begin
                        ram_we  <= 0; // RAM load
                    end
                    rsel      <= rom_dout[ 8:6];
                    r_field   <= rom_dout[ 6:4];
                    y_field   <= rom_dout[ 3:2];
                    post_load <= 1;
                    case( rom_dout[1:0] )
                        2'd0: begin // *rN
                            inc_sel <= 2'd1;
                            step_sel<= 0;
                        end
                        2'd1: begin // *rN++
                            inc_sel <= 2'd2;
                            step_sel<= 0;
                        end
                        2'd2: begin // *rN--
                            inc_sel  <= 2'd0;
                            step_sel <= 0;
                        end
                        2'd3: begin // *rN++j
                            step_sel <= 1;
                            ksel     <= 0;
                        end
                    endcase
                    double   <= 1;
                end

                5'b0011?: begin // F1 Y
                    dau_dec_en    <= 1;
                    dau_op_fields <= rom_dout[10:5];
                    //mul_en
                end

                5'b10011: // if CON F2
                begin
                    dau_con_en    <= 1;
                    dau_op_fields <= rom_dout[10:5];
                end

                5'b10100, // F1, *rN = y, 2 cycles
                5'b10111, // F1, y[k]=Y
                5'b11100, // F1, Y=a0[l]
                5'b00100: // F1, Y=a1[l]
                begin
                    dau_dec_en <= 1;
                    dau_op_fields <= rom_dout[10:5];
                    case( rom_dout[15:11] )
                        5'b10100: begin // RAM write
                            ram_we <= 1;
                            rsel   <= 3'b010;  // DAU
                        end
                        5'b10111: begin // write to y[l] register
                            dau_ram_load <= 1;
                        end
                        default: begin
                            rsel <= 3'b010; // DAU
                            acc_sel <= 1;
                            a_field <= { rom_dout[4], ~rom_dout[15] };
                        end
                    endcase
                    pc_halt   <= 1;
                    double    <= 1;
                    y_field   <= rom_dout[3:2];
                    r_field   <= rom_dout[4] ? 3'd1 : 3'd2; // select y or yl
                    post_load <= 1;
                    case( rom_dout[1:0] )
                        2'd0: begin // *rN
                            inc_sel <= 2'd1;
                            step_sel<= 0;
                        end
                        2'd1: begin // *rN++
                            inc_sel <= 2'd2;
                            step_sel<= 0;
                        end
                        2'd2: begin // *rN--
                            inc_sel  <= 2'd0;
                            step_sel <= 0;
                        end
                        2'd3: begin // *rN++j
                            step_sel <= 1;
                            ksel     <= 0;
                        end
                    endcase
                end
                5'b11010: begin // conditional branch
                    dau_con_en    <= 1;
                end
                5'b1110: begin // do
                    do_data  <= rom_dout[10:0];
                    do_start <= 1;
                    pc_halt  <= rom_dout[10:7]==4'd0;
                    double   <= rom_dout[10:7]==4'd0;
                end
                default: fault<=1;
            endcase
        end
    end
end

endmodule