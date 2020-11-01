`timescale 1ns / 1ps

module test;

reg         clk;
wire        rst, cen, iack;
reg         auto_finish;

wire [15:0] ext_addr;
reg  [15:0] ext_data;
reg  [12:0] prog_addr;
wire [15:0] prog_data16;
wire [ 7:0] prog_data;
reg         prog_we;
reg         irq;
reg  [15:0] rom[0:8191];

reg  [15:0] pbus_in;
wire [15:0] pbus_out;
wire        pods_n;        // parallel output data strobe
wire        pids_n;        // parallel input  data strobe
wire        psel;          // peripheral select
wire        ock, sdo, sadd;


assign      cen = 1;
assign      rst = prog_we;
assign      prog_data16 = rom[ prog_addr[12:1] ];
assign      prog_data   = prog_addr[0] ? prog_data16[15:8] : prog_data16[7:0];

integer f, fcnt;

initial begin
    f = $fopen("test.bin","rb");
    fcnt=$fread( rom, f);
    $fclose(f);
end

initial begin
    clk = 0;
    forever #15 clk=~clk;
end

task do_report;
    // YAAU
    $display("r0=0x%04X", UUT.u_ram_aau.r0);
    $display("r1=0x%04X", UUT.u_ram_aau.r1);
    $display("r2=0x%04X", UUT.u_ram_aau.r2);
    $display("r3=0x%04X", UUT.u_ram_aau.r3);
    $display("rb=0x%04X", UUT.u_ram_aau.rb);
    $display("re=0x%04X", UUT.u_ram_aau.re);
    $display("j =0x%04X", UUT.u_ram_aau.j);
    $display("k =0x%04X", UUT.u_ram_aau.k);
    // XAUU
    $display("pr=0x%04X", UUT.u_rom_aau.pr);
    $display("pt=0x%04X", UUT.u_rom_aau.pt);
    $display("i =0x%04X", UUT.u_rom_aau.i);
    $display("pi=0x%04X", UUT.u_rom_aau.pi);
    $display("pc=0x%04X", UUT.u_rom_aau.pc);
    // DAU
    $display("a0=0x%04X", UUT.u_dau.a0);
    $display("a1=0x%04X", UUT.u_dau.a1);
    $display("x=0x%04X", UUT.u_dau.x);
    $display("y=0x%04X", UUT.u_dau.y);
    $display("p=0x%08X", UUT.u_dau.p);
    $display("c0=0x%04X", UUT.u_dau.c0);
    $display("c1=0x%04X", UUT.u_dau.c1);
    $display("c2=0x%04X", UUT.u_dau.c2);
    $display("auc=0x%04X", UUT.u_dau.auc);
    $display("psw=0x%04X", UUT.u_dau.psw);
endtask

initial begin
    prog_addr = 0;
    prog_we   = 1;
    #45_000;
    if( auto_finish ) begin
        do_report();
        $finish;
    end
end

always @(posedge clk) begin
    if( prog_addr < 13'd512 ) begin
        prog_addr <= prog_addr + 1'd1;
    end else begin
        prog_we <= 0;
    end
end

// Provide some input to the parallel port
always @(posedge pids_n, posedge rst) begin
    if( rst ) begin
        pbus_in <= 16'hbeef;
    end else begin
        pbus_in <= pbus_in+1;
    end
end

// Generate an interrupt after a certain parallel port output
reg last_podsn;

always @(posedge clk, posedge rst) begin
    if( rst ) begin
        irq <= 0;
        last_podsn <= 1;
        auto_finish <= 1;
    end else begin
        last_podsn <= pods_n;
        if( pbus_out==16'hcafe && pods_n && !last_podsn) irq <= 1;
        if( pbus_out==16'hdead && pods_n && !last_podsn) begin
            auto_finish <= 0;
            if( !auto_finish ) begin
                do_report();
                $finish;
            end else begin
                $display("INFO: automatic simulation finish is disabled");
            end
        end
        else if( iack ) irq <= 0;
    end
end

jtdsp16 UUT(
    .rst        ( rst       ),
    .clk        ( clk       ),
    .cen        ( cen       ),
    .ext_mode   ( 1'b0      ),
    // Parallel I/O
    .pbus_in    ( pbus_in   ),
    .pbus_out   ( pbus_out  ),
    .pods_n     ( pods_n    ),
    .pids_n     ( pids_n    ),
    .psel       ( psel      ),
    // Serial I/O
    .sdo        ( sdo       ),
    .ock        ( ock       ),
    .sadd       ( sadd      ),
    // interrupts
    .irq        ( irq       ),
    .iack       ( iack      ),
    // ROM programming interface
    .prog_addr  ( prog_addr ),
    .prog_data  ( prog_data ),
    .prog_we    ( prog_we   )
);

`ifndef NODUMP
always @(negedge prog_we) begin
    $dumpfile("test.lxt");
    $dumpvars;
end
`endif

endmodule