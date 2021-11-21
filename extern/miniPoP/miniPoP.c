/*
MiniPop, a barebones thread-safe version of SDLPop for routing
Copyright (C) 2021 Sergio Martin

based on

SDLPoP, a port/conversion of the DOS game Prince of Persia.
Copyright (C) 2013-2020  Dávid Nagy

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

The authors of this program may be contacted at https://forum.princed.org
*/

#pragma once
#include <SDL.h>
#include "dirent.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#define MAX_CACHED_FILES 2048
#define POP_MAX_PATH 256
#define POP_MAX_OPTIONS_SIZE 256

#if SDL_BYTEORDER != SDL_LIL_ENDIAN
  #error This program is not (yet) prepared for big endian CPUs, please contact the author.
#endif

// This macro is from SDL_types.h / SDL_stdinc.h .
// It used to be #undefined at the end of that file, but since some time in 2006 it's kept available.
// And SDL's definition changed in SDL 2.0.6, which caused a warning at this redefinition.
// So we should just use the macro from SDL and not define our own.
/* Make sure the types really have the right sizes */
/*
#define SDL_COMPILE_TIME_ASSERT(name, x)               \
       typedef int SDL_dummy_ ## name[(x) * 2 - 1]
*/

// "far" and "near" makes sense only for 16-bit
#define far
#define near
#define __pascal
#define malloc_near malloc
#define malloc_far malloc
#define free_near free
#define free_far free
#define memset_near memset
#define memset_far memset
#define memcpy_near memcpy
#define memcpy_far memcpy

typedef Uint8 byte;
typedef Sint8 sbyte;
typedef Uint16 word;
typedef Uint32 dword;

typedef struct rect_type
{
  short top, left, bottom, right;
} rect_type;

typedef struct piece
{
  byte base_id;
  byte floor_left;
  sbyte base_y;
  byte right_id;
  byte floor_right;
  sbyte right_y;
  byte stripe_id;
  byte topright_id;
  byte bottom_id;
  byte fore_id;
  byte fore_x;
  sbyte fore_y;
} piece;
typedef struct tile_and_mod
{
  byte tiletype;
  byte modifier;
} tile_and_mod;

typedef int __pascal far (*add_table_type)(short chtab_id, int id, sbyte xh, sbyte xl, int ybottom, int blit, byte peel);

typedef struct back_table_type
{
  sbyte xh;
  sbyte xl;
  short y;
  byte chtab_id;
  byte id;
  int blit;
} back_table_type;

typedef struct midtable_type
{
  sbyte xh;
  sbyte xl;
  short y;
  byte chtab_id;
  byte id;
  byte peel;
  rect_type clip;
  int blit;
} midtable_type;

typedef struct wipetable_type
{
  short left;
  short bottom;
  sbyte height;
  short width;
  sbyte color;
  sbyte layer;
} wipetable_type;

enum soundflags
{
  sfDigi = 1,
  sfMidi = 2,
  soundflags_4 = 4,
  sfLoop = 0x80
};

enum tiles
{
  tiles_0_empty = 0,
  tiles_1_floor = 1,
  tiles_2_spike = 2,
  tiles_3_pillar = 3,
  tiles_4_gate = 4,
  tiles_5_stuck = 5,
  tiles_6_closer = 6,             // a.k.a. drop button
  tiles_7_doortop_with_floor = 7, // a.k.a. tapestry
  tiles_8_bigpillar_bottom = 8,
  tiles_9_bigpillar_top = 9,
  tiles_10_potion = 10,
  tiles_11_loose = 11,
  tiles_12_doortop = 12, // a.k.a. tapestry top
  tiles_13_mirror = 13,
  tiles_14_debris = 14,          // a.k.a. broken floor
  tiles_15_opener = 15,          // a.k.a. raise button
  tiles_16_level_door_left = 16, // a.k.a. exit door
  tiles_17_level_door_right = 17,
  tiles_18_chomper = 18,
  tiles_19_torch = 19,
  tiles_20_wall = 20,
  tiles_21_skeleton = 21,
  tiles_22_sword = 22,
  tiles_23_balcony_left = 23,
  tiles_24_balcony_right = 24,
  tiles_25_lattice_pillar = 25,
  tiles_26_lattice_down = 26, // a.k.a. lattice support
  tiles_27_lattice_small = 27,
  tiles_28_lattice_left = 28,
  tiles_29_lattice_right = 29,
  tiles_30_torch_with_debris = 30
};

enum chtabs
{
  id_chtab_0_sword = 0,
  id_chtab_1_flameswordpotion = 1,
  id_chtab_2_kid = 2,
  id_chtab_3_princessinstory = 3,
  id_chtab_4_jaffarinstory_princessincutscenes = 4,
  id_chtab_5_guard = 5,
  id_chtab_6_environment = 6,
  id_chtab_7_environmentwall = 7,
  id_chtab_8_princessroom = 8,
  id_chtab_9_princessbed = 9
};

enum blitters
{
  blitters_0_no_transp = 0,
  // It seems to me that the "or" blitter can be safely replaced with the "transparent" blitter.
  blitters_2_or = 2,
  blitters_3_xor = 3, // used for the shadow
  blitters_white = 8,
  blitters_9_black = 9,
  blitters_10h_transp = 0x10,
  /* 0x40..0x4F will draw a monochrome image with color 0..15 */
  blitters_40h_mono = 0x40,
  blitters_46h_mono_6 = 0x46,  // used for palace wall patterns
  blitters_4Ch_mono_12 = 0x4C, // used for chomper blood
};

enum full_image_id
{
  TITLE_MAIN = 0,
  TITLE_PRESENTS,
  TITLE_GAME,
  TITLE_POP,
  TITLE_MECHNER,
  HOF_POP,
  STORY_FRAME,
  STORY_ABSENCE,
  STORY_MARRY,
  STORY_HAIL,
  STORY_CREDITS,
  MAX_FULL_IMAGES
};

enum grmodes
{
  gmCga = 1,
  gmHgaHerc = 2,
  gmEga = 3,
  gmTga = 4,
  gmMcgaVga = 5
};

enum sound_modes
{
  smAuto = 0,
  smAdlib = 1,
  smGblast = 2,
  smSblast = 3,
  smCovox = 4,
  smIbmg = 5,
  smTandy = 6
};

#pragma pack(push, 1)
typedef struct link_type
{
  byte left, right, up, down;
} link_type;

typedef struct level_type
{
  byte fg[720];
  byte bg[720];
  byte doorlinks1[256];
  byte doorlinks2[256];
  link_type roomlinks[24];
  byte used_rooms;
  byte roomxs[24];
  byte roomys[24];
  byte fill_1[15];
  byte start_room;
  byte start_pos;
  sbyte start_dir;
  byte fill_2[4];
  byte guards_tile[24];
  byte guards_dir[24];
  byte guards_x[24];
  byte guards_seq_lo[24];
  byte guards_skill[24];
  byte guards_seq_hi[24];
  byte guards_color[24];
  byte fill_3[18];
} level_type;
SDL_COMPILE_TIME_ASSERT(level_size, sizeof(level_type) == 2305);
#pragma pack(pop)

typedef struct image_type
{
 int h;
 int w;
} image_type;

typedef struct peel_type
{
  SDL_Surface *peel;
  rect_type rect;
} peel_type;

typedef struct chtab_type
{
  word n_images;
  word chtab_palette_bits;
  word has_palette_bits;
  // This is a variable-size array, with n_images elements.
  image_type *far images[];
} chtab_type;

typedef struct full_image_type
{
  int id;
  chtab_type **chtab;
  enum blitters blitter;
  int xpos, ypos;
} full_image_type;

#pragma pack(push, 1)
typedef struct rgb_type
{
  byte r, g, b;
} rgb_type;
typedef struct dat_pal_type
{
  word row_bits;
  byte n_colors;
  rgb_type vga[16];
  byte cga[16];
  byte ega[32];
} dat_pal_type;
typedef struct dat_shpl_type
{
  byte n_images;
  dat_pal_type palette;
} dat_shpl_type;
SDL_COMPILE_TIME_ASSERT(dat_shpl_size, sizeof(dat_shpl_type) == 100);
#pragma pack(pop)

typedef struct char_type
{
  byte frame;
  byte x;
  byte y;
  sbyte direction;
  sbyte curr_col;
  sbyte curr_row;
  byte action;
  sbyte fall_x;
  sbyte fall_y;
  byte room;
  byte repeat;
  byte charid;
  byte sword;
  sbyte alive;
  word curr_seq;
} char_type;

enum charids
{
  charid_0_kid = 0,
  charid_1_shadow = 1,
  charid_2_guard = 2,
  charid_3 = 3,
  charid_4_skeleton = 4,
  charid_5_princess = 5,
  charid_6_vizier = 6,
  charid_24_mouse = 0x18
};

enum sword_status
{
  sword_0_sheathed = 0,
  sword_2_drawn = 2
};

typedef struct auto_move_type
{
  short time, move;
} auto_move_type;

/* obj_type:
 0 = Kid, princess, vizier
 1 = shadow
 2 = Guard
 3 = sword
 4 = mirror image
 5 = hurt splash
 0x80 = loose floor
*/
typedef struct objtable_type
{
  sbyte xh;
  sbyte xl;
  short y;
  byte chtab_id;
  byte id;
  sbyte direction;
  byte obj_type;
  rect_type clip;
  byte tilepos;
} objtable_type;

typedef struct frame_type
{
  byte image;

  // 0x3F: sword image
  // 0xC0: chtab
  byte sword;

  sbyte dx;
  sbyte dy;

  // 0x1F: weight x
  // 0x20: thin
  // 0x40: needs floor
  // 0x80: even/odd pixel
  byte flags;
} frame_type;

enum frame_flags
{
  FRAME_WEIGHT_X = 0x1F,
  FRAME_THIN = 0x20,
  FRAME_NEEDS_FLOOR = 0x40,
  FRAME_EVEN_ODD_PIXEL = 0x80,
};

typedef struct trob_type
{
  byte tilepos;
  byte room;
  sbyte type;
} trob_type;

typedef struct mob_type
{
  byte xh;
  byte y;
  byte room;
  sbyte speed;
  byte type;
  byte row;
} mob_type;

enum directions
{
  dir_0_right = 0x00,
  dir_56_none = 0x56,
  dir_FF_left = -1
};

enum actions
{
  actions_0_stand = 0,
  actions_1_run_jump = 1,
  actions_2_hang_climb = 2,
  actions_3_in_midair = 3,
  actions_4_in_freefall = 4,
  actions_5_bumped = 5,
  actions_6_hang_straight = 6,
  actions_7_turn = 7,
  actions_99_hurt = 99,
};

typedef struct sword_table_type
{
  byte id;
  sbyte x;
  sbyte y;
} sword_table_type;

#pragma pack(push, 1)
typedef struct dat_header_type
{
  Uint32 table_offset;
  Uint16 table_size;
} dat_header_type;
SDL_COMPILE_TIME_ASSERT(dat_header_size, sizeof(dat_header_type) == 6);

typedef struct dat_res_type
{
  Uint16 id;
  Uint32 offset;
  Uint16 size;
} dat_res_type;
SDL_COMPILE_TIME_ASSERT(dat_res_size, sizeof(dat_res_type) == 8);

typedef struct dat_table_type
{
  Uint16 res_count;
  dat_res_type entries[];
} dat_table_type;
SDL_COMPILE_TIME_ASSERT(dat_table_size, sizeof(dat_table_type) == 2);

typedef struct image_data_type
{
  Uint16 height;
  Uint16 width;
  Uint16 flags;
  byte data[];
} image_data_type;
SDL_COMPILE_TIME_ASSERT(image_data_size, sizeof(image_data_type) == 6);
#pragma pack(pop)

typedef struct dat_type
{
  struct dat_type *next_dat;
  FILE *handle;
  char filename[POP_MAX_PATH];
  dat_table_type *dat_table;
  // handle and dat_table are NULL if the DAT is a directory.
} dat_type;

typedef void __pascal far (*cutscene_ptr_type)();

#ifdef USE_FADE
typedef struct palette_fade_type
{
  word which_rows;
  word wait_time;
  word fade_pos;
  rgb_type original_pal[256];
  rgb_type faded_pal[256];
  int __pascal far (*proc_fade_frame)(struct palette_fade_type far *palette_buffer);
  void __pascal far (*proc_restore_free)(struct palette_fade_type far *palette_buffer);
} palette_fade_type;
#endif

#ifndef O_BINARY
  #define O_BINARY 0
#endif

#ifdef USE_TEXT
typedef struct font_type
{
  byte first_char;
  byte last_char;
  short height_above_baseline;
  short height_below_baseline;
  short space_between_lines;
  short space_between_chars;
  chtab_type *chtab;
} font_type;

typedef struct textstate_type
{
  short current_x;
  short current_y;
  short textblit;
  short textcolor;
  font_type *ptr_font;
} textstate_type;

  #pragma pack(push, 1)
typedef struct rawfont_type
{
  byte first_char;
  byte last_char;
  short height_above_baseline;
  short height_below_baseline;
  short space_between_lines;
  short space_between_chars;
  word offsets[];
} rawfont_type;
SDL_COMPILE_TIME_ASSERT(rawfont_type, sizeof(rawfont_type) == 10);
  #pragma pack(pop)

#endif

typedef enum data_location
{
  data_none = 0,
  data_DAT = 1,
  data_directory = 2
} data_location;

enum sound_type
{
  sound_speaker = 0,
  sound_digi = 1,
  sound_midi = 2,
  sound_chunk = 3,
  sound_music = 4,
  sound_digi_converted = 6,
};
#pragma pack(push, 1)
typedef struct note_type
{
  word frequency; // 0x00 or 0x01 = rest, 0x12 = end
  byte length;
} note_type;
SDL_COMPILE_TIME_ASSERT(note_type, sizeof(note_type) == 3);
typedef struct speaker_type
{ // IBM
  word tempo;
  note_type notes[];
} speaker_type;
SDL_COMPILE_TIME_ASSERT(speaker_type, sizeof(speaker_type) == 2);

typedef struct digi_type
{ // wave in 1.0 and 1.1
  word sample_rate;
  word sample_count;
  word unknown;
  byte sample_size; // =8
  byte samples[];
} digi_type;
SDL_COMPILE_TIME_ASSERT(digi_type, sizeof(digi_type) == 7);

typedef struct digi_new_type
{ // wave in 1.3 and 1.4 (and PoP2)
  word sample_rate;
  byte sample_size; // =8
  word sample_count;
  word unknown;
  word unknown2;
  byte samples[];
} digi_new_type;
SDL_COMPILE_TIME_ASSERT(digi_new_type, sizeof(digi_new_type) == 9);

typedef struct midi_type
{
  char chunk_type[4];
  dword chunk_length;
  union
  {
    struct
    {
      word format;
      word num_tracks;
      word delta;
      byte tracks[0];
    } header;
    byte data[0];
  };

} midi_type;

typedef struct converted_audio_type
{
  int length;
  short samples[];
} converted_audio_type;

typedef struct sound_buffer_type
{
  byte type;
  union
  {
    speaker_type speaker;
    digi_type digi;
    digi_new_type digi_new;
    midi_type midi;
    converted_audio_type converted;
  };
} sound_buffer_type;

typedef struct midi_raw_chunk_type
{
  char chunk_type[4];
  dword chunk_length;
  union
  {
    struct
    {
      word format;
      word num_tracks;
      word time_division;
      byte tracks[0];
    } header;
    byte data[0];
  };

} midi_raw_chunk_type;

typedef struct midi_event_type
{
  dword delta_time;
  byte event_type;
  union
  {
    struct
    {
      byte channel;
      byte param1;
      byte param2;
    } channel;
    struct
    {
      dword length;
      byte *data;
    } sysex;
    struct
    {
      byte type;
      dword length;
      byte *data;
    } meta;
  };

} midi_event_type;

typedef struct midi_track_type
{
  dword size;
  int num_events;
  midi_event_type *events;
  int event_index;
  int64_t next_pause_tick;
} midi_track_type;

typedef struct parsed_midi_type
{
  int num_tracks;
  midi_track_type *tracks;
  dword ticks_per_beat;
} parsed_midi_type;

#pragma pack(push, 1)
typedef struct operator_type
{
  byte mul;
  byte ksl_tl;
  byte a_d;
  byte s_r;
  byte waveform;
} operator_type;

typedef struct instrument_type
{
  byte blocknum_low;
  byte blocknum_high;
  byte FB_conn;
  operator_type operators[2];
  byte percussion;
  byte unknown[2];
} instrument_type;
#pragma pack(pop)

struct dialog_type; // (declaration only)
typedef struct dialog_settings_type
{
  void (*method_1)(struct dialog_type *dialog);
  void (*method_2_frame)(struct dialog_type *dialog);
  short top_border;
  short left_border;
  short bottom_border;
  short right_border;
  short shadow_bottom;
  short shadow_right;
  short outer_border;
} dialog_settings_type;

typedef struct dialog_type
{
  dialog_settings_type *settings;
  rect_type text_rect;
  rect_type peel_rect;
  word has_peel;
  peel_type *peel;
} dialog_type;

#pragma pack(pop)

enum soundids
{
  sound_0_fell_to_death = 0,
  sound_1_falling = 1,
  sound_2_tile_crashing = 2,
  sound_3_button_pressed = 3,
  sound_4_gate_closing = 4,
  sound_5_gate_opening = 5,
  sound_6_gate_closing_fast = 6,
  sound_7_gate_stop = 7,
  sound_8_bumped = 8,
  sound_9_grab = 9,
  sound_10_sword_vs_sword = 10,
  sound_11_sword_moving = 11,
  sound_12_guard_hurt = 12,
  sound_13_kid_hurt = 13,
  sound_14_leveldoor_closing = 14,
  sound_15_leveldoor_sliding = 15,
  sound_16_medium_land = 16,
  sound_17_soft_land = 17,
  sound_18_drink = 18,
  sound_19_draw_sword = 19,
  sound_20_loose_shake_1 = 20,
  sound_21_loose_shake_2 = 21,
  sound_22_loose_shake_3 = 22,
  sound_23_footstep = 23,
  sound_24_death_regular = 24,
  sound_25_presentation = 25,
  sound_26_embrace = 26,
  sound_27_cutscene_2_4_6_12 = 27,
  sound_28_death_in_fight = 28,
  sound_29_meet_Jaffar = 29,
  sound_30_big_potion = 30,
  //sound_31 = 31,
  sound_32_shadow_music = 32,
  sound_33_small_potion = 33,
  //sound_34 = 34,
  sound_35_cutscene_8_9 = 35,
  sound_36_out_of_time = 36,
  sound_37_victory = 37,
  sound_38_blink = 38,
  sound_39_low_weight = 39,
  sound_40_cutscene_12_short_time = 40,
  sound_41_end_level_music = 41,
  //sound_42 = 42,
  sound_43_victory_Jaffar = 43,
  sound_44_skel_alive = 44,
  sound_45_jump_through_mirror = 45,
  sound_46_chomped = 46,
  sound_47_chomper = 47,
  sound_48_spiked = 48,
  sound_49_spikes = 49,
  sound_50_story_2_princess = 50,
  sound_51_princess_door_opening = 51,
  sound_52_story_4_Jaffar_leaves = 52,
  sound_53_story_3_Jaffar_comes = 53,
  sound_54_intro_music = 54,
  sound_55_story_1_absence = 55,
  sound_56_ending_music = 56,
};

#define NUM_TIMERS 3
enum timerids
{
  timer_0 = 0,
  timer_1 = 1,
  timer_2 = 2,
};

enum frameids
{
  frame_0 = 0,
  frame_1_start_run = 1,
  frame_2_start_run = 2,
  frame_3_start_run = 3,
  frame_4_start_run = 4,
  frame_5_start_run = 5,
  frame_6_start_run = 6,
  frame_7_run = 7,
  frame_8_run = 8,
  frame_9_run = 9,
  frame_10_run = 10,
  frame_11_run = 11,
  frame_12_run = 12,
  frame_13_run = 13,
  frame_14_run = 14,
  frame_15_stand = 15,
  frame_16_standing_jump_1 = 16,
  frame_17_standing_jump_2 = 17,
  frame_18_standing_jump_3 = 18,
  frame_19_standing_jump_4 = 19,
  frame_20_standing_jump_5 = 20,
  frame_21_standing_jump_6 = 21,
  frame_22_standing_jump_7 = 22,
  frame_23_standing_jump_8 = 23,
  frame_24_standing_jump_9 = 24,
  frame_25_standing_jump_10 = 25,
  frame_26_standing_jump_11 = 26,
  frame_27_standing_jump_12 = 27,
  frame_28_standing_jump_13 = 28,
  frame_29_standing_jump_14 = 29,
  frame_30_standing_jump_15 = 30,
  frame_31_standing_jump_16 = 31,
  frame_32_standing_jump_17 = 32,
  frame_33_standing_jump_18 = 33,
  frame_34_start_run_jump_1 = 34,
  frame_35_start_run_jump_2 = 35,
  frame_36_start_run_jump_3 = 36,
  frame_37_start_run_jump_4 = 37,
  frame_38_start_run_jump_5 = 38,
  frame_39_start_run_jump_6 = 39,
  frame_40_running_jump_1 = 40,
  frame_41_running_jump_2 = 41,
  frame_42_running_jump_3 = 42,
  frame_43_running_jump_4 = 43,
  frame_44_running_jump_5 = 44,
  frame_45_turn = 45,
  frame_46_turn = 46,
  frame_47_turn = 47,
  frame_48_turn = 48,
  frame_49_turn = 49,
  frame_50_turn = 50,
  frame_51_turn = 51,
  frame_52_turn = 52,
  frame_53_runturn = 53,
  frame_54_runturn = 54,
  frame_55_runturn = 55,
  frame_56_runturn = 56,
  frame_57_runturn = 57,
  frame_58_runturn = 58,
  frame_59_runturn = 59,
  frame_60_runturn = 60,
  frame_61_runturn = 61,
  frame_62_runturn = 62,
  frame_63_runturn = 63,
  frame_64_runturn = 64,
  frame_65_runturn = 65,
  frame_67_start_jump_up_1 = 67,
  frame_68_start_jump_up_2 = 68,
  frame_69_start_jump_up_3 = 69,
  frame_70_jumphang = 70,
  frame_71_jumphang = 71,
  frame_72_jumphang = 72,
  frame_73_jumphang = 73,
  frame_74_jumphang = 74,
  frame_75_jumphang = 75,
  frame_76_jumphang = 76,
  frame_77_jumphang = 77,
  frame_78_jumphang = 78,
  frame_79_jumphang = 79,
  frame_80_jumphang = 80,
  frame_81_hangdrop_1 = 81,
  frame_82_hangdrop_2 = 82,
  frame_83_hangdrop_3 = 83,
  frame_84_hangdrop_4 = 84,
  frame_85_hangdrop_5 = 85,
  frame_86_test_foot = 86,
  frame_87_hanging_1 = 87,
  frame_88_hanging_2 = 88,
  frame_89_hanging_3 = 89,
  frame_90_hanging_4 = 90,
  frame_91_hanging_5 = 91,
  frame_92_hanging_6 = 92,
  frame_93_hanging_7 = 93,
  frame_94_hanging_8 = 94,
  frame_95_hanging_9 = 95,
  frame_96_hanging_10 = 96,
  frame_97_hanging_11 = 97,
  frame_98_hanging_12 = 98,
  frame_99_hanging_13 = 99,
  frame_102_start_fall_1 = 102,
  frame_103_start_fall_2 = 103,
  frame_104_start_fall_3 = 104,
  frame_105_start_fall_4 = 105,
  frame_106_fall = 106,
  frame_107_fall_land_1 = 107,
  frame_108_fall_land_2 = 108,
  frame_109_crouch = 109,
  frame_110_stand_up_from_crouch_1 = 110,
  frame_111_stand_up_from_crouch_2 = 111,
  frame_112_stand_up_from_crouch_3 = 112,
  frame_113_stand_up_from_crouch_4 = 113,
  frame_114_stand_up_from_crouch_5 = 114,
  frame_115_stand_up_from_crouch_6 = 115,
  frame_116_stand_up_from_crouch_7 = 116,
  frame_117_stand_up_from_crouch_8 = 117,
  frame_118_stand_up_from_crouch_9 = 118,
  frame_119_stand_up_from_crouch_10 = 119,
  frame_121_stepping_1 = 121,
  frame_122_stepping_2 = 122,
  frame_123_stepping_3 = 123,
  frame_124_stepping_4 = 124,
  frame_125_stepping_5 = 125,
  frame_126_stepping_6 = 126,
  frame_127_stepping_7 = 127,
  frame_128_stepping_8 = 128,
  frame_129_stepping_9 = 129,
  frame_130_stepping_10 = 130,
  frame_131_stepping_11 = 131,
  frame_132_stepping_12 = 132,
  frame_133_sheathe = 133,
  frame_134_sheathe = 134,
  frame_135_climbing_1 = 135,
  frame_136_climbing_2 = 136,
  frame_137_climbing_3 = 137,
  frame_138_climbing_4 = 138,
  frame_139_climbing_5 = 139,
  frame_140_climbing_6 = 140,
  frame_141_climbing_7 = 141,
  frame_142_climbing_8 = 142,
  frame_143_climbing_9 = 143,
  frame_144_climbing_10 = 144,
  frame_145_climbing_11 = 145,
  frame_146_climbing_12 = 146,
  frame_147_climbing_13 = 147,
  frame_148_climbing_14 = 148,
  frame_149_climbing_15 = 149,
  frame_150_parry = 150,
  frame_151_strike_1 = 151,
  frame_152_strike_2 = 152,
  frame_153_strike_3 = 153,
  frame_154_poking = 154,
  frame_155_guy_7 = 155,
  frame_156_guy_8 = 156,
  frame_157_walk_with_sword = 157,
  frame_158_stand_with_sword = 158,
  frame_159_fighting = 159,
  frame_160_fighting = 160,
  frame_161_parry = 161,
  frame_162_block_to_strike = 162,
  frame_163_fighting = 163,
  frame_164_fighting = 164,
  frame_165_walk_with_sword = 165,
  frame_166_stand_inactive = 166,
  frame_167_blocked = 167,
  frame_168_back = 168,
  frame_169_begin_block = 169,
  frame_170_stand_with_sword = 170,
  frame_171_stand_with_sword = 171,
  frame_172_jumpfall_2 = 172,
  frame_173_jumpfall_3 = 173,
  frame_174_jumpfall_4 = 174,
  frame_175_jumpfall_5 = 175,
  frame_177_spiked = 177,
  frame_178_chomped = 178,
  frame_179_collapse_1 = 179,
  frame_180_collapse_2 = 180,
  frame_181_collapse_3 = 181,
  frame_182_collapse_4 = 182,
  frame_183_collapse_5 = 183,
  frame_185_dead = 185,
  frame_186_mouse_1 = 186,
  frame_187_mouse_2 = 187,
  frame_188_mouse_stand = 188,
  frame_191_drink = 191,
  frame_192_drink = 192,
  frame_193_drink = 193,
  frame_194_drink = 194,
  frame_195_drink = 195,
  frame_196_drink = 196,
  frame_197_drink = 197,
  frame_198_drink = 198,
  frame_199_drink = 199,
  frame_200_drink = 200,
  frame_201_drink = 201,
  frame_202_drink = 202,
  frame_203_drink = 203,
  frame_204_drink = 204,
  frame_205_drink = 205,
  frame_207_draw_1 = 207,
  frame_208_draw_2 = 208,
  frame_209_draw_3 = 209,
  frame_210_draw_4 = 210,
  frame_217_exit_stairs_1 = 217,
  frame_218_exit_stairs_2 = 218,
  frame_219_exit_stairs_3 = 219,
  frame_220_exit_stairs_4 = 220,
  frame_221_exit_stairs_5 = 221,
  frame_222_exit_stairs_6 = 222,
  frame_223_exit_stairs_7 = 223,
  frame_224_exit_stairs_8 = 224,
  frame_225_exit_stairs_9 = 225,
  frame_226_exit_stairs_10 = 226,
  frame_227_exit_stairs_11 = 227,
  frame_228_exit_stairs_12 = 228,
  frame_229_found_sword = 229,
  frame_230_sheathe = 230,
  frame_231_sheathe = 231,
  frame_232_sheathe = 232,
  frame_233_sheathe = 233,
  frame_234_sheathe = 234,
  frame_235_sheathe = 235,
  frame_236_sheathe = 236,
  frame_237_sheathe = 237,
  frame_238_sheathe = 238,
  frame_239_sheathe = 239,
  frame_240_sheathe = 240,
};

enum altset2ids
{
  alt2frame_54_Vstand = 54,
  //... incomplete
};

enum seqids
{
  seq_1_start_run = 1,
  seq_2_stand = 2,
  seq_3_standing_jump = 3,
  seq_4_run_jump = 4,
  seq_5_turn = 5,
  seq_6_run_turn = 6,
  seq_7_fall = 7,
  seq_8_jump_up_and_grab_straight = 8,
  seq_10_climb_up = 10,
  seq_11_release_ledge_and_land = 11,
  seq_13_stop_run = 13,
  seq_14_jump_up_into_ceiling = 14,
  seq_15_grab_ledge_midair = 15,
  seq_16_jump_up_and_grab = 16,
  seq_17_soft_land = 17,
  seq_18_fall_after_standing_jump = 18,
  seq_19_fall = 19,
  seq_20_medium_land = 20,
  seq_21_fall_after_running_jump = 21,
  seq_22_crushed = 22,
  seq_23_release_ledge_and_fall = 23,
  seq_24_jump_up_and_grab_forward = 24,
  seq_25_hang_against_wall = 25,
  seq_26_crouch_while_running = 26,
  seq_28_jump_up_with_nothing_above = 28,
  seq_29_safe_step_1 = 29,
  seq_30_safe_step_2 = 30,
  seq_31_safe_step_3 = 31,
  seq_32_safe_step_4 = 32,
  seq_33_safe_step_5 = 33,
  seq_34_safe_step_6 = 34,
  seq_35_safe_step_7 = 35,
  seq_36_safe_step_8 = 36,
  seq_37_safe_step_9 = 37,
  seq_38_safe_step_10 = 38,
  seq_39_safe_step_11 = 39,
  seq_40_safe_step_12 = 40,
  seq_41_safe_step_13 = 41,
  seq_42_safe_step_14 = 42,
  seq_43_start_run_after_turn = 43,
  seq_44_step_on_edge = 44,
  seq_45_bumpfall = 45,
  seq_46_hardbump = 46,
  seq_47_bump = 47,
  seq_49_stand_up_from_crouch = 49,
  seq_50_crouch = 50,
  seq_51_spiked = 51,
  seq_52_loose_floor_fell_on_kid = 52,
  seq_54_chomped = 54,
  seq_55_draw_sword = 55,
  seq_56_guard_forward_with_sword = 56,
  seq_57_back_with_sword = 57,
  seq_58_guard_strike = 58,
  seq_60_turn_with_sword = 60,
  seq_61_parry_after_strike = 61,
  seq_62_parry = 62,
  seq_63_guard_stand_active = 63,
  seq_64_pushed_back_with_sword = 64,
  seq_65_bump_forward_with_sword = 65,
  seq_66_strike_after_parry = 66,
  seq_68_climb_down = 68,
  seq_69_attack_was_parried = 69,
  seq_70_go_up_on_level_door = 70,
  seq_71_dying = 71,
  seq_73_climb_up_to_closed_gate = 73,
  seq_74_hit_by_sword = 74,
  seq_75_strike = 75,
  seq_77_guard_stand_inactive = 77,
  seq_78_drink = 78,
  seq_79_crouch_hop = 79,
  seq_80_stand_flipped = 80,
  seq_81_kid_pushed_off_ledge = 81,
  seq_82_guard_pushed_off_ledge = 82,
  seq_83_guard_fall = 83,
  seq_84_run = 84,
  seq_85_stabbed_to_death = 85,
  seq_86_forward_with_sword = 86,
  seq_87_guard_become_inactive = 87,
  seq_88_skel_wake_up = 88,
  seq_89_turn_draw_sword = 89,
  seq_90_en_garde = 90,
  seq_91_get_sword = 91,
  seq_92_put_sword_away = 92,
  seq_93_put_sword_away_fast = 93,
  seq_94_princess_stand_PV1 = 94,
  seq_95_Jaffar_stand_PV1 = 95,
  seq_101_mouse_stands_up = 101,
  seq_103_princess_lying_PV2 = 103,
  seq_104_start_fall_in_front_of_wall = 104,
  seq_105_mouse_forward = 105,
  seq_106_mouse = 106,
  seq_107_mouse_stand_up_and_go = 107,
  seq_108_princess_turn_and_hug = 108,
  seq_109_princess_stand_PV2 = 109,
  seq_110_princess_crouching_PV2 = 110,
  seq_111_princess_stand_up_PV2 = 111,
  seq_112_princess_crouch_down_PV2 = 112,
  seq_114_mouse_stand = 114,
};

enum seqtbl_instructions
{
  SEQ_DX = 0xFB,
  SEQ_DY = 0xFA,
  SEQ_FLIP = 0xFE,
  SEQ_JMP_IF_FEATHER = 0xF7,
  SEQ_JMP = 0xFF,
  SEQ_UP = 0xFD,
  SEQ_DOWN = 0xFC,
  SEQ_ACTION = 0xF9,
  SEQ_SET_FALL = 0xF8,
  SEQ_KNOCK_UP = 0xF5,
  SEQ_KNOCK_DOWN = 0xF4,
  SEQ_SOUND = 0xF2,
  SEQ_END_LEVEL = 0xF1,
  SEQ_GET_ITEM = 0xF3,
  SEQ_DIE = 0xF6,
};

enum seqtbl_sounds
{
  SND_SILENT = 0,
  SND_FOOTSTEP = 1,
  SND_BUMP = 2,
  SND_DRINK = 3,
  SND_LEVEL = 4,
};

enum colorids
{
  color_0_black = 0,
  color_1_blue = 1,
  color_2_green = 2,
  color_3_cyan = 3,
  color_4_red = 4,
  color_5_magenta = 5,
  color_6_brown = 6,
  color_7_lightgray = 7,
  color_8_darkgray = 8,
  color_9_brightblue = 9,
  color_10_brightgreen = 10,
  color_11_brightcyan = 11,
  color_12_brightred = 12,
  color_13_brightmagenta = 13,
  color_14_brightyellow = 14,
  color_15_brightwhite = 15,
};

#ifdef USE_REPLAY
enum replay_special_moves
{
  MOVE_RESTART_LEVEL = 1, // player pressed Ctrl+A
  MOVE_EFFECT_END = 2,    // music stops, causing the end of feather effect or level 1 crouch immobilization
};

enum replay_seek_targets
{
  replay_seek_0_next_room = 0,
  replay_seek_1_next_level = 1,
  replay_seek_2_end = 2,
};
#endif

#define COUNT(array) (sizeof(array) / sizeof(array[0]))

// These are or'ed with SDL_SCANCODE_* constants in last_key_scancode.
enum key_modifiers
{
  WITH_SHIFT = 0x8000,
  WITH_CTRL = 0x4000,
  WITH_ALT = 0x2000,
};

#define MAX_OPTION_VALUE_NAME_LENGTH 20
typedef struct key_value_type
{
  char key[MAX_OPTION_VALUE_NAME_LENGTH];
  int value;
} key_value_type;

typedef struct names_list_type
{
  byte type; // 0 = names list, 1 = key/value pair list
  union
  {
    struct
    {
      const char (*data)[][MAX_OPTION_VALUE_NAME_LENGTH];
      word count;
    } names;
    struct
    {
      key_value_type *data;
      word count;
    } kv_pairs;
  };
} names_list_type;

// Macros for declaring and initializing a names_list_type (for names lists and key/value pair lists).
#define NAMES_LIST(listname, ...)                                    \
  const char listname[][MAX_OPTION_VALUE_NAME_LENGTH] = __VA_ARGS__; \
  names_list_type listname##_list = {.type = 0, .names = {&listname, COUNT(listname)}}

#define KEY_VALUE_LIST(listname, ...)            \
  const key_value_type listname[] = __VA_ARGS__; \
  names_list_type listname##_list = {.type = 1, .kv_pairs = {(key_value_type *)&listname, COUNT(listname)}}

#pragma pack(push, 1)
typedef struct fixes_options_type
{
  byte enable_crouch_after_climbing;
  byte enable_freeze_time_during_end_music;
  byte enable_remember_guard_hp;
  byte fix_gate_sounds;
  byte fix_two_coll_bug;
  byte fix_infinite_down_bug;
  byte fix_gate_drawing_bug;
  byte fix_bigpillar_climb;
  byte fix_jump_distance_at_edge;
  byte fix_edge_distance_check_when_climbing;
  byte fix_painless_fall_on_guard;
  byte fix_wall_bump_triggers_tile_below;
  byte fix_stand_on_thin_air;
  byte fix_press_through_closed_gates;
  byte fix_grab_falling_speed;
  byte fix_skeleton_chomper_blood;
  byte fix_move_after_drink;
  byte fix_loose_left_of_potion;
  byte fix_guard_following_through_closed_gates;
  byte fix_safe_landing_on_spikes;
  byte fix_glide_through_wall;
  byte fix_drop_through_tapestry;
  byte fix_land_against_gate_or_tapestry;
  byte fix_unintended_sword_strike;
  byte fix_retreat_without_leaving_room;
  byte fix_running_jump_through_tapestry;
  byte fix_push_guard_into_wall;
  byte fix_jump_through_wall_above_gate;
  byte fix_chompers_not_starting;
  byte fix_feather_interrupted_by_leveldoor;
  byte fix_offscreen_guards_disappearing;
  byte fix_move_after_sheathe;
  byte fix_hidden_floors_during_flashing;
  byte fix_hang_on_teleport;
  byte fix_exit_door;
  byte fix_quicksave_during_feather;
  byte fix_quicksave_during_lvl1_music;
} fixes_options_type;

#define NUM_GUARD_SKILLS 12

typedef struct custom_options_type
{
  word start_minutes_left;
  word start_ticks_left;
  word start_hitp;
  word max_hitp_allowed;
  word saving_allowed_first_level;
  word saving_allowed_last_level;
  byte start_upside_down;
  byte start_in_blind_mode;
  word copyprot_level;
  byte drawn_tile_top_level_edge;
  byte drawn_tile_left_level_edge;
  byte level_edge_hit_tile;
  byte allow_triggering_any_tile;
  byte enable_wda_in_palace;
  rgb_type vga_palette[16];
  word first_level;
  byte skip_title;
  word shift_L_allowed_until_level;
  word shift_L_reduced_minutes;
  word shift_L_reduced_ticks;
  word demo_hitp;
  word demo_end_room;
  word intro_music_level;
  word intro_music_time_initial;
  word intro_music_time_restart;
  word have_sword_from_level;
  word checkpoint_level;
  sbyte checkpoint_respawn_dir;
  byte checkpoint_respawn_room;
  byte checkpoint_respawn_tilepos;
  byte checkpoint_clear_tile_room;
  byte checkpoint_clear_tile_col;
  byte checkpoint_clear_tile_row;
  word skeleton_level;
  byte skeleton_room;
  byte skeleton_trigger_column_1;
  byte skeleton_trigger_column_2;
  byte skeleton_column;
  byte skeleton_row;
  byte skeleton_require_open_level_door;
  byte skeleton_skill;
  byte skeleton_reappear_room;
  byte skeleton_reappear_x;
  byte skeleton_reappear_row;
  byte skeleton_reappear_dir;
  word mirror_level;
  byte mirror_room;
  byte mirror_column;
  byte mirror_row;
  byte mirror_tile;
  byte show_mirror_image;
  word falling_exit_level;
  byte falling_exit_room;
  word falling_entry_level;
  byte falling_entry_room;
  word mouse_level;
  byte mouse_room;
  word mouse_delay;
  byte mouse_object;
  byte mouse_start_x;
  word loose_tiles_level;
  byte loose_tiles_room_1;
  byte loose_tiles_room_2;
  byte loose_tiles_first_tile;
  byte loose_tiles_last_tile;
  word jaffar_victory_level;
  byte jaffar_victory_flash_time;
  word hide_level_number_from_level;
  byte level_13_level_number;
  word victory_stops_time_level;
  word win_level;
  byte win_room;
  byte loose_floor_delay;
  byte tbl_level_type[16];
  word tbl_level_color[16];
  short tbl_guard_type[16];
  byte tbl_guard_hp[16];
  byte tbl_cutscenes_by_index[16];
  byte tbl_entry_pose[16];
  sbyte tbl_seamless_exit[16];

  // guard skills
  word strikeprob[NUM_GUARD_SKILLS];
  word restrikeprob[NUM_GUARD_SKILLS];
  word blockprob[NUM_GUARD_SKILLS];
  word impblockprob[NUM_GUARD_SKILLS];
  word advprob[NUM_GUARD_SKILLS];
  word refractimer[NUM_GUARD_SKILLS];
  word extrastrength[NUM_GUARD_SKILLS];

  // shadow's starting positions
  byte init_shad_6[8];
  byte init_shad_5[8];
  byte init_shad_12[8];
  // automatic moves
  auto_move_type demo_moves[25];     // prince on demo level
  auto_move_type shad_drink_move[8]; // shadow on level 5

  // speeds
  byte base_speed;
  byte fight_speed;
  byte chomper_speed;

} custom_options_type;
#pragma pack(pop)

typedef struct directory_listing_type directory_listing_type;

#define BASE_FPS 60

#define FEATHER_FALL_LENGTH 18.75

struct directory_listing_type
{
  DIR *dp;
  char *found_filename;
  const char *extension;
};

#define VGA_PALETTE_DEFAULT \
  {                         \
    {0x00, 0x00, 0x00},     \
      {0x00, 0x00, 0x2A},   \
      {0x00, 0x2A, 0x00},   \
      {0x00, 0x2A, 0x2A},   \
      {0x2A, 0x00, 0x00},   \
      {0x2A, 0x00, 0x2A},   \
      {0x2A, 0x15, 0x00},   \
      {0x2A, 0x2A, 0x2A},   \
      {0x15, 0x15, 0x15},   \
      {0x15, 0x15, 0x3F},   \
      {0x15, 0x3F, 0x15},   \
      {0x15, 0x3F, 0x3F},   \
      {0x3F, 0x15, 0x15},   \
      {0x3F, 0x15, 0x3F},   \
      {0x3F, 0x3F, 0x15},   \
      {0x3F, 0x3F, 0x3F},   \
  }

#define backtable_count table_counts[0]
#define foretable_count table_counts[1]
#define wipetable_count table_counts[2]
#define midtable_count table_counts[3]
#define objtable_count table_counts[4]


#define SEQTBL_BASE 0x196E
#define SEQTBL_0 (seqtbl - SEQTBL_BASE)

// This expands a two-byte number into two comma-separated bytes, used for the JMP destinations
#define DW(data_word) (data_word) & 0x00FF, (((data_word)&0xFF00) >> 8)

// Shorter notation for the sequence table instructions
#define act(action) SEQ_ACTION, action
#define jmp(dest) SEQ_JMP, DW(dest)
#define jmp_if_feather(dest) SEQ_JMP_IF_FEATHER, DW(dest)
#define dx(amount) SEQ_DX, (byte)amount
#define dy(amount) SEQ_DY, (byte)amount
#define snd(sound) SEQ_SOUND, sound
#define set_fall(x, y) SEQ_SET_FALL, (byte)x, (byte)y

// This splits the byte array into labeled "sections" that are packed tightly next to each other
// However, it only seems to work correctly in the Debug configuration...
//#define LABEL(label) }; const byte label##_eventual_ptr[] __attribute__ ((aligned(1))) = {
#define LABEL(label) // disable
//#define OFFSET(label) label - seqtbl + SEQTBL_BASE

// Labels
#define running SEQTBL_BASE                // 0x196E
#define startrun 5 + running               //SEQTBL_BASE + 5     // 0x1973
#define runstt1 2 + startrun               //SEQTBL_BASE + 7     // 0x1975
#define runstt4 3 + runstt1                //SEQTBL_BASE + 10    // 0x1978
#define runcyc1 9 + runstt4                //SEQTBL_BASE + 19    // 0x1981
#define runcyc7 20 + runcyc1               //SEQTBL_BASE + 39    // 0x1995
#define stand 11 + runcyc7                 //SEQTBL_BASE + 50    // 0x19A0
#define goalertstand 6 + stand             //SEQTBL_BASE + 56    // 0x19A6
#define alertstand 2 + goalertstand        //SEQTBL_BASE + 58    // 0x19A8
#define arise 4 + alertstand               //SEQTBL_BASE + 62    // 0x19AC
#define guardengarde 21 + arise            //SEQTBL_BASE + 83    // 0x19C1
#define engarde 3 + guardengarde           //SEQTBL_BASE + 86    // 0x19C4
#define ready 14 + engarde                 //SEQTBL_BASE + 100   // 0x19D2
#define ready_loop 6 + ready               //SEQTBL_BASE + 106   // 0x19D8
#define stabbed 4 + ready_loop             //SEQTBL_BASE + 110   // 0x19DC
#define strikeadv 29 + stabbed             //SEQTBL_BASE + 139   // 0x19F9
#define strikeret 14 + strikeadv           //SEQTBL_BASE + 153   // 0x1A07
#define advance 12 + strikeret             //SEQTBL_BASE + 165   // 0x1A13
#define fastadvance 15 + advance           //SEQTBL_BASE + 180   // 0x1A22
#define retreat 12 + fastadvance           //SEQTBL_BASE + 192   // 0x1A2E
#define strike 14 + retreat                //SEQTBL_BASE + 206   // 0x1A3C
#define faststrike 6 + strike              //SEQTBL_BASE + 212   // 0x1A42
#define guy4 3 + faststrike                //SEQTBL_BASE + 215   // 0x1A45
#define guy7 5 + guy4                      //SEQTBL_BASE + 220   // 0x1A4A
#define guy8 3 + guy7                      //SEQTBL_BASE + 223   // 0x1A4D
#define blockedstrike 7 + guy8             //SEQTBL_BASE + 230   // 0x1A54
#define blocktostrike 6 + blockedstrike    //SEQTBL_BASE + 236   // 0x1A5A
#define readyblock 4 + blocktostrike       //SEQTBL_BASE + 240   // 0x1A5E
#define blocking 1 + readyblock            //SEQTBL_BASE + 241   // 0x1A5F
#define striketoblock 4 + blocking         //SEQTBL_BASE + 245   // 0x1A63
#define landengarde 5 + striketoblock      //SEQTBL_BASE + 250   // 0x1A68
#define bumpengfwd 6 + landengarde         //SEQTBL_BASE + 256   // 0x1A6E
#define bumpengback 7 + bumpengfwd         //SEQTBL_BASE + 263   // 0x1A75
#define flee 7 + bumpengback               //SEQTBL_BASE + 270   // 0x1A7C
#define turnengarde 7 + flee               //SEQTBL_BASE + 277   // 0x1A83
#define alertturn 8 + turnengarde          //SEQTBL_BASE + 285   // 0x1A8B
#define standjump 8 + alertturn            //SEQTBL_BASE + 293   // 0x1A93
#define sjland 29 + standjump              //SEQTBL_BASE + 322   // 0x1AB0
#define runjump 29 + sjland                //SEQTBL_BASE + 351   // 0x1ACD
#define rjlandrun 46 + runjump             //SEQTBL_BASE + 397   // 0x1AFB
#define rdiveroll 9 + rjlandrun            //SEQTBL_BASE + 406   // 0x1B04
#define rdiveroll_crouch 18 + rdiveroll    //SEQTBL_BASE + 424   // 0x1B16
#define sdiveroll 4 + rdiveroll_crouch     //SEQTBL_BASE + 428   // 0x1B1A
#define crawl 1 + sdiveroll                //SEQTBL_BASE + 429   // 0x1B1B
#define crawl_crouch 14 + crawl            //SEQTBL_BASE + 443   // 0x1B29
#define turndraw 4 + crawl_crouch          //SEQTBL_BASE + 447   // 0x1B2D
#define turn 12 + turndraw                 //SEQTBL_BASE + 459   // 0x1B39
#define turnrun 26 + turn                  //SEQTBL_BASE + 485   // 0x1B53
#define runturn 7 + turnrun                //SEQTBL_BASE + 492   // 0x1B5A
#define fightfall 43 + runturn             //SEQTBL_BASE + 535   // 0x1B85
#define efightfall 28 + fightfall          //SEQTBL_BASE + 563   // 0x1BA1
#define efightfallfwd 30 + efightfall      //SEQTBL_BASE + 593   // 0x1BBF
#define stepfall 28 + efightfallfwd        //SEQTBL_BASE + 621   // 0x1BDB
#define fall1 9 + stepfall                 //SEQTBL_BASE + 630   // 0x1BE4
#define patchfall 22 + fall1               //SEQTBL_BASE + 652   // 0x1BFA
#define stepfall2 7 + patchfall            //SEQTBL_BASE + 659   // 0x1C01
#define stepfloat 5 + stepfall2            //SEQTBL_BASE + 664   // 0x1C06
#define jumpfall 22 + stepfloat            //SEQTBL_BASE + 686   // 0x1C1C
#define rjumpfall 28 + jumpfall            //SEQTBL_BASE + 714   // 0x1C38
#define jumphangMed 28 + rjumpfall         //SEQTBL_BASE + 742   // 0x1C54
#define jumphangLong 21 + jumphangMed      //SEQTBL_BASE + 763   // 0x1C69
#define jumpbackhang 27 + jumphangLong     //SEQTBL_BASE + 790   // 0x1C84
#define hang 29 + jumpbackhang             //SEQTBL_BASE + 819   // 0x1CA1
#define hang1 3 + hang                     //SEQTBL_BASE + 822   // 0x1CA4
#define hangstraight 45 + hang1            //SEQTBL_BASE + 867   // 0x1CD1
#define hangstraight_loop 7 + hangstraight //SEQTBL_BASE + 874   // 0x1CD8
#define climbfail 4 + hangstraight_loop    //SEQTBL_BASE + 878   // 0x1CDC
#define climbdown 16 + climbfail           //SEQTBL_BASE + 894   // 0x1CEC
#define climbup 24 + climbdown             //SEQTBL_BASE + 918   // 0x1D04
#define hangdrop 33 + climbup              //SEQTBL_BASE + 951   // 0x1D25
#define hangfall 17 + hangdrop             //SEQTBL_BASE + 968   // 0x1D36
#define freefall 19 + hangfall             //SEQTBL_BASE + 987   // 0x1D49
#define freefall_loop 2 + freefall         //SEQTBL_BASE + 989   // 0x1D4B
#define runstop 4 + freefall_loop          //SEQTBL_BASE + 993   // 0x1D4F
#define jumpup 25 + runstop                //SEQTBL_BASE + 1018  // 0x1D68
#define highjump 21 + jumpup               //SEQTBL_BASE + 1039  // 0x1D7D
#define superhijump 30 + highjump          //SEQTBL_BASE + 1069  // 0x1D9B
#define fallhang 91 + superhijump          //SEQTBL_BASE + 1160  // 0x1DF6
#define bump 6 + fallhang                  //SEQTBL_BASE + 1166  // 0x1DFC
#define bumpfall 10 + bump                 //SEQTBL_BASE + 1176  // 0x1E06
#define bumpfloat 31 + bumpfall            //SEQTBL_BASE + 1207  // 0x1E25
#define hardbump 22 + bumpfloat            //SEQTBL_BASE + 1229  // 0x1E3B
#define testfoot 30 + hardbump             //SEQTBL_BASE + 1259  // 0x1E59
#define stepback 31 + testfoot             //SEQTBL_BASE + 1290  // 0x1E78
#define step14 5 + stepback                //SEQTBL_BASE + 1295  // 0x1E7D
#define step13 31 + step14                 //SEQTBL_BASE + 1326  // 0x1E9C
#define step12 31 + step13                 //SEQTBL_BASE + 1357  // 0x1EBB
#define step11 31 + step12                 //SEQTBL_BASE + 1388  // 0x1EDA
#define step10 29 + step11                 //SEQTBL_BASE + 1417  // 0x1EF7
#define step10a 5 + step10                 //SEQTBL_BASE + 1422  // 0x1EFC
#define step9 23 + step10a                 //SEQTBL_BASE + 1445  // 0x1F13
#define step8 6 + step9                    //SEQTBL_BASE + 1451  // 0x1F19
#define step7 26 + step8                   //SEQTBL_BASE + 1477  // 0x1F33
#define step6 21 + step7                   //SEQTBL_BASE + 1498  // 0x1F48
#define step5 21 + step6                   //SEQTBL_BASE + 1519  // 0x1F5D
#define step4 21 + step5                   //SEQTBL_BASE + 1540  // 0x1F72
#define step3 16 + step4                   //SEQTBL_BASE + 1556  // 0x1F82
#define step2 16 + step3                   //SEQTBL_BASE + 1572  // 0x1F92
#define step1 12 + step2                   //SEQTBL_BASE + 1584  // 0x1F9E
#define stoop 9 + step1                    //SEQTBL_BASE + 1593  // 0x1FA7
#define stoop_crouch 8 + stoop             //SEQTBL_BASE + 1601  // 0x1FAF
#define standup 4 + stoop_crouch           //SEQTBL_BASE + 1605  // 0x1FB3
#define pickupsword 23 + standup           //SEQTBL_BASE + 1628  // 0x1FCA
#define resheathe 16 + pickupsword         //SEQTBL_BASE + 1644  // 0x1FDA
#define fastsheathe 33 + resheathe         //SEQTBL_BASE + 1677  // 0x1FFB
#define drinkpotion 14 + fastsheathe       //SEQTBL_BASE + 1691  // 0x2009
#define softland 34 + drinkpotion          //SEQTBL_BASE + 1725  // 0x202B
#define softland_crouch 11 + softland      //SEQTBL_BASE + 1736  // 0x2036
#define landrun 4 + softland_crouch        //SEQTBL_BASE + 1740  // 0x203A
#define medland 32 + landrun               //SEQTBL_BASE + 1772  // 0x205A
#define hardland 66 + medland              //SEQTBL_BASE + 1838  // 0x209C
#define hardland_dead 9 + hardland         //SEQTBL_BASE + 1847  // 0x20A5
#define stabkill 4 + hardland_dead         //SEQTBL_BASE + 1851  // 0x20A9
#define dropdead 5 + stabkill              //SEQTBL_BASE + 1856  // 0x20AE
#define dropdead_dead 12 + dropdead        //SEQTBL_BASE + 1868  // 0x20BA
#define impale 4 + dropdead_dead           //SEQTBL_BASE + 1872  // 0x20BE
#define impale_dead 7 + impale             //SEQTBL_BASE + 1879  // 0x20C5
#define halve 4 + impale_dead              //SEQTBL_BASE + 1883  // 0x20C9
#define halve_dead 4 + halve               //SEQTBL_BASE + 1887  // 0x20CD
#define crush 4 + halve_dead               //SEQTBL_BASE + 1891  // 0x20D1
#define deadfall 3 + crush                 //SEQTBL_BASE + 1894  // 0x20D4
#define deadfall_loop 5 + deadfall         //SEQTBL_BASE + 1899  // 0x20D9
#define climbstairs 4 + deadfall_loop      //SEQTBL_BASE + 1903  // 0x20DD
#define climbstairs_loop 81 + climbstairs  //SEQTBL_BASE + 1984  // 0x212E
#define Vstand 4 + climbstairs_loop        //SEQTBL_BASE + 1988  // 0x2132
#define Vraise 4 + Vstand                  //SEQTBL_BASE + 1992  // 0x2136
#define Vraise_loop 21 + Vraise            //SEQTBL_BASE + 2013  // 0x214B
#define Vwalk 4 + Vraise_loop              //SEQTBL_BASE + 2017  // 0x214F
#define Vwalk1 2 + Vwalk                   //SEQTBL_BASE + 2019  // 0x2151
#define Vwalk2 3 + Vwalk1                  //SEQTBL_BASE + 2022  // 0x2154
#define Vstop 18 + Vwalk2                  //SEQTBL_BASE + 2040  // 0x2166
#define Vexit 7 + Vstop                    //SEQTBL_BASE + 2047  // 0x216D
#define Pstand 40 + Vexit                  //SEQTBL_BASE + 2087  // 0x2195
#define Palert 4 + Pstand                  //SEQTBL_BASE + 2091  // 0x2199
#define Pstepback 15 + Palert              //SEQTBL_BASE + 2106  // 0x21A8
#define Pstepback_loop 16 + Pstepback      //SEQTBL_BASE + 2122  // 0x21B8
#define Plie 4 + Pstepback_loop            //SEQTBL_BASE + 2126  // 0x21BC
#define Pwaiting 4 + Plie                  //SEQTBL_BASE + 2130  // 0x21C0
#define Pembrace 4 + Pwaiting              //SEQTBL_BASE + 2134  // 0x21C4
#define Pembrace_loop 30 + Pembrace        //SEQTBL_BASE + 2164  // 0x21E2
#define Pstroke 4 + Pembrace_loop          //SEQTBL_BASE + 2168  // 0x21E6
#define Prise 4 + Pstroke                  //SEQTBL_BASE + 2172  // 0x21EA
#define Prise_loop 14 + Prise              //SEQTBL_BASE + 2186  // 0x21F8
#define Pcrouch 4 + Prise_loop             //SEQTBL_BASE + 2190  // 0x21FC
#define Pcrouch_loop 64 + Pcrouch          //SEQTBL_BASE + 2254  // 0x223C
#define Pslump 4 + Pcrouch_loop            //SEQTBL_BASE + 2258  // 0x2240
#define Pslump_loop 1 + Pslump             //SEQTBL_BASE + 2259  // 0x2241
#define Mscurry 4 + Pslump_loop            //SEQTBL_BASE + 2263  // 0x2245
#define Mscurry1 2 + Mscurry               //SEQTBL_BASE + 2265  // 0x2247
#define Mstop 12 + Mscurry1                //SEQTBL_BASE + 2277  // 0x2253
#define Mraise 4 + Mstop                   //SEQTBL_BASE + 2281  // 0x2257
#define Mleave 4 + Mraise                  //SEQTBL_BASE + 2285  // 0x225B
#define Mclimb 19 + Mleave                 //SEQTBL_BASE + 2304  // 0x226E
#define Mclimb_loop 2 + Mclimb             //SEQTBL_BASE + 2306  // 0x2270

const word seqtbl_offsets[] = {
  0x0000, startrun, stand, standjump, runjump, turn, runturn, stepfall, jumphangMed, hang, climbup, hangdrop, freefall, runstop, jumpup, fallhang, jumpbackhang, softland, jumpfall, stepfall2, medland, rjumpfall, hardland, hangfall, jumphangLong, hangstraight, rdiveroll, sdiveroll, highjump, step1, step2, step3, step4, step5, step6, step7, step8, step9, step10, step11, step12, step13, step14, turnrun, testfoot, bumpfall, hardbump, bump, superhijump, standup, stoop, impale, crush, deadfall, halve, engarde, advance, retreat, strike, flee, turnengarde, striketoblock, readyblock, landengarde, bumpengfwd, bumpengback, blocktostrike, strikeadv, climbdown, blockedstrike, climbstairs, dropdead, stepback, climbfail, stabbed, faststrike, strikeret, alertstand, drinkpotion, crawl, alertturn, fightfall, efightfall, efightfallfwd, running, stabkill, fastadvance, goalertstand, arise, turndraw, guardengarde, pickupsword, resheathe, fastsheathe, Pstand, Vstand, Vwalk, Vstop, Palert, Pstepback, Vexit, Mclimb, Vraise, Plie, patchfall, Mscurry, Mstop, Mleave, Pembrace, Pwaiting, Pstroke, Prise, Pcrouch, Pslump, Mraise};

// data:196E
static const byte seqtbl[] = {

  LABEL(running) // running
  act(actions_1_run_jump),
  jmp(runcyc1), // goto running: frame 7

  LABEL(startrun) // startrun
  act(actions_1_run_jump),
  LABEL(runstt1) frame_1_start_run,
  frame_2_start_run,
  frame_3_start_run,
  LABEL(runstt4) frame_4_start_run,
  dx(8),
  frame_5_start_run,
  dx(3),
  frame_6_start_run,
  dx(3),
  LABEL(runcyc1) frame_7_run,
  dx(5),
  frame_8_run,
  dx(1),
  snd(SND_FOOTSTEP),
  frame_9_run,
  dx(2),
  frame_10_run,
  dx(4),
  frame_11_run,
  dx(5),
  frame_12_run,
  dx(2),
  LABEL(runcyc7) snd(SND_FOOTSTEP),
  frame_13_run,
  dx(3),
  frame_14_run,
  dx(4),
  jmp(runcyc1),

  LABEL(stand) // stand
  act(actions_0_stand),
  frame_15_stand,
  jmp(stand), // goto "stand"

  LABEL(goalertstand) // alert stand
  act(actions_1_run_jump),
  LABEL(alertstand) frame_166_stand_inactive,
  jmp(alertstand), // goto "alertstand"

  LABEL(arise) // arise (skeleton)
  act(actions_5_bumped),
  dx(10),
  frame_177_spiked,
  frame_177_spiked,
  dx(-7),
  dy(-2),
  frame_178_chomped,
  dx(5),
  dy(2),
  frame_166_stand_inactive,
  dx(-1),
  jmp(ready), // goto "ready"

  // guard engarde
  LABEL(guardengarde)
    jmp(ready), // goto "ready"

  // engarde
  LABEL(engarde)
    act(actions_1_run_jump),
  dx(2),
  frame_207_draw_1,
  frame_208_draw_2,
  dx(2),
  frame_209_draw_3,
  dx(2),
  frame_210_draw_4,
  dx(3),
  LABEL(ready) act(actions_1_run_jump),
  snd(SND_SILENT),
  frame_158_stand_with_sword,
  frame_170_stand_with_sword,
  LABEL(ready_loop) frame_171_stand_with_sword,
  jmp(ready_loop), // goto ":loop"

  LABEL(stabbed) // stabbed
  act(actions_5_bumped),
  set_fall(-1, 0),
  frame_172_jumpfall_2,
  dx(-1),
  dy(1),
  frame_173_jumpfall_3,
  dx(-1),
  frame_174_jumpfall_4,
  dx(-1),
  dy(2), // frame 175 is commented out in the Apple II source
  dx(-2),
  dy(1),
  dx(-5),
  dy(-4),
  jmp(guy8), // goto "guy8"

  // strike - advance
  LABEL(strikeadv)
    act(actions_1_run_jump),
  set_fall(1, 0),
  frame_155_guy_7,
  dx(2),
  frame_165_walk_with_sword,
  dx(-2),
  jmp(ready), // goto "ready"

  LABEL(strikeret) // strike - retreat
  act(actions_1_run_jump),
  set_fall(-1, 0),
  frame_155_guy_7,
  frame_156_guy_8,
  frame_157_walk_with_sword,
  frame_158_stand_with_sword,
  jmp(retreat), // goto "retreat"

  LABEL(advance) // advance
  act(actions_1_run_jump),
  set_fall(1, 0),
  dx(2),
  frame_163_fighting,
  dx(4),
  frame_164_fighting,
  frame_165_walk_with_sword,
  jmp(ready), // goto "ready"

  LABEL(fastadvance) // fast advance
  act(actions_1_run_jump),
  set_fall(1, 0),
  dx(6),
  frame_164_fighting,
  frame_165_walk_with_sword,
  jmp(ready), // goto "ready"

  LABEL(retreat) // retreat
  act(actions_1_run_jump),
  set_fall(-1, 0),
  dx(-3),
  frame_160_fighting,
  dx(-2),
  frame_157_walk_with_sword,
  jmp(ready), // goto "ready"

  LABEL(strike) // strike
  act(actions_1_run_jump),
  set_fall(-1, 0),
  frame_168_back,
  LABEL(faststrike) act(actions_1_run_jump),
  frame_151_strike_1,
  LABEL(guy4) act(actions_1_run_jump),
  frame_152_strike_2,
  frame_153_strike_3,
  frame_154_poking,
  LABEL(guy7) act(actions_5_bumped),
  frame_155_guy_7,
  LABEL(guy8) act(actions_1_run_jump),
  frame_156_guy_8,
  frame_157_walk_with_sword,
  jmp(ready), // goto "ready"

  LABEL(blockedstrike) // blocked strike
  act(actions_1_run_jump),
  frame_167_blocked,
  jmp(guy7), // goto "guy7"

  // block to strike
  LABEL(blocktostrike) // "blocktostrike"
  frame_162_block_to_strike,
  jmp(guy4), // goto "guy4"

  LABEL(readyblock) // ready block
  frame_169_begin_block,
  LABEL(blocking) frame_150_parry,
  jmp(ready), // goto "ready"

  LABEL(striketoblock) // strike to block
  frame_159_fighting,
  frame_160_fighting,
  jmp(blocking), // goto "blocking"

  LABEL(landengarde) // land en garde
  act(actions_1_run_jump),
  SEQ_KNOCK_DOWN,
  jmp(ready), // goto "ready"

  LABEL(bumpengfwd) // bump en garde (forward)
  act(actions_5_bumped),
  dx(-8),
  jmp(ready), // goto "ready"

  LABEL(bumpengback) // bump en garde (back)
  act(actions_5_bumped),
  frame_160_fighting,
  frame_157_walk_with_sword,
  jmp(ready), // goto "ready"

  LABEL(flee) // flee
  act(actions_7_turn),
  dx(-8),
  jmp(turn), // goto "turn"

  LABEL(turnengarde) // turn en garde
  act(actions_5_bumped),
  SEQ_FLIP,
  dx(5),
  jmp(retreat), // goto "retreat"

  LABEL(alertturn) // alert turn (for enemies)
  act(actions_5_bumped),
  SEQ_FLIP,
  dx(18),
  jmp(goalertstand), // goto "goalertstand"

  LABEL(standjump) // standing jump
  act(actions_1_run_jump),
  frame_16_standing_jump_1,
  frame_17_standing_jump_2,
  dx(2),
  frame_18_standing_jump_3,
  dx(2),
  frame_19_standing_jump_4,
  dx(2),
  frame_20_standing_jump_5,
  dx(2),
  frame_21_standing_jump_6,
  dx(2),
  frame_22_standing_jump_7,
  dx(7),
  frame_23_standing_jump_8,
  dx(9),
  frame_24_standing_jump_9,
  dx(5),
  dy(-6),
  /* "sjland" */ LABEL(sjland) frame_25_standing_jump_10,
  dx(1),
  dy(6),
  frame_26_standing_jump_11,
  dx(4),
  SEQ_KNOCK_DOWN,
  snd(SND_FOOTSTEP),
  frame_27_standing_jump_12,
  dx(-3),
  frame_28_standing_jump_13,
  dx(5),
  frame_29_standing_jump_14,
  snd(SND_FOOTSTEP),
  frame_30_standing_jump_15,
  frame_31_standing_jump_16,
  frame_32_standing_jump_17,
  frame_33_standing_jump_18,
  dx(1),
  jmp(stand), // goto "stand"

  LABEL(runjump) // running jump
  act(actions_1_run_jump),
  snd(SND_FOOTSTEP),
  frame_34_start_run_jump_1,
  dx(5),
  frame_35_start_run_jump_2,
  dx(6),
  frame_36_start_run_jump_3,
  dx(3),
  frame_37_start_run_jump_4,
  dx(5),
  snd(SND_FOOTSTEP),
  frame_38_start_run_jump_5,
  dx(7),
  frame_39_start_run_jump_6,
  dx(12),
  dy(-3),
  frame_40_running_jump_1,
  dx(8),
  dy(-9),
  frame_41_running_jump_2,
  dx(8),
  dy(-2),
  frame_42_running_jump_3,
  dx(4),
  dy(11),
  frame_43_running_jump_4,
  dx(4),
  dy(3),
  /* "rjlandrun" */ LABEL(rjlandrun) frame_44_running_jump_5,
  dx(5),
  SEQ_KNOCK_DOWN,
  snd(SND_FOOTSTEP),
  jmp(runcyc1), // goto "runcyc1"

  LABEL(rdiveroll) // run dive roll
  act(actions_1_run_jump),
  dx(1),
  frame_107_fall_land_1,
  dx(2),
  dx(2),
  frame_108_fall_land_2,
  dx(2),
  frame_109_crouch,
  dx(2),
  frame_109_crouch,
  dx(2),
  /* ":crouch" */ LABEL(rdiveroll_crouch) frame_109_crouch,
  jmp(rdiveroll_crouch), // goto ":crouch"

  LABEL(sdiveroll) 0x00, // stand dive roll; not implemented

  LABEL(crawl) // crawl
  act(actions_1_run_jump),
  dx(1),
  frame_110_stand_up_from_crouch_1,
  frame_111_stand_up_from_crouch_2,
  dx(2),
  frame_112_stand_up_from_crouch_3,
  dx(2),
  frame_108_fall_land_2,
  dx(2),
  /* ":crouch" */ LABEL(crawl_crouch) frame_109_crouch,
  jmp(crawl_crouch), // goto ":crouch"

  LABEL(turndraw) // turn draw
  act(actions_7_turn),
  SEQ_FLIP,
  dx(6),
  frame_45_turn,
  dx(1),
  frame_46_turn,
  jmp(engarde), // goto "engarde"

  LABEL(turn) // turn
  act(actions_7_turn),
  SEQ_FLIP,
  dx(6),
  frame_45_turn,
  dx(1),
  frame_46_turn,
  dx(2),
  frame_47_turn,
  dx(-1),
  /* "finishturn" */ frame_48_turn,
  dx(1),
  frame_49_turn,
  dx(-2),
  frame_50_turn,
  frame_51_turn,
  frame_52_turn,
  jmp(stand), // goto "stand"

  LABEL(turnrun) // turnrun (from frame 48)
  act(actions_1_run_jump),
  dx(-1),
  jmp(runstt1), // goto "runstt1"

  LABEL(runturn) // runturn
  act(actions_1_run_jump),
  dx(1),
  frame_53_runturn,
  dx(1),
  snd(SND_FOOTSTEP),
  frame_54_runturn,
  dx(8),
  frame_55_runturn,
  snd(SND_FOOTSTEP),
  frame_56_runturn,
  dx(7),
  frame_57_runturn,
  dx(3),
  frame_58_runturn,
  dx(1),
  frame_59_runturn,
  frame_60_runturn,
  dx(2),
  frame_61_runturn,
  dx(-1),
  frame_62_runturn,
  frame_63_runturn,
  frame_64_runturn,
  dx(-1),
  frame_65_runturn,
  dx(-14),
  SEQ_FLIP,
  jmp(runcyc7), // goto "runcyc7"

  LABEL(fightfall) // fightfall (backward)
  act(actions_3_in_midair),
  dy(-1),
  frame_102_start_fall_1,
  dx(-2),
  dy(6),
  frame_103_start_fall_2,
  dx(-2),
  dy(9),
  frame_104_start_fall_3,
  dx(-1),
  dy(12),
  frame_105_start_fall_4,
  dx(-3),
  set_fall(0, 15),
  jmp(freefall), // goto "freefall"

  LABEL(efightfall) // enemy fight fall
  act(actions_3_in_midair),
  dy(-1),
  dx(-2),
  frame_102_start_fall_1,
  dx(-3),
  dy(6),
  frame_103_start_fall_2,
  dx(-3),
  dy(9),
  frame_104_start_fall_3,
  dx(-2),
  dy(12),
  frame_105_start_fall_4,
  dx(-3),
  set_fall(0, 15),
  jmp(freefall), // goto "freefall"

  LABEL(efightfallfwd) // enemy fight fall forward
  act(actions_3_in_midair),
  dx(1),
  dy(-1),
  frame_102_start_fall_1,
  dx(2),
  dy(6),
  frame_103_start_fall_2,
  dx(-1),
  dy(9),
  frame_104_start_fall_3,
  dy(12),
  frame_105_start_fall_4,
  dx(-2),
  set_fall(1, 15),
  jmp(freefall), // goto "freefall"

  LABEL(stepfall) // stepfall
  act(actions_3_in_midair),
  dx(1),
  dy(3),
  jmp_if_feather(stepfloat), // goto "stepfloat"
  /* "fall1" */ LABEL(fall1) frame_102_start_fall_1,
  dx(2),
  dy(6),
  frame_103_start_fall_2,
  dx(-1),
  dy(9),
  frame_104_start_fall_3,
  dy(12),
  frame_105_start_fall_4,
  dx(-2),
  set_fall(1, 15),
  jmp(freefall), // goto "freefall"

  LABEL(patchfall) // patchfall
  dx(-1),
  dy(-3),
  jmp(fall1), // goto "fall1"

  LABEL(stepfall2) // stepfall2 (from frame 12)
  dx(1),
  jmp(stepfall), // goto "stepfall"

  LABEL(stepfloat) // stepfloat
  frame_102_start_fall_1,
  dx(2),
  dy(3),
  frame_103_start_fall_2,
  dx(-1),
  dy(4),
  frame_104_start_fall_3,
  dy(5),
  frame_105_start_fall_4,
  dx(-2),
  set_fall(1, 6),
  jmp(freefall), // goto "freefall"

  LABEL(jumpfall) // jump fall (from standing jump)
  act(actions_3_in_midair),
  dx(1),
  dy(3),
  frame_102_start_fall_1,
  dx(2),
  dy(6),
  frame_103_start_fall_2,
  dx(1),
  dy(9),
  frame_104_start_fall_3,
  dx(2),
  dy(12),
  frame_105_start_fall_4,
  set_fall(2, 15),
  jmp(freefall), // goto "freefall"

  LABEL(rjumpfall) // running jump fall
  act(actions_3_in_midair),
  dx(1),
  dy(3),
  frame_102_start_fall_1,
  dx(3),
  dy(6),
  frame_103_start_fall_2,
  dx(2),
  dy(9),
  frame_104_start_fall_3,
  dx(3),
  dy(12),
  frame_105_start_fall_4,
  set_fall(3, 15),
  jmp(freefall), // goto "freefall"

  LABEL(jumphangMed) // jumphang (medium: DX = 0)
  act(actions_1_run_jump),
  frame_67_start_jump_up_1,
  frame_68_start_jump_up_2,
  frame_69_start_jump_up_3,
  frame_70_jumphang,
  frame_71_jumphang,
  frame_72_jumphang,
  frame_73_jumphang,
  frame_74_jumphang,
  frame_75_jumphang,
  frame_76_jumphang,
  frame_77_jumphang,
  act(actions_2_hang_climb),
  frame_78_jumphang,
  frame_79_jumphang,
  frame_80_jumphang,
  jmp(hang), // goto "hang"

  LABEL(jumphangLong) // jumphang (long: DX = 4)
  act(actions_1_run_jump),
  frame_67_start_jump_up_1,
  frame_68_start_jump_up_2,
  frame_69_start_jump_up_3,
  frame_70_jumphang,
  frame_71_jumphang,
  frame_72_jumphang,
  frame_73_jumphang,
  frame_74_jumphang,
  frame_75_jumphang,
  frame_76_jumphang,
  frame_77_jumphang,
  act(actions_2_hang_climb),
  dx(1),
  frame_78_jumphang,
  dx(2),
  frame_79_jumphang,
  dx(1),
  frame_80_jumphang,
  jmp(hang), // goto "hang"

  LABEL(jumpbackhang) // jumpbackhang
  act(actions_1_run_jump),
  frame_67_start_jump_up_1,
  frame_68_start_jump_up_2,
  frame_69_start_jump_up_3,
  frame_70_jumphang,
  frame_71_jumphang,
  frame_72_jumphang,
  frame_73_jumphang,
  frame_74_jumphang,
  frame_75_jumphang,
  frame_76_jumphang,
  dx(-1),
  frame_77_jumphang,
  act(actions_2_hang_climb),
  dx(-2),
  frame_78_jumphang,
  dx(-1),
  frame_79_jumphang,
  dx(-1),
  frame_80_jumphang,
  jmp(hang), // goto "hang"

  LABEL(hang) // hang
  act(actions_2_hang_climb),
  frame_91_hanging_5,
  /* "hang1" */ LABEL(hang1) frame_90_hanging_4,
  frame_89_hanging_3,
  frame_88_hanging_2,
  frame_87_hanging_1,
  frame_87_hanging_1,
  frame_87_hanging_1,
  frame_88_hanging_2,
  frame_89_hanging_3,
  frame_90_hanging_4,
  frame_91_hanging_5,
  frame_92_hanging_6,
  frame_93_hanging_7,
  frame_94_hanging_8,
  frame_95_hanging_9,
  frame_96_hanging_10,
  frame_97_hanging_11,
  frame_98_hanging_12,
  frame_99_hanging_13,
  frame_97_hanging_11,
  frame_96_hanging_10,
  frame_95_hanging_9,
  frame_94_hanging_8,
  frame_93_hanging_7,
  frame_92_hanging_6,
  frame_91_hanging_5,
  frame_90_hanging_4,
  frame_89_hanging_3,
  frame_88_hanging_2,
  frame_87_hanging_1,
  frame_88_hanging_2,
  frame_89_hanging_3,
  frame_90_hanging_4,
  frame_91_hanging_5,
  frame_92_hanging_6,
  frame_93_hanging_7,
  frame_94_hanging_8,
  frame_95_hanging_9,
  frame_96_hanging_10,
  frame_95_hanging_9,
  frame_94_hanging_8,
  frame_93_hanging_7,
  frame_92_hanging_6,
  jmp(hangdrop), // goto "hangdrop"

  LABEL(hangstraight) // hangstraight
  act(actions_6_hang_straight),
  frame_92_hanging_6, // Apple II source has a bump sound here
  frame_93_hanging_7,
  frame_93_hanging_7,
  frame_92_hanging_6,
  frame_92_hanging_6,
  /* ":loop" */ LABEL(hangstraight_loop) frame_91_hanging_5,
  jmp(hangstraight_loop), // goto ":loop"

  LABEL(climbfail) // climbfail
  frame_135_climbing_1,
  frame_136_climbing_2,
  frame_137_climbing_3,
  frame_137_climbing_3,
  frame_138_climbing_4,
  frame_138_climbing_4,
  frame_138_climbing_4,
  frame_138_climbing_4,
  frame_137_climbing_3,
  frame_136_climbing_2,
  frame_135_climbing_1,
  dx(-7),
  jmp(hangdrop), // goto "hangdrop"

  LABEL(climbdown) // climbdown
  act(actions_1_run_jump),
  frame_148_climbing_14,
  frame_145_climbing_11,
  frame_144_climbing_10,
  frame_143_climbing_9,
  frame_142_climbing_8,
  frame_141_climbing_7,
  dx(-5),
  dy(63),
  SEQ_DOWN,
  act(actions_3_in_midair),
  frame_140_climbing_6,
  frame_138_climbing_4,
  frame_136_climbing_2,
  frame_91_hanging_5,
  act(actions_2_hang_climb),
  jmp(hang1), // goto "hang1"

  LABEL(climbup) // climbup
  act(actions_1_run_jump),
  frame_135_climbing_1,
  frame_136_climbing_2,
  frame_137_climbing_3,
  frame_138_climbing_4,
  frame_139_climbing_5,
  frame_140_climbing_6,
  dx(5),
  dy(-63),
  SEQ_UP,
  frame_141_climbing_7,
  frame_142_climbing_8,
  frame_143_climbing_9,
  frame_144_climbing_10,
  frame_145_climbing_11,
  frame_146_climbing_12,
  frame_147_climbing_13,
  frame_148_climbing_14,
  act(actions_5_bumped), // to clear flags
  frame_149_climbing_15,
  act(actions_1_run_jump),
  frame_118_stand_up_from_crouch_9,
  frame_119_stand_up_from_crouch_10,
  dx(1),
  jmp(stand), // goto "stand"

  LABEL(hangdrop) // hangdrop
  frame_81_hangdrop_1,
  frame_82_hangdrop_2,
  act(actions_5_bumped),
  frame_83_hangdrop_3,
  act(actions_1_run_jump),
  SEQ_KNOCK_DOWN,
  snd(SND_SILENT),
  frame_84_hangdrop_4,
  frame_85_hangdrop_5,
  dx(3),
  jmp(stand), // goto "stand"

  LABEL(hangfall) // hangfall
  act(actions_3_in_midair),
  frame_81_hangdrop_1,
  dy(6),
  frame_81_hangdrop_1,
  dy(9),
  frame_81_hangdrop_1,
  dy(12),
  dx(2),
  set_fall(0, 12),
  jmp(freefall), // goto "freefall"

  LABEL(freefall) // freefall
  act(actions_4_in_freefall),
  /* ":loop" */ LABEL(freefall_loop) frame_106_fall,
  jmp(freefall_loop), // goto :loop

  LABEL(runstop) // runstop
  act(actions_1_run_jump),
  frame_53_runturn,
  dx(2),
  snd(SND_FOOTSTEP),
  frame_54_runturn,
  dx(7),
  frame_55_runturn,
  snd(SND_FOOTSTEP),
  frame_56_runturn,
  dx(2),
  frame_49_turn,
  dx(-2),
  frame_50_turn,
  frame_51_turn,
  frame_52_turn,
  jmp(stand), // goto "stand"

  LABEL(jumpup) // jump up (and touch ceiling)
  act(actions_1_run_jump),
  frame_67_start_jump_up_1,
  frame_68_start_jump_up_2,
  frame_69_start_jump_up_3,
  frame_70_jumphang,
  frame_71_jumphang,
  frame_72_jumphang,
  frame_73_jumphang,
  frame_74_jumphang,
  frame_75_jumphang,
  frame_76_jumphang,
  frame_77_jumphang,
  frame_78_jumphang,
  act(actions_0_stand),
  SEQ_KNOCK_UP,
  frame_79_jumphang,
  jmp(hangdrop), // goto "hangdrop"

  LABEL(highjump) // highjump (no ceiling above)
  act(actions_1_run_jump),
  frame_67_start_jump_up_1,
  frame_68_start_jump_up_2,
  frame_69_start_jump_up_3,
  frame_70_jumphang,
  frame_71_jumphang,
  frame_72_jumphang,
  frame_73_jumphang,
  frame_74_jumphang,
  frame_75_jumphang,
  frame_76_jumphang,
  frame_77_jumphang,
  frame_78_jumphang,
  frame_79_jumphang,
  dy(-4),
  frame_79_jumphang,
  dy(-2),
  frame_79_jumphang,
  frame_79_jumphang,
  dy(2),
  frame_79_jumphang,
  dy(4),
  jmp(hangdrop), // goto "hangdrop"

  LABEL(superhijump) // superhijump (when weightless)
  frame_67_start_jump_up_1,
  frame_68_start_jump_up_2,
  frame_69_start_jump_up_3,
  frame_70_jumphang,
  frame_71_jumphang,
  frame_72_jumphang,
  frame_73_jumphang,
  frame_74_jumphang,
  frame_75_jumphang,
  frame_76_jumphang,
  dy(-1),
  frame_77_jumphang,
  dy(-3),
  frame_78_jumphang,
  dy(-4),
  frame_79_jumphang,
  dy(-10),
  frame_79_jumphang,
  dy(-9),
  frame_79_jumphang,
  dy(-8),
  frame_79_jumphang,
  dy(-7),
  frame_79_jumphang,
  dy(-6),
  frame_79_jumphang,
  dy(-5),
  frame_79_jumphang,
  dy(-4),
  frame_79_jumphang,
  dy(-3),
  frame_79_jumphang,
  dy(-2),
  frame_79_jumphang,
  dy(-2),
  frame_79_jumphang,
  dy(-1),
  frame_79_jumphang,
  dy(-1),
  frame_79_jumphang,
  dy(-1),
  frame_79_jumphang,
  frame_79_jumphang,
  frame_79_jumphang,
  frame_79_jumphang,
  dy(1),
  frame_79_jumphang,
  dy(1),
  frame_79_jumphang,
  dy(2),
  frame_79_jumphang,
  dy(2),
  frame_79_jumphang,
  dy(3),
  frame_79_jumphang,
  dy(4),
  frame_79_jumphang,
  dy(5),
  frame_79_jumphang,
  dy(6),
  frame_79_jumphang,
  set_fall(0, 6),
  jmp(freefall), // goto "freefall"

  LABEL(fallhang) // fall hang
  act(actions_3_in_midair),
  frame_80_jumphang,
  jmp(hang), // goto "hang"

  LABEL(bump) // bump
  act(actions_5_bumped),
  dx(-4),
  frame_50_turn,
  frame_51_turn,
  frame_52_turn,
  jmp(stand), // goto "stand"

  LABEL(bumpfall) // bumpfall
  act(actions_5_bumped),
  dx(1),
  dy(3),
  jmp_if_feather(bumpfloat),
  frame_102_start_fall_1,
  dx(2),
  dy(6),
  frame_103_start_fall_2,
  dx(-1),
  dy(9),
  frame_104_start_fall_3,
  dy(12),
  frame_105_start_fall_4,
  dx(-2),
  set_fall(0, 15),
  jmp(freefall), // goto "freefall"

  LABEL(bumpfloat) // bumpfloat
  frame_102_start_fall_1,
  dx(2),
  dy(3),
  frame_103_start_fall_2,
  dx(-1),
  dy(4),
  frame_104_start_fall_3,
  dy(5),
  frame_105_start_fall_4,
  dx(-2),
  set_fall(0, 6),
  jmp(freefall), // goto "freefall"

  LABEL(hardbump) // hard bump
  act(actions_5_bumped),
  dx(-1),
  dy(-4),
  frame_102_start_fall_1,
  dx(-1),
  dy(3),
  dx(-3),
  dy(1),
  SEQ_KNOCK_DOWN,
  dx(1),
  snd(SND_FOOTSTEP),
  frame_107_fall_land_1,
  dx(2),
  frame_108_fall_land_2,
  snd(SND_FOOTSTEP),
  frame_109_crouch,
  jmp(standup), // goto "standup"

  LABEL(testfoot) // test foot
  frame_121_stepping_1,
  dx(1),
  frame_122_stepping_2,
  frame_123_stepping_3,
  dx(2),
  frame_124_stepping_4,
  dx(4),
  frame_125_stepping_5,
  dx(3),
  frame_126_stepping_6,
  dx(-4),
  frame_86_test_foot,
  snd(SND_FOOTSTEP),
  SEQ_KNOCK_DOWN,
  dx(-4),
  frame_116_stand_up_from_crouch_7,
  dx(-2),
  frame_117_stand_up_from_crouch_8,
  frame_118_stand_up_from_crouch_9,
  frame_119_stand_up_from_crouch_10,
  jmp(stand), // goto "stand"

  LABEL(stepback) // step back
  dx(-5),
  jmp(stand), // goto "stand"

  LABEL(step14) // step forward 14 pixels
  act(actions_1_run_jump),
  frame_121_stepping_1,
  dx(1),
  frame_122_stepping_2,
  dx(1),
  frame_123_stepping_3,
  dx(3),
  frame_124_stepping_4,
  dx(4),
  frame_125_stepping_5,
  dx(3),
  frame_126_stepping_6,
  dx(-1),
  dx(3),
  frame_127_stepping_7,
  frame_128_stepping_8,
  frame_129_stepping_9,
  frame_130_stepping_10,
  frame_131_stepping_11,
  frame_132_stepping_12,
  jmp(stand), // goto "stand"

  LABEL(step13) // step forward 13 pixels
  act(actions_1_run_jump),
  frame_121_stepping_1,
  dx(1),
  frame_122_stepping_2,
  dx(1),
  frame_123_stepping_3,
  dx(3),
  frame_124_stepping_4,
  dx(4),
  frame_125_stepping_5,
  dx(3),
  frame_126_stepping_6,
  dx(-1),
  dx(2),
  frame_127_stepping_7,
  frame_128_stepping_8,
  frame_129_stepping_9,
  frame_130_stepping_10,
  frame_131_stepping_11,
  frame_132_stepping_12,
  jmp(stand), // goto "stand"

  LABEL(step12) // step forward 12 pixels
  act(actions_1_run_jump),
  frame_121_stepping_1,
  dx(1),
  frame_122_stepping_2,
  dx(1),
  frame_123_stepping_3,
  dx(3),
  frame_124_stepping_4,
  dx(4),
  frame_125_stepping_5,
  dx(3),
  frame_126_stepping_6,
  dx(-1),
  dx(1),
  frame_127_stepping_7,
  frame_128_stepping_8,
  frame_129_stepping_9,
  frame_130_stepping_10,
  frame_131_stepping_11,
  frame_132_stepping_12,
  jmp(stand), // goto "stand"

  LABEL(step11) // step forward 11 pixels (normal step)
  act(actions_1_run_jump),
  frame_121_stepping_1,
  dx(1),
  frame_122_stepping_2,
  dx(1),
  frame_123_stepping_3,
  dx(3),
  frame_124_stepping_4,
  dx(4),
  frame_125_stepping_5,
  dx(3),
  frame_126_stepping_6,
  dx(-1),
  frame_127_stepping_7,
  frame_128_stepping_8,
  frame_129_stepping_9,
  frame_130_stepping_10,
  frame_131_stepping_11,
  frame_132_stepping_12,
  jmp(stand), // goto "stand"

  LABEL(step10) // step forward 10 pixels
  act(actions_1_run_jump),
  frame_121_stepping_1,
  dx(1),
  /* "step10a "*/ LABEL(step10a) frame_122_stepping_2,
  dx(1),
  frame_123_stepping_3,
  dx(3),
  frame_124_stepping_4,
  dx(4),
  frame_125_stepping_5,
  dx(3),
  frame_126_stepping_6,
  dx(-) 2,
  frame_128_stepping_8,
  frame_129_stepping_9,
  frame_130_stepping_10,
  frame_131_stepping_11,
  frame_132_stepping_12,
  jmp(stand), // goto "stand"

  LABEL(step9) // step forward 9 pixels
  act(actions_1_run_jump),
  frame_121_stepping_1,
  jmp(step10a), // goto "step10a"

  LABEL(step8) // step forward 8 pixels
  act(actions_1_run_jump),
  frame_121_stepping_1,
  dx(1),
  frame_122_stepping_2,
  dx(1),
  frame_123_stepping_3,
  dx(3),
  frame_124_stepping_4,
  dx(4),
  frame_125_stepping_5,
  dx(-1),
  frame_127_stepping_7,
  frame_128_stepping_8,
  frame_129_stepping_9,
  frame_130_stepping_10,
  frame_131_stepping_11,
  frame_132_stepping_12,
  jmp(stand), // goto "stand"

  LABEL(step7) // step forward 7 pixels
  act(actions_1_run_jump),
  frame_121_stepping_1,
  dx(1),
  frame_122_stepping_2,
  dx(1),
  frame_123_stepping_3,
  dx(3),
  frame_124_stepping_4,
  dx(2),
  frame_129_stepping_9,
  frame_130_stepping_10,
  frame_131_stepping_11,
  frame_132_stepping_12,
  jmp(stand), // goto "stand"

  LABEL(step6) // step forward 6 pixels
  act(actions_1_run_jump),
  frame_121_stepping_1,
  dx(1),
  frame_122_stepping_2,
  dx(1),
  frame_123_stepping_3,
  dx(2),
  frame_124_stepping_4,
  dx(2),
  frame_129_stepping_9,
  frame_130_stepping_10,
  frame_131_stepping_11,
  frame_132_stepping_12,
  jmp(stand), // goto "stand"

  LABEL(step5) // step forward 5 pixels
  act(actions_1_run_jump),
  frame_121_stepping_1,
  dx(1),
  frame_122_stepping_2,
  dx(1),
  frame_123_stepping_3,
  dx(2),
  frame_124_stepping_4,
  dx(1),
  frame_129_stepping_9,
  frame_130_stepping_10,
  frame_131_stepping_11,
  frame_132_stepping_12,
  jmp(stand), // goto "stand"

  LABEL(step4) // step forward 4 pixels
  act(actions_1_run_jump),
  frame_121_stepping_1,
  dx(1),
  frame_122_stepping_2,
  dx(1),
  frame_123_stepping_3,
  dx(2),
  frame_131_stepping_11,
  frame_132_stepping_12,
  jmp(stand), // goto "stand"

  LABEL(step3) // step forward 3 pixels
  act(actions_1_run_jump),
  frame_121_stepping_1,
  dx(1),
  frame_122_stepping_2,
  dx(1),
  frame_123_stepping_3,
  dx(1),
  frame_131_stepping_11,
  frame_132_stepping_12,
  jmp(stand), // goto "stand"

  LABEL(step2) // step forward 2 pixels
  act(actions_1_run_jump),
  frame_121_stepping_1,
  dx(1),
  frame_122_stepping_2,
  dx(1),
  frame_132_stepping_12,
  jmp(stand), // goto "stand"

  LABEL(step1) // step forward 1 pixel
  act(actions_1_run_jump),
  frame_121_stepping_1,
  dx(1),
  frame_132_stepping_12,
  jmp(stand), // goto "stand"

  LABEL(stoop) // stoop
  act(actions_1_run_jump),
  dx(1),
  frame_107_fall_land_1,
  dx(2),
  frame_108_fall_land_2,
  /* ":crouch" */ LABEL(stoop_crouch) frame_109_crouch,
  jmp(stoop_crouch), // goto ":crouch

  LABEL(standup) // stand up
  act(actions_5_bumped),
  dx(1),
  frame_110_stand_up_from_crouch_1,
  frame_111_stand_up_from_crouch_2,
  dx(2),
  frame_112_stand_up_from_crouch_3,
  frame_113_stand_up_from_crouch_4,
  dx(1),
  frame_114_stand_up_from_crouch_5,
  frame_115_stand_up_from_crouch_6,
  frame_116_stand_up_from_crouch_7,
  dx(-4),
  frame_117_stand_up_from_crouch_8,
  frame_118_stand_up_from_crouch_9,
  frame_119_stand_up_from_crouch_10,
  jmp(stand), // goto "stand"

  LABEL(pickupsword) // pick up sword
  act(actions_1_run_jump),
  SEQ_GET_ITEM,
  1,
  frame_229_found_sword,
  frame_229_found_sword,
  frame_229_found_sword,
  frame_229_found_sword,
  frame_229_found_sword,
  frame_229_found_sword,
  frame_230_sheathe,
  frame_231_sheathe,
  frame_232_sheathe,
  jmp(resheathe), // goto "resheathe"

  LABEL(resheathe) // resheathe
  act(actions_1_run_jump),
  dx(-5),
  frame_233_sheathe,
  frame_234_sheathe,
  frame_235_sheathe,
  frame_236_sheathe,
  frame_237_sheathe,
  frame_238_sheathe,
  frame_239_sheathe,
  frame_240_sheathe,
  frame_133_sheathe,
  frame_133_sheathe,
  frame_134_sheathe,
  frame_134_sheathe,
  frame_134_sheathe,
  frame_48_turn,
  dx(1),
  frame_49_turn,
  dx(-2),
  act(actions_5_bumped),
  frame_50_turn,
  act(actions_1_run_jump),
  frame_51_turn,
  frame_52_turn,
  jmp(stand), // goto "stand"

  LABEL(fastsheathe) // fast sheathe
  act(actions_1_run_jump),
  dx(-5),
  frame_234_sheathe,
  frame_236_sheathe,
  frame_238_sheathe,
  frame_240_sheathe,
  frame_134_sheathe,
  dx(-1),
  jmp(stand), // goto "stand"

  LABEL(drinkpotion) // drink potion
  act(actions_1_run_jump),
  dx(4),
  frame_191_drink,
  frame_192_drink,
  frame_193_drink,
  frame_194_drink,
  frame_195_drink,
  frame_196_drink,
  frame_197_drink,
  snd(SND_DRINK),
  frame_198_drink,
  frame_199_drink,
  frame_200_drink,
  frame_201_drink,
  frame_202_drink,
  frame_203_drink,
  frame_204_drink,
  frame_205_drink,
  frame_205_drink,
  frame_205_drink,
  SEQ_GET_ITEM,
  1,
  frame_205_drink,
  frame_205_drink,
  frame_201_drink,
  frame_198_drink,
  dx(-4),
  jmp(stand), // goto "stand"

  LABEL(softland) // soft land
  act(actions_5_bumped),
  SEQ_KNOCK_DOWN,
  dx(1),
  frame_107_fall_land_1,
  dx(2),
  frame_108_fall_land_2,
  act(actions_1_run_jump),
  /* ":crouch" */ LABEL(softland_crouch) frame_109_crouch,
  jmp(softland_crouch), // goto ":crouch"

  LABEL(landrun) // land run
  act(actions_1_run_jump),
  dy(-2),
  dx(1),
  frame_107_fall_land_1,
  dx(2),
  frame_108_fall_land_2,
  frame_109_crouch,
  dx(1),
  frame_110_stand_up_from_crouch_1,
  frame_111_stand_up_from_crouch_2,
  dx(2),
  frame_112_stand_up_from_crouch_3,
  frame_113_stand_up_from_crouch_4,
  dx(1),
  dy(1),
  frame_114_stand_up_from_crouch_5,
  dy(1),
  frame_115_stand_up_from_crouch_6,
  dx(-2),
  jmp(runstt4), // goto "runstt4"

  LABEL(medland) // medium land (1.5 - 2 stories)
  act(actions_5_bumped),
  SEQ_KNOCK_DOWN,
  dy(-2),
  dx(1),
  dx(2),
  frame_108_fall_land_2,
  frame_109_crouch,
  frame_109_crouch,
  frame_109_crouch,
  frame_109_crouch,
  frame_109_crouch,
  frame_109_crouch,
  frame_109_crouch,
  frame_109_crouch,
  frame_109_crouch,
  frame_109_crouch,
  frame_109_crouch,
  frame_109_crouch,
  frame_109_crouch,
  frame_109_crouch,
  frame_109_crouch,
  frame_109_crouch,
  frame_109_crouch,
  frame_109_crouch,
  frame_109_crouch,
  frame_109_crouch,
  frame_109_crouch,
  frame_109_crouch,
  frame_109_crouch,
  frame_109_crouch,
  frame_109_crouch,
  frame_109_crouch,
  frame_109_crouch,
  frame_109_crouch,
  frame_109_crouch,
  dx(1),
  frame_110_stand_up_from_crouch_1,
  frame_110_stand_up_from_crouch_1,
  frame_110_stand_up_from_crouch_1,
  frame_111_stand_up_from_crouch_2,
  dx(2),
  frame_112_stand_up_from_crouch_3,
  frame_113_stand_up_from_crouch_4,
  dx(1),
  dy(1),
  frame_114_stand_up_from_crouch_5,
  dy(1),
  frame_115_stand_up_from_crouch_6,
  frame_116_stand_up_from_crouch_7,
  dx(-4),
  frame_117_stand_up_from_crouch_8,
  frame_118_stand_up_from_crouch_9,
  frame_119_stand_up_from_crouch_10,
  jmp(stand), // goto "stand"

  LABEL(hardland) // hard land (splat!; >2 stories)
  act(actions_5_bumped),
  SEQ_KNOCK_DOWN,
  dy(-2),
  dx(3),
  frame_185_dead,
  SEQ_DIE,
  /* ":dead" */ LABEL(hardland_dead) frame_185_dead,
  jmp(hardland_dead), // goto ":dead"

  LABEL(stabkill) // stabkill
  act(actions_5_bumped),
  jmp(dropdead), // goto "dropdead"

  LABEL(dropdead) // dropdead
  act(actions_1_run_jump),
  SEQ_DIE,
  frame_179_collapse_1,
  frame_180_collapse_2,
  frame_181_collapse_3,
  frame_182_collapse_4,
  dx(1),
  frame_183_collapse_5,
  dx(-4),
  /* ":dead" */ LABEL(dropdead_dead) frame_185_dead,
  jmp(dropdead_dead), // goto ":dead"

  LABEL(impale) // impale
  act(actions_1_run_jump),
  SEQ_KNOCK_DOWN,
  dx(4),
  frame_177_spiked,
  SEQ_DIE,
  /* ":dead" */ LABEL(impale_dead) frame_177_spiked,
  jmp(impale_dead), // goto ":dead"

  LABEL(halve) // halve
  act(actions_1_run_jump),
  frame_178_chomped,
  SEQ_DIE,
  /* ":dead" */ LABEL(halve_dead) frame_178_chomped,
  jmp(halve_dead), // goto ":dead"

  LABEL(crush)  // crush
  jmp(medland), // goto "medland"

  LABEL(deadfall) // deadfall
  set_fall(0, 0),
  act(actions_4_in_freefall),
  /* ":loop"*/ LABEL(deadfall_loop) frame_185_dead,
  jmp(deadfall_loop), // goto ":loop"

  LABEL(climbstairs) // climb stairs
  act(actions_5_bumped),
  dx(-5),
  dy(-1),
  snd(SND_FOOTSTEP),
  frame_217_exit_stairs_1,
  frame_218_exit_stairs_2,
  frame_219_exit_stairs_3,
  dx(1),
  frame_220_exit_stairs_4,
  dx(-4),
  dy(-3),
  snd(SND_FOOTSTEP),
  frame_221_exit_stairs_5,
  dx(-4),
  dy(-2),
  frame_222_exit_stairs_6,
  dx(-2),
  dy(-3),
  frame_223_exit_stairs_7,
  dx(-3),
  dy(-8),
  snd(SND_LEVEL),
  snd(SND_FOOTSTEP),
  frame_224_exit_stairs_8,
  dx(-1),
  dy(-1),
  frame_225_exit_stairs_9,
  dx(-3),
  dy(-4),
  frame_226_exit_stairs_10,
  dx(-1),
  dy(-5),
  snd(SND_FOOTSTEP),
  frame_227_exit_stairs_11,
  dx(-2),
  dy(-1),
  frame_228_exit_stairs_12,
  frame_0,
  snd(SND_FOOTSTEP),
  frame_0,
  frame_0,
  frame_0, // these footsteps are only heard when the music is off
  snd(SND_FOOTSTEP),
  frame_0,
  frame_0,
  frame_0,
  snd(SND_FOOTSTEP),
  frame_0,
  frame_0,
  frame_0,
  snd(SND_FOOTSTEP),
  SEQ_END_LEVEL,
  /* ":loop" */ LABEL(climbstairs_loop) frame_0,
  jmp(climbstairs_loop), // goto ":loop"

  LABEL(Vstand) // Vizier: stand
  alt2frame_54_Vstand,
  jmp(Vstand), // goto "Vstand"

  LABEL(Vraise) // Vizier: raise arms
  85,
  67,
  67,
  67, // numbers refer to frames in the "alternate" frame sets
  67,
  67,
  67,
  67,
  67,
  67,
  67,
  68,
  69,
  70,
  71,
  72,
  73,
  74,
  75,
  83,
  84,
  /* ":loop" */ LABEL(Vraise_loop) 76,
  jmp(Vraise_loop), // goto ":loop"

  LABEL(Vwalk) // Vizier: walk
  dx(1),
  /* "Vwalk1" */ LABEL(Vwalk1) 48,
  dx(2),
  /* "Vwalk2" */ LABEL(Vwalk2) 49,
  dx(6),
  50,
  dx(1),
  51,
  dx(-1),
  52,
  dx(1),
  53,
  dx(1),
  jmp(Vwalk1), // goto "Vwalk1"

  LABEL(Vstop) // Vizier: stop
  dx(1),
  55,
  56,
  jmp(Vstand),

  LABEL(Vexit) // Vizier: lower arms, turn & exit ("Vexit")
  77,
  78,
  79,
  80,
  81,
  82,
  dx(1),
  54,
  54,
  54,
  54,
  54,
  54,
  57,
  58,
  59,
  60,
  61,
  dx(2),
  62,
  dx(-1),
  63,
  dx(-3),
  64,
  65,
  dx(-1),
  66,
  SEQ_FLIP,
  dx(16),
  dx(3),
  jmp(Vwalk2), // goto "Vwalk2"

  // Princess: stand
  LABEL(Pstand) 11,
  jmp(Pstand), // goto "Pstand"

  LABEL(Palert) // Princess: alert
  2,
  3,
  4,
  5,
  6,
  7,
  8,
  9,
  SEQ_FLIP,
  dx(8),
  11,
  jmp(Pstand),

  LABEL(Pstepback) // Princess: step back
  SEQ_FLIP,
  dx(11),
  12,
  dx(1),
  13,
  dx(1),
  14,
  dx(3),
  15,
  dx(1),
  16,
  /* ":loop" */ LABEL(Pstepback_loop) 17,
  jmp(Pstepback_loop), // goto ":loop"

  LABEL(Plie) // Princess lying on cushions ("Plie")
  19,
  jmp(Plie), // goto "Plie"

  LABEL(Pwaiting) // Princess: waiting
  /* ":loop" */ 20,
  jmp(Pwaiting), // goto ":loop"

  LABEL(Pembrace) // Princess: embrace
  21,
  dx(1),
  22,
  23,
  24,
  dx(1),
  25,
  dx(-3),
  26,
  dx(-2),
  27,
  dx(-4),
  28,
  dx(-3),
  29,
  dx(-2),
  30,
  dx(-3),
  31,
  dx(-1),
  32,
  /* ":loop" */ LABEL(Pembrace_loop) 33,
  jmp(Pembrace_loop), // goto ":loop"

  LABEL(Pstroke) // Princess: stroke mouse
  /* ":loop" */ 37,
  jmp(Pstroke), // goto ":loop"

  LABEL(Prise) // Princess: rise
  37,
  38,
  39,
  40,
  41,
  42,
  43,
  44,
  45,
  46,
  47,
  SEQ_FLIP,
  dx(12),
  /* ":loop" */ LABEL(Prise_loop) 11,
  jmp(Prise_loop), // goto ":loop"

  LABEL(Pcrouch) // Princess: crouch & stroke mouse
  11,
  11,
  SEQ_FLIP,
  dx(13),
  47,
  46,
  45,
  44,
  43,
  42,
  41,
  40,
  39,
  38,
  37,
  36,
  36,
  36,
  35,
  35,
  35,
  34,
  34,
  34,
  34,
  34,
  34,
  34,
  35,
  35,
  36,
  36,
  36,
  35,
  35,
  35,
  34,
  34,
  34,
  34,
  34,
  34,
  34,
  35,
  35,
  36,
  36,
  36,
  35,
  35,
  35,
  34,
  34,
  34,
  34,
  34,
  34,
  34,
  34,
  34,
  35,
  35,
  35,
  /* ":loop" */ LABEL(Pcrouch_loop) 36,
  jmp(Pcrouch_loop), // goto ":loop"

  LABEL(Pslump) // Princess: slump shoulders
  1,
  /* ":loop" */ LABEL(Pslump_loop) 18,
  jmp(Pslump_loop), // goto ":loop"

  LABEL(Mscurry) // Mouse: scurry
  act(actions_1_run_jump),
  /* "Mscurry1" */ LABEL(Mscurry1) frame_186_mouse_1,
  dx(5),
  frame_186_mouse_1,
  dx(3),
  frame_187_mouse_2,
  dx(4),
  jmp(Mscurry1), // goto "Mscurry1"

  LABEL(Mstop) // Mouse: stop
  /* ":loop" */ frame_186_mouse_1,
  jmp(Mstop), // goto ":loop"

  LABEL(Mraise) // Mouse: raise head
  /* ":loop" */ frame_188_mouse_stand,
  jmp(Mraise), // goto ":loop"

  LABEL(Mleave) // Mouse: leave
  act(actions_0_stand),
  frame_186_mouse_1,
  frame_186_mouse_1,
  frame_186_mouse_1,
  frame_188_mouse_stand,
  frame_188_mouse_stand,
  frame_188_mouse_stand,
  frame_188_mouse_stand,
  frame_188_mouse_stand,
  frame_188_mouse_stand,
  frame_188_mouse_stand,
  frame_188_mouse_stand,
  SEQ_FLIP,
  dx(8),
  jmp(Mscurry1), // goto "Mscurry1"

  LABEL(Mclimb) // Mouse: climb
  frame_186_mouse_1,
  frame_186_mouse_1,
  /* ":loop" */ LABEL(Mclimb_loop) frame_188_mouse_stand,
  jmp(Mclimb_loop) // goto ":loop"

};
extern const byte seqtbl[]; // the sequence table is defined in seqtbl.c
extern const word seqtbl_offsets[];
static const byte optgraf_min[] = {0x01, 0x1E, 0x4B, 0x4E, 0x56, 0x65, 0x7F, 0x0A};
static const byte optgraf_max[] = {0x09, 0x1F, 0x4D, 0x53, 0x5B, 0x7B, 0x8F, 0x0D};

static const char *const tbl_guard_dat[] = {"GUARD.DAT", "FAT.DAT", "SKEL.DAT", "VIZIER.DAT", "SHADOW.DAT"};
static const char *const tbl_envir_gr[] = {"", "C", "C", "E", "E", "V"};
static const char *const tbl_envir_ki[] = {"DUNGEON", "PALACE"};
static const rect_type rect_titles = {106, 24, 195, 296};

static const short y_something[] = {-1, 62, 125, 188, 25};
static const byte loose_sound[] = {0, 1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0};
static const word y_loose_land[] = {2, 65, 128, 191, 254};
static const byte leveldoor_close_speeds[] = {0, 5, 17, 99, 0};
static const byte gate_close_speeds[] = {0, 0, 0, 20, 40, 60, 80, 100, 120};
static const sbyte door_delta[] = {-1, 4, 4};
static const byte door_fram_slice[] = {67, 59, 58, 57, 56, 55, 54, 53, 52};
static const word floor_left_overlay[] = {32, 151, 151, 150, 150, 151, 32, 32};
static const byte spikes_fram_fore[] = {0, 139, 140, 141, 142, 143, 142, 140, 139, 0};
static const byte chomper_fram_for[] = {106, 107, 108, 109, 110, 0};
static const byte wall_fram_main[] = {8, 10, 6, 4};
static const byte spikes_fram_left[] = {0, 128, 129, 130, 131, 132, 131, 129, 128, 0};
static const byte potion_fram_bubb[] = {0, 16, 17, 18, 19, 20, 21, 22};
static const byte chomper_fram1[] = {3, 2, 0, 1, 4, 3, 3, 0};
static const byte chomper_fram_bot[] = {101, 102, 103, 104, 105, 0};
static const byte chomper_fram_top[] = {0, 0, 111, 112, 113, 0};
static const byte chomper_fram_y[] = {0, 0, 0x25, 0x2F, 0x32};
static const byte loose_fram_left[] = {41, 69, 41, 70, 70, 41, 41, 41, 70, 70, 70, 0};
static const byte loose_fram_bottom[] = {43, 73, 43, 74, 74, 43, 43, 43, 74, 74, 74, 0};
static const byte wall_fram_bottom[] = {7, 9, 5, 3};
static const byte spikes_fram_right[] = {0, 134, 135, 136, 137, 138, 137, 135, 134, 0};
static const byte loose_fram_right[] = {42, 71, 42, 72, 72, 42, 42, 42, 72, 72, 72, 0};
static const byte blueline_fram1[] = {0, 124, 125, 126};
static const sbyte blueline_fram_y[] = {0, -20, -20, 0};
static const byte blueline_fram3[] = {44, 44, 45, 45};
static const byte doortop_fram_bot[] = {78, 80, 82, 0};
static const byte door_fram_top[] = {60, 61, 62, 63, 64, 65, 66, 67};
static const byte doortop_fram_top[] = {0, 81, 83, 0};
static const word col_xh[] = {0, 4, 8, 12, 16, 20, 24, 28, 32, 36};

static const piece tile_table[31] = {
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},        // 0x00 empty
  {41, 1, 0, 42, 1, 2, 145, 0, 43, 0, 0, 0},   // 0x01 floor
  {127, 1, 0, 133, 1, 2, 145, 0, 43, 0, 0, 0}, // 0x02 spike
  {92, 1, 0, 93, 1, 2, 0, 94, 43, 95, 1, 0},   // 0x03 pillar
  {46, 1, 0, 47, 1, 2, 0, 48, 43, 49, 3, 0},   // 0x04 door
  {41, 1, 1, 35, 1, 3, 145, 0, 36, 0, 0, 0},   // 0x05 stuck floor
  {41, 1, 0, 42, 1, 2, 145, 0, 96, 0, 0, 0},   // 0x06 close button
  {46, 1, 0, 0, 0, 2, 0, 0, 43, 49, 3, 0},     // 0x07 door top with floor
  {86, 1, 0, 87, 1, 2, 0, 0, 43, 88, 1, 0},    // 0x08 big pillar bottom
  {0, 0, 0, 89, 0, 3, 0, 90, 0, 91, 1, 3},     // 0x09 big pillar top
  {41, 1, 0, 42, 1, 2, 145, 0, 43, 12, 2, -3}, // 0x0A potion
  {0, 1, 0, 0, 0, 0, 145, 0, 0, 0, 0, 0},      // 0x0B loose floor
  {0, 0, 0, 0, 0, 2, 0, 0, 85, 49, 3, 0},      // 0x0C door top
  {75, 1, 0, 42, 1, 2, 0, 0, 43, 77, 0, 0},    // 0x0D mirror
  {97, 1, 0, 98, 1, 2, 145, 0, 43, 100, 0, 0}, // 0x0E debris
  {147, 1, 0, 42, 1, 1, 145, 0, 149, 0, 0, 0}, // 0x0F open button
  {41, 1, 0, 37, 0, 0, 0, 38, 43, 0, 0, 0},    // 0x10 leveldoor left
  {0, 0, 0, 39, 1, 2, 0, 40, 43, 0, 0, 0},     // 0x11 leveldoor right
  {0, 0, 0, 42, 1, 2, 145, 0, 43, 0, 0, 0},    // 0x12 chomper
  {41, 1, 0, 42, 1, 2, 0, 0, 43, 0, 0, 0},     // 0x13 torch
  {0, 0, 0, 1, 1, 2, 0, 2, 0, 0, 0, 0},        // 0x14 wall
  {30, 1, 0, 31, 1, 2, 0, 0, 43, 0, 0, 0},     // 0x15 skeleton
  {41, 1, 0, 42, 1, 2, 145, 0, 43, 0, 0, 0},   // 0x16 sword
  {41, 1, 0, 10, 0, 0, 0, 11, 43, 0, 0, 0},    // 0x17 balcony left
  {0, 0, 0, 12, 1, 2, 0, 13, 43, 0, 0, 0},     // 0x18 balcony right
  {92, 1, 0, 42, 1, 2, 145, 0, 43, 95, 1, 0},  // 0x19 lattice pillar
  {1, 0, 0, 0, 0, 0, 0, 0, 2, 9, 0, -53},      // 0x1A lattice down
  {3, 0, -10, 0, 0, 0, 0, 0, 0, 9, 0, -53},    // 0x1B lattice small
  {4, 0, -10, 0, 0, 0, 0, 0, 0, 9, 0, -53},    // 0x1C lattice left
  {5, 0, -10, 0, 0, 0, 0, 0, 0, 9, 0, -53},    // 0x1D lattice right
  {97, 1, 0, 98, 1, 2, 0, 0, 43, 100, 0, 0},   // 0x1E debris with torch
};

static const byte sound_prio_table[] = {
  0x14, // sound_0_fell_to_death
  0x1E, // sound_1_falling
  0x23, // sound_2_tile_crashing
  0x66, // sound_3_button_pressed
  0x32, // sound_4_gate_closing
  0x37, // sound_5_gate_opening
  0x30, // sound_6_gate_closing_fast
  0x30, // sound_7_gate_stop
  0x4B, // sound_8_bumped
  0x50, // sound_9_grab
  0x0A, // sound_10_sword_vs_sword
  0x12, // sound_11_sword_moving
  0x0C, // sound_12_guard_hurt
  0x0B, // sound_13_kid_hurt
  0x69, // sound_14_leveldoor_closing
  0x6E, // sound_15_leveldoor_sliding
  0x73, // sound_16_medium_land
  0x78, // sound_17_soft_land
  0x7D, // sound_18_drink
  0x82, // sound_19_draw_sword
  0x91, // sound_20_loose_shake_1
  0x96, // sound_21_loose_shake_2
  0x9B, // sound_22_loose_shake_3
  0xA0, // sound_23_footstep
  0x01, // sound_24_death_regular
  0x01, // sound_25_presentation
  0x01, // sound_26_embrace
  0x01, // sound_27_cutscene_2_4_6_12
  0x01, // sound_28_death_in_fight
  0x13, // sound_29_meet_Jaffar
  0x01, // sound_30_big_potion
  0x01, // sound_31
  0x01, // sound_32_shadow_music
  0x01, // sound_33_small_potion
  0x01, // sound_34
  0x01, // sound_35_cutscene_8_9
  0x01, // sound_36_out_of_time
  0x01, // sound_37_victory
  0x01, // sound_38_blink
  0x00, // sound_39_low_weight
  0x01, // sound_40_cutscene_12_short_time
  0x01, // sound_41_end_level_music
  0x01, // sound_42
  0x01, // sound_43_victory_Jaffar
  0x87, // sound_44_skel_alive
  0x8C, // sound_45_jump_through_mirror
  0x0F, // sound_46_chomped
  0x10, // sound_47_chomper
  0x19, // sound_48_spiked
  0x16, // sound_49_spikes
  0x01, // sound_50_story_2_princess
  0x00, // sound_51_princess_door_opening
  0x01, // sound_52_story_4_Jaffar_leaves
  0x01, // sound_53_story_3_Jaffar_comes
  0x01, // sound_54_intro_music
  0x01, // sound_55_story_1_absence
  0x01, // sound_56_ending_music
  0x00};

static const byte sound_pcspeaker_exists[] = {
  1, // sound_0_fell_to_death
  0, // sound_1_falling
  1, // sound_2_tile_crashing
  1, // sound_3_button_pressed
  1, // sound_4_gate_closing
  1, // sound_5_gate_opening
  1, // sound_6_gate_closing_fast
  1, // sound_7_gate_stop
  1, // sound_8_bumped
  1, // sound_9_grab
  1, // sound_10_sword_vs_sword
  0, // sound_11_sword_moving
  1, // sound_12_guard_hurt
  1, // sound_13_kid_hurt
  1, // sound_14_leveldoor_closing
  1, // sound_15_leveldoor_sliding
  1, // sound_16_medium_land
  1, // sound_17_soft_land
  1, // sound_18_drink
  0, // sound_19_draw_sword
  0, // sound_20_loose_shake_1
  0, // sound_21_loose_shake_2
  0, // sound_22_loose_shake_3
  1, // sound_23_footstep
  1, // sound_24_death_regular
  1, // sound_25_presentation
  1, // sound_26_embrace
  1, // sound_27_cutscene_2_4_6_12
  1, // sound_28_death_in_fight
  1, // sound_29_meet_Jaffar
  1, // sound_30_big_potion
  1, // sound_31
  1, // sound_32_shadow_music
  1, // sound_33_small_potion
  1, // sound_34
  1, // sound_35_cutscene_8_9
  1, // sound_36_out_of_time
  1, // sound_37_victory
  1, // sound_38_blink
  1, // sound_39_low_weight
  1, // sound_40_cutscene_12_short_time
  1, // sound_41_end_level_music
  1, // sound_42
  1, // sound_43_victory_Jaffar
  1, // sound_44_skel_alive
  1, // sound_45_jump_through_mirror
  1, // sound_46_chomped
  1, // sound_47_chomper
  1, // sound_48_spiked
  1, // sound_49_spikes
  1, // sound_50_story_2_princess
  1, // sound_51_princess_door_opening
  1, // sound_52_story_4_Jaffar_leaves
  1, // sound_53_story_3_Jaffar_comes
  1, // sound_54_intro_music
  1, // sound_55_story_1_absence
  1, // sound_56_ending_music
  0};

static char const *const tbl_quotes[2] = {
  "\"(****/****) Incredibly realistic. . . The "
  "adventurer character actually looks human as he "
  "runs, jumps, climbs, and hangs from ledges.\"\n"
  "\n"
  "                                  Computer Entertainer\n"
  "\n"
  "\n"
  "\n"
  "\n"
  "\"A tremendous achievement. . . Mechner has crafted "
  "the smoothest animation ever seen in a game of this "
  "type.\n"
  "\n"
  "\"PRINCE OF PERSIA is the STAR WARS of its field.\"\n"
  "\n"
  "                                  Computer Gaming World",
  "\"An unmitigated delight. . . comes as close to "
  "(perfection) as any arcade game has come in a long, "
  "long time. . . what makes this game so wonderful (am "
  "I gushing?) is that the little onscreen character "
  "does not move like a little onscreen character -- he "
  "moves like a person.\"\n"
  "\n"
  "                                      Nibble"};

#ifdef USE_COPYPROT
// data:017A
static const word copyprot_word[] = {9, 1, 6, 4, 5, 3, 6, 3, 4, 4, 3, 2, 12, 5, 13, 1, 9, 2, 2, 4, 9, 4, 11, 8, 5, 4, 1, 6, 2, 4, 6, 8, 4, 2, 7, 11, 5, 4, 1, 2};
// data:012A
static const word copyprot_line[] = {2, 1, 5, 4, 3, 5, 1, 3, 7, 2, 2, 4, 6, 6, 2, 6, 3, 1, 2, 3, 2, 2, 3, 10, 5, 6, 5, 6, 3, 5, 7, 2, 2, 4, 5, 7, 2, 6, 5, 5};
// data:00DA
static const word copyprot_page[] = {5, 3, 7, 3, 3, 4, 1, 5, 12, 5, 11, 10, 1, 2, 8, 8, 2, 4, 6, 1, 4, 7, 3, 2, 1, 7, 10, 1, 4, 3, 4, 1, 4, 1, 8, 1, 1, 10, 3, 3};
#endif

static const sbyte wall_dist_from_left[] = {0, 10, 0, -1, 0, 0};
static const sbyte wall_dist_from_right[] = {0, 0, 10, 13, 0, 0};

// data:1712
static const sword_table_type sword_tbl[] = {
  {255, 0, 0},
  {0, 0, -9},
  {5, -9, -29},
  {1, 7, -25},
  {2, 17, -26},
  {6, 7, -14},
  {7, 0, -5},
  {3, 17, -16},
  {4, 16, -19},
  {30, 12, -9},
  {8, 13, -34},
  {9, 7, -25},
  {10, 10, -16},
  {11, 10, -11},
  {12, 22, -21},
  {13, 28, -23},
  {14, 13, -35},
  {15, 0, -38},
  {16, 0, -29},
  {17, 21, -19},
  {18, 14, -23},
  {19, 21, -22},
  {19, 22, -23},
  {17, 7, -13},
  {17, 15, -18},
  {7, 0, -8},
  {1, 7, -27},
  {28, 14, -28},
  {8, 7, -27},
  {4, 6, -23},
  {4, 9, -21},
  {10, 11, -18},
  {13, 24, -23},
  {13, 19, -23},
  {13, 21, -23},
  {20, 7, -32},
  {21, 14, -32},
  {22, 14, -31},
  {23, 14, -29},
  {24, 28, -28},
  {25, 28, -28},
  {26, 21, -25},
  {27, 14, -22},
  {255, 14, -25},
  {255, 21, -25},
  {29, 0, -16},
  {8, 8, -37},
  {31, 14, -24},
  {32, 14, -24},
  {33, 7, -14},
  {8, 8, -37},
};

// data:22A6
static const sbyte tile_div_tbl[256] = {
  -5, -5, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 14, 14};

// data:23A6
static const byte tile_mod_tbl[256] = {
  12, 13, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0, 1};

// data:0FE0
static const frame_type frame_table_kid[] = {
  {255, 0x00 | 0, 0, 0, 0x00 | 0},
  {0, 0x00 | 0, 1, 0, 0xC0 | 4},
  {1, 0x00 | 0, 1, 0, 0x40 | 4},
  {2, 0x00 | 0, 3, 0, 0x40 | 7},
  {3, 0x00 | 0, 4, 0, 0x40 | 8},
  {4, 0x00 | 0, 0, 0, 0xE0 | 6},
  {5, 0x00 | 0, 0, 0, 0x40 | 9},
  {6, 0x00 | 0, 0, 0, 0x40 | 10},
  {7, 0x00 | 0, 0, 0, 0xC0 | 5},
  {8, 0x00 | 0, 0, 0, 0x40 | 4},
  {9, 0x00 | 0, 0, 0, 0x40 | 7},
  {10, 0x00 | 0, 0, 0, 0x40 | 11},
  {11, 0x00 | 0, 0, 0, 0x40 | 3},
  {12, 0x00 | 0, 0, 0, 0xC0 | 3},
  {13, 0x00 | 0, 0, 0, 0x40 | 7},
  {14, 0x00 | 9, 0, 0, 0x40 | 3},
  {15, 0x00 | 0, 0, 0, 0xC0 | 3},
  {16, 0x00 | 0, 0, 0, 0x40 | 4},
  {17, 0x00 | 0, 0, 0, 0x40 | 6},
  {18, 0x00 | 0, 0, 0, 0x40 | 8},
  {19, 0x00 | 0, 0, 0, 0x80 | 9},
  {20, 0x00 | 0, 0, 0, 0x00 | 11},
  {21, 0x00 | 0, 0, 0, 0x80 | 11},
  {22, 0x00 | 0, 0, 0, 0x00 | 17},
  {23, 0x00 | 0, 0, 0, 0x00 | 7},
  {24, 0x00 | 0, 0, 0, 0x00 | 5},
  {25, 0x00 | 0, 0, 0, 0xC0 | 1},
  {26, 0x00 | 0, 0, 0, 0xC0 | 6},
  {27, 0x00 | 0, 0, 0, 0x40 | 3},
  {28, 0x00 | 0, 0, 0, 0x40 | 8},
  {29, 0x00 | 0, 0, 0, 0x40 | 2},
  {30, 0x00 | 0, 0, 0, 0x40 | 2},
  {31, 0x00 | 0, 0, 0, 0xC0 | 2},
  {32, 0x00 | 0, 0, 0, 0xC0 | 2},
  {33, 0x00 | 0, 0, 0, 0x40 | 3},
  {34, 0x00 | 0, 0, 0, 0x40 | 8},
  {35, 0x00 | 0, 0, 0, 0xC0 | 14},
  {36, 0x00 | 0, 0, 0, 0xC0 | 1},
  {37, 0x00 | 0, 0, 0, 0x40 | 5},
  {38, 0x00 | 0, 0, 0, 0x80 | 14},
  {39, 0x00 | 0, 0, 0, 0x00 | 11},
  {40, 0x00 | 0, 0, 0, 0x80 | 11},
  {41, 0x00 | 0, 0, 0, 0x80 | 10},
  {42, 0x00 | 0, 0, 0, 0x00 | 1},
  {43, 0x00 | 0, 0, 0, 0xC0 | 4},
  {44, 0x00 | 0, 0, 0, 0xC0 | 3},
  {45, 0x00 | 0, 0, 0, 0xC0 | 3},
  {46, 0x00 | 0, 0, 0, 0xA0 | 5},
  {47, 0x00 | 0, 0, 0, 0xA0 | 4},
  {48, 0x00 | 0, 0, 0, 0x60 | 6},
  {49, 0x00 | 0, 4, 0, 0x60 | 7},
  {50, 0x00 | 0, 3, 0, 0x60 | 6},
  {51, 0x00 | 0, 1, 0, 0x40 | 4},
  {64, 0x00 | 0, 0, 0, 0xC0 | 2},
  {65, 0x00 | 0, 0, 0, 0x40 | 1},
  {66, 0x00 | 0, 0, 0, 0x40 | 2},
  {67, 0x00 | 0, 0, 0, 0x00 | 0},
  {68, 0x00 | 0, 0, 0, 0x00 | 0},
  {69, 0x00 | 0, 0, 0, 0x80 | 0},
  {70, 0x00 | 0, 0, 0, 0x00 | 0},
  {71, 0x00 | 0, 0, 0, 0x80 | 0},
  {72, 0x00 | 0, 0, 0, 0x00 | 0},
  {73, 0x00 | 0, 0, 0, 0x80 | 0},
  {74, 0x00 | 0, 0, 0, 0x00 | 0},
  {75, 0x00 | 0, 0, 0, 0x00 | 0},
  {76, 0x00 | 0, 0, 0, 0x80 | 0},
  {255, 0x00 | 0, 0, 0, 0x00 | 0},
  {80, 0x00 | 0, -2, 0, 0x40 | 1},
  {81, 0x00 | 0, -2, 0, 0x40 | 1},
  {82, 0x00 | 0, -1, 0, 0xC0 | 2},
  {83, 0x00 | 0, -2, 0, 0x40 | 2},
  {84, 0x00 | 0, -2, 0, 0x40 | 1},
  {85, 0x00 | 0, -2, 0, 0x40 | 1},
  {86, 0x00 | 0, -2, 0, 0x40 | 1},
  {87, 0x00 | 0, -1, 0, 0x00 | 7},
  {88, 0x00 | 0, -1, 0, 0x00 | 5},
  {89, 0x00 | 0, 2, 0, 0x00 | 7},
  {90, 0x00 | 0, 2, 0, 0x00 | 7},
  {91, 0x00 | 0, 2, -3, 0x00 | 0},
  {92, 0x00 | 0, 2, -10, 0x00 | 0},
  {93, 0x00 | 0, 2, -11, 0x80 | 0},
  {94, 0x00 | 0, 3, -2, 0x40 | 3},
  {95, 0x00 | 0, 3, 0, 0xC0 | 3},
  {96, 0x00 | 0, 3, 0, 0xC0 | 3},
  {97, 0x00 | 0, 3, 0, 0x60 | 3},
  {98, 0x00 | 0, 4, 0, 0xE0 | 3},
  {28, 0x00 | 0, 0, 0, 0x00 | 0},
  {99, 0x00 | 0, 7, -14, 0x80 | 0},
  {100, 0x00 | 0, 7, -12, 0x80 | 0},
  {101, 0x00 | 0, 4, -12, 0x00 | 0},
  {102, 0x00 | 0, 3, -10, 0x80 | 0},
  {103, 0x00 | 0, 2, -10, 0x80 | 0},
  {104, 0x00 | 0, 1, -10, 0x80 | 0},
  {105, 0x00 | 0, 0, -11, 0x00 | 0},
  {106, 0x00 | 0, -1, -12, 0x00 | 0},
  {107, 0x00 | 0, -1, -14, 0x00 | 0},
  {108, 0x00 | 0, -1, -14, 0x00 | 0},
  {109, 0x00 | 0, -1, -15, 0x80 | 0},
  {110, 0x00 | 0, -1, -15, 0x80 | 0},
  {111, 0x00 | 0, 0, -15, 0x00 | 0},
  {255, 0x00 | 0, 0, 0, 0x00 | 0},
  {255, 0x00 | 0, 0, 0, 0x00 | 0},
  {112, 0x00 | 0, 0, 0, 0xC0 | 6},
  {113, 0x00 | 0, 0, 0, 0x40 | 6},
  {114, 0x00 | 0, 0, 0, 0xC0 | 5},
  {115, 0x00 | 0, 0, 0, 0x40 | 5},
  {116, 0x00 | 0, 0, 0, 0xC0 | 2},
  {117, 0x00 | 0, 0, 0, 0xC0 | 4},
  {118, 0x00 | 0, 0, 0, 0xC0 | 5},
  {119, 0x00 | 0, 0, 0, 0x40 | 6},
  {120, 0x00 | 0, 0, 0, 0x40 | 7},
  {121, 0x00 | 0, 0, 0, 0x40 | 7},
  {122, 0x00 | 0, 0, 0, 0x40 | 9},
  {123, 0x00 | 0, 0, 0, 0xC0 | 8},
  {124, 0x00 | 0, 0, 0, 0xC0 | 9},
  {125, 0x00 | 0, 0, 0, 0x40 | 9},
  {126, 0x00 | 0, 0, 0, 0x40 | 5},
  {127, 0x00 | 0, 2, 0, 0x40 | 5},
  {128, 0x00 | 0, 2, 0, 0xC0 | 5},
  {129, 0x00 | 0, 0, 0, 0xC0 | 3},
  {255, 0x00 | 0, 0, 0, 0x00 | 0},
  {133, 0x00 | 0, 0, 0, 0x40 | 3},
  {134, 0x00 | 0, 0, 0, 0xC0 | 4},
  {135, 0x00 | 0, 0, 0, 0xC0 | 5},
  {136, 0x00 | 0, 0, 0, 0x40 | 8},
  {137, 0x00 | 0, 0, 0, 0x60 | 12},
  {138, 0x00 | 0, 0, 0, 0xE0 | 15},
  {139, 0x00 | 0, 0, 0, 0x60 | 3},
  {140, 0x00 | 0, 0, 0, 0xC0 | 3},
  {141, 0x00 | 0, 0, 0, 0x40 | 3},
  {142, 0x00 | 0, 0, 0, 0x40 | 3},
  {143, 0x00 | 0, 0, 0, 0x40 | 4},
  {144, 0x00 | 0, 0, 0, 0x40 | 4},
  {172, 0x00 | 0, 0, 1, 0xC0 | 1},
  {173, 0x00 | 0, 0, 1, 0xC0 | 7},
  {145, 0x00 | 0, 0, -12, 0x00 | 1},
  {146, 0x00 | 0, 0, -21, 0x00 | 0},
  {147, 0x00 | 0, 1, -26, 0x80 | 0},
  {148, 0x00 | 0, 4, -32, 0x80 | 0},
  {149, 0x00 | 0, 6, -36, 0x80 | 1},
  {150, 0x00 | 0, 7, -41, 0x80 | 2},
  {151, 0x00 | 0, 2, 17, 0x40 | 2},
  {152, 0x00 | 0, 4, 9, 0xC0 | 4},
  {153, 0x00 | 0, 4, 5, 0xC0 | 9},
  {154, 0x00 | 0, 4, 4, 0xC0 | 8},
  {155, 0x00 | 0, 5, 0, 0x60 | 9},
  {156, 0x00 | 0, 5, 0, 0xE0 | 9},
  {157, 0x00 | 0, 5, 0, 0xE0 | 8},
  {158, 0x00 | 0, 5, 0, 0x60 | 9},
  {159, 0x00 | 0, 5, 0, 0x60 | 9},
  {184, 0x00 | 16, 0, 2, 0x80 | 0},
  {174, 0x00 | 26, 0, 2, 0x80 | 0},
  {175, 0x00 | 18, 3, 2, 0x00 | 0},
  {176, 0x00 | 22, 7, 2, 0xC0 | 4},
  {177, 0x00 | 21, 10, 2, 0x00 | 0},
  {178, 0x00 | 23, 7, 2, 0x80 | 0},
  {179, 0x00 | 25, 4, 2, 0x80 | 0},
  {180, 0x00 | 24, 0, 2, 0xC0 | 14},
  {181, 0x00 | 15, 0, 2, 0xC0 | 13},
  {182, 0x00 | 20, 3, 2, 0x00 | 0},
  {183, 0x00 | 31, 3, 2, 0x00 | 0},
  {184, 0x00 | 16, 0, 2, 0x80 | 0},
  {185, 0x00 | 17, 0, 2, 0x80 | 0},
  {186, 0x00 | 32, 0, 2, 0x00 | 0},
  {187, 0x00 | 33, 0, 2, 0x80 | 0},
  {188, 0x00 | 34, 2, 2, 0xC0 | 3},
  {14, 0x00 | 0, 0, 0, 0x40 | 3},
  {189, 0x00 | 19, 7, 2, 0x80 | 0},
  {190, 0x00 | 14, 1, 2, 0x80 | 0},
  {191, 0x00 | 27, 0, 2, 0x80 | 0},
  {181, 0x00 | 15, 0, 2, 0xC0 | 13},
  {181, 0x00 | 15, 0, 2, 0xC0 | 13},
  {112, 0x00 | 43, 0, 0, 0xC0 | 6},
  {113, 0x00 | 44, 0, 0, 0x40 | 6},
  {114, 0x00 | 45, 0, 0, 0xC0 | 5},
  {115, 0x00 | 46, 0, 0, 0x40 | 5},
  {114, 0x00 | 0, 0, 0, 0xC0 | 5},
  {78, 0x00 | 0, 0, 3, 0x80 | 10},
  {77, 0x00 | 0, 4, 3, 0x80 | 7},
  {211, 0x00 | 0, 0, 1, 0x40 | 4},
  {212, 0x00 | 0, 0, 1, 0x40 | 4},
  {213, 0x00 | 0, 0, 1, 0x40 | 4},
  {214, 0x00 | 0, 0, 1, 0x40 | 7},
  {215, 0x00 | 0, 0, 7, 0x40 | 11},
  {255, 0x00 | 0, 0, 0, 0x00 | 0},
  {79, 0x00 | 0, 4, 7, 0x40 | 9},
  {130, 0x00 | 0, 0, 0, 0x40 | 4},
  {131, 0x00 | 0, 0, 0, 0x40 | 4},
  {132, 0x00 | 0, 0, 2, 0x40 | 4},
  {255, 0x00 | 0, 0, 0, 0x00 | 0},
  {255, 0x00 | 0, 0, 0, 0x00 | 0},
  {192, 0x00 | 0, 0, 0, 0x00 | 0},
  {193, 0x00 | 0, 0, 1, 0x00 | 0},
  {194, 0x00 | 0, 0, 0, 0x80 | 0},
  {195, 0x00 | 0, 0, 0, 0x00 | 0},
  {196, 0x00 | 0, -1, 0, 0x00 | 0},
  {197, 0x00 | 0, -1, 0, 0x00 | 0},
  {198, 0x00 | 0, -1, 0, 0x00 | 0},
  {199, 0x00 | 0, -4, 0, 0x00 | 0},
  {200, 0x00 | 0, -4, 0, 0x80 | 0},
  {201, 0x00 | 0, -4, 0, 0x00 | 0},
  {202, 0x00 | 0, -4, 0, 0x00 | 0},
  {203, 0x00 | 0, -4, 0, 0x00 | 0},
  {204, 0x00 | 0, -4, 0, 0x00 | 0},
  {205, 0x00 | 0, -5, 0, 0x00 | 0},
  {206, 0x00 | 0, -5, 0, 0x00 | 0},
  {255, 0x00 | 0, 0, 0, 0x00 | 0},
  {207, 0x00 | 0, 0, 1, 0x40 | 6},
  {208, 0x00 | 0, 0, 1, 0xC0 | 6},
  {209, 0x00 | 0, 0, 1, 0xC0 | 8},
  {210, 0x00 | 0, 0, 1, 0x40 | 10},
  {255, 0x00 | 0, 0, 0, 0x00 | 0},
  {255, 0x00 | 0, 0, 0, 0x00 | 0},
  {255, 0x00 | 0, 0, 0, 0x00 | 0},
  {255, 0x00 | 0, 0, 0, 0x00 | 0},
  {255, 0x00 | 0, 0, 0, 0x00 | 0},
  {255, 0x00 | 0, 0, 0, 0x00 | 0},
  {52, 0x00 | 0, 0, 0, 0x80 | 0},
  {53, 0x00 | 0, 0, 0, 0x00 | 0},
  {54, 0x00 | 0, 0, 0, 0x00 | 0},
  {55, 0x00 | 0, 0, 0, 0x00 | 0},
  {56, 0x00 | 0, 0, 0, 0x80 | 0},
  {57, 0x00 | 0, 0, 0, 0x00 | 0},
  {58, 0x00 | 0, 0, 0, 0x00 | 0},
  {59, 0x00 | 0, 0, 0, 0x00 | 0},
  {60, 0x00 | 0, 0, 0, 0x80 | 0},
  {61, 0x00 | 0, 0, 0, 0x00 | 0},
  {62, 0x00 | 0, 0, 0, 0x80 | 0},
  {63, 0x00 | 0, 0, 0, 0x00 | 0},
  {160, 0x00 | 35, 1, 1, 0xC0 | 3},
  {161, 0x00 | 36, 0, 1, 0x40 | 9},
  {162, 0x00 | 37, 0, 1, 0xC0 | 3},
  {163, 0x00 | 38, 0, 1, 0x40 | 9},
  {164, 0x00 | 39, 0, 1, 0xC0 | 3},
  {165, 0x00 | 40, 1, 1, 0x40 | 9},
  {166, 0x00 | 41, 1, 1, 0x40 | 3},
  {167, 0x00 | 42, 1, 1, 0xC0 | 9},
  {168, 0x00 | 0, 4, 1, 0xC0 | 6},
  {169, 0x00 | 0, 3, 1, 0xC0 | 10},
  {170, 0x00 | 0, 1, 1, 0x40 | 3},
  {171, 0x00 | 0, 1, 1, 0xC0 | 8},
};

// data:1496
static const frame_type frame_tbl_guard[] = {
  {255, 0x00 | 0, 0, 0, 0x00 | 0},
  {12, 0xC0 | 13, 2, 1, 0x00 | 0},
  {2, 0xC0 | 1, 3, 1, 0x00 | 0},
  {3, 0xC0 | 2, 4, 1, 0x00 | 0},
  {4, 0xC0 | 3, 7, 1, 0x40 | 4},
  {5, 0xC0 | 4, 10, 1, 0x00 | 0},
  {6, 0xC0 | 5, 7, 1, 0x80 | 0},
  {7, 0xC0 | 6, 4, 1, 0x80 | 0},
  {8, 0xC0 | 7, 0, 1, 0x80 | 0},
  {9, 0xC0 | 8, 0, 1, 0xC0 | 13},
  {10, 0xC0 | 11, 7, 1, 0x80 | 0},
  {11, 0xC0 | 12, 3, 1, 0x00 | 0},
  {12, 0xC0 | 13, 2, 1, 0x00 | 0},
  {13, 0xC0 | 0, 2, 1, 0x00 | 0},
  {14, 0xC0 | 28, 0, 1, 0x00 | 0},
  {15, 0xC0 | 29, 0, 1, 0x80 | 0},
  {16, 0xC0 | 30, 2, 1, 0xC0 | 3},
  {17, 0xC0 | 9, -1, 1, 0x40 | 8},
  {18, 0xC0 | 10, 7, 1, 0x80 | 0},
  {19, 0xC0 | 14, 3, 1, 0x80 | 0},
  {9, 0xC0 | 8, 0, 1, 0x80 | 0},
  {20, 0xC0 | 8, 0, 1, 0xC0 | 13},
  {21, 0xC0 | 8, 0, 1, 0xC0 | 13},
  {22, 0xC0 | 47, 0, 0, 0xC0 | 6},
  {23, 0xC0 | 48, 0, 0, 0x40 | 6},
  {24, 0xC0 | 49, 0, 0, 0xC0 | 5},
  {24, 0xC0 | 49, 0, 0, 0xC0 | 5},
  {24, 0xC0 | 49, 0, 0, 0xC0 | 5},
  {26, 0xC0 | 0, 0, 3, 0x80 | 10},
  {27, 0xC0 | 0, 4, 4, 0x80 | 7},
  {28, 0xC0 | 0, -2, 1, 0x40 | 4},
  {29, 0xC0 | 0, -2, 1, 0x40 | 4},
  {30, 0xC0 | 0, -2, 1, 0x40 | 4},
  {31, 0xC0 | 0, -2, 2, 0x40 | 7},
  {32, 0xC0 | 0, -2, 2, 0x40 | 10},
  {255, 0x00 | 0, 0, 0, 0x00 | 0},
  {33, 0xC0 | 0, 3, 4, 0xC0 | 9},
  {255, 0x00 | 0, 0, 0, 0x00 | 0},
  {255, 0x00 | 0, 0, 0, 0x00 | 0},
  {255, 0x00 | 0, 0, 0, 0x00 | 0},
  {255, 0x00 | 0, 0, 0, 0x00 | 0},
};

// data:1564
static const frame_type frame_tbl_cuts[] = {
  {255, 0x00 | 0, 0, 0, 0x00 | 0},
  {15, 0x40 | 0, 0, 0, 0x00 | 0},
  {1, 0x40 | 0, 0, 0, 0x80 | 0},
  {2, 0x40 | 0, 0, 0, 0x80 | 0},
  {3, 0x40 | 0, 0, 0, 0x80 | 0},
  {4, 0x40 | 0, -1, 0, 0x00 | 0},
  {5, 0x40 | 0, 2, 0, 0x80 | 0},
  {6, 0x40 | 0, 2, 0, 0x00 | 0},
  {7, 0x40 | 0, 0, 0, 0x80 | 0},
  {8, 0x40 | 0, 1, 0, 0x80 | 0},
  {255, 0x00 | 0, 0, 0, 0x00 | 0},
  {0, 0x40 | 0, 0, 0, 0x80 | 0},
  {9, 0x40 | 0, 0, 0, 0x80 | 0},
  {10, 0x40 | 0, 0, 0, 0x00 | 0},
  {11, 0x40 | 0, 0, 0, 0x80 | 0},
  {12, 0x40 | 0, 0, 0, 0x80 | 0},
  {13, 0x40 | 0, 0, 0, 0x80 | 0},
  {14, 0x40 | 0, 0, 0, 0x00 | 0},
  {16, 0x40 | 0, 0, 0, 0x00 | 0},
  {0, 0x80 | 0, 0, 0, 0x00 | 0},
  {2, 0x80 | 0, 0, 0, 0x00 | 0},
  {3, 0x80 | 0, 0, 0, 0x00 | 0},
  {4, 0x80 | 0, 0, 0, 0x80 | 0},
  {5, 0x80 | 0, 0, 0, 0x00 | 0},
  {6, 0x80 | 0, 0, 0, 0x80 | 0},
  {7, 0x80 | 0, 0, 0, 0x80 | 0},
  {8, 0x80 | 0, 0, 0, 0x00 | 0},
  {9, 0x80 | 0, 0, 0, 0x00 | 0},
  {10, 0x80 | 0, 0, 0, 0x00 | 0},
  {11, 0x80 | 0, 0, 0, 0x00 | 0},
  {12, 0x80 | 0, 0, 0, 0x00 | 0},
  {13, 0x80 | 0, 0, 0, 0x00 | 0},
  {14, 0x80 | 0, 0, 0, 0x00 | 0},
  {15, 0x80 | 0, 0, 0, 0x00 | 0},
  {16, 0x80 | 0, 0, 0, 0x00 | 0},
  {17, 0x80 | 0, 0, 0, 0x00 | 0},
  {18, 0x80 | 0, 0, 0, 0x00 | 0},
  {19, 0x80 | 0, 0, 0, 0x00 | 0},
  {20, 0x80 | 0, 0, 0, 0x80 | 0},
  {21, 0x80 | 0, 0, 0, 0x80 | 0},
  {22, 0x80 | 0, 1, 0, 0x00 | 0},
  {23, 0x80 | 0, -1, 0, 0x00 | 0},
  {24, 0x80 | 0, 2, 0, 0x00 | 0},
  {25, 0x80 | 0, 1, 0, 0x80 | 0},
  {26, 0x80 | 0, 0, 0, 0x80 | 0},
  {27, 0x80 | 0, 0, 0, 0x80 | 0},
  {28, 0x80 | 0, 0, 0, 0x80 | 0},
  {29, 0x80 | 0, -1, 0, 0x00 | 0},
  {0, 0x80 | 0, 0, 0, 0x80 | 0},
  {1, 0x80 | 0, 0, 0, 0x80 | 0},
  {2, 0x80 | 0, 0, 0, 0x80 | 0},
  {3, 0x80 | 0, 0, 0, 0x00 | 0},
  {4, 0x80 | 0, 0, 0, 0x00 | 0},
  {5, 0x80 | 0, 0, 0, 0x80 | 0},
  {6, 0x80 | 0, 0, 0, 0x80 | 0},
  {7, 0x80 | 0, 0, 0, 0x80 | 0},
  {8, 0x80 | 0, 0, 0, 0x80 | 0},
  {9, 0x80 | 0, 0, 0, 0x80 | 0},
  {10, 0x80 | 0, 0, 0, 0x80 | 0},
  {11, 0x80 | 0, 0, 0, 0x80 | 0},
  {12, 0x80 | 0, 0, 0, 0x80 | 0},
  {13, 0x80 | 0, 0, 0, 0x00 | 0},
  {14, 0x80 | 0, 0, 0, 0x80 | 0},
  {15, 0x80 | 0, 0, 0, 0x00 | 0},
  {16, 0x80 | 0, 0, 0, 0x00 | 0},
  {17, 0x80 | 0, 0, 0, 0x80 | 0},
  {18, 0x80 | 0, 0, 0, 0x00 | 0},
  {19, 0x80 | 0, 3, 0, 0x00 | 0},
  {20, 0x80 | 0, 3, 0, 0x00 | 0},
  {21, 0x80 | 0, 3, 0, 0x00 | 0},
  {22, 0x80 | 0, 2, 0, 0x00 | 0},
  {23, 0x80 | 0, 3, 0, 0x80 | 0},
  {24, 0x80 | 0, 5, 0, 0x00 | 0},
  {25, 0x80 | 0, 5, 0, 0x00 | 0},
  {26, 0x80 | 0, 1, 0, 0x80 | 0},
  {27, 0x80 | 0, 2, 0, 0x80 | 0},
  {28, 0x80 | 0, 2, 0, 0x80 | 0},
  {29, 0x80 | 0, 1, 0, 0x80 | 0},
  {30, 0x80 | 0, 1, 0, 0x00 | 0},
  {31, 0x80 | 0, 2, 0, 0x00 | 0},
  {32, 0x80 | 0, 3, 0, 0x00 | 0},
  {33, 0x80 | 0, 3, 0, 0x00 | 0},
  {34, 0x80 | 0, 0, 0, 0x80 | 0},
  {35, 0x80 | 0, 2, 0, 0x80 | 0},
  {36, 0x80 | 0, 2, 0, 0x80 | 0},
  {37, 0x80 | 0, 1, 0, 0x00 | 0},
};

static const rect_type screen_rect = {0, 0, 200, 320};
static const char copyprot_letter[] = {'A', 'A', 'B', 'B', 'C', 'C', 'D', 'D', 'E', 'F', 'F', 'G', 'H', 'H', 'I', 'I', 'J', 'J', 'K', 'L', 'L', 'M', 'M', 'N', 'O', 'O', 'P', 'P', 'R', 'R', 'S', 'S', 'T', 'T', 'U', 'U', 'V', 'Y', 'W'};
static const word tbl_line[] = {0, 10, 20};
static const byte chtab_flip_clip[10] = {1, 0, 1, 1, 1, 1, 0, 0, 0, 0};
static const byte chtab_shift[10] = {0, 1, 0, 0, 0, 0, 1, 1, 1, 0};
static const rect_type rect_top = {0, 0, 192, 320};
static const rect_type rect_bottom_text = {193, 70, 202, 250};
static const short y_land[] = {-8, 55, 118, 181, 244};
static const word copyprot_tile[] = {1, 5, 7, 9, 11, 21, 1, 3, 7, 11, 17, 21, 25, 27};
static const byte x_bump[] = {-12, 2, 16, 30, 44, 58, 72, 86, 100, 114, 128, 142, 156, 170, 184, 198, 212, 226, 240, 254};
static const short y_clip[] = {-60, 3, 66, 129, 192};
static const sbyte dir_front[] = {-1, 1};
static const sbyte dir_behind[] = {1, -1};

static custom_options_type custom_defaults = {
                                                  .start_minutes_left = 60,
                                                  .start_ticks_left = 719,
                                                  .start_hitp = 3,
                                                  .max_hitp_allowed = 10,
                                                  .saving_allowed_first_level = 3,
                                                  .saving_allowed_last_level = 13,
                                                  .start_upside_down = 0,
                                                  .start_in_blind_mode = 0,
                                                  // data:009E
                                                  .copyprot_level = 2,
                                                  .drawn_tile_top_level_edge = tiles_1_floor,
                                                  .drawn_tile_left_level_edge = tiles_20_wall,
                                                  .level_edge_hit_tile = tiles_20_wall,
                                                  .allow_triggering_any_tile = 0,
                                                  .enable_wda_in_palace = 0,
                                                  .vga_palette = VGA_PALETTE_DEFAULT,
                                                  .first_level = 1,
                                                  .skip_title = 0,
                                                  .shift_L_allowed_until_level = 4,
                                                  .shift_L_reduced_minutes = 15,
                                                  .shift_L_reduced_ticks = 719,
                                                  .demo_hitp = 4,
                                                  .demo_end_room = 24,
                                                  .intro_music_level = 1,
                                                  .intro_music_time_initial = 33,
                                                  .intro_music_time_restart = 4,
                                                  .have_sword_from_level = 2,
                                                  .checkpoint_level = 3,
                                                  .checkpoint_respawn_dir = dir_FF_left,
                                                  .checkpoint_respawn_room = 2,
                                                  .checkpoint_respawn_tilepos = 6,
                                                  .checkpoint_clear_tile_room = 7,
                                                  .checkpoint_clear_tile_col = 4,
                                                  .checkpoint_clear_tile_row = 0,
                                                  .skeleton_level = 3,
                                                  .skeleton_room = 1,
                                                  .skeleton_trigger_column_1 = 2,
                                                  .skeleton_trigger_column_2 = 3,
                                                  .skeleton_column = 5,
                                                  .skeleton_row = 1,
                                                  .skeleton_require_open_level_door = 1,
                                                  .skeleton_skill = 2,
                                                  .skeleton_reappear_room = 3,
                                                  .skeleton_reappear_x = 133,
                                                  .skeleton_reappear_row = 1,
                                                  .skeleton_reappear_dir = dir_0_right,
                                                  .mirror_level = 4,
                                                  .mirror_room = 4,
                                                  .mirror_column = 4,
                                                  .mirror_row = 0,
                                                  .mirror_tile = tiles_13_mirror,
                                                  .show_mirror_image = 1,
                                                  .falling_exit_level = 6,
                                                  .falling_exit_room = 1,
                                                  .falling_entry_level = 7,
                                                  .falling_entry_room = 17,
                                                  .mouse_level = 8,
                                                  .mouse_room = 16,
                                                  .mouse_delay = 150,
                                                  .mouse_object = 24,
                                                  .mouse_start_x = 200,
                                                  .loose_tiles_level = 13,
                                                  .loose_tiles_room_1 = 23,
                                                  .loose_tiles_room_2 = 16,
                                                  .loose_tiles_first_tile = 22,
                                                  .loose_tiles_last_tile = 27,
                                                  .jaffar_victory_level = 13,
                                                  .jaffar_victory_flash_time = 18,
                                                  .hide_level_number_from_level = 14,
                                                  .level_13_level_number = 12,
                                                  .victory_stops_time_level = 13,
                                                  .win_level = 14,
                                                  .win_room = 5,
                                                  .loose_floor_delay = 11,
                                                  // data:02B2
                                                  .tbl_level_type = {0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0},
                                                  // 1.3
                                                  .tbl_level_color = {0, 0, 0, 1, 0, 0, 0, 1, 2, 2, 0, 0, 3, 3, 4, 0},
                                                  // data:03D4
                                                  .tbl_guard_type = {0, 0, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 4, 3, -1, -1},
                                                  // data:0EDA
                                                  .tbl_guard_hp = {4, 3, 3, 3, 3, 4, 5, 4, 4, 5, 5, 5, 4, 6, 0, 0},
                                                  .tbl_cutscenes_by_index = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
                                                  .tbl_entry_pose = {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0},
                                                  .tbl_seamless_exit = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 23, -1, -1, -1},

                                                  // guard skills
                                                  .strikeprob = {61, 100, 61, 61, 61, 40, 100, 220, 0, 48, 32, 48},
                                                  .restrikeprob = {0, 0, 0, 5, 5, 175, 16, 8, 0, 255, 255, 150},
                                                  .blockprob = {0, 150, 150, 200, 200, 255, 200, 250, 0, 255, 255, 255},
                                                  .impblockprob = {0, 61, 61, 100, 100, 145, 100, 250, 0, 145, 255, 175},
                                                  .advprob = {255, 200, 200, 200, 255, 255, 200, 0, 0, 255, 100, 100},
                                                  .refractimer = {16, 16, 16, 16, 8, 8, 8, 8, 0, 8, 0, 0},
                                                  .extrastrength = {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0},

                                                  // shadow's starting positions
                                                  .init_shad_6 = {0x0F, 0x51, 0x76, 0, 0, 1, 0, 0},
                                                  .init_shad_5 = {0x0F, 0x37, 0x37, 0, 0xFF, 0, 0, 0},
                                                  .init_shad_12 = {0x0F, 0x51, 0xE8, 0, 0, 0, 0, 0},
                                                  // automatic moves
                                                  .demo_moves = {{0x00, 0}, {0x01, 1}, {0x0D, 0}, {0x1E, 1}, {0x25, 5}, {0x2F, 0}, {0x30, 1}, {0x41, 0}, {0x49, 2}, {0x4B, 0}, {0x63, 2}, {0x64, 0}, {0x73, 5}, {0x80, 6}, {0x88, 3}, {0x9D, 7}, {0x9E, 0}, {0x9F, 1}, {0xAB, 4}, {0xB1, 0}, {0xB2, 1}, {0xBC, 0}, {0xC1, 1}, {0xCD, 0}, {0xE9, -1}},
                                                  .shad_drink_move = {{0x00, 0}, {0x01, 1}, {0x0E, 0}, {0x12, 6}, {0x1D, 7}, {0x2D, 2}, {0x31, 1}, {0xFF, -2}},

                                                  // speeds
                                                  .base_speed = 5,
                                                  .fight_speed = 6,
                                                  .chomper_speed = 15,
                                                };
char *g_argv[] = {(char *)"prince"};


#define get_frame(frame_table, frame) get_frame_internal(frame_table, frame, #frame_table, COUNT(frame_table))
#define locate_file(filename) locate_file_(filename, alloca(POP_MAX_PATH), POP_MAX_PATH)
#define snprintf_check(dst, size, ...) do {   \
  int __len;     \
  __len = snprintf(dst, size, __VA_ARGS__); \
  if (__len < 0 || __len >= size) {  \
   fprintf(stderr, "%s: buffer truncation detected!\n", __func__);\
  }      \
 } while (0)

__thread word copyprot_room[] = {3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4};
__thread byte sound_interruptible[] = {
                                                   0, // sound_0_fell_to_death
                                                   1, // sound_1_falling
                                                   1, // sound_2_tile_crashing
                                                   1, // sound_3_button_pressed
                                                   1, // sound_4_gate_closing
                                                   1, // sound_5_gate_opening
                                                   0, // sound_6_gate_closing_fast
                                                   1, // sound_7_gate_stop
                                                   1, // sound_8_bumped
                                                   1, // sound_9_grab
                                                   1, // sound_10_sword_vs_sword
                                                   1, // sound_11_sword_moving
                                                   1, // sound_12_guard_hurt
                                                   1, // sound_13_kid_hurt
                                                   0, // sound_14_leveldoor_closing
                                                   0, // sound_15_leveldoor_sliding
                                                   1, // sound_16_medium_land
                                                   1, // sound_17_soft_land
                                                   0, // sound_18_drink
                                                   1, // sound_19_draw_sword
                                                   1, // sound_20_loose_shake_1
                                                   1, // sound_21_loose_shake_2
                                                   1, // sound_22_loose_shake_3
                                                   1, // sound_23_footstep
                                                   0, // sound_24_death_regular
                                                   0, // sound_25_presentation
                                                   0, // sound_26_embrace
                                                   0, // sound_27_cutscene_2_4_6_12
                                                   0, // sound_28_death_in_fight
                                                   1, // sound_29_meet_Jaffar
                                                   0, // sound_30_big_potion
                                                   0, // sound_31
                                                   0, // sound_32_shadow_music
                                                   0, // sound_33_small_potion
                                                   0, // sound_34
                                                   0, // sound_35_cutscene_8_9
                                                   0, // sound_36_out_of_time
                                                   0, // sound_37_victory
                                                   0, // sound_38_blink
                                                   0, // sound_39_low_weight
                                                   0, // sound_40_cutscene_12_short_time
                                                   0, // sound_41_end_level_music
                                                   0, // sound_42
                                                   0, // sound_43_victory_Jaffar
                                                   0, // sound_44_skel_alive
                                                   0, // sound_45_jump_through_mirror
                                                   0, // sound_46_chomped
                                                   1, // sound_47_chomper
                                                   0, // sound_48_spiked
                                                   0, // sound_49_spikes
                                                   0, // sound_50_story_2_princess
                                                   0, // sound_51_princess_door_opening
                                                   0, // sound_52_story_4_Jaffar_leaves
                                                   0, // sound_53_story_3_Jaffar_comes
                                                   0, // sound_54_intro_music
                                                   0, // sound_55_story_1_absence
                                                   0, // sound_56_ending_music
                                                   0};


__thread custom_options_type *custom = &custom_defaults;
__thread byte is_validate_mode;
__thread word text_time_remaining;
__thread word text_time_total;
__thread word is_show_time;
__thread word checkpoint;
__thread word upside_down;
__thread word resurrect_time;
__thread word dont_reset_time;
__thread short rem_min;
__thread word rem_tick;
__thread word hitp_beg_lev;
__thread word need_level1_music;
__thread byte sound_flags = 0;
__thread word draw_mode;
__thread short start_level = -1;
__thread byte *guard_palettes;
__thread chtab_type *chtab_addrs[10];
__thread word copyprot_plac;
__thread word copyprot_idx;
__thread word cplevel_entr[14];
__thread dialog_type *copyprot_dialog;
__thread rect_type dialog_rect_1 = {60, 56, 124, 264};
__thread rect_type dialog_rect_2 = {61, 56, 120, 264};
__thread word drawn_room;
__thread byte curr_tile;
__thread byte curr_modifier;
__thread tile_and_mod leftroom_[3];
__thread tile_and_mod row_below_left_[10];
__thread word loaded_room;
__thread byte *curr_room_tiles;
__thread byte *curr_room_modif;
__thread word draw_xh;
__thread word current_level = -1;
__thread byte graphics_mode = 0;
__thread word room_L;
__thread word room_R;
__thread word room_A;
__thread word room_B;
__thread word room_BR;
__thread word room_BL;
__thread word room_AR;
__thread word room_AL;
__thread level_type level;
__thread short table_counts[5];
__thread short drects_count;
__thread short peels_count;
__thread back_table_type foretable[200];
__thread back_table_type backtable[200];
__thread midtable_type midtable[50];
__thread peel_type *peels_table[50];
__thread rect_type drects[30];
__thread sbyte obj_direction;
__thread short obj_clip_left;
__thread short obj_clip_top;
__thread short obj_clip_right;
__thread short obj_clip_bottom;
__thread wipetable_type wipetable[300];
__thread word need_drects;
__thread word leveldoor_right;
__thread word leveldoor_ybottom;
__thread byte palace_wall_colors[44 * 3];
__thread word seed_was_init = 0;
__thread dword random_seed;
__thread byte *doorlink2_ad;
__thread byte *doorlink1_ad;
__thread sbyte control_shift;
__thread sbyte control_y;
__thread sbyte control_x;
__thread char_type Kid;
__thread word is_keyboard_mode = 0;
__thread word is_paused;
__thread word is_restart_level;
__thread byte sound_mode = 0;
__thread byte is_sound_on = 0x0F;
__thread word next_level;
__thread short guardhp_delta;
__thread word guardhp_curr;
__thread word next_room;
__thread word hitp_curr;
__thread word hitp_max;
__thread short hitp_delta;
__thread word flash_color;
__thread word flash_time;
__thread char_type Guard;
__thread word need_quotes;
__thread short roomleave_result;
__thread word different_room;
__thread sound_buffer_type *sound_pointers[58];
__thread word guardhp_max;
__thread word is_feather_fall;
__thread chtab_type *chtab_title40;
__thread chtab_type *chtab_title50;
__thread short hof_count;
__thread word demo_mode = 0;
__thread short mobs_count;
__thread short trobs_count;
__thread short next_sound;
__thread word grab_timer;
__thread short can_guard_see_kid;
__thread word holding_sword;
__thread short united_with_shadow;
__thread word leveldoor_open;
__thread word demo_index;
__thread short demo_time;
__thread word have_sword;
__thread char_type Char;
__thread char_type Opp;
__thread short knock;
__thread word is_guard_notice;
__thread byte wipe_frames[30];
__thread sbyte wipe_heights[30];
__thread byte redraw_frames_anim[30];
__thread byte redraw_frames2[30];
__thread byte redraw_frames_floor_overlay[30];
__thread byte redraw_frames_full[30];
__thread byte redraw_frames_fore[30];
__thread byte tile_object_redraw[30];
__thread byte redraw_frames_above[10];
__thread word need_full_redraw;
__thread short n_curr_objs;
__thread objtable_type objtable[50];
__thread short curr_objs[50];
__thread byte obj_xh;
__thread byte obj_xl;
__thread byte obj_y;
__thread byte obj_chtab;
__thread byte obj_id;
__thread byte obj_tilepos;
__thread short obj_x;
__thread frame_type cur_frame;
__thread word seamless;
__thread trob_type trob;
__thread trob_type trobs[30];
__thread short redraw_height;
__thread byte curr_tilepos;
__thread short curr_room;
__thread mob_type curmob;
__thread mob_type mobs[14];
__thread short tile_col;
__thread word curr_guard_color;
__thread byte key_states[SDL_NUM_SCANCODES];
__thread word is_screaming;
__thread word offguard; // name from Apple II source
__thread word droppedout; // name from Apple II source
__thread word exit_room_timer;
__thread short char_col_right;
__thread short char_col_left;
__thread short char_top_row;
__thread short prev_char_top_row;
__thread short prev_char_col_right;
__thread short prev_char_col_left;
__thread short char_bottom_row;
__thread short guard_notice_timer;
__thread short jumped_through_mirror;
__thread byte curr_tile2;
__thread short tile_row;
__thread word char_width_half;
__thread word char_height;
__thread short char_x_left;
__thread short char_x_left_coll;
__thread short char_x_right_coll;
__thread short char_x_right;
__thread short char_top_y;
__thread byte fall_frame;
__thread byte through_tile;
__thread sbyte infrontx; // name from Apple II source
__thread word current_sound;
__thread sbyte control_shift2;
__thread sbyte control_forward;
__thread word guard_skill;
__thread sbyte control_backward;
__thread sbyte control_up;
__thread sbyte control_down;
__thread sbyte ctrl1_forward;
__thread sbyte ctrl1_backward;
__thread sbyte ctrl1_up;
__thread sbyte ctrl1_down;
__thread sbyte ctrl1_shift2;
__thread word shadow_initialized;
__thread word guard_refrac;
__thread word kid_sword_strike;
__thread byte edge_type;
__thread char **sound_names;
__thread int g_argc;
__thread sbyte collision_row;
__thread sbyte prev_collision_row;
__thread sbyte prev_coll_room[10];
__thread sbyte curr_row_coll_room[10];
__thread sbyte below_row_coll_room[10];
__thread sbyte above_row_coll_room[10];
__thread byte curr_row_coll_flags[10];
__thread byte above_row_coll_flags[10];
__thread byte below_row_coll_flags[10];
__thread byte prev_coll_flags[10];
__thread short pickup_obj_type;
__thread word justblocked; // name from Apple II source
__thread word last_loose_sound;
__thread int last_key_scancode;
__thread word curmob_index;
__thread dat_type *dathandle;
__thread word need_redraw_because_flipped;
__thread byte *level_var_palettes;
__thread word first_start = 1;
__thread sbyte distance_mirror;
__thread Uint64 last_transition_counter;
__thread sbyte bump_col_left_of_wall;
__thread sbyte bump_col_right_of_wall;
__thread sbyte right_checked_col;
__thread sbyte left_checked_col;
__thread short coll_tile_left_xpos;
__thread word curr_tile_temp;
__thread long int _cachedFilePointerTable[MAX_CACHED_FILES];
__thread char *_cachedFileBufferTable[MAX_CACHED_FILES];
__thread size_t _cachedFileBufferSizes[MAX_CACHED_FILES];
__thread char _cachedFilePathTable[MAX_CACHED_FILES][POP_MAX_PATH];
__thread size_t _cachedFileCounter = 0;
__thread char exe_dir[POP_MAX_PATH] = ".";
__thread bool found_exe_dir = false;
__thread word which_quote;
__thread dat_type *dat_chain_ptr = NULL;
__thread char last_text_input;
__thread short drawn_row;
__thread short draw_bottom_y;
__thread short draw_main_y;
__thread short drawn_col;
__thread byte tile_left;
__thread byte modifier_left;

FILE *fcache_open(const char *filename, const char *mode)
{
  // Check if already cached, and return it directly from cache if it is
  for (size_t i = 0; i < _cachedFileCounter; i++)
    if (strcmp(filename, _cachedFilePathTable[i]) == 0)
    {
      _cachedFilePointerTable[i] = 0; // Rewinding
      return (FILE *)i;
    }

  // Checking if file is directory. Do not open in that case.
  struct stat path_stat;
  stat(filename, &path_stat);
  if (S_ISREG(path_stat.st_mode) == 0)
    return NULL;

  // Open source file
  FILE *srcFile = fopen(filename, "rb");

  // If failed to load, return immediately
  if (srcFile == NULL)
    return NULL;

  // Finding out file's size
  fseek(srcFile, 0, SEEK_END);
  long int fileSize = ftell(srcFile);
  fseek(srcFile, 0, SEEK_SET);

  // Creating cached file buffer
  _cachedFileBufferTable[_cachedFileCounter] = malloc(fileSize);

  // Reading source file info buffer
  fread(_cachedFileBufferTable[_cachedFileCounter], 1, fileSize, srcFile);

  // Closing source file
  fclose(srcFile);

  // Add filepath to the cache table
  strcpy(_cachedFilePathTable[_cachedFileCounter], filename);

  // Storing file size
  _cachedFileBufferSizes[_cachedFileCounter] = fileSize;

  // Resetting pointer
  _cachedFilePointerTable[_cachedFileCounter] = 0;

  // Setting found ptr
  FILE *foundPtr = (FILE *)_cachedFileCounter;

  // Increase cached file counter
  _cachedFileCounter++;

  return foundPtr;
}

size_t fcache_read(void *ptr, size_t size, size_t count, FILE *stream)
{
  size_t fileId = (size_t)stream;
  size_t readCount = _cachedFileBufferSizes[fileId] / size;
  if (readCount > count)
    readCount = count;

  // Copying data from cached file to pointer
  memcpy(ptr, &_cachedFileBufferTable[fileId][_cachedFilePointerTable[fileId]], readCount * size);

  // Advancing file pointer
  _cachedFilePointerTable[fileId] += readCount * size;

  // Returning number of elements read
  return readCount;
}

int fcache_seek(FILE *stream, long int offset, int origin)
{
  long int base;
  size_t fileId = (size_t)stream;
  if (origin == SEEK_SET)
    base = 0;
  if (origin == SEEK_CUR)
    base = _cachedFilePointerTable[fileId];
  if (origin == SEEK_END)
    base = _cachedFileBufferSizes[fileId] - 1;

  long int dest = base + offset;
  if (dest > _cachedFileBufferSizes[fileId] - 1)
    return -1;
  if (dest < 0)
    return -1;

  _cachedFilePointerTable[fileId] = base + offset;

  return 0;
}

int fcache_tell(FILE *stream)
{
  size_t fileId = (size_t)stream;
  return _cachedFilePointerTable[fileId] + 1;
}

int fcache_close(FILE *file)
{
  // Do not actually close it. This will be done at the end
  return 0;
}


void find_exe_dir()
{
  if (found_exe_dir)
    return;
  snprintf_check(exe_dir, sizeof(exe_dir), "%s", g_argv[0]);
  char *last_slash = NULL;
  char *pos = exe_dir;
  for (char c = *pos; c != '\0'; c = *(++pos))
  {
    if (c == '/' || c == '\\')
    {
      last_slash = pos;
    }
  }
  if (last_slash != NULL)
  {
    *last_slash = '\0';
  }
  found_exe_dir = true;
}

bool file_exists(const char *filename)
{
  return (access(filename, 0) != -1);
}

const char *locate_file_(const char *filename, char *path_buffer, int buffer_size)
{
  if (file_exists(filename))
  {
    return filename;
  }
  else
  {
    // If failed, it may be that SDLPoP is being run from the wrong different working directory.
    // We can try to rescue the situation by loading from the directory of the executable.
    find_exe_dir();
    snprintf_check(path_buffer, buffer_size, "%s/%s", exe_dir, filename);
    return (const char *)path_buffer;
  }
}

directory_listing_type *create_directory_listing_and_find_first_file(const char *directory, const char *extension)
{
  directory_listing_type *data = calloc(1, sizeof(directory_listing_type));
  bool ok = false;
  data->dp = opendir(directory);
  if (data->dp != NULL)
  {
    struct dirent *ep;
    while ((ep = readdir(data->dp)))
    {
      char *ext = strrchr(ep->d_name, '.');
      if (ext != NULL && strcasecmp(ext + 1, extension) == 0)
      {
        data->found_filename = ep->d_name;
        data->extension = extension;
        ok = true;
        break;
      }
    }
  }
  if (ok)
  {
    return data;
  }
  else
  {
    free(data);
    return NULL;
  }
}

char *get_current_filename_from_directory_listing(directory_listing_type *data)
{
  return data->found_filename;
}

bool find_next_file(directory_listing_type *data)
{
  bool ok = false;
  struct dirent *ep;
  while ((ep = readdir(data->dp)))
  {
    char *ext = strrchr(ep->d_name, '.');
    if (ext != NULL && strcasecmp(ext + 1, data->extension) == 0)
    {
      data->found_filename = ep->d_name;
      ok = true;
      break;
    }
  }
  return ok;
}

void close_directory_listing(directory_listing_type *data)
{
  closedir(data->dp);
  free(data);
}

// seg009:000D
int __pascal far read_key()
{
  // stub
  int key = last_key_scancode;
  last_key_scancode = 0;
  return key;
}

// seg009:019A
void __pascal far clear_kbd_buf()
{
  // stub
  last_key_scancode = 0;
  last_text_input = 0;
}

// seg009:040A
word __pascal far prandom(word max)
{
  if (!seed_was_init)
  {
    // init from current time
    random_seed = time(NULL);
    seed_was_init = 1;
  }
  random_seed = random_seed * 214013 + 2531011;
  return (random_seed >> 16) % (max + 1);
}

static FILE *open_dat_from_root_or_data_dir(const char *filename)
{
  FILE *fp = NULL;
  fp = fcache_open(filename, "rb");

  // if failed, try if the DAT file can be opened in the data/ directory, instead of the main folder
  if (fp == NULL)
  {
    char data_path[POP_MAX_PATH];
    snprintf_check(data_path, sizeof(data_path), "data/%s", filename);

    if (!file_exists(data_path))
    {
      find_exe_dir();
      snprintf_check(data_path, sizeof(data_path), "%s/data/%s", exe_dir, filename);
    }

    // verify that this is a regular file and not a directory (otherwise, don't open)
    fp = fcache_open(data_path, "rb");
  }
  return fp;
}

// seg009:0F58
dat_type *__pascal open_dat(const char *filename, int drive)
{
  FILE *fp = NULL;
  fp = open_dat_from_root_or_data_dir(filename);
  dat_header_type dat_header;
  dat_table_type *dat_table = NULL;

  dat_type *pointer = (dat_type *)calloc(1, sizeof(dat_type));
  snprintf_check(pointer->filename, sizeof(pointer->filename), "%s", filename);
  pointer->next_dat = dat_chain_ptr;
  dat_chain_ptr = pointer;

  if (fp != NULL)
  {
    if (fcache_read(&dat_header, 6, 1, fp) != 1)
      goto failed;
    dat_table = (dat_table_type *)malloc(dat_header.table_size);
    if (dat_table == NULL ||
        fcache_seek(fp, dat_header.table_offset, SEEK_SET) ||
        fcache_read(dat_table, dat_header.table_size, 1, fp) != 1)
      goto failed;
    pointer->handle = fp;
    pointer->dat_table = dat_table;
  }
out:
  // stub
  return pointer;
failed:
  perror(filename);
  if (fp)
    fcache_close(fp);
  if (dat_table)
    free(dat_table);
  goto out;
}

// seg000:0000
void far pop_main()
{
  // Fix bug: with start_in_blind_mode enabled, moving objects are not displayed until blind mode is toggled off+on??
  need_drects = 1;

  dathandle = open_dat("PRINCE.DAT", 0);

  parse_cmdline_sound();

  init_game_main();
}

// seg009:9F80
void far *__pascal load_from_opendats_alloc(int resource, const char *extension, data_location *out_result, int *out_size)
{
  // stub
  //printf("id = %d\n",resource);
  dat_type *pointer;
  data_location result;
  byte checksum;
  int size;
  FILE *fp = NULL;
  load_from_opendats_metadata(resource, extension, &fp, &result, &checksum, &size, &pointer);
  if (out_result != NULL)
    *out_result = result;
  if (out_size != NULL)
    *out_size = size;
  if (result == data_none)
    return NULL;

  void *area = malloc(size);
  //read(fd, area, size);
  if (fcache_read(area, size, 1, fp) != 1)
  {
    printf("ERROR\n");
    fprintf(stderr, "%s: %s, resource %d, size %d, failed: %s\n", __func__, pointer->filename, resource, size, strerror(errno));
    free(area);
    area = NULL;
  }

  if (result == data_directory)
    fcache_close(fp);
  /* XXX: check checksum */
  return area;
}


// seg009:121A
image_type *far __pascal far load_image(int resource_id, dat_pal_type *palette)
{
  // stub
  data_location result;
  int size;
  image_data_type *image_data = (image_data_type*)load_from_opendats_alloc(resource_id, "png", &result, &size);
  image_type *image = (image_type*)malloc(sizeof(image_data));
  switch (result)
  {
  case data_none:
    return NULL;
    break;
  case data_DAT:
  { // DAT
   image->h = image_data->height;
   image->w = image_data->width;
  }
  break;
  case data_directory:
  { // directory
    SDL_RWops *rw = SDL_RWFromConstMem(image_data, size);
    SDL_Surface* image2 = IMG_Load_RW(rw, 0);
    image->h = image2->h;
    image->w = image2->w;
  }
  break;
  }
  if (image_data != NULL) free(image_data);

  return image;
}

// seg009:104E
chtab_type *__pascal load_sprites_from_file(int resource, int palette_bits, int quit_on_error)
{
  int i;
  int n_images = 0;
  //int has_palette_bits = 1;
  chtab_type *chtab = NULL;
  dat_shpl_type *shpl = (dat_shpl_type *)load_from_opendats_alloc(resource, "pal", NULL, NULL);

  dat_pal_type *pal_ptr = &shpl->palette;

  n_images = shpl->n_images;
  size_t alloc_size = sizeof(chtab_type) + sizeof(void far *) * n_images;
  chtab = (chtab_type *)malloc(alloc_size);
  memset(chtab, 0, alloc_size);
  chtab->n_images = n_images;
  for (i = 1; i <= n_images; i++)
  {
    chtab->images[i - 1] = load_image(resource + i, pal_ptr);;
  }
  return chtab;
}


void load_from_opendats_metadata(int resource_id, const char *extension, FILE **out_fp, data_location *result, byte *checksum, int *size, dat_type **out_pointer)
{
  char image_filename[POP_MAX_PATH];
  FILE *fp = NULL;
  dat_type *pointer;
  *result = data_none;
  // Go through all open DAT files.
  for (pointer = dat_chain_ptr; fp == NULL && pointer != NULL; pointer = pointer->next_dat)
  {
    *out_pointer = pointer;
    if (pointer->handle != NULL)
    {
      // If it's an actual DAT file:
      fp = pointer->handle;
      dat_table_type *dat_table = pointer->dat_table;
      int i;
      for (i = 0; i < dat_table->res_count; ++i)
      {
        if (dat_table->entries[i].id == resource_id)
        {
          break;
        }
      }
      if (i < dat_table->res_count)
      {
        // found
        *result = data_DAT;
        *size = dat_table->entries[i].size;
        if (fcache_seek(fp, dat_table->entries[i].offset, SEEK_SET) ||
            fcache_read(checksum, 1, 1, fp) != 1)
        {
          perror(pointer->filename);
          fp = NULL;
        }
      }
      else
      {
        // not found
        fp = NULL;
      }
    }
    else
    {
      // If it's a directory:
      char filename_no_ext[POP_MAX_PATH];
      // strip the .DAT file extension from the filename (use folders simply named TITLE, KID, VPALACE, etc.)
      strncpy(filename_no_ext, pointer->filename, sizeof(filename_no_ext));
      size_t len = strlen(filename_no_ext);
      if (len >= 5 && filename_no_ext[len - 4] == '.')
      {
        filename_no_ext[len - 4] = '\0'; // terminate, so ".DAT" is deleted from the filename
      }
      snprintf_check(image_filename, sizeof(image_filename), "data/%s/res%d.%s", filename_no_ext, resource_id, extension);

        //printf("loading (binary) %s",image_filename);
        const char *filename = locate_file(image_filename);
        //printf("File: %s\n", filename);
        fp = fcache_open(filename, "rb");
        if (fp != NULL)
        {
          *result = data_directory;
        }

      if (fp != NULL)
      {
        *result = data_directory;

        fcache_seek(fp, 0L, SEEK_END);
        *size = fcache_tell(fp);
        fcache_seek(fp, 0L, SEEK_SET);
      }
    }
  }
  *out_fp = fp;
  if (fp == NULL)
  {
    *result = data_none;
    //  printf(" FAILED\n");
    //return NULL;
  }
  //...
}

// seg009:9F34
void __pascal far close_dat(dat_type far *pointer)
{
  dat_type **prev = &dat_chain_ptr;
  dat_type *curr = dat_chain_ptr;
  while (curr != NULL)
  {
    if (curr == pointer)
    {
      *prev = curr->next_dat;
      if (curr->handle)
        fcache_close(curr->handle);
      if (curr->dat_table)
        free(curr->dat_table);
      free(curr);
      return;
    }
    curr = curr->next_dat;
    prev = &((*prev)->next_dat);
  }
  // stub
}


// seg009:A172
int __pascal far load_from_opendats_to_area(int resource, void far *area, int length, const char *extension)
{
  // stub
  //return 0;
  dat_type *pointer;
  data_location result;
  byte checksum;
  int size;
  FILE *fp = NULL;
  load_from_opendats_metadata(resource, extension, &fp, &result, &checksum, &size, &pointer);
  if (result == data_none)
    return 0;
  if (fcache_read(area, MIN(size, length), 1, fp) != 1)
  {
    fprintf(stderr, "%s: %s, resource %d, size %d, failed: %s\n", __func__, pointer->filename, resource, size, strerror(errno));
    memset(area, 0, MIN(size, length));
  }
  if (result == data_directory)
    fcache_close(fp);
  /* XXX: check checksum */
  return 0;
}

// seg000:024F
void __pascal far init_game_main()
{
  doorlink1_ad = /*&*/ level.doorlinks1;
  doorlink2_ad = /*&*/ level.doorlinks2;
  prandom(1);
  // PRINCE.DAT: sword
  chtab_addrs[id_chtab_0_sword] = load_sprites_from_file(700, 1 << 2, 1);
  // PRINCE.DAT: flame, sword on floor, potion
  chtab_addrs[id_chtab_1_flameswordpotion] = load_sprites_from_file(150, 1 << 3, 1);
  close_dat(dathandle);

  start_game();
}

void __pascal far init_copyprot()
{
  word which_entry;
  word pos;
  word entry_used[40];
  byte letts_used[26];

  copyprot_plac = prandom(13);
  memset(&entry_used, 0, sizeof(entry_used));
  memset(&letts_used, 0, sizeof(letts_used));
  for (pos = 0; pos < 14; ++pos)
  {
    do
    {
      if (pos == copyprot_plac)
      {
        which_entry = copyprot_idx = prandom(39);
      }
      else
      {
        which_entry = prandom(39);
      }
    } while (entry_used[which_entry] || letts_used[copyprot_letter[which_entry] - 'A']);
    cplevel_entr[pos] = which_entry;
    entry_used[which_entry] = 1;
    letts_used[copyprot_letter[which_entry] - 'A'] = 1;
  }
}

void __pascal far start_game()
{
  // Prevent filling of stack.
  // start_game is called from many places to restart the game, for example:
  // process_key, play_frame, draw_game_frame, play_level, control_kid, end_sequence, expired
  init_copyprot();

 init_game(start_level);
}

void restore_room_after_quick_load()
{
  int temp1 = curr_guard_color;
  int temp2 = next_level;
  reset_level_unused_fields(false);
  curr_guard_color = temp1;
  next_level = temp2;

  //need_full_redraw = 1;
  different_room = 1;
  // Show the room where the prince is, even if the player moved the view away from it (with the H,J,U,N keys).
  next_room = drawn_room = Kid.room;
  load_room_links();
  //draw_level_first();
  //gen_palace_wall_colors();
  is_guard_notice = 0; // prevent guard turning around immediately
  //redraw_screen(1); // for room_L

  hitp_delta = guardhp_delta = 1; // force HP redraw
  // Don't draw guard HP if a previously viewed room (with the H,J,U,N keys) had a guard but the current room doesn't have one.
  if (Guard.room != drawn_room)
  {
    // Like in clear_char().
    Guard.direction = dir_56_none;
    guardhp_curr = 0;
  }

  loadkid_and_opp();
  // Get rid of "press button" message if kid was dead before quickload.
  text_time_total = text_time_remaining = 0;
}

Uint32 temp_shift_release_callback(Uint32 interval, void *param)
{
  const Uint8 *state = SDL_GetKeyboardState(NULL);
  if (state[SDL_SCANCODE_LSHIFT])
    key_states[SDL_SCANCODE_LSHIFT] = 1;
  if (state[SDL_SCANCODE_RSHIFT])
    key_states[SDL_SCANCODE_RSHIFT] = 1;
  return 0; // causes the timer to be removed
}

// seg000:04CD
int __pascal far process_key()
{
  int key = 0;

  // If the Kid died, Enter or Shift will restart the level.
  if (rem_min != 0 && Kid.alive > 6 && (control_shift || key == SDL_SCANCODE_RETURN))
  {
    key = SDL_SCANCODE_A | WITH_CTRL; // Ctrl+A
  }
  if (key == 0)
    return 0;
  if (is_keyboard_mode)
    clear_kbd_buf();

  switch (key)
  {
  case SDL_SCANCODE_ESCAPE:              // Esc
  case SDL_SCANCODE_ESCAPE | WITH_SHIFT: // allow pause while grabbing
    is_paused = 1;
    break;
  case SDL_SCANCODE_A | WITH_CTRL: // Ctrl+A
    if (current_level != 15)
    {
      is_restart_level = 1;
    }
    break;
  case SDL_SCANCODE_K | WITH_CTRL: // Ctrl+K
    is_keyboard_mode = 1;
    break;
  case SDL_SCANCODE_R | WITH_CTRL: // Ctrl+R
    start_level = -1;
    start_game();
    break;
  }

  return 1;
}

// seg000:08EB
void __pascal far play_frame()
{
  do_mobs();
  process_trobs();
  check_skel();
  check_can_guard_see_kid();
  // if level is restarted, return immediately
  if (play_kid_frame())
    return;
  play_guard_frame();
  if (0 == resurrect_time)
  {
    check_sword_hurting();
    check_sword_hurt();
  }
  check_sword_vs_sword();
  do_delta_hp();
  exit_room();
  check_the_end();
  check_guard_fallout();
  if (current_level == 0)
  {
    // Special event: level 0 running exit
    if (Kid.room == 24)
    {
      start_level = -1;
      need_quotes = 1;
      start_game();
    }
  }
  else if (current_level == 6)
  {
    // Special event: level 6 falling exit
    if (roomleave_result == -2)
    {
      Kid.y = -1;
      ++next_level;
    }
  }
  else if (current_level == 12)
  {
    // Special event: level 12 running exit
    if (Kid.room == 23 )
    {
      ++next_level;
      // Sounds must be stopped, because play_level_2() checks next_level only if there are no sounds playing.
      seamless = 1;
    }
  }
}

// seg007:1041
short __pascal far get_curr_tile(short tilepos)
{
  curr_modifier = curr_room_modif[tilepos];
  return curr_tile = curr_room_tiles[tilepos] & 0x1F;
}


// seg000:0B12
void __pascal far anim_tile_modif()
{
  word tilepos;
  for (tilepos = 0; tilepos < 30; ++tilepos)
  {
    switch (get_curr_tile(tilepos))
    {
    case tiles_10_potion:
      start_anim_potion(drawn_room, tilepos);
      break;
    case tiles_19_torch:
    case tiles_30_torch_with_debris:
      start_anim_torch(drawn_room, tilepos);
      break;
    case tiles_22_sword:
      start_anim_sword(drawn_room, tilepos);
      break;
    }
  }

  // Animate torches in the rightmost column of the left-side room as well, because they are visible in the current room.
  for (int row = 0; row <= 2; row++)
  {
    switch (get_tile(room_L, 9, row))
    {
    case tiles_19_torch:
    case tiles_30_torch_with_debris:
      start_anim_torch(room_L, row * 10 + 9);
      break;
    }
  }
}

void __pascal far load_lev_spr(int level)
{
  dat_type *dathandle;
  short guardtype;
  char filename[20];
  dathandle = NULL;
  current_level = next_level = level;
  snprintf(filename, sizeof(filename), "%s%s.DAT", tbl_envir_gr[gmMcgaVga], tbl_envir_ki[custom->tbl_level_type[current_level]]);
  load_chtab_from_file(id_chtab_6_environment, 200, filename, 1 << 5);
  load_more_opt_graf(filename);
  guardtype = custom->tbl_guard_type[current_level];
  if (guardtype != -1)
  {
    if (guardtype == 0)
    {
      dathandle = open_dat(custom->tbl_level_type[current_level] ? "GUARD1.DAT" : "GUARD2.DAT", 0);
    }
    load_chtab_from_file(id_chtab_5_guard, 750, tbl_guard_dat[guardtype], 1 << 8);
    if (dathandle)
    {
      close_dat(dathandle);
    }
  }
  curr_guard_color = 0;
  load_chtab_from_file(id_chtab_7_environmentwall, 360, filename, 1 << 6);
}

// seg000:0E6C
void __pascal far load_level()
{
  dat_type *dathandle;
  dathandle = open_dat("LEVELS.DAT", 0);
  load_from_opendats_to_area(current_level + 2000, &level, sizeof(level), "bin");
  close_dat(dathandle);

  reset_level_unused_fields(true); // added
}

void reset_level_unused_fields(bool loading_clean_level)
{
  // Entirely unused fields in the level format: reset to zero for now
  // They can be repurposed to add new stuff to the level format in the future
  memset(level.roomxs, 0, sizeof(level.roomxs));
  memset(level.roomys, 0, sizeof(level.roomys));
  memset(level.fill_1, 0, sizeof(level.fill_1));
  memset(level.fill_2, 0, sizeof(level.fill_2));
  memset(level.fill_3, 0, sizeof(level.fill_3));

  // level.used_rooms is 25 on some levels. Limit it to the actual number of rooms.
  if (level.used_rooms > 24)
    level.used_rooms = 24;

  // For these fields, only use the bits that are actually used, and set the rest to zero.
  // Good for repurposing the unused bits in the future.
  int i;
  for (i = 0; i < level.used_rooms; ++i)
  {
    //level.guards_dir[i]   &= 0x01; // 1 bit in use
    level.guards_skill[i] &= 0x0F; // 4 bits in use
  }

  // In savestates, additional information may be stored (e.g. remembered guard hp) - should not reset this then!
  if (loading_clean_level)
  {
    for (i = 0; i < level.used_rooms; ++i)
    {
      level.guards_color[i] &= 0x0F; // 4 bits in use (other 4 bits repurposed as remembered guard hp)
    }
  }
}

// seg000:0EA8
// returns 1 if level is restarted, 0 otherwise
int __pascal far play_kid_frame()
{
  loadkid_and_opp();
  load_fram_det_col();
  check_killed_shadow();
  play_kid();
  if (upside_down && Char.alive >= 0)
  {
    upside_down = 0;
    need_redraw_because_flipped = 1;
  }
  if (is_restart_level)
  {
    return 1;
  }
  if (Char.room != 0)
  {
    play_seq();
    fall_accel();
    fall_speed();
    load_frame_to_obj();
    load_fram_det_col();
    set_char_collision();
    bump_into_opponent();
    check_collisions();
    check_bumped();
    check_gate_push();
    check_action();
    check_press();
    check_spike_below();
    if (resurrect_time == 0)
    {
      check_spiked();
      check_chomped_kid();
    }
    check_knock();
  }
  savekid();
  return 0;
}

// seg000:0F48
void __pascal far play_guard_frame()
{
  if (Guard.direction != dir_56_none)
  {
    loadshad_and_opp();
    load_fram_det_col();
    check_killed_shadow();
    play_guard();
    if (Char.room == drawn_room)
    {
      play_seq();
      if (Char.x >= 44 && Char.x < 211)
      {
        fall_accel();
        fall_speed();
        load_frame_to_obj();
        load_fram_det_col();
        set_char_collision();
        check_guard_bumped();
        check_action();
        check_press();
        check_spike_below();
        check_spiked();
        check_chomped_guard();
      }
    }
    saveshad();
  }
}

// seg000:0FBD
void __pascal far check_the_end()
{
  if (next_room != 0 && next_room != drawn_room)
  {
    drawn_room = next_room;
    load_room_links();
    different_room = 1;
    exit_room_timer = 2; // Added to remember exit room timer
    loadkid();
    anim_tile_modif();
    start_chompers();
    check_fall_flo();
    check_shadow();
  }
}

// seg000:1009
void __pascal far check_fall_flo()
{
  // Special event: falling floors
  if (current_level == /*13*/ custom->loose_tiles_level &&
      (drawn_room == /*23*/ custom->loose_tiles_room_1 || drawn_room == /*16*/ custom->loose_tiles_room_2))
  {
    get_room_address(curr_room = room_A);
    for (curr_tilepos = /*22*/ custom->loose_tiles_first_tile;
         curr_tilepos <= /*27*/ custom->loose_tiles_last_tile;
         ++curr_tilepos)
    {
      make_loose_fall(-(prandom(0xFF) & 0x0F));
    }
  }
}


// seg000:11EC
void __pascal far add_life()
{
  short hpmax = hitp_max;
  ++hpmax;
  // CusPop: set maximum number of hitpoints (max_hitp_allowed, default = 10)
  //	if (hpmax > 10) hpmax = 10; // original
  if (hpmax > custom->max_hitp_allowed)
    hpmax = custom->max_hitp_allowed;
  hitp_max = hpmax;
  set_health_life();
}

// seg000:1200
void __pascal far set_health_life()
{
  hitp_delta = hitp_max - hitp_curr;
}


// seg000:127B
void __pascal far do_delta_hp()
{
  // level 12: if the shadow is hurt, Kid is also hurt
  if (Opp.charid == charid_1_shadow &&
      current_level == 12 &&
      guardhp_delta != 0)
  {
    hitp_delta = guardhp_delta;
  }
  hitp_curr = MIN(MAX(hitp_curr + hitp_delta, 0), hitp_max);
  guardhp_curr = MIN(MAX(guardhp_curr + guardhp_delta, 0), guardhp_max);
}

// seg000:1353
void __pascal far check_sword_vs_sword()
{
}

// seg000:136A
void __pascal far load_chtab_from_file(int chtab_id, int resource, const char near *filename, int palette_bits)
{
  //printf("Loading chtab %d, id %d from %s\n",chtab_id,resource,filename);
  dat_type *dathandle;
  if (chtab_addrs[chtab_id] != NULL)
    return;
  dathandle = open_dat(filename, 0);
  chtab_addrs[chtab_id] = load_sprites_from_file(resource, palette_bits, 1);
  close_dat(dathandle);
}

// seg009:12EF
void __pascal far load_one_optgraf(chtab_type *chtab_ptr, dat_pal_type far *pal_ptr, int base_id, int min_index, int max_index)
{
  short index;
  for (index = min_index; index <= max_index; ++index)
  {
    image_type *image = load_image(base_id + index + 1, pal_ptr);
    if (image != NULL)
      chtab_ptr->images[index] = image;
  }
}

// seg000:13FC
void __pascal far load_more_opt_graf(const char *filename)
{
  // stub
  dat_type *dathandle;
  dat_shpl_type area;
  short graf_index;
  dathandle = NULL;
  for (graf_index = 0; graf_index < 8; ++graf_index)
  {
    /*if (...) */ {
      if (dathandle == NULL)
      {
        dathandle = open_dat(filename, 0);
        load_from_opendats_to_area(200, &area, sizeof(area), "pal");
        area.palette.row_bits = 0x20;
      }
      load_one_optgraf(chtab_addrs[id_chtab_6_environment], &area.palette, 1200, optgraf_min[graf_index] - 1, optgraf_max[graf_index] - 1);
    }
  }
  if (dathandle != NULL)
  {
    close_dat(dathandle);
  }
}

// seg000:148D
int __pascal far do_paused()
{
  word key;
  key = 0;
  next_room = 0;
  control_shift = 0;
  control_y = 0;
  control_x = 0;
  read_keyb_control();
  key = process_key();
  return key || control_shift;
}

// seg000:1500
void __pascal far read_keyb_control()
{
  if (key_states[SDL_SCANCODE_UP] || key_states[SDL_SCANCODE_HOME] || key_states[SDL_SCANCODE_PAGEUP] || key_states[SDL_SCANCODE_KP_8] || key_states[SDL_SCANCODE_KP_7] || key_states[SDL_SCANCODE_KP_9])
  {
    control_y = -1;
  }
  else if (key_states[SDL_SCANCODE_CLEAR] || key_states[SDL_SCANCODE_DOWN] || key_states[SDL_SCANCODE_KP_5] || key_states[SDL_SCANCODE_KP_2])
  {
    control_y = 1;
  }
  if (key_states[SDL_SCANCODE_LEFT] || key_states[SDL_SCANCODE_HOME] || key_states[SDL_SCANCODE_KP_4] || key_states[SDL_SCANCODE_KP_7])
  {
    control_x = -1;
  }
  else if (key_states[SDL_SCANCODE_RIGHT] || key_states[SDL_SCANCODE_PAGEUP] || key_states[SDL_SCANCODE_KP_6] || key_states[SDL_SCANCODE_KP_9])
  {
    control_x = 1;
  }
  control_shift = -(key_states[SDL_SCANCODE_LSHIFT] || key_states[SDL_SCANCODE_RSHIFT]);
}

// seg000:15E9
void __pascal far toggle_upside()
{
  upside_down = ~upside_down;
  need_redraw_because_flipped = 1;
}

// seg000:15F8
void __pascal far feather_fall()
{
  is_feather_fall = 1;
  flash_color = 2; // green
  flash_time = 3;
}

// seg000:172C
void __pascal far gen_palace_wall_colors()
{
  dword old_randseed;
  word prev_color;
  short row;
  short subrow;
  word color_base;
  short column;
  word color;

  old_randseed = random_seed;
  random_seed = drawn_room;
  prandom(1); // discard
  for (row = 0; row < 3; row++)
  {
    for (subrow = 0; subrow < 4; subrow++)
    {
      if (subrow % 2)
      {
        color_base = 0x61; // 0x61..0x64 in subrow 1 and 3
      }
      else
      {
        color_base = 0x66; // 0x66..0x69 in subrow 0 and 2
      }
      prev_color = -1;
      for (column = 0; column <= 10; ++column)
      {
        do
        {
          color = color_base + prandom(3);
        } while (color == prev_color);
        palace_wall_colors[44 * row + 11 * subrow + column] = color;
        //palace_wall_colors[row][subrow][column] = color;
        prev_color = color;
      }
    }
  }
  random_seed = old_randseed;
}

// seg000:1D2C
void __pascal far load_kid_sprite()
{
  load_chtab_from_file(id_chtab_2_kid, 400, "KID.DAT", 1 << 7);
}

// seg000:1F7B
void __pascal far parse_cmdline_sound()
{
    // Use digi (wave) sounds and MIDI music.
    sound_flags |= sfDigi;
    sound_flags |= sfMidi;
    sound_mode = smSblast;
}

// seg002:0000
void __pascal far do_init_shad(const byte *source, int seq_index)
{
  memcpy_near(&Char, source, 7);
  seqtbl_offset_char(seq_index);
  Char.charid = charid_1_shadow;
  demo_time = 0;
  guard_skill = 3;
  guardhp_delta = guardhp_curr = guardhp_max = 4;
  saveshad();
}

// seg002:0044
void __pascal far get_guard_hp()
{
  guardhp_delta = guardhp_curr = guardhp_max = custom->extrastrength[guard_skill] + custom->tbl_guard_hp[current_level];
}

// seg002:0064
void __pascal far check_shadow()
{
  offguard = 0;
  if (current_level == 12)
  {
    // Special event: level 12 shadow
    if (!united_with_shadow && drawn_room == 15)
    {
      Char.room = drawn_room;
      if (get_tile(15, 1, 0) == tiles_22_sword)
      {
        return;
      }
      shadow_initialized = 0;
      do_init_shad(/*&*/ custom->init_shad_12, 7 /*fall*/);
      return;
    }
  }
  else if (current_level == 6)
  {
    // Special event: level 6 shadow
    Char.room = drawn_room;
    if (Char.room == 1)
    {
      if (leveldoor_open != 0x4D)
      {
        leveldoor_open = 0x4D;
      }
      do_init_shad(/*&*/ custom->init_shad_6, 2 /*stand*/);
      return;
    }
  }
  else if (current_level == 5)
  {
    // Special event: level 5 shadow
    Char.room = drawn_room;
    if (Char.room == 24)
    {
      if (get_tile(24, 3, 0) != tiles_10_potion)
      {
        return;
      }
      do_init_shad(/*&*/ custom->init_shad_5, 2 /*stand*/);
      return;
    }
  }
  enter_guard();
}

// seg002:0112
void __pascal far enter_guard()
{
  word room_minus_1;
  word guard_tile;
  word frame;
  byte seq_hi;
  // arrays are indexed 0..23 instead of 1..24
  room_minus_1 = drawn_room - 1;
  frame = Char.frame; // hm?
  guard_tile = level.guards_tile[room_minus_1];
  if (guard_tile >= 30)
    return;

  Char.room = drawn_room;
  Char.curr_row = guard_tile / 10;
  Char.y = y_land[Char.curr_row + 1];
  Char.x = level.guards_x[room_minus_1];
  Char.curr_col = get_tile_div_mod_m7(Char.x);
  Char.direction = level.guards_dir[room_minus_1];
  // only regular guards have different colors (and only on VGA)
   curr_guard_color = 0;

#ifdef REMEMBER_GUARD_HP
  int remembered_hp = (level.guards_color[room_minus_1] & 0xF0) >> 4;
#endif
  curr_guard_color &= 0x0F; // added; only least significant 4 bits are used for guard color

  // level 3 has skeletons with infinite lives
  //if (current_level == 3) {
  if (custom->tbl_guard_type[current_level] == 2)
  {
    Char.charid = charid_4_skeleton;
  }
  else
  {
    Char.charid = charid_2_guard;
  }
  seq_hi = level.guards_seq_hi[room_minus_1];
  if (seq_hi == 0)
  {
    if (Char.charid == charid_4_skeleton)
    {
      Char.sword = sword_2_drawn;
      seqtbl_offset_char(seq_63_guard_stand_active); // stand active (when entering room) (skeleton)
    }
    else
    {
      Char.sword = sword_0_sheathed;
      seqtbl_offset_char(seq_77_guard_stand_inactive); // stand inactive (when entering room)
    }
  }
  else
  {
    Char.curr_seq = level.guards_seq_lo[room_minus_1] + (seq_hi << 8);
  }
  play_seq();
  guard_skill = level.guards_skill[room_minus_1];
  if (guard_skill >= NUM_GUARD_SKILLS)
  {
    guard_skill = 3;
  }
  frame = Char.frame;
  if (frame == frame_185_dead || frame == frame_177_spiked || frame == frame_178_chomped)
  {
    Char.alive = 1;
    guardhp_curr = 0;
  }
  else
  {
    Char.alive = -1;
    justblocked = 0;
    guard_refrac = 0;
    is_guard_notice = 0;
    get_guard_hp();
  }
  Char.fall_y = 0;
  Char.fall_x = 0;
  Char.action = actions_1_run_jump;
  saveshad();
}

// seg002:0269
void __pascal far check_guard_fallout()
{
  if (Guard.direction == dir_56_none || Guard.y < 211)
  {
    return;
  }
  if (Guard.charid == charid_1_shadow)
  {
    if (Guard.action != actions_4_in_freefall)
    {
      return;
    }
    loadshad();
    clear_char();
    saveshad();
  }
  else if (Guard.charid == charid_4_skeleton &&
           (Guard.room = level.roomlinks[Guard.room - 1].down) == /*3*/ custom->skeleton_reappear_room)
  {
    // if skeleton falls down into room 3
    Guard.x = /*133*/ custom->skeleton_reappear_x;
    Guard.curr_row = /*1*/ custom->skeleton_reappear_row;
    Guard.direction = /*dir_0_right*/ custom->skeleton_reappear_dir;
    Guard.alive = -1;
    leave_guard();
  }
  else
  {
    on_guard_killed();
    level.guards_tile[drawn_room - 1] = -1;
    Guard.direction = dir_56_none;
    guardhp_curr = 0;
  }
}

// seg002:02F5
void __pascal far leave_guard()
{
  word room_minus_1;
  if (Guard.direction == dir_56_none || Guard.charid == charid_1_shadow || Guard.charid == charid_24_mouse)
  {
    return;
  }
  // arrays are indexed 0..23 instead of 1..24
  room_minus_1 = Guard.room - 1;
  level.guards_tile[room_minus_1] = get_tilepos(0, Guard.curr_row);

  level.guards_color[room_minus_1] = curr_guard_color & 0x0F; // restriction to 4 bits added

  level.guards_x[room_minus_1] = Guard.x;
  level.guards_dir[room_minus_1] = Guard.direction;
  level.guards_skill[room_minus_1] = guard_skill;
  if (Guard.alive < 0)
  {
    level.guards_seq_hi[room_minus_1] = 0;
  }
  else
  {
    level.guards_seq_lo[room_minus_1] = Guard.curr_seq;
    level.guards_seq_hi[room_minus_1] = Guard.curr_seq >> 8;
  }
  Guard.direction = dir_56_none;
  guardhp_curr = 0;
}



// seg002:0486
int __pascal far goto_other_room(short direction)
{
  //printf("goto_other_room: direction = %d, Char.room = %d\n", direction, Char.room);
  short opposite_dir;
  byte other_room = ((byte *)&level.roomlinks[Char.room - 1])[direction];
  Char.room = other_room;
  if (direction == 0)
  {
    // left
    Char.x += 140;
    opposite_dir = 1;
  }
  else if (direction == 1)
  {
    // right
    Char.x -= 140;
    opposite_dir = 0;
  }
  else if (direction == 2)
  {
    // up
    Char.y += 189;
    Char.curr_row = y_to_row_mod4(Char.y);
    opposite_dir = 3;
  }
  else
  {
    // down
    Char.y -= 189;
    Char.curr_row = y_to_row_mod4(Char.y);
    opposite_dir = 2;
  }
  return opposite_dir;
}

// seg002:039E
void __pascal far follow_guard()
{
  level.guards_tile[Kid.room - 1] = 0xFF;
  level.guards_tile[Guard.room - 1] = 0xFF;
  loadshad();
  goto_other_room(roomleave_result);
  saveshad();
}

// seg002:0504
short __pascal far leave_room()
{
  short frame;
  word action;
  short chary;
  short leave_dir;
  chary = Char.y;
  action = Char.action;
  frame = Char.frame;
  if (action != actions_5_bumped &&
      action != actions_4_in_freefall &&
      action != actions_3_in_midair &&
      (sbyte)chary < 10 && (sbyte)chary > -16)
  {
    leave_dir = 2; // up
  }
  else if (chary >= 211)
  {
    leave_dir = 3; // down
  }
  else if (
    // frames 135..149: climb up
    (frame >= frame_135_climbing_1 && frame < 150) ||
    // frames 110..119: standing up from crouch
    (frame >= frame_110_stand_up_from_crouch_1 && frame < 120) ||
    // frames 150..162: with sword
    (frame >= frame_150_parry && frame < 163
       ) ||
    // frames 166..168: with sword
    (frame >= frame_166_stand_inactive && frame < 169) ||
    action == actions_7_turn // turn
  )
  {
    return -1;
  }
  else if (Char.direction != dir_0_right)
  {
    // looking left
    if (char_x_left <= 54)
    {
      leave_dir = 0; // left
    }
    else if (char_x_left >= 198)
    {
      leave_dir = 1; // right
    }
    else
    {
      return -1;
    }
  }
  else
  {
    // looking right
    get_tile(Char.room, 9, Char.curr_row);
    if (curr_tile2 != tiles_7_doortop_with_floor &&
        curr_tile2 != tiles_12_doortop &&
        char_x_right >= 201)
    {
      leave_dir = 1; // right
    }
    else if (char_x_right <= 57)
    {
      leave_dir = 0; // left
    }
    else
    {
      return -1;
    }
  }
  switch (leave_dir)
  {
  case 0: // left
    play_mirr_mus();
    level3_set_chkp();
    Jaffar_exit();
    break;
  case 1: // right
    sword_disappears();
    meet_Jaffar();
    break;
  //case 2: // up
  case 3: // down
    // Special event: falling exit
    if (current_level == custom->falling_exit_level /*6*/ && Char.room == custom->falling_exit_room /*1*/)
    {
      return -2;
    }
    break;
  }
  goto_other_room(leave_dir);
#ifdef USE_REPLAY
  if (skipping_replay && replay_seek_target == replay_seek_0_next_room)
    skipping_replay = 0;
#endif
  return leave_dir;
}

// seg002:03C7
void __pascal far exit_room()
{
  word leave;
  word kid_room_m1;
  leave = 0;
  if (exit_room_timer != 0)
  {
    --exit_room_timer;
      return;
  }
  loadkid();
  load_frame_to_obj();
  set_char_collision();
  roomleave_result = leave_room();
  if (roomleave_result < 0)
  {
    return;
  }
  savekid();
  next_room = Char.room;
  if (Guard.direction == dir_56_none)
    return;
  if (Guard.alive < 0 && Guard.sword == sword_2_drawn)
  {
    kid_room_m1 = Kid.room - 1;
    // kid_room_m1 might be 65535 (-1) when the prince fell out of the level (to room 0) while a guard was active.
    // In this case, the indexing in the following condition crashes on Linux.
    if ((kid_room_m1 >= 0 && kid_room_m1 <= 23) &&
        (level.guards_tile[kid_room_m1] >= 30 || level.guards_seq_hi[kid_room_m1] != 0))
    {
      if (roomleave_result == 0)
      {
        // left
        if (Guard.x >= 91)
          leave = 1;
      }
      else if (roomleave_result == 1)
      {
        // right
        if (Guard.x < 165)
          leave = 1;
      }
      else if (roomleave_result == 2)
      {
        // up
        if (Guard.curr_row >= 0)
          leave = 1;
      }
      else
      {
        // down
        if (Guard.curr_row < 3)
          leave = 1;
      }
    }
    else
    {
      leave = 1;
    }
  }
  else
  {
    leave = 1;
  }
  if (leave)
  {
    leave_guard();
  }
  else
  {
    follow_guard();
  }
}

// seg002:0643
void __pascal far Jaffar_exit()
{
  if (leveldoor_open == 2)
  {
    get_tile(24, 0, 0);
    trigger_button(0, 0, -1);
  }
}

// seg002:0665
void __pascal far level3_set_chkp()
{
  // Special event: set checkpoint
  if (current_level == /*3*/ custom->checkpoint_level && Char.room == 7 /* TODO: add a custom option */)
  {
    checkpoint = 1;
    hitp_beg_lev = hitp_max;
  }
}

// seg002:0680
void __pascal far sword_disappears()
{
  // Special event: sword disappears
  if (current_level == 12 && Char.room == 18)
  {
    get_tile(15, 1, 0);
    curr_room_tiles[curr_tilepos] = tiles_1_floor;
    curr_room_modif[curr_tilepos] = 0; // added, a nonzero modifier may show fake tiles
  }
}

// seg002:06AE
void __pascal far meet_Jaffar()
{
  // Special event: play music
  if (current_level == 13 && leveldoor_open == 0 && Char.room == 3)
  {
    // Special event: Jaffar waits a bit (28/12=2.33 seconds)
    guard_notice_timer = 28;
  }
}

// seg002:06D3
void __pascal far play_mirr_mus()
{
  // Special event: mirror music
  if (
    leveldoor_open != 0 &&
    leveldoor_open != 0x4D && // was the music played already?
    current_level == /*4*/ custom->mirror_level &&
    Char.curr_row == /*0*/ custom->mirror_row &&
    Char.room == 11 /* TODO: add a custom option */
  )
  {
    leveldoor_open = 0x4D;
  }
}

// seg002:0706
void __pascal far move_0_nothing()
{
  control_shift = 0;
  control_y = 0;
  control_x = 0;
  control_shift2 = 0;
  control_down = 0;
  control_up = 0;
  control_backward = 0;
  control_forward = 0;
}

// seg002:0721
void __pascal far move_1_forward()
{
  control_x = -1;
  control_forward = -1;
}

// seg002:072A
void __pascal far move_2_backward()
{
  control_backward = -1;
  control_x = 1;
}

// seg002:0735
void __pascal far move_3_up()
{
  control_y = -1;
  control_up = -1;
}

// seg002:073E
void __pascal far move_4_down()
{
  control_down = -1;
  control_y = 1;
}

// seg002:0749
void __pascal far move_up_back()
{
  control_up = -1;
  move_2_backward();
}

// seg002:0753
void __pascal far move_down_back()
{
  control_down = -1;
  move_2_backward();
}

// seg002:075D
void __pascal far move_down_forw()
{
  control_down = -1;
  move_1_forward();
}

// seg002:0767
void __pascal far move_6_shift()
{
  control_shift = -1;
  control_shift2 = -1;
}

// seg002:0770
void __pascal far move_7()
{
  control_shift = 0;
}

// seg002:0776
void __pascal far autocontrol_opponent()
{
  word charid;
  move_0_nothing();
  charid = Char.charid;
  if (charid == charid_0_kid)
  {
    autocontrol_kid();
  }
  else
  {
    if (justblocked)
      --justblocked;
    if (kid_sword_strike)
      --kid_sword_strike;
    if (guard_refrac)
      --guard_refrac;
    if (charid == charid_24_mouse)
    {
      autocontrol_mouse();
    }
    else if (charid == charid_4_skeleton)
    {
      autocontrol_skeleton();
    }
    else if (charid == charid_1_shadow)
    {
      autocontrol_shadow();
    }
    else if (current_level == 13)
    {
      autocontrol_Jaffar();
    }
    else
    {
      autocontrol_guard();
    }
  }
}

// seg002:07EB
void __pascal far autocontrol_mouse()
{
  if (Char.direction == dir_56_none)
  {
    return;
  }
  if (Char.action == actions_0_stand)
  {
    if (Char.x >= 200)
    {
      clear_char();
    }
  }
  else
  {
    if (Char.x < 166)
    {
      seqtbl_offset_char(seq_107_mouse_stand_up_and_go); // mouse
      play_seq();
    }
  }
}

// seg002:081D
void __pascal far autocontrol_shadow()
{
  if (current_level == 4)
  {
    autocontrol_shadow_level4();
  }
  else if (current_level == 5)
  {
    autocontrol_shadow_level5();
  }
  else if (current_level == 6)
  {
    autocontrol_shadow_level6();
  }
  else if (current_level == 12)
  {
    autocontrol_shadow_level12();
  }
}

// seg002:0850
void __pascal far autocontrol_skeleton()
{
  Char.sword = sword_2_drawn;
  autocontrol_guard();
}

// seg002:085A
void __pascal far autocontrol_Jaffar()
{
  autocontrol_guard();
}

// seg002:085F
void __pascal far autocontrol_kid()
{
  autocontrol_guard();
}

// seg002:0864
void __pascal far autocontrol_guard()
{
  if (Char.sword < sword_2_drawn)
  {
    autocontrol_guard_inactive();
  }
  else
  {
    autocontrol_guard_active();
  }
}

// seg002:0876
void __pascal far autocontrol_guard_inactive()
{
  short distance;
  if (Kid.alive >= 0)
    return;
  distance = char_opp_dist();
  if (Opp.curr_row != Char.curr_row || (word)distance < (word)-8)
  {
    // If Kid made a sound ...
    if (is_guard_notice)
    {
      is_guard_notice = 0;
      if (distance < 0)
      {
        // ... and Kid is behind Guard, Guard turns around.
        if ((word)distance < (word)-4)
        {
          move_4_down();
        }
        return;
      }
    }
    else if (distance < 0)
    {
      return;
    }
  }
  if (can_guard_see_kid)
  {
    // If Guard can see Kid, Guard moves to fighting pose.
    if (current_level != 13 || guard_notice_timer == 0)
    {
      move_down_forw();
    }
  }
}

// seg002:08DC
void __pascal far autocontrol_guard_active()
{
  short opp_frame;
  short char_frame;
  short distance;
  char_frame = Char.frame;
  if (char_frame != frame_166_stand_inactive && char_frame >= 150 && can_guard_see_kid != 1)
  {
    if (can_guard_see_kid == 0)
    {
      if (droppedout != 0)
      {
        guard_follows_kid_down();
        //return;
      }
      else if (Char.charid != charid_4_skeleton)
      {
        move_down_back();
      }
      //return;
    }
    else
    { // can_guard_see_kid == 2
      opp_frame = Opp.frame;
      distance = char_opp_dist();
      if (distance >= 12 &&
          // frames 102..117: falling and landing
          opp_frame >= frame_102_start_fall_1 && opp_frame < frame_118_stand_up_from_crouch_9 &&
          Opp.action == actions_5_bumped)
      {
        return;
      }
      if (distance < 35)
      {
        if ((Char.sword < sword_2_drawn && distance < 8) || distance < 12)
        {
          if (Char.direction == Opp.direction)
          {
            // turn around
            move_2_backward();
            //return;
          }
          else
          {
            move_1_forward();
            //return;
          }
        }
        else
        {
          autocontrol_guard_kid_in_sight(distance);
          //return;
        }
      }
      else
      {
        if (guard_refrac != 0)
          return;
        if (Char.direction != Opp.direction)
        {
          // frames 7..14: running
          // frames 34..43: run-jump
          if (opp_frame >= frame_7_run && opp_frame < 15)
          {
            if (distance < 40)
              move_6_shift();
            return;
          }
          else if (opp_frame >= frame_34_start_run_jump_1 && opp_frame < 44)
          {
            if (distance < 50)
              move_6_shift();
            return;
            //return;
          }
        }
        autocontrol_guard_kid_far();
      }
      //...
    }
    //...
  }
}

// seg006:0FC3
int __pascal far wall_type(byte tiletype)
{
  switch (tiletype)
  {
  case tiles_4_gate:
  case tiles_7_doortop_with_floor:
  case tiles_12_doortop:
    return 1; // wall at right
  case tiles_13_mirror:
    return 2; // wall at left
  case tiles_18_chomper:
    return 3; // chomper at left
  case tiles_20_wall:
    return 4; // wall at both sides
  default:
    return 0; // no wall
  }
}

// seg002:09CB
void __pascal far autocontrol_guard_kid_far()
{
  if (tile_is_floor(get_tile_infrontof_char()) ||
      tile_is_floor(get_tile_infrontof2_char()))
  {
    move_1_forward();
  }
  else
  {
    move_2_backward();
  }
}

// seg002:09F8
void __pascal far guard_follows_kid_down()
{
  // This is called from autocontrol_guard_active, so char=Guard, Opp=Kid
  word opp_action;
  opp_action = Opp.action;
  if (opp_action == actions_2_hang_climb || opp_action == actions_6_hang_straight)
  {
    return;
  }
  if ( // there is wall in front of Guard
    wall_type(get_tile_infrontof_char()) != 0 ||
    (!tile_is_floor(curr_tile2) && ((get_tile(curr_room, tile_col, ++tile_row) == tiles_2_spike ||
                                     // Guard would fall on loose floor
                                     curr_tile2 == tiles_11_loose ||
                                     // ... or wall (?)
                                     wall_type(curr_tile2) != 0 ||
                                     // ... or into a chasm
                                     !tile_is_floor(curr_tile2)) ||
                                    // ... or Kid is not below
                                    Char.curr_row + 1 != Opp.curr_row)))
  {
    // don't follow
    droppedout = 0;
    move_2_backward();
  }
  else
  {
    // follow
    move_1_forward();
  }
}

// seg002:0A93
void __pascal far autocontrol_guard_kid_in_sight(short distance)
{
  if (Opp.sword == sword_2_drawn)
  {
    autocontrol_guard_kid_armed(distance);
  }
  else if (guard_refrac == 0)
  {
    if (distance < 29)
    {
      move_6_shift();
    }
    else
    {
      move_1_forward();
    }
  }
}

// seg002:0AC1
void __pascal far autocontrol_guard_kid_armed(short distance)
{
  if (distance < 10 || distance >= 29)
  {
    guard_advance();
  }
  else
  {
    guard_block();
    if (guard_refrac == 0)
    {
      if (distance < 12 || distance >= 29)
      {
        guard_advance();
      }
      else
      {
        guard_strike();
      }
    }
  }
}

// seg002:0AF5
void __pascal far guard_advance()
{
  if (guard_skill == 0 || kid_sword_strike == 0)
  {
    if (custom->advprob[guard_skill] > prandom(255))
    {
      move_1_forward();
    }
  }
}

// seg002:0B1D
void __pascal far guard_block()
{
  word opp_frame;
  opp_frame = Opp.frame;
  if (opp_frame == frame_152_strike_2 || opp_frame == frame_153_strike_3 || opp_frame == frame_162_block_to_strike)
  {
    if (justblocked != 0)
    {
      if (custom->impblockprob[guard_skill] > prandom(255))
      {
        move_3_up();
      }
    }
    else
    {
      if (custom->blockprob[guard_skill] > prandom(255))
      {
        move_3_up();
      }
    }
  }
}

// seg002:0B73
void __pascal far guard_strike()
{
  word opp_frame;
  word char_frame;
  opp_frame = Opp.frame;
  if (opp_frame == frame_169_begin_block || opp_frame == frame_151_strike_1)
    return;
  char_frame = Char.frame;
  if (char_frame == frame_161_parry || char_frame == frame_150_parry)
  {
    if (custom->restrikeprob[guard_skill] > prandom(255))
    {
      move_6_shift();
    }
  }
  else
  {
    if (custom->strikeprob[guard_skill] > prandom(255))
    {
      move_6_shift();
    }
  }
}

// seg002:0BCD
void __pascal far hurt_by_sword()
{
  short distance;
  if (Char.alive >= 0)
    return;
  if (Char.sword != sword_2_drawn)
  {
    // Being hurt when not in fighting pose means death.
    take_hp(100);
    seqtbl_offset_char(seq_85_stabbed_to_death); // dying (stabbed unarmed)
  loc_4276:
    if (get_tile_behind_char() != 0 ||
        (distance = distance_to_edge_weight()) < 4)
    {
      seqtbl_offset_char(seq_85_stabbed_to_death); // dying (stabbed)
      if (Char.charid != charid_0_kid &&
          Char.direction < dir_0_right && // looking left
          (curr_tile2 == tiles_4_gate || get_tile_at_char() == tiles_4_gate))
      {
          Char.x = x_bump[tile_col - (curr_tile2 != tiles_4_gate) + 5] + 7;
        Char.x = char_dx_forward(10);
      }
      Char.y = y_land[Char.curr_row + 1];
      Char.fall_y = 0;
    }
    else
    {
      Char.x = char_dx_forward(distance - 20);
      load_fram_det_col();
      inc_curr_row();
      seqtbl_offset_char(seq_81_kid_pushed_off_ledge); // Kid/Guard is killed and pushed off the ledge
    }
  }
  else
  {
    // You can't hurt skeletons
    if (Char.charid != charid_4_skeleton)
    {
      if (take_hp(1))
        goto loc_4276;
    }
    seqtbl_offset_char(seq_74_hit_by_sword); // being hit with sword
    Char.y = y_land[Char.curr_row + 1];
    Char.fall_y = 0;
  }
  // sound 13: Kid hurt (by sword), sound 12: Guard hurt (by sword)
  play_seq();
}

// seg002:0CD4
void __pascal far check_sword_hurt()
{
  if (Guard.action == actions_99_hurt)
  {
    if (Kid.action == actions_99_hurt)
    {
      Kid.action = actions_1_run_jump;
    }
    loadshad();
    hurt_by_sword();
    saveshad();
    guard_refrac = custom->refractimer[guard_skill];
  }
  else
  {
    if (Kid.action == actions_99_hurt)
    {
      loadkid();
      hurt_by_sword();
      savekid();
    }
  }
}

// seg002:0D1A
void __pascal far check_sword_hurting()
{
  short kid_frame;
  kid_frame = Kid.frame;
  // frames 217..228: go up on stairs
  if (kid_frame != 0 && (kid_frame < frame_219_exit_stairs_3 || kid_frame >= 229))
  {
    loadshad_and_opp();
    check_hurting();
    saveshad_and_opp();
    loadkid_and_opp();
    check_hurting();
    savekid_and_opp();
  }
}

// seg002:0D56
void __pascal far check_hurting()
{
  short opp_frame, char_frame, distance, min_hurt_range;
  if (Char.sword != sword_2_drawn)
    return;
  if (Char.curr_row != Opp.curr_row)
    return;
  char_frame = Char.frame;
  // frames 153..154: poking with sword
  if (char_frame != frame_153_strike_3 && char_frame != frame_154_poking)
    return;
  // If char is poking ...
  distance = char_opp_dist();
  opp_frame = Opp.frame;
  // frames 161 and 150: parrying
  if (distance < 0 || distance >= 29 ||
      (opp_frame != frame_161_parry && opp_frame != frame_150_parry))
  {
    // ... and Opp is not parrying
    // frame 154: poking
    if (Char.frame == frame_154_poking)
    {
      if (Opp.sword < sword_2_drawn)
      {
        min_hurt_range = 8;
      }
      else
      {
        min_hurt_range = 12;
      }
      distance = char_opp_dist();
      if (distance >= min_hurt_range && distance < 29)
      {
        Opp.action = actions_99_hurt;
      }
    }
  }
  else
  {
    Opp.frame = frame_161_parry;
    if (Char.charid != charid_0_kid)
    {
      justblocked = 4;
    }
    seqtbl_offset_char(seq_69_attack_was_parried); // attack was parried
    play_seq();
  }
  if (Char.direction == dir_56_none)
    return; // Fix looping "sword moving" sound.
  // frame 154: poking
  // frame 161: parrying
  if (Char.frame == frame_154_poking && Opp.frame != frame_161_parry && Opp.action != actions_99_hurt)
  {
  }
}

// seg002:0E1F
void __pascal far check_skel()
{
  // Special event: skeleton wakes
  if (current_level == /*3*/ custom->skeleton_level &&
      Guard.direction == dir_56_none &&
      drawn_room == /*1*/ custom->skeleton_room &&
      (leveldoor_open != 0 || !custom->skeleton_require_open_level_door) &&
      (Kid.curr_col == /*2*/ custom->skeleton_trigger_column_1 ||
       Kid.curr_col == /*3*/ custom->skeleton_trigger_column_2))
  {
    get_tile(drawn_room, /*5*/ custom->skeleton_column, /*1*/ custom->skeleton_row);
    if (curr_tile2 == tiles_21_skeleton)
    {
      // erase skeleton
      curr_room_tiles[curr_tilepos] = tiles_1_floor;
      redraw_height = 24;
      ++curr_tilepos;
      Char.room = drawn_room;
      Char.curr_row = /*1*/ custom->skeleton_row;
      Char.y = y_land[Char.curr_row + 1];
      Char.curr_col = /*5*/ custom->skeleton_column;
      Char.x = x_bump[Char.curr_col + 5] + 14;
      Char.direction = dir_FF_left;
      seqtbl_offset_char(seq_88_skel_wake_up); // skel wake up
      play_seq();
      guard_skill = /*2*/ custom->skeleton_skill;
      Char.alive = -1;
      guardhp_max = guardhp_curr = 3;
      Char.fall_x = Char.fall_y = 0;
      is_guard_notice = guard_refrac = 0;
      Char.sword = sword_2_drawn;
      Char.charid = charid_4_skeleton;
      saveshad();
    }
  }
}

// seg002:0F3F
void __pascal far do_auto_moves(const auto_move_type *moves_ptr)
{
  short demoindex;
  short curr_move;
  if (demo_time >= 0xFE)
    return;
  ++demo_time;
  demoindex = demo_index;
  if (moves_ptr[demoindex].time <= demo_time)
  {
    ++demo_index;
  }
  else
  {
    demoindex = demo_index - 1;
  }
  curr_move = moves_ptr[demoindex].move;
  switch (curr_move)
  {
  case -1:
    break;
  case 0:
    move_0_nothing();
    break;
  case 1:
    move_1_forward();
    break;
  case 2:
    move_2_backward();
    break;
  case 3:
    move_3_up();
    break;
  case 4:
    move_4_down();
    break;
  case 5:
    move_3_up();
    move_1_forward();
    break;
  case 6:
    move_6_shift();
    break;
  case 7:
    move_7();
    break;
  }
}

// seg002:1000
void __pascal far autocontrol_shadow_level4()
{
  if (Char.room == 4)
  {
    if (Char.x < 80)
    {
      clear_char();
    }
    else
    {
      move_1_forward();
    }
  }
}

// seg002:101A
void __pascal far autocontrol_shadow_level5()
{
  if (Char.room == 24)
  {
    if (demo_time == 0)
    {
      get_tile(24, 1, 0);
      // is the door open?
      if (curr_room_modif[curr_tilepos] < 80)
        return;
      demo_index = 0;
    }
    do_auto_moves(custom->shad_drink_move);
    if (Char.x < 15)
    {
      clear_char();
    }
  }
}

// seg002:1064
void __pascal far autocontrol_shadow_level6()
{
  if (Char.room == 1 &&
      Kid.frame == frame_43_running_jump_4 && // a frame in run-jump
      Kid.x < 128)
  {
    move_6_shift();
    move_1_forward();
  }
}

// seg002:1082
void __pascal far autocontrol_shadow_level12()
{
  short opp_frame;
  short xdiff;
  if (Char.room == 15 && shadow_initialized == 0)
  {
    if (Opp.x >= 150)
    {
      do_init_shad(/*&*/ custom->init_shad_12, 7 /*fall*/);
      return;
    }
    shadow_initialized = 1;
  }
  if (Char.sword >= sword_2_drawn)
  {
    // if the Kid puts his sword away, the shadow does the same,
    // but only if the shadow was already hurt (?)
    if (offguard == 0 || guard_refrac == 0)
    {
      autocontrol_guard_active();
    }
    else
    {
      move_4_down();
    }
    return;
  }
  if (Opp.sword >= sword_2_drawn || offguard == 0)
  {
    xdiff = 0x7000; // bugfix/workaround
    // This behavior matches the DOS version but not the Apple II source.
    if (can_guard_see_kid < 2 || (xdiff = char_opp_dist()) >= 90)
    {
      if (xdiff < 0)
      {
        move_2_backward();
      }
      return;
    }
    // Shadow draws his sword
    if (Char.frame == 15)
    {
      move_down_forw();
    }
    return;
  }
  if (char_opp_dist() < 10)
  {
    // unite with the shadow
    flash_color = color_15_brightwhite; // white
    flash_time = 18;
    // get an extra HP for uniting the shadow
    add_life();
    // time of Kid-shadow flash
    united_with_shadow = 42;
    // put the Kid where the shadow was
    Char.charid = charid_0_kid;
    savekid();
    // remove the shadow
    clear_char();
    return;
  }
  if (can_guard_see_kid == 2)
  {
    // If Kid runs to shadow, shadow runs to Kid.
    opp_frame = Opp.frame;
    // frames 1..14: running
    // frames 121..132: stepping
    if ((opp_frame >= frame_3_start_run && opp_frame < frame_15_stand) ||
        (opp_frame >= frame_127_stepping_7 && opp_frame < 133))
    {
      move_1_forward();
    }
  }
}

// seg003:0000
void __pascal far init_game(int level)
{
  load_kid_sprite();
  text_time_remaining = 0;
  text_time_total = 0;
  checkpoint = 0;
  upside_down = 0; // N.B. upside_down is also reset in set_start_pos()
  resurrect_time = 0;
  if (!dont_reset_time)
  {
    rem_min = custom->start_minutes_left; // 60
    rem_tick = custom->start_ticks_left;  // 719
    hitp_beg_lev = custom->start_hitp;    // 3
  }

  need_level1_music = (level == /*1*/ custom->intro_music_level);
  play_level(level);
}

// seg003:005C
void __pascal far play_level(int level_number)
{
  if (level_number == custom->copyprot_level)
  {
    level_number = 15;
  }
  for (;;)
  {
    if (demo_mode && level_number > 2)
    {
      start_level = -1;
      need_quotes = 1;
      start_game();
    }
    if (level_number != current_level)
    {
    }
    if (level_number != current_level)
    {
      load_lev_spr(level_number);
    }
    load_level();
    pos_guards();
    clear_coll_rooms();
    clear_saved_ctrl();
    drawn_room = 0;
    mobs_count = 0;
    trobs_count = 0;
    next_sound = -1;
    holding_sword = 0;
    grab_timer = 0;
    can_guard_see_kid = 0;
    united_with_shadow = 0;
    flash_time = 0;
    leveldoor_open = 0;
    demo_index = 0;
    demo_time = 0;
    guardhp_curr = 0;
    hitp_delta = 0;
    Guard.charid = charid_2_guard;
    Guard.direction = dir_56_none;
    do_startpos();
    have_sword = /*(level_number != 1)*/ (level_number == 0 || level_number >= custom->have_sword_from_level);
    find_start_level_door();
    // busy waiting?
    draw_level_first();
    level_number = play_level_2();
    // hacked...
    if (level_number == custom->copyprot_level && !demo_mode)
    {
      level_number = 15;
    }
    else
    {
      if (level_number == 16)
      {
        level_number = custom->copyprot_level;
        custom->copyprot_level = -1;
      }
    }
  }
}

// seg003:01A3
void __pascal far do_startpos()
{
  word x;
  // Special event: start at checkpoint
  if (current_level == /*3*/ custom->checkpoint_level && checkpoint)
  {
    level.start_dir = /*dir_FF_left*/ custom->checkpoint_respawn_dir;
    level.start_room = /*2*/ custom->checkpoint_respawn_room;
    level.start_pos = /*6*/ custom->checkpoint_respawn_tilepos;
    // Special event: remove loose floor
    get_tile(/*7*/ custom->checkpoint_clear_tile_room,
             /*4*/ custom->checkpoint_clear_tile_col,
             /*0*/ custom->checkpoint_clear_tile_row);
    curr_room_tiles[curr_tilepos] = tiles_0_empty;
  }
  next_room = Char.room = level.start_room;
  x = level.start_pos;
  Char.curr_col = x % 10;
  Char.curr_row = x / 10;
  Char.x = x_bump[Char.curr_col + 5] + 14;
  // Start in the opposite direction (and turn into the correct one).
  Char.direction = ~level.start_dir;
  if (seamless == 0)
  {
    if (current_level != 0)
    {
      x = hitp_beg_lev;
    }
    else
    {
      // HP on demo level
      x = /*4*/ custom->demo_hitp;
    }
    hitp_max = hitp_curr = x;
  }
  if (/*current_level == 1*/ custom->tbl_entry_pose[current_level] == 1)
  {
    // Special event: press button + falling entry
    get_tile(5, 2, 0);
    trigger_button(0, 0, -1);
    seqtbl_offset_char(seq_7_fall); // fall
  }
  else if (/*current_level == 13*/ custom->tbl_entry_pose[current_level] == 2)
  {
    // Special event: running entry
    seqtbl_offset_char(seq_84_run); // run
  }
  else
  {
    seqtbl_offset_char(seq_5_turn); // turn
  }
  set_start_pos();
}

// seg003:028A
void __pascal far set_start_pos()
{
  Char.y = y_land[Char.curr_row + 1];
  Char.alive = -1;
  Char.charid = charid_0_kid;
  is_screaming = 0;
  knock = 0;
  upside_down = custom->start_upside_down; // 0
  is_feather_fall = 0;
  Char.fall_y = 0;
  Char.fall_x = 0;
  offguard = 0;
  Char.sword = sword_0_sheathed;
  droppedout = 0;
  play_seq();
  if (current_level == /*7*/ custom->falling_entry_level && Char.room == /*17*/ custom->falling_entry_room)
  {
    // Special event: level 7 falling entry
    // level 7, room 17: show room below
    goto_other_room(3);
  }
  savekid();
}

// seg003:02E6
void __pascal far find_start_level_door()
{
  short tilepos;
  get_room_address(Kid.room);
  for (tilepos = 0; tilepos < 30; ++tilepos)
  {
    if ((curr_room_tiles[tilepos] & 0x1F) == tiles_16_level_door_left)
    {
      start_level_door(Kid.room, tilepos);
    }
  }
}

// seg003:0326
void __pascal far draw_level_first()
{
  next_room = Kid.room;
  check_the_end();
  if (custom->tbl_level_type[current_level])
  {
    gen_palace_wall_colors();
  }
  redraw_screen(0);
}

// seg003:037B
void __pascal far redraw_screen(int drawing_different_room)
{

  different_room = 0;
  clear_kbd_buf();
  exit_room_timer = 2;
}


// seg003:04F8
int __pascal far play_level_2()
{
  while (1)
  { // main loop
    guardhp_delta = 0;
    hitp_delta = 0;
    timers();
    play_frame();

    if (is_restart_level)
    {
      is_restart_level = 0;
      return current_level;
    }
    else
    {
      if (next_level == current_level)
      {
      }
      else
      {
        hitp_beg_lev = hitp_max;
        checkpoint = 0;

        return next_level;
      }
    }
  }
}

// seg003:0706
void __pascal far check_knock()
{
  if (knock)
  {
    do_knock(Char.room, Char.curr_row - (knock > 0));
    knock = 0;
  }
}

// seg003:0735
void __pascal far timers()
{
  if (united_with_shadow > 0)
  {
    --united_with_shadow;
    if (united_with_shadow == 0)
    {
      --united_with_shadow;
    }
  }
  if (guard_notice_timer > 0)
  {
    --guard_notice_timer;
  }
  if (resurrect_time > 0)
  {
    --resurrect_time;
  }

  if (is_feather_fall) is_feather_fall++;

  if (is_feather_fall > 225)
  {
    is_feather_fall = 0;
  }

  // Special event: mouse
  if (current_level == /*8*/ custom->mouse_level && Char.room == /*16*/ custom->mouse_room && leveldoor_open)
  {
    ++leveldoor_open;
    // time before mouse comes: 150/12=12.5 seconds
    if (leveldoor_open == /*150*/ custom->mouse_delay)
    {
      do_mouse();
    }
  }
}

// seg003:0798
void __pascal far check_mirror()
{
  if (jumped_through_mirror == -1)
  {
    jump_through_mirror();
  }
}

// seg003:080A
void __pascal far jump_through_mirror()
{
  jumped_through_mirror = 0;
  Char.charid = charid_1_shadow;
  guardhp_max = guardhp_curr = hitp_max;
  hitp_curr = 1;
}

// seg003:085B
void __pascal far check_mirror_image()
{
  short distance;
  short xpos;
  xpos = x_bump[Char.curr_col + 5] + 10;
  distance = distance_to_edge_weight();
  if (Char.direction >= dir_0_right)
  {
    distance = (~distance) + 14;
  }
  distance_mirror = distance - 2;
  Char.x = (xpos << 1) - Char.x;
  Char.direction = ~Char.direction;
}

// seg003:08AA
void __pascal far bump_into_opponent()
{
  // This is called from play_kid_frame, so char=Kid, Opp=Guard
  short distance;
  if (can_guard_see_kid >= 2 &&
      Char.sword == sword_0_sheathed && // Kid must not be in fighting pose
      Opp.sword != sword_0_sheathed &&  // but Guard must
      Opp.action < 2 &&
      Char.direction != Opp.direction // must be facing toward each other
  )
  {
    distance = char_opp_dist();
    if (ABS(distance) <= 15)
    {
      Char.y = y_land[Char.curr_row + 1];
      Char.fall_y = 0;
      seqtbl_offset_char(seq_47_bump); // bump into opponent
      play_seq();
    }
  }
}

// seg003:0913
void __pascal far pos_guards()
{
  short guard_tile;
  short room1;
  for (room1 = 0; room1 < 24; ++room1)
  {
    guard_tile = level.guards_tile[room1];
    if (guard_tile < 30)
    {
      level.guards_x[room1] = x_bump[guard_tile % 10 + 5] + 14;
      level.guards_seq_hi[room1] = 0;
    }
  }
}

// seg003:0A99
byte __pascal far get_tile_at_kid(int xpos)
{
  return get_tile(Kid.room, get_tile_div_mod_m7(xpos), Kid.curr_row);
}


// seg003:0959
void __pascal far check_can_guard_see_kid()
{
  /*
Possible results in can_guard_see_kid:
0: Guard can't see Kid
1: Guard can see Kid, but won't come
2: Guard can see Kid, and will come
*/
  short kid_frame;
  short left_pos;
  short temp;
  short right_pos;
  kid_frame = Kid.frame;
  if (Guard.charid == charid_24_mouse)
  {
    // If the prince is fighting a guard, and the player does a quickload to a state where the prince is near the mouse, the prince would draw the sword.
    // The following line prevents this.
    can_guard_see_kid = 0;
    return;
  }
  if ((Guard.charid != charid_1_shadow || current_level == 12) &&
      // frames 217..228: going up on stairs
      kid_frame != 0 && (kid_frame < frame_219_exit_stairs_3 || kid_frame >= 229) &&
      Guard.direction != dir_56_none && Kid.alive < 0 && Guard.alive < 0 && Kid.room == Guard.room && Kid.curr_row == Guard.curr_row)
  {
    can_guard_see_kid = 2;
    left_pos = x_bump[Kid.curr_col + 5] + 7;
    right_pos = x_bump[Guard.curr_col + 5] + 7;
    if (left_pos > right_pos)
    {
      temp = left_pos;
      left_pos = right_pos;
      right_pos = temp;
    }
    // A chomper is on the left side of a tile, so it doesn't count.
    if (get_tile_at_kid(left_pos) == tiles_18_chomper)
    {
      left_pos += 14;
    }
    // A gate is on the right side of a tile, so it doesn't count.
    if (get_tile_at_kid(right_pos) == tiles_4_gate
    )
    {
      right_pos -= 14;
    }
    if (right_pos >= left_pos)
    {
      while (left_pos <= right_pos)
      {
        // Can't see through these tiles.
        if (get_tile_at_kid(left_pos) == tiles_20_wall ||
            curr_tile2 == tiles_7_doortop_with_floor ||
            curr_tile2 == tiles_12_doortop)
        {
          can_guard_see_kid = 0;
          return;
        }
        // Can see through these, but won't go through them.
        if (curr_tile2 == tiles_11_loose ||
            curr_tile2 == tiles_18_chomper ||
            (curr_tile2 == tiles_4_gate && curr_room_modif[curr_tilepos] < 112) ||
            !tile_is_floor(curr_tile2))
        {
          can_guard_see_kid = 1;
        }
        left_pos += 14;
      }
    }
  }
  else
  {
    can_guard_see_kid = 0;
  }
}


// seg003:0ABA
void __pascal far do_mouse()
{
  loadkid();
  Char.charid = /*charid_24_mouse*/ custom->mouse_object;
  Char.x = /*200*/ custom->mouse_start_x;
  Char.curr_row = 0;
  Char.y = y_land[Char.curr_row + 1];
  Char.alive = -1;
  Char.direction = dir_FF_left;
  guardhp_curr = 1;
  seqtbl_offset_char(seq_105_mouse_forward); // mouse forward
  play_seq();
  saveshad();
}


// seg006:0006
int __pascal far get_tile(int room, int col, int row)
{
  curr_room = room;
  tile_col = col;
  tile_row = row;
  curr_room = find_room_of_tile();
  // bugfix: check_chomped_kid may call with room = -1
  if (curr_room > 0)
  {
    get_room_address(curr_room);
    curr_tilepos = tbl_line[tile_row] + tile_col;
    curr_tile2 = curr_room_tiles[curr_tilepos] & 0x1F;
  }
  else
  {
    // wall in room 0
    curr_tile2 = custom->level_edge_hit_tile; // tiles_20_wall
  }
  return curr_tile2;
}

// seg006:005D
int __pascal far find_room_of_tile()
{
again:
  if (tile_col < 0)
  {
    tile_col += 10;
    if (curr_room)
    {
      curr_room = level.roomlinks[curr_room - 1].left;
    }
    //find_room_of_tile();
    goto again;
  }
  if (tile_col >= 10)
  {
    tile_col -= 10;
    if (curr_room)
    {
      curr_room = level.roomlinks[curr_room - 1].right;
    }
    //find_room_of_tile();
    goto again;
  }
  // if (tile_row < 0) was here originally
  if (tile_row < 0)
  {
    tile_row += 3;
    if (curr_room)
    {
      curr_room = level.roomlinks[curr_room - 1].up;
    }
    //find_room_of_tile();
    goto again;
  }
  if (tile_row >= 3)
  {
    tile_row -= 3;
    if (curr_room)
    {
      curr_room = level.roomlinks[curr_room - 1].down;
    }
    //find_room_of_tile();
    goto again;
  }
  return curr_room;
}

// seg006:00EC
int __pascal far get_tilepos(int tile_col, int tile_row)
{
  if (tile_row < 0)
  {
    return -(tile_col + 1);
  }
  else if (tile_row >= 3 || tile_col >= 10 || tile_col < 0)
  {
    return 30;
  }
  else
  {
    return tbl_line[tile_row] + tile_col;
  }
}

// seg006:01F5
short __pascal far dx_weight()
{
  sbyte var_2;
  var_2 = cur_frame.dx - (cur_frame.flags & FRAME_WEIGHT_X);
  return char_dx_forward(var_2);
}

// seg006:0124
int __pascal far get_tilepos_nominus(int tile_col, int tile_row)
{
  short var_2;
  var_2 = get_tilepos(tile_col, tile_row);
  if (var_2 < 0)
    return 30;
  else
    return var_2;
}

// seg006:0144
void __pascal far load_fram_det_col()
{
  load_frame();
  determine_col();
}

// seg006:014D
void __pascal far determine_col()
{
  Char.curr_col = get_tile_div_mod_m7(dx_weight());
}


void get_frame_internal(const frame_type frame_table[], int frame, const char *frame_table_name, int count)
{
  if (frame >= 0 && frame < count)
  {
    cur_frame = frame_table[frame];
  }
  else
  {
    printf("Tried to use %s[%d], not in 0..%d\n", frame_table_name, frame, count - 1);
    static const frame_type blank_frame = {255, 0, 0, 0, 0};
    cur_frame = blank_frame;
  }
}

// seg006:015A
void __pascal far load_frame()
{
  short frame;
  short add_frame;
  frame = Char.frame;
  add_frame = 0;
  switch (Char.charid)
  {
  case charid_0_kid:
  case charid_24_mouse:
  use_table_kid:
    get_frame(frame_table_kid, frame);
    break;
  case charid_2_guard:
  case charid_4_skeleton:
    if (frame >= 102 && frame < 107)
      add_frame = 70;
    goto use_table_guard;
  case charid_1_shadow:
    if (frame < 150 || frame >= 190)
      goto use_table_kid;
  use_table_guard:
    get_frame(frame_tbl_guard, frame + add_frame - 149);
    break;
  case charid_5_princess:
  case charid_6_vizier:
    //  use_table_cutscene:
    get_frame(frame_tbl_cuts, frame);
    break;
  }
}

// seg006:0213
int __pascal far char_dx_forward(int delta_x)
{
  if (Char.direction < dir_0_right)
  {
    delta_x = -delta_x;
  }
  return delta_x + Char.x;
}

// seg006:0234
int __pascal far obj_dx_forward(int delta_x)
{
  if (obj_direction < dir_0_right)
  {
    delta_x = -delta_x;
  }
  obj_x += delta_x;
  return obj_x;
}

// seg006:0254
void __pascal far play_seq()
{
  for (;;)
  {
    byte item = *(SEQTBL_0 + Char.curr_seq++);
    switch (item)
    {
    case SEQ_DX: // dx
      Char.x = char_dx_forward(*(SEQTBL_0 + Char.curr_seq++));
      break;
    case SEQ_DY: // dy
      Char.y += *(SEQTBL_0 + Char.curr_seq++);
      break;
    case SEQ_FLIP: // flip
      Char.direction = ~Char.direction;
      break;
    case SEQ_JMP_IF_FEATHER: // jump if feather
      if (!is_feather_fall)
      {
        ++Char.curr_seq;
        ++Char.curr_seq;
        break;
      }
      // fallthrough!
    case SEQ_JMP: // jump
      Char.curr_seq = *(const word *)(SEQTBL_0 + Char.curr_seq);
      break;
    case SEQ_UP: // up
      --Char.curr_row;
      start_chompers();
      break;
    case SEQ_DOWN: // down
      inc_curr_row();
      start_chompers();
      break;
    case SEQ_ACTION: // action
      Char.action = *(SEQTBL_0 + Char.curr_seq++);
      break;
    case SEQ_SET_FALL: // set fall
      Char.fall_x = *(SEQTBL_0 + Char.curr_seq++);
      Char.fall_y = *(SEQTBL_0 + Char.curr_seq++);
      break;
    case SEQ_KNOCK_UP: // knock up
      knock = 1;
      break;
    case SEQ_KNOCK_DOWN: // knock down
      knock = -1;
      break;
    case SEQ_SOUND: // sound
      switch (*(SEQTBL_0 + Char.curr_seq++))
      {
      case SND_SILENT: // no sound actually played, but guards still notice the kid
        is_guard_notice = 1;
        break;
      case SND_FOOTSTEP:               // feet
        is_guard_notice = 1;
        break;
      case SND_BUMP:                // bump
        is_guard_notice = 1;
        break;
      case SND_DRINK:               // drink
        break;
      case SND_LEVEL: // level
        if (is_sound_on)
        {
          if (current_level == 4)
          {
          }
          else if (current_level != 13 && current_level != 15)
          {
          }
        }
        break;
      }
      break;
    case SEQ_END_LEVEL: // end level
      ++next_level;
      break;
    case SEQ_GET_ITEM: // get item
      if (*(SEQTBL_0 + Char.curr_seq++) == 1)
      {
        proc_get_object();
      }
      break;
    case SEQ_DIE: // nop
      break;
    default:
      Char.frame = item;
      //if (Char.frame == 185) Char.frame = 185;
      return;
    }
  }
}

// seg006:03DE
int __pascal far get_tile_div_mod_m7(int xpos)
{
  return get_tile_div_mod(xpos - 7);
}

// seg006:03F0
int __pascal far get_tile_div_mod(int xpos)
{
  // Determine tile column (xh) and the position within the tile (xl) from xpos.

  // DOS PoP does this:
  // obj_xl = tile_mod_tbl[xpos];
  // return tile_div_tbl[xpos];

  // xpos uses a coordinate system in which the left edge of the screen is 58, and each tile is 14 units wide.
  int x = xpos - 58;
  int xl = x % 14;
  int xh = x / 14;
  if (xl < 0)
  {
    // Integer division rounds towards zero, but we want to round down.
    --xh;
    // Modulo returns a negative number if x is negative, but we want 0 <= xl < 14.
    xl += 14;
  }

  // For compatibility with the DOS version, we allow for overflow access to these tables
  // Considering the case of negative overflow
  if (xpos < 0)
  {
    // In this case DOS PoP reads the bytes directly before tile_div_tbl[] and tile_mod_tbl[] in the memory.
    // Here we simulate these reads.
    // Before tile_mod_tbl[] is tile_div_tbl[], and before tile_div_tbl[] are the following bytes:
    static const byte bogus[] = {0x02, 0x00, 0x41, 0x00, 0x80, 0x00, 0xBF, 0x00, 0xFE, 0x00, 0xFF, 0x01, 0x01, 0xFF, 0xC4, 0xFF, 0x03, 0x00, 0x42, 0x00, 0x81, 0x00, 0xC0, 0x00, 0xF8, 0xFF, 0x37, 0x00, 0x76, 0x00, 0xB5, 0x00, 0xF4, 0x00};
    if (COUNT(bogus) + xpos >= 0)
    {
      xh = bogus[COUNT(bogus) + xpos];               // simulating tile_div_tbl[xpos]
      xl = tile_div_tbl[COUNT(tile_div_tbl) + xpos]; // simulating tile_mod_tbl[xpos]
    }
    else
    {
      printf("xpos = %d (< %d) out of range for simulation of index overflow!\n", xpos, -(int)COUNT(bogus));
    }
  }

  // Considering the case of positive overflow
  int tblSize = 256;

  if (xpos >= tblSize)
  {
    // In this case DOS PoP reads the bytes directly after tile_div_tbl[], that is: and tile_mod_tbl[]
    // Here we simulate these reads.
    // After tile_mod_tbl[] there are the following bytes:
    static const byte bogus[] = {0xF4, 0x02, 0x10, 0x1E, 0x2C, 0x3A, 0x48, 0x56, 0x64, 0x72, 0x80, 0x8E, 0x9C, 0xAA, 0xB8, 0xC6, 0xD4, 0xE2, 0xF0, 0xFE, 0x00, 0x0A, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x0D, 0x00, 0x00, 0x00, 0x00};
    if (xpos - tblSize < COUNT(bogus))
    {
      xh = tile_mod_tbl[xpos - tblSize]; // simulating tile_div_tbl[xpos]
      xl = bogus[xpos - tblSize];        // simulating tile_mod_tbl[xpos]
    }
    else
    {
      printf("xpos = %d (> %d) out of range for simulation of index overflow!\n", xpos, (int)COUNT(bogus) + tblSize);
    }
  }

  obj_xl = xl;
  return xh;
}

// seg006:0433
int __pascal far y_to_row_mod4(int ypos)
{
  return (ypos + 60) / 63 % 4 - 1;
}

// seg006:044F
void __pascal far loadkid()
{
  Char = Kid;
}

// seg006:0464
void __pascal far savekid()
{
  Kid = Char;
}

// seg006:0479
void __pascal far loadshad()
{
  Char = Guard;
}

// seg006:048E
void __pascal far saveshad()
{
  Guard = Char;
}

// seg006:04A3
void __pascal far loadkid_and_opp()
{
  loadkid();
  Opp = Guard;
}

// seg006:04BC
void __pascal far savekid_and_opp()
{
  savekid();
  Guard = Opp;
}

// seg006:04D5
void __pascal far loadshad_and_opp()
{
  loadshad();
  Opp = Kid;
}

// seg006:04EE
void __pascal far saveshad_and_opp()
{
  saveshad();
  Kid = Opp;
}

// seg006:0507
void __pascal far reset_obj_clip()
{
  obj_clip_left = 0;
  obj_clip_top = 0;
  obj_clip_right = 320;
  obj_clip_bottom = 192;
}

// seg006:051C
void __pascal far x_to_xh_and_xl(int xpos, sbyte *xh_addr, sbyte *xl_addr)
{
  if (xpos < 0)
  {
    *xh_addr = -((ABS(-xpos) >> 3) + 1);
    *xl_addr = -((-xpos - 1) % 8 - 7);
  }
  else
  {
    *xh_addr = ABS(xpos) >> 3;
    *xl_addr = xpos % 8;
  }
}

// seg006:057C
void __pascal far fall_accel()
{
  if (Char.action == actions_4_in_freefall)
  {
    if (is_feather_fall)
    {
      ++Char.fall_y;
      if (Char.fall_y > 4)
        Char.fall_y = 4;
    }
    else
    {
      Char.fall_y += 3;
      if (Char.fall_y > 33)
        Char.fall_y = 33;
    }
  }
}

// seg006:05AE
void __pascal far fall_speed()
{
  Char.y += Char.fall_y;
  if (Char.action == actions_4_in_freefall)
  {
    Char.x = char_dx_forward(Char.fall_x);
    load_fram_det_col();
  }
}

// seg006:05CD
void __pascal far check_action()
{
  short frame;
  short action;
  action = Char.action;
  frame = Char.frame;
  // frame 109: crouching
  if (action == actions_6_hang_straight ||
      action == actions_5_bumped)
  {
    if (frame == frame_109_crouch
    )
    {
      check_on_floor();
    }
  }
  else if (action == actions_4_in_freefall)
  {
    do_fall();
  }
  else if (action == actions_3_in_midair)
  {
    // frame 102..106: start fall + fall
    if (frame >= frame_102_start_fall_1 && frame < frame_106_fall)
    {
      check_grab();
    }
  }
  else if (action != actions_2_hang_climb)
  {
    check_on_floor();
  }
}

// seg006:0628
int __pascal far tile_is_floor(int tiletype)
{
  switch (tiletype)
  {
  case tiles_0_empty:
  case tiles_9_bigpillar_top:
  case tiles_12_doortop:
  case tiles_20_wall:
  case tiles_26_lattice_down:
  case tiles_27_lattice_small:
  case tiles_28_lattice_left:
  case tiles_29_lattice_right:
    return 0;
  default:
    return 1;
  }
}

// seg006:0658
void __pascal far check_spiked()
{
  short harmful;
  short frame;
  frame = Char.frame;
  if (get_tile(Char.room, Char.curr_col, Char.curr_row) == tiles_2_spike)
  {
    harmful = is_spike_harmful();
    // frames 7..14: running
    // frames 34..39: start run-jump
    // frame 43: land from run-jump
    // frame 26: lang from standing jump
    if (
      (harmful >= 2 && ((frame >= frame_7_run && frame < 15) || (frame >= frame_34_start_run_jump_1 && frame < 40))) ||
      ((frame == frame_43_running_jump_4 || frame == frame_26_standing_jump_11) && harmful != 0))
    {
      spiked();
    }
  }
}

// seg006:06BD
int __pascal far take_hp(int count)
{
  word dead;
  dead = 0;
  if (Char.charid == charid_0_kid)
  {
    if (count >= hitp_curr)
    {
      hitp_delta = -hitp_curr;
      dead = 1;
    }
    else
    {
      hitp_delta = -count;
    }
  }
  else
  {
    if (count >= guardhp_curr)
    {
      guardhp_delta = -guardhp_curr;
      dead = 1;
    }
    else
    {
      guardhp_delta = -count;
    }
  }
  return dead;
}

// seg006:070D
int __pascal far get_tile_at_char()
{
  return get_tile(Char.room, Char.curr_col, Char.curr_row);
}

// Get an image, with index and NULL checks.
image_type *get_image(short chtab_id, int id)
{
  if (chtab_id < 0 || chtab_id > COUNT(chtab_addrs))
  {
    //  printf("Tried to use chtab %d not in 0..%d\n", chtab_id, (int)COUNT(chtab_addrs));
    return NULL;
  }
  chtab_type *chtab = chtab_addrs[chtab_id];
  if (chtab == NULL)
  {
    //  printf("Tried to use null chtab %d\n", chtab_id);
    return NULL;
  }
  if (id < 0 || id >= chtab->n_images)
  {
    //  if (id != 255) printf("Tried to use image %d of chtab %d, not in 0..%d\n", id, chtab_id, chtab->n_images-1);
    return NULL;
  }
  return chtab->images[id];
}


// seg006:0723
void __pascal far set_char_collision()
{
  image_type *image = get_image(obj_chtab, obj_id);
  if (image == NULL)
  {
    char_width_half = 0;
    char_height = 0;
  }
  else
  {
    char_width_half = (image->/*width*/ w + 1) / 2;
    char_height = image->/*height*/ h;
  }
  char_x_left = obj_x / 2 + 58;
  if (Char.direction >= dir_0_right)
  {
    char_x_left -= char_width_half;
  }
  char_x_left_coll = char_x_left;
  char_x_right_coll = char_x_right = char_x_left + char_width_half;
  char_top_y = obj_y - char_height + 1;
  if (char_top_y >= 192)
  {
    char_top_y = 0;
  }
  char_top_row = y_to_row_mod4(char_top_y);
  char_bottom_row = y_to_row_mod4(obj_y);
  if (char_bottom_row == -1)
  {
    char_bottom_row = 3;
  }
  char_col_left = MAX(get_tile_div_mod(char_x_left), 0);
  char_col_right = MIN(get_tile_div_mod(char_x_right), 9);
  if (cur_frame.flags & FRAME_THIN)
  {
    // "thin" this frame for collision detection
    char_x_left_coll += 4;
    char_x_right_coll -= 4;
  }
}

// seg006:0815
void __pascal far check_on_floor()
{
  if (cur_frame.flags & FRAME_NEEDS_FLOOR)
  {
    if (get_tile_at_char() == tiles_20_wall)
    {
      in_wall();
    }
    if (!tile_is_floor(curr_tile2))
    {
      // Special event: floors appear
      if (current_level == 12 &&
          united_with_shadow < 0 &&
          Char.curr_row == 0 &&
          (Char.room == 2 || (Char.room == 13 && tile_col >= 6)))
      {
        curr_room_tiles[curr_tilepos] = tiles_1_floor;
        ++curr_tilepos;
      }
      else
      {
        start_fall();
      }
    }
  }
}

// seg006:08B9
void __pascal far start_fall()
{
  short frame;
  word seq_id;
  frame = Char.frame;
  Char.sword = sword_0_sheathed;
  inc_curr_row();
  start_chompers();
  fall_frame = frame;
  if (frame == frame_9_run)
  {
    // frame 9: run
    seq_id = seq_7_fall; // fall (when?)
  }
  else if (frame == frame_13_run)
  {
    // frame 13: run
    seq_id = seq_19_fall; // fall (when?)
  }
  else if (frame == frame_26_standing_jump_11)
  {
    // frame 26: land after standing jump
    seq_id = seq_18_fall_after_standing_jump; // fall after standing jump
  }
  else if (frame == frame_44_running_jump_5)
  {
    // frame 44: land after running jump
    seq_id = seq_21_fall_after_running_jump; // fall after running jump
  }
  else if (frame >= frame_81_hangdrop_1 && frame < 86)
  {
    // frame 81..85: land after jump up
    seq_id = seq_19_fall; // fall after jumping up
    Char.x = char_dx_forward(5);
    load_fram_det_col();
  }
  else if (frame >= 150 && frame < 180)
  {
    // frame 150..179: with sword + fall + dead
    if (Char.charid == charid_2_guard)
    {
      if (Char.curr_row == 3 && Char.curr_col == 10)
      {
        clear_char();
        return;
      }
      if (Char.fall_x < 0)
      {
        seq_id = seq_82_guard_pushed_off_ledge; // Guard is pushed off the ledge
        if (Char.direction < dir_0_right && distance_to_edge_weight() <= 7)
        {
          Char.x = char_dx_forward(-5);
        }
      }
      else
      {
        droppedout = 0;
        seq_id = seq_83_guard_fall; // fall after forwarding with sword
      }
    }
    else
    {
      droppedout = 1;
      if (Char.direction < dir_0_right && distance_to_edge_weight() <= 7)
      {
        Char.x = char_dx_forward(-5);
      }
      seq_id = seq_81_kid_pushed_off_ledge; // fall after backing with sword / Kid is pushed off the ledge
    }
  }
  else
  {
    seq_id = seq_7_fall; // fall after stand, run, step, crouch
  }
  seqtbl_offset_char(seq_id);
  play_seq();
  load_fram_det_col();
  if (get_tile_at_char() == tiles_20_wall)
  {
    in_wall();
    return;
  }
  int tile = get_tile_infrontof_char();
  if (tile == tiles_20_wall
  )
  {
    if (fall_frame != 44 || distance_to_edge_weight() >= 6)
    {
      Char.x = char_dx_forward(-1);
    }
    else
    {
      seqtbl_offset_char(seq_104_start_fall_in_front_of_wall); // start fall (when?)
      play_seq();
    }
    load_fram_det_col();
  }
}

// seg006:0A19
void __pascal far check_grab()
{
  word old_x;

  #define MAX_GRAB_FALLING_SPEED 32

  if (control_shift < 0 &&                    // press Shift to grab
      Char.fall_y < MAX_GRAB_FALLING_SPEED && // you can't grab if you're falling too fast ...
      Char.alive < 0 &&                       // ... or dead
      (word)y_land[Char.curr_row + 1] <= (word)(Char.y + 25))
  {
    //printf("Falling speed: %d\t x: %d\n", Char.fall_y, Char.x);
    old_x = Char.x;
    Char.x = char_dx_forward(-8);
    load_fram_det_col();
    if (!can_grab_front_above())
    {
      Char.x = old_x;
    }
    else
    {
      Char.x = char_dx_forward(distance_to_edge_weight());
      Char.y = y_land[Char.curr_row + 1];
      Char.fall_y = 0;
      seqtbl_offset_char(seq_15_grab_ledge_midair); // grab a ledge (after falling)
      play_seq();
      grab_timer = 12;
      is_screaming = 0;
    }
  }
}

// seg006:0ABD
int __pascal far can_grab_front_above()
{
  through_tile = get_tile_above_char();
  get_tile_front_above_char();
  return can_grab();
}

// seg006:0ACD
void __pascal far in_wall()
{
  short delta_x;
  delta_x = distance_to_edge_weight();
  if (delta_x >= 8 || get_tile_infrontof_char() == tiles_20_wall)
  {
    delta_x = 6 - delta_x;
  }
  else
  {
    delta_x += 4;
  }
  Char.x = char_dx_forward(delta_x);
  load_fram_det_col();
  get_tile_at_char();
}

// seg006:0B0C
int __pascal far get_tile_infrontof_char()
{
  return get_tile(Char.room, infrontx = dir_front[Char.direction + 1] + Char.curr_col, Char.curr_row);
}

// seg006:0B30
int __pascal far get_tile_infrontof2_char()
{
  short var_2;
  var_2 = dir_front[Char.direction + 1];
  return get_tile(Char.room, infrontx = (var_2 << 1) + Char.curr_col, Char.curr_row);
}

// seg006:0B66
int __pascal far get_tile_behind_char()
{
  return get_tile(Char.room, dir_behind[Char.direction + 1] + Char.curr_col, Char.curr_row);
}

// seg006:0B8A
int __pascal far distance_to_edge_weight()
{
  return distance_to_edge(dx_weight());
}

// seg006:0B94
int __pascal far distance_to_edge(int xpos)
{
  short distance;
  get_tile_div_mod_m7(xpos);
  distance = obj_xl;
  if (Char.direction == dir_0_right)
  {
    distance = 13 - distance;
  }
  return distance;
}

// seg006:0BC4
void __pascal far fell_out()
{
  if (Char.alive < 0 && Char.room == 0)
  {
    take_hp(100);
    Char.alive = 0;
    Char.frame = frame_185_dead; // dead
  }
}

// seg006:0BEE
void __pascal far play_kid()
{
  fell_out();
  control_kid();
  if (Char.alive >= 0 && is_dead())
  {
    if (resurrect_time)
    {
      loadkid();
      hitp_delta = hitp_max;
      seqtbl_offset_char(seq_2_stand); // stand
      Char.x += 8;
      play_seq();
      load_fram_det_col();
      set_start_pos();
    }
    is_show_time = 0;
    ++Char.alive;
  }
}

// seg006:0CD1
void __pascal far control_kid()
{
  if (Char.alive < 0 && hitp_curr == 0)
  {
    Char.alive = 0;
  }
  if (grab_timer != 0)
  {
    --grab_timer;
  }
  rest_ctrl_1();
  do_paused();
  read_user_control();
  user_control();
  save_ctrl_1();
}

// seg006:0D49
void __pascal far do_demo()
{
  if (checkpoint)
  {
    control_shift2 = release_arrows();
    control_forward = control_x = -1;
  }
  else if (Char.sword)
  {
    guard_skill = 10;
    autocontrol_opponent();
    guard_skill = 11;
  }
  else
  {
    do_auto_moves(custom->demo_moves);
  }
}

// seg006:0D85
void __pascal far play_guard()
{
  if (Char.charid == charid_24_mouse)
  {
    autocontrol_opponent();
  }
  else
  {
    if (Char.alive < 0)
    {
      if (guardhp_curr == 0)
      {
        Char.alive = 0;
        on_guard_killed();
      }
      else
      {
        goto loc_7A65;
      }
    }
    if (Char.charid == charid_1_shadow)
    {
      clear_char();
    }
  loc_7A65:
    autocontrol_opponent();
    control();
  }
}

// seg006:0DC0
void __pascal far user_control()
{
  if (Char.direction >= dir_0_right)
  {
    flip_control_x();
    control();
    flip_control_x();
  }
  else
  {
    control();
  }
}

// seg006:0DDC
void __pascal far flip_control_x()
{
  byte temp;
  control_x = -control_x;
  temp = control_forward;
  control_forward = control_backward;
  control_backward = temp;
}

// seg006:0E00
int __pascal far release_arrows()
{
  control_backward = control_forward = control_up = control_down = 0;
  return 1;
}

// seg006:0E12
void __pascal far save_ctrl_1()
{
  ctrl1_forward = control_forward;
  ctrl1_backward = control_backward;
  ctrl1_up = control_up;
  ctrl1_down = control_down;
  ctrl1_shift2 = control_shift2;
}

// seg006:0E31
void __pascal far rest_ctrl_1()
{
  control_forward = ctrl1_forward;
  control_backward = ctrl1_backward;
  control_up = ctrl1_up;
  control_down = ctrl1_down;
  control_shift2 = ctrl1_shift2;
}

// seg006:0E8E
void __pascal far clear_saved_ctrl()
{
  ctrl1_forward = ctrl1_backward = ctrl1_up = ctrl1_down = ctrl1_shift2 = 0;
}

// seg006:0EAF
void __pascal far read_user_control()
{
  if (control_forward >= 0)
  {
    if (control_x < 0)
    {
      if (control_forward == 0)
      {
        control_forward = -1;
      }
    }
    else
    {
      control_forward = 0;
    }
  }
  if (control_backward >= 0)
  {
    if (control_x == 1)
    {
      if (control_backward == 0)
      {
        control_backward = -1;
      }
    }
    else
    {
      control_backward = 0;
    }
  }
  if (control_up >= 0)
  {
    if (control_y < 0)
    {
      if (control_up == 0)
      {
        control_up = -1;
      }
    }
    else
    {
      control_up = 0;
    }
  }
  if (control_down >= 0)
  {
    if (control_y == 1)
    {
      if (control_down == 0)
      {
        control_down = -1;
      }
    }
    else
    {
      control_down = 0;
    }
  }
  if (control_shift2 >= 0)
  {
    if (control_shift < 0)
    {
      if (control_shift2 == 0)
      {
        control_shift2 = -1;
      }
    }
    else
    {
      control_shift2 = 0;
    }
  }
}

// seg006:0F55
int __pascal far can_grab()
{
  // Can char grab curr_tile2 through through_tile?
  byte modifier;
  modifier = curr_room_modif[curr_tilepos];
  // can't grab through wall
  if (through_tile == tiles_20_wall)
    return 0;
  // can't grab through a door top if looking right
  if (through_tile == tiles_12_doortop && Char.direction >= dir_0_right)
    return 0;
  // can't grab through floor
  if (tile_is_floor(through_tile))
    return 0;
  // can't grab a shaking loose floor
  // Allow climbing onto a shaking loose floor if the delay is greater than the default. TODO: This should be a separate option.
  if (curr_tile2 == tiles_11_loose && modifier != 0 && !(custom->loose_floor_delay > 11))
    return 0;
  // a doortop with floor can be grabbed only from the left (looking right)
  if (curr_tile2 == tiles_7_doortop_with_floor && Char.direction < dir_0_right)
    return 0;
  // can't grab something that has no floor
  if (!tile_is_floor(curr_tile2))
    return 0;
  return 1;
}

// seg006:1005
int __pascal far get_tile_above_char()
{
  return get_tile(Char.room, Char.curr_col, Char.curr_row - 1);
}

// seg006:1020
int __pascal far get_tile_behind_above_char()
{
  return get_tile(Char.room, dir_behind[Char.direction + 1] + Char.curr_col, Char.curr_row - 1);
}

// seg006:1049
int __pascal far get_tile_front_above_char()
{
  return get_tile(Char.room, infrontx = dir_front[Char.direction + 1] + Char.curr_col, Char.curr_row - 1);
}

// seg006:1072
int __pascal far back_delta_x(int delta_x)
{
  if (Char.direction < dir_0_right)
  {
    // direction = left
    return delta_x;
  }
  else
  {
    // direction = right
    return -delta_x;
  }
}

// seg006:108A
void __pascal far do_pickup(int obj_type)
{
  pickup_obj_type = obj_type;
  control_shift2 = 1;
  // erase picked up item
  curr_room_tiles[curr_tilepos] = tiles_1_floor;
  curr_room_modif[curr_tilepos] = 0;
  redraw_height = 35;
}

// seg006:10E6
void __pascal far check_press()
{
  short frame;
  short action;
  frame = Char.frame;
  action = Char.action;
  // frames 87..99: hanging
  // frames 135..140: start climb up
  if ((frame >= frame_87_hanging_1 && frame < 100) || (frame >= frame_135_climbing_1 && frame < frame_141_climbing_7))
  {
    // the pressed tile is the one that the char is grabbing
    get_tile_above_char();
  }
  else if (action == actions_7_turn || action == actions_5_bumped || action < actions_2_hang_climb)
  {
    // frame 79: jumping up
    if (frame == frame_79_jumphang && get_tile_above_char() == tiles_11_loose)
    {
      // break a loose floor from above
      make_loose_fall(1);
    }
    else
    {
      // the pressed tile is the one that the char is standing on
      if (!(cur_frame.flags & FRAME_NEEDS_FLOOR))
        return;
      get_tile_at_char();
    }
  }
  else
  {
    return;
  }
  if (curr_tile2 == tiles_15_opener || curr_tile2 == tiles_6_closer)
  {
    if (Char.alive < 0)
    {
      trigger_button(1, 0, -1);
    }
    else
    {
      died_on_button();
    }
  }
  else if (curr_tile2 == tiles_11_loose)
  {
    is_guard_notice = 1;
    make_loose_fall(1);
  }
}

// seg006:1199
void __pascal far check_spike_below()
{
  short not_finished;
  short room;
  short row;
  short col;
  short right_col;
  right_col = get_tile_div_mod_m7(char_x_right);
  if (right_col < 0)
    return;
  row = Char.curr_row;
  room = Char.room;
  for (col = get_tile_div_mod_m7(char_x_left); col <= right_col; ++col)
  {
    row = Char.curr_row;
    do
    {
      not_finished = 0;
      if (get_tile(room, col, row) == tiles_2_spike)
      {
        start_anim_spike(curr_room, curr_tilepos);
      }
      else if (
        !tile_is_floor(curr_tile2) &&
        curr_room != 0 &&
        room == curr_room
      )
      {
        ++row;
        not_finished = 1;
      }
    } while (not_finished);
  }
}

// seg006:1231
void __pascal far clip_char()
{
  short frame;
  short room;
  short action;
  short col;
  short var_A;
  short row;
  short var_E;
  frame = Char.frame;
  action = Char.action;
  room = Char.room;
  row = Char.curr_row;
  reset_obj_clip();
  // frames 217..228: going up the level door
  if (frame >= frame_224_exit_stairs_8 && frame < 229)
  {
    obj_clip_top = leveldoor_ybottom + 1;
    obj_clip_right = leveldoor_right;
  }
  else
  {
    if (
      get_tile(room, char_col_left, char_top_row) == tiles_20_wall ||
      tile_is_floor(curr_tile2))
    {
      // frame 79: jump up, frame 81: grab
      if ((action == actions_0_stand && (frame == frame_79_jumphang || frame == frame_81_hangdrop_1)) ||
          get_tile(room, char_col_right, char_top_row) == tiles_20_wall ||
          tile_is_floor(curr_tile2))
      {
        var_E = row + 1;
        if (var_E == 1 ||
            ((var_A = y_clip[var_E]) < obj_y && var_A - 15 < char_top_y))
        {
          obj_clip_top = char_top_y = y_clip[var_E];
        }
      }
    }
    col = get_tile_div_mod(char_x_left_coll - 4);
    if (get_tile(room, col + 1, row) == tiles_7_doortop_with_floor ||
        curr_tile2 == tiles_12_doortop)
    {
      obj_clip_right = (tile_col << 5) + 32;
    }
    else
    {
      if ((get_tile(room, col, row) != tiles_7_doortop_with_floor &&
           curr_tile2 != tiles_12_doortop) ||
          action == actions_3_in_midair ||
          (action == actions_4_in_freefall && frame == frame_106_fall) ||
          (action == actions_5_bumped && frame == frame_107_fall_land_1) ||
          (Char.direction < dir_0_right && (action == actions_2_hang_climb ||
                                            action == actions_6_hang_straight ||
                                            (action == actions_1_run_jump &&
                                             frame >= frame_137_climbing_3 && frame < frame_140_climbing_6))))
      {
        if (
          (get_tile(room, col = get_tile_div_mod(char_x_right_coll), row) == tiles_20_wall ||
           (curr_tile2 == tiles_13_mirror && Char.direction == dir_0_right)) &&
          (get_tile(room, col, char_top_row) == tiles_20_wall ||
           curr_tile2 == tiles_13_mirror) &&
          room == curr_room)
        {
          obj_clip_right = tile_col << 5;
        }
      }
      else
      {
        obj_clip_right = (tile_col << 5) + 32;
      }
    }
  }
}

// seg006:13E6
void __pascal far stuck_lower()
{
  if (get_tile_at_char() == tiles_5_stuck)
  {
    ++Char.y;
  }
}

// seg006:13F3
void __pascal far set_objtile_at_char()
{
  short char_frame;
  short char_action;
  char_frame = Char.frame;
  char_action = Char.action;
  if (char_action == actions_1_run_jump)
  {
    tile_row = char_bottom_row;
    tile_col = char_col_left;
  }
  else
  {
    tile_row = Char.curr_row;
    tile_col = Char.curr_col;
  }
  // frame 135..148: climbing
  if ((char_frame >= frame_135_climbing_1 && char_frame < 149) ||
      char_action == actions_2_hang_climb ||
      char_action == actions_3_in_midair ||
      char_action == actions_4_in_freefall ||
      char_action == actions_6_hang_straight)
  {
    --tile_col;
  }
  obj_tilepos = get_tilepos_nominus(tile_col, tile_row);
  //printf("set_objtile_at_char: obj_tile = %d\n", obj_tile); // debug
}

// seg006:1463
void __pascal far proc_get_object()
{
  if (Char.charid != charid_0_kid || pickup_obj_type == 0)
    return;
  if (pickup_obj_type == -1)
  {
    have_sword = -1;
    flash_color = color_14_brightyellow;
    flash_time = 8;
  }
  else
  {
    switch (--pickup_obj_type)
    {
    case 0: // health
      if (hitp_curr != hitp_max)
      {
        hitp_delta = 1;
        flash_color = color_4_red;
        flash_time = 2;
      }
      break;
    case 1: // life
      flash_color = color_4_red;
      flash_time = 4;
      add_life();
      break;
    case 2: // feather
      feather_fall();
      break;
    case 3: // invert
      toggle_upside();
      break;
    case 5: // open
      get_tile(8, 0, 0);
      trigger_button(0, 0, -1);
      break;
    case 4: // hurt
      // Special event: blue potions on potions level take half of HP
      if (current_level == 15)
      {
        hitp_delta = -((hitp_max + 1) >> 1);
      }
      else
      {
        hitp_delta = -1;
      }
      break;
    }
  }
}

// seg006:1599
int __pascal far is_dead()
{
  // 177: spiked, 178: chomped, 185: dead
  // or maybe this was a switch-case?
  return Char.frame >= frame_177_spiked && (Char.frame <= frame_178_chomped || Char.frame == frame_185_dead);
}


// seg006:15E8
void __pascal far on_guard_killed()
{
  if (current_level == 0)
  {
    // demo level: after killing Guard, run out of room
    checkpoint = 1;
    demo_index = demo_time = 0;
  }
  else if (current_level == /*13*/ custom->jaffar_victory_level)
  {
    // Jaffar's level: flash
    flash_color = color_15_brightwhite; // white
    flash_time = /*18*/ custom->jaffar_victory_flash_time;
    is_show_time = 1;
    leveldoor_open = 2;
  }
  else if (Char.charid != charid_1_shadow)
  {
  }
}

// seg006:1634
void __pascal far clear_char()
{
  Char.direction = dir_56_none;
  Char.alive = 0;
  Char.action = 0;
  guardhp_curr = 0;
}

// data:42EC
byte obj2_tilepos;
// data:34A6
word obj2_x;
// data:34A8
byte obj2_y;
// data:599E
sbyte obj2_direction;
// data:5948
byte obj2_id;
// data:42BE
byte obj2_chtab;
// data:4D90
short obj2_clip_top;
// data:460C
short obj2_clip_bottom;
// data:4C94
short obj2_clip_left;
// data:4CDE
short obj2_clip_right;

// seg006:1654
void __pascal far save_obj()
{
  obj2_tilepos = obj_tilepos;
  obj2_x = obj_x;
  obj2_y = obj_y;
  obj2_direction = obj_direction;
  obj2_id = obj_id;
  obj2_chtab = obj_chtab;
  obj2_clip_top = obj_clip_top;
  obj2_clip_bottom = obj_clip_bottom;
  obj2_clip_left = obj_clip_left;
  obj2_clip_right = obj_clip_right;
}

// seg006:1691
void __pascal far load_obj()
{
  obj_tilepos = obj2_tilepos;
  obj_x = obj2_x;
  obj_y = obj2_y;
  obj_direction = obj2_direction;
  obj_id = obj2_id;
  obj_chtab = obj2_chtab;
  obj_clip_top = obj2_clip_top;
  obj_clip_bottom = obj2_clip_bottom;
  obj_clip_left = obj2_clip_left;
  obj_clip_right = obj2_clip_right;
}

// seg006:16CE
void __pascal far draw_hurt_splash()
{
  short frame;
  frame = Char.frame;
  if (frame != frame_178_chomped)
  { // chomped
    save_obj();
    obj_tilepos = -1;
    // frame 185: dead
    // frame 106..110: fall + land
    if (frame == frame_185_dead || (frame >= frame_106_fall && frame < 111))
    {
      obj_y += 4;
      obj_dx_forward(5);
    }
    else if (frame == frame_177_spiked)
    { // spiked
      obj_dx_forward(-5);
    }
    else
    {
      obj_y -= ((Char.charid == charid_0_kid) << 2) + 11;
      obj_dx_forward(5);
    }
    if (Char.charid == charid_0_kid)
    {
      obj_chtab = id_chtab_2_kid;
      obj_id = 218; // splash!
    }
    else
    {
      obj_chtab = id_chtab_5_guard;
      obj_id = 1; // splash!
    }
    reset_obj_clip();
    load_obj();
  }
}

// seg006:175D
void __pascal far check_killed_shadow()
{
  // Special event: killed the shadow
  if (current_level == 12)
  {
    if ((Char.charid | Opp.charid) == charid_1_shadow &&
        Char.alive < 0 && Opp.alive >= 0)
    {
      flash_color = color_15_brightwhite; // white
      flash_time = 5;
      take_hp(100);
    }
  }
}

// seg006:1827
void __pascal far control_guard_inactive()
{
  if (Char.frame == frame_166_stand_inactive && control_down < 0)
  {
    if (control_forward < 0)
    {
      draw_sword();
    }
    else
    {
      control_down = 1;
      seqtbl_offset_char(seq_80_stand_flipped); // stand flipped
    }
  }
}

// seg006:1852
int __pascal far char_opp_dist()
{
  // >0 if Opp is in front of char
  // <0 if Opp is behind char
  short distance;
  if (Char.room != Opp.room)
  {
    return 999;
  }
  distance = Opp.x - Char.x;
  if (Char.direction < dir_0_right)
  {
    distance = -distance;
  }
  if (distance >= 0 && Char.direction != Opp.direction)
  {
    distance += 13;
  }
  return distance;
}

// seg006:189B
void __pascal far inc_curr_row()
{
  ++Char.curr_row;
}

// seg004:0004
void __pascal far check_collisions()
{
  short column;
  bump_col_left_of_wall = bump_col_right_of_wall = -1;
  if (Char.action == actions_7_turn)
    return;
  collision_row = Char.curr_row;
  move_coll_to_prev();
  prev_collision_row = collision_row;
  right_checked_col = MIN(get_tile_div_mod_m7(char_x_right_coll) + 2, 11);
  left_checked_col = get_tile_div_mod_m7(char_x_left_coll) - 1;
  get_row_collision_data(collision_row, curr_row_coll_room, curr_row_coll_flags);
  get_row_collision_data(collision_row + 1, below_row_coll_room, below_row_coll_flags);
  get_row_collision_data(collision_row - 1, above_row_coll_room, above_row_coll_flags);
  for (column = 9; column >= 0; --column)
  {
    if (curr_row_coll_room[column] >= 0 &&
        prev_coll_room[column] == curr_row_coll_room[column])
    {
      // char bumps into left of wall
      if (
        (prev_coll_flags[column] & 0x0F) == 0 &&
        (curr_row_coll_flags[column] & 0x0F) != 0)
      {
        bump_col_left_of_wall = column;
      }
      // char bumps into right of wall
      if (
        (prev_coll_flags[column] & 0xF0) == 0 &&
        (curr_row_coll_flags[column] & 0xF0) != 0)
      {
        bump_col_right_of_wall = column;
      }
    }
  }
}

// seg004:00DF
void __pascal far move_coll_to_prev()
{
  sbyte *row_coll_room_ptr;
  byte *row_coll_flags_ptr;
  short column;
  if (collision_row == prev_collision_row ||
      collision_row + 3 == prev_collision_row ||
      collision_row - 3 == prev_collision_row)
  {
    row_coll_room_ptr = curr_row_coll_room;
    row_coll_flags_ptr = curr_row_coll_flags;
  }
  else if (
    collision_row + 1 == prev_collision_row ||
    collision_row - 2 == prev_collision_row)
  {
    row_coll_room_ptr = above_row_coll_room;
    row_coll_flags_ptr = above_row_coll_flags;
  }
  else
  {
    row_coll_room_ptr = below_row_coll_room;
    row_coll_flags_ptr = below_row_coll_flags;
  }
  for (column = 0; column < 10; ++column)
  {
    prev_coll_room[column] = row_coll_room_ptr[column];
    prev_coll_flags[column] = row_coll_flags_ptr[column];
    below_row_coll_room[column] = -1;
    above_row_coll_room[column] = -1;
    curr_row_coll_room[column] = -1;
  }
}

// seg004:0185
void __pascal far get_row_collision_data(short row, sbyte *row_coll_room_ptr, byte *row_coll_flags_ptr)
{
  short right_wall_xpos;
  byte curr_flags;
  short room;
  short column;
  short left_wall_xpos;
  room = Char.room;
  coll_tile_left_xpos = x_bump[left_checked_col + 5] + 7;
  for (column = left_checked_col; column <= right_checked_col; ++column)
  {
    left_wall_xpos = get_left_wall_xpos(room, column, row);
    right_wall_xpos = get_right_wall_xpos(room, column, row);
    // char bumps into left of wall
    curr_flags = (left_wall_xpos < char_x_right_coll) * 0x0F;
    // char bumps into right of wall
    curr_flags |= (right_wall_xpos > char_x_left_coll) * 0xF0;
    row_coll_flags_ptr[tile_col] = curr_flags;
    row_coll_room_ptr[tile_col] = curr_room;
    coll_tile_left_xpos += 14;
  }
}

// seg004:0226
int __pascal far get_left_wall_xpos(int room, int column, int row)
{
  short type;
  type = wall_type(get_tile(room, column, row));
  if (type)
  {
    return wall_dist_from_left[type] + coll_tile_left_xpos;
  }
  else
  {
    return 0xFF;
  }
}

// seg004:025F
int __pascal far get_right_wall_xpos(int room, int column, int row)
{
  short type;
  type = wall_type(get_tile(room, column, row));
  if (type)
  {
    return coll_tile_left_xpos - wall_dist_from_right[type] + 13;
  }
  else
  {
    return 0;
  }
}

// seg004:029D
void __pascal far check_bumped()
{
  if (
    Char.action != actions_2_hang_climb &&
    Char.action != actions_6_hang_straight &&
    // frames 135..149: climb up
    (Char.frame < frame_135_climbing_1 || Char.frame >= 149))
  {
    if (bump_col_left_of_wall >= 0)
    {
      check_bumped_look_right();
    }
    else if (bump_col_right_of_wall >= 0)
    {
      check_bumped_look_left();
    }
  }
}

// seg004:02D2
void __pascal far check_bumped_look_left()
{
  if ((Char.sword == sword_2_drawn || Char.direction < dir_0_right) && // looking left
      is_obstacle_at_col(bump_col_right_of_wall))
  {
    bumped(get_right_wall_xpos(curr_room, tile_col, tile_row) - char_x_left_coll, dir_0_right);
  }
}

// seg004:030A
void __pascal far check_bumped_look_right()
{
  if ((Char.sword == sword_2_drawn || Char.direction == dir_0_right) && // looking right
      is_obstacle_at_col(bump_col_left_of_wall))
  {
    bumped(get_left_wall_xpos(curr_room, tile_col, tile_row) - char_x_right_coll, dir_FF_left);
  }
}

// seg004:0343
int __pascal far is_obstacle_at_col(int tile_col)
{
  short tile_row;
  tile_row = Char.curr_row;
  if (tile_row < 0)
  {
    tile_row += 3;
  }
  if (tile_row >= 3)
  {
    tile_row -= 3;
  }
  get_tile(curr_row_coll_room[tile_col], tile_col, tile_row);
  return is_obstacle();
}

// seg004:037E
int __pascal far is_obstacle()
{
  if (curr_tile2 == tiles_10_potion)
  {
    return 0;
  }
  else if (curr_tile2 == tiles_4_gate)
  {
    if (!can_bump_into_gate())
      return 0;
  }
  else if (curr_tile2 == tiles_18_chomper)
  {
    // is the chomper closed?
    if (curr_room_modif[curr_tilepos] != 2)
      return 0;
  }
  else if (
    curr_tile2 == tiles_13_mirror &&
    Char.charid == charid_0_kid &&
    Char.frame >= frame_39_start_run_jump_6 && Char.frame < frame_44_running_jump_5 && // run-jump
    Char.direction < dir_0_right                                                       // right-to-left only
  )
  {
    curr_room_modif[curr_tilepos] = 0x56; // broken mirror or what?
    jumped_through_mirror = -1;
    return 0;
  }
  coll_tile_left_xpos = xpos_in_drawn_room(x_bump[tile_col + 5]) + 7;
  return 1;
}

// seg004:0405
int __pascal far xpos_in_drawn_room(int xpos)
{
  if (curr_room != drawn_room)
  {
    if (curr_room == room_L || curr_room == room_BL)
    {
      xpos -= 140;
    }
    else if (curr_room == room_R || curr_room == room_BR)
    {
      xpos += 140;
    }
  }
  return xpos;
}

// seg004:0448
void __pascal far bumped(sbyte delta_x, sbyte push_direction)
{
  // frame 177: spiked
  if (Char.alive < 0 && Char.frame != frame_177_spiked)
  {
    Char.x += delta_x;
    if (push_direction < dir_0_right)
    {
      // pushing left
      if (curr_tile2 == tiles_20_wall)
      {
        get_tile(curr_room, --tile_col, tile_row);
      }
    }
    else
    {
      // pushing right
      if (curr_tile2 == tiles_12_doortop ||
          curr_tile2 == tiles_7_doortop_with_floor ||
          curr_tile2 == tiles_20_wall)
      {
        ++tile_col;
        if (curr_room == 0 && tile_col == 10)
        {
          curr_room = Char.room;
          tile_col = 0;
        }
        get_tile(curr_room, tile_col, tile_row);
      }
    }
    if (tile_is_floor(curr_tile2))
    {
      bumped_floor(push_direction);
    }
    else
    {
      bumped_fall();
    }
  }
}

// seg004:0A10
int __pascal far dist_from_wall_forward(byte tiletype)
{
  short type;
  if (tiletype == tiles_4_gate && !can_bump_into_gate())
  {
    return -1;
  }
  else
  {
    coll_tile_left_xpos = x_bump[tile_col + 5] + 7;
    type = wall_type(tiletype);
    if (type == 0)
      return -1;
    if (Char.direction < dir_0_right)
    {
      // looking left
      //return wall_dist_from_right[type] + char_x_left_coll - coll_tile_left_xpos - 13;
      return char_x_left_coll - (coll_tile_left_xpos + 13 - wall_dist_from_right[type]);
    }
    else
    {
      // looking right
      return wall_dist_from_left[type] + coll_tile_left_xpos - char_x_right_coll;
    }
  }
}


// seg004:04E4
void __pascal far bumped_fall()
{
  short action;
  action = Char.action;
  Char.x = char_dx_forward(-4);
  if (action == actions_4_in_freefall)
  {
    Char.fall_x = 0;
  }
  else
  {
    seqtbl_offset_char(seq_45_bumpfall); // fall after bumped
    play_seq();
  }
  bumped_sound();
}

// seg004:0520
void __pascal far bumped_floor(sbyte push_direction)
{
  short frame;
  short seq_index;
  if (Char.sword != sword_2_drawn && (word)(y_land[Char.curr_row + 1] - Char.y) >= (word)15)
  {
    bumped_fall();
  }
  else
  {
    Char.y = y_land[Char.curr_row + 1];
    if (Char.fall_y >= 22)
    {
      Char.x = char_dx_forward(-5);
    }
    else
    {
      Char.fall_y = 0;
      if (Char.alive)
      {
        if (Char.sword == sword_2_drawn)
        {
          if (push_direction == Char.direction)
          {
            seqtbl_offset_char(seq_65_bump_forward_with_sword); // pushed forward with sword (Kid)
            play_seq();
            Char.x = char_dx_forward(1);
            return;
          }
          else
          {
            seq_index = seq_64_pushed_back_with_sword; // pushed back with sword
          }
        }
        else
        {
          frame = Char.frame;
          if (frame == 24 || frame == 25 ||
              (frame >= 40 && frame < 43) ||
              (frame >= frame_102_start_fall_1 && frame < 107))
          {
            seq_index = seq_46_hardbump; // bump into wall after run-jump (crouch)
          }
          else
          {
            seq_index = seq_47_bump; // bump into wall
          }
        }
        seqtbl_offset_char(seq_index);
        play_seq();
        bumped_sound();
      }
    }
  }
}

// seg004:05F1
void __pascal far bumped_sound()
{
  is_guard_notice = 1;
}

// seg004:0601
void __pascal far clear_coll_rooms()
{
  memset_near(prev_coll_room, -1, sizeof(prev_coll_room));
  memset_near(curr_row_coll_room, -1, sizeof(curr_row_coll_room));
  memset_near(below_row_coll_room, -1, sizeof(below_row_coll_room));
  memset_near(above_row_coll_room, -1, sizeof(above_row_coll_room));
  prev_collision_row = -1;
}

// seg004:0657
int __pascal far can_bump_into_gate()
{
  return (curr_room_modif[curr_tilepos] >> 2) + 6 < char_height;
}

// seg004:067C
int __pascal far get_edge_distance()
{
  /*
Possible results in edge_type:
0: closer/sword/potion
1: edge
2: floor (nothing near char)
*/
  short distance;
  byte tiletype;
  determine_col();
  load_frame_to_obj();
  set_char_collision();
  tiletype = get_tile_at_char();
  if (wall_type(tiletype) != 0)
  {
    tile_col = Char.curr_col;
    distance = dist_from_wall_forward(tiletype);
    if (distance >= 0)
    {
    loc_59DD:
      if (distance < 14)
      {
        edge_type = 1;
      }
      else
      {
        edge_type = 2;
        distance = 11;
      }
    }
    else
    {
      goto loc_59E8;
    }
  }
  else
  {
  loc_59E8:
    tiletype = get_tile_infrontof_char();
    if (tiletype == tiles_12_doortop && Char.direction >= dir_0_right)
    {
    loc_59FB:
      edge_type = 0;
      distance = distance_to_edge_weight();
    }
    else
    {
      if (wall_type(tiletype) != 0)
      {
        tile_col = infrontx;
        distance = dist_from_wall_forward(tiletype);
        if (distance >= 0)
          goto loc_59DD;
      }
      if (tiletype == tiles_11_loose)
        goto loc_59FB;
      if (
        tiletype == tiles_6_closer ||
        tiletype == tiles_22_sword ||
        tiletype == tiles_10_potion)
      {
        distance = distance_to_edge_weight();
        if (distance != 0)
        {
          edge_type = 0;
        }
        else
        {
          edge_type = 2;
          distance = 11;
        }
      }
      else
      {
        if (tile_is_floor(tiletype))
        {
          edge_type = 2;
          distance = 11;
        }
        else
        {
          goto loc_59FB;
        }
      }
    }
  }
  curr_tile2 = tiletype;
  return distance;
}

// seg004:076B
void __pascal far check_chomped_kid()
{
  short tile_col;
  short tile_row;
  tile_row = Char.curr_row;
  for (tile_col = 0; tile_col < 10; ++tile_col)
  {
    if (curr_row_coll_flags[tile_col] == 0xFF &&
        get_tile(curr_row_coll_room[tile_col], tile_col, tile_row) == tiles_18_chomper &&
        (curr_room_modif[curr_tilepos] & 0x7F) == 2 // closed chomper
    )
    {
      chomped();
    }
  }
}

// seg004:07BF
void __pascal far chomped()
{
    curr_room_modif[curr_tilepos] |= 0x80; // put blood
  if (Char.frame != frame_178_chomped && Char.room == curr_room)
  {
      Char.x = x_bump[tile_col + 5] + 7;
    Char.x = char_dx_forward(7 - !Char.direction);
    Char.y = y_land[Char.curr_row + 1];
    take_hp(100);
    seqtbl_offset_char(seq_54_chomped); // chomped
    play_seq();
  }
}

// seg004:0A7B
int __pascal far dist_from_wall_behind(byte tiletype)
{
  short type;
  type = wall_type(tiletype);
  if (type == 0)
  {
    return 99;
  }
  else
  {
    if (Char.direction >= dir_0_right)
    {
      // looking right
      //return wall_dist_from_right[type] + char_x_left_coll - coll_tile_left_xpos - 13;
      return char_x_left_coll - (coll_tile_left_xpos + 13 - wall_dist_from_right[type]);
    }
    else
    {
      // looking left
      return wall_dist_from_left[type] + coll_tile_left_xpos - char_x_right_coll;
    }
  }
}


// seg004:0833
void __pascal far check_gate_push()
{
  // Closing gate pushes Kid
  short frame;
  short orig_col;
  frame = Char.frame;
  if (Char.action == actions_7_turn ||
      frame == frame_15_stand ||                      // stand
      (frame >= frame_108_fall_land_2 && frame < 111) // crouch
  )
  {
    get_tile_at_char();
    orig_col = tile_col;
    if ((curr_tile2 == tiles_4_gate ||
         get_tile(curr_room, --tile_col, tile_row) == tiles_4_gate) &&
        (curr_row_coll_flags[tile_col] & prev_coll_flags[tile_col]) == 0xFF &&
        can_bump_into_gate())
    {
      bumped_sound();
      //printf("check_gate_push: orig_col = %d, tile_col = %d, curr_room = %d, Char.room = %d, orig_room = %d\n", orig_col, tile_col, curr_room, Char.room, orig_room);
      // push Kid left if orig_col <= tile_col, gate at char's tile
      // push Kid right if orig_col > tile_col, gate is left from char's tile
      Char.x += 5 - (orig_col <= tile_col) * 10;
    }
  }
}

// seg004:08C3
void __pascal far check_guard_bumped()
{
  if (
    Char.action == actions_1_run_jump &&
    Char.alive < 0 &&
    Char.sword >= sword_2_drawn)
  {
    if (

      get_tile_at_char() == tiles_20_wall ||
      curr_tile2 == tiles_7_doortop_with_floor ||
      (curr_tile2 == tiles_4_gate && can_bump_into_gate()) ||
      (Char.direction >= dir_0_right && (get_tile(curr_room, --tile_col, tile_row) == tiles_7_doortop_with_floor ||
                                         (curr_tile2 == tiles_4_gate && can_bump_into_gate()))))
    {
      load_frame_to_obj();
      set_char_collision();
      if (is_obstacle())
      {
        short delta_x;
        delta_x = dist_from_wall_behind(curr_tile2);
        if (delta_x < 0 && delta_x > -13)
        {
          Char.x = char_dx_forward(-delta_x);
          seqtbl_offset_char(seq_65_bump_forward_with_sword); // pushed to wall with sword (Guard)
          play_seq();
          load_fram_det_col();
        }
      }
    }
  }
}

// seg004:0989
void __pascal far check_chomped_guard()
{
  get_tile_at_char();
  if (!check_chomped_here())
  {
    get_tile(curr_room, ++tile_col, tile_row);
    check_chomped_here();
  }
}

// seg004:09B0
int __pascal far check_chomped_here()
{
  if (curr_tile2 == tiles_18_chomper &&
      (curr_room_modif[curr_tilepos] & 0x7F) == 2)
  {
    coll_tile_left_xpos = x_bump[tile_col + 5] + 7;
    if (get_left_wall_xpos(curr_room, tile_col, tile_row) < char_x_right_coll &&
        get_right_wall_xpos(curr_room, tile_col, tile_row) > char_x_left_coll)
    {
      chomped();
      return 1;
    }
    else
    {
      return 0;
    }
  }
  else
  {
    return 0;
  }
}

// seg007:0000
void __pascal far process_trobs()
{
  word need_delete;
  word index;
  word new_index;
  need_delete = 0;
  if (trobs_count == 0)
    return;
  for (index = 0; index < trobs_count; ++index)
  {
    trob = trobs[index];
    animate_tile();
    trobs[index].type = trob.type;
    if (trob.type < 0)
    {
      need_delete = 1;
    }
  }
  if (need_delete)
  {
    for (index = new_index = 0; index < trobs_count; ++index)
    {
      if (trobs[index].type >= 0)
      {
        trobs[new_index++] = trobs[index];
      }
    }
    trobs_count = new_index;
  }
}


// seg007:00AF
void __pascal far animate_tile()
{
  get_room_address(trob.room);
  switch (get_curr_tile(trob.tilepos))
  {
  case tiles_19_torch:
  case tiles_30_torch_with_debris:
    animate_torch();
    break;
  case tiles_6_closer:
  case tiles_15_opener:
    animate_button();
    break;
  case tiles_2_spike:
    animate_spike();
    break;
  case tiles_11_loose:
    animate_loose();
    break;
  case tiles_0_empty:
    animate_empty();
    break;
  case tiles_18_chomper:
    animate_chomper();
    break;
  case tiles_4_gate:
    animate_door();
    break;
  case tiles_16_level_door_left:
    animate_leveldoor();
    break;
  case tiles_10_potion:
    animate_potion();
    break;
  case tiles_22_sword:
    animate_sword();
    break;
  default:
    trob.type = -1;
    break;
  }
  curr_room_modif[trob.tilepos] = curr_modifier;
}

// seg007:0166
short __pascal far is_trob_in_drawn_room()
{
  if (trob.room != drawn_room)
  {
    trob.type = -1;
    return 0;
  }
  else
  {
    return 1;
  }
}

// seg007:0258
short __pascal far get_trob_pos_in_drawn_room()
{
  short tilepos;
  tilepos = trob.tilepos;
  if (trob.room == room_A)
  {
    if (tilepos >= 20 && tilepos < 30)
    {
      // 20..29 -> -1..-10
      tilepos = 19 - tilepos;
    }
    else
    {
      tilepos = 30;
    }
  }
  else
  {
    if (trob.room != drawn_room)
    {
      tilepos = 30;
    }
  }
  return tilepos;
}

// seg007:029D
short __pascal far get_trob_right_pos_in_drawn_room()
{
  word tilepos;
  tilepos = trob.tilepos;
  if (trob.room == drawn_room)
  {
    if (tilepos % 10 != 9)
    {
      ++tilepos;
    }
    else
    {
      tilepos = 30;
    }
  }
  else if (trob.room == room_L)
  {
    if (tilepos % 10 == 9)
    {
      tilepos -= 9;
    }
    else
    {
      tilepos = 30;
    }
  }
  else if (trob.room == room_A)
  {
    if (tilepos >= 20 && tilepos < 29)
    {
      // 20..28 -> -2..-10
      tilepos = 18 - tilepos;
    }
    else
    {
      tilepos = 30;
    }
  }
  else if (trob.room == room_AL && tilepos == 29)
  {
    tilepos = -1;
  }
  else
  {
    tilepos = 30;
  }
  return tilepos;
}

// seg007:032C
short __pascal far get_trob_right_above_pos_in_drawn_room()
{
  word tilepos;
  tilepos = trob.tilepos;
  if (trob.room == drawn_room)
  {
    if (tilepos % 10 != 9)
    {
      if (tilepos < 10)
      {
        // 0..8 -> -2..-10
        tilepos = -(tilepos + 2);
      }
      else
      {
        tilepos -= 9;
      }
    }
    else
    {
      tilepos = 30;
    }
  }
  else if (trob.room == room_L)
  {
    if (tilepos == 9)
    {
      tilepos = -1;
    }
    else
    {
      if (tilepos % 10 == 9)
      {
        tilepos -= 19;
      }
      else
      {
        tilepos = 30;
      }
    }
  }
  else if (trob.room == room_B)
  {
    if (tilepos < 9)
    {
      tilepos += 21;
    }
    else
    {
      tilepos = 30;
    }
  }
  else if (trob.room == room_BL && tilepos == 9)
  {
    tilepos = 20;
  }
  else
  {
    tilepos = 30;
  }
  return tilepos;
}

// seg007:06CD
short __pascal far get_torch_frame(short curr)
{
  short next;
  next = prandom(255);
  if (next != curr)
  {
    if (next < 9)
    {
      return next;
    }
    else
    {
      next = curr;
    }
  }
  ++next;
  if (next >= 9)
    next = 0;
  return next;
}


// seg007:03CF
void __pascal far animate_torch()
{
  //if (is_trob_in_drawn_room()) {
  // Keep animating torches in the rightmost column of the left-side room as well, because they are visible in the current room.
  if (trob.room == drawn_room || (trob.room == room_L && (trob.tilepos % 10) == 9))
  {
    curr_modifier = get_torch_frame(curr_modifier);
  }
  else
  {
    trob.type = -1;
  }
}

// seg007:06AD
short __pascal far bubble_next_frame(short curr)
{
  short next;
  next = curr + 1;
  if (next >= 8)
    next = 1;
  return next;
}

// seg007:03E9
void __pascal far animate_potion()
{
  word type;
  if (trob.type >= 0 && is_trob_in_drawn_room())
  {
    type = curr_modifier & 0xF8;
    curr_modifier = bubble_next_frame(curr_modifier & 0x07) | type;
  }
}

// seg007:0425
void __pascal far animate_sword()
{
  if (is_trob_in_drawn_room())
  {
    --curr_modifier;
    if (curr_modifier == 0)
    {
      curr_modifier = (prandom(255) & 0x3F) + 0x28;
    }
  }
}

// seg007:0448
void __pascal far animate_chomper()
{
  word blood;
  word frame;
  if (trob.type >= 0)
  {
    blood = curr_modifier & 0x80;
    frame = (curr_modifier & 0x7F) + 1;
    if (frame > /*15*/ custom->chomper_speed)
    {
      frame = 1;
    }
    curr_modifier = blood | frame;
    if (frame == 2)
    {
    }
    // If either:
    // - Kid left this room
    // - Kid left this row
    // - Kid died but not in this chomper
    // and chomper is past frame 6
    // then stop.
    if ((trob.room != drawn_room || trob.tilepos / 10 != Kid.curr_row ||
         (Kid.alive >= 0 && blood == 0)) &&
        (curr_modifier & 0x7F) >= 6)
    {
      trob.type = -1;
    }
  }
}

// seg007:04D3
void __pascal far animate_spike()
{
  if (trob.type >= 0)
  {
    // 0xFF means a disabled spike.
    if (curr_modifier == 0xFF)
      return;
    if (curr_modifier & 0x80)
    {
      --curr_modifier;
      if (curr_modifier & 0x7F)
        return;
      curr_modifier = 6;
    }
    else
    {
      ++curr_modifier;
      if (curr_modifier == 5)
      {
        curr_modifier = 0x8F;
      }
      else if (curr_modifier == 9)
      {
        curr_modifier = 0;
        trob.type = -1;
      }
    }
  }
}

// seg007:0522
void __pascal far animate_door()
{
  /*
Possible values of anim_type:
0: closing
1: open
2: permanent open
3,4,5,6,7,8: fast closing with speeds 20,40,60,80,100,120 /4 pixel/frame
*/
  sbyte anim_type;
  anim_type = trob.type;
  if (anim_type >= 0)
  {
    if (anim_type >= 3)
    {
      // closing fast
      if (anim_type < 8)
      {
        ++anim_type;
        trob.type = anim_type;
      }
      short new_mod = curr_modifier - gate_close_speeds[anim_type];
      curr_modifier = new_mod;
      //if ((sbyte)curr_modifier < 0) {
      if (new_mod < 0)
      {
        //if ((curr_modifier -= gate_close_speeds[anim_type]) < 0) {
        curr_modifier = 0;
        trob.type = -1;
      }
    }
    else
    {
      if (curr_modifier != 0xFF)
      {
        // 0xFF means permanently open.
        curr_modifier += door_delta[anim_type];
        if (anim_type == 0)
        {
          // closing
          if (curr_modifier != 0)
          {
            if (curr_modifier < 188)
            {
              if ((curr_modifier & 3) == 3)
              {
              }
            }
          }
          else
          {
            gate_stop();
          }
        }
        else
        {
          // opening
          if (curr_modifier < 188)
          {
            if ((curr_modifier & 7) == 0)
            {
            }
          }
          else
          {
            // stop
            if (anim_type < 2)
            {
              // after regular open
              curr_modifier = 238;
              trob.type = 0;                 // closing
            }
            else
            {
              // after permanent open
              curr_modifier = 0xFF; // keep open
              gate_stop();
            }
          }
        }
      }
      else
      {
        gate_stop();
      }
    }
  }
}

// seg007:05E3
void __pascal far gate_stop()
{
  trob.type = -1;
}

// seg007:05F1
void __pascal far animate_leveldoor()
{
  /*
Possible values of trob_type:
0: open
1: open (with button)
2: open
3,4,5,6: fast closing with speeds 0,5,17,99 pixel/frame
*/
  word trob_type;
  trob_type = trob.type;
  if (trob.type >= 0)
  {
    if (trob_type >= 3)
    {
      // closing
      ++trob.type;
      curr_modifier -= leveldoor_close_speeds[trob.type - 3];
      if ((sbyte)curr_modifier < 0)
      {
        curr_modifier = 0;
        trob.type = -1;
      }
      else
      {
        if (trob.type == 4)
        {
          sound_interruptible[sound_15_leveldoor_sliding] = 1;
        }
      }
    }
    else
    {
      // opening
      ++curr_modifier;
      if (curr_modifier >= 43)
      {
        trob.type = -1;
        if (leveldoor_open == 0 || leveldoor_open == 2)
        {
          leveldoor_open = 1;
          if (current_level == /*4*/ custom->mirror_level)
          {
            // Special event: place mirror
            get_tile(/*4*/ custom->mirror_room, /*4*/ custom->mirror_column, /*0*/ custom->mirror_row);
            curr_room_tiles[curr_tilepos] = /*tiles_13_mirror*/ custom->mirror_tile;
          }
        }
      }
      else
      {
        sound_interruptible[sound_15_leveldoor_sliding] = 0;
      }
    }
  }
}




// seg007:081E
void __pascal far start_anim_torch(short room, short tilepos)
{
  curr_room_modif[tilepos] = prandom(8);
  add_trob(room, tilepos, 1);
}

// seg007:0847
void __pascal far start_anim_potion(short room, short tilepos)
{
  curr_room_modif[tilepos] &= 0xF8;
  curr_room_modif[tilepos] |= prandom(6) + 1;
  add_trob(room, tilepos, 1);
}

// seg007:087C
void __pascal far start_anim_sword(short room, short tilepos)
{
  curr_room_modif[tilepos] = prandom(0xFF) & 0x1F;
  add_trob(room, tilepos, 1);
}

// seg007:08A7
void __pascal far start_anim_chomper(short room, short tilepos, byte modifier)
{
  short old_modifier;
  old_modifier = curr_room_modif[tilepos];
  if (old_modifier == 0 || old_modifier >= 6)
  {
    curr_room_modif[tilepos] = modifier;
    add_trob(room, tilepos, 1);
  }
}

// seg007:08E3
void __pascal far start_anim_spike(short room, short tilepos)
{
  sbyte old_modifier;
  old_modifier = curr_room_modif[tilepos];
  if (old_modifier <= 0)
  {
    if (old_modifier == 0)
    {
      add_trob(room, tilepos, 1);
    }
    else
    {
      // 0xFF means a disabled spike.
      if (old_modifier != (sbyte)0xFF)
      {
        curr_room_modif[tilepos] = 0x8F;
      }
    }
  }
}

// seg007:092C
short __pascal far trigger_gate(short room, short tilepos, short button_type)
{
  byte modifier;
  modifier = curr_room_modif[tilepos];
  if (button_type == tiles_15_opener)
  {
    // If the gate is permanently open, don't to anything.
    if (modifier == 0xFF)
      return -1;
    if (modifier >= 188)
    {                                 // if it's already open
      curr_room_modif[tilepos] = 238; // keep it open for a while
      return -1;
    }
    curr_room_modif[tilepos] = (modifier + 3) & 0xFC;
    return 1; // regular open
  }
  else if (button_type == tiles_14_debris)
  {
    // If it's not fully open:
    if (modifier < 188)
      return 2;                      // permanent open
    curr_room_modif[tilepos] = 0xFF; // keep open
    return -1;
  }
  else
  {
    if (modifier != 0)
    {
      return 3; // close fast
    }
    else
    {
      // already closed
      return -1;
    }
  }
}

// seg007:0999
short __pascal far trigger_1(short target_type, short room, short tilepos, short button_type)
{
  short result;
  result = -1;
  if (target_type == tiles_4_gate)
  {
    result = trigger_gate(room, tilepos, button_type);
  }
  else if (target_type == tiles_16_level_door_left)
  {
    if (curr_room_modif[tilepos] != 0)
    {
      result = -1;
    }
    else
    {
      result = 1;
    }
  }
  else if (custom->allow_triggering_any_tile)
  { //allow_triggering_any_tile hack
    result = 1;
  }
  return result;
}

// seg007:0BF2
short __pascal far get_doorlink_tile(short index)
{
  return doorlink1_ad[index] & 0x1F;
}

// seg007:0C09
short __pascal far get_doorlink_next(short index)
{
  return !(doorlink1_ad[index] & 0x80);
}

// seg007:0C26
short __pascal far get_doorlink_room(short index)
{
  return ((doorlink1_ad[index] & 0x60) >> 5) +
         ((doorlink2_ad[index] & 0xE0) >> 3);
}

// seg007:09E5
void __pascal far do_trigger_list(short index, short button_type)
{
  word room;
  word tilepos;
  byte target_type;
  sbyte trigger_result;
  // while (doorlink1_ad[index] != -1) { // these can't be equal!
  while (1)
  { // Same as the above but just a little faster and no compiler warning.
    room = get_doorlink_room(index);
    get_room_address(room);
    tilepos = get_doorlink_tile(index);
    target_type = curr_room_tiles[tilepos] & 0x1F;
    trigger_result = trigger_1(target_type, room, tilepos, button_type);
    if (trigger_result >= 0)
    {
      add_trob(room, tilepos, trigger_result);
    }
    if (get_doorlink_next(index++) == 0)
      break;
  }
}

// seg007:0ACA
short __pascal far find_trob()
{
  short index;
  for (index = 0; index < trobs_count; ++index)
  {
    if (trobs[index].tilepos == trob.tilepos &&
        trobs[index].room == trob.room)
      return index;
  }
  return -1;
}

// seg007:0A5A
void __pascal far add_trob(byte room, byte tilepos, sbyte type)
{
  short found;
  trob.room = room;
  trob.tilepos = tilepos;
  trob.type = type;
  found = find_trob();
  if (found == -1)
  {
    // add new
    if (trobs_count == 30)
      return;
    trobs[trobs_count++] = trob;
  }
  else
  {
    // change existing
    trobs[found].type = trob.type;
  }
}



// seg007:0BB6
short __pascal far get_doorlink_timer(short index)
{
  return doorlink2_ad[index] & 0x1F;
}

// seg007:0BCD
short __pascal far set_doorlink_timer(short index, byte value)
{
  doorlink2_ad[index] &= 0xE0;
  doorlink2_ad[index] |= value & 0x1F;
  return doorlink2_ad[index];
}



// seg007:0C53
void __pascal far trigger_button(int playsound, int button_type, int modifier)
{
  sbyte link_timer;
  get_curr_tile(curr_tilepos);
  if (button_type == 0)
  {
    // 0 means currently selected
    button_type = curr_tile;
  }
  if (modifier == -1)
  {
    // -1 means currently selected
    modifier = curr_modifier;
  }
  link_timer = get_doorlink_timer(modifier);
  // is the event jammed?
  if (link_timer != 0x1F)
  {
    set_doorlink_timer(modifier, 5);
    if (link_timer < 2)
    {
      add_trob(curr_room, curr_tilepos, 1);
      is_guard_notice = 1;
    }
    do_trigger_list(modifier, button_type);
  }
}

// seg007:0CD9
void __pascal far died_on_button()
{
  word button_type;
  word modifier;
  button_type = get_curr_tile(curr_tilepos);
  modifier = curr_modifier;
  if (curr_tile == tiles_15_opener)
  {
    curr_room_tiles[curr_tilepos] = tiles_1_floor;
    curr_room_modif[curr_tilepos] = 0;
    button_type = tiles_14_debris; // force permanent open
  }
  else
  {
    curr_room_tiles[curr_tilepos] = tiles_5_stuck;
  }
  trigger_button(1, button_type, modifier);
}

// seg007:0D3A
void __pascal far animate_button()
{
  word var_2;
  if (trob.type >= 0)
  {
    set_doorlink_timer(curr_modifier, var_2 = get_doorlink_timer(curr_modifier) - 1);
    if (var_2 < 2)
    {
      trob.type = -1;
    }
  }
}

// seg007:0D72
void __pascal far start_level_door(short room, short tilepos)
{
  curr_room_modif[tilepos] = 43; // start fully open
  add_trob(room, tilepos, 3);
}

// seg007:0D93
void __pascal far animate_empty()
{
  trob.type = -1;
}

// seg007:0D9D
void __pascal far animate_loose()
{
  word room;
  word row;
  word tilepos;
  short anim_type;
  anim_type = trob.type;
  if (anim_type >= 0)
  {
    ++curr_modifier;
    if (curr_modifier & 0x80)
    {
      // just shaking
      // don't stop on level 13, needed for the auto-falling floors
      if (current_level == 13)
        return;
      if (curr_modifier >= 0x84)
      {
        curr_modifier = 0;
        trob.type = -1;
      }
      loose_shake(!curr_modifier);
    }
    else
    {
      // something is on the floor
      // should it fall already?
      if (curr_modifier >= /*11*/ custom->loose_floor_delay)
      {
        curr_modifier = remove_loose(room = trob.room, tilepos = trob.tilepos);
        trob.type = -1;
        curmob.xh = (tilepos % 10) << 2;
        row = tilepos / 10;
        curmob.y = y_loose_land[row + 1];
        curmob.room = room;
        curmob.speed = 0;
        curmob.type = 0;
        curmob.row = row;
        add_mob();
      }
      else
      {
        loose_shake(0);
      }
    }
  }
}


// seg007:0E55
void __pascal far loose_shake(int arg_0)
{
  word sound_id;
  if (arg_0 || loose_sound[curr_modifier & 0x7F])
  {
    do
    {
      // Sounds 20,21,22: loose floor shaking
      sound_id = prandom(2) + sound_20_loose_shake_1;
    } while (sound_id == last_loose_sound);

    prandom(2); // For vanilla pop compatibility, an RNG cycle is wasted here
                  // Note: In DOS PoP, it's wasted a few lines below.
      last_loose_sound = sound_id;
  }
}

// seg007:0EB8
int __pascal far remove_loose(int room, int tilepos)
{
  curr_room_tiles[tilepos] = tiles_0_empty;
  // note: the level type is used to determine the modifier of the empty space left behind
  return custom->tbl_level_type[current_level];
}

// seg007:0ED5
void __pascal far make_loose_fall(byte modifier)
{
  // is it a "solid" loose floor?
  if ((curr_room_tiles[curr_tilepos] & 0x20) == 0)
  {
    if ((sbyte)curr_room_modif[curr_tilepos] <= 0)
    {
      curr_room_modif[curr_tilepos] = modifier;
      add_trob(curr_room, curr_tilepos, 0);
    }
  }
}


// seg007:0F9A
int __pascal far next_chomper_timing(byte timing)
{
  // 15,12,9,6,13,10,7,14,11,8,repeat
  timing -= 3;
  if (timing < 6)
  {
    timing += 10;
  }
  return timing;
}


// seg007:0F13
void __pascal far start_chompers()
{
  short timing;
  short modifier;
  short tilepos;
  short column;
  timing = 15;
  if ((byte)Char.curr_row < 3)
  {
    get_room_address(Char.room);
    for (column = 0, tilepos = tbl_line[Char.curr_row];
         column < 10;
         ++column, ++tilepos)
    {
      if (get_curr_tile(tilepos) == tiles_18_chomper)
      {
        modifier = curr_modifier & 0x7F;
        if (modifier == 0 || modifier >= 6)
        {
          start_anim_chomper(Char.room, tilepos, timing | (curr_modifier & 0x80));
          timing = next_chomper_timing(timing);
        }
      }
    }
  }
}

// seg007:0FB4
void __pascal far loose_make_shake()
{
  // don't shake on level 13
  if (curr_room_modif[curr_tilepos] == 0 && current_level != 13)
  {
    curr_room_modif[curr_tilepos] = 0x80;
    add_trob(curr_room, curr_tilepos, 1);
  }
}

// seg007:0FE0
void __pascal far do_knock(int room, int tile_row)
{
  short tile_col;
  for (tile_col = 0; tile_col < 10; ++tile_col)
  {
    if (get_tile(room, tile_col, tile_row) == tiles_11_loose)
    {
      loose_make_shake();
    }
  }
}

// seg007:1010
void __pascal far add_mob()
{
  mobs[mobs_count++] = curmob;
}




// seg007:1063
void __pascal far do_mobs()
{
  short n_mobs;
  short index;
  short new_index;
  n_mobs = mobs_count;
  for (curmob_index = 0; n_mobs > curmob_index; ++curmob_index)
  {
    curmob = mobs[curmob_index];
    move_mob();
    check_loose_fall_on_kid();
    mobs[curmob_index] = curmob;
  }
  new_index = 0;
  for (index = 0; index < mobs_count; ++index)
  {
    if (mobs[index].speed != -1)
    {
      mobs[new_index++] = mobs[index];
    }
  }
  mobs_count = new_index;
}

// seg007:110F
void __pascal far move_mob()
{
  if (curmob.type == 0)
  {
    move_loose();
  }
  if (curmob.speed <= 0)
  {
    ++curmob.speed;
  }
}

// seg007:1126
void __pascal far move_loose()
{
  if (curmob.speed < 0)
    return;
  if (curmob.speed < 29)
    curmob.speed += 3;
  curmob.y += curmob.speed;
  if (curmob.room == 0)
  {
    if (curmob.y < 210)
    {
      return;
    }
    else
    {
      curmob.speed = -2;
      return;
    }
  }
  if (curmob.y < 226 && y_something[curmob.row + 1] <= curmob.y)
  {
    // fell into a different row
    curr_tile_temp = get_tile(curmob.room, curmob.xh >> 2, curmob.row);
    if (curr_tile_temp == tiles_11_loose)
    {
      loose_fall();
    }
    if (curr_tile_temp == tiles_0_empty ||
        curr_tile_temp == tiles_11_loose)
    {
      mob_down_a_row();
      return;
    }
    do_knock(curmob.room, curmob.row);
    curmob.y = y_something[curmob.row + 1];
    curmob.speed = -2;
    loose_land();
  }
}

// seg007:11E8
void __pascal far loose_land()
{
  short button_type;
  short tiletype;
  button_type = 0;
  tiletype = get_tile(curmob.room, curmob.xh >> 2, curmob.row);
  switch (tiletype)
  {
  case tiles_15_opener:
    curr_room_tiles[curr_tilepos] = tiles_14_debris;
    button_type = tiles_14_debris;
  // fallthrough!
  case tiles_6_closer:
    trigger_button(1, button_type, -1);
    tiletype = get_tile(curmob.room, curmob.xh >> 2, curmob.row);
  // fallthrough!
  case tiles_1_floor:
  case tiles_2_spike:
  case tiles_10_potion:
  case tiles_19_torch:
  case tiles_30_torch_with_debris:
    if (tiletype == tiles_19_torch ||
        tiletype == tiles_30_torch_with_debris)
    {
      curr_room_tiles[curr_tilepos] = tiles_30_torch_with_debris;
    }
    else
    {
      curr_room_tiles[curr_tilepos] = tiles_14_debris;
    }
  }
}

// seg007:12CB
void __pascal far loose_fall()
{
  curr_room_modif[curr_tilepos] = remove_loose(curr_room, curr_tilepos);
  curmob.speed >>= 1;
  mobs[curmob_index] = curmob;
  curmob.y += 6;
  mob_down_a_row();
  add_mob();
  curmob = mobs[curmob_index];
}

// seg007:1387
void __pascal far mob_down_a_row()
{
  ++curmob.row;
  if (curmob.row >= 3)
  {
    curmob.y -= 192;
    curmob.row = 0;
    curmob.room = level.roomlinks[curmob.room - 1].down;
  }
}


// seg007:14DE
void __pascal far add_mob_to_objtable(int ypos)
{
  word index;
  objtable_type *curr_obj;
  index = objtable_count++;
  curr_obj = &objtable[index];
  curr_obj->obj_type = curmob.type | 0x80;
  curr_obj->xh = curmob.xh;
  curr_obj->xl = 0;
  curr_obj->y = ypos;
  curr_obj->chtab_id = id_chtab_6_environment;
  curr_obj->id = 10;
  curr_obj->clip.top = 0;
  curr_obj->clip.left = 0;
  curr_obj->clip.right = 40;
}

// seg007:1556
int __pascal far is_spike_harmful()
{
  sbyte modifier;
  modifier = curr_room_modif[curr_tilepos];
  if (modifier == 0 || modifier == -1)
  {
    return 0;
  }
  else if (modifier < 0)
  {
    return 1;
  }
  else if (modifier < 5)
  {
    return 2;
  }
  else
  {
    return 0;
  }
}

// seg007:1591
void __pascal far check_loose_fall_on_kid()
{
  loadkid();
  if (Char.room == curmob.room &&
      Char.curr_col == curmob.xh >> 2 &&
      curmob.y < Char.y &&
      Char.y - 30 < curmob.y)
  {
    fell_on_your_head();
    savekid();
  }
}

// seg007:15D3
void __pascal far fell_on_your_head()
{
  short frame;
  short action;
  frame = Char.frame;
  action = Char.action;
  // loose floors hurt you in frames 5..14 (running) only on level 13
  if (
    (current_level == 13 || (frame < frame_5_start_run || frame >= 15)) &&
    (action < actions_2_hang_climb || action == actions_7_turn))
  {
    Char.y = y_land[Char.curr_row + 1];
    if (take_hp(1))
    {
      seqtbl_offset_char(seq_22_crushed); // dead (because of loose floor)
      if (frame == frame_177_spiked)
      { // spiked
        Char.x = char_dx_forward(-12);
      }
    }
    else
    {
      if (frame != frame_109_crouch)
      { // crouching
        if (get_tile_behind_char() == 0)
        {
          Char.x = char_dx_forward(-2);
        }
        seqtbl_offset_char(seq_52_loose_floor_fell_on_kid); // loose floor fell on Kid
      }
    }
  }
}


// seg005:000A
void __pascal far seqtbl_offset_char(short seq_index)
{
  Char.curr_seq = seqtbl_offsets[seq_index];
}

// seg005:001D
void __pascal far seqtbl_offset_opp(int seq_index)
{
  Opp.curr_seq = seqtbl_offsets[seq_index];
}

// seg005:0030
void __pascal far do_fall()
{
  if (is_screaming == 0 && Char.fall_y >= 31)
  {
    is_screaming = 1;
  }
  if ((word)y_land[Char.curr_row + 1] > (word)Char.y)
  {
    check_grab();
  }
  else
  {
    if (get_tile_at_char() == tiles_20_wall)
    {
      in_wall();
    }

    if (tile_is_floor(curr_tile2))
    {
      land();
    }
    else
    {
      inc_curr_row();
    }
  }
}

// seg005:0090
void __pascal far land()
{
  word seq_id;
  is_screaming = 0;
  Char.y = y_land[Char.curr_row + 1];
  if (get_tile_at_char() != tiles_2_spike)
  {
    if (!tile_is_floor(get_tile_infrontof_char()) &&
        distance_to_edge_weight() < 3)  Char.x = char_dx_forward(-3);
    start_chompers();
  }
  else
  {
    // fell on spikes
    goto loc_5EE6;
  }
  if (Char.alive < 0)
  {
    // alive
    if ((distance_to_edge_weight() >= 12 &&
         get_tile_behind_char() == tiles_2_spike) ||
        get_tile_at_char() == tiles_2_spike)
    {
    // fell on spikes
    loc_5EE6:
      if (is_spike_harmful())
      {
        spiked();
        return;
      }
    }
    {
      if (Char.fall_y < 22)
      {
      // fell 1 row
      loc_5EFD:
        if (Char.charid >= charid_2_guard || Char.sword == sword_2_drawn)
        {
          Char.sword = sword_2_drawn;
          seq_id = seq_63_guard_stand_active; // stand active after landing
        }
        else
        {
          seq_id = seq_17_soft_land; // crouch (soft land)
        }
        if (Char.charid == charid_0_kid)
        {
          is_guard_notice = 1;
        }
      }
      else if (Char.fall_y < 33)
      {
        // fell 2 rows
        if (Char.charid == charid_1_shadow)
          goto loc_5EFD;
        if (Char.charid == charid_2_guard)
          goto loc_5F6C;
        // kid (or skeleton (bug!))
        if (!take_hp(1))
        {
          // still alive
          is_guard_notice = 1;
          seq_id = seq_20_medium_land; // medium land (lose 1 HP, crouch)
        }
        else
        {
          // dead (this was the last HP)
          goto loc_5F75;
        }
      }
      else
      {
        // fell 3 or more rows
        goto loc_5F6C;
      }
    }
  }
  else
  {
  // dead
  loc_5F6C:
    take_hp(100);
  loc_5F75:
    seq_id = seq_22_crushed;           // dead (after falling)
  }
  seqtbl_offset_char(seq_id);
  play_seq();
  Char.fall_y = 0;
}

// seg005:01B7
void __pascal far spiked()
{
  // If someone falls into spikes, those spikes become harmless (to others).
  curr_room_modif[curr_tilepos] = 0xFF;
  Char.y = y_land[Char.curr_row + 1];
    Char.x = x_bump[tile_col + 5] + 10;
  Char.x = char_dx_forward(8);
  Char.fall_y = 0;
  take_hp(100);
  seqtbl_offset_char(seq_51_spiked); // spiked
  play_seq();
}

// seg005:0213
void __pascal far control()
{
  short char_frame;
  short char_action;
  char_frame = Char.frame;
  if (Char.alive >= 0)
  {
    if (char_frame == frame_15_stand ||             // stand
        char_frame == frame_166_stand_inactive ||   // stand
        char_frame == frame_158_stand_with_sword || // stand with sword
        char_frame == frame_171_stand_with_sword    // stand with sword
    )
    {
      seqtbl_offset_char(seq_71_dying); // dying (not stabbed)
    }
  }
  else
  {
    char_action = Char.action;
    if (char_action == actions_5_bumped ||
        char_action == actions_4_in_freefall)
    {
      release_arrows();
    }
    else if (Char.sword == sword_2_drawn)
    {
      control_with_sword();
    }
    else if (Char.charid >= charid_2_guard)
    {
      control_guard_inactive();
    }
    else if (char_frame == frame_15_stand ||                  // standing
             (char_frame >= frame_50_turn && char_frame < 53) // end of turning
    )
    {
      control_standing();
    }
    else if (char_frame == frame_48_turn)
    { // a frame in turning
      control_turning();
    }
    else if (char_frame < 4)
    { // start run
      control_startrun();
    }
    else if (char_frame >= frame_67_start_jump_up_1 && char_frame < frame_70_jumphang)
    { // start jump up
      control_jumpup();
    }
    else if (char_frame < 15)
    { // running
      control_running();
    }
    else if (char_frame >= frame_87_hanging_1 && char_frame < 100)
    { // hanging
      control_hanging();
    }
    else if (char_frame == frame_109_crouch)
    { // crouching
      control_crouched();
    }
  }
}

// seg005:02EB
void __pascal far control_crouched()
{
  if (need_level1_music != 0 && current_level == /*1*/ custom->intro_music_level)
  {
     if (need_level1_music == 1)
     {
       need_level1_music = 2;
     }
     else
     {
         need_level1_music = 0;
     }
  }
  else
  {
    need_level1_music = 0;
    if (control_shift2 < 0 && check_get_item())
      return;
    if (control_y != 1)
    {
      seqtbl_offset_char(seq_49_stand_up_from_crouch); // stand up from crouch
    }
    else
    {
      if (control_forward < 0)
      {
        control_forward = 1;                   // disable automatic repeat
        seqtbl_offset_char(seq_79_crouch_hop); // crouch-hop
      }
    }
  }
}

// seg005:0358
void __pascal far control_standing()
{
  short var_2;
  if (control_shift2 < 0 && control_shift < 0 && check_get_item())
  {
    return;
  }
  if (Char.charid != charid_0_kid && control_down < 0 && control_forward < 0)
  {
    draw_sword();
    return;
  } //else
  if (have_sword)
  {
    if (offguard != 0 && control_shift >= 0)
      goto loc_6213;
    if (can_guard_see_kid >= 2)
    {
      var_2 = char_opp_dist();
      if (var_2 >= -10 && var_2 < 90)
      {
        holding_sword = 1;
        if ((word)var_2 < (word)-6)
        {
          if (Opp.charid == charid_1_shadow &&
              (Opp.action == actions_3_in_midair || (Opp.frame >= frame_107_fall_land_1 && Opp.frame < 118)))
          {
            offguard = 0;
          }
          else
          {
            draw_sword();
            return;
          }
        }
        else
        {
          back_pressed();
          return;
        }
      }
    }
    else
    {
      offguard = 0;
    }
  }
  if (control_shift < 0)
  {
    if (control_backward < 0)
    {
      back_pressed();
    }
    else if (control_up < 0)
    {
      up_pressed();
    }
    else if (control_down < 0)
    {
      down_pressed();
    }
    else if (control_x < 0 && control_forward < 0)
    {
      safe_step();
    }
  }
  else
  loc_6213:
    if (control_forward < 0)
    {
      if (is_keyboard_mode && control_up < 0)
      {
        standing_jump();
      }
      else
      {
        forward_pressed();
      }
    }
    else if (control_backward < 0)
    {
      back_pressed();
    }
    else if (control_up < 0)
    {
      if (is_keyboard_mode && control_forward < 0)
      {
        standing_jump();
      }
      else
      {
        up_pressed();
      }
    }
    else if (control_down < 0)
    {
      down_pressed();
    }
    else if (control_x < 0)
    {
      forward_pressed();
    }
}

// seg005:0482
void __pascal far up_pressed()
{
  int leveldoor_tilepos = -1;
  if (get_tile_at_char() == tiles_16_level_door_left)
    leveldoor_tilepos = curr_tilepos;
  else if (get_tile_behind_char() == tiles_16_level_door_left)
    leveldoor_tilepos = curr_tilepos;
  else if (get_tile_infrontof_char() == tiles_16_level_door_left)
    leveldoor_tilepos = curr_tilepos;
  if ((leveldoor_tilepos != -1) && level.start_room != drawn_room && leveldoor_open)
  {
    go_up_leveldoor();
  }
  else
  {
    if (control_x < 0)
    {
      standing_jump();
    }
    else
    {
      check_jump_up();
    }
  }
}

// seg005:04C7
void __pascal far down_pressed()
{
  control_down = 1; // disable automatic repeat
  if (!tile_is_floor(get_tile_infrontof_char()) &&
      distance_to_edge_weight() < 3)
  {
    Char.x = char_dx_forward(5);
    load_fram_det_col();
  }
  else
  {
    if (!tile_is_floor(get_tile_behind_char()) &&
        distance_to_edge_weight() >= 8)
    {
      through_tile = get_tile_behind_char();
      get_tile_at_char();
      if (can_grab() &&
          (Char.direction >= dir_0_right ||
           get_tile_at_char() != tiles_4_gate ||
           curr_room_modif[curr_tilepos] >> 2 >= 6))
      {
        Char.x = char_dx_forward(distance_to_edge_weight() - 9);
        seqtbl_offset_char(seq_68_climb_down); // climb down
      }
      else
      {
        crouch();
      }
    }
    else
    {
      crouch();
    }
  }
}

// seg005:0574
void __pascal far go_up_leveldoor()
{
  Char.x = x_bump[tile_col + 5] + 10;
  Char.direction = dir_FF_left;                   // right
  seqtbl_offset_char(seq_70_go_up_on_level_door); // go up on level door
}

// seg005:058F
void __pascal far control_turning()
{
  if (control_shift >= 0 && control_x < 0 && control_y >= 0)
  {
    seqtbl_offset_char(seq_43_start_run_after_turn); // start run and run (after turning)
  }
}

// seg005:05AD
void __pascal far crouch()
{
  seqtbl_offset_char(seq_50_crouch); // crouch
  control_down = release_arrows();
}

// seg005:05BE
void __pascal far back_pressed()
{
  word seq_id;
  control_backward = release_arrows();
  // After turn, Kid will draw sword if ...
  if (have_sword == 0 ||       // if Kid has sword
      can_guard_see_kid < 2 || // and can see Guard
      char_opp_dist() > 0 ||   // and Guard was behind him
      distance_to_edge_weight() < 2)
  {
    seq_id = seq_5_turn; // turn
  }
  else
  {
    Char.sword = sword_2_drawn;
    offguard = 0;
    seq_id = seq_89_turn_draw_sword; // turn and draw sword
  }
  seqtbl_offset_char(seq_id);
}

// seg005:060F
void __pascal far forward_pressed()
{
  short distance;
  distance = get_edge_distance();

  if (edge_type == 1 && curr_tile2 != tiles_18_chomper && distance < 8)
  {
    // If char is near a wall, step instead of run.
    if (control_forward < 0)
    {
      safe_step();
    }
  }
  else
  {
    seqtbl_offset_char(seq_1_start_run); // start run and run
  }
}

// seg005:0649
void __pascal far control_running()
{
  if (control_x == 0 && (Char.frame == frame_7_run || Char.frame == frame_11_run))
  {
    control_forward = release_arrows();
    seqtbl_offset_char(seq_13_stop_run); // stop run
  }
  else if (control_x > 0)
  {
    control_backward = release_arrows();
    seqtbl_offset_char(seq_6_run_turn); // run-turn
  }
  else if (control_y < 0 && control_up < 0)
  {
    run_jump();
  }
  else if (control_down < 0)
  {
    control_down = 1;                                // disable automatic repeat
    seqtbl_offset_char(seq_26_crouch_while_running); // crouch while running
  }
}

// seg005:06A8
void __pascal far safe_step()
{
  short distance;
  control_shift2 = 1;  // disable automatic repeat
  control_forward = 1; // disable automatic repeat
  distance = get_edge_distance();
  if (distance)
  {
    Char.repeat = 1;
    seqtbl_offset_char(distance + 28); // 29..42: safe step to edge
  }
  else if (edge_type != 1 && Char.repeat != 0)
  {
    Char.repeat = 0;
    seqtbl_offset_char(seq_44_step_on_edge); // step on edge
  }
  else
  {
    seqtbl_offset_char(seq_39_safe_step_11); // unsafe step (off ledge)
  }
}

// seg005:06F0
int __pascal far check_get_item()
{
  if (get_tile_at_char() == tiles_10_potion ||
      curr_tile2 == tiles_22_sword)
  {
    if (!tile_is_floor(get_tile_behind_char()))
    {
      return 0;
    }
    Char.x = char_dx_forward(-14);
    load_fram_det_col();
  }
  if (get_tile_infrontof_char() == tiles_10_potion ||
      curr_tile2 == tiles_22_sword)
  {
    get_item();
    return 1;
  }
  return 0;
}

// seg005:073E
void __pascal far get_item()
{
  short distance;
  if (Char.frame != frame_109_crouch)
  { // crouching
    distance = get_edge_distance();
    if (edge_type != 2)
    {
      Char.x = char_dx_forward(distance);
    }
    if (Char.direction >= dir_0_right)
    {
      Char.x = char_dx_forward((curr_tile2 == tiles_10_potion) - 2);
    }
    crouch();
  }
  else if (curr_tile2 == tiles_22_sword)
  {
    do_pickup(-1);
    seqtbl_offset_char(seq_91_get_sword); // get sword
  }
  else
  { // potion
    do_pickup(curr_room_modif[curr_tilepos] >> 3);
    seqtbl_offset_char(seq_78_drink); // drink
    if (current_level == 15)
    {
      short index;
      for (index = 0; index < 14; ++index)
      {
        // remove letter on potions level
        if (copyprot_room[index] == curr_room &&
            copyprot_tile[index] == curr_tilepos)
        {
          copyprot_room[index] = 0;
          break;
        }
      }
    }
  }
}

// seg005:07FF
void __pascal far control_startrun()
{
  if (control_y < 0 && control_x < 0)
  {
    standing_jump();
  }
}

// seg005:0812
void __pascal far control_jumpup()
{
  if (control_x < 0 || control_forward < 0)
  {
    standing_jump();
  }
}

// seg005:0825
void __pascal far standing_jump()
{
  control_up = control_forward = 1;        // disable automatic repeat
  seqtbl_offset_char(seq_3_standing_jump); // standing jump
}

// seg005:0836
void __pascal far check_jump_up()
{
  control_up = release_arrows();
  through_tile = get_tile_above_char();
  get_tile_front_above_char();
  if (can_grab())
  {
    grab_up_with_floor_behind();
  }
  else
  {
    through_tile = get_tile_behind_above_char();
    get_tile_above_char();
    if (can_grab())
    {
      jump_up_or_grab();
    }
    else
    {
      jump_up();
    }
  }
}

// seg005:087B
void __pascal far jump_up_or_grab()
{
  short distance;
  distance = distance_to_edge_weight();
  if (distance < 6)
  {
    jump_up();
  }
  else if (!tile_is_floor(get_tile_behind_char()))
  {
    // There is not floor behind char.
    grab_up_no_floor_behind();
  }
  else
  {
    // There is floor behind char, go back a bit.
    Char.x = char_dx_forward(distance - 14);
    load_fram_det_col();
    grab_up_with_floor_behind();
  }
}

// seg005:08C7
void __pascal far grab_up_no_floor_behind()
{
  get_tile_above_char();
  Char.x = char_dx_forward(distance_to_edge_weight() - 10);
  seqtbl_offset_char(seq_16_jump_up_and_grab); // jump up and grab (no floor behind)
}

// seg005:08E6
void __pascal far jump_up()
{
  short distance;
  control_up = release_arrows();
  distance = get_edge_distance();
  if (distance < 4 && edge_type == 1)
  {
    Char.x = char_dx_forward(distance - 3);
  }
  get_tile(Char.room, get_tile_div_mod(back_delta_x(0) + dx_weight() - 6), Char.curr_row - 1);
  if (curr_tile2 != tiles_20_wall && !tile_is_floor(curr_tile2))
  {
    seqtbl_offset_char(seq_28_jump_up_with_nothing_above); // jump up with nothing above
  }
  else
  {
    seqtbl_offset_char(seq_14_jump_up_into_ceiling); // jump up with wall or floor above
  }
}

// seg005:0968
void __pascal far control_hanging()
{
  if (Char.alive < 0)
  {
    if (grab_timer == 0 && control_y < 0)
    {
      can_climb_up();
    }
    else if (control_shift < 0)
    {
      // hanging against a wall or a doortop
      if (Char.action != actions_6_hang_straight &&
          (get_tile_at_char() == tiles_20_wall ||
           (Char.direction == dir_FF_left && ( // facing left
                                               curr_tile2 == tiles_7_doortop_with_floor ||
                                               curr_tile2 == tiles_12_doortop))))
      {
        if (grab_timer == 0)
        {
        }
        seqtbl_offset_char(seq_25_hang_against_wall); // hang against wall (straight)
      }
      else
      {
        if (!tile_is_floor(get_tile_above_char()))
        {
          hang_fall();
        }
      }
    }
    else
    {
      hang_fall();
    }
  }
  else
  {
    hang_fall();
  }
}

// seg005:09DF
void __pascal far can_climb_up()
{
  short seq_id;
  seq_id = seq_10_climb_up; // climb up
  control_up = control_shift2 = release_arrows();
  get_tile_above_char();
  if (((curr_tile2 == tiles_13_mirror || curr_tile2 == tiles_18_chomper) &&
       Char.direction == dir_0_right) ||
      (curr_tile2 == tiles_4_gate && Char.direction != dir_0_right &&
       curr_room_modif[curr_tilepos] >> 2 < 6))
  {
    seq_id = seq_73_climb_up_to_closed_gate; // climb up to closed gate and down
  }
  seqtbl_offset_char(seq_id);
}

// seg005:0A46
void __pascal far hang_fall()
{
  control_down = release_arrows();
  if (!tile_is_floor(get_tile_behind_char()) &&
      !tile_is_floor(get_tile_at_char()))
  {
    seqtbl_offset_char(seq_23_release_ledge_and_fall); // release ledge and fall
  }
  else
  {
    if (get_tile_at_char() == tiles_20_wall ||
        (Char.direction < dir_0_right && ( // looking left
                                           curr_tile2 == tiles_7_doortop_with_floor ||
                                           curr_tile2 == tiles_12_doortop)))
    {
      Char.x = char_dx_forward(-7);
    }
    seqtbl_offset_char(seq_11_release_ledge_and_land); // end of climb down
  }
}

// seg005:0AA8
void __pascal far grab_up_with_floor_behind()
{
  short distance;
  distance = distance_to_edge_weight();

  // The global variable edge_type (which we need!) gets set as a side effect of get_edge_distance()
  short edge_distance = get_edge_distance();
  //printf("Distance to edge weight: %d\tedge type: %d\tedge distance: %d\n", distance, edge_type, edge_distance);

  #define JUMP_STRAIGHT_CONDITION distance < 4 && edge_distance < 4 && edge_type != 1

  if (JUMP_STRAIGHT_CONDITION)
  {
    Char.x = char_dx_forward(distance);
    seqtbl_offset_char(seq_8_jump_up_and_grab_straight); // jump up and grab (when?)
  }
  else
  {
    Char.x = char_dx_forward(distance - 4);
    seqtbl_offset_char(seq_24_jump_up_and_grab_forward); // jump up and grab (with floor behind)
  }
}

// seg005:0AF7
void __pascal far run_jump()
{
  short var_2;
  short xpos;
  short col;
  short var_8;
  if (Char.frame >= frame_7_run)
  {
    // Align Kid to edge of floor.
    xpos = char_dx_forward(4);
    col = get_tile_div_mod_m7(xpos);
    for (var_2 = 0; var_2 < 2; ++var_2)
    {
      col += dir_front[Char.direction + 1];
      get_tile(Char.room, col, Char.curr_row);
      if (curr_tile2 == tiles_2_spike || !tile_is_floor(curr_tile2))
      {
        var_8 = distance_to_edge(xpos) + 14 * var_2 - 14;
        if ((word)var_8 < (word)-8 || var_8 >= 2)
        {
          if (var_8 < 128)
            return;
          var_8 = -3;
        }
        Char.x = char_dx_forward(var_8 + 4);
        break;
      }
    }
    control_up = release_arrows();      // disable automatic repeat
    seqtbl_offset_char(seq_4_run_jump); // run-jump
  }
}

// sseg005:0BB5
void __pascal far back_with_sword()
{
  short frame;
  frame = Char.frame;
  if (frame == frame_158_stand_with_sword || frame == frame_170_stand_with_sword || frame == frame_171_stand_with_sword)
  {
    control_backward = 1;                       // disable automatic repeat
    seqtbl_offset_char(seq_57_back_with_sword); // back with sword
  }
}

// seg005:0BE3
void __pascal far forward_with_sword()
{
  short frame;
  frame = Char.frame;
  if (frame == frame_158_stand_with_sword || frame == frame_170_stand_with_sword || frame == frame_171_stand_with_sword)
  {
    control_forward = 1; // disable automatic repeat
    if (Char.charid != charid_0_kid)
    {
      seqtbl_offset_char(seq_56_guard_forward_with_sword); // forward with sword (Guard)
    }
    else
    {
      seqtbl_offset_char(seq_86_forward_with_sword); // forward with sword (Kid)
    }
  }
}

// seg005:0C1D
void __pascal far draw_sword()
{
  word seq_id;
  seq_id = seq_55_draw_sword; // draw sword
  control_forward = control_shift2 = release_arrows();
  if (Char.charid == charid_0_kid)
  {
    offguard = 0;
  }
  else if (Char.charid != charid_1_shadow)
  {
    seq_id = seq_90_en_garde; // stand active
  }
  Char.sword = sword_2_drawn;
  seqtbl_offset_char(seq_id);
}

// seg005:0C67
void __pascal far control_with_sword()
{
  short distance;
  if (Char.action < actions_2_hang_climb)
  {
    if (get_tile_at_char() == tiles_11_loose || can_guard_see_kid >= 2)
    {
      distance = char_opp_dist();
      if ((word)distance < (word)90)
      {
        swordfight();
        return;
      }
      else if (distance < 0)
      {
        if ((word)distance < (word)-4)
        {
          seqtbl_offset_char(seq_60_turn_with_sword); // turn with sword (after switching places)
          return;
        }
        else
        {
          swordfight();
          return;
        }
      }
    } /*else*/
    {
      if (Char.charid == charid_0_kid && Char.alive < 0)
      {
        holding_sword = 0;
      }
      if (Char.charid < charid_2_guard)
      {
        // frame 171: stand with sword
        if (Char.frame == frame_171_stand_with_sword)
        {
          Char.sword = sword_0_sheathed;
          seqtbl_offset_char(seq_92_put_sword_away); // put sword away (Guard died)
        }
      }
      else
      {
        swordfight();
      }
    }
  }
}

// seg005:0CDB
void __pascal far swordfight()
{
  short frame;
  short seq_id;
  short charid;
  frame = Char.frame;
  charid = Char.charid;
  // frame 161: parry
  if (frame == frame_161_parry && control_shift2 >= 0)
  {
    seqtbl_offset_char(seq_57_back_with_sword); // back with sword (when parrying)
    return;
  }
  else if (control_shift2 < 0)
  {
    if (charid == charid_0_kid)
    {
      kid_sword_strike = 15;
    }
    sword_strike();
    if (control_shift2 == 1)
      return;
  }
  if (control_down < 0)
  {
    if (frame == frame_158_stand_with_sword || frame == frame_170_stand_with_sword || frame == frame_171_stand_with_sword)
    {
      control_down = 1; // disable automatic repeat
      Char.sword = sword_0_sheathed;
      if (charid == charid_0_kid)
      {
        offguard = 1;
        guard_refrac = 9;
        holding_sword = 0;
        seq_id = seq_93_put_sword_away_fast; // put sword away fast (down pressed)
      }
      else if (charid == charid_1_shadow)
      {
        seq_id = seq_92_put_sword_away; // put sword away
      }
      else
      {
        seq_id = seq_87_guard_become_inactive; // stand inactive (when Kid leaves sight)
      }
      seqtbl_offset_char(seq_id);
    }
  }
  else if (control_up < 0)
  {
    parry();
  }
  else if (control_forward < 0)
  {
    forward_with_sword();
  }
  else if (control_backward < 0)
  {
    back_with_sword();
  }
}

// seg005:0DB0
void __pascal far sword_strike()
{
  short frame;
  short seq_id;
  frame = Char.frame;
  if (frame == frame_157_walk_with_sword ||  // walk with sword
      frame == frame_158_stand_with_sword || // stand with sword
      frame == frame_170_stand_with_sword || // stand with sword
      frame == frame_171_stand_with_sword || // stand with sword
      frame == frame_165_walk_with_sword     // walk with sword
  )
  {
    if (Char.charid == charid_0_kid)
    {
      seq_id = seq_75_strike; // strike with sword (Kid)
    }
    else
    {
      seq_id = seq_58_guard_strike; // strike with sword (Guard)
    }
  }
  else if (frame == frame_150_parry || frame == frame_161_parry)
  {                                     // parry
    seq_id = seq_66_strike_after_parry; // strike with sword after parrying
  }
  else
  {
    return;
  }
  control_shift2 = 1; // disable automatic repeat
  seqtbl_offset_char(seq_id);
}

// seg005:0E0F
void __pascal far parry()
{
  short opp_frame;
  short char_frame;
  short var_6;
  short seq_id;
  short char_charid;
  char_frame = Char.frame;
  opp_frame = Opp.frame;
  char_charid = Char.charid;
  seq_id = seq_62_parry; // defend (parry) with sword
  var_6 = 0;
  if (
    char_frame == frame_158_stand_with_sword || // stand with sword
    char_frame == frame_170_stand_with_sword || // stand with sword
    char_frame == frame_171_stand_with_sword || // stand with sword
    char_frame == frame_168_back ||             // back?
    char_frame == frame_165_walk_with_sword     // walk with sword
  )
  {
    if (char_opp_dist() >= 32 && char_charid != charid_0_kid)
    {
      back_with_sword();
      return;
    }
    else if (char_charid == charid_0_kid)
    {
      if (opp_frame == frame_168_back)
        return;
      if (opp_frame != frame_151_strike_1 &&
          opp_frame != frame_152_strike_2 &&
          opp_frame != frame_162_block_to_strike)
      {
        if (opp_frame == frame_153_strike_3)
        { // strike
          var_6 = 1;
        }
        else if (char_charid != charid_0_kid)
        {
          back_with_sword();
          return;
        }
      }
    }
    else
    {
      if (opp_frame != frame_152_strike_2)
        return;
    }
  }
  else
  {
    if (char_frame != frame_167_blocked)
      return;
    seq_id = seq_61_parry_after_strike; // parry after striking with sword
  }
  control_up = 1; // disable automatic repeat
  seqtbl_offset_char(seq_id);
  if (var_6)
  {
    play_seq();
  }
}

// seg008:0006
void __pascal far redraw_room()
{
  memset_near(table_counts, 0, sizeof(table_counts));
  reset_obj_clip();
  draw_room();
}

// seg008:0035
void __pascal far load_room_links()
{
  room_BR = 0;
  room_BL = 0;
  room_AR = 0;
  room_AL = 0;
  if (drawn_room)
  {
    get_room_address(drawn_room);
    room_L = level.roomlinks[drawn_room - 1].left;
    room_R = level.roomlinks[drawn_room - 1].right;
    room_A = level.roomlinks[drawn_room - 1].up;
    room_B = level.roomlinks[drawn_room - 1].down;
    if (room_A)
    {
      room_AL = level.roomlinks[room_A - 1].left;
      room_AR = level.roomlinks[room_A - 1].right;
    }
    else
    {
      if (room_L)
      {
        room_AL = level.roomlinks[room_L - 1].up;
      }
      if (room_R)
      {
        room_AR = level.roomlinks[room_R - 1].up;
      }
    }
    if (room_B)
    {
      room_BL = level.roomlinks[room_B - 1].left;
      room_BR = level.roomlinks[room_B - 1].right;
    }
    else
    {
      if (room_L)
      {
        room_BL = level.roomlinks[room_L - 1].down;
      }
      if (room_R)
      {
        room_BR = level.roomlinks[room_R - 1].down;
      }
    }
  }
  else
  {
    room_B = 0;
    room_A = 0;
    room_R = 0;
    room_L = 0;
  }
}

// seg008:0125
void __pascal far draw_room()
{
  word saved_room;
  saved_room = drawn_room;
  drawn_room = room_A;
  load_room_links();
  drawn_row = 2;
  for (drawn_col = 0; drawn_col < 10; ++drawn_col)
  {
    draw_main_y = -1;
    draw_bottom_y = 2;
  }
  drawn_room = saved_room;
  load_room_links();
}


// seg008:1E0C
void __pascal far get_room_address(int room)
{
  //if (room < 0 || room > 24) printf("Tried to access room %d, not in 0..24.\n", room);
  loaded_room = (word)room;
  if (room)
  {
    curr_room_tiles = &level.fg[(room - 1) * 30];
    curr_room_modif = &level.bg[(room - 1) * 30];
  }
}

// seg008:2448
void __pascal far load_frame_to_obj()
{
  word chtab_base;
  chtab_base = id_chtab_2_kid;
  reset_obj_clip();
  load_frame();
  obj_direction = Char.direction;
  obj_id = cur_frame.image;
  // top 6 bits of sword are the chtab
  obj_chtab = chtab_base + (cur_frame.sword >> 6);
  obj_x = (char_dx_forward(cur_frame.dx) << 1) - 116;
  obj_y = cur_frame.dy + Char.y;
  if ((sbyte)(cur_frame.flags ^ obj_direction) >= 0)
  {
    // 0x80: even/odd pixel
    ++obj_x;
  }
}
