#include "sdlpop-instance.h"
#include <dlfcn.h>
#include "types.h"

char* __prince_argv[] = { (char*)"prince" };

void SDLPopInstance::initialize()
{
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

 // play_level(1);

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
}

SDLPopInstance::SDLPopInstance()
{
 dllHandle_ = dlopen("./libsdlPopLib.so", RTLD_LAZY);

 // Functions
 restore_room_after_quick_load = (restore_room_after_quick_load_t) dlsym(dllHandle_, "restore_room_after_quick_load");
 load_global_options = (load_global_options_t) dlsym(dllHandle_, "load_global_options");
 check_mod_param = (check_mod_param_t) dlsym(dllHandle_, "check_mod_param");
 load_ingame_settings = (load_ingame_settings_t) dlsym(dllHandle_, "load_ingame_settings");
 turn_sound_on_off = (turn_sound_on_off_t) dlsym(dllHandle_, "turn_sound_on_off");
 load_mod_options = (load_mod_options_t) dlsym(dllHandle_, "load_mod_options");
 apply_seqtbl_patches = (apply_seqtbl_patches_t) dlsym(dllHandle_, "apply_seqtbl_patches");
 open_dat = (open_dat_t) dlsym(dllHandle_, "open_dat");
 parse_grmode = (parse_grmode_t) dlsym(dllHandle_, "parse_grmode");
 init_timer = (init_timer_t) dlsym(dllHandle_, "init_timer");
 parse_cmdline_sound = (parse_cmdline_sound_t) dlsym(dllHandle_, "parse_cmdline_sound");
 set_hc_pal = (set_hc_pal_t) dlsym(dllHandle_, "set_hc_pal");
 rect_sthg = (rect_sthg_t) dlsym(dllHandle_, "rect_sthg");
 show_loading = (show_loading_t) dlsym(dllHandle_, "show_loading");
 set_joy_mode = (set_joy_mode_t) dlsym(dllHandle_, "set_joy_mode");
 init_copyprot_dialog = (init_copyprot_dialog_t) dlsym(dllHandle_, "init_copyprot_dialog");
 init_record_replay = (init_record_replay_t) dlsym(dllHandle_, "init_record_replay");
 init_menu = (init_menu_t) dlsym(dllHandle_, "init_menu");
 prandom = (prandom_t) dlsym(dllHandle_, "prandom");
 load_from_opendats_alloc = (load_from_opendats_alloc_t) dlsym(dllHandle_, "load_from_opendats_alloc");
 set_pal_256 = (set_pal_256_t) dlsym(dllHandle_, "set_pal_256");
 set_pal = (set_pal_t) dlsym(dllHandle_, "set_pal");
 load_sprites_from_file = (load_sprites_from_file_t) dlsym(dllHandle_, "load_sprites_from_file");
 close_dat = (close_dat_t) dlsym(dllHandle_, "close_dat");
 init_lighting = (init_lighting_t) dlsym(dllHandle_, "init_lighting");
 load_all_sounds = (load_all_sounds_t) dlsym(dllHandle_, "load_all_sounds");
 release_title_images = (release_title_images_t) dlsym(dllHandle_, "release_title_images");
 free_optsnd_chtab = (free_optsnd_chtab_t) dlsym(dllHandle_, "free_optsnd_chtab");
 make_offscreen_buffer = (make_offscreen_buffer_t) dlsym(dllHandle_, "make_offscreen_buffer");
 load_lev_spr = (load_lev_spr_t) dlsym(dllHandle_, "load_lev_spr");
 load_level = (load_level_t) dlsym(dllHandle_, "load_level");
 pos_guards = (pos_guards_t) dlsym(dllHandle_, "pos_guards");
 clear_coll_rooms = (clear_coll_rooms_t) dlsym(dllHandle_, "clear_coll_rooms");
 clear_saved_ctrl = (clear_saved_ctrl_t) dlsym(dllHandle_, "clear_saved_ctrl");
 do_startpos = (do_startpos_t) dlsym(dllHandle_, "do_startpos");
 find_start_level_door = (find_start_level_door_t) dlsym(dllHandle_, "find_start_level_door");
 check_sound_playing = (check_sound_playing_t) dlsym(dllHandle_, "check_sound_playing");
 do_paused = (check_sound_playing_t) dlsym(dllHandle_, "do_paused");
 load_kid_sprite = (load_kid_sprite_t) dlsym(dllHandle_, "load_kid_sprite");
 idle = (idle_t) dlsym(dllHandle_, "idle");
 hof_read = (hof_read_t) dlsym(dllHandle_, "hof_read");
 stop_sounds = (stop_sounds_t) dlsym(dllHandle_, "stop_sounds");
 show_copyprot = (show_copyprot_t) dlsym(dllHandle_, "show_copyprot");
 reset_timer = (reset_timer_t) dlsym(dllHandle_, "reset_timer");
 free_peels = (free_peels_t) dlsym(dllHandle_, "free_peels");
 play_level_2 = (play_level_2_t) dlsym(dllHandle_, "play_level_2");
 timers = (timers_t) dlsym(dllHandle_, "timers");
 play_frame = (play_frame_t) dlsym(dllHandle_, "play_frame");
 draw_game_frame = (draw_game_frame_t) dlsym(dllHandle_, "draw_game_frame");
 update_screen = (update_screen_t) dlsym(dllHandle_, "update_screen");
 do_simple_wait = (do_simple_wait_t) dlsym(dllHandle_, "do_simple_wait");
 reset_level_unused_fields = (reset_level_unused_fields_t) dlsym(dllHandle_, "reset_level_unused_fields");
 load_room_links = (load_room_links_t) dlsym(dllHandle_, "load_room_links");
 set_timer_length = (set_timer_length_t) dlsym(dllHandle_, "set_timer_length");
 draw_level_first = (draw_level_first_t) dlsym(dllHandle_, "draw_level_first");
 play_level = (play_level_t) dlsym(dllHandle_, "play_level");

 // State variables
 Kid = (char_type*) dlsym(dllHandle_, "Kid");
 Guard = (char_type*) dlsym(dllHandle_, "Guard");
 Char = (char_type*) dlsym(dllHandle_, "Char");
 Opp = (char_type*) dlsym(dllHandle_, "Opp");
 trobs_count = (short*) dlsym(dllHandle_, "trobs_count");
 trobs = (trobs_t*) dlsym(dllHandle_, "trobs");
 mobs_count = (short*) dlsym(dllHandle_, "mobs_count");
 mobs = (mobs_t*) dlsym(dllHandle_, "mobs");
 level = (level_type*) dlsym(dllHandle_, "level");
 drawn_room = (word*) dlsym(dllHandle_, "drawn_room");
 leveldoor_open = (word*) dlsym(dllHandle_, "leveldoor_open");
 hitp_curr = (word*) dlsym(dllHandle_, "hitp_curr");
 guardhp_curr = (word*) dlsym(dllHandle_, "guardhp_curr");
 current_level = (word*) dlsym(dllHandle_, "current_level");
 next_level = (word*) dlsym(dllHandle_, "next_level");
 checkpoint = (word*) dlsym(dllHandle_, "checkpoint");
 upside_down = (word*) dlsym(dllHandle_, "upside_down");
 exit_room_timer = (word*) dlsym(dllHandle_, "exit_room_timer");
 hitp_max = (word*) dlsym(dllHandle_, "hitp_max");
 hitp_beg_lev = (word*) dlsym(dllHandle_, "hitp_beg_lev");
 grab_timer = (word*) dlsym(dllHandle_, "grab_timer");
 holding_sword = (word*) dlsym(dllHandle_, "holding_sword");
 united_with_shadow = (short*) dlsym(dllHandle_, "united_with_shadow");
 pickup_obj_type = (short*) dlsym(dllHandle_, "pickup_obj_type");
 kid_sword_strike = (word*) dlsym(dllHandle_, "kid_sword_strike");
 offguard = (word*) dlsym(dllHandle_, "offguard");
 have_sword = (word*) dlsym(dllHandle_, "have_sword");
 guardhp_max = (word*) dlsym(dllHandle_, "guardhp_max");
 demo_index = (word*) dlsym(dllHandle_, "demo_index");
 demo_time = (short*) dlsym(dllHandle_, "demo_time");
 curr_guard_color = (word*) dlsym(dllHandle_, "curr_guard_color");
 guard_notice_timer = (short*) dlsym(dllHandle_, "guard_notice_timer");
 guard_skill = (word*) dlsym(dllHandle_, "guard_skill");
 shadow_initialized = (word*) dlsym(dllHandle_, "shadow_initialized");
 guard_refrac = (word*) dlsym(dllHandle_, "guard_refrac");
 justblocked = (word*) dlsym(dllHandle_, "justblocked");
 droppedout = (word*) dlsym(dllHandle_, "droppedout");
 curr_row_coll_room = (curr_row_coll_room_t*) dlsym(dllHandle_, "curr_row_coll_room");
 curr_row_coll_flags = (curr_row_coll_flags_t*) dlsym(dllHandle_, "curr_row_coll_flags");
 above_row_coll_room = (above_row_coll_room_t*) dlsym(dllHandle_, "above_row_coll_room");
 above_row_coll_flags = (above_row_coll_flags_t*) dlsym(dllHandle_, "above_row_coll_flags");
 below_row_coll_room = (below_row_coll_room_t*) dlsym(dllHandle_, "below_row_coll_room");
 below_row_coll_flags = (below_row_coll_flags_t*) dlsym(dllHandle_, "below_row_coll_flags");
 prev_collision_row = (sbyte*) dlsym(dllHandle_, "prev_collision_row");
 flash_color = (word*) dlsym(dllHandle_, "flash_color");
 flash_time = (word*) dlsym(dllHandle_, "flash_time");
 need_level1_music = (word*) dlsym(dllHandle_, "need_level1_music");
 is_screaming = (word*) dlsym(dllHandle_, "is_screaming");
 is_feather_fall = (word*) dlsym(dllHandle_, "is_feather_fall");
 last_loose_sound = (word*) dlsym(dllHandle_, "last_loose_sound");
 random_seed = (dword*) dlsym(dllHandle_, "random_seed");
 rem_min = (short*) dlsym(dllHandle_, "rem_min");
 rem_tick = (word*) dlsym(dllHandle_, "rem_tick");
 control_x = (sbyte*) dlsym(dllHandle_, "control_x");
 control_y = (sbyte*) dlsym(dllHandle_, "control_y");
 control_shift = (sbyte*) dlsym(dllHandle_, "control_shift");
 control_forward = (sbyte*) dlsym(dllHandle_, "control_forward");
 control_backward = (sbyte*) dlsym(dllHandle_, "control_backward");
 control_up = (sbyte*) dlsym(dllHandle_, "control_up");
 control_down = (sbyte*) dlsym(dllHandle_, "control_down");
 control_shift2 = (sbyte*) dlsym(dllHandle_, "control_shift2");
 ctrl1_forward = (sbyte*) dlsym(dllHandle_, "ctrl1_forward");
 ctrl1_backward = (sbyte*) dlsym(dllHandle_, "ctrl1_backward");
 ctrl1_up = (sbyte*) dlsym(dllHandle_, "ctrl1_up");
 ctrl1_down = (sbyte*) dlsym(dllHandle_, "ctrl1_down");
 ctrl1_shift2 = (sbyte*) dlsym(dllHandle_, "ctrl1_shift2");
 curr_tick = (dword*) dlsym(dllHandle_, "curr_tick");
 dathandle = (dat_type**) dlsym(dllHandle_, "dathandle");
 level_var_palettes = (byte**) dlsym(dllHandle_, "level_var_palettes");
 automatic_control = (int*) dlsym(dllHandle_, "automatic_control");
 is_blind_mode = (word*) dlsym(dllHandle_, "is_blind_mode");
 seed_was_init = (word*) dlsym(dllHandle_, "seed_was_init");
 dathandle = (dat_type**) dlsym(dllHandle_, "dathandle");
 need_drects = (word*) dlsym(dllHandle_, "need_drects");
 g_argc = (int*) dlsym(dllHandle_, "g_argc");
 g_argv = (char***) dlsym(dllHandle_, "g_argv");
 current_target_surface = (surface_type**) dlsym(dllHandle_, "current_target_surface");
 onscreen_surface_ = (SDL_Surface**) dlsym(dllHandle_, "onscreen_surface_");
 screen_rect = (rect_type*) dlsym(dllHandle_, "screen_rect");
 cheats_enabled = (word*) dlsym(dllHandle_, "cheats_enabled");
 draw_mode = (word*) dlsym(dllHandle_, "draw_mode");
 demo_mode = (word*) dlsym(dllHandle_, "demo_mode");
 play_demo_level = (int*) dlsym(dllHandle_, "play_demo_level");
 doorlink1_ad = (byte**) dlsym(dllHandle_, "doorlink1_ad");
 doorlink2_ad = (byte**) dlsym(dllHandle_, "doorlink2_ad");
 guard_palettes = (byte**) dlsym(dllHandle_, "guard_palettes");
 chtab_addrs = (chtab_addrs_t*) dlsym(dllHandle_, "chtab_addrs");
 start_level = (short*) dlsym(dllHandle_, "start_level");
 offscreen_surface = (surface_type**) dlsym(dllHandle_, "offscreen_surface");
 rect_top = (rect_type*) dlsym(dllHandle_, "rect_top");
 text_time_remaining = (word*) dlsym(dllHandle_, "text_time_remaining");
 text_time_total = (word*) dlsym(dllHandle_, "text_time_total");
 is_show_time = (word*) dlsym(dllHandle_, "is_show_time");
 resurrect_time = (word*) dlsym(dllHandle_, "resurrect_time");
 custom = (custom_options_type**) dlsym(dllHandle_, "custom");
 next_sound = (short*) dlsym(dllHandle_, "next_sound");
 can_guard_see_kid = (short*) dlsym(dllHandle_, "can_guard_see_kid");
 hitp_delta = (short*) dlsym(dllHandle_, "hitp_delta");
 guardhp_delta = (short*) dlsym(dllHandle_, "guardhp_delta");
 different_room = (word*) dlsym(dllHandle_, "different_room");
 next_room = (word*) dlsym(dllHandle_, "next_room");
 is_guard_notice = (word*) dlsym(dllHandle_, "is_guard_notice");
 need_full_redraw = (word*) dlsym(dllHandle_, "need_full_redraw");
}

SDLPopInstance::~SDLPopInstance()
{
 dlclose(dllHandle_);
}
