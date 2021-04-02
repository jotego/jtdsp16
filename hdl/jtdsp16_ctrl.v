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
    input             ph1,
    // Instruction fields
    output reg        dau_dec_en,
    output reg        con_check,
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
    output reg        dau_rmux_load,
    output reg        dau_imm_load,
    output reg        dau_ram_load,
    output reg        dau_acc_load,
    output reg        dau_pt_load,
    output reg        dau_yacc_load,
    output reg        dau_acc_ram,
    output reg        dau_special,
    output reg        st_a0,
    output reg        st_a1,
    output reg        st_ah,
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
    output            icall,
    output reg        pc_halt,
    output reg        xaau_ram_load,
    output reg        xaau_imm_load,
    output reg        xaau_acc_load,
    // *pt++[i] reads
    output reg        pt_read,
    output reg        xaau_istep,
    // instruction fields
    output reg [11:0] i_field,
    // IRQ
    input             irq,
    output reg        irq_start,
    output reg        iack,
    // cache
    output            do_start,
    output            do_out,
    output reg        do_short,
    output reg        do_redo,
    output reg        do_save,
    output reg [10:0] do_data,
    output     [ 3:0] do_pc,
    // X load control
    output            up_xram,
    output            up_xrom,
    output            up_xext,
    output            up_xcache,

    // Parallel port
    output reg        pio_imm_load,
    output reg        pio_ram_load,
    output reg        pio_acc_load,
    output reg        pdx_read,

    // Serial port
    output reg        sio_imm_load,
    output reg        sio_acc_load,
    output reg        sio_ram_load,

    // Data buses
    input      [15:0] rom_dout,

    // Debug
    output reg        fault
);

reg       double;
wire      con_ok;
// Y control
reg       pre_step_sel, pre_ksel;
reg [1:0] pre_inc_sel;

// do/redo loops
reg [6:0] do_k_cnt, do_k;
reg [3:0] do_ni_cnt, do_ni;
reg       do_cnt_ld, do_1done;
wire      do_1stloop, do_busy, do_over, do_incache;

// interrupts
reg       irq_ok, clr_iack;

assign    long_imm = rom_dout;
assign    con_ok   = ~con_check | con_result;

// do/redo
assign    do_over    = do_ni_cnt == do_ni;
assign    do_pc      = do_ni_cnt;
assign    do_start   = (!do_redo && do_1stloop) || (do_redo && !do_busy);
assign    do_busy    = do_k_cnt!=7'd0;
assign    do_1stloop = ((do_over && do_k_cnt==do_k && do_busy) || do_short) && !do_1done;
assign    do_out     = do_busy && do_over && do_k_cnt==7'd1 && !double;
assign    do_incache = do_k_cnt > 0 && do_k_cnt != do_k;

// interrupts
assign    icall      = 0; // software interrupts are not implemented

always @(*) begin
    case( rom_dout[15:11] )
        5'd0, 5'd1, 5'd14, 5'd16, 5'd17, 5'd24, 5'd26: irq_ok = 0;
        default: irq_ok = 1;
    endcase
end

always @(*) begin
    pre_step_sel = 0;
    pre_ksel     = 0;
    pre_inc_sel  = 2'd0;
    case( rom_dout[1:0] )
        2'd0: begin // *rN
            pre_inc_sel  = 2'd1;
            pre_step_sel = 0;
        end
        2'd1: begin // *rN++
            pre_inc_sel  = 2'd2;
            pre_step_sel = 0;
        end
        2'd2: begin // *rN--
            pre_inc_sel  = 2'd0;
            pre_step_sel = 0;
        end
        2'd3: begin // *rN++j
            pre_step_sel = 1;
            pre_ksel     = 0;
        end
    endcase
end

// DO counter
always @(posedge clk, posedge rst) begin
    if(rst) begin
        do_ni_cnt <= 4'd0;
        do_k_cnt  <= 7'd0;
    end else if(ph1) begin
        if( do_cnt_ld ) begin
            do_ni_cnt <= (do_redo||do_short) ? 4'd0 : 4'd1;
            do_k_cnt  <= do_k;
        end else if(!double) begin
            do_ni_cnt <= do_over ? 4'd0 : do_ni_cnt+4'd1;
            if( do_over && do_k_cnt!=7'd0 ) begin
                do_k_cnt <= do_k_cnt-7'd1;
            end
        end
    end
end


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
        ram_we        <= 0;
        pc_halt       <= 0;
        con_check     <= 0;
        xaau_ram_load <= 0;
        xaau_imm_load <= 0;
        xaau_acc_load <= 0;
        xaau_istep    <= 0;
        do_data       <= 11'd0;
        pt_read       <= 0;
        // Cache
        do_cnt_ld     <= 0;
        do_redo       <= 0;
        do_ni         <= 4'd0;
        do_k          <= 7'd0;
        do_1done      <= 0;
        do_save       <= 0;
        // interrupts
        irq_start     <= 0;
        iack          <= 0;
        clr_iack      <= 0;
        // *r++ control lines:
        y_field       <= 2'b0;
        step_sel      <= 0;
        ksel          <= 0;
        inc_sel       <= 2'b0;
        // DAU
        a_field       <= 2'd0;
        c_field       <= 5'd0;
        dau_dec_en    <= 0;
        dau_rmux_load <= 0;
        dau_imm_load  <= 0;
        dau_ram_load  <= 0;
        dau_pt_load   <= 0;
        dau_yacc_load <= 0;
        dau_special   <= 0;
        dau_acc_ram   <= 0;
        rsel          <= 3'd0;
        st_a0         <= 0;
        st_a1         <= 0;
        st_ah         <= 0;
        acc_sel       <= 0;
        // Parallel port
        pio_imm_load  <= 0;
        pio_ram_load  <= 0;
        pio_acc_load  <= 0;
        pdx_read      <= 0;
        // Serial port
        sio_imm_load  <= 0;
        sio_acc_load  <= 0;
        sio_ram_load  <= 0;

        do_short    <= 0;
        fault         <= 0;
    end else if(ph1) begin
        t_field       <= rom_dout[15:11];
        i_field       <= rom_dout[11: 0];
        c_field       <= rom_dout[ 4: 0];
        y_field       <= rom_dout[ 3:2];
        dau_op_fields <= rom_dout[10:5];
        a_field       <= 2'd0;
        short_imm     <= rom_dout[ 8: 0];

        // disable all control signals
        short_load    <= 0;
        long_load     <= 0;
        ram_load      <= 0;
        acc_load      <= 0;
        ram_we        <= 0;
        double        <= 0;
        post_load     <= 0;
        pc_halt       <= 0;
        con_check     <= 0;
        do_short      <= 0;

        // XAAU
        goto_ja       <= 0;
        goto_b        <= 0;
        call_ja       <= 0;
        xaau_ram_load <= 0;
        xaau_imm_load <= 0;
        xaau_acc_load <= 0;
        xaau_istep    <= 0;
        pt_read       <= 0;

        // Cache
        do_cnt_ld     <= 0;
        do_save       <= 0;

        // interrupts
        irq_start     <= 0;

        // DAU
        dau_dec_en    <= 0;
        dau_rmux_load <= 0;
        dau_imm_load  <= 0;
        dau_ram_load  <= 0;
        dau_acc_load  <= 0;
        dau_pt_load   <= 0;
        dau_yacc_load <= 0;
        dau_acc_ram   <= 0;
        dau_special   <= 0;
        st_a0         <= 0;
        st_a1         <= 0;
        st_ah         <= 0;
        acc_sel       <= 0;

        // Parallel port
        pio_imm_load  <= 0;
        pio_ram_load  <= 0;
        pio_acc_load  <= 0;
        pdx_read      <= 0;

        // Serial portf
        sio_imm_load  <= 0;
        sio_acc_load  <= 0;
        sio_ram_load  <= 0;

        if( !double ) begin
            if( irq && !do_busy && irq_ok && !iack ) begin
                irq_start <= 1;
                pc_halt   <= 1;
                double    <= 1;
                iack      <= 1;
                clr_iack  <= 0;
            end else begin
                if( irq_ok && clr_iack ) begin
                    iack     <= 0;
                    clr_iack <= 0;
                end

                casez( rom_dout[15:11] ) // T
                    5'b0000?: begin // goto JA
                        goto_ja <= con_ok;
                        pc_halt <= 1;
                        double  <= 1;
                        if( do_busy ) fault <= 1; // illegal instruction
                    end

                    5'b0001?: begin // short imm j, k, rb, re
                        short_load <= 1;
                        r_field    <= rom_dout[11:9]^3'b100;
                    end

                    5'b1000?: begin // call JA
                        call_ja <= con_ok;
                        pc_halt <= 1;
                        double  <= 1;
                        if( do_busy ) fault <= 1; // illegal instruction
                    end

                    5'b11000: begin // goto B (ret, iret, goto pt, call pt)
                        goto_b  <= con_ok || (rom_dout[10:8]==3'b1); // iret is always executed
                        pc_halt <= 1;
                        double  <= 1;
                        if( rom_dout[10:8]==3'b1 ) clr_iack <= 1;
                        if( do_busy ) fault <= 1; // illegal instruction
                    end

                    5'b01000: begin // aT=R
                        r_field      <=  rom_dout[6:4];
                        rsel         <=  rom_dout[8:6];
                        dau_rmux_load<= 1;
                        pdx_read     <=  rom_dout[9:6]==4'b0111;
                        st_a0        <=  rom_dout[10];
                        st_a1        <= ~rom_dout[10];
                        st_ah        <= 1;
                        double       <= 1;
                        pc_halt      <= 1;
                    end

                    5'b01001, 5'b01011: begin // R=a0 / R=a1
                        r_field      <=  rom_dout[6:4];
                        a_field      <=  { 1'b1, rom_dout[12] };
                        acc_sel      <= 1;
                        dau_acc_load <= rom_dout[8:7]==2'b10;  // DAU register
                        acc_load     <= rom_dout[8:7]==2'b00;  // RAM AAU register
                        xaau_acc_load<= rom_dout[8:7]==2'b01;  // ROM AAU register
                        sio_acc_load <= rom_dout[8:6]==3'b110; // SIO register
                        pio_acc_load <= rom_dout[8:6]==3'b111; // PIO register
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
                        if( do_busy ) fault <= 1; // illegal instruction
                    end

                    5'b01111, // R=Y RAM load to r0-r3
                    5'b01100  // Y=R r0-r3 storage to RAM
                    : begin
                        if( rom_dout[15:10] == 6'b011110 ) begin
                            ram_load      <= rom_dout[ 9:7]==3'b000; // YAAU register as destination
                            xaau_ram_load <= rom_dout[ 9:7]==3'b001; // YAAU register as destination
                            dau_ram_load  <= rom_dout[ 9:7]==3'b010; // DAU register as destination
                            sio_ram_load  <= rom_dout[ 9:6]==4'b0110;
                            pio_ram_load  <= rom_dout[ 9:6]==4'b0111;
                        end else begin
                            pdx_read <= rom_dout[9:6]==4'b0111;
                        end
                        if( rom_dout[15:11] == 5'b01100 ) begin
                            ram_we  <= 1; // RAM write
                        end else begin
                            ram_we  <= 0; // RAM load
                        end
                        rsel      <= rom_dout[ 8:6];
                        r_field   <= rom_dout[ 6:4];
                        pc_halt   <= 1;
                        double    <= 1;
                        // Y control
                        post_load <= 1;
                        inc_sel   <= pre_inc_sel;
                        step_sel  <= pre_step_sel;
                        ksel      <= pre_ksel;
                    end

                    5'b00110, // Y    F1
                    5'b00111: // aT=Y F1
                    begin
                        dau_dec_en    <= 1;
                        // accumulator storing
                        if( rom_dout[11] ) begin
                            st_a0      <=  rom_dout[10];
                            st_a1      <= ~rom_dout[10];
                            st_ah      <=  rom_dout[4];
                            dau_acc_ram <= 1;
                        end
                        // Y control
                        post_load <= 1;
                        inc_sel   <= pre_inc_sel;
                        step_sel  <= pre_step_sel;
                        ksel      <= pre_ksel;
                    end

                    5'b10011: begin // if CON F2
                        dau_dec_en    <= 1;
                        dau_special   <= 1;
                    end

                    5'b10101: begin // Z:y F1
                        // F1
                        dau_dec_en    <= 1;
                        // DAU
                        // zyh_swap      <=  rom_dout[4];
                        // zyl_swap      <= ~rom_dout[4];
                        dau_ram_load  <= 1;
                        // Register mux
                        rsel          <= 3'b100; // DAU
                        r_field       <= rom_dout[4] ? 3'd1 /* yh */: 3'd2 /* yl */;
                        ram_we        <= 1;
                        // RAM AAU
                        y_field       <= rom_dout[3:2]; // selects register
                        inc_sel       <= 2'd2; // +1. Only the zp case is supported
                        step_sel      <= 0;
                        post_load     <= 1;
                        double        <= 1;
                        pc_halt       <= 1;
                    end

                    5'b10110: begin // F1, x=Y, 1 cycle
                        dau_dec_en    <= 1;
                        dau_ram_load  <= 1;
                        r_field       <= 3'd0; // x
                        // Y control
                        post_load <= 1;
                        inc_sel   <= pre_inc_sel;
                        step_sel  <= pre_step_sel;
                        ksel      <= pre_ksel;
                    end
                    5'b11001,       // 25, F1, y = a0, x = *pt++[i]
                    5'b11011: begin // 27, F1, y = a1, x = *pt++[i], 2 or 1 cycles (cache)
                        // F1
                        dau_dec_en    <= 1;
                        // y load
                        a_field       <= { 1'b1, rom_dout[12]};
                        dau_yacc_load <= 1;
                        // x load
                        dau_pt_load   <= 1;
                        xaau_istep    <= rom_dout[4];
                        pt_read       <= 1;
                        if( !do_incache ) begin
                            double    <= 1;
                            pc_halt   <= 1;
                        end
                    end
                    5'b11111: begin // F1, y = Y, x = *pt++[i], 2 or 1 cycles (cache)
                        // F1
                        dau_dec_en    <= 1;
                        // y load
                        dau_ram_load  <= 1;
                        r_field       <= 3'd1; // y
                        // x load
                        dau_pt_load   <= 1;
                        xaau_istep    <= rom_dout[4];
                        pt_read       <= 1;
                        // Y control
                        post_load <= 1;
                        inc_sel   <= pre_inc_sel;
                        step_sel  <= pre_step_sel;
                        ksel      <= pre_ksel;
                        if( !do_incache ) begin
                            double    <= 1;
                            pc_halt   <= 1;
                        end
                    end
                    5'b10100, // F1, Y = y[l], 2 cycles
                    5'b10111, // F1, y[l]=Y, 1 cycle
                    5'b11100, // F1, Y=a0[l], 2 cycles
                    5'b00100: // F1, Y=a1[l], 2 cycles
                    begin
                        case( rom_dout[15:11] )
                            5'b10100: begin // RAM write
                                ram_we <= 1;
                                rsel   <= 3'b100;  // DAU
                                double <= 1;
                                pc_halt<= 1;
                            end
                            5'b10111: begin // write to y[l] register
                                dau_ram_load <= 1;
                            end
                            default: begin
                                rsel <= 3'b010; // DAU
                                acc_sel <= 1;
                                ram_we  <= 1;
                                double  <= 1;
                                pc_halt <= 1;
                                a_field <= { rom_dout[4], ~rom_dout[15] };
                            end
                        endcase
                        dau_dec_en    <= 1;
                        r_field   <= rom_dout[4] ? 3'd1  /* y */: 3'd2 /* yl */; // select y or yl
                        // Y control
                        post_load <= 1;
                        inc_sel   <= pre_inc_sel;
                        step_sel  <= pre_step_sel;
                        ksel      <= pre_ksel;
                    end
                    5'b11010: begin
                        if( !rom_dout[10] ) con_check <= 1; // conditional branch
                        // else trigger icall - not implemented
                    end
                    5'b01110: begin // do
                        do_data  <= rom_dout[10:0];
                        do_cnt_ld <= 1;
                        do_1done  <= 0;
                        if( rom_dout[10:7]==4'd0 ) begin // redo
                            pc_halt   <= 1;
                            double    <= 1;
                            do_redo   <= 1;
                            do_k      <= rom_dout[ 6:0];
                        end else begin // do
                            do_ni     <= rom_dout[10:7]-4'd1;
                            do_redo   <= 0;
                            do_save   <= 1;
                            if( rom_dout[10:7]==4'd1 ) begin
                                // when NI=1 the next instruction must be executed
                                // in two cycles but there is no time to catch it
                                // via the do_1stloop signal
                                do_short <= 1;
                                do_k     <= rom_dout[ 6:0]-7'd1;
                            end else begin
                                do_k     <= rom_dout[ 6:0];
                            end
                        end
                    end
                    default: fault<=1;
                endcase
            end
        end
        if( (do_1stloop && !do_redo) || do_out ) begin
            // last instruction of 1st loop in DO takes two cycles
            // last instruction of whole DO/REDO sequence takes two cycles
            if( !do_1stloop && do_out ) begin
                do_redo    <= 0;
            end
            pc_halt  <= 1;
            double   <= 1;
            do_1done <= 1;
        end
    end
end

endmodule