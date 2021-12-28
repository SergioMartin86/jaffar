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

  // Setting levels.dat path
  sprintf(levels_file, "LEVELS.DAT");
  const char *lvlsPath = std::getenv("SDLPOP_LEVELS_FILE");
  if (lvlsPath != NULL) sprintf(levels_file, lvlsPath);

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

  doorlink1_ad = /*&*/ gameState.level.doorlinks1;
  doorlink2_ad = /*&*/ gameState.level.doorlinks2;
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
  gameState.checkpoint = 0;
  resurrect_time = 0;
  gameState.hitp_beg_lev = custom->start_hitp;    // 3
  gameState.current_level = 0;
  startLevel(1);
  gameState.need_level1_music = custom->intro_music_time_initial;
}

void miniPoPInstance::startLevel(const word level)
{
 ///////////////////////////////////////////////////////////////
  // play_level
  if (level != gameState.current_level) load_lev_spr(level);

  load_kid_sprite();
  load_level();
  pos_guards();
  clear_coll_rooms();
  clear_saved_ctrl();

  gameState.drawn_room = 0;
  gameState.mobs_count = 0;
  gameState.trobs_count = 0;
  next_sound = -1;
  gameState.holding_sword = 0;
  gameState.grab_timer = 0;
  gameState.can_guard_see_kid = 0;
  gameState.united_with_shadow = 0;
  gameState.leveldoor_open = 0;
  gameState.demo_index = 0;
  gameState.demo_time = 0;
  gameState.guardhp_curr = 0;
  hitp_delta = 0;
  gameState.Guard.charid = charid_2_guard;
  gameState.Guard.direction = dir_56_none;
  do_startpos();

  // (level_number != 1)
  gameState.have_sword = (level == 0 || level >= custom->have_sword_from_level);

  find_start_level_door();

 if (gameState.need_level1_music != 0 && gameState.current_level == custom->intro_music_level)
   gameState.need_level1_music = custom->intro_music_time_restart;
}

void miniPoPInstance::setSeed(const dword randomSeed)
{
  gameState.random_seed = randomSeed;
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

dword miniPoPInstance::advanceRNGState(const dword randomSeed)
{
 return randomSeed * 214013 + 2531011;
}

dword miniPoPInstance::reverseRNGState(const dword randomSeed)
{
 return (randomSeed + 4292436285) * 3115528533;
}


void miniPoPInstance::advanceFrame(const uint8_t &move)
{
 // Move Ids =        0    1    2    3    4    5     6     7     8    9     10    11    12    13    14
 //_possibleMoves = {".", "S", "U", "L", "R", "D", "LU", "LD", "RU", "RD", "SR", "SL", "SU", "SD", "CA" };

 key_states[SDL_SCANCODE_UP] = 0;
 key_states[SDL_SCANCODE_DOWN] = 0;
 key_states[SDL_SCANCODE_LEFT] = 0;
 key_states[SDL_SCANCODE_RIGHT] = 0;
 key_states[SDL_SCANCODE_RSHIFT] = 0;

 if (move == 0) ;
 if (move == 1) key_states[SDL_SCANCODE_RSHIFT] = 1;
 if (move == 2) key_states[SDL_SCANCODE_UP] = 1;
 if (move == 3) key_states[SDL_SCANCODE_LEFT] = 1;
 if (move == 4) key_states[SDL_SCANCODE_RIGHT] = 1;
 if (move == 5) key_states[SDL_SCANCODE_DOWN] = 1;
 if (move == 6) { key_states[SDL_SCANCODE_LEFT] = 1; key_states[SDL_SCANCODE_UP] = 1; }
 if (move == 7) { key_states[SDL_SCANCODE_LEFT] = 1; key_states[SDL_SCANCODE_DOWN] = 1; }
 if (move == 8) { key_states[SDL_SCANCODE_RIGHT] = 1; key_states[SDL_SCANCODE_UP] = 1; }
 if (move == 9) { key_states[SDL_SCANCODE_RIGHT] = 1; key_states[SDL_SCANCODE_DOWN] = 1; }
 if (move == 10) { key_states[SDL_SCANCODE_RSHIFT] = 1; key_states[SDL_SCANCODE_RIGHT] = 1; }
 if (move == 11) { key_states[SDL_SCANCODE_RSHIFT] = 1; key_states[SDL_SCANCODE_LEFT] = 1; }
 if (move == 12) { key_states[SDL_SCANCODE_RSHIFT] = 1; key_states[SDL_SCANCODE_UP] = 1; }
 if (move == 13) { key_states[SDL_SCANCODE_RSHIFT] = 1; key_states[SDL_SCANCODE_DOWN] = 1; }
 if (move == 14) { is_restart_level = 1; }


 guardhp_delta = 0;
 hitp_delta = 0;
 timers();
 play_frame();

 if (is_restart_level == 1)
 {
  startLevel(gameState.current_level);
  draw_level_first();
 }

 // if we're on lvl 4, check mirror
 if (gameState.current_level == 4)
 {
  if (jumped_through_mirror == -1) gameState.Guard.x = 245;
  check_mirror();
 }

 // If level has changed, then load it
 if (gameState.current_level != gameState.next_level)
 {
  if (gameState.current_level == custom->copyprot_level-1 && gameState.next_level == custom->copyprot_level)
   gameState.next_level = 15;

  if (gameState.current_level == 15)
   gameState.next_level = custom->copyprot_level;

  startLevel(gameState.next_level);

  // Handle cutscenes
  //if (gameState.next_level == 2) for (size_t i = 0; i < 3; i++) gameState.random_seed = advanceRNGState(gameState.random_seed);
 }

 is_restart_level = 0;
}

int miniPoPInstance::getKidSequenceId()
{
 int seqIdx = gameState.Kid.curr_seq;
 for (int i = 0; i < 113; i++)
  if (seqIdx >= seqOffsets[i] && seqIdx < seqOffsets[i+1]) return i;
 return 153; // Unrecognized
}

int miniPoPInstance::getGuardSequenceId()
{
 int seqIdx = gameState.Guard.curr_seq;
 for (int i = 0; i < 113; i++)
  if (seqIdx >= seqOffsets[i] && seqIdx < seqOffsets[i+1])
   return i;
 return 153; // Unrecognized
}


void miniPoPInstance::printFrameInfo()
{
  printf("[Jaffar]  + Current/Next Level: %2d / %2d\n", gameState.current_level, gameState.next_level);
  printf("[Jaffar]  + [Kid]   Room: %d, Pos.x: %3d, Pos.y: %3d, Frame: %3d, HP: %d/%d\n", int(gameState.Kid.room), int(gameState.Kid.x), int(gameState.Kid.y), int(gameState.Kid.frame), int(gameState.hitp_curr), int(gameState.hitp_max));
  printf("[Jaffar]  + [Guard] Room: %d, Pos.x: %3d, Pos.y: %3d, Frame: %3d, HP: %d/%d\n", int(gameState.Guard.room), int(gameState.Guard.x), int(gameState.Guard.y), int(gameState.Guard.frame), int(gameState.guardhp_curr), int(gameState.guardhp_max));
  printf("[Jaffar]  + Exit Room Timer: %d\n", gameState.exit_room_timer);
  printf("[Jaffar]  + Reached Checkpoint: %s\n", gameState.checkpoint ? "Yes" : "No");
  printf("[Jaffar]  + Feather Fall: %d\n", gameState.is_feather_fall);
  printf("[Jaffar]  + RNG State: 0x%08X (Last Loose Tile Sound Id: %d)\n", gameState.random_seed, gameState.last_loose_sound);
}

miniPoPInstance::miniPoPInstance()
{
}

miniPoPInstance::~miniPoPInstance()
{
}
