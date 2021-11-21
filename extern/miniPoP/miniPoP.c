/*
MiniPop, a barebones thread-safe version of SDLPop for routing
Copyright (C) 2021 Sergio Martin

based on

SDLPoP, a port/conversion of the DOS game Prince of Persia.
Copyright (C) 2013-2020  Dávid Nagy

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

The authors of this program may be contacted at https://forum.princed.org
*/


#include "common.h"
#include "const.h"
#include <math.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#define get_frame(frame_table, frame) get_frame_internal(frame_table, frame, #frame_table, COUNT(frame_table))

__thread word copyprot_room[] = {3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4};
__thread byte sound_interruptible[] = {
                                                   0, // sound_0_fell_to_death
                                                   1, // sound_1_falling
                                                   1, // sound_2_tile_crashing
                                                   1, // sound_3_button_pressed
                                                   1, // sound_4_gate_closing
                                                   1, // sound_5_gate_opening
                                                   0, // sound_6_gate_closing_fast
                                                   1, // sound_7_gate_stop
                                                   1, // sound_8_bumped
                                                   1, // sound_9_grab
                                                   1, // sound_10_sword_vs_sword
                                                   1, // sound_11_sword_moving
                                                   1, // sound_12_guard_hurt
                                                   1, // sound_13_kid_hurt
                                                   0, // sound_14_leveldoor_closing
                                                   0, // sound_15_leveldoor_sliding
                                                   1, // sound_16_medium_land
                                                   1, // sound_17_soft_land
                                                   0, // sound_18_drink
                                                   1, // sound_19_draw_sword
                                                   1, // sound_20_loose_shake_1
                                                   1, // sound_21_loose_shake_2
                                                   1, // sound_22_loose_shake_3
                                                   1, // sound_23_footstep
                                                   0, // sound_24_death_regular
                                                   0, // sound_25_presentation
                                                   0, // sound_26_embrace
                                                   0, // sound_27_cutscene_2_4_6_12
                                                   0, // sound_28_death_in_fight
                                                   1, // sound_29_meet_Jaffar
                                                   0, // sound_30_big_potion
                                                   0, // sound_31
                                                   0, // sound_32_shadow_music
                                                   0, // sound_33_small_potion
                                                   0, // sound_34
                                                   0, // sound_35_cutscene_8_9
                                                   0, // sound_36_out_of_time
                                                   0, // sound_37_victory
                                                   0, // sound_38_blink
                                                   0, // sound_39_low_weight
                                                   0, // sound_40_cutscene_12_short_time
                                                   0, // sound_41_end_level_music
                                                   0, // sound_42
                                                   0, // sound_43_victory_Jaffar
                                                   0, // sound_44_skel_alive
                                                   0, // sound_45_jump_through_mirror
                                                   0, // sound_46_chomped
                                                   1, // sound_47_chomper
                                                   0, // sound_48_spiked
                                                   0, // sound_49_spikes
                                                   0, // sound_50_story_2_princess
                                                   0, // sound_51_princess_door_opening
                                                   0, // sound_52_story_4_Jaffar_leaves
                                                   0, // sound_53_story_3_Jaffar_comes
                                                   0, // sound_54_intro_music
                                                   0, // sound_55_story_1_absence
                                                   0, // sound_56_ending_music
                                                   0};


__thread custom_options_type *custom = &custom_defaults;
__thread byte is_validate_mode;
__thread word text_time_remaining;
__thread word text_time_total;
__thread word is_show_time;
__thread word checkpoint;
__thread word upside_down;
__thread word resurrect_time;
__thread word dont_reset_time;
__thread short rem_min;
__thread word rem_tick;
__thread word hitp_beg_lev;
__thread word need_level1_music;
__thread byte sound_flags = 0;
__thread word draw_mode;
__thread short start_level = -1;
__thread byte *guard_palettes;
__thread chtab_type *chtab_addrs[10];
__thread word copyprot_plac;
__thread word copyprot_idx;
__thread word cplevel_entr[14];
__thread dialog_type *copyprot_dialog;
__thread rect_type dialog_rect_1 = {60, 56, 124, 264};
__thread rect_type dialog_rect_2 = {61, 56, 120, 264};
__thread word drawn_room;
__thread byte curr_tile;
__thread byte curr_modifier;
__thread tile_and_mod leftroom_[3];
__thread tile_and_mod row_below_left_[10];
__thread word loaded_room;
__thread byte *curr_room_tiles;
__thread byte *curr_room_modif;
__thread word draw_xh;
__thread word current_level = -1;
__thread byte graphics_mode = 0;
__thread word room_L;
__thread word room_R;
__thread word room_A;
__thread word room_B;
__thread word room_BR;
__thread word room_BL;
__thread word room_AR;
__thread word room_AL;
__thread level_type level;
__thread short table_counts[5];
__thread short drects_count;
__thread short peels_count;
__thread back_table_type foretable[200];
__thread back_table_type backtable[200];
__thread midtable_type midtable[50];
__thread peel_type *peels_table[50];
__thread rect_type drects[30];
__thread sbyte obj_direction;
__thread short obj_clip_left;
__thread short obj_clip_top;
__thread short obj_clip_right;
__thread short obj_clip_bottom;
__thread wipetable_type wipetable[300];
__thread word need_drects;
__thread word leveldoor_right;
__thread word leveldoor_ybottom;
__thread byte palace_wall_colors[44 * 3];
__thread word seed_was_init = 0;
__thread dword random_seed;
__thread byte *doorlink2_ad;
__thread byte *doorlink1_ad;
__thread sbyte control_shift;
__thread sbyte control_y;
__thread sbyte control_x;
__thread char_type Kid;
__thread word is_keyboard_mode = 0;
__thread word is_paused;
__thread word is_restart_level;
__thread byte sound_mode = 0;
__thread byte is_sound_on = 0x0F;
__thread word next_level;
__thread short guardhp_delta;
__thread word guardhp_curr;
__thread word next_room;
__thread word hitp_curr;
__thread word hitp_max;
__thread short hitp_delta;
__thread word flash_color;
__thread word flash_time;
__thread char_type Guard;
__thread word need_quotes;
__thread short roomleave_result;
__thread word different_room;
__thread sound_buffer_type *sound_pointers[58];
__thread word guardhp_max;
__thread word is_feather_fall;
__thread chtab_type *chtab_title40;
__thread chtab_type *chtab_title50;
__thread short hof_count;
__thread word demo_mode = 0;
__thread short mobs_count;
__thread short trobs_count;
__thread short next_sound;
__thread word grab_timer;
__thread short can_guard_see_kid;
__thread word holding_sword;
__thread short united_with_shadow;
__thread word leveldoor_open;
__thread word demo_index;
__thread short demo_time;
__thread word have_sword;
__thread char_type Char;
__thread char_type Opp;
__thread short knock;
__thread word is_guard_notice;
__thread byte wipe_frames[30];
__thread sbyte wipe_heights[30];
__thread byte redraw_frames_anim[30];
__thread byte redraw_frames2[30];
__thread byte redraw_frames_floor_overlay[30];
__thread byte redraw_frames_full[30];
__thread byte redraw_frames_fore[30];
__thread byte tile_object_redraw[30];
__thread byte redraw_frames_above[10];
__thread word need_full_redraw;
__thread short n_curr_objs;
__thread objtable_type objtable[50];
__thread short curr_objs[50];
__thread byte obj_xh;
__thread byte obj_xl;
__thread byte obj_y;
__thread byte obj_chtab;
__thread byte obj_id;
__thread byte obj_tilepos;
__thread short obj_x;
__thread frame_type cur_frame;
__thread word seamless;
__thread trob_type trob;
__thread trob_type trobs[30];
__thread short redraw_height;
__thread byte curr_tilepos;
__thread short curr_room;
__thread mob_type curmob;
__thread mob_type mobs[14];
__thread short tile_col;
__thread word curr_guard_color;
__thread byte key_states[SDL_NUM_SCANCODES];
__thread word is_screaming;
__thread word offguard; // name from Apple II source
__thread word droppedout; // name from Apple II source
__thread word exit_room_timer;
__thread short char_col_right;
__thread short char_col_left;
__thread short char_top_row;
__thread short prev_char_top_row;
__thread short prev_char_col_right;
__thread short prev_char_col_left;
__thread short char_bottom_row;
__thread short guard_notice_timer;
__thread short jumped_through_mirror;
__thread byte curr_tile2;
__thread short tile_row;
__thread word char_width_half;
__thread word char_height;
__thread short char_x_left;
__thread short char_x_left_coll;
__thread short char_x_right_coll;
__thread short char_x_right;
__thread short char_top_y;
__thread byte fall_frame;
__thread byte through_tile;
__thread sbyte infrontx; // name from Apple II source
__thread word current_sound;
__thread sbyte control_shift2;
__thread sbyte control_forward;
__thread word guard_skill;
__thread sbyte control_backward;
__thread sbyte control_up;
__thread sbyte control_down;
__thread sbyte ctrl1_forward;
__thread sbyte ctrl1_backward;
__thread sbyte ctrl1_up;
__thread sbyte ctrl1_down;
__thread sbyte ctrl1_shift2;
__thread word shadow_initialized;
__thread word guard_refrac;
__thread word kid_sword_strike;
__thread byte edge_type;
__thread char **sound_names;
__thread int g_argc;
__thread sbyte collision_row;
__thread sbyte prev_collision_row;
__thread sbyte prev_coll_room[10];
__thread sbyte curr_row_coll_room[10];
__thread sbyte below_row_coll_room[10];
__thread sbyte above_row_coll_room[10];
__thread byte curr_row_coll_flags[10];
__thread byte above_row_coll_flags[10];
__thread byte below_row_coll_flags[10];
__thread byte prev_coll_flags[10];
__thread short pickup_obj_type;
__thread word justblocked; // name from Apple II source
__thread word last_loose_sound;
__thread int last_key_scancode;
__thread word curmob_index;
__thread dat_type *dathandle;
__thread word need_redraw_because_flipped;
__thread byte *level_var_palettes;
__thread word first_start = 1;
__thread sbyte distance_mirror;
__thread Uint64 last_transition_counter;
__thread sbyte bump_col_left_of_wall;
__thread sbyte bump_col_right_of_wall;
__thread sbyte right_checked_col;
__thread sbyte left_checked_col;
__thread short coll_tile_left_xpos;
__thread word curr_tile_temp;
__thread long int _cachedFilePointerTable[MAX_CACHED_FILES];
__thread char *_cachedFileBufferTable[MAX_CACHED_FILES];
__thread size_t _cachedFileBufferSizes[MAX_CACHED_FILES];
__thread char _cachedFilePathTable[MAX_CACHED_FILES][POP_MAX_PATH];
__thread size_t _cachedFileCounter = 0;
__thread char exe_dir[POP_MAX_PATH] = ".";
__thread bool found_exe_dir = false;
__thread word which_quote;
__thread dat_type *dat_chain_ptr = NULL;
__thread char last_text_input;
__thread short drawn_row;
__thread short draw_bottom_y;
__thread short draw_main_y;
__thread short drawn_col;
__thread byte tile_left;
__thread byte modifier_left;

FILE *fcache_open(const char *filename, const char *mode)
{
  // Check if already cached, and return it directly from cache if it is
  for (size_t i = 0; i < _cachedFileCounter; i++)
    if (strcmp(filename, _cachedFilePathTable[i]) == 0)
    {
      _cachedFilePointerTable[i] = 0; // Rewinding
      return (FILE *)i;
    }

  // Checking if file is directory. Do not open in that case.
  struct stat path_stat;
  stat(filename, &path_stat);
  if (S_ISREG(path_stat.st_mode) == 0)
    return NULL;

  // Open source file
  FILE *srcFile = fopen(filename, "rb");

  // If failed to load, return immediately
  if (srcFile == NULL)
    return NULL;

  // Finding out file's size
  fseek(srcFile, 0, SEEK_END);
  long int fileSize = ftell(srcFile);
  fseek(srcFile, 0, SEEK_SET);

  // Creating cached file buffer
  _cachedFileBufferTable[_cachedFileCounter] = malloc(fileSize);

  // Reading source file info buffer
  fread(_cachedFileBufferTable[_cachedFileCounter], 1, fileSize, srcFile);

  // Closing source file
  fclose(srcFile);

  // Add filepath to the cache table
  strcpy(_cachedFilePathTable[_cachedFileCounter], filename);

  // Storing file size
  _cachedFileBufferSizes[_cachedFileCounter] = fileSize;

  // Resetting pointer
  _cachedFilePointerTable[_cachedFileCounter] = 0;

  // Setting found ptr
  FILE *foundPtr = (FILE *)_cachedFileCounter;

  // Increase cached file counter
  _cachedFileCounter++;

  return foundPtr;
}

size_t fcache_read(void *ptr, size_t size, size_t count, FILE *stream)
{
  size_t fileId = (size_t)stream;
  size_t readCount = _cachedFileBufferSizes[fileId] / size;
  if (readCount > count)
    readCount = count;

  // Copying data from cached file to pointer
  memcpy(ptr, &_cachedFileBufferTable[fileId][_cachedFilePointerTable[fileId]], readCount * size);

  // Advancing file pointer
  _cachedFilePointerTable[fileId] += readCount * size;

  // Returning number of elements read
  return readCount;
}

int fcache_seek(FILE *stream, long int offset, int origin)
{
  long int base;
  size_t fileId = (size_t)stream;
  if (origin == SEEK_SET)
    base = 0;
  if (origin == SEEK_CUR)
    base = _cachedFilePointerTable[fileId];
  if (origin == SEEK_END)
    base = _cachedFileBufferSizes[fileId] - 1;

  long int dest = base + offset;
  if (dest > _cachedFileBufferSizes[fileId] - 1)
    return -1;
  if (dest < 0)
    return -1;

  _cachedFilePointerTable[fileId] = base + offset;

  return 0;
}

int fcache_tell(FILE *stream)
{
  size_t fileId = (size_t)stream;
  return _cachedFilePointerTable[fileId] + 1;
}

int fcache_close(FILE *file)
{
  // Do not actually close it. This will be done at the end
  return 0;
}


void find_exe_dir()
{
  if (found_exe_dir)
    return;
  snprintf_check(exe_dir, sizeof(exe_dir), "%s", g_argv[0]);
  char *last_slash = NULL;
  char *pos = exe_dir;
  for (char c = *pos; c != '\0'; c = *(++pos))
  {
    if (c == '/' || c == '\\')
    {
      last_slash = pos;
    }
  }
  if (last_slash != NULL)
  {
    *last_slash = '\0';
  }
  found_exe_dir = true;
}

bool file_exists(const char *filename)
{
  return (access(filename, F_OK) != -1);
}

const char *locate_file_(const char *filename, char *path_buffer, int buffer_size)
{
  if (file_exists(filename))
  {
    return filename;
  }
  else
  {
    // If failed, it may be that SDLPoP is being run from the wrong different working directory.
    // We can try to rescue the situation by loading from the directory of the executable.
    find_exe_dir();
    snprintf_check(path_buffer, buffer_size, "%s/%s", exe_dir, filename);
    return (const char *)path_buffer;
  }
}

directory_listing_type *create_directory_listing_and_find_first_file(const char *directory, const char *extension)
{
  directory_listing_type *data = calloc(1, sizeof(directory_listing_type));
  bool ok = false;
  data->dp = opendir(directory);
  if (data->dp != NULL)
  {
    struct dirent *ep;
    while ((ep = readdir(data->dp)))
    {
      char *ext = strrchr(ep->d_name, '.');
      if (ext != NULL && strcasecmp(ext + 1, extension) == 0)
      {
        data->found_filename = ep->d_name;
        data->extension = extension;
        ok = true;
        break;
      }
    }
  }
  if (ok)
  {
    return data;
  }
  else
  {
    free(data);
    return NULL;
  }
}

char *get_current_filename_from_directory_listing(directory_listing_type *data)
{
  return data->found_filename;
}

bool find_next_file(directory_listing_type *data)
{
  bool ok = false;
  struct dirent *ep;
  while ((ep = readdir(data->dp)))
  {
    char *ext = strrchr(ep->d_name, '.');
    if (ext != NULL && strcasecmp(ext + 1, data->extension) == 0)
    {
      data->found_filename = ep->d_name;
      ok = true;
      break;
    }
  }
  return ok;
}

void close_directory_listing(directory_listing_type *data)
{
  closedir(data->dp);
  free(data);
}

// seg009:000D
int __pascal far read_key()
{
  // stub
  int key = last_key_scancode;
  last_key_scancode = 0;
  return key;
}

// seg009:019A
void __pascal far clear_kbd_buf()
{
  // stub
  last_key_scancode = 0;
  last_text_input = 0;
}

// seg009:040A
word __pascal far prandom(word max)
{
  if (!seed_was_init)
  {
    // init from current time
    random_seed = time(NULL);
    seed_was_init = 1;
  }
  random_seed = random_seed * 214013 + 2531011;
  return (random_seed >> 16) % (max + 1);
}

static FILE *open_dat_from_root_or_data_dir(const char *filename)
{
  FILE *fp = NULL;
  fp = fcache_open(filename, "rb");

  // if failed, try if the DAT file can be opened in the data/ directory, instead of the main folder
  if (fp == NULL)
  {
    char data_path[POP_MAX_PATH];
    snprintf_check(data_path, sizeof(data_path), "data/%s", filename);

    if (!file_exists(data_path))
    {
      find_exe_dir();
      snprintf_check(data_path, sizeof(data_path), "%s/data/%s", exe_dir, filename);
    }

    // verify that this is a regular file and not a directory (otherwise, don't open)
    fp = fcache_open(data_path, "rb");
  }
  return fp;
}

// seg009:0F58
dat_type *__pascal open_dat(const char *filename, int drive)
{
  FILE *fp = NULL;
  fp = open_dat_from_root_or_data_dir(filename);
  dat_header_type dat_header;
  dat_table_type *dat_table = NULL;

  dat_type *pointer = (dat_type *)calloc(1, sizeof(dat_type));
  snprintf_check(pointer->filename, sizeof(pointer->filename), "%s", filename);
  pointer->next_dat = dat_chain_ptr;
  dat_chain_ptr = pointer;

  if (fp != NULL)
  {
    if (fcache_read(&dat_header, 6, 1, fp) != 1)
      goto failed;
    dat_table = (dat_table_type *)malloc(dat_header.table_size);
    if (dat_table == NULL ||
        fcache_seek(fp, dat_header.table_offset, SEEK_SET) ||
        fcache_read(dat_table, dat_header.table_size, 1, fp) != 1)
      goto failed;
    pointer->handle = fp;
    pointer->dat_table = dat_table;
  }
out:
  // stub
  return pointer;
failed:
  perror(filename);
  if (fp)
    fcache_close(fp);
  if (dat_table)
    free(dat_table);
  goto out;
}

// seg000:0000
void far pop_main()
{
  // Fix bug: with start_in_blind_mode enabled, moving objects are not displayed until blind mode is toggled off+on??
  need_drects = 1;

  dathandle = open_dat("PRINCE.DAT", 0);

  parse_cmdline_sound();

  init_game_main();
}

// seg009:9F80
void far *__pascal load_from_opendats_alloc(int resource, const char *extension, data_location *out_result, int *out_size)
{
  // stub
  //printf("id = %d\n",resource);
  dat_type *pointer;
  data_location result;
  byte checksum;
  int size;
  FILE *fp = NULL;
  load_from_opendats_metadata(resource, extension, &fp, &result, &checksum, &size, &pointer);
  if (out_result != NULL)
    *out_result = result;
  if (out_size != NULL)
    *out_size = size;
  if (result == data_none)
    return NULL;

  void *area = malloc(size);
  //read(fd, area, size);
  if (fcache_read(area, size, 1, fp) != 1)
  {
    printf("ERROR\n");
    fprintf(stderr, "%s: %s, resource %d, size %d, failed: %s\n", __func__, pointer->filename, resource, size, strerror(errno));
    free(area);
    area = NULL;
  }

  if (result == data_directory)
    fcache_close(fp);
  /* XXX: check checksum */
  return area;
}


// seg009:121A
image_type *far __pascal far load_image(int resource_id, dat_pal_type *palette)
{
  // stub
  data_location result;
  int size;
  image_data_type *image_data = (image_data_type*)load_from_opendats_alloc(resource_id, "png", &result, &size);
  image_type *image = (image_type*)malloc(sizeof(image_data));
  switch (result)
  {
  case data_none:
    return NULL;
    break;
  case data_DAT:
  { // DAT
   image->h = image_data->height;
   image->w = image_data->width;
  }
  break;
  case data_directory:
  { // directory
    SDL_RWops *rw = SDL_RWFromConstMem(image_data, size);
    SDL_Surface* image2 = IMG_Load_RW(rw, 0);
    image->h = image2->h;
    image->w = image2->w;
  }
  break;
  }
  if (image_data != NULL) free(image_data);

  return image;
}

// seg009:104E
chtab_type *__pascal load_sprites_from_file(int resource, int palette_bits, int quit_on_error)
{
  int i;
  int n_images = 0;
  //int has_palette_bits = 1;
  chtab_type *chtab = NULL;
  dat_shpl_type *shpl = (dat_shpl_type *)load_from_opendats_alloc(resource, "pal", NULL, NULL);

  dat_pal_type *pal_ptr = &shpl->palette;

  n_images = shpl->n_images;
  size_t alloc_size = sizeof(chtab_type) + sizeof(void far *) * n_images;
  chtab = (chtab_type *)malloc(alloc_size);
  memset(chtab, 0, alloc_size);
  chtab->n_images = n_images;
  for (i = 1; i <= n_images; i++)
  {
    chtab->images[i - 1] = load_image(resource + i, pal_ptr);;
  }
  return chtab;
}


void load_from_opendats_metadata(int resource_id, const char *extension, FILE **out_fp, data_location *result, byte *checksum, int *size, dat_type **out_pointer)
{
  char image_filename[POP_MAX_PATH];
  FILE *fp = NULL;
  dat_type *pointer;
  *result = data_none;
  // Go through all open DAT files.
  for (pointer = dat_chain_ptr; fp == NULL && pointer != NULL; pointer = pointer->next_dat)
  {
    *out_pointer = pointer;
    if (pointer->handle != NULL)
    {
      // If it's an actual DAT file:
      fp = pointer->handle;
      dat_table_type *dat_table = pointer->dat_table;
      int i;
      for (i = 0; i < dat_table->res_count; ++i)
      {
        if (dat_table->entries[i].id == resource_id)
        {
          break;
        }
      }
      if (i < dat_table->res_count)
      {
        // found
        *result = data_DAT;
        *size = dat_table->entries[i].size;
        if (fcache_seek(fp, dat_table->entries[i].offset, SEEK_SET) ||
            fcache_read(checksum, 1, 1, fp) != 1)
        {
          perror(pointer->filename);
          fp = NULL;
        }
      }
      else
      {
        // not found
        fp = NULL;
      }
    }
    else
    {
      // If it's a directory:
      char filename_no_ext[POP_MAX_PATH];
      // strip the .DAT file extension from the filename (use folders simply named TITLE, KID, VPALACE, etc.)
      strncpy(filename_no_ext, pointer->filename, sizeof(filename_no_ext));
      size_t len = strlen(filename_no_ext);
      if (len >= 5 && filename_no_ext[len - 4] == '.')
      {
        filename_no_ext[len - 4] = '\0'; // terminate, so ".DAT" is deleted from the filename
      }
      snprintf_check(image_filename, sizeof(image_filename), "data/%s/res%d.%s", filename_no_ext, resource_id, extension);

        //printf("loading (binary) %s",image_filename);
        const char *filename = locate_file(image_filename);
        //printf("File: %s\n", filename);
        fp = fcache_open(filename, "rb");
        if (fp != NULL)
        {
          *result = data_directory;
        }

      if (fp != NULL)
      {
        *result = data_directory;

        fcache_seek(fp, 0L, SEEK_END);
        *size = fcache_tell(fp);
        fcache_seek(fp, 0L, SEEK_SET);
      }
    }
  }
  *out_fp = fp;
  if (fp == NULL)
  {
    *result = data_none;
    //  printf(" FAILED\n");
    //return NULL;
  }
  //...
}

// seg009:9F34
void __pascal far close_dat(dat_type far *pointer)
{
  dat_type **prev = &dat_chain_ptr;
  dat_type *curr = dat_chain_ptr;
  while (curr != NULL)
  {
    if (curr == pointer)
    {
      *prev = curr->next_dat;
      if (curr->handle)
        fcache_close(curr->handle);
      if (curr->dat_table)
        free(curr->dat_table);
      free(curr);
      return;
    }
    curr = curr->next_dat;
    prev = &((*prev)->next_dat);
  }
  // stub
}


// seg009:A172
int __pascal far load_from_opendats_to_area(int resource, void far *area, int length, const char *extension)
{
  // stub
  //return 0;
  dat_type *pointer;
  data_location result;
  byte checksum;
  int size;
  FILE *fp = NULL;
  load_from_opendats_metadata(resource, extension, &fp, &result, &checksum, &size, &pointer);
  if (result == data_none)
    return 0;
  if (fcache_read(area, MIN(size, length), 1, fp) != 1)
  {
    fprintf(stderr, "%s: %s, resource %d, size %d, failed: %s\n", __func__, pointer->filename, resource, size, strerror(errno));
    memset(area, 0, MIN(size, length));
  }
  if (result == data_directory)
    fcache_close(fp);
  /* XXX: check checksum */
  return 0;
}

// seg000:024F
void __pascal far init_game_main()
{
  doorlink1_ad = /*&*/ level.doorlinks1;
  doorlink2_ad = /*&*/ level.doorlinks2;
  prandom(1);
  // PRINCE.DAT: sword
  chtab_addrs[id_chtab_0_sword] = load_sprites_from_file(700, 1 << 2, 1);
  // PRINCE.DAT: flame, sword on floor, potion
  chtab_addrs[id_chtab_1_flameswordpotion] = load_sprites_from_file(150, 1 << 3, 1);
  close_dat(dathandle);

  start_game();
}

void __pascal far init_copyprot()
{
  word which_entry;
  word pos;
  word entry_used[40];
  byte letts_used[26];

  copyprot_plac = prandom(13);
  memset(&entry_used, 0, sizeof(entry_used));
  memset(&letts_used, 0, sizeof(letts_used));
  for (pos = 0; pos < 14; ++pos)
  {
    do
    {
      if (pos == copyprot_plac)
      {
        which_entry = copyprot_idx = prandom(39);
      }
      else
      {
        which_entry = prandom(39);
      }
    } while (entry_used[which_entry] || letts_used[copyprot_letter[which_entry] - 'A']);
    cplevel_entr[pos] = which_entry;
    entry_used[which_entry] = 1;
    letts_used[copyprot_letter[which_entry] - 'A'] = 1;
  }
}

void __pascal far start_game()
{
  // Prevent filling of stack.
  // start_game is called from many places to restart the game, for example:
  // process_key, play_frame, draw_game_frame, play_level, control_kid, end_sequence, expired
  init_copyprot();

 init_game(start_level);
}

void restore_room_after_quick_load()
{
  int temp1 = curr_guard_color;
  int temp2 = next_level;
  reset_level_unused_fields(false);
  curr_guard_color = temp1;
  next_level = temp2;

  //need_full_redraw = 1;
  different_room = 1;
  // Show the room where the prince is, even if the player moved the view away from it (with the H,J,U,N keys).
  next_room = drawn_room = Kid.room;
  load_room_links();
  //draw_level_first();
  //gen_palace_wall_colors();
  is_guard_notice = 0; // prevent guard turning around immediately
  //redraw_screen(1); // for room_L

  hitp_delta = guardhp_delta = 1; // force HP redraw
  // Don't draw guard HP if a previously viewed room (with the H,J,U,N keys) had a guard but the current room doesn't have one.
  if (Guard.room != drawn_room)
  {
    // Like in clear_char().
    Guard.direction = dir_56_none;
    guardhp_curr = 0;
  }

  loadkid_and_opp();
  // Get rid of "press button" message if kid was dead before quickload.
  text_time_total = text_time_remaining = 0;
}

Uint32 temp_shift_release_callback(Uint32 interval, void *param)
{
  const Uint8 *state = SDL_GetKeyboardState(NULL);
  if (state[SDL_SCANCODE_LSHIFT])
    key_states[SDL_SCANCODE_LSHIFT] = 1;
  if (state[SDL_SCANCODE_RSHIFT])
    key_states[SDL_SCANCODE_RSHIFT] = 1;
  return 0; // causes the timer to be removed
}

// seg000:04CD
int __pascal far process_key()
{
  int key = 0;

  // If the Kid died, Enter or Shift will restart the level.
  if (rem_min != 0 && Kid.alive > 6 && (control_shift || key == SDL_SCANCODE_RETURN))
  {
    key = SDL_SCANCODE_A | WITH_CTRL; // Ctrl+A
  }
  if (key == 0)
    return 0;
  if (is_keyboard_mode)
    clear_kbd_buf();

  switch (key)
  {
  case SDL_SCANCODE_ESCAPE:              // Esc
  case SDL_SCANCODE_ESCAPE | WITH_SHIFT: // allow pause while grabbing
    is_paused = 1;
    break;
  case SDL_SCANCODE_A | WITH_CTRL: // Ctrl+A
    if (current_level != 15)
    {
      is_restart_level = 1;
    }
    break;
  case SDL_SCANCODE_K | WITH_CTRL: // Ctrl+K
    is_keyboard_mode = 1;
    break;
  case SDL_SCANCODE_R | WITH_CTRL: // Ctrl+R
    start_level = -1;
    start_game();
    break;
  }

  return 1;
}

// seg000:08EB
void __pascal far play_frame()
{
  do_mobs();
  process_trobs();
  check_skel();
  check_can_guard_see_kid();
  // if level is restarted, return immediately
  if (play_kid_frame())
    return;
  play_guard_frame();
  if (0 == resurrect_time)
  {
    check_sword_hurting();
    check_sword_hurt();
  }
  check_sword_vs_sword();
  do_delta_hp();
  exit_room();
  check_the_end();
  check_guard_fallout();
  if (current_level == 0)
  {
    // Special event: level 0 running exit
    if (Kid.room == 24)
    {
      start_level = -1;
      need_quotes = 1;
      start_game();
    }
  }
  else if (current_level == 6)
  {
    // Special event: level 6 falling exit
    if (roomleave_result == -2)
    {
      Kid.y = -1;
      ++next_level;
    }
  }
  else if (current_level == 12)
  {
    // Special event: level 12 running exit
    if (Kid.room == 23 )
    {
      ++next_level;
      // Sounds must be stopped, because play_level_2() checks next_level only if there are no sounds playing.
      seamless = 1;
    }
  }
}

// seg007:1041
short __pascal far get_curr_tile(short tilepos)
{
  curr_modifier = curr_room_modif[tilepos];
  return curr_tile = curr_room_tiles[tilepos] & 0x1F;
}


// seg000:0B12
void __pascal far anim_tile_modif()
{
  word tilepos;
  for (tilepos = 0; tilepos < 30; ++tilepos)
  {
    switch (get_curr_tile(tilepos))
    {
    case tiles_10_potion:
      start_anim_potion(drawn_room, tilepos);
      break;
    case tiles_19_torch:
    case tiles_30_torch_with_debris:
      start_anim_torch(drawn_room, tilepos);
      break;
    case tiles_22_sword:
      start_anim_sword(drawn_room, tilepos);
      break;
    }
  }

  // Animate torches in the rightmost column of the left-side room as well, because they are visible in the current room.
  for (int row = 0; row <= 2; row++)
  {
    switch (get_tile(room_L, 9, row))
    {
    case tiles_19_torch:
    case tiles_30_torch_with_debris:
      start_anim_torch(room_L, row * 10 + 9);
      break;
    }
  }
}

void __pascal far load_lev_spr(int level)
{
  dat_type *dathandle;
  short guardtype;
  char filename[20];
  dathandle = NULL;
  current_level = next_level = level;
  snprintf(filename, sizeof(filename), "%s%s.DAT", tbl_envir_gr[gmMcgaVga], tbl_envir_ki[custom->tbl_level_type[current_level]]);
  load_chtab_from_file(id_chtab_6_environment, 200, filename, 1 << 5);
  load_more_opt_graf(filename);
  guardtype = custom->tbl_guard_type[current_level];
  if (guardtype != -1)
  {
    if (guardtype == 0)
    {
      dathandle = open_dat(custom->tbl_level_type[current_level] ? "GUARD1.DAT" : "GUARD2.DAT", 0);
    }
    load_chtab_from_file(id_chtab_5_guard, 750, tbl_guard_dat[guardtype], 1 << 8);
    if (dathandle)
    {
      close_dat(dathandle);
    }
  }
  curr_guard_color = 0;
  load_chtab_from_file(id_chtab_7_environmentwall, 360, filename, 1 << 6);
}

// seg000:0E6C
void __pascal far load_level()
{
  dat_type *dathandle;
  dathandle = open_dat("LEVELS.DAT", 0);
  load_from_opendats_to_area(current_level + 2000, &level, sizeof(level), "bin");
  close_dat(dathandle);

  reset_level_unused_fields(true); // added
}

void reset_level_unused_fields(bool loading_clean_level)
{
  // Entirely unused fields in the level format: reset to zero for now
  // They can be repurposed to add new stuff to the level format in the future
  memset(level.roomxs, 0, sizeof(level.roomxs));
  memset(level.roomys, 0, sizeof(level.roomys));
  memset(level.fill_1, 0, sizeof(level.fill_1));
  memset(level.fill_2, 0, sizeof(level.fill_2));
  memset(level.fill_3, 0, sizeof(level.fill_3));

  // level.used_rooms is 25 on some levels. Limit it to the actual number of rooms.
  if (level.used_rooms > 24)
    level.used_rooms = 24;

  // For these fields, only use the bits that are actually used, and set the rest to zero.
  // Good for repurposing the unused bits in the future.
  int i;
  for (i = 0; i < level.used_rooms; ++i)
  {
    //level.guards_dir[i]   &= 0x01; // 1 bit in use
    level.guards_skill[i] &= 0x0F; // 4 bits in use
  }

  // In savestates, additional information may be stored (e.g. remembered guard hp) - should not reset this then!
  if (loading_clean_level)
  {
    for (i = 0; i < level.used_rooms; ++i)
    {
      level.guards_color[i] &= 0x0F; // 4 bits in use (other 4 bits repurposed as remembered guard hp)
    }
  }
}

// seg000:0EA8
// returns 1 if level is restarted, 0 otherwise
int __pascal far play_kid_frame()
{
  loadkid_and_opp();
  load_fram_det_col();
  check_killed_shadow();
  play_kid();
  if (upside_down && Char.alive >= 0)
  {
    upside_down = 0;
    need_redraw_because_flipped = 1;
  }
  if (is_restart_level)
  {
    return 1;
  }
  if (Char.room != 0)
  {
    play_seq();
    fall_accel();
    fall_speed();
    load_frame_to_obj();
    load_fram_det_col();
    set_char_collision();
    bump_into_opponent();
    check_collisions();
    check_bumped();
    check_gate_push();
    check_action();
    check_press();
    check_spike_below();
    if (resurrect_time == 0)
    {
      check_spiked();
      check_chomped_kid();
    }
    check_knock();
  }
  savekid();
  return 0;
}

// seg000:0F48
void __pascal far play_guard_frame()
{
  if (Guard.direction != dir_56_none)
  {
    loadshad_and_opp();
    load_fram_det_col();
    check_killed_shadow();
    play_guard();
    if (Char.room == drawn_room)
    {
      play_seq();
      if (Char.x >= 44 && Char.x < 211)
      {
        fall_accel();
        fall_speed();
        load_frame_to_obj();
        load_fram_det_col();
        set_char_collision();
        check_guard_bumped();
        check_action();
        check_press();
        check_spike_below();
        check_spiked();
        check_chomped_guard();
      }
    }
    saveshad();
  }
}

// seg000:0FBD
void __pascal far check_the_end()
{
  if (next_room != 0 && next_room != drawn_room)
  {
    drawn_room = next_room;
    load_room_links();
    different_room = 1;
    exit_room_timer = 2; // Added to remember exit room timer
    loadkid();
    anim_tile_modif();
    start_chompers();
    check_fall_flo();
    check_shadow();
  }
}

// seg000:1009
void __pascal far check_fall_flo()
{
  // Special event: falling floors
  if (current_level == /*13*/ custom->loose_tiles_level &&
      (drawn_room == /*23*/ custom->loose_tiles_room_1 || drawn_room == /*16*/ custom->loose_tiles_room_2))
  {
    get_room_address(curr_room = room_A);
    for (curr_tilepos = /*22*/ custom->loose_tiles_first_tile;
         curr_tilepos <= /*27*/ custom->loose_tiles_last_tile;
         ++curr_tilepos)
    {
      make_loose_fall(-(prandom(0xFF) & 0x0F));
    }
  }
}


// seg000:11EC
void __pascal far add_life()
{
  short hpmax = hitp_max;
  ++hpmax;
  // CusPop: set maximum number of hitpoints (max_hitp_allowed, default = 10)
  //	if (hpmax > 10) hpmax = 10; // original
  if (hpmax > custom->max_hitp_allowed)
    hpmax = custom->max_hitp_allowed;
  hitp_max = hpmax;
  set_health_life();
}

// seg000:1200
void __pascal far set_health_life()
{
  hitp_delta = hitp_max - hitp_curr;
}


// seg000:127B
void __pascal far do_delta_hp()
{
  // level 12: if the shadow is hurt, Kid is also hurt
  if (Opp.charid == charid_1_shadow &&
      current_level == 12 &&
      guardhp_delta != 0)
  {
    hitp_delta = guardhp_delta;
  }
  hitp_curr = MIN(MAX(hitp_curr + hitp_delta, 0), hitp_max);
  guardhp_curr = MIN(MAX(guardhp_curr + guardhp_delta, 0), guardhp_max);
}

// seg000:1353
void __pascal far check_sword_vs_sword()
{
}

// seg000:136A
void __pascal far load_chtab_from_file(int chtab_id, int resource, const char near *filename, int palette_bits)
{
  //printf("Loading chtab %d, id %d from %s\n",chtab_id,resource,filename);
  dat_type *dathandle;
  if (chtab_addrs[chtab_id] != NULL)
    return;
  dathandle = open_dat(filename, 0);
  chtab_addrs[chtab_id] = load_sprites_from_file(resource, palette_bits, 1);
  close_dat(dathandle);
}

// seg009:12EF
void __pascal far load_one_optgraf(chtab_type *chtab_ptr, dat_pal_type far *pal_ptr, int base_id, int min_index, int max_index)
{
  short index;
  for (index = min_index; index <= max_index; ++index)
  {
    image_type *image = load_image(base_id + index + 1, pal_ptr);
    if (image != NULL)
      chtab_ptr->images[index] = image;
  }
}

// seg000:13FC
void __pascal far load_more_opt_graf(const char *filename)
{
  // stub
  dat_type *dathandle;
  dat_shpl_type area;
  short graf_index;
  dathandle = NULL;
  for (graf_index = 0; graf_index < 8; ++graf_index)
  {
    /*if (...) */ {
      if (dathandle == NULL)
      {
        dathandle = open_dat(filename, 0);
        load_from_opendats_to_area(200, &area, sizeof(area), "pal");
        area.palette.row_bits = 0x20;
      }
      load_one_optgraf(chtab_addrs[id_chtab_6_environment], &area.palette, 1200, optgraf_min[graf_index] - 1, optgraf_max[graf_index] - 1);
    }
  }
  if (dathandle != NULL)
  {
    close_dat(dathandle);
  }
}

// seg000:148D
int __pascal far do_paused()
{
  word key;
  key = 0;
  next_room = 0;
  control_shift = 0;
  control_y = 0;
  control_x = 0;
  read_keyb_control();
  key = process_key();
  return key || control_shift;
}

// seg000:1500
void __pascal far read_keyb_control()
{
  if (key_states[SDL_SCANCODE_UP] || key_states[SDL_SCANCODE_HOME] || key_states[SDL_SCANCODE_PAGEUP] || key_states[SDL_SCANCODE_KP_8] || key_states[SDL_SCANCODE_KP_7] || key_states[SDL_SCANCODE_KP_9])
  {
    control_y = -1;
  }
  else if (key_states[SDL_SCANCODE_CLEAR] || key_states[SDL_SCANCODE_DOWN] || key_states[SDL_SCANCODE_KP_5] || key_states[SDL_SCANCODE_KP_2])
  {
    control_y = 1;
  }
  if (key_states[SDL_SCANCODE_LEFT] || key_states[SDL_SCANCODE_HOME] || key_states[SDL_SCANCODE_KP_4] || key_states[SDL_SCANCODE_KP_7])
  {
    control_x = -1;
  }
  else if (key_states[SDL_SCANCODE_RIGHT] || key_states[SDL_SCANCODE_PAGEUP] || key_states[SDL_SCANCODE_KP_6] || key_states[SDL_SCANCODE_KP_9])
  {
    control_x = 1;
  }
  control_shift = -(key_states[SDL_SCANCODE_LSHIFT] || key_states[SDL_SCANCODE_RSHIFT]);
}

// seg000:15E9
void __pascal far toggle_upside()
{
  upside_down = ~upside_down;
  need_redraw_because_flipped = 1;
}

// seg000:15F8
void __pascal far feather_fall()
{
  is_feather_fall = 1;
  flash_color = 2; // green
  flash_time = 3;
}

// seg000:172C
void __pascal far gen_palace_wall_colors()
{
  dword old_randseed;
  word prev_color;
  short row;
  short subrow;
  word color_base;
  short column;
  word color;

  old_randseed = random_seed;
  random_seed = drawn_room;
  prandom(1); // discard
  for (row = 0; row < 3; row++)
  {
    for (subrow = 0; subrow < 4; subrow++)
    {
      if (subrow % 2)
      {
        color_base = 0x61; // 0x61..0x64 in subrow 1 and 3
      }
      else
      {
        color_base = 0x66; // 0x66..0x69 in subrow 0 and 2
      }
      prev_color = -1;
      for (column = 0; column <= 10; ++column)
      {
        do
        {
          color = color_base + prandom(3);
        } while (color == prev_color);
        palace_wall_colors[44 * row + 11 * subrow + column] = color;
        //palace_wall_colors[row][subrow][column] = color;
        prev_color = color;
      }
    }
  }
  random_seed = old_randseed;
}

// seg000:1D2C
void __pascal far load_kid_sprite()
{
  load_chtab_from_file(id_chtab_2_kid, 400, "KID.DAT", 1 << 7);
}

// seg000:1F7B
void __pascal far parse_cmdline_sound()
{
    // Use digi (wave) sounds and MIDI music.
    sound_flags |= sfDigi;
    sound_flags |= sfMidi;
    sound_mode = smSblast;
}

// seg002:0000
void __pascal far do_init_shad(const byte *source, int seq_index)
{
  memcpy_near(&Char, source, 7);
  seqtbl_offset_char(seq_index);
  Char.charid = charid_1_shadow;
  demo_time = 0;
  guard_skill = 3;
  guardhp_delta = guardhp_curr = guardhp_max = 4;
  saveshad();
}

// seg002:0044
void __pascal far get_guard_hp()
{
  guardhp_delta = guardhp_curr = guardhp_max = custom->extrastrength[guard_skill] + custom->tbl_guard_hp[current_level];
}

// seg002:0064
void __pascal far check_shadow()
{
  offguard = 0;
  if (current_level == 12)
  {
    // Special event: level 12 shadow
    if (!united_with_shadow && drawn_room == 15)
    {
      Char.room = drawn_room;
      if (get_tile(15, 1, 0) == tiles_22_sword)
      {
        return;
      }
      shadow_initialized = 0;
      do_init_shad(/*&*/ custom->init_shad_12, 7 /*fall*/);
      return;
    }
  }
  else if (current_level == 6)
  {
    // Special event: level 6 shadow
    Char.room = drawn_room;
    if (Char.room == 1)
    {
      if (leveldoor_open != 0x4D)
      {
        leveldoor_open = 0x4D;
      }
      do_init_shad(/*&*/ custom->init_shad_6, 2 /*stand*/);
      return;
    }
  }
  else if (current_level == 5)
  {
    // Special event: level 5 shadow
    Char.room = drawn_room;
    if (Char.room == 24)
    {
      if (get_tile(24, 3, 0) != tiles_10_potion)
      {
        return;
      }
      do_init_shad(/*&*/ custom->init_shad_5, 2 /*stand*/);
      return;
    }
  }
  enter_guard();
}

// seg002:0112
void __pascal far enter_guard()
{
  word room_minus_1;
  word guard_tile;
  word frame;
  byte seq_hi;
  // arrays are indexed 0..23 instead of 1..24
  room_minus_1 = drawn_room - 1;
  frame = Char.frame; // hm?
  guard_tile = level.guards_tile[room_minus_1];
  if (guard_tile >= 30)
    return;

  Char.room = drawn_room;
  Char.curr_row = guard_tile / 10;
  Char.y = y_land[Char.curr_row + 1];
  Char.x = level.guards_x[room_minus_1];
  Char.curr_col = get_tile_div_mod_m7(Char.x);
  Char.direction = level.guards_dir[room_minus_1];
  // only regular guards have different colors (and only on VGA)
   curr_guard_color = 0;

#ifdef REMEMBER_GUARD_HP
  int remembered_hp = (level.guards_color[room_minus_1] & 0xF0) >> 4;
#endif
  curr_guard_color &= 0x0F; // added; only least significant 4 bits are used for guard color

  // level 3 has skeletons with infinite lives
  //if (current_level == 3) {
  if (custom->tbl_guard_type[current_level] == 2)
  {
    Char.charid = charid_4_skeleton;
  }
  else
  {
    Char.charid = charid_2_guard;
  }
  seq_hi = level.guards_seq_hi[room_minus_1];
  if (seq_hi == 0)
  {
    if (Char.charid == charid_4_skeleton)
    {
      Char.sword = sword_2_drawn;
      seqtbl_offset_char(seq_63_guard_stand_active); // stand active (when entering room) (skeleton)
    }
    else
    {
      Char.sword = sword_0_sheathed;
      seqtbl_offset_char(seq_77_guard_stand_inactive); // stand inactive (when entering room)
    }
  }
  else
  {
    Char.curr_seq = level.guards_seq_lo[room_minus_1] + (seq_hi << 8);
  }
  play_seq();
  guard_skill = level.guards_skill[room_minus_1];
  if (guard_skill >= NUM_GUARD_SKILLS)
  {
    guard_skill = 3;
  }
  frame = Char.frame;
  if (frame == frame_185_dead || frame == frame_177_spiked || frame == frame_178_chomped)
  {
    Char.alive = 1;
    guardhp_curr = 0;
  }
  else
  {
    Char.alive = -1;
    justblocked = 0;
    guard_refrac = 0;
    is_guard_notice = 0;
    get_guard_hp();
  }
  Char.fall_y = 0;
  Char.fall_x = 0;
  Char.action = actions_1_run_jump;
  saveshad();
}

// seg002:0269
void __pascal far check_guard_fallout()
{
  if (Guard.direction == dir_56_none || Guard.y < 211)
  {
    return;
  }
  if (Guard.charid == charid_1_shadow)
  {
    if (Guard.action != actions_4_in_freefall)
    {
      return;
    }
    loadshad();
    clear_char();
    saveshad();
  }
  else if (Guard.charid == charid_4_skeleton &&
           (Guard.room = level.roomlinks[Guard.room - 1].down) == /*3*/ custom->skeleton_reappear_room)
  {
    // if skeleton falls down into room 3
    Guard.x = /*133*/ custom->skeleton_reappear_x;
    Guard.curr_row = /*1*/ custom->skeleton_reappear_row;
    Guard.direction = /*dir_0_right*/ custom->skeleton_reappear_dir;
    Guard.alive = -1;
    leave_guard();
  }
  else
  {
    on_guard_killed();
    level.guards_tile[drawn_room - 1] = -1;
    Guard.direction = dir_56_none;
    guardhp_curr = 0;
  }
}

// seg002:02F5
void __pascal far leave_guard()
{
  word room_minus_1;
  if (Guard.direction == dir_56_none || Guard.charid == charid_1_shadow || Guard.charid == charid_24_mouse)
  {
    return;
  }
  // arrays are indexed 0..23 instead of 1..24
  room_minus_1 = Guard.room - 1;
  level.guards_tile[room_minus_1] = get_tilepos(0, Guard.curr_row);

  level.guards_color[room_minus_1] = curr_guard_color & 0x0F; // restriction to 4 bits added

  level.guards_x[room_minus_1] = Guard.x;
  level.guards_dir[room_minus_1] = Guard.direction;
  level.guards_skill[room_minus_1] = guard_skill;
  if (Guard.alive < 0)
  {
    level.guards_seq_hi[room_minus_1] = 0;
  }
  else
  {
    level.guards_seq_lo[room_minus_1] = Guard.curr_seq;
    level.guards_seq_hi[room_minus_1] = Guard.curr_seq >> 8;
  }
  Guard.direction = dir_56_none;
  guardhp_curr = 0;
}



// seg002:0486
int __pascal far goto_other_room(short direction)
{
  //printf("goto_other_room: direction = %d, Char.room = %d\n", direction, Char.room);
  short opposite_dir;
  byte other_room = ((byte *)&level.roomlinks[Char.room - 1])[direction];
  Char.room = other_room;
  if (direction == 0)
  {
    // left
    Char.x += 140;
    opposite_dir = 1;
  }
  else if (direction == 1)
  {
    // right
    Char.x -= 140;
    opposite_dir = 0;
  }
  else if (direction == 2)
  {
    // up
    Char.y += 189;
    Char.curr_row = y_to_row_mod4(Char.y);
    opposite_dir = 3;
  }
  else
  {
    // down
    Char.y -= 189;
    Char.curr_row = y_to_row_mod4(Char.y);
    opposite_dir = 2;
  }
  return opposite_dir;
}

// seg002:039E
void __pascal far follow_guard()
{
  level.guards_tile[Kid.room - 1] = 0xFF;
  level.guards_tile[Guard.room - 1] = 0xFF;
  loadshad();
  goto_other_room(roomleave_result);
  saveshad();
}

// seg002:0504
short __pascal far leave_room()
{
  short frame;
  word action;
  short chary;
  short leave_dir;
  chary = Char.y;
  action = Char.action;
  frame = Char.frame;
  if (action != actions_5_bumped &&
      action != actions_4_in_freefall &&
      action != actions_3_in_midair &&
      (sbyte)chary < 10 && (sbyte)chary > -16)
  {
    leave_dir = 2; // up
  }
  else if (chary >= 211)
  {
    leave_dir = 3; // down
  }
  else if (
    // frames 135..149: climb up
    (frame >= frame_135_climbing_1 && frame < 150) ||
    // frames 110..119: standing up from crouch
    (frame >= frame_110_stand_up_from_crouch_1 && frame < 120) ||
    // frames 150..162: with sword
    (frame >= frame_150_parry && frame < 163
       ) ||
    // frames 166..168: with sword
    (frame >= frame_166_stand_inactive && frame < 169) ||
    action == actions_7_turn // turn
  )
  {
    return -1;
  }
  else if (Char.direction != dir_0_right)
  {
    // looking left
    if (char_x_left <= 54)
    {
      leave_dir = 0; // left
    }
    else if (char_x_left >= 198)
    {
      leave_dir = 1; // right
    }
    else
    {
      return -1;
    }
  }
  else
  {
    // looking right
    get_tile(Char.room, 9, Char.curr_row);
    if (curr_tile2 != tiles_7_doortop_with_floor &&
        curr_tile2 != tiles_12_doortop &&
        char_x_right >= 201)
    {
      leave_dir = 1; // right
    }
    else if (char_x_right <= 57)
    {
      leave_dir = 0; // left
    }
    else
    {
      return -1;
    }
  }
  switch (leave_dir)
  {
  case 0: // left
    play_mirr_mus();
    level3_set_chkp();
    Jaffar_exit();
    break;
  case 1: // right
    sword_disappears();
    meet_Jaffar();
    break;
  //case 2: // up
  case 3: // down
    // Special event: falling exit
    if (current_level == custom->falling_exit_level /*6*/ && Char.room == custom->falling_exit_room /*1*/)
    {
      return -2;
    }
    break;
  }
  goto_other_room(leave_dir);
#ifdef USE_REPLAY
  if (skipping_replay && replay_seek_target == replay_seek_0_next_room)
    skipping_replay = 0;
#endif
  return leave_dir;
}

// seg002:03C7
void __pascal far exit_room()
{
  word leave;
  word kid_room_m1;
  leave = 0;
  if (exit_room_timer != 0)
  {
    --exit_room_timer;
      return;
  }
  loadkid();
  load_frame_to_obj();
  set_char_collision();
  roomleave_result = leave_room();
  if (roomleave_result < 0)
  {
    return;
  }
  savekid();
  next_room = Char.room;
  if (Guard.direction == dir_56_none)
    return;
  if (Guard.alive < 0 && Guard.sword == sword_2_drawn)
  {
    kid_room_m1 = Kid.room - 1;
    // kid_room_m1 might be 65535 (-1) when the prince fell out of the level (to room 0) while a guard was active.
    // In this case, the indexing in the following condition crashes on Linux.
    if ((kid_room_m1 >= 0 && kid_room_m1 <= 23) &&
        (level.guards_tile[kid_room_m1] >= 30 || level.guards_seq_hi[kid_room_m1] != 0))
    {
      if (roomleave_result == 0)
      {
        // left
        if (Guard.x >= 91)
          leave = 1;
      }
      else if (roomleave_result == 1)
      {
        // right
        if (Guard.x < 165)
          leave = 1;
      }
      else if (roomleave_result == 2)
      {
        // up
        if (Guard.curr_row >= 0)
          leave = 1;
      }
      else
      {
        // down
        if (Guard.curr_row < 3)
          leave = 1;
      }
    }
    else
    {
      leave = 1;
    }
  }
  else
  {
    leave = 1;
  }
  if (leave)
  {
    leave_guard();
  }
  else
  {
    follow_guard();
  }
}

// seg002:0643
void __pascal far Jaffar_exit()
{
  if (leveldoor_open == 2)
  {
    get_tile(24, 0, 0);
    trigger_button(0, 0, -1);
  }
}

// seg002:0665
void __pascal far level3_set_chkp()
{
  // Special event: set checkpoint
  if (current_level == /*3*/ custom->checkpoint_level && Char.room == 7 /* TODO: add a custom option */)
  {
    checkpoint = 1;
    hitp_beg_lev = hitp_max;
  }
}

// seg002:0680
void __pascal far sword_disappears()
{
  // Special event: sword disappears
  if (current_level == 12 && Char.room == 18)
  {
    get_tile(15, 1, 0);
    curr_room_tiles[curr_tilepos] = tiles_1_floor;
    curr_room_modif[curr_tilepos] = 0; // added, a nonzero modifier may show fake tiles
  }
}

// seg002:06AE
void __pascal far meet_Jaffar()
{
  // Special event: play music
  if (current_level == 13 && leveldoor_open == 0 && Char.room == 3)
  {
    // Special event: Jaffar waits a bit (28/12=2.33 seconds)
    guard_notice_timer = 28;
  }
}

// seg002:06D3
void __pascal far play_mirr_mus()
{
  // Special event: mirror music
  if (
    leveldoor_open != 0 &&
    leveldoor_open != 0x4D && // was the music played already?
    current_level == /*4*/ custom->mirror_level &&
    Char.curr_row == /*0*/ custom->mirror_row &&
    Char.room == 11 /* TODO: add a custom option */
  )
  {
    leveldoor_open = 0x4D;
  }
}

// seg002:0706
void __pascal far move_0_nothing()
{
  control_shift = 0;
  control_y = 0;
  control_x = 0;
  control_shift2 = 0;
  control_down = 0;
  control_up = 0;
  control_backward = 0;
  control_forward = 0;
}

// seg002:0721
void __pascal far move_1_forward()
{
  control_x = -1;
  control_forward = -1;
}

// seg002:072A
void __pascal far move_2_backward()
{
  control_backward = -1;
  control_x = 1;
}

// seg002:0735
void __pascal far move_3_up()
{
  control_y = -1;
  control_up = -1;
}

// seg002:073E
void __pascal far move_4_down()
{
  control_down = -1;
  control_y = 1;
}

// seg002:0749
void __pascal far move_up_back()
{
  control_up = -1;
  move_2_backward();
}

// seg002:0753
void __pascal far move_down_back()
{
  control_down = -1;
  move_2_backward();
}

// seg002:075D
void __pascal far move_down_forw()
{
  control_down = -1;
  move_1_forward();
}

// seg002:0767
void __pascal far move_6_shift()
{
  control_shift = -1;
  control_shift2 = -1;
}

// seg002:0770
void __pascal far move_7()
{
  control_shift = 0;
}

// seg002:0776
void __pascal far autocontrol_opponent()
{
  word charid;
  move_0_nothing();
  charid = Char.charid;
  if (charid == charid_0_kid)
  {
    autocontrol_kid();
  }
  else
  {
    if (justblocked)
      --justblocked;
    if (kid_sword_strike)
      --kid_sword_strike;
    if (guard_refrac)
      --guard_refrac;
    if (charid == charid_24_mouse)
    {
      autocontrol_mouse();
    }
    else if (charid == charid_4_skeleton)
    {
      autocontrol_skeleton();
    }
    else if (charid == charid_1_shadow)
    {
      autocontrol_shadow();
    }
    else if (current_level == 13)
    {
      autocontrol_Jaffar();
    }
    else
    {
      autocontrol_guard();
    }
  }
}

// seg002:07EB
void __pascal far autocontrol_mouse()
{
  if (Char.direction == dir_56_none)
  {
    return;
  }
  if (Char.action == actions_0_stand)
  {
    if (Char.x >= 200)
    {
      clear_char();
    }
  }
  else
  {
    if (Char.x < 166)
    {
      seqtbl_offset_char(seq_107_mouse_stand_up_and_go); // mouse
      play_seq();
    }
  }
}

// seg002:081D
void __pascal far autocontrol_shadow()
{
  if (current_level == 4)
  {
    autocontrol_shadow_level4();
  }
  else if (current_level == 5)
  {
    autocontrol_shadow_level5();
  }
  else if (current_level == 6)
  {
    autocontrol_shadow_level6();
  }
  else if (current_level == 12)
  {
    autocontrol_shadow_level12();
  }
}

// seg002:0850
void __pascal far autocontrol_skeleton()
{
  Char.sword = sword_2_drawn;
  autocontrol_guard();
}

// seg002:085A
void __pascal far autocontrol_Jaffar()
{
  autocontrol_guard();
}

// seg002:085F
void __pascal far autocontrol_kid()
{
  autocontrol_guard();
}

// seg002:0864
void __pascal far autocontrol_guard()
{
  if (Char.sword < sword_2_drawn)
  {
    autocontrol_guard_inactive();
  }
  else
  {
    autocontrol_guard_active();
  }
}

// seg002:0876
void __pascal far autocontrol_guard_inactive()
{
  short distance;
  if (Kid.alive >= 0)
    return;
  distance = char_opp_dist();
  if (Opp.curr_row != Char.curr_row || (word)distance < (word)-8)
  {
    // If Kid made a sound ...
    if (is_guard_notice)
    {
      is_guard_notice = 0;
      if (distance < 0)
      {
        // ... and Kid is behind Guard, Guard turns around.
        if ((word)distance < (word)-4)
        {
          move_4_down();
        }
        return;
      }
    }
    else if (distance < 0)
    {
      return;
    }
  }
  if (can_guard_see_kid)
  {
    // If Guard can see Kid, Guard moves to fighting pose.
    if (current_level != 13 || guard_notice_timer == 0)
    {
      move_down_forw();
    }
  }
}

// seg002:08DC
void __pascal far autocontrol_guard_active()
{
  short opp_frame;
  short char_frame;
  short distance;
  char_frame = Char.frame;
  if (char_frame != frame_166_stand_inactive && char_frame >= 150 && can_guard_see_kid != 1)
  {
    if (can_guard_see_kid == 0)
    {
      if (droppedout != 0)
      {
        guard_follows_kid_down();
        //return;
      }
      else if (Char.charid != charid_4_skeleton)
      {
        move_down_back();
      }
      //return;
    }
    else
    { // can_guard_see_kid == 2
      opp_frame = Opp.frame;
      distance = char_opp_dist();
      if (distance >= 12 &&
          // frames 102..117: falling and landing
          opp_frame >= frame_102_start_fall_1 && opp_frame < frame_118_stand_up_from_crouch_9 &&
          Opp.action == actions_5_bumped)
      {
        return;
      }
      if (distance < 35)
      {
        if ((Char.sword < sword_2_drawn && distance < 8) || distance < 12)
        {
          if (Char.direction == Opp.direction)
          {
            // turn around
            move_2_backward();
            //return;
          }
          else
          {
            move_1_forward();
            //return;
          }
        }
        else
        {
          autocontrol_guard_kid_in_sight(distance);
          //return;
        }
      }
      else
      {
        if (guard_refrac != 0)
          return;
        if (Char.direction != Opp.direction)
        {
          // frames 7..14: running
          // frames 34..43: run-jump
          if (opp_frame >= frame_7_run && opp_frame < 15)
          {
            if (distance < 40)
              move_6_shift();
            return;
          }
          else if (opp_frame >= frame_34_start_run_jump_1 && opp_frame < 44)
          {
            if (distance < 50)
              move_6_shift();
            return;
            //return;
          }
        }
        autocontrol_guard_kid_far();
      }
      //...
    }
    //...
  }
}

// seg006:0FC3
int __pascal far wall_type(byte tiletype)
{
  switch (tiletype)
  {
  case tiles_4_gate:
  case tiles_7_doortop_with_floor:
  case tiles_12_doortop:
    return 1; // wall at right
  case tiles_13_mirror:
    return 2; // wall at left
  case tiles_18_chomper:
    return 3; // chomper at left
  case tiles_20_wall:
    return 4; // wall at both sides
  default:
    return 0; // no wall
  }
}

// seg002:09CB
void __pascal far autocontrol_guard_kid_far()
{
  if (tile_is_floor(get_tile_infrontof_char()) ||
      tile_is_floor(get_tile_infrontof2_char()))
  {
    move_1_forward();
  }
  else
  {
    move_2_backward();
  }
}

// seg002:09F8
void __pascal far guard_follows_kid_down()
{
  // This is called from autocontrol_guard_active, so char=Guard, Opp=Kid
  word opp_action;
  opp_action = Opp.action;
  if (opp_action == actions_2_hang_climb || opp_action == actions_6_hang_straight)
  {
    return;
  }
  if ( // there is wall in front of Guard
    wall_type(get_tile_infrontof_char()) != 0 ||
    (!tile_is_floor(curr_tile2) && ((get_tile(curr_room, tile_col, ++tile_row) == tiles_2_spike ||
                                     // Guard would fall on loose floor
                                     curr_tile2 == tiles_11_loose ||
                                     // ... or wall (?)
                                     wall_type(curr_tile2) != 0 ||
                                     // ... or into a chasm
                                     !tile_is_floor(curr_tile2)) ||
                                    // ... or Kid is not below
                                    Char.curr_row + 1 != Opp.curr_row)))
  {
    // don't follow
    droppedout = 0;
    move_2_backward();
  }
  else
  {
    // follow
    move_1_forward();
  }
}

// seg002:0A93
void __pascal far autocontrol_guard_kid_in_sight(short distance)
{
  if (Opp.sword == sword_2_drawn)
  {
    autocontrol_guard_kid_armed(distance);
  }
  else if (guard_refrac == 0)
  {
    if (distance < 29)
    {
      move_6_shift();
    }
    else
    {
      move_1_forward();
    }
  }
}

// seg002:0AC1
void __pascal far autocontrol_guard_kid_armed(short distance)
{
  if (distance < 10 || distance >= 29)
  {
    guard_advance();
  }
  else
  {
    guard_block();
    if (guard_refrac == 0)
    {
      if (distance < 12 || distance >= 29)
      {
        guard_advance();
      }
      else
      {
        guard_strike();
      }
    }
  }
}

// seg002:0AF5
void __pascal far guard_advance()
{
  if (guard_skill == 0 || kid_sword_strike == 0)
  {
    if (custom->advprob[guard_skill] > prandom(255))
    {
      move_1_forward();
    }
  }
}

// seg002:0B1D
void __pascal far guard_block()
{
  word opp_frame;
  opp_frame = Opp.frame;
  if (opp_frame == frame_152_strike_2 || opp_frame == frame_153_strike_3 || opp_frame == frame_162_block_to_strike)
  {
    if (justblocked != 0)
    {
      if (custom->impblockprob[guard_skill] > prandom(255))
      {
        move_3_up();
      }
    }
    else
    {
      if (custom->blockprob[guard_skill] > prandom(255))
      {
        move_3_up();
      }
    }
  }
}

// seg002:0B73
void __pascal far guard_strike()
{
  word opp_frame;
  word char_frame;
  opp_frame = Opp.frame;
  if (opp_frame == frame_169_begin_block || opp_frame == frame_151_strike_1)
    return;
  char_frame = Char.frame;
  if (char_frame == frame_161_parry || char_frame == frame_150_parry)
  {
    if (custom->restrikeprob[guard_skill] > prandom(255))
    {
      move_6_shift();
    }
  }
  else
  {
    if (custom->strikeprob[guard_skill] > prandom(255))
    {
      move_6_shift();
    }
  }
}

// seg002:0BCD
void __pascal far hurt_by_sword()
{
  short distance;
  if (Char.alive >= 0)
    return;
  if (Char.sword != sword_2_drawn)
  {
    // Being hurt when not in fighting pose means death.
    take_hp(100);
    seqtbl_offset_char(seq_85_stabbed_to_death); // dying (stabbed unarmed)
  loc_4276:
    if (get_tile_behind_char() != 0 ||
        (distance = distance_to_edge_weight()) < 4)
    {
      seqtbl_offset_char(seq_85_stabbed_to_death); // dying (stabbed)
      if (Char.charid != charid_0_kid &&
          Char.direction < dir_0_right && // looking left
          (curr_tile2 == tiles_4_gate || get_tile_at_char() == tiles_4_gate))
      {
          Char.x = x_bump[tile_col - (curr_tile2 != tiles_4_gate) + 5] + 7;
        Char.x = char_dx_forward(10);
      }
      Char.y = y_land[Char.curr_row + 1];
      Char.fall_y = 0;
    }
    else
    {
      Char.x = char_dx_forward(distance - 20);
      load_fram_det_col();
      inc_curr_row();
      seqtbl_offset_char(seq_81_kid_pushed_off_ledge); // Kid/Guard is killed and pushed off the ledge
    }
  }
  else
  {
    // You can't hurt skeletons
    if (Char.charid != charid_4_skeleton)
    {
      if (take_hp(1))
        goto loc_4276;
    }
    seqtbl_offset_char(seq_74_hit_by_sword); // being hit with sword
    Char.y = y_land[Char.curr_row + 1];
    Char.fall_y = 0;
  }
  // sound 13: Kid hurt (by sword), sound 12: Guard hurt (by sword)
  play_seq();
}

// seg002:0CD4
void __pascal far check_sword_hurt()
{
  if (Guard.action == actions_99_hurt)
  {
    if (Kid.action == actions_99_hurt)
    {
      Kid.action = actions_1_run_jump;
    }
    loadshad();
    hurt_by_sword();
    saveshad();
    guard_refrac = custom->refractimer[guard_skill];
  }
  else
  {
    if (Kid.action == actions_99_hurt)
    {
      loadkid();
      hurt_by_sword();
      savekid();
    }
  }
}

// seg002:0D1A
void __pascal far check_sword_hurting()
{
  short kid_frame;
  kid_frame = Kid.frame;
  // frames 217..228: go up on stairs
  if (kid_frame != 0 && (kid_frame < frame_219_exit_stairs_3 || kid_frame >= 229))
  {
    loadshad_and_opp();
    check_hurting();
    saveshad_and_opp();
    loadkid_and_opp();
    check_hurting();
    savekid_and_opp();
  }
}

// seg002:0D56
void __pascal far check_hurting()
{
  short opp_frame, char_frame, distance, min_hurt_range;
  if (Char.sword != sword_2_drawn)
    return;
  if (Char.curr_row != Opp.curr_row)
    return;
  char_frame = Char.frame;
  // frames 153..154: poking with sword
  if (char_frame != frame_153_strike_3 && char_frame != frame_154_poking)
    return;
  // If char is poking ...
  distance = char_opp_dist();
  opp_frame = Opp.frame;
  // frames 161 and 150: parrying
  if (distance < 0 || distance >= 29 ||
      (opp_frame != frame_161_parry && opp_frame != frame_150_parry))
  {
    // ... and Opp is not parrying
    // frame 154: poking
    if (Char.frame == frame_154_poking)
    {
      if (Opp.sword < sword_2_drawn)
      {
        min_hurt_range = 8;
      }
      else
      {
        min_hurt_range = 12;
      }
      distance = char_opp_dist();
      if (distance >= min_hurt_range && distance < 29)
      {
        Opp.action = actions_99_hurt;
      }
    }
  }
  else
  {
    Opp.frame = frame_161_parry;
    if (Char.charid != charid_0_kid)
    {
      justblocked = 4;
    }
    seqtbl_offset_char(seq_69_attack_was_parried); // attack was parried
    play_seq();
  }
  if (Char.direction == dir_56_none)
    return; // Fix looping "sword moving" sound.
  // frame 154: poking
  // frame 161: parrying
  if (Char.frame == frame_154_poking && Opp.frame != frame_161_parry && Opp.action != actions_99_hurt)
  {
  }
}

// seg002:0E1F
void __pascal far check_skel()
{
  // Special event: skeleton wakes
  if (current_level == /*3*/ custom->skeleton_level &&
      Guard.direction == dir_56_none &&
      drawn_room == /*1*/ custom->skeleton_room &&
      (leveldoor_open != 0 || !custom->skeleton_require_open_level_door) &&
      (Kid.curr_col == /*2*/ custom->skeleton_trigger_column_1 ||
       Kid.curr_col == /*3*/ custom->skeleton_trigger_column_2))
  {
    get_tile(drawn_room, /*5*/ custom->skeleton_column, /*1*/ custom->skeleton_row);
    if (curr_tile2 == tiles_21_skeleton)
    {
      // erase skeleton
      curr_room_tiles[curr_tilepos] = tiles_1_floor;
      redraw_height = 24;
      ++curr_tilepos;
      Char.room = drawn_room;
      Char.curr_row = /*1*/ custom->skeleton_row;
      Char.y = y_land[Char.curr_row + 1];
      Char.curr_col = /*5*/ custom->skeleton_column;
      Char.x = x_bump[Char.curr_col + 5] + 14;
      Char.direction = dir_FF_left;
      seqtbl_offset_char(seq_88_skel_wake_up); // skel wake up
      play_seq();
      guard_skill = /*2*/ custom->skeleton_skill;
      Char.alive = -1;
      guardhp_max = guardhp_curr = 3;
      Char.fall_x = Char.fall_y = 0;
      is_guard_notice = guard_refrac = 0;
      Char.sword = sword_2_drawn;
      Char.charid = charid_4_skeleton;
      saveshad();
    }
  }
}

// seg002:0F3F
void __pascal far do_auto_moves(const auto_move_type *moves_ptr)
{
  short demoindex;
  short curr_move;
  if (demo_time >= 0xFE)
    return;
  ++demo_time;
  demoindex = demo_index;
  if (moves_ptr[demoindex].time <= demo_time)
  {
    ++demo_index;
  }
  else
  {
    demoindex = demo_index - 1;
  }
  curr_move = moves_ptr[demoindex].move;
  switch (curr_move)
  {
  case -1:
    break;
  case 0:
    move_0_nothing();
    break;
  case 1:
    move_1_forward();
    break;
  case 2:
    move_2_backward();
    break;
  case 3:
    move_3_up();
    break;
  case 4:
    move_4_down();
    break;
  case 5:
    move_3_up();
    move_1_forward();
    break;
  case 6:
    move_6_shift();
    break;
  case 7:
    move_7();
    break;
  }
}

// seg002:1000
void __pascal far autocontrol_shadow_level4()
{
  if (Char.room == 4)
  {
    if (Char.x < 80)
    {
      clear_char();
    }
    else
    {
      move_1_forward();
    }
  }
}

// seg002:101A
void __pascal far autocontrol_shadow_level5()
{
  if (Char.room == 24)
  {
    if (demo_time == 0)
    {
      get_tile(24, 1, 0);
      // is the door open?
      if (curr_room_modif[curr_tilepos] < 80)
        return;
      demo_index = 0;
    }
    do_auto_moves(custom->shad_drink_move);
    if (Char.x < 15)
    {
      clear_char();
    }
  }
}

// seg002:1064
void __pascal far autocontrol_shadow_level6()
{
  if (Char.room == 1 &&
      Kid.frame == frame_43_running_jump_4 && // a frame in run-jump
      Kid.x < 128)
  {
    move_6_shift();
    move_1_forward();
  }
}

// seg002:1082
void __pascal far autocontrol_shadow_level12()
{
  short opp_frame;
  short xdiff;
  if (Char.room == 15 && shadow_initialized == 0)
  {
    if (Opp.x >= 150)
    {
      do_init_shad(/*&*/ custom->init_shad_12, 7 /*fall*/);
      return;
    }
    shadow_initialized = 1;
  }
  if (Char.sword >= sword_2_drawn)
  {
    // if the Kid puts his sword away, the shadow does the same,
    // but only if the shadow was already hurt (?)
    if (offguard == 0 || guard_refrac == 0)
    {
      autocontrol_guard_active();
    }
    else
    {
      move_4_down();
    }
    return;
  }
  if (Opp.sword >= sword_2_drawn || offguard == 0)
  {
    xdiff = 0x7000; // bugfix/workaround
    // This behavior matches the DOS version but not the Apple II source.
    if (can_guard_see_kid < 2 || (xdiff = char_opp_dist()) >= 90)
    {
      if (xdiff < 0)
      {
        move_2_backward();
      }
      return;
    }
    // Shadow draws his sword
    if (Char.frame == 15)
    {
      move_down_forw();
    }
    return;
  }
  if (char_opp_dist() < 10)
  {
    // unite with the shadow
    flash_color = color_15_brightwhite; // white
    flash_time = 18;
    // get an extra HP for uniting the shadow
    add_life();
    // time of Kid-shadow flash
    united_with_shadow = 42;
    // put the Kid where the shadow was
    Char.charid = charid_0_kid;
    savekid();
    // remove the shadow
    clear_char();
    return;
  }
  if (can_guard_see_kid == 2)
  {
    // If Kid runs to shadow, shadow runs to Kid.
    opp_frame = Opp.frame;
    // frames 1..14: running
    // frames 121..132: stepping
    if ((opp_frame >= frame_3_start_run && opp_frame < frame_15_stand) ||
        (opp_frame >= frame_127_stepping_7 && opp_frame < 133))
    {
      move_1_forward();
    }
  }
}

// seg003:0000
void __pascal far init_game(int level)
{
  load_kid_sprite();
  text_time_remaining = 0;
  text_time_total = 0;
  checkpoint = 0;
  upside_down = 0; // N.B. upside_down is also reset in set_start_pos()
  resurrect_time = 0;
  if (!dont_reset_time)
  {
    rem_min = custom->start_minutes_left; // 60
    rem_tick = custom->start_ticks_left;  // 719
    hitp_beg_lev = custom->start_hitp;    // 3
  }

  need_level1_music = (level == /*1*/ custom->intro_music_level);
  play_level(level);
}

// seg003:005C
void __pascal far play_level(int level_number)
{
  if (level_number == custom->copyprot_level)
  {
    level_number = 15;
  }
  for (;;)
  {
    if (demo_mode && level_number > 2)
    {
      start_level = -1;
      need_quotes = 1;
      start_game();
    }
    if (level_number != current_level)
    {
    }
    if (level_number != current_level)
    {
      load_lev_spr(level_number);
    }
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
    have_sword = /*(level_number != 1)*/ (level_number == 0 || level_number >= custom->have_sword_from_level);
    find_start_level_door();
    // busy waiting?
    draw_level_first();
    level_number = play_level_2();
    // hacked...
    if (level_number == custom->copyprot_level && !demo_mode)
    {
      level_number = 15;
    }
    else
    {
      if (level_number == 16)
      {
        level_number = custom->copyprot_level;
        custom->copyprot_level = -1;
      }
    }
  }
}

// seg003:01A3
void __pascal far do_startpos()
{
  word x;
  // Special event: start at checkpoint
  if (current_level == /*3*/ custom->checkpoint_level && checkpoint)
  {
    level.start_dir = /*dir_FF_left*/ custom->checkpoint_respawn_dir;
    level.start_room = /*2*/ custom->checkpoint_respawn_room;
    level.start_pos = /*6*/ custom->checkpoint_respawn_tilepos;
    // Special event: remove loose floor
    get_tile(/*7*/ custom->checkpoint_clear_tile_room,
             /*4*/ custom->checkpoint_clear_tile_col,
             /*0*/ custom->checkpoint_clear_tile_row);
    curr_room_tiles[curr_tilepos] = tiles_0_empty;
  }
  next_room = Char.room = level.start_room;
  x = level.start_pos;
  Char.curr_col = x % 10;
  Char.curr_row = x / 10;
  Char.x = x_bump[Char.curr_col + 5] + 14;
  // Start in the opposite direction (and turn into the correct one).
  Char.direction = ~level.start_dir;
  if (seamless == 0)
  {
    if (current_level != 0)
    {
      x = hitp_beg_lev;
    }
    else
    {
      // HP on demo level
      x = /*4*/ custom->demo_hitp;
    }
    hitp_max = hitp_curr = x;
  }
  if (/*current_level == 1*/ custom->tbl_entry_pose[current_level] == 1)
  {
    // Special event: press button + falling entry
    get_tile(5, 2, 0);
    trigger_button(0, 0, -1);
    seqtbl_offset_char(seq_7_fall); // fall
  }
  else if (/*current_level == 13*/ custom->tbl_entry_pose[current_level] == 2)
  {
    // Special event: running entry
    seqtbl_offset_char(seq_84_run); // run
  }
  else
  {
    seqtbl_offset_char(seq_5_turn); // turn
  }
  set_start_pos();
}

// seg003:028A
void __pascal far set_start_pos()
{
  Char.y = y_land[Char.curr_row + 1];
  Char.alive = -1;
  Char.charid = charid_0_kid;
  is_screaming = 0;
  knock = 0;
  upside_down = custom->start_upside_down; // 0
  is_feather_fall = 0;
  Char.fall_y = 0;
  Char.fall_x = 0;
  offguard = 0;
  Char.sword = sword_0_sheathed;
  droppedout = 0;
  play_seq();
  if (current_level == /*7*/ custom->falling_entry_level && Char.room == /*17*/ custom->falling_entry_room)
  {
    // Special event: level 7 falling entry
    // level 7, room 17: show room below
    goto_other_room(3);
  }
  savekid();
}

// seg003:02E6
void __pascal far find_start_level_door()
{
  short tilepos;
  get_room_address(Kid.room);
  for (tilepos = 0; tilepos < 30; ++tilepos)
  {
    if ((curr_room_tiles[tilepos] & 0x1F) == tiles_16_level_door_left)
    {
      start_level_door(Kid.room, tilepos);
    }
  }
}

// seg003:0326
void __pascal far draw_level_first()
{
  next_room = Kid.room;
  check_the_end();
  if (custom->tbl_level_type[current_level])
  {
    gen_palace_wall_colors();
  }
  redraw_screen(0);
}

// seg003:037B
void __pascal far redraw_screen(int drawing_different_room)
{

  different_room = 0;
  clear_kbd_buf();
  exit_room_timer = 2;
}


// seg003:04F8
int __pascal far play_level_2()
{
  while (1)
  { // main loop
    guardhp_delta = 0;
    hitp_delta = 0;
    timers();
    play_frame();

    if (is_restart_level)
    {
      is_restart_level = 0;
      return current_level;
    }
    else
    {
      if (next_level == current_level)
      {
      }
      else
      {
        hitp_beg_lev = hitp_max;
        checkpoint = 0;

        return next_level;
      }
    }
  }
}

// seg003:0706
void __pascal far check_knock()
{
  if (knock)
  {
    do_knock(Char.room, Char.curr_row - (knock > 0));
    knock = 0;
  }
}

// seg003:0735
void __pascal far timers()
{
  if (united_with_shadow > 0)
  {
    --united_with_shadow;
    if (united_with_shadow == 0)
    {
      --united_with_shadow;
    }
  }
  if (guard_notice_timer > 0)
  {
    --guard_notice_timer;
  }
  if (resurrect_time > 0)
  {
    --resurrect_time;
  }

  if (is_feather_fall) is_feather_fall++;

  if (is_feather_fall > 225)
  {
    is_feather_fall = 0;
  }

  // Special event: mouse
  if (current_level == /*8*/ custom->mouse_level && Char.room == /*16*/ custom->mouse_room && leveldoor_open)
  {
    ++leveldoor_open;
    // time before mouse comes: 150/12=12.5 seconds
    if (leveldoor_open == /*150*/ custom->mouse_delay)
    {
      do_mouse();
    }
  }
}

// seg003:0798
void __pascal far check_mirror()
{
  if (jumped_through_mirror == -1)
  {
    jump_through_mirror();
  }
}

// seg003:080A
void __pascal far jump_through_mirror()
{
  jumped_through_mirror = 0;
  Char.charid = charid_1_shadow;
  guardhp_max = guardhp_curr = hitp_max;
  hitp_curr = 1;
}

// seg003:085B
void __pascal far check_mirror_image()
{
  short distance;
  short xpos;
  xpos = x_bump[Char.curr_col + 5] + 10;
  distance = distance_to_edge_weight();
  if (Char.direction >= dir_0_right)
  {
    distance = (~distance) + 14;
  }
  distance_mirror = distance - 2;
  Char.x = (xpos << 1) - Char.x;
  Char.direction = ~Char.direction;
}

// seg003:08AA
void __pascal far bump_into_opponent()
{
  // This is called from play_kid_frame, so char=Kid, Opp=Guard
  short distance;
  if (can_guard_see_kid >= 2 &&
      Char.sword == sword_0_sheathed && // Kid must not be in fighting pose
      Opp.sword != sword_0_sheathed &&  // but Guard must
      Opp.action < 2 &&
      Char.direction != Opp.direction // must be facing toward each other
  )
  {
    distance = char_opp_dist();
    if (ABS(distance) <= 15)
    {
      Char.y = y_land[Char.curr_row + 1];
      Char.fall_y = 0;
      seqtbl_offset_char(seq_47_bump); // bump into opponent
      play_seq();
    }
  }
}

// seg003:0913
void __pascal far pos_guards()
{
  short guard_tile;
  short room1;
  for (room1 = 0; room1 < 24; ++room1)
  {
    guard_tile = level.guards_tile[room1];
    if (guard_tile < 30)
    {
      level.guards_x[room1] = x_bump[guard_tile % 10 + 5] + 14;
      level.guards_seq_hi[room1] = 0;
    }
  }
}

// seg003:0A99
byte __pascal far get_tile_at_kid(int xpos)
{
  return get_tile(Kid.room, get_tile_div_mod_m7(xpos), Kid.curr_row);
}


// seg003:0959
void __pascal far check_can_guard_see_kid()
{
  /*
Possible results in can_guard_see_kid:
0: Guard can't see Kid
1: Guard can see Kid, but won't come
2: Guard can see Kid, and will come
*/
  short kid_frame;
  short left_pos;
  short temp;
  short right_pos;
  kid_frame = Kid.frame;
  if (Guard.charid == charid_24_mouse)
  {
    // If the prince is fighting a guard, and the player does a quickload to a state where the prince is near the mouse, the prince would draw the sword.
    // The following line prevents this.
    can_guard_see_kid = 0;
    return;
  }
  if ((Guard.charid != charid_1_shadow || current_level == 12) &&
      // frames 217..228: going up on stairs
      kid_frame != 0 && (kid_frame < frame_219_exit_stairs_3 || kid_frame >= 229) &&
      Guard.direction != dir_56_none && Kid.alive < 0 && Guard.alive < 0 && Kid.room == Guard.room && Kid.curr_row == Guard.curr_row)
  {
    can_guard_see_kid = 2;
    left_pos = x_bump[Kid.curr_col + 5] + 7;
    right_pos = x_bump[Guard.curr_col + 5] + 7;
    if (left_pos > right_pos)
    {
      temp = left_pos;
      left_pos = right_pos;
      right_pos = temp;
    }
    // A chomper is on the left side of a tile, so it doesn't count.
    if (get_tile_at_kid(left_pos) == tiles_18_chomper)
    {
      left_pos += 14;
    }
    // A gate is on the right side of a tile, so it doesn't count.
    if (get_tile_at_kid(right_pos) == tiles_4_gate
    )
    {
      right_pos -= 14;
    }
    if (right_pos >= left_pos)
    {
      while (left_pos <= right_pos)
      {
        // Can't see through these tiles.
        if (get_tile_at_kid(left_pos) == tiles_20_wall ||
            curr_tile2 == tiles_7_doortop_with_floor ||
            curr_tile2 == tiles_12_doortop)
        {
          can_guard_see_kid = 0;
          return;
        }
        // Can see through these, but won't go through them.
        if (curr_tile2 == tiles_11_loose ||
            curr_tile2 == tiles_18_chomper ||
            (curr_tile2 == tiles_4_gate && curr_room_modif[curr_tilepos] < 112) ||
            !tile_is_floor(curr_tile2))
        {
          can_guard_see_kid = 1;
        }
        left_pos += 14;
      }
    }
  }
  else
  {
    can_guard_see_kid = 0;
  }
}


// seg003:0ABA
void __pascal far do_mouse()
{
  loadkid();
  Char.charid = /*charid_24_mouse*/ custom->mouse_object;
  Char.x = /*200*/ custom->mouse_start_x;
  Char.curr_row = 0;
  Char.y = y_land[Char.curr_row + 1];
  Char.alive = -1;
  Char.direction = dir_FF_left;
  guardhp_curr = 1;
  seqtbl_offset_char(seq_105_mouse_forward); // mouse forward
  play_seq();
  saveshad();
}


// seg006:0006
int __pascal far get_tile(int room, int col, int row)
{
  curr_room = room;
  tile_col = col;
  tile_row = row;
  curr_room = find_room_of_tile();
  // bugfix: check_chomped_kid may call with room = -1
  if (curr_room > 0)
  {
    get_room_address(curr_room);
    curr_tilepos = tbl_line[tile_row] + tile_col;
    curr_tile2 = curr_room_tiles[curr_tilepos] & 0x1F;
  }
  else
  {
    // wall in room 0
    curr_tile2 = custom->level_edge_hit_tile; // tiles_20_wall
  }
  return curr_tile2;
}

// seg006:005D
int __pascal far find_room_of_tile()
{
again:
  if (tile_col < 0)
  {
    tile_col += 10;
    if (curr_room)
    {
      curr_room = level.roomlinks[curr_room - 1].left;
    }
    //find_room_of_tile();
    goto again;
  }
  if (tile_col >= 10)
  {
    tile_col -= 10;
    if (curr_room)
    {
      curr_room = level.roomlinks[curr_room - 1].right;
    }
    //find_room_of_tile();
    goto again;
  }
  // if (tile_row < 0) was here originally
  if (tile_row < 0)
  {
    tile_row += 3;
    if (curr_room)
    {
      curr_room = level.roomlinks[curr_room - 1].up;
    }
    //find_room_of_tile();
    goto again;
  }
  if (tile_row >= 3)
  {
    tile_row -= 3;
    if (curr_room)
    {
      curr_room = level.roomlinks[curr_room - 1].down;
    }
    //find_room_of_tile();
    goto again;
  }
  return curr_room;
}

// seg006:00EC
int __pascal far get_tilepos(int tile_col, int tile_row)
{
  if (tile_row < 0)
  {
    return -(tile_col + 1);
  }
  else if (tile_row >= 3 || tile_col >= 10 || tile_col < 0)
  {
    return 30;
  }
  else
  {
    return tbl_line[tile_row] + tile_col;
  }
}

// seg006:01F5
short __pascal far dx_weight()
{
  sbyte var_2;
  var_2 = cur_frame.dx - (cur_frame.flags & FRAME_WEIGHT_X);
  return char_dx_forward(var_2);
}

// seg006:0124
int __pascal far get_tilepos_nominus(int tile_col, int tile_row)
{
  short var_2;
  var_2 = get_tilepos(tile_col, tile_row);
  if (var_2 < 0)
    return 30;
  else
    return var_2;
}

// seg006:0144
void __pascal far load_fram_det_col()
{
  load_frame();
  determine_col();
}

// seg006:014D
void __pascal far determine_col()
{
  Char.curr_col = get_tile_div_mod_m7(dx_weight());
}


void get_frame_internal(const frame_type frame_table[], int frame, const char *frame_table_name, int count)
{
  if (frame >= 0 && frame < count)
  {
    cur_frame = frame_table[frame];
  }
  else
  {
    printf("Tried to use %s[%d], not in 0..%d\n", frame_table_name, frame, count - 1);
    static const frame_type blank_frame = {255, 0, 0, 0, 0};
    cur_frame = blank_frame;
  }
}

// seg006:015A
void __pascal far load_frame()
{
  short frame;
  short add_frame;
  frame = Char.frame;
  add_frame = 0;
  switch (Char.charid)
  {
  case charid_0_kid:
  case charid_24_mouse:
  use_table_kid:
    get_frame(frame_table_kid, frame);
    break;
  case charid_2_guard:
  case charid_4_skeleton:
    if (frame >= 102 && frame < 107)
      add_frame = 70;
    goto use_table_guard;
  case charid_1_shadow:
    if (frame < 150 || frame >= 190)
      goto use_table_kid;
  use_table_guard:
    get_frame(frame_tbl_guard, frame + add_frame - 149);
    break;
  case charid_5_princess:
  case charid_6_vizier:
    //  use_table_cutscene:
    get_frame(frame_tbl_cuts, frame);
    break;
  }
}

// seg006:0213
int __pascal far char_dx_forward(int delta_x)
{
  if (Char.direction < dir_0_right)
  {
    delta_x = -delta_x;
  }
  return delta_x + Char.x;
}

// seg006:0234
int __pascal far obj_dx_forward(int delta_x)
{
  if (obj_direction < dir_0_right)
  {
    delta_x = -delta_x;
  }
  obj_x += delta_x;
  return obj_x;
}

// seg006:0254
void __pascal far play_seq()
{
  for (;;)
  {
    byte item = *(SEQTBL_0 + Char.curr_seq++);
    switch (item)
    {
    case SEQ_DX: // dx
      Char.x = char_dx_forward(*(SEQTBL_0 + Char.curr_seq++));
      break;
    case SEQ_DY: // dy
      Char.y += *(SEQTBL_0 + Char.curr_seq++);
      break;
    case SEQ_FLIP: // flip
      Char.direction = ~Char.direction;
      break;
    case SEQ_JMP_IF_FEATHER: // jump if feather
      if (!is_feather_fall)
      {
        ++Char.curr_seq;
        ++Char.curr_seq;
        break;
      }
      // fallthrough!
    case SEQ_JMP: // jump
      Char.curr_seq = *(const word *)(SEQTBL_0 + Char.curr_seq);
      break;
    case SEQ_UP: // up
      --Char.curr_row;
      start_chompers();
      break;
    case SEQ_DOWN: // down
      inc_curr_row();
      start_chompers();
      break;
    case SEQ_ACTION: // action
      Char.action = *(SEQTBL_0 + Char.curr_seq++);
      break;
    case SEQ_SET_FALL: // set fall
      Char.fall_x = *(SEQTBL_0 + Char.curr_seq++);
      Char.fall_y = *(SEQTBL_0 + Char.curr_seq++);
      break;
    case SEQ_KNOCK_UP: // knock up
      knock = 1;
      break;
    case SEQ_KNOCK_DOWN: // knock down
      knock = -1;
      break;
    case SEQ_SOUND: // sound
      switch (*(SEQTBL_0 + Char.curr_seq++))
      {
      case SND_SILENT: // no sound actually played, but guards still notice the kid
        is_guard_notice = 1;
        break;
      case SND_FOOTSTEP:               // feet
        is_guard_notice = 1;
        break;
      case SND_BUMP:                // bump
        is_guard_notice = 1;
        break;
      case SND_DRINK:               // drink
        break;
      case SND_LEVEL: // level
        if (is_sound_on)
        {
          if (current_level == 4)
          {
          }
          else if (current_level != 13 && current_level != 15)
          {
          }
        }
        break;
      }
      break;
    case SEQ_END_LEVEL: // end level
      ++next_level;
      break;
    case SEQ_GET_ITEM: // get item
      if (*(SEQTBL_0 + Char.curr_seq++) == 1)
      {
        proc_get_object();
      }
      break;
    case SEQ_DIE: // nop
      break;
    default:
      Char.frame = item;
      //if (Char.frame == 185) Char.frame = 185;
      return;
    }
  }
}

// seg006:03DE
int __pascal far get_tile_div_mod_m7(int xpos)
{
  return get_tile_div_mod(xpos - 7);
}

// seg006:03F0
int __pascal far get_tile_div_mod(int xpos)
{
  // Determine tile column (xh) and the position within the tile (xl) from xpos.

  // DOS PoP does this:
  // obj_xl = tile_mod_tbl[xpos];
  // return tile_div_tbl[xpos];

  // xpos uses a coordinate system in which the left edge of the screen is 58, and each tile is 14 units wide.
  int x = xpos - 58;
  int xl = x % 14;
  int xh = x / 14;
  if (xl < 0)
  {
    // Integer division rounds towards zero, but we want to round down.
    --xh;
    // Modulo returns a negative number if x is negative, but we want 0 <= xl < 14.
    xl += 14;
  }

  // For compatibility with the DOS version, we allow for overflow access to these tables
  // Considering the case of negative overflow
  if (xpos < 0)
  {
    // In this case DOS PoP reads the bytes directly before tile_div_tbl[] and tile_mod_tbl[] in the memory.
    // Here we simulate these reads.
    // Before tile_mod_tbl[] is tile_div_tbl[], and before tile_div_tbl[] are the following bytes:
    static const byte bogus[] = {0x02, 0x00, 0x41, 0x00, 0x80, 0x00, 0xBF, 0x00, 0xFE, 0x00, 0xFF, 0x01, 0x01, 0xFF, 0xC4, 0xFF, 0x03, 0x00, 0x42, 0x00, 0x81, 0x00, 0xC0, 0x00, 0xF8, 0xFF, 0x37, 0x00, 0x76, 0x00, 0xB5, 0x00, 0xF4, 0x00};
    if (COUNT(bogus) + xpos >= 0)
    {
      xh = bogus[COUNT(bogus) + xpos];               // simulating tile_div_tbl[xpos]
      xl = tile_div_tbl[COUNT(tile_div_tbl) + xpos]; // simulating tile_mod_tbl[xpos]
    }
    else
    {
      printf("xpos = %d (< %d) out of range for simulation of index overflow!\n", xpos, -(int)COUNT(bogus));
    }
  }

  // Considering the case of positive overflow
  int tblSize = 256;

  if (xpos >= tblSize)
  {
    // In this case DOS PoP reads the bytes directly after tile_div_tbl[], that is: and tile_mod_tbl[]
    // Here we simulate these reads.
    // After tile_mod_tbl[] there are the following bytes:
    static const byte bogus[] = {0xF4, 0x02, 0x10, 0x1E, 0x2C, 0x3A, 0x48, 0x56, 0x64, 0x72, 0x80, 0x8E, 0x9C, 0xAA, 0xB8, 0xC6, 0xD4, 0xE2, 0xF0, 0xFE, 0x00, 0x0A, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x0D, 0x00, 0x00, 0x00, 0x00};
    if (xpos - tblSize < COUNT(bogus))
    {
      xh = tile_mod_tbl[xpos - tblSize]; // simulating tile_div_tbl[xpos]
      xl = bogus[xpos - tblSize];        // simulating tile_mod_tbl[xpos]
    }
    else
    {
      printf("xpos = %d (> %d) out of range for simulation of index overflow!\n", xpos, (int)COUNT(bogus) + tblSize);
    }
  }

  obj_xl = xl;
  return xh;
}

// seg006:0433
int __pascal far y_to_row_mod4(int ypos)
{
  return (ypos + 60) / 63 % 4 - 1;
}

// seg006:044F
void __pascal far loadkid()
{
  Char = Kid;
}

// seg006:0464
void __pascal far savekid()
{
  Kid = Char;
}

// seg006:0479
void __pascal far loadshad()
{
  Char = Guard;
}

// seg006:048E
void __pascal far saveshad()
{
  Guard = Char;
}

// seg006:04A3
void __pascal far loadkid_and_opp()
{
  loadkid();
  Opp = Guard;
}

// seg006:04BC
void __pascal far savekid_and_opp()
{
  savekid();
  Guard = Opp;
}

// seg006:04D5
void __pascal far loadshad_and_opp()
{
  loadshad();
  Opp = Kid;
}

// seg006:04EE
void __pascal far saveshad_and_opp()
{
  saveshad();
  Kid = Opp;
}

// seg006:0507
void __pascal far reset_obj_clip()
{
  obj_clip_left = 0;
  obj_clip_top = 0;
  obj_clip_right = 320;
  obj_clip_bottom = 192;
}

// seg006:051C
void __pascal far x_to_xh_and_xl(int xpos, sbyte *xh_addr, sbyte *xl_addr)
{
  if (xpos < 0)
  {
    *xh_addr = -((ABS(-xpos) >> 3) + 1);
    *xl_addr = -((-xpos - 1) % 8 - 7);
  }
  else
  {
    *xh_addr = ABS(xpos) >> 3;
    *xl_addr = xpos % 8;
  }
}

// seg006:057C
void __pascal far fall_accel()
{
  if (Char.action == actions_4_in_freefall)
  {
    if (is_feather_fall)
    {
      ++Char.fall_y;
      if (Char.fall_y > 4)
        Char.fall_y = 4;
    }
    else
    {
      Char.fall_y += 3;
      if (Char.fall_y > 33)
        Char.fall_y = 33;
    }
  }
}

// seg006:05AE
void __pascal far fall_speed()
{
  Char.y += Char.fall_y;
  if (Char.action == actions_4_in_freefall)
  {
    Char.x = char_dx_forward(Char.fall_x);
    load_fram_det_col();
  }
}

// seg006:05CD
void __pascal far check_action()
{
  short frame;
  short action;
  action = Char.action;
  frame = Char.frame;
  // frame 109: crouching
  if (action == actions_6_hang_straight ||
      action == actions_5_bumped)
  {
    if (frame == frame_109_crouch
    )
    {
      check_on_floor();
    }
  }
  else if (action == actions_4_in_freefall)
  {
    do_fall();
  }
  else if (action == actions_3_in_midair)
  {
    // frame 102..106: start fall + fall
    if (frame >= frame_102_start_fall_1 && frame < frame_106_fall)
    {
      check_grab();
    }
  }
  else if (action != actions_2_hang_climb)
  {
    check_on_floor();
  }
}

// seg006:0628
int __pascal far tile_is_floor(int tiletype)
{
  switch (tiletype)
  {
  case tiles_0_empty:
  case tiles_9_bigpillar_top:
  case tiles_12_doortop:
  case tiles_20_wall:
  case tiles_26_lattice_down:
  case tiles_27_lattice_small:
  case tiles_28_lattice_left:
  case tiles_29_lattice_right:
    return 0;
  default:
    return 1;
  }
}

// seg006:0658
void __pascal far check_spiked()
{
  short harmful;
  short frame;
  frame = Char.frame;
  if (get_tile(Char.room, Char.curr_col, Char.curr_row) == tiles_2_spike)
  {
    harmful = is_spike_harmful();
    // frames 7..14: running
    // frames 34..39: start run-jump
    // frame 43: land from run-jump
    // frame 26: lang from standing jump
    if (
      (harmful >= 2 && ((frame >= frame_7_run && frame < 15) || (frame >= frame_34_start_run_jump_1 && frame < 40))) ||
      ((frame == frame_43_running_jump_4 || frame == frame_26_standing_jump_11) && harmful != 0))
    {
      spiked();
    }
  }
}

// seg006:06BD
int __pascal far take_hp(int count)
{
  word dead;
  dead = 0;
  if (Char.charid == charid_0_kid)
  {
    if (count >= hitp_curr)
    {
      hitp_delta = -hitp_curr;
      dead = 1;
    }
    else
    {
      hitp_delta = -count;
    }
  }
  else
  {
    if (count >= guardhp_curr)
    {
      guardhp_delta = -guardhp_curr;
      dead = 1;
    }
    else
    {
      guardhp_delta = -count;
    }
  }
  return dead;
}

// seg006:070D
int __pascal far get_tile_at_char()
{
  return get_tile(Char.room, Char.curr_col, Char.curr_row);
}

// Get an image, with index and NULL checks.
image_type *get_image(short chtab_id, int id)
{
  if (chtab_id < 0 || chtab_id > COUNT(chtab_addrs))
  {
    //  printf("Tried to use chtab %d not in 0..%d\n", chtab_id, (int)COUNT(chtab_addrs));
    return NULL;
  }
  chtab_type *chtab = chtab_addrs[chtab_id];
  if (chtab == NULL)
  {
    //  printf("Tried to use null chtab %d\n", chtab_id);
    return NULL;
  }
  if (id < 0 || id >= chtab->n_images)
  {
    //  if (id != 255) printf("Tried to use image %d of chtab %d, not in 0..%d\n", id, chtab_id, chtab->n_images-1);
    return NULL;
  }
  return chtab->images[id];
}


// seg006:0723
void __pascal far set_char_collision()
{
  image_type *image = get_image(obj_chtab, obj_id);
  if (image == NULL)
  {
    char_width_half = 0;
    char_height = 0;
  }
  else
  {
    char_width_half = (image->/*width*/ w + 1) / 2;
    char_height = image->/*height*/ h;
  }
  char_x_left = obj_x / 2 + 58;
  if (Char.direction >= dir_0_right)
  {
    char_x_left -= char_width_half;
  }
  char_x_left_coll = char_x_left;
  char_x_right_coll = char_x_right = char_x_left + char_width_half;
  char_top_y = obj_y - char_height + 1;
  if (char_top_y >= 192)
  {
    char_top_y = 0;
  }
  char_top_row = y_to_row_mod4(char_top_y);
  char_bottom_row = y_to_row_mod4(obj_y);
  if (char_bottom_row == -1)
  {
    char_bottom_row = 3;
  }
  char_col_left = MAX(get_tile_div_mod(char_x_left), 0);
  char_col_right = MIN(get_tile_div_mod(char_x_right), 9);
  if (cur_frame.flags & FRAME_THIN)
  {
    // "thin" this frame for collision detection
    char_x_left_coll += 4;
    char_x_right_coll -= 4;
  }
}

// seg006:0815
void __pascal far check_on_floor()
{
  if (cur_frame.flags & FRAME_NEEDS_FLOOR)
  {
    if (get_tile_at_char() == tiles_20_wall)
    {
      in_wall();
    }
    if (!tile_is_floor(curr_tile2))
    {
      // Special event: floors appear
      if (current_level == 12 &&
          united_with_shadow < 0 &&
          Char.curr_row == 0 &&
          (Char.room == 2 || (Char.room == 13 && tile_col >= 6)))
      {
        curr_room_tiles[curr_tilepos] = tiles_1_floor;
        ++curr_tilepos;
      }
      else
      {
        start_fall();
      }
    }
  }
}

// seg006:08B9
void __pascal far start_fall()
{
  short frame;
  word seq_id;
  frame = Char.frame;
  Char.sword = sword_0_sheathed;
  inc_curr_row();
  start_chompers();
  fall_frame = frame;
  if (frame == frame_9_run)
  {
    // frame 9: run
    seq_id = seq_7_fall; // fall (when?)
  }
  else if (frame == frame_13_run)
  {
    // frame 13: run
    seq_id = seq_19_fall; // fall (when?)
  }
  else if (frame == frame_26_standing_jump_11)
  {
    // frame 26: land after standing jump
    seq_id = seq_18_fall_after_standing_jump; // fall after standing jump
  }
  else if (frame == frame_44_running_jump_5)
  {
    // frame 44: land after running jump
    seq_id = seq_21_fall_after_running_jump; // fall after running jump
  }
  else if (frame >= frame_81_hangdrop_1 && frame < 86)
  {
    // frame 81..85: land after jump up
    seq_id = seq_19_fall; // fall after jumping up
    Char.x = char_dx_forward(5);
    load_fram_det_col();
  }
  else if (frame >= 150 && frame < 180)
  {
    // frame 150..179: with sword + fall + dead
    if (Char.charid == charid_2_guard)
    {
      if (Char.curr_row == 3 && Char.curr_col == 10)
      {
        clear_char();
        return;
      }
      if (Char.fall_x < 0)
      {
        seq_id = seq_82_guard_pushed_off_ledge; // Guard is pushed off the ledge
        if (Char.direction < dir_0_right && distance_to_edge_weight() <= 7)
        {
          Char.x = char_dx_forward(-5);
        }
      }
      else
      {
        droppedout = 0;
        seq_id = seq_83_guard_fall; // fall after forwarding with sword
      }
    }
    else
    {
      droppedout = 1;
      if (Char.direction < dir_0_right && distance_to_edge_weight() <= 7)
      {
        Char.x = char_dx_forward(-5);
      }
      seq_id = seq_81_kid_pushed_off_ledge; // fall after backing with sword / Kid is pushed off the ledge
    }
  }
  else
  {
    seq_id = seq_7_fall; // fall after stand, run, step, crouch
  }
  seqtbl_offset_char(seq_id);
  play_seq();
  load_fram_det_col();
  if (get_tile_at_char() == tiles_20_wall)
  {
    in_wall();
    return;
  }
  int tile = get_tile_infrontof_char();
  if (tile == tiles_20_wall
  )
  {
    if (fall_frame != 44 || distance_to_edge_weight() >= 6)
    {
      Char.x = char_dx_forward(-1);
    }
    else
    {
      seqtbl_offset_char(seq_104_start_fall_in_front_of_wall); // start fall (when?)
      play_seq();
    }
    load_fram_det_col();
  }
}

// seg006:0A19
void __pascal far check_grab()
{
  word old_x;

  #define MAX_GRAB_FALLING_SPEED 32

  if (control_shift < 0 &&                    // press Shift to grab
      Char.fall_y < MAX_GRAB_FALLING_SPEED && // you can't grab if you're falling too fast ...
      Char.alive < 0 &&                       // ... or dead
      (word)y_land[Char.curr_row + 1] <= (word)(Char.y + 25))
  {
    //printf("Falling speed: %d\t x: %d\n", Char.fall_y, Char.x);
    old_x = Char.x;
    Char.x = char_dx_forward(-8);
    load_fram_det_col();
    if (!can_grab_front_above())
    {
      Char.x = old_x;
    }
    else
    {
      Char.x = char_dx_forward(distance_to_edge_weight());
      Char.y = y_land[Char.curr_row + 1];
      Char.fall_y = 0;
      seqtbl_offset_char(seq_15_grab_ledge_midair); // grab a ledge (after falling)
      play_seq();
      grab_timer = 12;
      is_screaming = 0;
    }
  }
}

// seg006:0ABD
int __pascal far can_grab_front_above()
{
  through_tile = get_tile_above_char();
  get_tile_front_above_char();
  return can_grab();
}

// seg006:0ACD
void __pascal far in_wall()
{
  short delta_x;
  delta_x = distance_to_edge_weight();
  if (delta_x >= 8 || get_tile_infrontof_char() == tiles_20_wall)
  {
    delta_x = 6 - delta_x;
  }
  else
  {
    delta_x += 4;
  }
  Char.x = char_dx_forward(delta_x);
  load_fram_det_col();
  get_tile_at_char();
}

// seg006:0B0C
int __pascal far get_tile_infrontof_char()
{
  return get_tile(Char.room, infrontx = dir_front[Char.direction + 1] + Char.curr_col, Char.curr_row);
}

// seg006:0B30
int __pascal far get_tile_infrontof2_char()
{
  short var_2;
  var_2 = dir_front[Char.direction + 1];
  return get_tile(Char.room, infrontx = (var_2 << 1) + Char.curr_col, Char.curr_row);
}

// seg006:0B66
int __pascal far get_tile_behind_char()
{
  return get_tile(Char.room, dir_behind[Char.direction + 1] + Char.curr_col, Char.curr_row);
}

// seg006:0B8A
int __pascal far distance_to_edge_weight()
{
  return distance_to_edge(dx_weight());
}

// seg006:0B94
int __pascal far distance_to_edge(int xpos)
{
  short distance;
  get_tile_div_mod_m7(xpos);
  distance = obj_xl;
  if (Char.direction == dir_0_right)
  {
    distance = 13 - distance;
  }
  return distance;
}

// seg006:0BC4
void __pascal far fell_out()
{
  if (Char.alive < 0 && Char.room == 0)
  {
    take_hp(100);
    Char.alive = 0;
    Char.frame = frame_185_dead; // dead
  }
}

// seg006:0BEE
void __pascal far play_kid()
{
  fell_out();
  control_kid();
  if (Char.alive >= 0 && is_dead())
  {
    if (resurrect_time)
    {
      loadkid();
      hitp_delta = hitp_max;
      seqtbl_offset_char(seq_2_stand); // stand
      Char.x += 8;
      play_seq();
      load_fram_det_col();
      set_start_pos();
    }
    is_show_time = 0;
    ++Char.alive;
  }
}

// seg006:0CD1
void __pascal far control_kid()
{
  if (Char.alive < 0 && hitp_curr == 0)
  {
    Char.alive = 0;
  }
  if (grab_timer != 0)
  {
    --grab_timer;
  }
  rest_ctrl_1();
  do_paused();
  read_user_control();
  user_control();
  save_ctrl_1();
}

// seg006:0D49
void __pascal far do_demo()
{
  if (checkpoint)
  {
    control_shift2 = release_arrows();
    control_forward = control_x = -1;
  }
  else if (Char.sword)
  {
    guard_skill = 10;
    autocontrol_opponent();
    guard_skill = 11;
  }
  else
  {
    do_auto_moves(custom->demo_moves);
  }
}

// seg006:0D85
void __pascal far play_guard()
{
  if (Char.charid == charid_24_mouse)
  {
    autocontrol_opponent();
  }
  else
  {
    if (Char.alive < 0)
    {
      if (guardhp_curr == 0)
      {
        Char.alive = 0;
        on_guard_killed();
      }
      else
      {
        goto loc_7A65;
      }
    }
    if (Char.charid == charid_1_shadow)
    {
      clear_char();
    }
  loc_7A65:
    autocontrol_opponent();
    control();
  }
}

// seg006:0DC0
void __pascal far user_control()
{
  if (Char.direction >= dir_0_right)
  {
    flip_control_x();
    control();
    flip_control_x();
  }
  else
  {
    control();
  }
}

// seg006:0DDC
void __pascal far flip_control_x()
{
  byte temp;
  control_x = -control_x;
  temp = control_forward;
  control_forward = control_backward;
  control_backward = temp;
}

// seg006:0E00
int __pascal far release_arrows()
{
  control_backward = control_forward = control_up = control_down = 0;
  return 1;
}

// seg006:0E12
void __pascal far save_ctrl_1()
{
  ctrl1_forward = control_forward;
  ctrl1_backward = control_backward;
  ctrl1_up = control_up;
  ctrl1_down = control_down;
  ctrl1_shift2 = control_shift2;
}

// seg006:0E31
void __pascal far rest_ctrl_1()
{
  control_forward = ctrl1_forward;
  control_backward = ctrl1_backward;
  control_up = ctrl1_up;
  control_down = ctrl1_down;
  control_shift2 = ctrl1_shift2;
}

// seg006:0E8E
void __pascal far clear_saved_ctrl()
{
  ctrl1_forward = ctrl1_backward = ctrl1_up = ctrl1_down = ctrl1_shift2 = 0;
}

// seg006:0EAF
void __pascal far read_user_control()
{
  if (control_forward >= 0)
  {
    if (control_x < 0)
    {
      if (control_forward == 0)
      {
        control_forward = -1;
      }
    }
    else
    {
      control_forward = 0;
    }
  }
  if (control_backward >= 0)
  {
    if (control_x == 1)
    {
      if (control_backward == 0)
      {
        control_backward = -1;
      }
    }
    else
    {
      control_backward = 0;
    }
  }
  if (control_up >= 0)
  {
    if (control_y < 0)
    {
      if (control_up == 0)
      {
        control_up = -1;
      }
    }
    else
    {
      control_up = 0;
    }
  }
  if (control_down >= 0)
  {
    if (control_y == 1)
    {
      if (control_down == 0)
      {
        control_down = -1;
      }
    }
    else
    {
      control_down = 0;
    }
  }
  if (control_shift2 >= 0)
  {
    if (control_shift < 0)
    {
      if (control_shift2 == 0)
      {
        control_shift2 = -1;
      }
    }
    else
    {
      control_shift2 = 0;
    }
  }
}

// seg006:0F55
int __pascal far can_grab()
{
  // Can char grab curr_tile2 through through_tile?
  byte modifier;
  modifier = curr_room_modif[curr_tilepos];
  // can't grab through wall
  if (through_tile == tiles_20_wall)
    return 0;
  // can't grab through a door top if looking right
  if (through_tile == tiles_12_doortop && Char.direction >= dir_0_right)
    return 0;
  // can't grab through floor
  if (tile_is_floor(through_tile))
    return 0;
  // can't grab a shaking loose floor
  // Allow climbing onto a shaking loose floor if the delay is greater than the default. TODO: This should be a separate option.
  if (curr_tile2 == tiles_11_loose && modifier != 0 && !(custom->loose_floor_delay > 11))
    return 0;
  // a doortop with floor can be grabbed only from the left (looking right)
  if (curr_tile2 == tiles_7_doortop_with_floor && Char.direction < dir_0_right)
    return 0;
  // can't grab something that has no floor
  if (!tile_is_floor(curr_tile2))
    return 0;
  return 1;
}

// seg006:1005
int __pascal far get_tile_above_char()
{
  return get_tile(Char.room, Char.curr_col, Char.curr_row - 1);
}

// seg006:1020
int __pascal far get_tile_behind_above_char()
{
  return get_tile(Char.room, dir_behind[Char.direction + 1] + Char.curr_col, Char.curr_row - 1);
}

// seg006:1049
int __pascal far get_tile_front_above_char()
{
  return get_tile(Char.room, infrontx = dir_front[Char.direction + 1] + Char.curr_col, Char.curr_row - 1);
}

// seg006:1072
int __pascal far back_delta_x(int delta_x)
{
  if (Char.direction < dir_0_right)
  {
    // direction = left
    return delta_x;
  }
  else
  {
    // direction = right
    return -delta_x;
  }
}

// seg006:108A
void __pascal far do_pickup(int obj_type)
{
  pickup_obj_type = obj_type;
  control_shift2 = 1;
  // erase picked up item
  curr_room_tiles[curr_tilepos] = tiles_1_floor;
  curr_room_modif[curr_tilepos] = 0;
  redraw_height = 35;
}

// seg006:10E6
void __pascal far check_press()
{
  short frame;
  short action;
  frame = Char.frame;
  action = Char.action;
  // frames 87..99: hanging
  // frames 135..140: start climb up
  if ((frame >= frame_87_hanging_1 && frame < 100) || (frame >= frame_135_climbing_1 && frame < frame_141_climbing_7))
  {
    // the pressed tile is the one that the char is grabbing
    get_tile_above_char();
  }
  else if (action == actions_7_turn || action == actions_5_bumped || action < actions_2_hang_climb)
  {
    // frame 79: jumping up
    if (frame == frame_79_jumphang && get_tile_above_char() == tiles_11_loose)
    {
      // break a loose floor from above
      make_loose_fall(1);
    }
    else
    {
      // the pressed tile is the one that the char is standing on
      if (!(cur_frame.flags & FRAME_NEEDS_FLOOR))
        return;
      get_tile_at_char();
    }
  }
  else
  {
    return;
  }
  if (curr_tile2 == tiles_15_opener || curr_tile2 == tiles_6_closer)
  {
    if (Char.alive < 0)
    {
      trigger_button(1, 0, -1);
    }
    else
    {
      died_on_button();
    }
  }
  else if (curr_tile2 == tiles_11_loose)
  {
    is_guard_notice = 1;
    make_loose_fall(1);
  }
}

// seg006:1199
void __pascal far check_spike_below()
{
  short not_finished;
  short room;
  short row;
  short col;
  short right_col;
  right_col = get_tile_div_mod_m7(char_x_right);
  if (right_col < 0)
    return;
  row = Char.curr_row;
  room = Char.room;
  for (col = get_tile_div_mod_m7(char_x_left); col <= right_col; ++col)
  {
    row = Char.curr_row;
    do
    {
      not_finished = 0;
      if (get_tile(room, col, row) == tiles_2_spike)
      {
        start_anim_spike(curr_room, curr_tilepos);
      }
      else if (
        !tile_is_floor(curr_tile2) &&
        curr_room != 0 &&
        room == curr_room
      )
      {
        ++row;
        not_finished = 1;
      }
    } while (not_finished);
  }
}

// seg006:1231
void __pascal far clip_char()
{
  short frame;
  short room;
  short action;
  short col;
  short var_A;
  short row;
  short var_E;
  frame = Char.frame;
  action = Char.action;
  room = Char.room;
  row = Char.curr_row;
  reset_obj_clip();
  // frames 217..228: going up the level door
  if (frame >= frame_224_exit_stairs_8 && frame < 229)
  {
    obj_clip_top = leveldoor_ybottom + 1;
    obj_clip_right = leveldoor_right;
  }
  else
  {
    if (
      get_tile(room, char_col_left, char_top_row) == tiles_20_wall ||
      tile_is_floor(curr_tile2))
    {
      // frame 79: jump up, frame 81: grab
      if ((action == actions_0_stand && (frame == frame_79_jumphang || frame == frame_81_hangdrop_1)) ||
          get_tile(room, char_col_right, char_top_row) == tiles_20_wall ||
          tile_is_floor(curr_tile2))
      {
        var_E = row + 1;
        if (var_E == 1 ||
            ((var_A = y_clip[var_E]) < obj_y && var_A - 15 < char_top_y))
        {
          obj_clip_top = char_top_y = y_clip[var_E];
        }
      }
    }
    col = get_tile_div_mod(char_x_left_coll - 4);
    if (get_tile(room, col + 1, row) == tiles_7_doortop_with_floor ||
        curr_tile2 == tiles_12_doortop)
    {
      obj_clip_right = (tile_col << 5) + 32;
    }
    else
    {
      if ((get_tile(room, col, row) != tiles_7_doortop_with_floor &&
           curr_tile2 != tiles_12_doortop) ||
          action == actions_3_in_midair ||
          (action == actions_4_in_freefall && frame == frame_106_fall) ||
          (action == actions_5_bumped && frame == frame_107_fall_land_1) ||
          (Char.direction < dir_0_right && (action == actions_2_hang_climb ||
                                            action == actions_6_hang_straight ||
                                            (action == actions_1_run_jump &&
                                             frame >= frame_137_climbing_3 && frame < frame_140_climbing_6))))
      {
        if (
          (get_tile(room, col = get_tile_div_mod(char_x_right_coll), row) == tiles_20_wall ||
           (curr_tile2 == tiles_13_mirror && Char.direction == dir_0_right)) &&
          (get_tile(room, col, char_top_row) == tiles_20_wall ||
           curr_tile2 == tiles_13_mirror) &&
          room == curr_room)
        {
          obj_clip_right = tile_col << 5;
        }
      }
      else
      {
        obj_clip_right = (tile_col << 5) + 32;
      }
    }
  }
}

// seg006:13E6
void __pascal far stuck_lower()
{
  if (get_tile_at_char() == tiles_5_stuck)
  {
    ++Char.y;
  }
}

// seg006:13F3
void __pascal far set_objtile_at_char()
{
  short char_frame;
  short char_action;
  char_frame = Char.frame;
  char_action = Char.action;
  if (char_action == actions_1_run_jump)
  {
    tile_row = char_bottom_row;
    tile_col = char_col_left;
  }
  else
  {
    tile_row = Char.curr_row;
    tile_col = Char.curr_col;
  }
  // frame 135..148: climbing
  if ((char_frame >= frame_135_climbing_1 && char_frame < 149) ||
      char_action == actions_2_hang_climb ||
      char_action == actions_3_in_midair ||
      char_action == actions_4_in_freefall ||
      char_action == actions_6_hang_straight)
  {
    --tile_col;
  }
  obj_tilepos = get_tilepos_nominus(tile_col, tile_row);
  //printf("set_objtile_at_char: obj_tile = %d\n", obj_tile); // debug
}

// seg006:1463
void __pascal far proc_get_object()
{
  if (Char.charid != charid_0_kid || pickup_obj_type == 0)
    return;
  if (pickup_obj_type == -1)
  {
    have_sword = -1;
    flash_color = color_14_brightyellow;
    flash_time = 8;
  }
  else
  {
    switch (--pickup_obj_type)
    {
    case 0: // health
      if (hitp_curr != hitp_max)
      {
        hitp_delta = 1;
        flash_color = color_4_red;
        flash_time = 2;
      }
      break;
    case 1: // life
      flash_color = color_4_red;
      flash_time = 4;
      add_life();
      break;
    case 2: // feather
      feather_fall();
      break;
    case 3: // invert
      toggle_upside();
      break;
    case 5: // open
      get_tile(8, 0, 0);
      trigger_button(0, 0, -1);
      break;
    case 4: // hurt
      // Special event: blue potions on potions level take half of HP
      if (current_level == 15)
      {
        hitp_delta = -((hitp_max + 1) >> 1);
      }
      else
      {
        hitp_delta = -1;
      }
      break;
    }
  }
}

// seg006:1599
int __pascal far is_dead()
{
  // 177: spiked, 178: chomped, 185: dead
  // or maybe this was a switch-case?
  return Char.frame >= frame_177_spiked && (Char.frame <= frame_178_chomped || Char.frame == frame_185_dead);
}


// seg006:15E8
void __pascal far on_guard_killed()
{
  if (current_level == 0)
  {
    // demo level: after killing Guard, run out of room
    checkpoint = 1;
    demo_index = demo_time = 0;
  }
  else if (current_level == /*13*/ custom->jaffar_victory_level)
  {
    // Jaffar's level: flash
    flash_color = color_15_brightwhite; // white
    flash_time = /*18*/ custom->jaffar_victory_flash_time;
    is_show_time = 1;
    leveldoor_open = 2;
  }
  else if (Char.charid != charid_1_shadow)
  {
  }
}

// seg006:1634
void __pascal far clear_char()
{
  Char.direction = dir_56_none;
  Char.alive = 0;
  Char.action = 0;
  guardhp_curr = 0;
}

// data:42EC
byte obj2_tilepos;
// data:34A6
word obj2_x;
// data:34A8
byte obj2_y;
// data:599E
sbyte obj2_direction;
// data:5948
byte obj2_id;
// data:42BE
byte obj2_chtab;
// data:4D90
short obj2_clip_top;
// data:460C
short obj2_clip_bottom;
// data:4C94
short obj2_clip_left;
// data:4CDE
short obj2_clip_right;

// seg006:1654
void __pascal far save_obj()
{
  obj2_tilepos = obj_tilepos;
  obj2_x = obj_x;
  obj2_y = obj_y;
  obj2_direction = obj_direction;
  obj2_id = obj_id;
  obj2_chtab = obj_chtab;
  obj2_clip_top = obj_clip_top;
  obj2_clip_bottom = obj_clip_bottom;
  obj2_clip_left = obj_clip_left;
  obj2_clip_right = obj_clip_right;
}

// seg006:1691
void __pascal far load_obj()
{
  obj_tilepos = obj2_tilepos;
  obj_x = obj2_x;
  obj_y = obj2_y;
  obj_direction = obj2_direction;
  obj_id = obj2_id;
  obj_chtab = obj2_chtab;
  obj_clip_top = obj2_clip_top;
  obj_clip_bottom = obj2_clip_bottom;
  obj_clip_left = obj2_clip_left;
  obj_clip_right = obj2_clip_right;
}

// seg006:16CE
void __pascal far draw_hurt_splash()
{
  short frame;
  frame = Char.frame;
  if (frame != frame_178_chomped)
  { // chomped
    save_obj();
    obj_tilepos = -1;
    // frame 185: dead
    // frame 106..110: fall + land
    if (frame == frame_185_dead || (frame >= frame_106_fall && frame < 111))
    {
      obj_y += 4;
      obj_dx_forward(5);
    }
    else if (frame == frame_177_spiked)
    { // spiked
      obj_dx_forward(-5);
    }
    else
    {
      obj_y -= ((Char.charid == charid_0_kid) << 2) + 11;
      obj_dx_forward(5);
    }
    if (Char.charid == charid_0_kid)
    {
      obj_chtab = id_chtab_2_kid;
      obj_id = 218; // splash!
    }
    else
    {
      obj_chtab = id_chtab_5_guard;
      obj_id = 1; // splash!
    }
    reset_obj_clip();
    load_obj();
  }
}

// seg006:175D
void __pascal far check_killed_shadow()
{
  // Special event: killed the shadow
  if (current_level == 12)
  {
    if ((Char.charid | Opp.charid) == charid_1_shadow &&
        Char.alive < 0 && Opp.alive >= 0)
    {
      flash_color = color_15_brightwhite; // white
      flash_time = 5;
      take_hp(100);
    }
  }
}

// seg006:1827
void __pascal far control_guard_inactive()
{
  if (Char.frame == frame_166_stand_inactive && control_down < 0)
  {
    if (control_forward < 0)
    {
      draw_sword();
    }
    else
    {
      control_down = 1;
      seqtbl_offset_char(seq_80_stand_flipped); // stand flipped
    }
  }
}

// seg006:1852
int __pascal far char_opp_dist()
{
  // >0 if Opp is in front of char
  // <0 if Opp is behind char
  short distance;
  if (Char.room != Opp.room)
  {
    return 999;
  }
  distance = Opp.x - Char.x;
  if (Char.direction < dir_0_right)
  {
    distance = -distance;
  }
  if (distance >= 0 && Char.direction != Opp.direction)
  {
    distance += 13;
  }
  return distance;
}

// seg006:189B
void __pascal far inc_curr_row()
{
  ++Char.curr_row;
}

// seg004:0004
void __pascal far check_collisions()
{
  short column;
  bump_col_left_of_wall = bump_col_right_of_wall = -1;
  if (Char.action == actions_7_turn)
    return;
  collision_row = Char.curr_row;
  move_coll_to_prev();
  prev_collision_row = collision_row;
  right_checked_col = MIN(get_tile_div_mod_m7(char_x_right_coll) + 2, 11);
  left_checked_col = get_tile_div_mod_m7(char_x_left_coll) - 1;
  get_row_collision_data(collision_row, curr_row_coll_room, curr_row_coll_flags);
  get_row_collision_data(collision_row + 1, below_row_coll_room, below_row_coll_flags);
  get_row_collision_data(collision_row - 1, above_row_coll_room, above_row_coll_flags);
  for (column = 9; column >= 0; --column)
  {
    if (curr_row_coll_room[column] >= 0 &&
        prev_coll_room[column] == curr_row_coll_room[column])
    {
      // char bumps into left of wall
      if (
        (prev_coll_flags[column] & 0x0F) == 0 &&
        (curr_row_coll_flags[column] & 0x0F) != 0)
      {
        bump_col_left_of_wall = column;
      }
      // char bumps into right of wall
      if (
        (prev_coll_flags[column] & 0xF0) == 0 &&
        (curr_row_coll_flags[column] & 0xF0) != 0)
      {
        bump_col_right_of_wall = column;
      }
    }
  }
}

// seg004:00DF
void __pascal far move_coll_to_prev()
{
  sbyte *row_coll_room_ptr;
  byte *row_coll_flags_ptr;
  short column;
  if (collision_row == prev_collision_row ||
      collision_row + 3 == prev_collision_row ||
      collision_row - 3 == prev_collision_row)
  {
    row_coll_room_ptr = curr_row_coll_room;
    row_coll_flags_ptr = curr_row_coll_flags;
  }
  else if (
    collision_row + 1 == prev_collision_row ||
    collision_row - 2 == prev_collision_row)
  {
    row_coll_room_ptr = above_row_coll_room;
    row_coll_flags_ptr = above_row_coll_flags;
  }
  else
  {
    row_coll_room_ptr = below_row_coll_room;
    row_coll_flags_ptr = below_row_coll_flags;
  }
  for (column = 0; column < 10; ++column)
  {
    prev_coll_room[column] = row_coll_room_ptr[column];
    prev_coll_flags[column] = row_coll_flags_ptr[column];
    below_row_coll_room[column] = -1;
    above_row_coll_room[column] = -1;
    curr_row_coll_room[column] = -1;
  }
}

// seg004:0185
void __pascal far get_row_collision_data(short row, sbyte *row_coll_room_ptr, byte *row_coll_flags_ptr)
{
  short right_wall_xpos;
  byte curr_flags;
  short room;
  short column;
  short left_wall_xpos;
  room = Char.room;
  coll_tile_left_xpos = x_bump[left_checked_col + 5] + 7;
  for (column = left_checked_col; column <= right_checked_col; ++column)
  {
    left_wall_xpos = get_left_wall_xpos(room, column, row);
    right_wall_xpos = get_right_wall_xpos(room, column, row);
    // char bumps into left of wall
    curr_flags = (left_wall_xpos < char_x_right_coll) * 0x0F;
    // char bumps into right of wall
    curr_flags |= (right_wall_xpos > char_x_left_coll) * 0xF0;
    row_coll_flags_ptr[tile_col] = curr_flags;
    row_coll_room_ptr[tile_col] = curr_room;
    coll_tile_left_xpos += 14;
  }
}

// seg004:0226
int __pascal far get_left_wall_xpos(int room, int column, int row)
{
  short type;
  type = wall_type(get_tile(room, column, row));
  if (type)
  {
    return wall_dist_from_left[type] + coll_tile_left_xpos;
  }
  else
  {
    return 0xFF;
  }
}

// seg004:025F
int __pascal far get_right_wall_xpos(int room, int column, int row)
{
  short type;
  type = wall_type(get_tile(room, column, row));
  if (type)
  {
    return coll_tile_left_xpos - wall_dist_from_right[type] + 13;
  }
  else
  {
    return 0;
  }
}

// seg004:029D
void __pascal far check_bumped()
{
  if (
    Char.action != actions_2_hang_climb &&
    Char.action != actions_6_hang_straight &&
    // frames 135..149: climb up
    (Char.frame < frame_135_climbing_1 || Char.frame >= 149))
  {
    if (bump_col_left_of_wall >= 0)
    {
      check_bumped_look_right();
    }
    else if (bump_col_right_of_wall >= 0)
    {
      check_bumped_look_left();
    }
  }
}

// seg004:02D2
void __pascal far check_bumped_look_left()
{
  if ((Char.sword == sword_2_drawn || Char.direction < dir_0_right) && // looking left
      is_obstacle_at_col(bump_col_right_of_wall))
  {
    bumped(get_right_wall_xpos(curr_room, tile_col, tile_row) - char_x_left_coll, dir_0_right);
  }
}

// seg004:030A
void __pascal far check_bumped_look_right()
{
  if ((Char.sword == sword_2_drawn || Char.direction == dir_0_right) && // looking right
      is_obstacle_at_col(bump_col_left_of_wall))
  {
    bumped(get_left_wall_xpos(curr_room, tile_col, tile_row) - char_x_right_coll, dir_FF_left);
  }
}

// seg004:0343
int __pascal far is_obstacle_at_col(int tile_col)
{
  short tile_row;
  tile_row = Char.curr_row;
  if (tile_row < 0)
  {
    tile_row += 3;
  }
  if (tile_row >= 3)
  {
    tile_row -= 3;
  }
  get_tile(curr_row_coll_room[tile_col], tile_col, tile_row);
  return is_obstacle();
}

// seg004:037E
int __pascal far is_obstacle()
{
  if (curr_tile2 == tiles_10_potion)
  {
    return 0;
  }
  else if (curr_tile2 == tiles_4_gate)
  {
    if (!can_bump_into_gate())
      return 0;
  }
  else if (curr_tile2 == tiles_18_chomper)
  {
    // is the chomper closed?
    if (curr_room_modif[curr_tilepos] != 2)
      return 0;
  }
  else if (
    curr_tile2 == tiles_13_mirror &&
    Char.charid == charid_0_kid &&
    Char.frame >= frame_39_start_run_jump_6 && Char.frame < frame_44_running_jump_5 && // run-jump
    Char.direction < dir_0_right                                                       // right-to-left only
  )
  {
    curr_room_modif[curr_tilepos] = 0x56; // broken mirror or what?
    jumped_through_mirror = -1;
    return 0;
  }
  coll_tile_left_xpos = xpos_in_drawn_room(x_bump[tile_col + 5]) + 7;
  return 1;
}

// seg004:0405
int __pascal far xpos_in_drawn_room(int xpos)
{
  if (curr_room != drawn_room)
  {
    if (curr_room == room_L || curr_room == room_BL)
    {
      xpos -= 140;
    }
    else if (curr_room == room_R || curr_room == room_BR)
    {
      xpos += 140;
    }
  }
  return xpos;
}

// seg004:0448
void __pascal far bumped(sbyte delta_x, sbyte push_direction)
{
  // frame 177: spiked
  if (Char.alive < 0 && Char.frame != frame_177_spiked)
  {
    Char.x += delta_x;
    if (push_direction < dir_0_right)
    {
      // pushing left
      if (curr_tile2 == tiles_20_wall)
      {
        get_tile(curr_room, --tile_col, tile_row);
      }
    }
    else
    {
      // pushing right
      if (curr_tile2 == tiles_12_doortop ||
          curr_tile2 == tiles_7_doortop_with_floor ||
          curr_tile2 == tiles_20_wall)
      {
        ++tile_col;
        if (curr_room == 0 && tile_col == 10)
        {
          curr_room = Char.room;
          tile_col = 0;
        }
        get_tile(curr_room, tile_col, tile_row);
      }
    }
    if (tile_is_floor(curr_tile2))
    {
      bumped_floor(push_direction);
    }
    else
    {
      bumped_fall();
    }
  }
}

// seg004:0A10
int __pascal far dist_from_wall_forward(byte tiletype)
{
  short type;
  if (tiletype == tiles_4_gate && !can_bump_into_gate())
  {
    return -1;
  }
  else
  {
    coll_tile_left_xpos = x_bump[tile_col + 5] + 7;
    type = wall_type(tiletype);
    if (type == 0)
      return -1;
    if (Char.direction < dir_0_right)
    {
      // looking left
      //return wall_dist_from_right[type] + char_x_left_coll - coll_tile_left_xpos - 13;
      return char_x_left_coll - (coll_tile_left_xpos + 13 - wall_dist_from_right[type]);
    }
    else
    {
      // looking right
      return wall_dist_from_left[type] + coll_tile_left_xpos - char_x_right_coll;
    }
  }
}


// seg004:04E4
void __pascal far bumped_fall()
{
  short action;
  action = Char.action;
  Char.x = char_dx_forward(-4);
  if (action == actions_4_in_freefall)
  {
    Char.fall_x = 0;
  }
  else
  {
    seqtbl_offset_char(seq_45_bumpfall); // fall after bumped
    play_seq();
  }
  bumped_sound();
}

// seg004:0520
void __pascal far bumped_floor(sbyte push_direction)
{
  short frame;
  short seq_index;
  if (Char.sword != sword_2_drawn && (word)(y_land[Char.curr_row + 1] - Char.y) >= (word)15)
  {
    bumped_fall();
  }
  else
  {
    Char.y = y_land[Char.curr_row + 1];
    if (Char.fall_y >= 22)
    {
      Char.x = char_dx_forward(-5);
    }
    else
    {
      Char.fall_y = 0;
      if (Char.alive)
      {
        if (Char.sword == sword_2_drawn)
        {
          if (push_direction == Char.direction)
          {
            seqtbl_offset_char(seq_65_bump_forward_with_sword); // pushed forward with sword (Kid)
            play_seq();
            Char.x = char_dx_forward(1);
            return;
          }
          else
          {
            seq_index = seq_64_pushed_back_with_sword; // pushed back with sword
          }
        }
        else
        {
          frame = Char.frame;
          if (frame == 24 || frame == 25 ||
              (frame >= 40 && frame < 43) ||
              (frame >= frame_102_start_fall_1 && frame < 107))
          {
            seq_index = seq_46_hardbump; // bump into wall after run-jump (crouch)
          }
          else
          {
            seq_index = seq_47_bump; // bump into wall
          }
        }
        seqtbl_offset_char(seq_index);
        play_seq();
        bumped_sound();
      }
    }
  }
}

// seg004:05F1
void __pascal far bumped_sound()
{
  is_guard_notice = 1;
}

// seg004:0601
void __pascal far clear_coll_rooms()
{
  memset_near(prev_coll_room, -1, sizeof(prev_coll_room));
  memset_near(curr_row_coll_room, -1, sizeof(curr_row_coll_room));
  memset_near(below_row_coll_room, -1, sizeof(below_row_coll_room));
  memset_near(above_row_coll_room, -1, sizeof(above_row_coll_room));
  prev_collision_row = -1;
}

// seg004:0657
int __pascal far can_bump_into_gate()
{
  return (curr_room_modif[curr_tilepos] >> 2) + 6 < char_height;
}

// seg004:067C
int __pascal far get_edge_distance()
{
  /*
Possible results in edge_type:
0: closer/sword/potion
1: edge
2: floor (nothing near char)
*/
  short distance;
  byte tiletype;
  determine_col();
  load_frame_to_obj();
  set_char_collision();
  tiletype = get_tile_at_char();
  if (wall_type(tiletype) != 0)
  {
    tile_col = Char.curr_col;
    distance = dist_from_wall_forward(tiletype);
    if (distance >= 0)
    {
    loc_59DD:
      if (distance < 14)
      {
        edge_type = 1;
      }
      else
      {
        edge_type = 2;
        distance = 11;
      }
    }
    else
    {
      goto loc_59E8;
    }
  }
  else
  {
  loc_59E8:
    tiletype = get_tile_infrontof_char();
    if (tiletype == tiles_12_doortop && Char.direction >= dir_0_right)
    {
    loc_59FB:
      edge_type = 0;
      distance = distance_to_edge_weight();
    }
    else
    {
      if (wall_type(tiletype) != 0)
      {
        tile_col = infrontx;
        distance = dist_from_wall_forward(tiletype);
        if (distance >= 0)
          goto loc_59DD;
      }
      if (tiletype == tiles_11_loose)
        goto loc_59FB;
      if (
        tiletype == tiles_6_closer ||
        tiletype == tiles_22_sword ||
        tiletype == tiles_10_potion)
      {
        distance = distance_to_edge_weight();
        if (distance != 0)
        {
          edge_type = 0;
        }
        else
        {
          edge_type = 2;
          distance = 11;
        }
      }
      else
      {
        if (tile_is_floor(tiletype))
        {
          edge_type = 2;
          distance = 11;
        }
        else
        {
          goto loc_59FB;
        }
      }
    }
  }
  curr_tile2 = tiletype;
  return distance;
}

// seg004:076B
void __pascal far check_chomped_kid()
{
  short tile_col;
  short tile_row;
  tile_row = Char.curr_row;
  for (tile_col = 0; tile_col < 10; ++tile_col)
  {
    if (curr_row_coll_flags[tile_col] == 0xFF &&
        get_tile(curr_row_coll_room[tile_col], tile_col, tile_row) == tiles_18_chomper &&
        (curr_room_modif[curr_tilepos] & 0x7F) == 2 // closed chomper
    )
    {
      chomped();
    }
  }
}

// seg004:07BF
void __pascal far chomped()
{
    curr_room_modif[curr_tilepos] |= 0x80; // put blood
  if (Char.frame != frame_178_chomped && Char.room == curr_room)
  {
      Char.x = x_bump[tile_col + 5] + 7;
    Char.x = char_dx_forward(7 - !Char.direction);
    Char.y = y_land[Char.curr_row + 1];
    take_hp(100);
    seqtbl_offset_char(seq_54_chomped); // chomped
    play_seq();
  }
}

// seg004:0A7B
int __pascal far dist_from_wall_behind(byte tiletype)
{
  short type;
  type = wall_type(tiletype);
  if (type == 0)
  {
    return 99;
  }
  else
  {
    if (Char.direction >= dir_0_right)
    {
      // looking right
      //return wall_dist_from_right[type] + char_x_left_coll - coll_tile_left_xpos - 13;
      return char_x_left_coll - (coll_tile_left_xpos + 13 - wall_dist_from_right[type]);
    }
    else
    {
      // looking left
      return wall_dist_from_left[type] + coll_tile_left_xpos - char_x_right_coll;
    }
  }
}


// seg004:0833
void __pascal far check_gate_push()
{
  // Closing gate pushes Kid
  short frame;
  short orig_col;
  frame = Char.frame;
  if (Char.action == actions_7_turn ||
      frame == frame_15_stand ||                      // stand
      (frame >= frame_108_fall_land_2 && frame < 111) // crouch
  )
  {
    get_tile_at_char();
    orig_col = tile_col;
    if ((curr_tile2 == tiles_4_gate ||
         get_tile(curr_room, --tile_col, tile_row) == tiles_4_gate) &&
        (curr_row_coll_flags[tile_col] & prev_coll_flags[tile_col]) == 0xFF &&
        can_bump_into_gate())
    {
      bumped_sound();
      //printf("check_gate_push: orig_col = %d, tile_col = %d, curr_room = %d, Char.room = %d, orig_room = %d\n", orig_col, tile_col, curr_room, Char.room, orig_room);
      // push Kid left if orig_col <= tile_col, gate at char's tile
      // push Kid right if orig_col > tile_col, gate is left from char's tile
      Char.x += 5 - (orig_col <= tile_col) * 10;
    }
  }
}

// seg004:08C3
void __pascal far check_guard_bumped()
{
  if (
    Char.action == actions_1_run_jump &&
    Char.alive < 0 &&
    Char.sword >= sword_2_drawn)
  {
    if (

      get_tile_at_char() == tiles_20_wall ||
      curr_tile2 == tiles_7_doortop_with_floor ||
      (curr_tile2 == tiles_4_gate && can_bump_into_gate()) ||
      (Char.direction >= dir_0_right && (get_tile(curr_room, --tile_col, tile_row) == tiles_7_doortop_with_floor ||
                                         (curr_tile2 == tiles_4_gate && can_bump_into_gate()))))
    {
      load_frame_to_obj();
      set_char_collision();
      if (is_obstacle())
      {
        short delta_x;
        delta_x = dist_from_wall_behind(curr_tile2);
        if (delta_x < 0 && delta_x > -13)
        {
          Char.x = char_dx_forward(-delta_x);
          seqtbl_offset_char(seq_65_bump_forward_with_sword); // pushed to wall with sword (Guard)
          play_seq();
          load_fram_det_col();
        }
      }
    }
  }
}

// seg004:0989
void __pascal far check_chomped_guard()
{
  get_tile_at_char();
  if (!check_chomped_here())
  {
    get_tile(curr_room, ++tile_col, tile_row);
    check_chomped_here();
  }
}

// seg004:09B0
int __pascal far check_chomped_here()
{
  if (curr_tile2 == tiles_18_chomper &&
      (curr_room_modif[curr_tilepos] & 0x7F) == 2)
  {
    coll_tile_left_xpos = x_bump[tile_col + 5] + 7;
    if (get_left_wall_xpos(curr_room, tile_col, tile_row) < char_x_right_coll &&
        get_right_wall_xpos(curr_room, tile_col, tile_row) > char_x_left_coll)
    {
      chomped();
      return 1;
    }
    else
    {
      return 0;
    }
  }
  else
  {
    return 0;
  }
}

// seg007:0000
void __pascal far process_trobs()
{
  word need_delete;
  word index;
  word new_index;
  need_delete = 0;
  if (trobs_count == 0)
    return;
  for (index = 0; index < trobs_count; ++index)
  {
    trob = trobs[index];
    animate_tile();
    trobs[index].type = trob.type;
    if (trob.type < 0)
    {
      need_delete = 1;
    }
  }
  if (need_delete)
  {
    for (index = new_index = 0; index < trobs_count; ++index)
    {
      if (trobs[index].type >= 0)
      {
        trobs[new_index++] = trobs[index];
      }
    }
    trobs_count = new_index;
  }
}


// seg007:00AF
void __pascal far animate_tile()
{
  get_room_address(trob.room);
  switch (get_curr_tile(trob.tilepos))
  {
  case tiles_19_torch:
  case tiles_30_torch_with_debris:
    animate_torch();
    break;
  case tiles_6_closer:
  case tiles_15_opener:
    animate_button();
    break;
  case tiles_2_spike:
    animate_spike();
    break;
  case tiles_11_loose:
    animate_loose();
    break;
  case tiles_0_empty:
    animate_empty();
    break;
  case tiles_18_chomper:
    animate_chomper();
    break;
  case tiles_4_gate:
    animate_door();
    break;
  case tiles_16_level_door_left:
    animate_leveldoor();
    break;
  case tiles_10_potion:
    animate_potion();
    break;
  case tiles_22_sword:
    animate_sword();
    break;
  default:
    trob.type = -1;
    break;
  }
  curr_room_modif[trob.tilepos] = curr_modifier;
}

// seg007:0166
short __pascal far is_trob_in_drawn_room()
{
  if (trob.room != drawn_room)
  {
    trob.type = -1;
    return 0;
  }
  else
  {
    return 1;
  }
}

// seg007:0258
short __pascal far get_trob_pos_in_drawn_room()
{
  short tilepos;
  tilepos = trob.tilepos;
  if (trob.room == room_A)
  {
    if (tilepos >= 20 && tilepos < 30)
    {
      // 20..29 -> -1..-10
      tilepos = 19 - tilepos;
    }
    else
    {
      tilepos = 30;
    }
  }
  else
  {
    if (trob.room != drawn_room)
    {
      tilepos = 30;
    }
  }
  return tilepos;
}

// seg007:029D
short __pascal far get_trob_right_pos_in_drawn_room()
{
  word tilepos;
  tilepos = trob.tilepos;
  if (trob.room == drawn_room)
  {
    if (tilepos % 10 != 9)
    {
      ++tilepos;
    }
    else
    {
      tilepos = 30;
    }
  }
  else if (trob.room == room_L)
  {
    if (tilepos % 10 == 9)
    {
      tilepos -= 9;
    }
    else
    {
      tilepos = 30;
    }
  }
  else if (trob.room == room_A)
  {
    if (tilepos >= 20 && tilepos < 29)
    {
      // 20..28 -> -2..-10
      tilepos = 18 - tilepos;
    }
    else
    {
      tilepos = 30;
    }
  }
  else if (trob.room == room_AL && tilepos == 29)
  {
    tilepos = -1;
  }
  else
  {
    tilepos = 30;
  }
  return tilepos;
}

// seg007:032C
short __pascal far get_trob_right_above_pos_in_drawn_room()
{
  word tilepos;
  tilepos = trob.tilepos;
  if (trob.room == drawn_room)
  {
    if (tilepos % 10 != 9)
    {
      if (tilepos < 10)
      {
        // 0..8 -> -2..-10
        tilepos = -(tilepos + 2);
      }
      else
      {
        tilepos -= 9;
      }
    }
    else
    {
      tilepos = 30;
    }
  }
  else if (trob.room == room_L)
  {
    if (tilepos == 9)
    {
      tilepos = -1;
    }
    else
    {
      if (tilepos % 10 == 9)
      {
        tilepos -= 19;
      }
      else
      {
        tilepos = 30;
      }
    }
  }
  else if (trob.room == room_B)
  {
    if (tilepos < 9)
    {
      tilepos += 21;
    }
    else
    {
      tilepos = 30;
    }
  }
  else if (trob.room == room_BL && tilepos == 9)
  {
    tilepos = 20;
  }
  else
  {
    tilepos = 30;
  }
  return tilepos;
}

// seg007:06CD
short __pascal far get_torch_frame(short curr)
{
  short next;
  next = prandom(255);
  if (next != curr)
  {
    if (next < 9)
    {
      return next;
    }
    else
    {
      next = curr;
    }
  }
  ++next;
  if (next >= 9)
    next = 0;
  return next;
}


// seg007:03CF
void __pascal far animate_torch()
{
  //if (is_trob_in_drawn_room()) {
  // Keep animating torches in the rightmost column of the left-side room as well, because they are visible in the current room.
  if (trob.room == drawn_room || (trob.room == room_L && (trob.tilepos % 10) == 9))
  {
    curr_modifier = get_torch_frame(curr_modifier);
  }
  else
  {
    trob.type = -1;
  }
}

// seg007:06AD
short __pascal far bubble_next_frame(short curr)
{
  short next;
  next = curr + 1;
  if (next >= 8)
    next = 1;
  return next;
}

// seg007:03E9
void __pascal far animate_potion()
{
  word type;
  if (trob.type >= 0 && is_trob_in_drawn_room())
  {
    type = curr_modifier & 0xF8;
    curr_modifier = bubble_next_frame(curr_modifier & 0x07) | type;
  }
}

// seg007:0425
void __pascal far animate_sword()
{
  if (is_trob_in_drawn_room())
  {
    --curr_modifier;
    if (curr_modifier == 0)
    {
      curr_modifier = (prandom(255) & 0x3F) + 0x28;
    }
  }
}

// seg007:0448
void __pascal far animate_chomper()
{
  word blood;
  word frame;
  if (trob.type >= 0)
  {
    blood = curr_modifier & 0x80;
    frame = (curr_modifier & 0x7F) + 1;
    if (frame > /*15*/ custom->chomper_speed)
    {
      frame = 1;
    }
    curr_modifier = blood | frame;
    if (frame == 2)
    {
    }
    // If either:
    // - Kid left this room
    // - Kid left this row
    // - Kid died but not in this chomper
    // and chomper is past frame 6
    // then stop.
    if ((trob.room != drawn_room || trob.tilepos / 10 != Kid.curr_row ||
         (Kid.alive >= 0 && blood == 0)) &&
        (curr_modifier & 0x7F) >= 6)
    {
      trob.type = -1;
    }
  }
}

// seg007:04D3
void __pascal far animate_spike()
{
  if (trob.type >= 0)
  {
    // 0xFF means a disabled spike.
    if (curr_modifier == 0xFF)
      return;
    if (curr_modifier & 0x80)
    {
      --curr_modifier;
      if (curr_modifier & 0x7F)
        return;
      curr_modifier = 6;
    }
    else
    {
      ++curr_modifier;
      if (curr_modifier == 5)
      {
        curr_modifier = 0x8F;
      }
      else if (curr_modifier == 9)
      {
        curr_modifier = 0;
        trob.type = -1;
      }
    }
  }
}

// seg007:0522
void __pascal far animate_door()
{
  /*
Possible values of anim_type:
0: closing
1: open
2: permanent open
3,4,5,6,7,8: fast closing with speeds 20,40,60,80,100,120 /4 pixel/frame
*/
  sbyte anim_type;
  anim_type = trob.type;
  if (anim_type >= 0)
  {
    if (anim_type >= 3)
    {
      // closing fast
      if (anim_type < 8)
      {
        ++anim_type;
        trob.type = anim_type;
      }
      short new_mod = curr_modifier - gate_close_speeds[anim_type];
      curr_modifier = new_mod;
      //if ((sbyte)curr_modifier < 0) {
      if (new_mod < 0)
      {
        //if ((curr_modifier -= gate_close_speeds[anim_type]) < 0) {
        curr_modifier = 0;
        trob.type = -1;
      }
    }
    else
    {
      if (curr_modifier != 0xFF)
      {
        // 0xFF means permanently open.
        curr_modifier += door_delta[anim_type];
        if (anim_type == 0)
        {
          // closing
          if (curr_modifier != 0)
          {
            if (curr_modifier < 188)
            {
              if ((curr_modifier & 3) == 3)
              {
              }
            }
          }
          else
          {
            gate_stop();
          }
        }
        else
        {
          // opening
          if (curr_modifier < 188)
          {
            if ((curr_modifier & 7) == 0)
            {
            }
          }
          else
          {
            // stop
            if (anim_type < 2)
            {
              // after regular open
              curr_modifier = 238;
              trob.type = 0;                 // closing
            }
            else
            {
              // after permanent open
              curr_modifier = 0xFF; // keep open
              gate_stop();
            }
          }
        }
      }
      else
      {
        gate_stop();
      }
    }
  }
}

// seg007:05E3
void __pascal far gate_stop()
{
  trob.type = -1;
}

// seg007:05F1
void __pascal far animate_leveldoor()
{
  /*
Possible values of trob_type:
0: open
1: open (with button)
2: open
3,4,5,6: fast closing with speeds 0,5,17,99 pixel/frame
*/
  word trob_type;
  trob_type = trob.type;
  if (trob.type >= 0)
  {
    if (trob_type >= 3)
    {
      // closing
      ++trob.type;
      curr_modifier -= leveldoor_close_speeds[trob.type - 3];
      if ((sbyte)curr_modifier < 0)
      {
        curr_modifier = 0;
        trob.type = -1;
      }
      else
      {
        if (trob.type == 4)
        {
          sound_interruptible[sound_15_leveldoor_sliding] = 1;
        }
      }
    }
    else
    {
      // opening
      ++curr_modifier;
      if (curr_modifier >= 43)
      {
        trob.type = -1;
        if (leveldoor_open == 0 || leveldoor_open == 2)
        {
          leveldoor_open = 1;
          if (current_level == /*4*/ custom->mirror_level)
          {
            // Special event: place mirror
            get_tile(/*4*/ custom->mirror_room, /*4*/ custom->mirror_column, /*0*/ custom->mirror_row);
            curr_room_tiles[curr_tilepos] = /*tiles_13_mirror*/ custom->mirror_tile;
          }
        }
      }
      else
      {
        sound_interruptible[sound_15_leveldoor_sliding] = 0;
      }
    }
  }
}




// seg007:081E
void __pascal far start_anim_torch(short room, short tilepos)
{
  curr_room_modif[tilepos] = prandom(8);
  add_trob(room, tilepos, 1);
}

// seg007:0847
void __pascal far start_anim_potion(short room, short tilepos)
{
  curr_room_modif[tilepos] &= 0xF8;
  curr_room_modif[tilepos] |= prandom(6) + 1;
  add_trob(room, tilepos, 1);
}

// seg007:087C
void __pascal far start_anim_sword(short room, short tilepos)
{
  curr_room_modif[tilepos] = prandom(0xFF) & 0x1F;
  add_trob(room, tilepos, 1);
}

// seg007:08A7
void __pascal far start_anim_chomper(short room, short tilepos, byte modifier)
{
  short old_modifier;
  old_modifier = curr_room_modif[tilepos];
  if (old_modifier == 0 || old_modifier >= 6)
  {
    curr_room_modif[tilepos] = modifier;
    add_trob(room, tilepos, 1);
  }
}

// seg007:08E3
void __pascal far start_anim_spike(short room, short tilepos)
{
  sbyte old_modifier;
  old_modifier = curr_room_modif[tilepos];
  if (old_modifier <= 0)
  {
    if (old_modifier == 0)
    {
      add_trob(room, tilepos, 1);
    }
    else
    {
      // 0xFF means a disabled spike.
      if (old_modifier != (sbyte)0xFF)
      {
        curr_room_modif[tilepos] = 0x8F;
      }
    }
  }
}

// seg007:092C
short __pascal far trigger_gate(short room, short tilepos, short button_type)
{
  byte modifier;
  modifier = curr_room_modif[tilepos];
  if (button_type == tiles_15_opener)
  {
    // If the gate is permanently open, don't to anything.
    if (modifier == 0xFF)
      return -1;
    if (modifier >= 188)
    {                                 // if it's already open
      curr_room_modif[tilepos] = 238; // keep it open for a while
      return -1;
    }
    curr_room_modif[tilepos] = (modifier + 3) & 0xFC;
    return 1; // regular open
  }
  else if (button_type == tiles_14_debris)
  {
    // If it's not fully open:
    if (modifier < 188)
      return 2;                      // permanent open
    curr_room_modif[tilepos] = 0xFF; // keep open
    return -1;
  }
  else
  {
    if (modifier != 0)
    {
      return 3; // close fast
    }
    else
    {
      // already closed
      return -1;
    }
  }
}

// seg007:0999
short __pascal far trigger_1(short target_type, short room, short tilepos, short button_type)
{
  short result;
  result = -1;
  if (target_type == tiles_4_gate)
  {
    result = trigger_gate(room, tilepos, button_type);
  }
  else if (target_type == tiles_16_level_door_left)
  {
    if (curr_room_modif[tilepos] != 0)
    {
      result = -1;
    }
    else
    {
      result = 1;
    }
  }
  else if (custom->allow_triggering_any_tile)
  { //allow_triggering_any_tile hack
    result = 1;
  }
  return result;
}

// seg007:0BF2
short __pascal far get_doorlink_tile(short index)
{
  return doorlink1_ad[index] & 0x1F;
}

// seg007:0C09
short __pascal far get_doorlink_next(short index)
{
  return !(doorlink1_ad[index] & 0x80);
}

// seg007:0C26
short __pascal far get_doorlink_room(short index)
{
  return ((doorlink1_ad[index] & 0x60) >> 5) +
         ((doorlink2_ad[index] & 0xE0) >> 3);
}

// seg007:09E5
void __pascal far do_trigger_list(short index, short button_type)
{
  word room;
  word tilepos;
  byte target_type;
  sbyte trigger_result;
  // while (doorlink1_ad[index] != -1) { // these can't be equal!
  while (1)
  { // Same as the above but just a little faster and no compiler warning.
    room = get_doorlink_room(index);
    get_room_address(room);
    tilepos = get_doorlink_tile(index);
    target_type = curr_room_tiles[tilepos] & 0x1F;
    trigger_result = trigger_1(target_type, room, tilepos, button_type);
    if (trigger_result >= 0)
    {
      add_trob(room, tilepos, trigger_result);
    }
    if (get_doorlink_next(index++) == 0)
      break;
  }
}

// seg007:0ACA
short __pascal far find_trob()
{
  short index;
  for (index = 0; index < trobs_count; ++index)
  {
    if (trobs[index].tilepos == trob.tilepos &&
        trobs[index].room == trob.room)
      return index;
  }
  return -1;
}

// seg007:0A5A
void __pascal far add_trob(byte room, byte tilepos, sbyte type)
{
  short found;
  trob.room = room;
  trob.tilepos = tilepos;
  trob.type = type;
  found = find_trob();
  if (found == -1)
  {
    // add new
    if (trobs_count == 30)
      return;
    trobs[trobs_count++] = trob;
  }
  else
  {
    // change existing
    trobs[found].type = trob.type;
  }
}



// seg007:0BB6
short __pascal far get_doorlink_timer(short index)
{
  return doorlink2_ad[index] & 0x1F;
}

// seg007:0BCD
short __pascal far set_doorlink_timer(short index, byte value)
{
  doorlink2_ad[index] &= 0xE0;
  doorlink2_ad[index] |= value & 0x1F;
  return doorlink2_ad[index];
}



// seg007:0C53
void __pascal far trigger_button(int playsound, int button_type, int modifier)
{
  sbyte link_timer;
  get_curr_tile(curr_tilepos);
  if (button_type == 0)
  {
    // 0 means currently selected
    button_type = curr_tile;
  }
  if (modifier == -1)
  {
    // -1 means currently selected
    modifier = curr_modifier;
  }
  link_timer = get_doorlink_timer(modifier);
  // is the event jammed?
  if (link_timer != 0x1F)
  {
    set_doorlink_timer(modifier, 5);
    if (link_timer < 2)
    {
      add_trob(curr_room, curr_tilepos, 1);
      is_guard_notice = 1;
    }
    do_trigger_list(modifier, button_type);
  }
}

// seg007:0CD9
void __pascal far died_on_button()
{
  word button_type;
  word modifier;
  button_type = get_curr_tile(curr_tilepos);
  modifier = curr_modifier;
  if (curr_tile == tiles_15_opener)
  {
    curr_room_tiles[curr_tilepos] = tiles_1_floor;
    curr_room_modif[curr_tilepos] = 0;
    button_type = tiles_14_debris; // force permanent open
  }
  else
  {
    curr_room_tiles[curr_tilepos] = tiles_5_stuck;
  }
  trigger_button(1, button_type, modifier);
}

// seg007:0D3A
void __pascal far animate_button()
{
  word var_2;
  if (trob.type >= 0)
  {
    set_doorlink_timer(curr_modifier, var_2 = get_doorlink_timer(curr_modifier) - 1);
    if (var_2 < 2)
    {
      trob.type = -1;
    }
  }
}

// seg007:0D72
void __pascal far start_level_door(short room, short tilepos)
{
  curr_room_modif[tilepos] = 43; // start fully open
  add_trob(room, tilepos, 3);
}

// seg007:0D93
void __pascal far animate_empty()
{
  trob.type = -1;
}

// seg007:0D9D
void __pascal far animate_loose()
{
  word room;
  word row;
  word tilepos;
  short anim_type;
  anim_type = trob.type;
  if (anim_type >= 0)
  {
    ++curr_modifier;
    if (curr_modifier & 0x80)
    {
      // just shaking
      // don't stop on level 13, needed for the auto-falling floors
      if (current_level == 13)
        return;
      if (curr_modifier >= 0x84)
      {
        curr_modifier = 0;
        trob.type = -1;
      }
      loose_shake(!curr_modifier);
    }
    else
    {
      // something is on the floor
      // should it fall already?
      if (curr_modifier >= /*11*/ custom->loose_floor_delay)
      {
        curr_modifier = remove_loose(room = trob.room, tilepos = trob.tilepos);
        trob.type = -1;
        curmob.xh = (tilepos % 10) << 2;
        row = tilepos / 10;
        curmob.y = y_loose_land[row + 1];
        curmob.room = room;
        curmob.speed = 0;
        curmob.type = 0;
        curmob.row = row;
        add_mob();
      }
      else
      {
        loose_shake(0);
      }
    }
  }
}


// seg007:0E55
void __pascal far loose_shake(int arg_0)
{
  word sound_id;
  if (arg_0 || loose_sound[curr_modifier & 0x7F])
  {
    do
    {
      // Sounds 20,21,22: loose floor shaking
      sound_id = prandom(2) + sound_20_loose_shake_1;
    } while (sound_id == last_loose_sound);

    prandom(2); // For vanilla pop compatibility, an RNG cycle is wasted here
                  // Note: In DOS PoP, it's wasted a few lines below.
      last_loose_sound = sound_id;
  }
}

// seg007:0EB8
int __pascal far remove_loose(int room, int tilepos)
{
  curr_room_tiles[tilepos] = tiles_0_empty;
  // note: the level type is used to determine the modifier of the empty space left behind
  return custom->tbl_level_type[current_level];
}

// seg007:0ED5
void __pascal far make_loose_fall(byte modifier)
{
  // is it a "solid" loose floor?
  if ((curr_room_tiles[curr_tilepos] & 0x20) == 0)
  {
    if ((sbyte)curr_room_modif[curr_tilepos] <= 0)
    {
      curr_room_modif[curr_tilepos] = modifier;
      add_trob(curr_room, curr_tilepos, 0);
    }
  }
}


// seg007:0F9A
int __pascal far next_chomper_timing(byte timing)
{
  // 15,12,9,6,13,10,7,14,11,8,repeat
  timing -= 3;
  if (timing < 6)
  {
    timing += 10;
  }
  return timing;
}


// seg007:0F13
void __pascal far start_chompers()
{
  short timing;
  short modifier;
  short tilepos;
  short column;
  timing = 15;
  if ((byte)Char.curr_row < 3)
  {
    get_room_address(Char.room);
    for (column = 0, tilepos = tbl_line[Char.curr_row];
         column < 10;
         ++column, ++tilepos)
    {
      if (get_curr_tile(tilepos) == tiles_18_chomper)
      {
        modifier = curr_modifier & 0x7F;
        if (modifier == 0 || modifier >= 6)
        {
          start_anim_chomper(Char.room, tilepos, timing | (curr_modifier & 0x80));
          timing = next_chomper_timing(timing);
        }
      }
    }
  }
}

// seg007:0FB4
void __pascal far loose_make_shake()
{
  // don't shake on level 13
  if (curr_room_modif[curr_tilepos] == 0 && current_level != 13)
  {
    curr_room_modif[curr_tilepos] = 0x80;
    add_trob(curr_room, curr_tilepos, 1);
  }
}

// seg007:0FE0
void __pascal far do_knock(int room, int tile_row)
{
  short tile_col;
  for (tile_col = 0; tile_col < 10; ++tile_col)
  {
    if (get_tile(room, tile_col, tile_row) == tiles_11_loose)
    {
      loose_make_shake();
    }
  }
}

// seg007:1010
void __pascal far add_mob()
{
  mobs[mobs_count++] = curmob;
}




// seg007:1063
void __pascal far do_mobs()
{
  short n_mobs;
  short index;
  short new_index;
  n_mobs = mobs_count;
  for (curmob_index = 0; n_mobs > curmob_index; ++curmob_index)
  {
    curmob = mobs[curmob_index];
    move_mob();
    check_loose_fall_on_kid();
    mobs[curmob_index] = curmob;
  }
  new_index = 0;
  for (index = 0; index < mobs_count; ++index)
  {
    if (mobs[index].speed != -1)
    {
      mobs[new_index++] = mobs[index];
    }
  }
  mobs_count = new_index;
}

// seg007:110F
void __pascal far move_mob()
{
  if (curmob.type == 0)
  {
    move_loose();
  }
  if (curmob.speed <= 0)
  {
    ++curmob.speed;
  }
}

// seg007:1126
void __pascal far move_loose()
{
  if (curmob.speed < 0)
    return;
  if (curmob.speed < 29)
    curmob.speed += 3;
  curmob.y += curmob.speed;
  if (curmob.room == 0)
  {
    if (curmob.y < 210)
    {
      return;
    }
    else
    {
      curmob.speed = -2;
      return;
    }
  }
  if (curmob.y < 226 && y_something[curmob.row + 1] <= curmob.y)
  {
    // fell into a different row
    curr_tile_temp = get_tile(curmob.room, curmob.xh >> 2, curmob.row);
    if (curr_tile_temp == tiles_11_loose)
    {
      loose_fall();
    }
    if (curr_tile_temp == tiles_0_empty ||
        curr_tile_temp == tiles_11_loose)
    {
      mob_down_a_row();
      return;
    }
    do_knock(curmob.room, curmob.row);
    curmob.y = y_something[curmob.row + 1];
    curmob.speed = -2;
    loose_land();
  }
}

// seg007:11E8
void __pascal far loose_land()
{
  short button_type;
  short tiletype;
  button_type = 0;
  tiletype = get_tile(curmob.room, curmob.xh >> 2, curmob.row);
  switch (tiletype)
  {
  case tiles_15_opener:
    curr_room_tiles[curr_tilepos] = tiles_14_debris;
    button_type = tiles_14_debris;
  // fallthrough!
  case tiles_6_closer:
    trigger_button(1, button_type, -1);
    tiletype = get_tile(curmob.room, curmob.xh >> 2, curmob.row);
  // fallthrough!
  case tiles_1_floor:
  case tiles_2_spike:
  case tiles_10_potion:
  case tiles_19_torch:
  case tiles_30_torch_with_debris:
    if (tiletype == tiles_19_torch ||
        tiletype == tiles_30_torch_with_debris)
    {
      curr_room_tiles[curr_tilepos] = tiles_30_torch_with_debris;
    }
    else
    {
      curr_room_tiles[curr_tilepos] = tiles_14_debris;
    }
  }
}

// seg007:12CB
void __pascal far loose_fall()
{
  curr_room_modif[curr_tilepos] = remove_loose(curr_room, curr_tilepos);
  curmob.speed >>= 1;
  mobs[curmob_index] = curmob;
  curmob.y += 6;
  mob_down_a_row();
  add_mob();
  curmob = mobs[curmob_index];
}

// seg007:1387
void __pascal far mob_down_a_row()
{
  ++curmob.row;
  if (curmob.row >= 3)
  {
    curmob.y -= 192;
    curmob.row = 0;
    curmob.room = level.roomlinks[curmob.room - 1].down;
  }
}


// seg007:14DE
void __pascal far add_mob_to_objtable(int ypos)
{
  word index;
  objtable_type *curr_obj;
  index = objtable_count++;
  curr_obj = &objtable[index];
  curr_obj->obj_type = curmob.type | 0x80;
  curr_obj->xh = curmob.xh;
  curr_obj->xl = 0;
  curr_obj->y = ypos;
  curr_obj->chtab_id = id_chtab_6_environment;
  curr_obj->id = 10;
  curr_obj->clip.top = 0;
  curr_obj->clip.left = 0;
  curr_obj->clip.right = 40;
}

// seg007:1556
int __pascal far is_spike_harmful()
{
  sbyte modifier;
  modifier = curr_room_modif[curr_tilepos];
  if (modifier == 0 || modifier == -1)
  {
    return 0;
  }
  else if (modifier < 0)
  {
    return 1;
  }
  else if (modifier < 5)
  {
    return 2;
  }
  else
  {
    return 0;
  }
}

// seg007:1591
void __pascal far check_loose_fall_on_kid()
{
  loadkid();
  if (Char.room == curmob.room &&
      Char.curr_col == curmob.xh >> 2 &&
      curmob.y < Char.y &&
      Char.y - 30 < curmob.y)
  {
    fell_on_your_head();
    savekid();
  }
}

// seg007:15D3
void __pascal far fell_on_your_head()
{
  short frame;
  short action;
  frame = Char.frame;
  action = Char.action;
  // loose floors hurt you in frames 5..14 (running) only on level 13
  if (
    (current_level == 13 || (frame < frame_5_start_run || frame >= 15)) &&
    (action < actions_2_hang_climb || action == actions_7_turn))
  {
    Char.y = y_land[Char.curr_row + 1];
    if (take_hp(1))
    {
      seqtbl_offset_char(seq_22_crushed); // dead (because of loose floor)
      if (frame == frame_177_spiked)
      { // spiked
        Char.x = char_dx_forward(-12);
      }
    }
    else
    {
      if (frame != frame_109_crouch)
      { // crouching
        if (get_tile_behind_char() == 0)
        {
          Char.x = char_dx_forward(-2);
        }
        seqtbl_offset_char(seq_52_loose_floor_fell_on_kid); // loose floor fell on Kid
      }
    }
  }
}


// seg005:000A
void __pascal far seqtbl_offset_char(short seq_index)
{
  Char.curr_seq = seqtbl_offsets[seq_index];
}

// seg005:001D
void __pascal far seqtbl_offset_opp(int seq_index)
{
  Opp.curr_seq = seqtbl_offsets[seq_index];
}

// seg005:0030
void __pascal far do_fall()
{
  if (is_screaming == 0 && Char.fall_y >= 31)
  {
    is_screaming = 1;
  }
  if ((word)y_land[Char.curr_row + 1] > (word)Char.y)
  {
    check_grab();
  }
  else
  {
    if (get_tile_at_char() == tiles_20_wall)
    {
      in_wall();
    }

    if (tile_is_floor(curr_tile2))
    {
      land();
    }
    else
    {
      inc_curr_row();
    }
  }
}

// seg005:0090
void __pascal far land()
{
  word seq_id;
  is_screaming = 0;
  Char.y = y_land[Char.curr_row + 1];
  if (get_tile_at_char() != tiles_2_spike)
  {
    if (!tile_is_floor(get_tile_infrontof_char()) &&
        distance_to_edge_weight() < 3)  Char.x = char_dx_forward(-3);
    start_chompers();
  }
  else
  {
    // fell on spikes
    goto loc_5EE6;
  }
  if (Char.alive < 0)
  {
    // alive
    if ((distance_to_edge_weight() >= 12 &&
         get_tile_behind_char() == tiles_2_spike) ||
        get_tile_at_char() == tiles_2_spike)
    {
    // fell on spikes
    loc_5EE6:
      if (is_spike_harmful())
      {
        spiked();
        return;
      }
    }
    {
      if (Char.fall_y < 22)
      {
      // fell 1 row
      loc_5EFD:
        if (Char.charid >= charid_2_guard || Char.sword == sword_2_drawn)
        {
          Char.sword = sword_2_drawn;
          seq_id = seq_63_guard_stand_active; // stand active after landing
        }
        else
        {
          seq_id = seq_17_soft_land; // crouch (soft land)
        }
        if (Char.charid == charid_0_kid)
        {
          is_guard_notice = 1;
        }
      }
      else if (Char.fall_y < 33)
      {
        // fell 2 rows
        if (Char.charid == charid_1_shadow)
          goto loc_5EFD;
        if (Char.charid == charid_2_guard)
          goto loc_5F6C;
        // kid (or skeleton (bug!))
        if (!take_hp(1))
        {
          // still alive
          is_guard_notice = 1;
          seq_id = seq_20_medium_land; // medium land (lose 1 HP, crouch)
        }
        else
        {
          // dead (this was the last HP)
          goto loc_5F75;
        }
      }
      else
      {
        // fell 3 or more rows
        goto loc_5F6C;
      }
    }
  }
  else
  {
  // dead
  loc_5F6C:
    take_hp(100);
  loc_5F75:
    seq_id = seq_22_crushed;           // dead (after falling)
  }
  seqtbl_offset_char(seq_id);
  play_seq();
  Char.fall_y = 0;
}

// seg005:01B7
void __pascal far spiked()
{
  // If someone falls into spikes, those spikes become harmless (to others).
  curr_room_modif[curr_tilepos] = 0xFF;
  Char.y = y_land[Char.curr_row + 1];
    Char.x = x_bump[tile_col + 5] + 10;
  Char.x = char_dx_forward(8);
  Char.fall_y = 0;
  take_hp(100);
  seqtbl_offset_char(seq_51_spiked); // spiked
  play_seq();
}

// seg005:0213
void __pascal far control()
{
  short char_frame;
  short char_action;
  char_frame = Char.frame;
  if (Char.alive >= 0)
  {
    if (char_frame == frame_15_stand ||             // stand
        char_frame == frame_166_stand_inactive ||   // stand
        char_frame == frame_158_stand_with_sword || // stand with sword
        char_frame == frame_171_stand_with_sword    // stand with sword
    )
    {
      seqtbl_offset_char(seq_71_dying); // dying (not stabbed)
    }
  }
  else
  {
    char_action = Char.action;
    if (char_action == actions_5_bumped ||
        char_action == actions_4_in_freefall)
    {
      release_arrows();
    }
    else if (Char.sword == sword_2_drawn)
    {
      control_with_sword();
    }
    else if (Char.charid >= charid_2_guard)
    {
      control_guard_inactive();
    }
    else if (char_frame == frame_15_stand ||                  // standing
             (char_frame >= frame_50_turn && char_frame < 53) // end of turning
    )
    {
      control_standing();
    }
    else if (char_frame == frame_48_turn)
    { // a frame in turning
      control_turning();
    }
    else if (char_frame < 4)
    { // start run
      control_startrun();
    }
    else if (char_frame >= frame_67_start_jump_up_1 && char_frame < frame_70_jumphang)
    { // start jump up
      control_jumpup();
    }
    else if (char_frame < 15)
    { // running
      control_running();
    }
    else if (char_frame >= frame_87_hanging_1 && char_frame < 100)
    { // hanging
      control_hanging();
    }
    else if (char_frame == frame_109_crouch)
    { // crouching
      control_crouched();
    }
  }
}

// seg005:02EB
void __pascal far control_crouched()
{
  if (need_level1_music != 0 && current_level == /*1*/ custom->intro_music_level)
  {
     if (need_level1_music == 1)
     {
       need_level1_music = 2;
     }
     else
     {
         need_level1_music = 0;
     }
  }
  else
  {
    need_level1_music = 0;
    if (control_shift2 < 0 && check_get_item())
      return;
    if (control_y != 1)
    {
      seqtbl_offset_char(seq_49_stand_up_from_crouch); // stand up from crouch
    }
    else
    {
      if (control_forward < 0)
      {
        control_forward = 1;                   // disable automatic repeat
        seqtbl_offset_char(seq_79_crouch_hop); // crouch-hop
      }
    }
  }
}

// seg005:0358
void __pascal far control_standing()
{
  short var_2;
  if (control_shift2 < 0 && control_shift < 0 && check_get_item())
  {
    return;
  }
  if (Char.charid != charid_0_kid && control_down < 0 && control_forward < 0)
  {
    draw_sword();
    return;
  } //else
  if (have_sword)
  {
    if (offguard != 0 && control_shift >= 0)
      goto loc_6213;
    if (can_guard_see_kid >= 2)
    {
      var_2 = char_opp_dist();
      if (var_2 >= -10 && var_2 < 90)
      {
        holding_sword = 1;
        if ((word)var_2 < (word)-6)
        {
          if (Opp.charid == charid_1_shadow &&
              (Opp.action == actions_3_in_midair || (Opp.frame >= frame_107_fall_land_1 && Opp.frame < 118)))
          {
            offguard = 0;
          }
          else
          {
            draw_sword();
            return;
          }
        }
        else
        {
          back_pressed();
          return;
        }
      }
    }
    else
    {
      offguard = 0;
    }
  }
  if (control_shift < 0)
  {
    if (control_backward < 0)
    {
      back_pressed();
    }
    else if (control_up < 0)
    {
      up_pressed();
    }
    else if (control_down < 0)
    {
      down_pressed();
    }
    else if (control_x < 0 && control_forward < 0)
    {
      safe_step();
    }
  }
  else
  loc_6213:
    if (control_forward < 0)
    {
      if (is_keyboard_mode && control_up < 0)
      {
        standing_jump();
      }
      else
      {
        forward_pressed();
      }
    }
    else if (control_backward < 0)
    {
      back_pressed();
    }
    else if (control_up < 0)
    {
      if (is_keyboard_mode && control_forward < 0)
      {
        standing_jump();
      }
      else
      {
        up_pressed();
      }
    }
    else if (control_down < 0)
    {
      down_pressed();
    }
    else if (control_x < 0)
    {
      forward_pressed();
    }
}

// seg005:0482
void __pascal far up_pressed()
{
  int leveldoor_tilepos = -1;
  if (get_tile_at_char() == tiles_16_level_door_left)
    leveldoor_tilepos = curr_tilepos;
  else if (get_tile_behind_char() == tiles_16_level_door_left)
    leveldoor_tilepos = curr_tilepos;
  else if (get_tile_infrontof_char() == tiles_16_level_door_left)
    leveldoor_tilepos = curr_tilepos;
  if ((leveldoor_tilepos != -1) && level.start_room != drawn_room && leveldoor_open)
  {
    go_up_leveldoor();
  }
  else
  {
    if (control_x < 0)
    {
      standing_jump();
    }
    else
    {
      check_jump_up();
    }
  }
}

// seg005:04C7
void __pascal far down_pressed()
{
  control_down = 1; // disable automatic repeat
  if (!tile_is_floor(get_tile_infrontof_char()) &&
      distance_to_edge_weight() < 3)
  {
    Char.x = char_dx_forward(5);
    load_fram_det_col();
  }
  else
  {
    if (!tile_is_floor(get_tile_behind_char()) &&
        distance_to_edge_weight() >= 8)
    {
      through_tile = get_tile_behind_char();
      get_tile_at_char();
      if (can_grab() &&
          (Char.direction >= dir_0_right ||
           get_tile_at_char() != tiles_4_gate ||
           curr_room_modif[curr_tilepos] >> 2 >= 6))
      {
        Char.x = char_dx_forward(distance_to_edge_weight() - 9);
        seqtbl_offset_char(seq_68_climb_down); // climb down
      }
      else
      {
        crouch();
      }
    }
    else
    {
      crouch();
    }
  }
}

// seg005:0574
void __pascal far go_up_leveldoor()
{
  Char.x = x_bump[tile_col + 5] + 10;
  Char.direction = dir_FF_left;                   // right
  seqtbl_offset_char(seq_70_go_up_on_level_door); // go up on level door
}

// seg005:058F
void __pascal far control_turning()
{
  if (control_shift >= 0 && control_x < 0 && control_y >= 0)
  {
    seqtbl_offset_char(seq_43_start_run_after_turn); // start run and run (after turning)
  }
}

// seg005:05AD
void __pascal far crouch()
{
  seqtbl_offset_char(seq_50_crouch); // crouch
  control_down = release_arrows();
}

// seg005:05BE
void __pascal far back_pressed()
{
  word seq_id;
  control_backward = release_arrows();
  // After turn, Kid will draw sword if ...
  if (have_sword == 0 ||       // if Kid has sword
      can_guard_see_kid < 2 || // and can see Guard
      char_opp_dist() > 0 ||   // and Guard was behind him
      distance_to_edge_weight() < 2)
  {
    seq_id = seq_5_turn; // turn
  }
  else
  {
    Char.sword = sword_2_drawn;
    offguard = 0;
    seq_id = seq_89_turn_draw_sword; // turn and draw sword
  }
  seqtbl_offset_char(seq_id);
}

// seg005:060F
void __pascal far forward_pressed()
{
  short distance;
  distance = get_edge_distance();

  if (edge_type == 1 && curr_tile2 != tiles_18_chomper && distance < 8)
  {
    // If char is near a wall, step instead of run.
    if (control_forward < 0)
    {
      safe_step();
    }
  }
  else
  {
    seqtbl_offset_char(seq_1_start_run); // start run and run
  }
}

// seg005:0649
void __pascal far control_running()
{
  if (control_x == 0 && (Char.frame == frame_7_run || Char.frame == frame_11_run))
  {
    control_forward = release_arrows();
    seqtbl_offset_char(seq_13_stop_run); // stop run
  }
  else if (control_x > 0)
  {
    control_backward = release_arrows();
    seqtbl_offset_char(seq_6_run_turn); // run-turn
  }
  else if (control_y < 0 && control_up < 0)
  {
    run_jump();
  }
  else if (control_down < 0)
  {
    control_down = 1;                                // disable automatic repeat
    seqtbl_offset_char(seq_26_crouch_while_running); // crouch while running
  }
}

// seg005:06A8
void __pascal far safe_step()
{
  short distance;
  control_shift2 = 1;  // disable automatic repeat
  control_forward = 1; // disable automatic repeat
  distance = get_edge_distance();
  if (distance)
  {
    Char.repeat = 1;
    seqtbl_offset_char(distance + 28); // 29..42: safe step to edge
  }
  else if (edge_type != 1 && Char.repeat != 0)
  {
    Char.repeat = 0;
    seqtbl_offset_char(seq_44_step_on_edge); // step on edge
  }
  else
  {
    seqtbl_offset_char(seq_39_safe_step_11); // unsafe step (off ledge)
  }
}

// seg005:06F0
int __pascal far check_get_item()
{
  if (get_tile_at_char() == tiles_10_potion ||
      curr_tile2 == tiles_22_sword)
  {
    if (!tile_is_floor(get_tile_behind_char()))
    {
      return 0;
    }
    Char.x = char_dx_forward(-14);
    load_fram_det_col();
  }
  if (get_tile_infrontof_char() == tiles_10_potion ||
      curr_tile2 == tiles_22_sword)
  {
    get_item();
    return 1;
  }
  return 0;
}

// seg005:073E
void __pascal far get_item()
{
  short distance;
  if (Char.frame != frame_109_crouch)
  { // crouching
    distance = get_edge_distance();
    if (edge_type != 2)
    {
      Char.x = char_dx_forward(distance);
    }
    if (Char.direction >= dir_0_right)
    {
      Char.x = char_dx_forward((curr_tile2 == tiles_10_potion) - 2);
    }
    crouch();
  }
  else if (curr_tile2 == tiles_22_sword)
  {
    do_pickup(-1);
    seqtbl_offset_char(seq_91_get_sword); // get sword
  }
  else
  { // potion
    do_pickup(curr_room_modif[curr_tilepos] >> 3);
    seqtbl_offset_char(seq_78_drink); // drink
    if (current_level == 15)
    {
      short index;
      for (index = 0; index < 14; ++index)
      {
        // remove letter on potions level
        if (copyprot_room[index] == curr_room &&
            copyprot_tile[index] == curr_tilepos)
        {
          copyprot_room[index] = 0;
          break;
        }
      }
    }
  }
}

// seg005:07FF
void __pascal far control_startrun()
{
  if (control_y < 0 && control_x < 0)
  {
    standing_jump();
  }
}

// seg005:0812
void __pascal far control_jumpup()
{
  if (control_x < 0 || control_forward < 0)
  {
    standing_jump();
  }
}

// seg005:0825
void __pascal far standing_jump()
{
  control_up = control_forward = 1;        // disable automatic repeat
  seqtbl_offset_char(seq_3_standing_jump); // standing jump
}

// seg005:0836
void __pascal far check_jump_up()
{
  control_up = release_arrows();
  through_tile = get_tile_above_char();
  get_tile_front_above_char();
  if (can_grab())
  {
    grab_up_with_floor_behind();
  }
  else
  {
    through_tile = get_tile_behind_above_char();
    get_tile_above_char();
    if (can_grab())
    {
      jump_up_or_grab();
    }
    else
    {
      jump_up();
    }
  }
}

// seg005:087B
void __pascal far jump_up_or_grab()
{
  short distance;
  distance = distance_to_edge_weight();
  if (distance < 6)
  {
    jump_up();
  }
  else if (!tile_is_floor(get_tile_behind_char()))
  {
    // There is not floor behind char.
    grab_up_no_floor_behind();
  }
  else
  {
    // There is floor behind char, go back a bit.
    Char.x = char_dx_forward(distance - 14);
    load_fram_det_col();
    grab_up_with_floor_behind();
  }
}

// seg005:08C7
void __pascal far grab_up_no_floor_behind()
{
  get_tile_above_char();
  Char.x = char_dx_forward(distance_to_edge_weight() - 10);
  seqtbl_offset_char(seq_16_jump_up_and_grab); // jump up and grab (no floor behind)
}

// seg005:08E6
void __pascal far jump_up()
{
  short distance;
  control_up = release_arrows();
  distance = get_edge_distance();
  if (distance < 4 && edge_type == 1)
  {
    Char.x = char_dx_forward(distance - 3);
  }
  get_tile(Char.room, get_tile_div_mod(back_delta_x(0) + dx_weight() - 6), Char.curr_row - 1);
  if (curr_tile2 != tiles_20_wall && !tile_is_floor(curr_tile2))
  {
    seqtbl_offset_char(seq_28_jump_up_with_nothing_above); // jump up with nothing above
  }
  else
  {
    seqtbl_offset_char(seq_14_jump_up_into_ceiling); // jump up with wall or floor above
  }
}

// seg005:0968
void __pascal far control_hanging()
{
  if (Char.alive < 0)
  {
    if (grab_timer == 0 && control_y < 0)
    {
      can_climb_up();
    }
    else if (control_shift < 0)
    {
      // hanging against a wall or a doortop
      if (Char.action != actions_6_hang_straight &&
          (get_tile_at_char() == tiles_20_wall ||
           (Char.direction == dir_FF_left && ( // facing left
                                               curr_tile2 == tiles_7_doortop_with_floor ||
                                               curr_tile2 == tiles_12_doortop))))
      {
        if (grab_timer == 0)
        {
        }
        seqtbl_offset_char(seq_25_hang_against_wall); // hang against wall (straight)
      }
      else
      {
        if (!tile_is_floor(get_tile_above_char()))
        {
          hang_fall();
        }
      }
    }
    else
    {
      hang_fall();
    }
  }
  else
  {
    hang_fall();
  }
}

// seg005:09DF
void __pascal far can_climb_up()
{
  short seq_id;
  seq_id = seq_10_climb_up; // climb up
  control_up = control_shift2 = release_arrows();
  get_tile_above_char();
  if (((curr_tile2 == tiles_13_mirror || curr_tile2 == tiles_18_chomper) &&
       Char.direction == dir_0_right) ||
      (curr_tile2 == tiles_4_gate && Char.direction != dir_0_right &&
       curr_room_modif[curr_tilepos] >> 2 < 6))
  {
    seq_id = seq_73_climb_up_to_closed_gate; // climb up to closed gate and down
  }
  seqtbl_offset_char(seq_id);
}

// seg005:0A46
void __pascal far hang_fall()
{
  control_down = release_arrows();
  if (!tile_is_floor(get_tile_behind_char()) &&
      !tile_is_floor(get_tile_at_char()))
  {
    seqtbl_offset_char(seq_23_release_ledge_and_fall); // release ledge and fall
  }
  else
  {
    if (get_tile_at_char() == tiles_20_wall ||
        (Char.direction < dir_0_right && ( // looking left
                                           curr_tile2 == tiles_7_doortop_with_floor ||
                                           curr_tile2 == tiles_12_doortop)))
    {
      Char.x = char_dx_forward(-7);
    }
    seqtbl_offset_char(seq_11_release_ledge_and_land); // end of climb down
  }
}

// seg005:0AA8
void __pascal far grab_up_with_floor_behind()
{
  short distance;
  distance = distance_to_edge_weight();

  // The global variable edge_type (which we need!) gets set as a side effect of get_edge_distance()
  short edge_distance = get_edge_distance();
  //printf("Distance to edge weight: %d\tedge type: %d\tedge distance: %d\n", distance, edge_type, edge_distance);

  #define JUMP_STRAIGHT_CONDITION distance < 4 && edge_distance < 4 && edge_type != 1

  if (JUMP_STRAIGHT_CONDITION)
  {
    Char.x = char_dx_forward(distance);
    seqtbl_offset_char(seq_8_jump_up_and_grab_straight); // jump up and grab (when?)
  }
  else
  {
    Char.x = char_dx_forward(distance - 4);
    seqtbl_offset_char(seq_24_jump_up_and_grab_forward); // jump up and grab (with floor behind)
  }
}

// seg005:0AF7
void __pascal far run_jump()
{
  short var_2;
  short xpos;
  short col;
  short var_8;
  if (Char.frame >= frame_7_run)
  {
    // Align Kid to edge of floor.
    xpos = char_dx_forward(4);
    col = get_tile_div_mod_m7(xpos);
    for (var_2 = 0; var_2 < 2; ++var_2)
    {
      col += dir_front[Char.direction + 1];
      get_tile(Char.room, col, Char.curr_row);
      if (curr_tile2 == tiles_2_spike || !tile_is_floor(curr_tile2))
      {
        var_8 = distance_to_edge(xpos) + 14 * var_2 - 14;
        if ((word)var_8 < (word)-8 || var_8 >= 2)
        {
          if (var_8 < 128)
            return;
          var_8 = -3;
        }
        Char.x = char_dx_forward(var_8 + 4);
        break;
      }
    }
    control_up = release_arrows();      // disable automatic repeat
    seqtbl_offset_char(seq_4_run_jump); // run-jump
  }
}

// sseg005:0BB5
void __pascal far back_with_sword()
{
  short frame;
  frame = Char.frame;
  if (frame == frame_158_stand_with_sword || frame == frame_170_stand_with_sword || frame == frame_171_stand_with_sword)
  {
    control_backward = 1;                       // disable automatic repeat
    seqtbl_offset_char(seq_57_back_with_sword); // back with sword
  }
}

// seg005:0BE3
void __pascal far forward_with_sword()
{
  short frame;
  frame = Char.frame;
  if (frame == frame_158_stand_with_sword || frame == frame_170_stand_with_sword || frame == frame_171_stand_with_sword)
  {
    control_forward = 1; // disable automatic repeat
    if (Char.charid != charid_0_kid)
    {
      seqtbl_offset_char(seq_56_guard_forward_with_sword); // forward with sword (Guard)
    }
    else
    {
      seqtbl_offset_char(seq_86_forward_with_sword); // forward with sword (Kid)
    }
  }
}

// seg005:0C1D
void __pascal far draw_sword()
{
  word seq_id;
  seq_id = seq_55_draw_sword; // draw sword
  control_forward = control_shift2 = release_arrows();
  if (Char.charid == charid_0_kid)
  {
    offguard = 0;
  }
  else if (Char.charid != charid_1_shadow)
  {
    seq_id = seq_90_en_garde; // stand active
  }
  Char.sword = sword_2_drawn;
  seqtbl_offset_char(seq_id);
}

// seg005:0C67
void __pascal far control_with_sword()
{
  short distance;
  if (Char.action < actions_2_hang_climb)
  {
    if (get_tile_at_char() == tiles_11_loose || can_guard_see_kid >= 2)
    {
      distance = char_opp_dist();
      if ((word)distance < (word)90)
      {
        swordfight();
        return;
      }
      else if (distance < 0)
      {
        if ((word)distance < (word)-4)
        {
          seqtbl_offset_char(seq_60_turn_with_sword); // turn with sword (after switching places)
          return;
        }
        else
        {
          swordfight();
          return;
        }
      }
    } /*else*/
    {
      if (Char.charid == charid_0_kid && Char.alive < 0)
      {
        holding_sword = 0;
      }
      if (Char.charid < charid_2_guard)
      {
        // frame 171: stand with sword
        if (Char.frame == frame_171_stand_with_sword)
        {
          Char.sword = sword_0_sheathed;
          seqtbl_offset_char(seq_92_put_sword_away); // put sword away (Guard died)
        }
      }
      else
      {
        swordfight();
      }
    }
  }
}

// seg005:0CDB
void __pascal far swordfight()
{
  short frame;
  short seq_id;
  short charid;
  frame = Char.frame;
  charid = Char.charid;
  // frame 161: parry
  if (frame == frame_161_parry && control_shift2 >= 0)
  {
    seqtbl_offset_char(seq_57_back_with_sword); // back with sword (when parrying)
    return;
  }
  else if (control_shift2 < 0)
  {
    if (charid == charid_0_kid)
    {
      kid_sword_strike = 15;
    }
    sword_strike();
    if (control_shift2 == 1)
      return;
  }
  if (control_down < 0)
  {
    if (frame == frame_158_stand_with_sword || frame == frame_170_stand_with_sword || frame == frame_171_stand_with_sword)
    {
      control_down = 1; // disable automatic repeat
      Char.sword = sword_0_sheathed;
      if (charid == charid_0_kid)
      {
        offguard = 1;
        guard_refrac = 9;
        holding_sword = 0;
        seq_id = seq_93_put_sword_away_fast; // put sword away fast (down pressed)
      }
      else if (charid == charid_1_shadow)
      {
        seq_id = seq_92_put_sword_away; // put sword away
      }
      else
      {
        seq_id = seq_87_guard_become_inactive; // stand inactive (when Kid leaves sight)
      }
      seqtbl_offset_char(seq_id);
    }
  }
  else if (control_up < 0)
  {
    parry();
  }
  else if (control_forward < 0)
  {
    forward_with_sword();
  }
  else if (control_backward < 0)
  {
    back_with_sword();
  }
}

// seg005:0DB0
void __pascal far sword_strike()
{
  short frame;
  short seq_id;
  frame = Char.frame;
  if (frame == frame_157_walk_with_sword ||  // walk with sword
      frame == frame_158_stand_with_sword || // stand with sword
      frame == frame_170_stand_with_sword || // stand with sword
      frame == frame_171_stand_with_sword || // stand with sword
      frame == frame_165_walk_with_sword     // walk with sword
  )
  {
    if (Char.charid == charid_0_kid)
    {
      seq_id = seq_75_strike; // strike with sword (Kid)
    }
    else
    {
      seq_id = seq_58_guard_strike; // strike with sword (Guard)
    }
  }
  else if (frame == frame_150_parry || frame == frame_161_parry)
  {                                     // parry
    seq_id = seq_66_strike_after_parry; // strike with sword after parrying
  }
  else
  {
    return;
  }
  control_shift2 = 1; // disable automatic repeat
  seqtbl_offset_char(seq_id);
}

// seg005:0E0F
void __pascal far parry()
{
  short opp_frame;
  short char_frame;
  short var_6;
  short seq_id;
  short char_charid;
  char_frame = Char.frame;
  opp_frame = Opp.frame;
  char_charid = Char.charid;
  seq_id = seq_62_parry; // defend (parry) with sword
  var_6 = 0;
  if (
    char_frame == frame_158_stand_with_sword || // stand with sword
    char_frame == frame_170_stand_with_sword || // stand with sword
    char_frame == frame_171_stand_with_sword || // stand with sword
    char_frame == frame_168_back ||             // back?
    char_frame == frame_165_walk_with_sword     // walk with sword
  )
  {
    if (char_opp_dist() >= 32 && char_charid != charid_0_kid)
    {
      back_with_sword();
      return;
    }
    else if (char_charid == charid_0_kid)
    {
      if (opp_frame == frame_168_back)
        return;
      if (opp_frame != frame_151_strike_1 &&
          opp_frame != frame_152_strike_2 &&
          opp_frame != frame_162_block_to_strike)
      {
        if (opp_frame == frame_153_strike_3)
        { // strike
          var_6 = 1;
        }
        else if (char_charid != charid_0_kid)
        {
          back_with_sword();
          return;
        }
      }
    }
    else
    {
      if (opp_frame != frame_152_strike_2)
        return;
    }
  }
  else
  {
    if (char_frame != frame_167_blocked)
      return;
    seq_id = seq_61_parry_after_strike; // parry after striking with sword
  }
  control_up = 1; // disable automatic repeat
  seqtbl_offset_char(seq_id);
  if (var_6)
  {
    play_seq();
  }
}

// seg008:0006
void __pascal far redraw_room()
{
  memset_near(table_counts, 0, sizeof(table_counts));
  reset_obj_clip();
  draw_room();
}

// seg008:0035
void __pascal far load_room_links()
{
  room_BR = 0;
  room_BL = 0;
  room_AR = 0;
  room_AL = 0;
  if (drawn_room)
  {
    get_room_address(drawn_room);
    room_L = level.roomlinks[drawn_room - 1].left;
    room_R = level.roomlinks[drawn_room - 1].right;
    room_A = level.roomlinks[drawn_room - 1].up;
    room_B = level.roomlinks[drawn_room - 1].down;
    if (room_A)
    {
      room_AL = level.roomlinks[room_A - 1].left;
      room_AR = level.roomlinks[room_A - 1].right;
    }
    else
    {
      if (room_L)
      {
        room_AL = level.roomlinks[room_L - 1].up;
      }
      if (room_R)
      {
        room_AR = level.roomlinks[room_R - 1].up;
      }
    }
    if (room_B)
    {
      room_BL = level.roomlinks[room_B - 1].left;
      room_BR = level.roomlinks[room_B - 1].right;
    }
    else
    {
      if (room_L)
      {
        room_BL = level.roomlinks[room_L - 1].down;
      }
      if (room_R)
      {
        room_BR = level.roomlinks[room_R - 1].down;
      }
    }
  }
  else
  {
    room_B = 0;
    room_A = 0;
    room_R = 0;
    room_L = 0;
  }
}

// seg008:0125
void __pascal far draw_room()
{
  word saved_room;
  saved_room = drawn_room;
  drawn_room = room_A;
  load_room_links();
  drawn_row = 2;
  for (drawn_col = 0; drawn_col < 10; ++drawn_col)
  {
    draw_main_y = -1;
    draw_bottom_y = 2;
  }
  drawn_room = saved_room;
  load_room_links();
}


// seg008:1E0C
void __pascal far get_room_address(int room)
{
  //if (room < 0 || room > 24) printf("Tried to access room %d, not in 0..24.\n", room);
  loaded_room = (word)room;
  if (room)
  {
    curr_room_tiles = &level.fg[(room - 1) * 30];
    curr_room_modif = &level.bg[(room - 1) * 30];
  }
}

// seg008:2448
void __pascal far load_frame_to_obj()
{
  word chtab_base;
  chtab_base = id_chtab_2_kid;
  reset_obj_clip();
  load_frame();
  obj_direction = Char.direction;
  obj_id = cur_frame.image;
  // top 6 bits of sword are the chtab
  obj_chtab = chtab_base + (cur_frame.sword >> 6);
  obj_x = (char_dx_forward(cur_frame.dx) << 1) - 116;
  obj_y = cur_frame.dy + Char.y;
  if ((sbyte)(cur_frame.flags ^ obj_direction) >= 0)
  {
    // 0x80: even/odd pixel
    ++obj_x;
  }
}
