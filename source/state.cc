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
  AddItem(&dest, pickup_obj_type, State::PER_FRAME_STATE);
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
  AddItem(&dest, is_feather_fall, State::PER_FRAME_STATE);
  AddItem(&dest, last_loose_sound, State::PER_FRAME_STATE);
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
  _hashTypeFallingTiles = NONE;

  if (isDefined(stateConfig, "Falling Tiles Hash Types") == true)
  {
   for (const auto& entry : stateConfig["Falling Tiles Hash Types"])
   {
    std::string hashType = entry["Type"].get<std::string>();
    int room = entry["Room"].get<int>();
    int column = entry["Column"].get<int>();
    auto idx = std::make_pair(room, column);
    if (hashType == "Index Only") _hashTypeMobs[idx] = INDEX_ONLY;
    if (hashType == "Full") _hashTypeMobs[idx] = FULL;
   }
  }
  else EXIT_WITH_ERROR("[Error] State Configuration 'Falling Tiles Hash Types' was not defined\n");

  if (isDefined(stateConfig, "Active Objects Hash Types") == true)
  {
   for (const auto& entry : stateConfig["Active Objects Hash Types"])
   {
    std::string hashType = entry["Type"].get<std::string>();
    int idx = entry["Index"].get<int>();
    if (hashType == "Index Only") _hashTypeTrobs[idx] = INDEX_ONLY;
    if (hashType == "Full") _hashTypeTrobs[idx] = FULL;
   }

  }
  else EXIT_WITH_ERROR("[Error] State Configuration 'Active Objects Hash Types' was not defined\n");

  if (isDefined(stateConfig, "Static Tile Hash Types") == true)
  {
   for (const auto& idx : stateConfig["Static Tile Hash Types"])  _hashTypeStatic.push_back(idx.get<int>());
  }
  else EXIT_WITH_ERROR("[Error] State Configuration 'Static Tile Hash Types' was not defined\n");

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
   const auto idx = std::make_pair(mob.room, mob.xh);
   if (_hashTypeMobs.count(idx))
   {
    const auto hashType = _hashTypeMobs.at(idx);
    if (hashType == INDEX_ONLY) { hash.Update(mob.room); hash.Update(mob.xh); }
    if (hashType == FULL)  hash.Update(mob);
   }
  }

  // Trobs are stationary animated objects. They only change in state, hence we only read BG
  for (int i = 0; i < trobs_count; ++i)
  {
    const auto &trob = trobs[i];
    const auto idx = (trob.room - 1) * 30 + trob.tilepos;

    if (_hashTypeTrobs.count(idx))
    {
     const auto hashType = _hashTypeTrobs.at(idx);
     if (hashType == INDEX_ONLY) hash.Update(idx);
     if (hashType == FULL) { hash.Update(idx); hash.Update(level.bg[idx]); }
    }
  }

  // Computing hash for static objects. They only change on tile type, hence we only read FG
  for (const auto idx : _hashTypeStatic)  hash.Update(level.fg[idx]);

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
