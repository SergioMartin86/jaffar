#include "SDLPopInstance.h"
#include <dlfcn.h>
#include "types.h"
#include <iostream>

char* __prince_argv[] = { (char*)"prince" };

void SDLPopInstance::initialize()
{

 // Resetting frame counter
 _currentFrame = 0;
 _currentMove = ".";

 // Setting data directory
 *found_exe_dir = true;
 sprintf(*exe_dir, "../extern/SDLPoP/");

 *is_validate_mode = 0;
 *g_argc = 1;
 *g_argv = __prince_argv;

 *random_seed = 1;
 *seed_was_init = 1;

 // debug only: check that the sequence table deobfuscation did not mess things
 // up

 load_global_options();
 check_mod_param();
 load_ingame_settings();
 turn_sound_on_off(1);
 load_mod_options();

 // CusPop option
 *is_blind_mode = 0;

 // Fix bug: with start_in_blind_mode enabled, moving objects are not displayed
 // until blind mode is toggled off+on??
 *need_drects = 1;

 apply_seqtbl_patches();
 *dathandle = open_dat("PRINCE.DAT", 0);

 /*video_mode =*/
 parse_grmode();

 init_timer(BASE_FPS);
 parse_cmdline_sound();

 set_hc_pal();

 *current_target_surface = rect_sthg(*onscreen_surface_, screen_rect);

 show_loading();
 set_joy_mode();

 *cheats_enabled = 0;
 *draw_mode = 0;
 *demo_mode = 0;

 init_copyprot_dialog();
 init_record_replay();

 *play_demo_level = 0;

 init_menu();

 //////////////////////////////////////////////
 // init_game_main

 *doorlink1_ad = /*&*/ level->doorlinks1;
 *doorlink2_ad = /*&*/ level->doorlinks2;

 prandom(1);
 *guard_palettes = (byte*) load_from_opendats_alloc(10, "bin", NULL, NULL);

 // (blood, hurt flash) #E00030 = red
 set_pal(12, 0x38, 0x00, 0x0C, 1);

 // (palace wall pattern) #C09850 = light brown
 set_pal(6, 0x30, 0x26, 0x14, 0);

 // Level color variations (1.3)
 *level_var_palettes = reinterpret_cast<byte*>(load_from_opendats_alloc(20, "bin", NULL, NULL));

 // PRINCE.DAT: sword
 (*chtab_addrs)[id_chtab_0_sword] = load_sprites_from_file(700, 1 << 2, 1);

 // PRINCE.DAT: flame, sword on floor, potion
 (*chtab_addrs)[id_chtab_1_flameswordpotion] = load_sprites_from_file(150, 1 << 3, 1);

 close_dat(*dathandle);
 init_lighting();
 load_all_sounds();

 hof_read();

 ///////////////////////////////////////////////////
 // start_game

 release_title_images();  // added
 free_optsnd_chtab();     // added

 *start_level = 1;

 ///////////////////////////////////////////////////////////////
 // init_game

 *offscreen_surface = make_offscreen_buffer(rect_top);

 load_kid_sprite();

 *text_time_remaining = 0;
 *text_time_total = 0;
 *is_show_time = 1;
 *checkpoint = 0;
 *upside_down = 0;  // N.B. upside_down is also reset in set_start_pos()
 *resurrect_time = 0;
 *rem_min = (*custom)->start_minutes_left;  // 60
 *rem_tick = (*custom)->start_ticks_left;   // 719
 *hitp_beg_lev = (*custom)->start_hitp;     // 3

 *need_level1_music = false;

//  play_level(2);

 ///////////////////////////////////////////////////////////////
 // play_level
 int level_number = 1;
 if (level_number != *current_level) load_lev_spr(level_number);

 load_level();
 pos_guards();
 clear_coll_rooms();
 clear_saved_ctrl();

 *drawn_room = 0;
 *mobs_count = 0;
 *trobs_count = 0;
 *next_sound = -1;
 *holding_sword = 0;
 *grab_timer = 0;
 *can_guard_see_kid = 0;
 *united_with_shadow = 0;
 *flash_time = 0;
 *leveldoor_open = 0;
 *demo_index = 0;
 *demo_time = 0;
 *guardhp_curr = 0;
 *hitp_delta = 0;
 Guard->charid = charid_2_guard;
 Guard->direction = dir_56_none;

 do_startpos();

 // (level_number != 1)
 *have_sword = (level_number == 0 || level_number >= (*custom)->have_sword_from_level);

 find_start_level_door();

 // busy waiting?
 while (check_sound_playing() && !do_paused()) idle();

 stop_sounds();

 draw_level_first();
 show_copyprot(0);
 reset_timer(timer_1);

 _prevDrawnRoom = *drawn_room;

 set_timer_length(timer_1, 20);
}

void SDLPopInstance::draw()
{
 restore_room_after_quick_load();
 draw_game_frame();
 update_screen();
 do_simple_wait(timer_1);
}

void SDLPopInstance::performMove(const std::string& move)
{
 _currentMove = move;

 (*key_states)[SDL_SCANCODE_UP] = 0;
 (*key_states)[SDL_SCANCODE_DOWN] = 0;
 (*key_states)[SDL_SCANCODE_LEFT] = 0;
 (*key_states)[SDL_SCANCODE_RIGHT] = 0;
 (*key_states)[SDL_SCANCODE_RSHIFT] = 0;
 (*key_states)[SDL_SCANCODE_A | WITH_CTRL] = 0;
 (*key_states)[SDL_SCANCODE_S | WITH_CTRL] = 0;

 if (move.find("R") != std::string::npos) { (*key_states)[SDL_SCANCODE_RIGHT] = 1; }
 if (move.find("L") != std::string::npos) { (*key_states)[SDL_SCANCODE_LEFT] = 1; }
 if (move.find("U") != std::string::npos) { (*key_states)[SDL_SCANCODE_UP] = 1; }
 if (move.find("D") != std::string::npos) { (*key_states)[SDL_SCANCODE_DOWN] = 1; }
 if (move.find("S") != std::string::npos) { (*key_states)[SDL_SCANCODE_RSHIFT] = 1; }
 if (move == "restart") { (*key_states)[SDL_SCANCODE_A | WITH_CTRL] = 1; }
 if (move == "toggle_sound") { (*key_states)[SDL_SCANCODE_S | WITH_CTRL] = 1; }
}

void SDLPopInstance::advanceFrame()
{
  *guardhp_delta = 0;
  *hitp_delta = 0;
  timers();
  play_frame();
  _currentFrame++;
}

void SDLPopInstance::printFrameInfo()
{
 std::cout << "Frame " << _currentFrame << " room=" << *drawn_room
           << " sdlPop.Kid->x=" << int(Kid->x) << " sdlPop.Kid->y=" << int(Kid->y)
           << " sdlPop.Guard->x=" << int(Guard->x) << " sdlPop.Guard->y=" << int(Guard->y)
           << " Keypress=" << _currentMove << " seed=" << *random_seed
           << std::endl;

 if (*drawn_room != _prevDrawnRoom)
 {
   printf("Room change!\n");
   _prevDrawnRoom = *drawn_room;
 }
}

SDLPopInstance::SDLPopInstance()
{
 _dllHandle = dlopen("./libsdlPopLib.so", RTLD_LAZY);

 // Functions
 restore_room_after_quick_load = (restore_room_after_quick_load_t) dlsym(_dllHandle, "restore_room_after_quick_load");
 load_global_options = (load_global_options_t) dlsym(_dllHandle, "load_global_options");
 check_mod_param = (check_mod_param_t) dlsym(_dllHandle, "check_mod_param");
 load_ingame_settings = (load_ingame_settings_t) dlsym(_dllHandle, "load_ingame_settings");
 turn_sound_on_off = (turn_sound_on_off_t) dlsym(_dllHandle, "turn_sound_on_off");
 load_mod_options = (load_mod_options_t) dlsym(_dllHandle, "load_mod_options");
 apply_seqtbl_patches = (apply_seqtbl_patches_t) dlsym(_dllHandle, "apply_seqtbl_patches");
 open_dat = (open_dat_t) dlsym(_dllHandle, "open_dat");
 parse_grmode = (parse_grmode_t) dlsym(_dllHandle, "parse_grmode");
 init_timer = (init_timer_t) dlsym(_dllHandle, "init_timer");
 parse_cmdline_sound = (parse_cmdline_sound_t) dlsym(_dllHandle, "parse_cmdline_sound");
 set_hc_pal = (set_hc_pal_t) dlsym(_dllHandle, "set_hc_pal");
 rect_sthg = (rect_sthg_t) dlsym(_dllHandle, "rect_sthg");
 show_loading = (show_loading_t) dlsym(_dllHandle, "show_loading");
 set_joy_mode = (set_joy_mode_t) dlsym(_dllHandle, "set_joy_mode");
 init_copyprot_dialog = (init_copyprot_dialog_t) dlsym(_dllHandle, "init_copyprot_dialog");
 init_record_replay = (init_record_replay_t) dlsym(_dllHandle, "init_record_replay");
 init_menu = (init_menu_t) dlsym(_dllHandle, "init_menu");
 prandom = (prandom_t) dlsym(_dllHandle, "prandom");
 load_from_opendats_alloc = (load_from_opendats_alloc_t) dlsym(_dllHandle, "load_from_opendats_alloc");
 set_pal_256 = (set_pal_256_t) dlsym(_dllHandle, "set_pal_256");
 set_pal = (set_pal_t) dlsym(_dllHandle, "set_pal");
 load_sprites_from_file = (load_sprites_from_file_t) dlsym(_dllHandle, "load_sprites_from_file");
 close_dat = (close_dat_t) dlsym(_dllHandle, "close_dat");
 init_lighting = (init_lighting_t) dlsym(_dllHandle, "init_lighting");
 load_all_sounds = (load_all_sounds_t) dlsym(_dllHandle, "load_all_sounds");
 release_title_images = (release_title_images_t) dlsym(_dllHandle, "release_title_images");
 free_optsnd_chtab = (free_optsnd_chtab_t) dlsym(_dllHandle, "free_optsnd_chtab");
 make_offscreen_buffer = (make_offscreen_buffer_t) dlsym(_dllHandle, "make_offscreen_buffer");
 load_lev_spr = (load_lev_spr_t) dlsym(_dllHandle, "load_lev_spr");
 load_level = (load_level_t) dlsym(_dllHandle, "load_level");
 pos_guards = (pos_guards_t) dlsym(_dllHandle, "pos_guards");
 clear_coll_rooms = (clear_coll_rooms_t) dlsym(_dllHandle, "clear_coll_rooms");
 clear_saved_ctrl = (clear_saved_ctrl_t) dlsym(_dllHandle, "clear_saved_ctrl");
 do_startpos = (do_startpos_t) dlsym(_dllHandle, "do_startpos");
 find_start_level_door = (find_start_level_door_t) dlsym(_dllHandle, "find_start_level_door");
 check_sound_playing = (check_sound_playing_t) dlsym(_dllHandle, "check_sound_playing");
 do_paused = (check_sound_playing_t) dlsym(_dllHandle, "do_paused");
 load_kid_sprite = (load_kid_sprite_t) dlsym(_dllHandle, "load_kid_sprite");
 idle = (idle_t) dlsym(_dllHandle, "idle");
 hof_read = (hof_read_t) dlsym(_dllHandle, "hof_read");
 stop_sounds = (stop_sounds_t) dlsym(_dllHandle, "stop_sounds");
 show_copyprot = (show_copyprot_t) dlsym(_dllHandle, "show_copyprot");
 reset_timer = (reset_timer_t) dlsym(_dllHandle, "reset_timer");
 free_peels = (free_peels_t) dlsym(_dllHandle, "free_peels");
 play_level_2 = (play_level_2_t) dlsym(_dllHandle, "play_level_2");
 timers = (timers_t) dlsym(_dllHandle, "timers");
 play_frame = (play_frame_t) dlsym(_dllHandle, "play_frame");
 draw_game_frame = (draw_game_frame_t) dlsym(_dllHandle, "draw_game_frame");
 update_screen = (update_screen_t) dlsym(_dllHandle, "update_screen");
 do_simple_wait = (do_simple_wait_t) dlsym(_dllHandle, "do_simple_wait");
 reset_level_unused_fields = (reset_level_unused_fields_t) dlsym(_dllHandle, "reset_level_unused_fields");
 load_room_links = (load_room_links_t) dlsym(_dllHandle, "load_room_links");
 set_timer_length = (set_timer_length_t) dlsym(_dllHandle, "set_timer_length");
 draw_level_first = (draw_level_first_t) dlsym(_dllHandle, "draw_level_first");
 play_level = (play_level_t) dlsym(_dllHandle, "play_level");

 // State variables
 Kid = (char_type*) dlsym(_dllHandle, "Kid");
 Guard = (char_type*) dlsym(_dllHandle, "Guard");
 Char = (char_type*) dlsym(_dllHandle, "Char");
 Opp = (char_type*) dlsym(_dllHandle, "Opp");
 trobs_count = (short*) dlsym(_dllHandle, "trobs_count");
 trobs = (trobs_t*) dlsym(_dllHandle, "trobs");
 mobs_count = (short*) dlsym(_dllHandle, "mobs_count");
 mobs = (mobs_t*) dlsym(_dllHandle, "mobs");
 level = (level_type*) dlsym(_dllHandle, "level");
 drawn_room = (word*) dlsym(_dllHandle, "drawn_room");
 leveldoor_open = (word*) dlsym(_dllHandle, "leveldoor_open");
 hitp_curr = (word*) dlsym(_dllHandle, "hitp_curr");
 guardhp_curr = (word*) dlsym(_dllHandle, "guardhp_curr");
 current_level = (word*) dlsym(_dllHandle, "current_level");
 next_level = (word*) dlsym(_dllHandle, "next_level");
 checkpoint = (word*) dlsym(_dllHandle, "checkpoint");
 upside_down = (word*) dlsym(_dllHandle, "upside_down");
 exit_room_timer = (word*) dlsym(_dllHandle, "exit_room_timer");
 hitp_max = (word*) dlsym(_dllHandle, "hitp_max");
 hitp_beg_lev = (word*) dlsym(_dllHandle, "hitp_beg_lev");
 grab_timer = (word*) dlsym(_dllHandle, "grab_timer");
 holding_sword = (word*) dlsym(_dllHandle, "holding_sword");
 united_with_shadow = (short*) dlsym(_dllHandle, "united_with_shadow");
 pickup_obj_type = (short*) dlsym(_dllHandle, "pickup_obj_type");
 kid_sword_strike = (word*) dlsym(_dllHandle, "kid_sword_strike");
 offguard = (word*) dlsym(_dllHandle, "offguard");
 have_sword = (word*) dlsym(_dllHandle, "have_sword");
 guardhp_max = (word*) dlsym(_dllHandle, "guardhp_max");
 demo_index = (word*) dlsym(_dllHandle, "demo_index");
 demo_time = (short*) dlsym(_dllHandle, "demo_time");
 curr_guard_color = (word*) dlsym(_dllHandle, "curr_guard_color");
 guard_notice_timer = (short*) dlsym(_dllHandle, "guard_notice_timer");
 guard_skill = (word*) dlsym(_dllHandle, "guard_skill");
 shadow_initialized = (word*) dlsym(_dllHandle, "shadow_initialized");
 guard_refrac = (word*) dlsym(_dllHandle, "guard_refrac");
 justblocked = (word*) dlsym(_dllHandle, "justblocked");
 droppedout = (word*) dlsym(_dllHandle, "droppedout");
 curr_row_coll_room = (curr_row_coll_room_t*) dlsym(_dllHandle, "curr_row_coll_room");
 curr_row_coll_flags = (curr_row_coll_flags_t*) dlsym(_dllHandle, "curr_row_coll_flags");
 above_row_coll_room = (above_row_coll_room_t*) dlsym(_dllHandle, "above_row_coll_room");
 above_row_coll_flags = (above_row_coll_flags_t*) dlsym(_dllHandle, "above_row_coll_flags");
 below_row_coll_room = (below_row_coll_room_t*) dlsym(_dllHandle, "below_row_coll_room");
 below_row_coll_flags = (below_row_coll_flags_t*) dlsym(_dllHandle, "below_row_coll_flags");
 prev_collision_row = (sbyte*) dlsym(_dllHandle, "prev_collision_row");
 flash_color = (word*) dlsym(_dllHandle, "flash_color");
 flash_time = (word*) dlsym(_dllHandle, "flash_time");
 need_level1_music = (word*) dlsym(_dllHandle, "need_level1_music");
 is_screaming = (word*) dlsym(_dllHandle, "is_screaming");
 is_feather_fall = (word*) dlsym(_dllHandle, "is_feather_fall");
 last_loose_sound = (word*) dlsym(_dllHandle, "last_loose_sound");
 random_seed = (dword*) dlsym(_dllHandle, "random_seed");
 rem_min = (short*) dlsym(_dllHandle, "rem_min");
 rem_tick = (word*) dlsym(_dllHandle, "rem_tick");
 control_x = (sbyte*) dlsym(_dllHandle, "control_x");
 control_y = (sbyte*) dlsym(_dllHandle, "control_y");
 control_shift = (sbyte*) dlsym(_dllHandle, "control_shift");
 control_forward = (sbyte*) dlsym(_dllHandle, "control_forward");
 control_backward = (sbyte*) dlsym(_dllHandle, "control_backward");
 control_up = (sbyte*) dlsym(_dllHandle, "control_up");
 control_down = (sbyte*) dlsym(_dllHandle, "control_down");
 control_shift2 = (sbyte*) dlsym(_dllHandle, "control_shift2");
 ctrl1_forward = (sbyte*) dlsym(_dllHandle, "ctrl1_forward");
 ctrl1_backward = (sbyte*) dlsym(_dllHandle, "ctrl1_backward");
 ctrl1_up = (sbyte*) dlsym(_dllHandle, "ctrl1_up");
 ctrl1_down = (sbyte*) dlsym(_dllHandle, "ctrl1_down");
 ctrl1_shift2 = (sbyte*) dlsym(_dllHandle, "ctrl1_shift2");
 curr_tick = (dword*) dlsym(_dllHandle, "curr_tick");
 dathandle = (dat_type**) dlsym(_dllHandle, "dathandle");
 level_var_palettes = (byte**) dlsym(_dllHandle, "level_var_palettes");
 is_blind_mode = (word*) dlsym(_dllHandle, "is_blind_mode");
 seed_was_init = (word*) dlsym(_dllHandle, "seed_was_init");
 dathandle = (dat_type**) dlsym(_dllHandle, "dathandle");
 need_drects = (word*) dlsym(_dllHandle, "need_drects");
 g_argc = (int*) dlsym(_dllHandle, "g_argc");
 g_argv = (char***) dlsym(_dllHandle, "g_argv");
 current_target_surface = (surface_type**) dlsym(_dllHandle, "current_target_surface");
 onscreen_surface_ = (SDL_Surface**) dlsym(_dllHandle, "onscreen_surface_");
 screen_rect = (rect_type*) dlsym(_dllHandle, "screen_rect");
 cheats_enabled = (word*) dlsym(_dllHandle, "cheats_enabled");
 draw_mode = (word*) dlsym(_dllHandle, "draw_mode");
 demo_mode = (word*) dlsym(_dllHandle, "demo_mode");
 play_demo_level = (int*) dlsym(_dllHandle, "play_demo_level");
 doorlink1_ad = (byte**) dlsym(_dllHandle, "doorlink1_ad");
 doorlink2_ad = (byte**) dlsym(_dllHandle, "doorlink2_ad");
 guard_palettes = (byte**) dlsym(_dllHandle, "guard_palettes");
 chtab_addrs = (chtab_addrs_t*) dlsym(_dllHandle, "chtab_addrs");
 start_level = (short*) dlsym(_dllHandle, "start_level");
 offscreen_surface = (surface_type**) dlsym(_dllHandle, "offscreen_surface");
 rect_top = (rect_type*) dlsym(_dllHandle, "rect_top");
 text_time_remaining = (word*) dlsym(_dllHandle, "text_time_remaining");
 text_time_total = (word*) dlsym(_dllHandle, "text_time_total");
 is_show_time = (word*) dlsym(_dllHandle, "is_show_time");
 resurrect_time = (word*) dlsym(_dllHandle, "resurrect_time");
 custom = (custom_options_type**) dlsym(_dllHandle, "custom");
 next_sound = (short*) dlsym(_dllHandle, "next_sound");
 can_guard_see_kid = (short*) dlsym(_dllHandle, "can_guard_see_kid");
 hitp_delta = (short*) dlsym(_dllHandle, "hitp_delta");
 guardhp_delta = (short*) dlsym(_dllHandle, "guardhp_delta");
 different_room = (word*) dlsym(_dllHandle, "different_room");
 next_room = (word*) dlsym(_dllHandle, "next_room");
 is_guard_notice = (word*) dlsym(_dllHandle, "is_guard_notice");
 need_full_redraw = (word*) dlsym(_dllHandle, "need_full_redraw");
 is_validate_mode = (byte*) dlsym(_dllHandle, "is_validate_mode");
 exe_dir = (exe_dir_t*) dlsym(_dllHandle, "exe_dir");
 found_exe_dir = (bool*) dlsym(_dllHandle, "found_exe_dir");
 key_states = (key_states_t*) dlsym(_dllHandle, "key_states");
}

SDLPopInstance::~SDLPopInstance()
{
 SDL_Quit();
 dlclose(_dllHandle);
}

