#include "miniPoPInstance.h"
#include "utils.h"
#include <dlfcn.h>
#include <iostream>
#include <omp.h>

void miniPoPInstance::initialize()
{
  // Looking for sdlpop for base data in default folders when running examples
  if (dirExists("extern/SDLPoP/"))
  {
    sprintf(exe_dir, "extern/SDLPoP/");
    found_exe_dir = true;
  }
  if (dirExists("../extern/SDLPoP/"))
  {
    sprintf(exe_dir, "../extern/SDLPoP/");
    found_exe_dir = true;
  }
  if (dirExists("../../extern/SDLPoP/"))
  {
    found_exe_dir = true;
    sprintf(exe_dir, "../../extern/SDLPoP/");
  }
  if (dirExists("../../../extern/SDLPoP/"))
  {
    sprintf(exe_dir, "../../../extern/SDLPoP/");
    found_exe_dir = true;
  }

  // If not found, looking for the SDLPOP_ROOT env variable
  if (found_exe_dir == false)
  {
    const char *envRoot = std::getenv("SDLPOP_ROOT");

    if (envRoot != NULL)
      if (dirExists(envRoot))
      {
        sprintf(exe_dir, envRoot);
        found_exe_dir = true;
      }
  }

  if (found_exe_dir == false)
  {
    fprintf(stderr, "[ERROR] Could not find the root folder for  Please set the SDLPOP_ROOT environment variable to the path where SDLPop is installed.\n");
    exit(-1);
  }

  // Setting argument config
  is_validate_mode = true;
  g_argc = 1;

  // Fix feather fall problem when quickload/quicksaving
  init_copyprot();

  // Fix bug: with start_in_blind_mode enabled, moving objects are not displayed
  // until blind mode is toggled off+on??
  need_drects = 1;

  dathandle = open_dat("PRINCE.DAT", 0);

  parse_cmdline_sound();

  //////////////////////////////////////////////
  // init_game_main

  doorlink1_ad = /*&*/ level.doorlinks1;
  doorlink2_ad = /*&*/ level.doorlinks2;
  guard_palettes = (byte *)load_from_opendats_alloc(10, "bin", NULL, NULL);

  // Level color variations (1.3)
  level_var_palettes = reinterpret_cast<byte *>(load_from_opendats_alloc(20, "bin", NULL, NULL));

  // PRINCE.DAT: sword
  chtab_addrs[id_chtab_0_sword] = load_sprites_from_file(700, 1 << 2, 1);

  // PRINCE.DAT: flame, sword on floor, potion
  chtab_addrs[id_chtab_1_flameswordpotion] = load_sprites_from_file(150, 1 << 3, 1);

  close_dat(dathandle);

  // start_game

  start_level = 1;

  ///////////////////////////////////////////////////////////////
  // init_game

  text_time_remaining = 0;
  text_time_total = 0;
  is_show_time = 1;
  checkpoint = 0;
  upside_down = 0; // N.B. upside_down is also reset in set_start_pos()
  resurrect_time = 0;
  rem_min = custom->start_minutes_left; // 60
  rem_tick = custom->start_ticks_left;  // 719
  hitp_beg_lev = custom->start_hitp;    // 3
  current_level = 0;
  startLevel(1);
  need_level1_music = custom->intro_music_time_initial;
}

void miniPoPInstance::startLevel(const word level)
{
 ///////////////////////////////////////////////////////////////
  // play_level
  if (level != current_level) load_lev_spr(level);

  load_kid_sprite();
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

  // (level_number != 1)
  have_sword = (level == 0 || level >= custom->have_sword_from_level);

  find_start_level_door();

//  draw_level_first();

  _prevDrawnRoom = drawn_room;

  // Setting exit door status
  isExitDoorOpen = isLevelExitDoorOpen();

 if (need_level1_music != 0 && current_level == custom->intro_music_level)
   need_level1_music = custom->intro_music_time_restart;
}

void miniPoPInstance::setSeed(const dword randomSeed)
{
  random_seed = randomSeed;
}

size_t miniPoPInstance::getElapsedMins()
{
 return 60 - rem_min;
}

size_t miniPoPInstance::getElapsedSecs()
{
 return (720 - rem_tick) / 12;
}

size_t miniPoPInstance::getElapsedMilisecs()
{
 return ceil( ((double)((720 - rem_tick) % 12) * (60.0 / 720.0)) * 1000.0 );
}

void miniPoPInstance::draw(ssize_t mins, ssize_t secs, ssize_t ms)
{
  restore_room_after_quick_load();
}

std::string miniPoPInstance::serializeFileCache()
{
 size_t cacheSize = 0;

 // Cache File Counter
 cacheSize += sizeof(size_t);

 // Cache File paths
 cacheSize += _cachedFileCounter * sizeof(char) * POP_MAX_PATH;

 // Cache File buffer sizes
 cacheSize += _cachedFileCounter * sizeof(size_t);

 // Size Per file:
 for (size_t i = 0; i < _cachedFileCounter; i++)
  cacheSize += _cachedFileBufferSizes[i];

 // Allocating cache string
 std::string cache;
 cache.resize(cacheSize);

 // Copying file counter
 size_t curPosition = 0;
 memcpy(&cache[curPosition], &_cachedFileCounter, sizeof(size_t));
 curPosition += sizeof(size_t);

 // Copying file paths
 memcpy(&cache[curPosition], _cachedFilePathTable, _cachedFileCounter * sizeof(char) * POP_MAX_PATH);
 curPosition += _cachedFileCounter * sizeof(char) * POP_MAX_PATH;

 // Copying buffer sizes
 memcpy(&cache[curPosition], _cachedFileBufferSizes, _cachedFileCounter * sizeof(size_t));
 curPosition += _cachedFileCounter * sizeof(size_t);

 // Per file:
 for (size_t i = 0; i < _cachedFileCounter; i++)
 {
  // Copying buffer sizes
  memcpy(&cache[curPosition], _cachedFileBufferTable[i], _cachedFileBufferSizes[i] * sizeof(char));
  curPosition += _cachedFileBufferSizes[i] * sizeof(char);
 }

 return cache;
}

void miniPoPInstance::deserializeFileCache(const std::string& cache)
{
 // Copying file counter
 size_t curPosition = 0;
 memcpy(&_cachedFileCounter, &cache[curPosition], sizeof(size_t));
 curPosition += sizeof(size_t);

 // Copying file paths
 memcpy(_cachedFilePathTable, &cache[curPosition], _cachedFileCounter * sizeof(char) * POP_MAX_PATH);
 curPosition += _cachedFileCounter * sizeof(char) * POP_MAX_PATH;

 // Copying buffer sizes
 memcpy(_cachedFileBufferSizes, &cache[curPosition], _cachedFileCounter * sizeof(size_t));
 curPosition += _cachedFileCounter * sizeof(size_t);

 // Per file:
 for (size_t i = 0; i < _cachedFileCounter; i++)
 {
  // Allocating file content buffer
  _cachedFileBufferTable[i] = (char*) malloc(_cachedFileBufferSizes[i]);

  // Copying buffer sizes
  memcpy(_cachedFileBufferTable[i], &cache[curPosition], _cachedFileBufferSizes[i] * sizeof(char));
  curPosition += _cachedFileBufferSizes[i] * sizeof(char);
 }
}

void miniPoPInstance::performMove(const std::string &move)
{
  key_states[SDL_SCANCODE_UP] = 0;
  key_states[SDL_SCANCODE_DOWN] = 0;
  key_states[SDL_SCANCODE_LEFT] = 0;
  key_states[SDL_SCANCODE_RIGHT] = 0;
  key_states[SDL_SCANCODE_RSHIFT] = 0;

  bool recognizedMove = false;

  if (move.find(".") != std::string::npos) { recognizedMove = true; }
  if (move.find("R") != std::string::npos)
  {
    key_states[SDL_SCANCODE_RIGHT] = 1;
    recognizedMove = true;
  }
  if (move.find("L") != std::string::npos)
  {
    key_states[SDL_SCANCODE_LEFT] = 1;
    recognizedMove = true;
  }
  if (move.find("U") != std::string::npos)
  {
    key_states[SDL_SCANCODE_UP] = 1;
    recognizedMove = true;
  }
  if (move.find("D") != std::string::npos)
  {
    key_states[SDL_SCANCODE_DOWN] = 1;
    recognizedMove = true;
  }
  if (move.find("S") != std::string::npos)
  {
    key_states[SDL_SCANCODE_RSHIFT] = 1;
    recognizedMove = true;
  }
  if (move == "CA") // Ctrl+A
  {
    is_restart_level = 1;
    recognizedMove = true;
  }

  if (recognizedMove == false)
    EXIT_WITH_ERROR("[Error] Unrecognized move: %s\n", move.c_str());
}

dword miniPoPInstance::advanceRNGState(const dword randomSeed)
{
 return randomSeed * 214013 + 2531011;
}

dword miniPoPInstance::reverseRNGState(const dword randomSeed)
{
 return (randomSeed + 4292436285) * 3115528533;
}


void miniPoPInstance::advanceFrame()
{
  guardhp_delta = 0;
  hitp_delta = 0;
  timers();

  play_frame();

  if (is_restart_level == 1)
   startLevel(current_level);

  // if we're on lvl 4, check mirror
  if (current_level == 4) check_mirror();

  // If level has changed, then load it
  if (current_level != next_level)
  {
   if (current_level == custom->copyprot_level-1 && next_level == custom->copyprot_level)
    next_level = 15;

   if (current_level == 15)
    next_level = custom->copyprot_level;

   startLevel(next_level);

   // Handle cutscenes
   //if (next_level == 2) for (size_t i = 0; i < 3; i++) random_seed = advanceRNGState(random_seed);
  }

  is_restart_level = 0;
  _prevDrawnRoom = drawn_room;
  isExitDoorOpen = isLevelExitDoorOpen();
}

void miniPoPInstance::printFrameInfo()
{
  printf("[Jaffar]  + Current/Next Level: %2d / %2d\n", current_level, next_level);
  printf("[Jaffar]  + Cumulative IGT: %2lu:%02lu.%03lu\n", getElapsedMins(), getElapsedSecs(), getElapsedMilisecs());
  printf("[Jaffar]  + [Kid]   Room: %d, Pos.x: %3d, Pos.y: %3d, Frame: %3d, HP: %d/%d, Sequence: %4d\n", int(Kid.room), int(Kid.x), int(Kid.y), int(Kid.frame), int(hitp_curr), int(hitp_max), int(Kid.curr_seq));
  printf("[Jaffar]  + [Guard] Room: %d, Pos.x: %3d, Pos.y: %3d, Frame: %3d, HP: %d/%d, Sequence: %4d\n", int(Guard.room), int(Guard.x), int(Guard.y), int(Guard.frame), int(guardhp_curr), int(guardhp_max), int(Guard.curr_seq));
  printf("[Jaffar]  + Exit Room Timer: %d\n", exit_room_timer);
  printf("[Jaffar]  + Exit Door Open: %s\n", isLevelExitDoorOpen() ? "Yes" : "No");
  printf("[Jaffar]  + Reached Checkpoint: %s\n", checkpoint ? "Yes" : "No");
  printf("[Jaffar]  + Feather Fall: %d\n", is_feather_fall);
  printf("[Jaffar]  + RNG State: 0x%08X (Last Loose Tile Sound Id: %d)\n", random_seed, last_loose_sound);

  // Level-Specific Settings
  if (current_level == 9) printf("[Jaffar]  + Rightmost Door: %d\n", level.bg[349]);
}

bool miniPoPInstance::isLevelExitDoorOpen()
{
  bool door_open = leveldoor_open;

  if (!door_open)
  {
    for (int i = 0; i < trobs_count; ++i)
    {
      const auto &trob = trobs[i];
      const auto idx = (trob.room - 1) * 30 + trob.tilepos;
      const auto type = level.fg[idx] & 0x1f;
      if (type == tiles_16_level_door_left)
      {
        door_open = true;
        break;
      }
    }
  }

  return door_open;
}

miniPoPInstance::miniPoPInstance()
{
}

miniPoPInstance::~miniPoPInstance()
{
}
