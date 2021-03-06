;; This is an adaptation of video.s from http://mc.pp.se/dc/sw.html.

.sh4
.include "powervr.inc"

TILE_WIDTH equ (640 / 32)
TILE_HEIGHT equ (480 / 32)

;.org 0x0c000000
.org 0x8c010000

start:
  ; First, make sure to run in the P2 area (privileged mode, no cache).
  mov.l setup_cache_addr, r0
  mov.l p2_mask, r1
  or r1, r0
  jmp @r0
  nop

setup_cache:
  ; Now that the SH4 is in P2, it's safe to enable the cache.
  mov.l ccr_addr, r0
  mov.w ccr_data, r1
  mov.l r1, @r0
  ; After changing CCR, eight instructions must be executed before it's
  ; safe to enter a cached area such as P1.
  mov.l main_addr, r0 ; 1
  mov #0, r1          ; 2
  nop                 ; 3
  nop                 ; 4
  nop                 ; 5
  nop                 ; 6
  nop                 ; 7
  nop                 ; 8
  jmp @r0             ; go
  mov r1, r0

.align 32
p2_mask:
  .dc32 0xa0000000
setup_cache_addr:
  .dc32 setup_cache
main_addr:
  .dc32 main
ccr_addr:
  .dc32 0xff00001c
ccr_data:
  .dc16 0x090d

main:
  ;; Setup NTSC and turn on video output enable.
  mov.l sync_cfg, r1
  mov.l sync_cfg_value, r2
  mov.l r2, @r1

  ;; Setup video sync.
  mov.l display_sync_load, r1
  mov.l display_sync_load_value, r2
  mov.l r2, @r1

  mov.l display_sync_width, r1
  mov.l display_sync_width_value, r2
  mov.l r2, @r1

  ;; Setup display mode.
  mov.l display_mode, r1
  mov.l display_mode_value, r2
  mov.l r2, @r1

  ;; Setup hborder.
  mov.l hborder, r1
  mov.l hborder_value, r2
  mov.l r2, @r1

  ;; Setup vborder.
  mov.l vborder, r1
  mov.l vborder_value, r2
  mov.l r2, @r1

  ;; Setup hposition.
  mov.l hposition, r1
  mov.l hposition_value, r2
  mov.l r2, @r1

  ;; Setup vposition.
  mov.l vposition, r1
  mov.l vposition_value, r2
  mov.l r2, @r1

  ;; Setup display size.
  mov.l display_size, r1
  mov.l display_size_value, r2
  mov.l r2, @r1

  ;; Setup misc setting.
  mov.l video_config, r1
  mov.l video_config_value, r2
  mov.l r2, @r1

  ;; Unknown...
  mov.l unknown_1, r1
  mov.l unknown_1_value, r2
  mov.l r2, @r1

  mov.l unknown_2, r1
  mov.l unknown_2_value, r2
  mov.l r2, @r1

  ;; Setup display memory region.
  mov.l display_memory_1, r1
  mov.l display_memory_1_value, r2
  ;mov #0, r2
  mov.l r2, @r1

  mov.l display_memory_2, r1
  mov.l display_memory_2_value, r2
  ;mov #0, r2
  mov.l r2, @r1

  ;; Setup border color
  mov.l border_col, r1
  mov #127, r2
  mov.l r2, @r1

  ;; Put some stuff in the frame buffer.
  mov.l frame_buffer, r1
  mov.l test_color, r2
  mov.l pixel_count, r0
frame_buffer_loop:
  mov.l r2, @r1
  add #4, r1
  add #-1, r0
  cmp/eq #0, r0
  bf frame_buffer_loop
  nop

  bra tile_accelerator
  nop

.align 32
border_col:
  .dc32 POWERVR_BORDER_COL
sync_cfg:
  .dc32 POWERVR_SYNC_CFG
sync_cfg_value:
  .dc32 POWERVR_SYNC_ENABLE | POWERVR_VIDEO_NTSC | POWERVR_VIDEO_INTERLACE
display_mode:
  .dc32 POWERVR_FB_DISPLAY_CFG
display_mode_value:
  ;; 8: Threshold when display mode is ARGB8888.
  ;; 2: Pixel mode 3 = RGB0888 (4 bytes / pixel)
  ;; 1: Line double.
  ;; 0: Enable.
  .dc32 (0xff << 8) | (3 << 2) | 1
display_memory_1:
  .dc32 POWERVR_FB_DISPLAY_ADDR1
display_memory_1_value:
  .dc32 0
display_memory_2:
  .dc32 POWERVR_FB_DISPLAY_ADDR2
display_memory_2_value:
  .dc32 640 * 240 * 4
frame_buffer:
  .dc32 0xa500_0000
test_color:
  .dc32 0xff00ff
pixel_count:
  .dc32 640 * 240
display_sync_load:
  .dc32 POWERVR_SYNC_LOAD
display_sync_load_value:
  .dc32 (524 << 16) | 857
display_sync_width:
  .dc32 POWERVR_SYNC_WIDTH
display_sync_width_value:
  .dc32 (0x1f << 22) | (364 << 12) | (0x06 << 8) | 0x3f
hborder:
  .dc32 POWERVR_HBORDER
hborder_value:
  .dc32 (126 << 16) | 837
vborder:
  .dc32 POWERVR_VBORDER
vborder_value:
  .dc32 (36 << 16) | 516
hposition:
  .dc32 POWERVR_HPOS
hposition_value:
  .dc32 164
vposition:
  .dc32 POWERVR_VPOS
vposition_value:
  .dc32 (18 << 16) | 18
display_size:
  .dc32 POWERVR_FB_DISPLAY_SIZE
display_size_value:
  .dc32 (641 << 20) | (239 << 10) | 639
video_config:
  .dc32 POWERVR_VIDEO_CFG
video_config_value:
  .dc32 (0x16 << 16)
unknown_1:
  .dc32 0xa05f8110
unknown_1_value:
  .dc32 0x00093f39
unknown_2:
  .dc32 0xa05f8114
unknown_2_value:
  .dc32 0x00200000

tile_accelerator:
  ;; Point r1 to the tile accelerator registers.
  mov.l ta_registers, r1

  ;; Reset tile accelerator.
  mov.l ta_reset, r1
  mov #1, r2
  mov.l r2, @r1
  mov #0, r2
  mov.l r2, @r1

  ;; Set object pointer buffer start.
  mov.l ta_object_pointer_buffer_start, r1
  mov.l ta_object_pointer_buffer_start_value, r2
  mov.l r2, @r1

  ;; Set object pointer buffer end.
  mov.l ta_object_pointer_buffer_end, r1
  mov.l ta_object_pointer_buffer_end_value, r2
  mov.l r2, @r1

  ;; Set vertex buffer start.
  mov.l ta_vertex_buffer_start, r1
  mov.l ta_vertex_buffer_start_value, r2
  mov.l r2, @r1

  ;; Set vertex buffer end.
  mov.l ta_vertex_buffer_end, r1
  mov.l ta_vertex_buffer_end_value, r2
  mov.l r2, @r1

  ;; Set how many tiles are on the screen (width / height).
  mov.l ta_tile_buffer_control, r1
  mov.l ta_tile_buffer_control_value, r2
  mov.l r2, @r1

  ;; Set object pointer buffer init.
  mov.l ta_object_pointer_buffer_init, r1
  mov.l ta_object_pointer_buffer_init_value, r2
  mov.l r2, @r1

  ;; Tile accelerator configure OPB.
  ;; Bits 1-0 set to 2 is opaque polygons size_16 (15 object pointers).
  mov.l ta_opb_cfg, r1
  mov.l ta_opb_cfg_value, r2
  mov.l r2, @r1

  ;; Tile accelerator vertex registration init.
  mov.l ta_vertex_registration_init, r1
  mov.l ta_vertex_registration_init_value, r2
  mov.l r2, @r1

  ;; Start render (write anything to this register).
  mov.l ta_start_render, r1
  ;mov.l r2, @r1

  bra while_1
  nop

.align 32
ta_registers:
  .dc32 0xa05f8000
ta_reset:
  .dc32 0xa05f8008
ta_opb_cfg:
  .dc32 0xa05f8140
ta_opb_cfg_value:
  .dc32 0x0010_0002
ta_object_pointer_buffer_start:
  .dc32 0xa05f8124
ta_object_pointer_buffer_start_value:
  .dc32 0
ta_object_pointer_buffer_end:
  .dc32 0xa05f812c
ta_object_pointer_buffer_end_value:
  .dc32 0
ta_vertex_buffer_start:
  .dc32 0xa05f8128
ta_vertex_buffer_start_value:
  .dc32 0
ta_vertex_buffer_end:
  .dc32 0xa05f8130
ta_vertex_buffer_end_value:
  .dc32 0
ta_tile_buffer_control:
  .dc32 0xa05f813c
ta_tile_buffer_control_value:
  .dc32 ((TILE_HEIGHT - 1) << 16) | (TILE_WIDTH - 1)
ta_object_pointer_buffer_init:
  .dc32 0xa05f8164
ta_object_pointer_buffer_init_value:
  .dc32 0
ta_start_render:
  .dc32 0xa05f8014
ta_vertex_registration_init:
  .dc32 0xa05f8144
ta_vertex_registration_init_value:
  .dc32 0xa05f8144

while_1:
  bra while_1
  nop

