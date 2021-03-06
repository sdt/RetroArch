/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2011-2015 - Daniel De Matteis
 *
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include <compat/strl.h>
#include <file/file_path.h>
#include <retro_inline.h>

#include "menu.h"
#include "menu_cbs.h"
#include "menu_display.h"
#include "menu_hash.h"
#include "menu_entry.h"
#include "menu_setting.h"
#include "menu_input.h"
#include "menu_shader.h"
#include "menu_navigation.h"

#include "../general.h"
#include "../performance.h"
#include "../retroarch.h"
#include "../input/input_common.h"
#include "../input/input_autodetect.h"

static int action_iterate_help(char *s, size_t len, const char *label)
{
   unsigned i;
   menu_handle_t *menu       = menu_driver_get_ptr();
   settings_t *settings      = config_get_ptr();

   switch (menu->help_screen_type)
   {
      case MENU_HELP_WELCOME:
         {
            static int64_t timeout_end;
            int64_t timeout;
            static bool timer_begin = false;
            static bool timer_end   = false;
            int64_t current         = rarch_get_time_usec();

            if (!timer_begin)
            {
               timeout_end = rarch_get_time_usec() +
                  3 /* seconds */ * 1000000;
               timer_begin = true;
               timer_end   = false;
            }

            timeout = (timeout_end - current) / 1000000;

            menu_hash_get_help(MENU_LABEL_WELCOME_TO_RETROARCH,
                  s, len);

            if (!timer_end && timeout <= 0)
            {
               timer_end   = true;
               timer_begin = false;
               timeout_end = 0;
               menu->help_screen_type = MENU_HELP_NONE;
               return 1;
            }
         }
         break;
      case MENU_HELP_CONTROLS:
         {
            char s2[PATH_MAX_LENGTH];
            const unsigned binds[] = {
               RETRO_DEVICE_ID_JOYPAD_UP,
               RETRO_DEVICE_ID_JOYPAD_DOWN,
               RETRO_DEVICE_ID_JOYPAD_A,
               RETRO_DEVICE_ID_JOYPAD_B,
               RETRO_DEVICE_ID_JOYPAD_SELECT,
               RETRO_DEVICE_ID_JOYPAD_START,
               RARCH_MENU_TOGGLE,
               RARCH_QUIT_KEY,
               RETRO_DEVICE_ID_JOYPAD_X,
               RETRO_DEVICE_ID_JOYPAD_Y,
            };
            char desc[ARRAY_SIZE(binds)][64] = {{0}};

            for (i = 0; i < ARRAY_SIZE(binds); i++)
            {
               const struct retro_keybind *keybind = (const struct retro_keybind*)
                  &settings->input.binds[0][binds[i]];
               const struct retro_keybind *auto_bind = (const struct retro_keybind*)
                  input_get_auto_bind(0, binds[i]);

               input_get_bind_string(desc[i], keybind, auto_bind, sizeof(desc[i]));
            }

            menu_hash_get_help(MENU_LABEL_VALUE_MENU_CONTROLS_PROLOG,
                  s2, sizeof(s2));

            snprintf(s, len,
                  "%s"
                  "[%s]: "
                  "%-20s\n"
                  "[%s]: "
                  "%-20s\n"
                  "[%s]: "
                  "%-20s\n"
                  "[%s]: "
                  "%-20s\n"
                  "[%s]: "
                  "%-20s\n"
                  "[%s]: "
                  "%-20s\n"
                  "[%s]: "
                  "%-20s\n"
                  "[%s]: "
                  "%-20s\n"
                  "[%s]: "
                  "%-20s\n",
                  s2,
                  menu_hash_to_str(MENU_LABEL_VALUE_BASIC_MENU_CONTROLS_SCROLL_UP),    desc[0],
                  menu_hash_to_str(MENU_LABEL_VALUE_BASIC_MENU_CONTROLS_SCROLL_DOWN),  desc[1],
                  menu_hash_to_str(MENU_LABEL_VALUE_BASIC_MENU_CONTROLS_CONFIRM),      desc[2],
                  menu_hash_to_str(MENU_LABEL_VALUE_BASIC_MENU_CONTROLS_BACK),         desc[3],
                  menu_hash_to_str(MENU_LABEL_VALUE_BASIC_MENU_CONTROLS_INFO),         desc[4],
                  menu_hash_to_str(MENU_LABEL_VALUE_BASIC_MENU_CONTROLS_START),        desc[5],
                  menu_hash_to_str(MENU_LABEL_VALUE_BASIC_MENU_CONTROLS_TOGGLE_MENU),  desc[6],
                  menu_hash_to_str(MENU_LABEL_VALUE_BASIC_MENU_CONTROLS_QUIT),         desc[7],
                  menu_hash_to_str(MENU_LABEL_VALUE_BASIC_MENU_CONTROLS_TOGGLE_KEYBOARD), desc[8]
                  );
         }
         break;
      case MENU_HELP_WHAT_IS_A_CORE:
         menu_hash_get_help(MENU_LABEL_VALUE_WHAT_IS_A_CORE_DESC,
               s, len);
         break;
      case MENU_HELP_LOADING_CONTENT:
         menu_hash_get_help(MENU_LABEL_LOAD_CONTENT,
               s, len);
         break;
      case MENU_HELP_CHANGE_VIRTUAL_GAMEPAD:
         menu_hash_get_help(MENU_LABEL_VALUE_HELP_CHANGE_VIRTUAL_GAMEPAD_DESC,
               s, len);
         break;
      case MENU_HELP_AUDIO_VIDEO_TROUBLESHOOTING:
         menu_hash_get_help(MENU_LABEL_VALUE_HELP_AUDIO_VIDEO_TROUBLESHOOTING_DESC,
               s, len);
         break;
      case MENU_HELP_SCANNING_CONTENT:
         menu_hash_get_help(MENU_LABEL_VALUE_HELP_SCANNING_CONTENT_DESC,
               s, len);
         break;
      case MENU_HELP_EXTRACT:
         menu_hash_get_help(MENU_LABEL_VALUE_EXTRACTING_PLEASE_WAIT,
               s, len);
         break;
      case MENU_HELP_NONE:
      default:
         break;
   }


   return 0;
}

static int action_iterate_info(char *s, size_t len, const char *label)
{
   uint32_t label_hash              = 0;
   menu_file_list_cbs_t *cbs        = NULL;
   menu_list_t *menu_list           = menu_list_get_ptr();
   size_t i                         = menu_navigation_get_current_selection();

   if (!menu_list)
      return 0;

   cbs = menu_list_get_actiondata_at_offset(menu_list->selection_buf, i);

   if (cbs->setting)
   {
      char needle[PATH_MAX_LENGTH];
      strlcpy(needle, cbs->setting->name, sizeof(needle));
      label_hash       = menu_hash_calculate(needle);
   }

   return menu_hash_get_help(label_hash, s, len);
}

static int action_iterate_menu_viewport(char *s, size_t len,
      const char *label, unsigned action, uint32_t hash)
{
   int stride_x = 1, stride_y = 1;
   menu_displaylist_info_t info     = {0};
   struct retro_game_geometry *geom = NULL;
   const char *base_msg             = NULL;
   unsigned type                    = 0;
   video_viewport_t *custom         = video_viewport_get_custom();
   menu_display_t *disp             = menu_display_get_ptr();
   menu_navigation_t *nav           = menu_navigation_get_ptr();
   menu_list_t *menu_list           = menu_list_get_ptr();
   settings_t *settings             = config_get_ptr();
   struct retro_system_av_info *av_info = video_viewport_get_system_av_info();

   if (!menu_list)
      return -1;

   menu_list_get_last_stack(menu_list, NULL, NULL, &type, NULL);

   geom = (struct retro_game_geometry*)&av_info->geometry;

   if (settings->video.scale_integer)
   {
      stride_x = geom->base_width;
      stride_y = geom->base_height;
   }

   switch (action)
   {
      case MENU_ACTION_UP:
         if (type == MENU_SETTINGS_CUSTOM_VIEWPORT)
         {
            custom->y      -= stride_y;
            custom->height += stride_y;
         }
         else if (custom->height >= (unsigned)stride_y)
            custom->height -= stride_y;

         event_command(EVENT_CMD_VIDEO_APPLY_STATE_CHANGES);
         break;

      case MENU_ACTION_DOWN:
         if (type == MENU_SETTINGS_CUSTOM_VIEWPORT)
         {
            custom->y += stride_y;
            if (custom->height >= (unsigned)stride_y)
               custom->height -= stride_y;
         }
         else
            custom->height += stride_y;

         event_command(EVENT_CMD_VIDEO_APPLY_STATE_CHANGES);
         break;

      case MENU_ACTION_LEFT:
         if (type == MENU_SETTINGS_CUSTOM_VIEWPORT)
         {
            custom->x     -= stride_x;
            custom->width += stride_x;
         }
         else if (custom->width >= (unsigned)stride_x)
            custom->width -= stride_x;

         event_command(EVENT_CMD_VIDEO_APPLY_STATE_CHANGES);
         break;

      case MENU_ACTION_RIGHT:
         if (type == MENU_SETTINGS_CUSTOM_VIEWPORT)
         {
            custom->x += stride_x;
            if (custom->width >= (unsigned)stride_x)
               custom->width -= stride_x;
         }
         else
            custom->width += stride_x;

         event_command(EVENT_CMD_VIDEO_APPLY_STATE_CHANGES);
         break;

      case MENU_ACTION_CANCEL:
         menu_list_pop_stack(menu_list);

         if (hash == MENU_LABEL_CUSTOM_VIEWPORT_2)
         {
            info.list          = menu_list->menu_stack;
            info.type          = MENU_SETTINGS_CUSTOM_VIEWPORT;
            info.directory_ptr = nav->selection_ptr;

            menu_displaylist_push_list(&info, DISPLAYLIST_INFO);
         }
         break;

      case MENU_ACTION_OK:
         menu_list_pop_stack(menu_list);

         if (type == MENU_SETTINGS_CUSTOM_VIEWPORT
               && !settings->video.scale_integer)
         {
            info.list          = menu_list->menu_stack;
            strlcpy(info.label,
                  menu_hash_to_str(MENU_LABEL_CUSTOM_VIEWPORT_2),
                  sizeof(info.label));
            info.type          = 0;
            info.directory_ptr = nav->selection_ptr;

            menu_displaylist_push_list(&info, DISPLAYLIST_INFO);
         }
         break;

      case MENU_ACTION_START:
         if (!settings->video.scale_integer)
         {
            video_viewport_t vp;
            video_driver_viewport_info(&vp);

            if (type == MENU_SETTINGS_CUSTOM_VIEWPORT)
            {
               custom->width  += custom->x;
               custom->height += custom->y;
               custom->x       = 0;
               custom->y       = 0;
            }
            else
            {
               custom->width   = vp.full_width - custom->x;
               custom->height  = vp.full_height - custom->y;
            }

            event_command(EVENT_CMD_VIDEO_APPLY_STATE_CHANGES);
         }
         break;

      case MENU_ACTION_MESSAGE:
         if (disp)
            disp->msg_force = true;
         break;

      default:
         break;
   }

   menu_list_get_last_stack(menu_list, NULL, &label, &type, NULL);

   if (settings->video.scale_integer)
   {
      custom->x     = 0;
      custom->y     = 0;
      custom->width = ((custom->width + geom->base_width - 1) /
            geom->base_width) * geom->base_width;
      custom->height = ((custom->height + geom->base_height - 1) /
            geom->base_height) * geom->base_height;
      base_msg       = "Set scale";
       
      snprintf(s, len, "%s (%4ux%4u, %u x %u scale)",
            base_msg,
            custom->width, custom->height,
            custom->width / geom->base_width,
            custom->height / geom->base_height);
   }
   else
   {
      if (type == MENU_SETTINGS_CUSTOM_VIEWPORT)
         base_msg = menu_hash_to_str(MENU_LABEL_VALUE_CUSTOM_VIEWPORT_1);
      else if (hash == MENU_LABEL_CUSTOM_VIEWPORT_2)
         base_msg = menu_hash_to_str(MENU_LABEL_VALUE_CUSTOM_VIEWPORT_2);

      snprintf(s, len, "%s (%d, %d : %4ux%4u)",
            base_msg, custom->x, custom->y, custom->width, custom->height);
   }

   if (!custom->width)
      custom->width = stride_x;
   if (!custom->height)
      custom->height = stride_y;

   aspectratio_lut[ASPECT_RATIO_CUSTOM].value =
      (float)custom->width / custom->height;

   event_command(EVENT_CMD_VIDEO_APPLY_STATE_CHANGES);

   return 0;
}

enum action_iterate_type
{
   ITERATE_TYPE_DEFAULT = 0,
   ITERATE_TYPE_HELP,
   ITERATE_TYPE_INFO,
   ITERATE_TYPE_MESSAGE,
   ITERATE_TYPE_VIEWPORT,
   ITERATE_TYPE_BIND
};

static enum action_iterate_type action_iterate_type(uint32_t hash)
{
   switch (hash)
   {
      case MENU_LABEL_HELP:
      case MENU_LABEL_HELP_CONTROLS:
      case MENU_LABEL_HELP_WHAT_IS_A_CORE:
      case MENU_LABEL_HELP_LOADING_CONTENT:
      case MENU_LABEL_HELP_CHANGE_VIRTUAL_GAMEPAD:
      case MENU_LABEL_HELP_AUDIO_VIDEO_TROUBLESHOOTING:
      case MENU_LABEL_HELP_SCANNING_CONTENT:
         return ITERATE_TYPE_HELP;
      case MENU_LABEL_INFO_SCREEN:
         return ITERATE_TYPE_INFO;
      case MENU_LABEL_MESSAGE:
         return ITERATE_TYPE_MESSAGE;
      case MENU_LABEL_CUSTOM_VIEWPORT_1:
      case MENU_LABEL_CUSTOM_VIEWPORT_2:
         return ITERATE_TYPE_VIEWPORT;
      case MENU_LABEL_CUSTOM_BIND:
      case MENU_LABEL_CUSTOM_BIND_ALL:
      case MENU_LABEL_CUSTOM_BIND_DEFAULTS:
         return ITERATE_TYPE_BIND;
   }

   return ITERATE_TYPE_DEFAULT;
}

/**
 * menu_iterate:
 * @input                    : input sample for this frame
 * @old_input                : input sample of the previous frame
 * @trigger_input            : difference' input sample - difference
 *                             between 'input' and 'old_input'
 *
 * Runs RetroArch menu for one frame.
 *
 * Returns: 0 on success, -1 if we need to quit out of the loop. 
 **/
int menu_iterate(bool render_this_frame, unsigned action)
{
   menu_entry_t entry;
   enum action_iterate_type iterate_type;
   size_t selected;
   const char *label         = NULL;
   int ret                   = 0;
   uint32_t hash             = 0;
   menu_handle_t *menu       = menu_driver_get_ptr();
   menu_navigation_t *nav    = menu_navigation_get_ptr();
   menu_display_t *disp      = menu_display_get_ptr();
   menu_list_t *menu_list    = menu_list_get_ptr();

   if (render_this_frame)
      menu_animation_update_time();
   menu_list_get_last_stack(menu_list, NULL, &label, NULL, NULL);

   if (!menu || !menu_list)
      return 0;

   menu->state.fb_is_dirty     = false;
   menu->state.do_messagebox   = false;
   menu->state.do_render       = false;
   menu->state.do_pop_stack    = false;
   menu->state.do_post_iterate = false;
   menu->state.pop_selected    = NULL;
   menu->state.msg[0]          = '\0';

   hash = menu_hash_calculate(label);
   
   iterate_type              = action_iterate_type(hash);

   if (action != MENU_ACTION_NOOP || menu_entries_needs_refresh() || menu_display_update_pending())
   {
      if (render_this_frame)
         menu->state.fb_is_dirty = true;
   }

   switch (iterate_type)
   {
      case ITERATE_TYPE_HELP:
         ret = action_iterate_help(menu->state.msg, sizeof(menu->state.msg), label);
         if (render_this_frame)
            menu->state.do_render       = true;
         menu->state.pop_selected    = NULL;
         menu->state.do_messagebox   = true;
         menu->state.do_pop_stack    = true;
         menu->state.do_post_iterate = true;
         if (ret == 1)
            action = MENU_ACTION_OK;
         break;
      case ITERATE_TYPE_BIND:
         if (menu_input_bind_iterate(menu->state.msg, sizeof(menu->state.msg)))
            menu_list_pop_stack(menu_list);
         else
            menu->state.do_messagebox = true;
         if (render_this_frame)
            menu->state.do_render        = true;
         break;
      case ITERATE_TYPE_VIEWPORT:
         ret = action_iterate_menu_viewport(menu->state.msg, sizeof(menu->state.msg), label, action, hash);
         if (render_this_frame)
            menu->state.do_render       = true;
         menu->state.do_messagebox   = true;
         break;
      case ITERATE_TYPE_INFO:
         ret = action_iterate_info(menu->state.msg, sizeof(menu->state.msg), label);
         menu->state.pop_selected    = &nav->selection_ptr;
         if (render_this_frame)
            menu->state.do_render       = true;
         menu->state.do_messagebox   = true;
         menu->state.do_pop_stack    = true;
         menu->state.do_post_iterate = true;
         break;
      case ITERATE_TYPE_MESSAGE:
         strlcpy(menu->state.msg, disp->message_contents, sizeof(menu->state.msg));
         menu->state.pop_selected    = &nav->selection_ptr;
         menu->state.do_messagebox   = true;
         menu->state.do_pop_stack    = true;
         break;
      case ITERATE_TYPE_DEFAULT:
         selected = menu_navigation_get_current_selection();
         /* FIXME: selected > selection_buf->list->size, i don't know why. */
         selected = max(min(selected, menu_list_get_size(menu_list)-1), 0);

         menu_entry_get(&entry,    selected, NULL, false);
         ret = menu_entry_action(&entry, selected, (enum menu_action)action);

         if (ret)
            goto end;

         menu->state.do_post_iterate = true;
         if (render_this_frame)
            menu->state.do_render       = true;

         /* Have to defer it so we let settings refresh. */
         if (menu->push_help_screen)
         {
            menu_displaylist_info_t info = {0};

            info.list = menu_list->menu_stack;
            strlcpy(info.label,
                  menu_hash_to_str(MENU_LABEL_HELP),
                  sizeof(info.label));

            menu_displaylist_push_list(&info, DISPLAYLIST_HELP);
         }
         break;
   }

   if (menu->state.do_pop_stack && action == MENU_ACTION_OK)
      menu_list_pop(menu_list->menu_stack, menu->state.pop_selected);
   
   if (menu->state.do_post_iterate)
      menu_input_post_iterate(&ret, action);

end:
   if (ret)
      return -1;
   return 0;
}

int menu_iterate_render(void)
{
   const menu_ctx_driver_t *driver = menu_ctx_driver_get_ptr();
   menu_handle_t *menu       = menu_driver_get_ptr();

   if (!menu)
      return -1;

   if (menu->state.fb_is_dirty != menu->state.do_messagebox)
      menu->state.fb_is_dirty = true;

   if (menu->state.fb_is_dirty)
      menu_display_fb_set_dirty();

   if (menu->state.do_messagebox && menu->state.msg[0] != '\0')
   {
      const ui_companion_driver_t *ui = ui_companion_get_ptr();
      if (driver->render_messagebox)
         driver->render_messagebox(menu->state.msg);
      if (ui_companion_is_on_foreground())
      {
         if (ui->render_messagebox)
            ui->render_messagebox(menu->state.msg);
      }
   }
      
   if (menu->state.do_render)
   {
      if (driver->render)
         driver->render();
   }

   if (menu_driver_alive() && !rarch_main_is_idle())
      menu_display_fb();

   menu_driver_set_texture();

   return 0;
}
