/*# name=Menu server enumerations and structures
*/

/* Enumeration `option' -- All possible values for menu.option[x].type */

typedef enum
{
  nothing,

  MISC_BLOCK=100, display_menu, display_file, message, file, other,
                  o_press_enter, key_poke, clear_stacked, o_if,
                  o_menupath, o_cls, mex, link_menu, o_return,


  XTERN_BLOCK=200, xtern_erlvl, xtern_dos, xtern_run, xtern_chain,
                   xtern_concur,

  MAIN_BLOCK=300, goodbye, statistics, o_yell, userlist, o_version,
                  user_editor, leave_comment, climax,

  MSG_BLOCK=400, same_direction, read_next, read_previous,
                 enter_message, msg_reply, read_nonstop,
                 read_original, read_reply, msg_list, msg_scan,
                 msg_inquir, msg_kill, msg_hurl, forward, msg_upload,
                 xport, read_individual, msg_checkmail, msg_change,
                 msg_tag, msg_browse, msg_current, msg_edit_user,
                 msg_upload_qwk, msg_toggle_kludges, msg_unreceive,
                 msg_restrict, msg_area, msg_track, msg_dload_attach,
                 msg_reply_area,

  FILE_BLOCK=500, locate, file_titles, file_type, upload, download, raw,
                  file_kill, contents, file_hurl, override_path,
                  newfiles, file_tag, file_area,

  /* Options generally found on the Change Setup menu */

  CHANGE_BLOCK=600, chg_city, chg_password, chg_help, chg_nulls,
                    chg_width, chg_length, chg_tabs, chg_more,
                    chg_video, chg_editor, chg_clear, chg_ibm,
                    chg_phone, chg_realname, chg_hotkeys,
                    chg_language, chg_userlist, chg_protocol,
                    chg_fsr, chg_archiver, chg_rip,

  EDIT_BLOCK=700, edit_save, edit_abort, edit_list, edit_edit,
                  edit_insert, edit_delete, edit_continue, edit_to,
                  edit_from, edit_subj, edit_handling, read_diskfile,
                  edit_quote,

  /* Stuff that was hacked on after the original implementation */

  CHAT_BLOCK=800, who_is_on, o_page, o_chat_cb, chat_toggle, o_chat_pvt,
    
  END_BLOCK,


  /* Everything below here is RESERVED by Maximus for future uses!         *
   * Also, everything ABOVE is fairly stable.  If changes have to be made, *
   * the old options above will NOT be re-used.  For example, if the       *
   * `edit_insert' command should become obsoleted for some reason, that   *
   * slot would either get retired and do nothing, or perform the NEW      *
   * edit_insert function.                                                 */

  rsvd=32766  /* This was stuck in to make sure that the `option'          *
               * enumeration uses a word, instead of a byte, in case we    *
               * really expand this structure sometime soon.               */

} option;

