; A small/simple kernal written in ACME assembler format

; -----------------------------------------
; clears the stack and decimal flag
; checks for a cartridge, jumps to vector at ($8000) if cartridge id found at $8004
; store pal/ntsc flag at 02a6 (1 for pal)
; sets up irq timer on cia 1 to run at 1/60th sec
; irq timer updates the jiffy clock at $a0-$a2
; sets screen to black
; loops infinitely, if $14 is non zero will jump to vector at ($0015)
; -----------------------------------------


; disable interrupts
	sei
	
; clear the stack
	ldx #$ff
	txs
	
; clear decimal flag
	cld
	
	; check for cartridge
	; a cartridge will have 'CBM80' at $8003
cartridge_check	
	ldx #$05
cartridge_check_loop	
	lda $8003,x
	cmp cartridge_id - 1,x
	bne end_of_cartridge_check
	dex
	bne cartridge_check_loop
	
	; it's a cartridge
	; jump to the vector at $8000
	jmp ($8000)
	
end_of_cartridge_check

turn_off_screen_sid
	; turn off screen
	; screen control register 1
	; vertical scroll 3, 25 rows, screen on, text mode
	; bits 0-2: vertical scroll
	; bit    3: screen height (1 = 25 rows)
	; bit    4: screen off/on
	; bit    5: text/bitmap mode
	; bit    6: extended background mode
	; bit    7: interrupt raster line
	lda #%00001011
	sta $d011   ; vic control register 1: scroll y

	; turn off sid
	; filter mode/main volume
	lda #0
	sta $d418

io_init
	; disable interrupts for both cias
	lda #%01111111
	; bit   0: enable timer a underflow interrupts
	; bit   1: enable timer a underflow interrupts
	; bit   2: enable tod alarm interrupts
	; bit   3: enable serial shif register interrupts
	; bit   4: enable flag pin interrupts
	; bit   7: bits 0-6 that are set to 1 are filled with this value
	; cia 1 interrupt control	
	sta $dc0d ; CIA1_ICR ; $DC0D
	
	; cia 2 interrrupt control, same bits as cia 1
	sta $dd0d ; CIA2_ICR ; $DD0D

	; cia port a
	; bits 0-5: select keyboard matrix column
	; bits 6-7: paddle selection
	sta $dc00 ; CIA1_PRA ; $DC00
	
	; setup keyboard scan: cia 1 port a = output, cia 1 port b = input
	; cia 1 port a data direction register
	lda #%11111111
	sta $dc02

	lda #0
	; cia 1 port b data direction register
	sta $dc03
	; cia 2 port b data direction register (for rs232), not used
	sta $dd03


	; cia 2 data direction register
	; 0 = read only, 1 = read/write
	; want to be able to write to bits 0-1 of dd00 to set vic address
	; don't really care about rs232 or serial bus bits
	lda #%00111111
	sta $dd02; CIA2_DDRA    ; $DD02

	; set the vic bank, rs232 output set to high
	; cia 2 port a
	; bits 0-1: vic bank
	; bit    2: rs232 data output
	; bit    3: serial bus atn signal
	; bit    4: serial bus clock pulse output
	; bit    5: serial bus data output
	; bit    6: serial bus clock pulse input
	; bit    7: serial bus data input 
	lda #%00000111 
	sta $dd00 ; CIA2_PRA     ; $DD00

	; turn off all timers
	; bit    0: stop/start timer
	; bit    1: timeout a output on port b bit 6/7
	; bit    2: timer output mode to port b bit 6/7
	; bit    3: 1 = one shot, 0 = continuous
	; bit    4: load start into timer
	; bits 5-6: what timer counts
	; bit    7: whether writing into tod sets tod or alarm tod 
	lda #%00001000
	sta $dc0e
	; cia 1 timer b
	sta $dc0f ;CIA1_CRB     ; $DC0F
	; cia 2 timer a
	sta $dd0e ; CIA2_CRA     ; $DD0E
	; cia 2 timer b
	sta $dd0f ; CIA2_CRB     ; $DD0F
	
cpu_init
	; basic/kernal/io mapped in, datasette motor off
	; bits 0-2: ram config
	; bit    3: datasette output signal level
	; bit    4: datasette button status
	; bit    5: datasette motor control 1=off
	lda #%00110111
	sta $01  ; processor port
	
	; processor port data direction, 0=read only,1=read/write
	ldx #%00101111
	stx $00  ; processor port data direction


detect_pal_ntsc

	lda $d012  ; raster read/write register
detect_pal_ntsc_check_raster
	cmp $d012
	beq detect_pal_ntsc_check_raster
	bmi detect_pal_ntsc

	; accumulator will have the value of the raster register before it goes from last line to 0
	; #$37 -> 312 rasterlines, PAL,  VIC 6569
	; #$06 -> 263 rasterlines, NTSC, VIC 6567R8
	; #$05 -> 262 rasterlines, NTSC, VIC 6567R56A

	cmp #$07
	bcc ntsc_detected

pal_detected

	lda #$01
	bne pal_ntsc_test_done
ntsc_detected

	lda #$00

pal_ntsc_test_done

	; accumulator has 1 for pal, 0 for ntsc
	sta $02a6
	tax
	
	; setup irq timer on cia1 to 1/60th sec
	lda irq_timing_lo,x
	sta $dc04
	lda irq_timing_hi,x
	sta $dc05

clear_ram

	; clear $0002-$0101, $0200-$03ff
	ldy #$00
	lda #$00
clear_ram_loop
	sta $0300,y
	sta $0200,y
	sta $0002,y
	iny
	bne clear_ram_loop

	; init vic ii registers $d000-$d02e
	; set to 0
vicii_init_registers	
	lda #$00
	ldx #$2e
vicii_init_registers_loop
	sta $d000,x
	dex
	bpl vicii_init_registers_loop

	; black border/bg
	sta $d020
	sta $d021

	; if $14 is non zero, kernal will jump to the address at $15/$16
	sta $14
	sta $15
	sta $16

	; turn screen back on, set to correct mode
	; screen control register 1
	; vertical scroll 3, 25 rows, screen on, text mode
	; bits 0-2: vertical scroll
	; bit    3: screen height (1 = 25 rows)
	; bit    4: screen off/on
	; bit    5: text/bitmap mode
	; bit    6: extended background mode
	; bit    7: interrupt raster line
	lda #%00011011
	sta $d011   ; vic control register 1: scroll y
	
	; screen control register 2
	; multicolor off, 40 columns, horizontal scroll 0
	; bits 0-2: horizontal scroll
	; bit    3: screen width
	; bit    4: multicolor on/off
	lda #%11001000
	sta $d016
	
	; charset address/video matrix address
	; character memory at $1000, screen memory at $400
	; bits 1-3: pointer to character memory relative to vic bank
	; bits 4-7: pointer to screen memory relative to vic bank
	lda #%00010100
	sta $d018
	
	
	; clear interrupts
	; interrupt status register
	; bit    0: acknowledge raster interrupt
	; bit    1: acknowledge sprite-background interrupt
	; bit    2: acknowledge sprite-sprite interrupt
	; bit    3: acknowledge light pen interrupt 
	lda #%00001111
	sta $d019                        ; clear all interrupts


	; enable timer a to run continuously
	lda #$11
	sta $dc0e

	; Enable timer interrupt
	lda #$81
	sta $dc0d	

	; enable interrupts
	cli
loop	

	lda $14
	beq loop
	jmp ($0015)

; nmi handler, do nothing
nmi
	rti
	
; irq trigged by cia 1 every 1/60th of a sec	
irq
	pha
	txa
	pha
	tya
	pha
	
	; update the jiffy clock at $a0 - $a2
	; 24bit number is number of jiffies (1/60th sec) since midnight
	; $4f1a00 jiffies per day
	inc $a2
	bne jiffy_check_day_rollover
	inc $a1
	bne jiffy_check_day_rollover
	inc $a0

jiffy_check_day_rollover

	lda $a0
	cmp #$4F

	bcc jiffy_done
	lda $a1
	cmp #$1A

	bcc jiffy_done
	
	; clock has reached 4f1a00, so reset it
jiffy_reset

	lda #$00
	sta $a0
	sta $a1
	sta $a2

jiffy_done

	; clear interrupt flags
	lda $dc0d
	pla
	tay
	pla
	tax
	pla
	
	rti

cartridge_id
!byte $c3, $c2, $cd, $38, $30

; values for cia 1 timer to be triggered 1/60 sec
irq_timing_lo
!byte $95 ; ntsc
!byte $25 ; pal

irq_timing_hi
!byte $42 ; ntsc
!byte $40 ; pal

jump_to_start = $14
start_vector  = $0015
