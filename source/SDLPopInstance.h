#pragma once

#include "config.h"
#include "types.h"
#include <string>

// Function types
typedef void (*restore_room_after_quick_load_t)(void);
typedef void (*load_global_options_t)(void);
typedef void (*check_mod_param_t)(void);
typedef void (*load_ingame_settings_t)(void);
typedef void (*turn_sound_on_off_t)(byte);
typedef void (*load_mod_options_t)(void);
typedef void (*apply_seqtbl_patches_t)(void);
typedef dat_type *(*__pascal open_dat_t)(const char *file, int drive);
typedef int (*__pascal far parse_grmode_t)(void);
typedef void (*__pascal far init_timer_t)(int frequency);
typedef void (*__pascal far parse_cmdline_sound_t)(void);
typedef void (*__pascal far set_hc_pal_t)(void);
typedef surface_type far *(*__pascal rect_sthg_t)(surface_type *surface, const rect_type far *rect);
typedef void (*__pascal far show_loading_t)(void);
typedef int (*__pascal far set_joy_mode_t)(void);
typedef void (*__pascal far init_copyprot_dialog_t)(void);
typedef void (*init_record_replay_t)(void);
typedef void (*init_menu_t)(void);
typedef word (*__pascal far prandom_t)(word max);
typedef void far *(*__pascal load_from_opendats_alloc_t)(int resource, const char *extension, data_location *out_result, int *out_size);
typedef void (*__pascal far set_pal_256_t)(rgb_type far *source);
typedef void (*__pascal far set_pal_t)(int index, int red, int green, int blue, int vsync);
typedef chtab_type *(*__pascal load_sprites_from_file_t)(int resource, int palette_bits, int quit_on_error);
typedef void (*__pascal far close_dat_t)(dat_type far *pointer);
typedef void (*init_lighting_t)(void);
typedef void (*load_all_sounds_t)(void);
typedef void (*__pascal far hof_read_t)(void);
typedef void (*__pascal far release_title_images_t)(void);
typedef void (*__pascal far free_optsnd_chtab_t)(void);
typedef surface_type far *(*__pascal make_offscreen_buffer_t)(const rect_type far *rect);
typedef void (*__pascal far load_kid_sprite_t)(void);
typedef void (*__pascal far load_lev_spr_t)(int level);
typedef void (*__pascal far load_level_t)(void);
typedef void (*__pascal far pos_guards_t)(void);
typedef void (*__pascal far clear_coll_rooms_t)(void);
typedef void (*__pascal far clear_saved_ctrl_t)(void);
typedef void (*__pascal far do_startpos_t)(void);
typedef void (*__pascal far find_start_level_door_t)(void);
typedef int (*__pascal far check_sound_playing_t)(void);
typedef int (*__pascal far do_paused_t)(void);
typedef void (*idle_t)(void);
typedef void (*__pascal far stop_sounds_t)(void);
typedef void (*__pascal far show_copyprot_t)(int where);
typedef void (*reset_timer_t)(int timer_index);
typedef void (*__pascal far free_peels_t)(void);
typedef int (*__pascal far play_level_2_t)(void);
typedef void (*__pascal far timers_t)(void);
typedef void (*__pascal far start_game_t)(void);
typedef void (*__pascal far play_frame_t)(void);
typedef void (*__pascal far draw_game_frame_t)(void);
typedef void (*update_screen_t)(void);
typedef void (*__pascal do_simple_wait_t)(int timer_index);
typedef void (*reset_level_unused_fields_t)(bool loading_clean_level);
typedef void (*__pascal far load_room_links_t)(void);
typedef void (*set_timer_length_t)(int timer_index, int length);
typedef void (*__pascal far draw_level_first_t)(void);
typedef void (*__pascal far play_level_t)(int level_number);
typedef int (*save_recorded_replay_t)(const char *full_filename);
typedef void (*start_recording_t)(void);
typedef void (*add_replay_move_t)(void);
typedef void (*__pascal far process_trobs_t)(void);
typedef void (*__pascal far do_mobs_t)(void);
typedef void (*__pascal far check_skel_t)(void);
typedef void (*__pascal far check_can_guard_see_kid_t)(void);
typedef void (*__pascal far check_mirror_t)(void);
typedef void (*__pascal far init_copyprot_t)(void);
typedef void (*__pascal far alter_mods_allrm_t)(void);
typedef void (*__pascal far start_replay_t)(void);
typedef void (*__pascal far display_text_bottom_t)(const char near *text);
typedef void (*__pascal far redraw_screen_t)(int drawing_different_room);

typedef chtab_type *chtab_addrs_t[10];
typedef mob_type mobs_t[14];
typedef trob_type trobs_t[30];
typedef sbyte curr_row_coll_room_t[10];
typedef sbyte below_row_coll_room_t[10];
typedef sbyte above_row_coll_room_t[10];
typedef byte curr_row_coll_flags_t[10];
typedef byte below_row_coll_flags_t[10];
typedef byte above_row_coll_flags_t[10];
typedef char exe_dir_t[256];
typedef byte key_states_t[SDL_NUM_SCANCODES];
typedef long int cachedFilePointerTable_t[MAX_CACHED_FILES];
typedef char* cachedFileBufferTable_t[MAX_CACHED_FILES];
typedef size_t cachedFileBufferSizes_t[MAX_CACHED_FILES];
typedef char cachedFilePathTable_t[MAX_CACHED_FILES][POP_MAX_PATH];
typedef size_t cachedFileCounter_t;



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
  void draw();

  // Perform a single move
  void performMove(const std::string &move);

  // Advance a frame
  void advanceFrame();

  // Print information about the current frame
  void printFrameInfo();

  // Function to transfer cache file contents to reduce pressure on I/O
  std::string serializeFileCache();
  void deserializeFileCache(const std::string& cache);

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

  // SDLPop Functions
  restore_room_after_quick_load_t restore_room_after_quick_load;
  load_global_options_t load_global_options;
  check_mod_param_t check_mod_param;
  load_ingame_settings_t load_ingame_settings;
  turn_sound_on_off_t turn_sound_on_off;
  load_mod_options_t load_mod_options;
  apply_seqtbl_patches_t apply_seqtbl_patches;
  open_dat_t open_dat;
  parse_grmode_t parse_grmode;
  init_timer_t init_timer;
  parse_cmdline_sound_t parse_cmdline_sound;
  set_hc_pal_t set_hc_pal;
  rect_sthg_t rect_sthg;
  show_loading_t show_loading;
  set_joy_mode_t set_joy_mode;
  init_copyprot_dialog_t init_copyprot_dialog;
  init_record_replay_t init_record_replay;
  init_menu_t init_menu;
  prandom_t prandom;
  load_from_opendats_alloc_t load_from_opendats_alloc;
  set_pal_256_t set_pal_256;
  set_pal_t set_pal;
  load_sprites_from_file_t load_sprites_from_file;
  close_dat_t close_dat;
  init_lighting_t init_lighting;
  load_all_sounds_t load_all_sounds;
  hof_read_t hof_read;
  release_title_images_t release_title_images;
  free_optsnd_chtab_t free_optsnd_chtab;
  make_offscreen_buffer_t make_offscreen_buffer;
  load_kid_sprite_t load_kid_sprite;
  load_lev_spr_t load_lev_spr;
  load_level_t load_level;
  pos_guards_t pos_guards;
  clear_coll_rooms_t clear_coll_rooms;
  clear_saved_ctrl_t clear_saved_ctrl;
  do_startpos_t do_startpos;
  find_start_level_door_t find_start_level_door;
  check_sound_playing_t check_sound_playing;
  do_paused_t do_paused;
  idle_t idle;
  stop_sounds_t stop_sounds;
  show_copyprot_t show_copyprot;
  reset_timer_t reset_timer;
  free_peels_t free_peels;
  play_level_2_t play_level_2;
  timers_t timers;
  play_frame_t play_frame;
  draw_game_frame_t draw_game_frame;
  update_screen_t update_screen;
  do_simple_wait_t do_simple_wait;
  reset_level_unused_fields_t reset_level_unused_fields;
  load_room_links_t load_room_links;
  set_timer_length_t set_timer_length;
  draw_level_first_t draw_level_first;
  play_level_t play_level;
  save_recorded_replay_t save_recorded_replay;
  start_recording_t start_recording;
  add_replay_move_t add_replay_move;
  process_trobs_t process_trobs;
  do_mobs_t do_mobs;
  check_skel_t check_skel;
  check_can_guard_see_kid_t check_can_guard_see_kid;
  check_mirror_t check_mirror;
  init_copyprot_t init_copyprot;
  alter_mods_allrm_t alter_mods_allrm;
  start_replay_t start_replay;
  start_game_t start_game;
  display_text_bottom_t display_text_bottom;
  redraw_screen_t redraw_screen;

  // SDLPop State variables
  char_type *Kid;     //
  char_type *Guard;   //
  char_type *Char;    //
  char_type *Opp;     //
  short *trobs_count; //
  trobs_t *trobs;     //
  short *mobs_count;  //
  mobs_t *mobs;       //
  level_type *level;  //
  word *drawn_room;
  word *leveldoor_open;
  word *hitp_curr;
  word *guardhp_curr;
  word *current_level;
  word *next_level;
  word *checkpoint;
  word *upside_down;
  word *exit_room_timer;
  word *hitp_max;
  word *hitp_beg_lev;
  word *grab_timer;
  word *holding_sword;
  short *united_with_shadow; //
  short *pickup_obj_type;    //
  word *kid_sword_strike;
  word *offguard;
  word *have_sword;
  word *guardhp_max;
  word *demo_index;
  short *demo_time; //
  word *curr_guard_color;
  short *guard_notice_timer; //
  word *guard_skill;
  word *shadow_initialized;
  word *guard_refrac;
  word *justblocked;
  word *droppedout;
  curr_row_coll_room_t *curr_row_coll_room;
  curr_row_coll_flags_t *curr_row_coll_flags;
  below_row_coll_room_t *below_row_coll_room;
  below_row_coll_flags_t *below_row_coll_flags;
  above_row_coll_room_t *above_row_coll_room;
  above_row_coll_flags_t *above_row_coll_flags;
  sbyte *prev_collision_row;
  word *flash_color;
  word *flash_time;
  word *need_level1_music;
  word *is_screaming;
  word *is_feather_fall;
  word *last_loose_sound;
  dword *random_seed;
  dword *preserved_seed;
  short *rem_min;
  word *rem_tick;
  sbyte *control_x;
  sbyte *control_y;
  sbyte *control_shift;
  sbyte *control_forward;
  sbyte *control_backward;
  sbyte *control_up;
  sbyte *control_down;
  sbyte *control_shift2;
  sbyte *ctrl1_forward;
  sbyte *ctrl1_backward;
  sbyte *ctrl1_up;
  sbyte *ctrl1_down;
  sbyte *ctrl1_shift2;
  dword *curr_tick;
  dat_type **dathandle;
  byte **level_var_palettes;
  word *is_blind_mode;
  word *seed_was_init;
  word *need_drects;
  int *g_argc;
  char ***g_argv;
  surface_type **current_target_surface;
  SDL_Surface **onscreen_surface_;
  rect_type *screen_rect;
  word *cheats_enabled;
  word *draw_mode;
  word *demo_mode;
  int *play_demo_level;
  byte **doorlink1_ad;
  byte **doorlink2_ad;
  byte **guard_palettes;
  chtab_addrs_t *chtab_addrs;
  short *start_level; //
  surface_type **offscreen_surface;
  rect_type *rect_top;
  word *text_time_remaining;    //
  word *text_time_total;        //
  word *is_show_time;           //
  word *resurrect_time;         //
  custom_options_type **custom; //
  short *next_sound;            //
  short *can_guard_see_kid;     //
  short *hitp_delta;            //
  short *guardhp_delta;         //
  word *different_room;         //
  word *next_room;              //
  word *is_guard_notice;        //
  word *need_full_redraw;
  byte *is_validate_mode;
  exe_dir_t *exe_dir;
  bool *found_exe_dir;
  key_states_t *key_states;
  word *is_cutscene;
  bool isExitDoorOpen;
  byte *enable_quicksave_penalty;
  word *is_restart_level;
  SDL_Window **window_;
  byte* enable_copyprot;
  fixes_options_type** fixes;
  word* copyprot_plac;

  // File cache variables
  cachedFilePointerTable_t* _cachedFilePointerTable;
  cachedFileBufferTable_t* _cachedFileBufferTable;
  cachedFileBufferSizes_t* _cachedFileBufferSizes;
  cachedFilePathTable_t* _cachedFilePathTable;
  cachedFileCounter_t* _cachedFileCounter;

  private:
  void *_dllHandle;
};
