#include "state.h"
#include "frame.h"
#include "metrohash64.h"
#include "utils.h"

extern nlohmann::json _scriptJs;

char quick_control[] = "........";
float replay_curr_tick = 0.0;

template <class T>
void AddItem(std::vector<State::Item> *dest, T &val, State::ItemType type)
{
  dest->push_back({&val, sizeof(val), type});
}

std::vector<State::Item> GenerateItemsMap(SDLPopInstance *sdlPop)
{
  std::vector<State::Item> dest;
  AddItem(&dest, quick_control, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->level, State::HASHABLE_MANUAL);
  AddItem(&dest, *sdlPop->checkpoint, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->upside_down, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->drawn_room, State::HASHABLE);
  AddItem(&dest, *sdlPop->current_level, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->next_level, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->mobs_count, State::HASHABLE_MANUAL);
  AddItem(&dest, *sdlPop->mobs, State::HASHABLE_MANUAL);
  AddItem(&dest, *sdlPop->trobs_count, State::HASHABLE_MANUAL);
  AddItem(&dest, *sdlPop->trobs, State::HASHABLE_MANUAL);
  AddItem(&dest, *sdlPop->leveldoor_open, State::HASHABLE);
  AddItem(&dest, *sdlPop->Kid, State::HASHABLE);
  AddItem(&dest, *sdlPop->hitp_curr, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->hitp_max, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->hitp_beg_lev, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->grab_timer, State::HASHABLE);
  AddItem(&dest, *sdlPop->holding_sword, State::HASHABLE);
  AddItem(&dest, *sdlPop->united_with_shadow, State::HASHABLE);
  AddItem(&dest, *sdlPop->have_sword, State::HASHABLE);
  /*AddItem(&dest, *sdlPop->ctrl1_forward, State::HASHABLE);
  AddItem(&dest, *sdlPop->ctrl1_backward, State::HASHABLE);
  AddItem(&dest, *sdlPop->ctrl1_up, State::HASHABLE);
  AddItem(&dest, *sdlPop->ctrl1_down, State::HASHABLE);
  AddItem(&dest, *sdlPop->ctrl1_shift2, State::HASHABLE);*/
  AddItem(&dest, *sdlPop->kid_sword_strike, State::HASHABLE);
  AddItem(&dest, *sdlPop->pickup_obj_type, State::HASHABLE);
  AddItem(&dest, *sdlPop->offguard, State::HASHABLE);
  // guard
  AddItem(&dest, *sdlPop->Guard, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->Char, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->Opp, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->guardhp_curr, State::HASHABLE);
  AddItem(&dest, *sdlPop->guardhp_max, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->demo_index, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->demo_time, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->curr_guard_color, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->guard_notice_timer, State::HASHABLE);
  AddItem(&dest, *sdlPop->guard_skill, State::HASHABLE);
  AddItem(&dest, *sdlPop->shadow_initialized, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->guard_refrac, State::HASHABLE);
  AddItem(&dest, *sdlPop->justblocked, State::HASHABLE);
  AddItem(&dest, *sdlPop->droppedout, State::HASHABLE);
  // collision
  AddItem(&dest, *sdlPop->curr_row_coll_room, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->curr_row_coll_flags, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->below_row_coll_room, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->below_row_coll_flags, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->above_row_coll_room, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->above_row_coll_flags, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->prev_collision_row, State::PER_FRAME_STATE);
  // flash
  AddItem(&dest, *sdlPop->flash_color, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->flash_time, State::PER_FRAME_STATE);
  // sounds
  AddItem(&dest, *sdlPop->need_level1_music, State::HASHABLE);
  AddItem(&dest, *sdlPop->is_screaming, State::HASHABLE);
  AddItem(&dest, *sdlPop->is_feather_fall, State::HASHABLE);
  AddItem(&dest, *sdlPop->last_loose_sound, State::HASHABLE);
  // AddItem(&dest, *sdlPop->next_sound, State::HASHABLE);
  // AddItem(&dest, *sdlPop->current_sound, State::HASHABLE);
  // random
  AddItem(&dest, *sdlPop->random_seed, State::PER_FRAME_STATE);
  // remaining time
  AddItem(&dest, *sdlPop->rem_min, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->rem_tick, State::PER_FRAME_STATE);
  // saved controls
  AddItem(&dest, *sdlPop->control_x, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->control_y, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->control_shift, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->control_forward, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->control_backward, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->control_up, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->control_down, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->control_shift2, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->ctrl1_forward, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->ctrl1_backward, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->ctrl1_up, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->ctrl1_down, State::PER_FRAME_STATE);
  AddItem(&dest, *sdlPop->ctrl1_shift2, State::PER_FRAME_STATE);
  // Support for overflow glitch
  AddItem(&dest, *sdlPop->exit_room_timer, State::PER_FRAME_STATE);
  // replay recording state
  AddItem(&dest, replay_curr_tick, State::PER_FRAME_STATE);
  return dest;
}

dword State::advanceRNGState(const dword randomSeed)
{
 return randomSeed * 214013 + 2531011;
}

dword State::reverseRNGState(const dword randomSeed)
{
 return (randomSeed + 4292436285) * 3115528533;
}

State::State(SDLPopInstance *sdlPop, nlohmann::json stateConfig)
{
  _sdlPop = sdlPop;
  _items = GenerateItemsMap(sdlPop);

  // Loading save file
  if (isDefined(stateConfig, "Path") == false) EXIT_WITH_ERROR("[ERROR] State configuration missing 'Path' key.\n");
  const std::string saveFile = stateConfig["Path"].get<std::string>();
  std::string saveString;
  bool status = loadStringFromFile(saveString, saveFile.c_str());
  if (status == false) EXIT_WITH_ERROR("[ERROR] Could not load save state %s\n", saveFile.c_str());
  loadState(saveString);

  // Parsing random seed information
  if (isDefined(stateConfig, "Random Seed") == false) EXIT_WITH_ERROR("[ERROR] State configuration missing 'Random Seed' key.\n");
  dword configSeed = stateConfig["Random Seed"].get<dword>();

  // Parsing last loose tile sound
  if (isDefined(stateConfig, "Last Loose Tile Sound") == false) EXIT_WITH_ERROR("[ERROR] State configuration missing 'Last Loose Tile Sound' key.\n");
  const word configLooseTileSound = stateConfig["Last Loose Tile Sound"].get<dword>();

  // Overriding RNG state if value > 0 was passed
  if (configSeed > 0)
  {
   // Processing screen objects that might affect RNG state
   _sdlPop->play_frame();

   // Reload State
   loadState(saveString);

   // Set Correct pre-seed
   _sdlPop->setSeed(configSeed);
  }

  // Setting value of last loose tile sound, overriding if value > 0 was passed
  if (configLooseTileSound != 0) *_sdlPop->last_loose_sound = configLooseTileSound;
}

uint64_t State::kidHash() const
{
  uint64_t hash;
  MetroHash64::Hash(reinterpret_cast<uint8_t *>(_sdlPop->Kid), sizeof(*_sdlPop->Kid), reinterpret_cast<uint8_t *>(&hash), 1);
  return hash;
}

uint64_t State::computeHash() const
{
  MetroHash64 hash;
  for (const auto &item : _items)
  {
    if (item.type == HASHABLE)
    {
      hash.Update(item.ptr, item.size);
    }
  }

  // manual hashes
  // hash.Update(_sdlPop->level->fg);
  for (const uint8_t x : _sdlPop->level->fg)
  {
    hash.Update(uint8_t(x & 0x1f));
  }

  hash.Update(_sdlPop->level->guards_x);
  hash.Update(_sdlPop->level->guards_dir);
  hash.Update(*_sdlPop->mobs, sizeof(mob_type) * (*_sdlPop->mobs_count));
  if (_sdlPop->Guard->alive) hash.Update(*_sdlPop->Guard);

  for (int i = 0; i < *_sdlPop->trobs_count; ++i)
  {
    const auto &trob = (*_sdlPop->trobs)[i];
    const auto idx = (trob.room - 1) * 30 + trob.tilepos;
    const auto type = _sdlPop->level->fg[idx] & 0x1f;
    switch (type)
    {
    case tiles_0_empty:
    case tiles_1_floor:
    case tiles_3_pillar:
    case tiles_5_stuck:
    case tiles_6_closer:
    case tiles_7_doortop_with_floor:
    case tiles_8_bigpillar_bottom:
    case tiles_9_bigpillar_top:
    case tiles_10_potion:
    case tiles_12_doortop:
    case tiles_13_mirror:
    case tiles_14_debris:
    case tiles_15_opener:
    case tiles_17_level_door_right:
    case tiles_19_torch:
    case tiles_20_wall:
    case tiles_21_skeleton:
    case tiles_22_sword:
    case tiles_23_balcony_left:
    case tiles_24_balcony_right:
    case tiles_25_lattice_pillar:
    case tiles_26_lattice_down:
    case tiles_27_lattice_small:
    case tiles_28_lattice_left:
    case tiles_29_lattice_right:
    case tiles_30_torch_with_debris:
      break;
    case tiles_11_loose:
    case tiles_16_level_door_left:
    case tiles_18_chomper:
    case tiles_2_spike:
    case tiles_4_gate:
      hash.Update(trob);
      hash.Update(_sdlPop->level->bg[idx]);
      hash.Update(_sdlPop->level->fg[idx]);
      break;
    default:
      EXIT_WITH_ERROR("Unknown trob type: %d\n", int(type));
    }
  }

  uint64_t result;
  hash.Finalize(reinterpret_cast<uint8_t *>(&result));
  return result;
}

void State::loadState(const std::string &data)
{
  if (data.size() != _FRAME_DATA_SIZE)
    EXIT_WITH_ERROR("[Error] Wrong state size. Expected %lu, got: %lu\n", _FRAME_DATA_SIZE, data.size());

  size_t curPos = 0;
  for (const auto &item : _items)
  {
    memcpy(item.ptr, &data.c_str()[curPos], item.size);
    curPos += item.size;
  }
}

std::string State::saveState() const
{
  std::string res;
  res.reserve(_FRAME_DATA_SIZE);
  for (const auto &item : _items)
  {
    res.append(reinterpret_cast<const char *>(item.ptr), item.size);
  }

  if (res.size() != _FRAME_DATA_SIZE)
    EXIT_WITH_ERROR("[Error] Wrong state size. Expected %lu, got: %lu\n", _FRAME_DATA_SIZE, res.size());

  return res;
}
