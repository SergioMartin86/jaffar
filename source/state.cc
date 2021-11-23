#include "state.h"
#include "frame.h"
#include "metrohash64.h"
#include "utils.h"

extern nlohmann::json _scriptJs;

size_t _currentStep;
char quick_control[] = "........";
float replay_curr_tick = 0.0;

template <class T>
void AddItem(std::vector<State::Item> *dest, T &val, State::ItemType type)
{
  dest->push_back({&val, sizeof(val), type});
}

std::vector<State::Item> GenerateItemsMap(miniPoPInstance *miniPop)
{
  std::vector<State::Item> dest;
  AddItem(&dest, quick_control, State::PER_FRAME_STATE);
  AddItem(&dest, level, State::HASHABLE_MANUAL);
  AddItem(&dest, checkpoint, State::PER_FRAME_STATE);
  AddItem(&dest, upside_down, State::PER_FRAME_STATE);
  AddItem(&dest, drawn_room, State::HASHABLE);
  AddItem(&dest, current_level, State::PER_FRAME_STATE);
  AddItem(&dest, next_level, State::PER_FRAME_STATE);
  AddItem(&dest, mobs_count, State::HASHABLE_MANUAL);
  AddItem(&dest, mobs, State::HASHABLE_MANUAL);
  AddItem(&dest, trobs_count, State::HASHABLE_MANUAL);
  AddItem(&dest, trobs, State::HASHABLE_MANUAL);
  AddItem(&dest, leveldoor_open, State::HASHABLE);
  AddItem(&dest, Kid, State::HASHABLE);
  AddItem(&dest, hitp_curr, State::PER_FRAME_STATE);
  AddItem(&dest, hitp_max, State::PER_FRAME_STATE);
  AddItem(&dest, hitp_beg_lev, State::PER_FRAME_STATE);
  AddItem(&dest, grab_timer, State::HASHABLE);
  AddItem(&dest, holding_sword, State::HASHABLE);
  AddItem(&dest, united_with_shadow, State::HASHABLE);
  AddItem(&dest, have_sword, State::HASHABLE);
  /*AddItem(&dest, ctrl1_forward, State::HASHABLE);
  AddItem(&dest, ctrl1_backward, State::HASHABLE);
  AddItem(&dest, ctrl1_up, State::HASHABLE);
  AddItem(&dest, ctrl1_down, State::HASHABLE);
  AddItem(&dest, ctrl1_shift2, State::HASHABLE);*/
  AddItem(&dest, kid_sword_strike, State::HASHABLE);
  AddItem(&dest, pickup_obj_type, State::HASHABLE);
  AddItem(&dest, offguard, State::HASHABLE);
  // guard
  AddItem(&dest, Guard, State::PER_FRAME_STATE);
  AddItem(&dest, Char, State::PER_FRAME_STATE);
  AddItem(&dest, Opp, State::PER_FRAME_STATE);
  AddItem(&dest, guardhp_curr, State::PER_FRAME_STATE);
  AddItem(&dest, guardhp_max, State::PER_FRAME_STATE);
  AddItem(&dest, demo_index, State::PER_FRAME_STATE);
  AddItem(&dest, demo_time, State::PER_FRAME_STATE);
  AddItem(&dest, curr_guard_color, State::PER_FRAME_STATE);
  AddItem(&dest, guard_notice_timer, State::HASHABLE);
  AddItem(&dest, guard_skill, State::PER_FRAME_STATE);
  AddItem(&dest, shadow_initialized, State::PER_FRAME_STATE);
  AddItem(&dest, guard_refrac, State::HASHABLE);
  AddItem(&dest, justblocked, State::HASHABLE);
  AddItem(&dest, droppedout, State::HASHABLE);
  // collision
  AddItem(&dest, curr_row_coll_room, State::PER_FRAME_STATE);
  AddItem(&dest, curr_row_coll_flags, State::PER_FRAME_STATE);
  AddItem(&dest, below_row_coll_room, State::PER_FRAME_STATE);
  AddItem(&dest, below_row_coll_flags, State::PER_FRAME_STATE);
  AddItem(&dest, above_row_coll_room, State::PER_FRAME_STATE);
  AddItem(&dest, above_row_coll_flags, State::PER_FRAME_STATE);
  AddItem(&dest, prev_collision_row, State::PER_FRAME_STATE);
  // flash
  AddItem(&dest, flash_color, State::PER_FRAME_STATE);
  AddItem(&dest, flash_time, State::PER_FRAME_STATE);
  // sounds
  AddItem(&dest, need_level1_music, State::HASHABLE);
  AddItem(&dest, is_screaming, State::HASHABLE);
  AddItem(&dest, is_feather_fall, State::HASHABLE);
  AddItem(&dest, last_loose_sound, State::HASHABLE);
  // AddItem(&dest, next_sound, State::HASHABLE);
  // AddItem(&dest, current_sound, State::HASHABLE);
  // random
  AddItem(&dest, random_seed, State::PER_FRAME_STATE);
  // remaining time
  AddItem(&dest, rem_min, State::PER_FRAME_STATE);
  AddItem(&dest, rem_tick, State::PER_FRAME_STATE);
  // saved controls
  AddItem(&dest, control_x, State::PER_FRAME_STATE);
  AddItem(&dest, control_y, State::PER_FRAME_STATE);
  AddItem(&dest, control_shift, State::PER_FRAME_STATE);
  AddItem(&dest, control_forward, State::PER_FRAME_STATE);
  AddItem(&dest, control_backward, State::PER_FRAME_STATE);
  AddItem(&dest, control_up, State::PER_FRAME_STATE);
  AddItem(&dest, control_down, State::PER_FRAME_STATE);
  AddItem(&dest, control_shift2, State::PER_FRAME_STATE);
  AddItem(&dest, ctrl1_forward, State::PER_FRAME_STATE);
  AddItem(&dest, ctrl1_backward, State::PER_FRAME_STATE);
  AddItem(&dest, ctrl1_up, State::PER_FRAME_STATE);
  AddItem(&dest, ctrl1_down, State::PER_FRAME_STATE);
  AddItem(&dest, ctrl1_shift2, State::PER_FRAME_STATE);
  // Support for overflow glitch
  AddItem(&dest, exit_room_timer, State::PER_FRAME_STATE);
  // replay recording state
  AddItem(&dest, replay_curr_tick, State::PER_FRAME_STATE);
  return dest;
}

State::State(miniPoPInstance *miniPop, const std::string& saveString, const nlohmann::json stateConfig)
{
  _miniPop = miniPop;
  _items = GenerateItemsMap(miniPop);

  // Setting hash types
  _hashTypeFallingTile = NONE;
  _hashTypeGate = NONE;
  _hashTypeSpike = NONE;
  _hashTypeLooseTile = NONE;
  _hashTypeExitDoor = NONE;
  _hashTypeChomper = NONE;

  if (isDefined(stateConfig, "Falling Tile Hash Type") == true)
  {
   std::string hashType = stateConfig["Falling Tile Hash Type"].get<std::string>();
   if (hashType == "Index Only") _hashTypeFallingTile = INDEX_ONLY;
   if (hashType == "Full") _hashTypeFallingTile = FULL;
  }
  else EXIT_WITH_ERROR("[Error] State Configuration 'Falling Tile Hash Type' was not defined\n");

  if (isDefined(stateConfig, "Gate Hash Type") == true)
  {
   std::string hashType = stateConfig["Gate Hash Type"].get<std::string>();
   if (hashType == "Index Only") _hashTypeGate = INDEX_ONLY;
   if (hashType == "Full") _hashTypeGate = FULL;
  }
  else EXIT_WITH_ERROR("[Error] State Configuration 'Gate Hash Type' was not defined\n");

  if (isDefined(stateConfig, "Spike Hash Type") == true)
  {
   std::string hashType = stateConfig["Spike Hash Type"].get<std::string>();
   if (hashType == "Index Only") _hashTypeSpike = INDEX_ONLY;
   if (hashType == "Full") _hashTypeSpike = FULL;
  }
  else EXIT_WITH_ERROR("[Error] State Configuration 'Spike Hash Type' was not defined\n");

  if (isDefined(stateConfig, "Loose Tile Hash Type") == true)
  {
   std::string hashType = stateConfig["Loose Tile Hash Type"].get<std::string>();
   if (hashType == "Index Only") _hashTypeLooseTile = INDEX_ONLY;
   if (hashType == "Full") _hashTypeLooseTile = FULL;
  }
  else EXIT_WITH_ERROR("[Error] State Configuration 'Loose Tile Hash Type' was not defined\n");

  if (isDefined(stateConfig, "Exit Door Hash Type") == true)
  {
   std::string hashType = stateConfig["Exit Door Hash Type"].get<std::string>();
   if (hashType == "Index Only") _hashTypeExitDoor = INDEX_ONLY;
   if (hashType == "Full") _hashTypeExitDoor = FULL;
  }
  else EXIT_WITH_ERROR("[Error] State Configuration 'Exit Door Hash Type' was not defined\n");

  if (isDefined(stateConfig, "Chomper Hash Type") == true)
  {
   std::string hashType = stateConfig["Chomper Hash Type"].get<std::string>();
   if (hashType == "Index Only") _hashTypeChomper = INDEX_ONLY;
   if (hashType == "Full") _hashTypeChomper = FULL;
  }
  else EXIT_WITH_ERROR("[Error] State Configuration 'Chomper Hash Type' was not defined\n");

  // Update the SDLPop instance with the savefile contents
  memcpy(_stateData, saveString.data(), _FRAME_DATA_SIZE);
  pushState();
  _miniPop->startLevel(next_level);

  // Backing up current RNG state
  auto rngState = random_seed;
  auto looseTileSound = last_loose_sound;

  // Processing screen objects that might affect RNG state
  play_frame();

  // Update the SDLPop instance with the savefile contents again
  memcpy(_stateData, saveString.data(), _FRAME_DATA_SIZE);
  pushState();

  // Recover original RNG state
  _miniPop->setSeed(rngState);
  last_loose_sound = looseTileSound;
}

uint64_t State::computeHash() const
{
  // Storage for hash calculation
  MetroHash64 hash;

  // For items that are automatically hashable, do that now
  for (const auto &item : _items) if (item.type == HASHABLE) hash.Update(item.ptr, item.size);

  // Manual hashing
  hash.Update(level.guards_x);
  hash.Update(level.guards_dir);
  if (Guard.alive) hash.Update(Guard);

  // Mobs are moving objects (falling tiles only afaik).
  for (int i = 0; i < mobs_count; i++)
  {
   const auto &mob = mobs[i];
   if (_hashTypeFallingTile == INDEX_ONLY) { hash.Update(mob.room); hash.Update(mob.xh); }
   if (_hashTypeFallingTile == FULL) hash.Update(mob);
  }

  // Trobs are stationary animated objects.
  for (int i = 0; i < trobs_count; ++i)
  {
    const auto &trob = trobs[i];
    const auto idx = (trob.room - 1) * 30 + trob.tilepos;
    const auto type = level.fg[idx] & 0x1f;
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
    case tiles_13_mirror:
      break;
     // For loose tiles, gates, and spikes, we only care that they have been disturbed/started (not the specific state)
    case tiles_4_gate:
     if (_hashTypeGate == INDEX_ONLY) hash.Update(idx);
     if (_hashTypeGate == FULL) { hash.Update(trob); hash.Update(level.bg[idx]); hash.Update(level.fg[idx]); }
     break;
    case tiles_2_spike:
     if (_hashTypeSpike == INDEX_ONLY) hash.Update(idx);
     if (_hashTypeSpike == FULL) { hash.Update(trob); hash.Update(level.bg[idx]); hash.Update(level.fg[idx]); }
     break;
    case tiles_11_loose:
     if (_hashTypeLooseTile == INDEX_ONLY) hash.Update(idx);
     if (_hashTypeLooseTile == FULL) { hash.Update(trob); hash.Update(level.bg[idx]); hash.Update(level.fg[idx]); }
     break;
    case tiles_16_level_door_left:
     if (_hashTypeExitDoor == INDEX_ONLY) hash.Update(idx);
     if (_hashTypeExitDoor == FULL) { hash.Update(trob); hash.Update(level.bg[idx]); hash.Update(level.fg[idx]); }
     break;
    case tiles_18_chomper:
     if (_hashTypeChomper == INDEX_ONLY) hash.Update(idx);
     if (_hashTypeChomper == FULL) { hash.Update(trob); hash.Update(level.bg[idx]); hash.Update(level.fg[idx]); }
     break;
    default:
      EXIT_WITH_ERROR("Unknown trob type: %d\n", int(type));
    }
  }

  uint64_t result;
  hash.Finalize(reinterpret_cast<uint8_t *>(&result));
  return result;
}

void State::pushState()
{
  size_t pos = 0;
  for (const auto &item : _items) { memcpy(item.ptr, &_stateData[pos],item.size); pos += item.size; }
  _miniPop->isExitDoorOpen = _miniPop->isLevelExitDoorOpen();

  different_room = 1;
  // Show the room where the prince is, even if the player moved the view away
  // from it (with the H,J,U,N keys).
  next_room = drawn_room = Kid.room;
  load_room_links();
}

void State::getState()
{
  size_t pos = 0;
  for (const auto &item : _items) { memcpy(&_stateData[pos], item.ptr, item.size); pos += item.size; }
}
