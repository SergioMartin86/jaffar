#pragma once

#include "types.h"
#include <string>


  // Function types
typedef void restore_room_after_quick_load_t(void);
typedef void check_mod_param_t(void);
typedef void load_ingame_settings_t(void);
typedef dat_type *__pascal open_dat_t(const char *file, int drive);
typedef void __pascal far parse_cmdline_sound_t(void);
typedef void init_record_replay_t(void);
typedef void init_menu_t(void);
typedef word __pascal far prandom_t(word max);
typedef void far *__pascal load_from_opendats_alloc_t(int resource, const char *extension, data_location *out_result, int *out_size);
typedef void __pascal far set_pal_256_t(rgb_type far *source);
typedef void __pascal far set_pal_t(int index, int red, int green, int blue, int vsync);
typedef chtab_type *__pascal load_sprites_from_file_t(int resource, int palette_bits, int quit_on_error);
typedef void __pascal far close_dat_t(dat_type far *pointer);
typedef void init_lighting_t(void);
typedef void __pascal far hof_read_t(void);
typedef void __pascal far load_kid_sprite_t(void);
typedef void __pascal far load_lev_spr_t(int level);
typedef void __pascal far load_level_t(void);
typedef void __pascal far pos_guards_t(void);
typedef void __pascal far clear_coll_rooms_t(void);
typedef void __pascal far clear_saved_ctrl_t(void);
typedef void __pascal far do_startpos_t(void);
typedef void __pascal far find_start_level_door_t(void);
typedef int __pascal far do_paused_t(void);
typedef void idle_t(void);
typedef void __pascal far stop_sounds_t(void);
typedef void reset_timer_t(int timer_index);
typedef void __pascal far free_peels_t(void);
typedef int __pascal far play_level_2_t(void);
typedef void __pascal far timers_t(void);
typedef void __pascal far start_game_t(void);
typedef void __pascal far play_frame_t(void);
typedef void __pascal far draw_game_frame_t(void);
typedef void __pascal do_simple_wait_t(int timer_index);
typedef void reset_level_unused_fields_t(bool loading_clean_level);
typedef void __pascal far load_room_links_t(void);
typedef void __pascal far draw_level_first_t(void);
typedef void __pascal far play_level_t(int level_number);
typedef int save_recorded_replay_t(const char *full_filename);
typedef void start_recording_t(void);
typedef void add_replay_move_t(void);
typedef void __pascal far process_trobs_t(void);
typedef void __pascal far do_mobs_t(void);
typedef void __pascal far check_skel_t(void);
typedef void __pascal far check_can_guard_see_kid_t(void);
typedef void __pascal far check_mirror_t(void);
typedef void __pascal far init_copyprot_t(void);
typedef void __pascal far alter_mods_allrm_t(void);
typedef void __pascal far start_replay_t(void);
typedef void __pascal far display_text_bottom_t(const char near *text);
typedef void __pascal far redraw_screen_t(int drawing_different_room);

extern "C" __thread word* copyprot_room;
extern "C" __thread byte* sound_interruptible;
extern "C" __thread custom_options_type *custom;
extern "C" __thread byte is_validate_mode;
extern "C" __thread word text_time_remaining;
extern "C" __thread word text_time_total;
extern "C" __thread word is_show_time;
extern "C" __thread word checkpoint;
extern "C" __thread word upside_down;
extern "C" __thread word resurrect_time;
extern "C" __thread word dont_reset_time;
extern "C" __thread short rem_min;
extern "C" __thread word rem_tick;
extern "C" __thread word hitp_beg_lev;
extern "C" __thread word need_level1_music;
extern "C" __thread byte sound_flags;
extern "C" __thread word draw_mode;
extern "C" __thread short start_level;
extern "C" __thread byte *guard_palettes;
extern "C" __thread chtab_type *chtab_addrs[10];
extern "C" __thread word copyprot_plac;
extern "C" __thread word copyprot_idx;
extern "C" __thread word cplevel_entr[14];
extern "C" __thread dialog_type *copyprot_dialog;
extern "C" __thread rect_type dialog_rect_1;
extern "C" __thread rect_type dialog_rect_2;
extern "C" __thread word drawn_room;
extern "C" __thread byte curr_tile;
extern "C" __thread byte curr_modifier;
extern "C" __thread tile_and_mod leftroom_[3];
extern "C" __thread tile_and_mod row_below_left_[10];
extern "C" __thread word loaded_room;
extern "C" __thread byte *curr_room_tiles;
extern "C" __thread byte *curr_room_modif;
extern "C" __thread word draw_xh;
extern "C" __thread word current_level;
extern "C" __thread byte graphics_mode;
extern "C" __thread word room_L;
extern "C" __thread word room_R;
extern "C" __thread word room_A;
extern "C" __thread word room_B;
extern "C" __thread word room_BR;
extern "C" __thread word room_BL;
extern "C" __thread word room_AR;
extern "C" __thread word room_AL;
extern "C" __thread level_type level;
extern "C" __thread short table_counts[5];
extern "C" __thread short drects_count;
extern "C" __thread short peels_count;
extern "C" __thread back_table_type foretable[200];
extern "C" __thread back_table_type backtable[200];
extern "C" __thread midtable_type midtable[50];
extern "C" __thread peel_type *peels_table[50];
extern "C" __thread rect_type drects[30];
extern "C" __thread sbyte obj_direction;
extern "C" __thread short obj_clip_left;
extern "C" __thread short obj_clip_top;
extern "C" __thread short obj_clip_right;
extern "C" __thread short obj_clip_bottom;
extern "C" __thread wipetable_type wipetable[300];
extern "C" __thread word need_drects;
extern "C" __thread word leveldoor_right;
extern "C" __thread word leveldoor_ybottom;
extern "C" __thread byte palace_wall_colors[44 * 3];
extern "C" __thread word seed_was_init;
extern "C" __thread dword random_seed;
extern "C" __thread byte *doorlink2_ad;
extern "C" __thread byte *doorlink1_ad;
extern "C" __thread sbyte control_shift;
extern "C" __thread sbyte control_y;
extern "C" __thread sbyte control_x;
extern "C" __thread char_type Kid;
extern "C" __thread word is_keyboard_mode;
extern "C" __thread word is_paused;
extern "C" __thread word is_restart_level;
extern "C" __thread byte sound_mode;
extern "C" __thread byte is_sound_on;
extern "C" __thread word next_level;
extern "C" __thread short guardhp_delta;
extern "C" __thread word guardhp_curr;
extern "C" __thread word next_room;
extern "C" __thread word hitp_curr;
extern "C" __thread word hitp_max;
extern "C" __thread short hitp_delta;
extern "C" __thread word flash_color;
extern "C" __thread word flash_time;
extern "C" __thread char_type Guard;
extern "C" __thread word need_quotes;
extern "C" __thread short roomleave_result;
extern "C" __thread word different_room;
extern "C" __thread sound_buffer_type *sound_pointers[58];
extern "C" __thread word guardhp_max;
extern "C" __thread word is_feather_fall;
extern "C" __thread chtab_type *chtab_title40;
extern "C" __thread chtab_type *chtab_title50;
extern "C" __thread short hof_count;
extern "C" __thread word demo_mode;
extern "C" __thread short mobs_count;
extern "C" __thread short trobs_count;
extern "C" __thread short next_sound;
extern "C" __thread word grab_timer;
extern "C" __thread short can_guard_see_kid;
extern "C" __thread word holding_sword;
extern "C" __thread short united_with_shadow;
extern "C" __thread word leveldoor_open;
extern "C" __thread word demo_index;
extern "C" __thread short demo_time;
extern "C" __thread word have_sword;
extern "C" __thread char_type Char;
extern "C" __thread char_type Opp;
extern "C" __thread short knock;
extern "C" __thread word is_guard_notice;
extern "C" __thread byte wipe_frames[30];
extern "C" __thread sbyte wipe_heights[30];
extern "C" __thread byte redraw_frames_anim[30];
extern "C" __thread byte redraw_frames2[30];
extern "C" __thread byte redraw_frames_floor_overlay[30];
extern "C" __thread byte redraw_frames_full[30];
extern "C" __thread byte redraw_frames_fore[30];
extern "C" __thread byte tile_object_redraw[30];
extern "C" __thread byte redraw_frames_above[10];
extern "C" __thread word need_full_redraw;
extern "C" __thread short n_curr_objs;
extern "C" __thread objtable_type objtable[50];
extern "C" __thread short curr_objs[50];
extern "C" __thread byte obj_xh;
extern "C" __thread byte obj_xl;
extern "C" __thread byte obj_y;
extern "C" __thread byte obj_chtab;
extern "C" __thread byte obj_id;
extern "C" __thread byte obj_tilepos;
extern "C" __thread short obj_x;
extern "C" __thread frame_type cur_frame;
extern "C" __thread word seamless;
extern "C" __thread trob_type trob;
extern "C" __thread trob_type trobs[30];
extern "C" __thread short redraw_height;
extern "C" __thread byte curr_tilepos;
extern "C" __thread short curr_room;
extern "C" __thread mob_type curmob;
extern "C" __thread mob_type mobs[14];
extern "C" __thread short tile_col;
extern "C" __thread word curr_guard_color;
extern "C" __thread byte key_states[SDL_NUM_SCANCODES];
extern "C" __thread word is_screaming;
extern "C" __thread word offguard; // name from Apple II source
extern "C" __thread word droppedout; // name from Apple II source
extern "C" __thread word exit_room_timer;
extern "C" __thread short char_col_right;
extern "C" __thread short char_col_left;
extern "C" __thread short char_top_row;
extern "C" __thread short prev_char_top_row;
extern "C" __thread short prev_char_col_right;
extern "C" __thread short prev_char_col_left;
extern "C" __thread short char_bottom_row;
extern "C" __thread short guard_notice_timer;
extern "C" __thread short jumped_through_mirror;
extern "C" __thread byte curr_tile2;
extern "C" __thread short tile_row;
extern "C" __thread word char_width_half;
extern "C" __thread word char_height;
extern "C" __thread short char_x_left;
extern "C" __thread short char_x_left_coll;
extern "C" __thread short char_x_right_coll;
extern "C" __thread short char_x_right;
extern "C" __thread short char_top_y;
extern "C" __thread byte fall_frame;
extern "C" __thread byte through_tile;
extern "C" __thread sbyte infrontx; // name from Apple II source
extern "C" __thread word current_sound;
extern "C" __thread sbyte control_shift2;
extern "C" __thread sbyte control_forward;
extern "C" __thread word guard_skill;
extern "C" __thread sbyte control_backward;
extern "C" __thread sbyte control_up;
extern "C" __thread sbyte control_down;
extern "C" __thread sbyte ctrl1_forward;
extern "C" __thread sbyte ctrl1_backward;
extern "C" __thread sbyte ctrl1_up;
extern "C" __thread sbyte ctrl1_down;
extern "C" __thread sbyte ctrl1_shift2;
extern "C" __thread word shadow_initialized;
extern "C" __thread word guard_refrac;
extern "C" __thread word kid_sword_strike;
extern "C" __thread byte edge_type;
extern "C" __thread char **sound_names;
extern "C" __thread int g_argc;
extern "C" __thread sbyte collision_row;
extern "C" __thread sbyte prev_collision_row;
extern "C" __thread sbyte prev_coll_room[10];
extern "C" __thread sbyte curr_row_coll_room[10];
extern "C" __thread sbyte below_row_coll_room[10];
extern "C" __thread sbyte above_row_coll_room[10];
extern "C" __thread byte curr_row_coll_flags[10];
extern "C" __thread byte above_row_coll_flags[10];
extern "C" __thread byte below_row_coll_flags[10];
extern "C" __thread byte prev_coll_flags[10];
extern "C" __thread short pickup_obj_type;
extern "C" __thread word justblocked; // name from Apple II source
extern "C" __thread word last_loose_sound;
extern "C" __thread int last_key_scancode;
extern "C" __thread word curmob_index;
extern "C" __thread dat_type *dathandle;
extern "C" __thread word need_redraw_because_flipped;
extern "C" __thread byte *level_var_palettes;
extern "C" __thread word first_start;
extern "C" __thread sbyte distance_mirror;
extern "C" __thread Uint64 last_transition_counter;
extern "C" __thread sbyte bump_col_left_of_wall;
extern "C" __thread sbyte bump_col_right_of_wall;
extern "C" __thread sbyte right_checked_col;
extern "C" __thread sbyte left_checked_col;
extern "C" __thread short coll_tile_left_xpos;
extern "C" __thread word curr_tile_temp;
extern "C" __thread long int _cachedFilePointerTable[MAX_CACHED_FILES];
extern "C" __thread char *_cachedFileBufferTable[MAX_CACHED_FILES];
extern "C" __thread size_t _cachedFileBufferSizes[MAX_CACHED_FILES];
extern "C" __thread char _cachedFilePathTable[MAX_CACHED_FILES][POP_MAX_PATH];
extern "C" __thread size_t _cachedFileCounter;
extern "C" __thread char exe_dir[POP_MAX_PATH];
extern "C" __thread bool found_exe_dir;
extern "C" __thread word which_quote;
extern "C" __thread dat_type *dat_chain_ptr;
extern "C" __thread char last_text_input;
extern "C" __thread short drawn_row;
extern "C" __thread short draw_bottom_y;
extern "C" __thread short draw_main_y;
extern "C" __thread short drawn_col;
extern "C" __thread byte tile_left;
extern "C" __thread byte modifier_left;

extern "C" restore_room_after_quick_load_t restore_room_after_quick_load;
extern "C" load_ingame_settings_t load_ingame_settings;
extern "C" open_dat_t open_dat;
extern "C" parse_cmdline_sound_t parse_cmdline_sound;
extern "C" init_record_replay_t init_record_replay;
extern "C" init_menu_t init_menu;
extern "C" prandom_t prandom;
extern "C" load_from_opendats_alloc_t load_from_opendats_alloc;
extern "C" set_pal_256_t set_pal_256;
extern "C" load_sprites_from_file_t load_sprites_from_file;
extern "C" close_dat_t close_dat;
extern "C" init_lighting_t init_lighting;
extern "C" hof_read_t hof_read;
extern "C" load_kid_sprite_t load_kid_sprite;
extern "C" load_lev_spr_t load_lev_spr;
extern "C" load_level_t load_level;
extern "C" pos_guards_t pos_guards;
extern "C" clear_coll_rooms_t clear_coll_rooms;
extern "C" clear_saved_ctrl_t clear_saved_ctrl;
extern "C" do_startpos_t do_startpos;
extern "C" find_start_level_door_t find_start_level_door;
extern "C" do_paused_t do_paused;
extern "C" idle_t idle;
extern "C" free_peels_t free_peels;
extern "C" play_level_2_t play_level_2;
extern "C" timers_t timers;
extern "C" play_frame_t play_frame;
extern "C" draw_game_frame_t draw_game_frame;
extern "C" do_simple_wait_t do_simple_wait;
extern "C" reset_level_unused_fields_t reset_level_unused_fields;
extern "C" load_room_links_t load_room_links;
extern "C" draw_level_first_t draw_level_first;
extern "C" play_level_t play_level;
extern "C" save_recorded_replay_t save_recorded_replay;
extern "C" start_recording_t start_recording;
extern "C" add_replay_move_t add_replay_move;
extern "C" process_trobs_t process_trobs;
extern "C" do_mobs_t do_mobs;
extern "C" check_skel_t check_skel;
extern "C" check_can_guard_see_kid_t check_can_guard_see_kid;
extern "C" check_mirror_t check_mirror;
extern "C" init_copyprot_t init_copyprot;
extern "C" alter_mods_allrm_t alter_mods_allrm;
extern "C" start_replay_t start_replay;
extern "C" start_game_t start_game;
extern "C" display_text_bottom_t display_text_bottom;
extern "C" redraw_screen_t redraw_screen;


class SDLPopInstance
{
  public:
  SDLPopInstance(const char* libraryFile, const bool multipleLibraries);
  ~SDLPopInstance();

  // Initializes the sdlPop instance
  void initialize(const bool useGUI);

  // Starts a given level
  void startLevel(const word level);

  // Set seed
  void setSeed(const dword randomSeed);

  // Draw a single frame
  void draw(ssize_t mins = -1, ssize_t secs = -1, ssize_t ms = -1);

  // Perform a single move
  void performMove(const std::string &move);

  // Advance a frame
  void advanceFrame();

  // Print information about the current frame
  void printFrameInfo();

  // Function to transfer cache file contents to reduce pressure on I/O
  std::string serializeFileCache();
  void deserializeFileCache(const std::string& cache);

  bool isExitDoorOpen;

  // Check if exit door is open
  bool isLevelExitDoorOpen();

  // Storing previously drawn room
  word _prevDrawnRoom;

  // Functions to advance/reverse RNG state
  unsigned int advanceRNGState(const unsigned int randomSeed);
  unsigned int reverseRNGState(const unsigned int randomSeed);

  // IGT Timing functions
  size_t getElapsedMins();
  size_t getElapsedSecs();
  size_t getElapsedMilisecs();
};
