#include "miniPoPInstance.h"
#include "utils.h"
#include <dlfcn.h>
#include <iostream>
#include <omp.h>

const char* seqNames[] = {"running", "startrun", "runstt1", "runstt4", "runcyc1", "runcyc7", "stand", "goalertstand", "alertstand", "arise", "guardengarde", "engarde", "ready", "ready_loop", "stabbed", "strikeadv", "strikeret", "advance", "fastadvance", "retreat", "strike", "faststrike", "guy4", "guy7", "guy8", "blockedstrike", "blocktostrike", "readyblock", "blocking", "striketoblock", "landengarde", "bumpengfwd", "bumpengback", "flee", "turnengarde", "alertturn", "standjump", "sjland", "runjump", "rjlandrun", "rdiveroll", "rdiveroll_crouch", "sdiveroll", "crawl", "crawl_crouch", "turndraw", "turn", "turnrun", "runturn", "fightfall", "efightfall", "efightfallfwd", "stepfall", "fall1", "patchfall", "stepfall2", "stepfloat", "jumpfall", "rjumpfall", "jumphangMed", "jumphangLong", "jumpbackhang", "hang", "hang1", "hangstraight", "hangstraight_loop", "climbfail", "climbdown", "climbup", "hangdrop", "hangfall", "freefall", "freefall_loop", "runstop", "jumpup", "highjump", "superhijump", "fallhang", "bump", "bumpfall", "bumpfloat", "hardbump", "testfoot", "stepback", "step14", "step13", "step12", "step11", "step10", "step10a", "step9", "step8", "step7", "step6", "step5", "step4", "step3", "step2", "step1", "stoop", "stoop_crouch", "standup", "pickupsword", "resheathe", "fastsheathe", "drinkpotion", "softland", "softland_crouch", "landrun", "medland", "hardland", "hardland_dead", "stabkill", "dropdead", "dropdead_dead", "impale", "impale_dead", "halve", "halve_dead", "crush", "deadfall", "deadfall_loop", "climbstairs", "climbstairs_loop", "Vstand", "Vraise", "Vraise_loop", "Vwalk", "Vwalk1", "Vwalk2", "Vstop", "Vexit", "Pstand", "Palert", "Pstepback", "Pstepback_loop", "Plie", "Pwaiting", "Pembrace", "Pembrace_loop", "Pstroke", "Prise", "Prise_loop", "Pcrouch", "Pcrouch_loop", "Pslump", "Pslump_loop", "Mscurry", "Mscurry1", "Mstop", "Mraise", "Mleave", "Mclimb", "unrecognized" };
const word seqOffsets[] = { 0x1973, 0x1975, 0x1978, 0x1981, 0x1995, 0x19A0, 0x19A6, 0x19A8, 0x19AC, 0x19C1, 0x19C4, 0x19D2, 0x19D8, 0x19DC, 0x19F9, 0x1A07, 0x1A13, 0x1A22, 0x1A2E, 0x1A3C, 0x1A42, 0x1A45, 0x1A4A, 0x1A4D, 0x1A54, 0x1A5A, 0x1A5E, 0x1A5F, 0x1A63, 0x1A68, 0x1A6E, 0x1A75, 0x1A7C, 0x1A83, 0x1A8B, 0x1A93, 0x1AB0, 0x1ACD, 0x1AFB, 0x1B04, 0x1B16, 0x1B1A, 0x1B1B, 0x1B29, 0x1B2D, 0x1B39, 0x1B53, 0x1B5A, 0x1B85, 0x1BA1, 0x1BBF, 0x1BDB, 0x1BE4, 0x1BFA, 0x1C01, 0x1C06, 0x1C1C, 0x1C38, 0x1C54, 0x1C69, 0x1C84, 0x1CA1, 0x1CA4, 0x1CD1, 0x1CD8, 0x1CDC, 0x1CEC, 0x1D04, 0x1D25, 0x1D36, 0x1D49, 0x1D4B, 0x1D4F, 0x1D68, 0x1D7D, 0x1D9B, 0x1DF6, 0x1DFC, 0x1E06, 0x1E25, 0x1E3B, 0x1E59, 0x1E78, 0x1E7D, 0x1E9C, 0x1EBB, 0x1EDA, 0x1EF7, 0x1EFC, 0x1F13, 0x1F19, 0x1F33, 0x1F48, 0x1F5D, 0x1F72, 0x1F82, 0x1F92, 0x1F9E, 0x1FA7, 0x1FAF, 0x1FB3, 0x1FCA, 0x1FDA, 0x1FFB, 0x2009, 0x202B, 0x2036, 0x203A, 0x205A, 0x209C, 0x20A5, 0x20A9, 0x20AE, 0x20BA, 0x20BE, 0x20C5, 0x20C9, 0x20CD, 0x20D1, 0x20D4, 0x20D9, 0x20DD, 0x212E, 0x2132, 0x2136, 0x214B, 0x214F, 0x2151, 0x2154, 0x2166, 0x216D, 0x2195, 0x2199, 0x21A8, 0x21B8, 0x21BC, 0x21C0, 0x21C4, 0x21E2, 0x21E6, 0x21EA, 0x21F8, 0x21FC, 0x223C, 0x2240, 0x2241, 0x2245, 0x2247, 0x2253, 0x2257, 0x225B, 0x226E, 0x2270 };

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
  {
   startLevel(current_level);
   draw_level_first();
  }

  // if we're on lvl 4, check mirror
  if (current_level == 4)
  {
   if (jumped_through_mirror == -1) Guard.x = 245;
   check_mirror();
  }

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
}

int miniPoPInstance::getKidSequenceId()
{
 int seqIdx = Kid.curr_seq;
 for (int i = 0; i < 113; i++)
  if (seqIdx >= seqOffsets[i] && seqIdx < seqOffsets[i+1]) return i;
 return 153; // Unrecognized
}

int miniPoPInstance::getGuardSequenceId()
{
 int seqIdx = Guard.curr_seq;
 for (int i = 0; i < 113; i++)
  if (seqIdx >= seqOffsets[i] && seqIdx < seqOffsets[i+1])
   return i;
 return 153; // Unrecognized
}


void miniPoPInstance::printFrameInfo()
{
  int kidSeqIdx = getKidSequenceId();
  int guardSeqIdx = getGuardSequenceId();

  printf("[Jaffar]  + Current/Next Level: %2d / %2d\n", current_level, next_level);
  printf("[Jaffar]  + Cumulative IGT: %2lu:%02lu.%03lu\n", getElapsedMins(), getElapsedSecs(), getElapsedMilisecs());
  printf("[Jaffar]  + [Kid]   Room: %d, Pos.x: %3d, Pos.y: %3d, Frame: %3d, HP: %d/%d, Seq: %2d (%s)\n", int(Kid.room), int(Kid.x), int(Kid.y), int(Kid.frame), int(hitp_curr), int(hitp_max), kidSeqIdx, seqNames[kidSeqIdx]);
  printf("[Jaffar]  + [Guard] Room: %d, Pos.x: %3d, Pos.y: %3d, Frame: %3d, HP: %d/%d, Seq: %2d (%s)\n", int(Guard.room), int(Guard.x), int(Guard.y), int(Guard.frame), int(guardhp_curr), int(guardhp_max), guardSeqIdx, seqNames[guardSeqIdx]);
  printf("[Jaffar]  + Exit Room Timer: %d\n", exit_room_timer);
  printf("[Jaffar]  + Reached Checkpoint: %s\n", checkpoint ? "Yes" : "No");
  printf("[Jaffar]  + Feather Fall: %d\n", is_feather_fall);
  printf("[Jaffar]  + RNG State: 0x%08X (Last Loose Tile Sound Id: %d)\n", random_seed, last_loose_sound);

  // Level-Specific Settings
  if (current_level == 9) printf("[Jaffar]  + Rightmost Door: %d\n", level.bg[349]);
}

miniPoPInstance::miniPoPInstance()
{
}

miniPoPInstance::~miniPoPInstance()
{
}
