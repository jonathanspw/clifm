
			/*  ########################################
			 *  #               CliFM                  #
			 *  # 	The KISS/non-curses file manager   #
			 *  ######################################## */

/* GPL2+ License 
 * Copyright (C) 2016-2021, L. Abramovich <johndoe.arch@outlook.com>
 * All rights reserved.

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#include "helpers.h"

#include <errno.h>
#include <libintl.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "aux.h"
#include "checks.h"
#include "colors.h"
#include "config.h"
#include "exec.h"
#include "history.h"
#include "init.h"
#include "jump.h"
#include "keybinds.h"
#include "listing.h"
#include "misc.h"
#include "navigation.h"
#include "profiles.h"
#include "prompt.h"
#include "readline.h"
#include "strings.h"
#include "remotes.h"

/* Globals */

struct usrvar_t *usr_var = (struct usrvar_t *)NULL;
struct actions_t *usr_actions = (struct actions_t *)NULL;
struct ws_t *ws = (struct ws_t *)NULL;
struct kbinds_t *kbinds = (struct kbinds_t *)NULL;
struct jump_t *jump_db = (struct jump_t *)NULL;
struct bookmarks_t *bookmarks = (struct bookmarks_t *)NULL;
struct fileinfo *file_info = (struct fileinfo *)NULL;
struct remote_t *remotes = (struct remote_t *)NULL;
#ifndef _NO_SUGGESTIONS
struct suggestions_t suggestion;
#endif
/* pmsg holds the current program message type */
enum prog_msg pmsg = nomsg;
struct param xargs;
unsigned short term_cols;

int curcol = 0,
	currow = 0,
	flags;

struct termios
	orig_termios,
	shell_tmodes;

off_t total_sel_size = 0;
pid_t own_pid = 0;

unsigned short
	term_cols = 0,
	term_rows = 0;

regex_t regex_exp;
size_t *ext_colors_len = (size_t *)NULL;

short
    auto_open = UNSET,
    autocd = UNSET,
	autojump = UNSET,
	bg_proc = 0,
    case_sens_dirjump = UNSET,
    case_sens_path_comp = UNSET,
    case_sensitive = UNSET,
    cd_lists_on_the_fly = UNSET,
    cd_on_quit = UNSET,
    classify = UNSET,
    clear_screen = UNSET,
    colorize = UNSET,
    columned = UNSET,
    config_ok = 1,
    copy_n_rename = 0,
    cp_cmd = UNSET,
    cur_ws = UNSET,
    dequoted = 0,
    dirhist_map = UNSET,
    disk_usage = UNSET,
    expand_bookmarks = UNSET,
    ext_cmd_ok = UNSET,
    files_counter = UNSET,
	filter_rev = 0,
    home_ok = 1,
#ifndef _NOICONS
    icons = 0,
#endif
    internal_cmd = 0,
    is_sel = 0,
    kb_shortcut = 0,
    kbind_busy = 0,
    light_mode = UNSET,
    list_folders_first = UNSET,
    logs_enabled = UNSET,
    long_view = UNSET,
    mime_match = 0,
    min_name_trim = UNSET,
    mv_cmd = UNSET,
    no_eln = UNSET,
    no_log = 0,
    only_dirs = UNSET,
    pager = UNSET,
    tips = UNSET,
    print_msg = 0,
	print_selfiles = UNSET,
	prompt_style = UNSET,
    recur_perm_error_flag = 0,
    restore_last_path = UNSET,
    sel_is_last = 0,
    selfile_ok = 1,
    share_selbox = UNSET,
    shell_terminal = 0,
    show_hidden = UNSET,
    sort = UNSET,
    sort_reverse = 0,
    sort_switch = 0,
    splash_screen = UNSET,
    suggestions = UNSET,
    suggest_filetype_color = UNSET,
    switch_cscheme = 0,
#ifndef _NOTRASH
    tr_as_rm = UNSET,
    trash_ok = 1,
#endif
    unicode = UNSET,
    welcome_message = UNSET;

int
    argc_bk = 0,
    dirhist_cur_index = 0,
    exit_code = 0,
    dirhist_total_index = 0,
    jump_total_rank = 0,
    max_dirhist = UNSET,
    max_files = UNSET,
    max_hist = UNSET,
    min_jump_rank = UNSET,
    max_jump_total_rank = UNSET,
    max_log = UNSET,
    max_path = UNSET,
	max_printselfiles = UNSET,
    shell_is_interactive = 0,
    trash_n = 0,
    *eln_as_file = (int *)0;

size_t
    actions_n = 0,
    aliases_n = 0,
    args_n = 0,
    bm_n = 0,
    cschemes_n = 0,
    current_hist_n = 0,
    eln_as_file_n = 0,
    ext_colors_n = 0,
    files = 0,
    jump_n = 0,
    kbinds_n = 0,
    longest = 0,
    msgs_n = 0,
    path_n = 0,
    path_progsn = 0,
    prompt_cmds_n = 0,
    remotes_n = 0,
    sel_n = 0,
    user_home_len = 0,
    usrvar_n = 0;

char
    div_line_char[NAME_MAX],
    hostname[HOST_NAME_MAX],

    *ACTIONS_FILE = (char *)NULL,
    *alt_bm_file = (char *)NULL,
    *alt_config_dir = (char *)NULL,
    *alt_config_file = (char *)NULL,
    *alt_kbinds_file = (char *)NULL,
    *alt_profile = (char *)NULL,
    *BM_FILE = (char *)NULL,
    *COLORS_DIR = (char *)NULL,
    *CONFIG_DIR = (char *)NULL,
    *CONFIG_DIR_GRAL = (char *)NULL,
    *CONFIG_FILE = (char *)NULL,
    *DATA_DIR = (char *)NULL,
    *cur_cscheme = (char *)NULL,
    *DIRHIST_FILE = (char *)NULL,
    *encoded_prompt = (char *)NULL,
    *file_cmd_path = (char *)NULL,
    *filter = (char *)NULL,
    *HIST_FILE = (char *)NULL,
    *jump_suggestion = (char *)NULL,
    *KBINDS_FILE = (char *)NULL,
    *last_cmd = (char *)NULL,
    *LOG_FILE = (char *)NULL,
    *ls_colors_bk = (char *)NULL,
    *MIME_FILE = (char *)NULL,
    *MSG_LOG_FILE = (char *)NULL,
    *opener = (char *)NULL,
    *pinned_dir = (char *)NULL,
    *PLUGINS_DIR = (char *)NULL,
    *PROFILE_FILE = (char *)NULL,
    *qc = (char *)NULL,
    *REMOTES_FILE = (char *)NULL,
    *SEL_FILE = (char *)NULL,
    *STDIN_TMP_DIR = (char *)NULL,
#ifndef _NO_SUGGESTIONS
	*suggestion_buf = (char *)NULL,
    *suggestion_strategy = (char *)NULL,
#endif
    *sys_shell = (char *)NULL,
    *term = (char *)NULL,
    *TMP_DIR = (char *)NULL,
#ifndef _NOTRASH
    *TRASH_DIR = (char *)NULL,
    *TRASH_FILES_DIR = (char *)NULL,
    *TRASH_INFO_DIR = (char *)NULL,
#endif
    *usr_cscheme = (char *)NULL,
    *user_home = (char *)NULL,

    **aliases = (char **)NULL,
    **argv_bk = (char **)NULL,
    **bin_commands = (char **)NULL,
    **bookmark_names = (char **)NULL,
    **color_schemes = (char **)NULL,
    **ext_colors = (char **)NULL,
    **history = (char **)NULL,
    **messages = (char **)NULL,
    **old_pwd = (char **)NULL,
    **paths = (char **)NULL,
    **profile_names = (char **)NULL,
    **prompt_cmds = (char **)NULL,
    **sel_elements = (char **)NULL;

/* This is not a comprehensive list of commands. It only lists
 * commands long version for TAB completion */
const char *INTERNAL_CMDS[] = {
    "actions",
    "alias",
    "auto-open",
    "autocd",
    "back",
    "bookmarks",
    "colors",
    "colorschemes",
    "columns",
    "commands",
    "desel",
	"dup",
    "edit",
    "exit",
    "export",
    "filter",
    "folders-first",
    "forth",
    "help",
    "hidden",
    "history",
    "icons",
    "jump",
    "keybinds",
    "log",
    "messages",
    "mime",
    "mountpoints",
    "move",
	"new",
    "open",
    "opener",
    "pager",
    "paste",
    "path",
    "pin",
    "profile",
    "prop",
    "quit",
    "refresh",
    "reload",
    "sel",
    "selbox",
    "shell",
    "sort",
    "splash",
    "tips",
    "trash",
    "undel",
    "unicode",
    "unpin",
    "untrash",
    "version",
    NULL};

/* Just a list of internal commands and fixed parameters for the
 * auto-suggestions system */
const char *PARAM_STR[] = {
	"actions edit",
	"autocd on",
	"acd on",
	"autocd off",
	"acd off",
	"autocd status",
	"acd status",
	"alias import",
	"ao on",
	"auto-open on",
	"ao off",
	"auto-open off",
	"ao status",
	"auto-open status",
	"b hist",
	"b clear",
	"back hist",
	"back clear",
	"bm add",
	"bm del",
	"bm edit",
	"bookmarks add",
	"bookmarks del",
	"bookmarks edit",
	"cs edit",
	"colorscheme edit",
	"edit",
	"edit reset",
	"ext on",
	"ext off",
	"ext status",
	"f hist",
	"f clear",
	"forth hist",
	"forth clear",
	"fc on",
	"filescounter on",
	"fc off",
	"filescounter off",
	"fc status",
	"filescounter status",
	"ff on",
	"folders-first on",
	"ff off",
	"folders-first off",
	"ff status",
	"folders-first status",
	"ft unset",
	"filter unset",
	"hf on",
	"hf off",
	"hf status",
	"hidden on",
	"hidden off",
	"hidden status",
	"history clear",
	"icons on",
	"icons off",
	"kb edit",
	"keybinds edit",
	"kb reset",
	"keybinds reset",
	"kb readline",
	"keybinds readline",
	"l edit",
	"lm on",
	"lm off",
	"log clear",
	"mm info",
	"mm edit",
	"mm import",
	"mime info",
	"mime edit",
	"mime import",
	"msg clear",
	"messages clear",
	"net edit",
	"net mount",
	"net unmount",
	"pg on",
	"pager on",
	"pg off",
	"pager off",
	"pg status",
	"pager status",
	"pf set",
	"pf add",
	"pf del",
	"profile set",
	"profile add",
	"profile del",
	"st none",
	"st name",
	"st size",
	"st atime",
	"st btime",
	"st ctime",
	"st owner",
	"st group",
	"st ext",
	"st inode",
	"st version",
	"sort none",
	"sort name",
	"sort size",
	"sort atime",
	"sort btime",
	"sort ctime",
	"sort owner",
	"sort group",
	"sort ext",
	"sort inode",
	"sort version",
	"st rev",
	"sort rev",
	"t list",
	"t clear",
	"t del",
	"tr list",
	"tr clear",
	"tr del",
	"trash list",
	"trash clear",
	"trash del",
	"u all",
	"undel all",
	"untrash all",
	"uc on",
	"unicode on",
	"uc off",
	"unicode off",
	"uc status",
	"unicode status",
	NULL};

/* To store all the 39 color variables I use, with 46 bytes each, I need
 * a total of 1,8Kb. It's not much but it could be less if I'd use
 * dynamically allocated arrays for them (which, on the other side,
 * would make the whole thing slower and more tedious) */

/* Colors */
char
	/* File types */
    bd_c[MAX_COLOR],  /* Block device */
    ca_c[MAX_COLOR],  /* Cap file */
    cd_c[MAX_COLOR],  /* Char device */
    di_c[MAX_COLOR],  /* Directory */
    ed_c[MAX_COLOR],  /* Empty dir */
    ee_c[MAX_COLOR],  /* Empty executable */
    ef_c[MAX_COLOR],  /* Empty reg file */
    ex_c[MAX_COLOR],  /* Executable */
    fi_c[MAX_COLOR],  /* Reg file */
    ln_c[MAX_COLOR],  /* Symlink */
    mh_c[MAX_COLOR],  /* Multi-hardlink file */
    nd_c[MAX_COLOR],  /* No read directory */
    ne_c[MAX_COLOR],  /* No read empty dir */
    nf_c[MAX_COLOR],  /* No read file */
    no_c[MAX_COLOR],  /* Unknown */
    or_c[MAX_COLOR],  /* Broken symlink */
    ow_c[MAX_COLOR],  /* Other writable */
    pi_c[MAX_COLOR],  /* FIFO, pipe */
    sg_c[MAX_COLOR],  /* SGID file */
    so_c[MAX_COLOR],  /* Socket */
    st_c[MAX_COLOR],  /* Sticky (not ow)*/
    su_c[MAX_COLOR],  /* SUID file */
    tw_c[MAX_COLOR],  /* Sticky other writable */
    uf_c[MAX_COLOR],  /* Non-'stat'able file */

	/* Interface */
    bm_c[MAX_COLOR], /* Bookmarked directory */
    dc_c[MAX_COLOR], /* Files counter color */
    df_c[MAX_COLOR], /* Default color */
    dh_c[MAX_COLOR], /* Dirhist index color */
    dl_c[MAX_COLOR], /* Dividing line index color */
    el_c[MAX_COLOR], /* ELN color */
    mi_c[MAX_COLOR], /* Misc indicators color */
    sc_c[MAX_COLOR], /* Auto-suggestions: external commands */
    sh_c[MAX_COLOR], /* Auto-suggestions: history */
    sf_c[MAX_COLOR], /* Auto-suggestions: filenames */
    sx_c[MAX_COLOR], /* Auto-suggestions: internal commands and params */
    wc_c[MAX_COLOR], /* Welcome message color */

    /* Colors used in the prompt, so that \001 and \002 needs to
	 * be added. This is why MAX_COLOR + 2 */
    em_c[MAX_COLOR + 2], /* Error msg color */
    li_c[MAX_COLOR + 2], /* Sel indicator color */
    nm_c[MAX_COLOR + 2], /* Notice msg color */
    wm_c[MAX_COLOR + 2], /* Warning msg color */
    si_c[MAX_COLOR + 2], /* stealth indicator color */
    ti_c[MAX_COLOR + 2], /* Trash indicator color */
    tx_c[MAX_COLOR + 2], /* Text color */
#ifndef _NOICONS
    dir_ico_c[MAX_COLOR], /* Directories icon color */
#endif
    tx_c[MAX_COLOR + 2]; /* Text color */

#ifdef LINUX_INOTIFY
int inotify_fd, inotify_wd = -1;
unsigned int INOTIFY_MASK = /*IN_ATTRIB |*/ IN_CREATE | IN_DELETE
	| IN_DELETE_SELF | /*IN_MODIFY |*/ IN_MOVE_SELF
	| IN_MOVED_FROM | IN_MOVED_TO | IN_EXCL_UNLINK ;
#elif defined(BSD_KQUEUE)
int kq, event_fd = -1;
struct kevent events_to_monitor[NUM_EVENT_FDS];
unsigned int KQUEUE_FFLAGS = NOTE_DELETE | NOTE_EXTEND| NOTE_LINK
	| NOTE_RENAME | NOTE_REVOKE | NOTE_WRITE;
struct timespec timeout;
#endif
int watch = -1;

			/**
				 * #############################
				 * #           MAIN            #
				 * #############################
				 * */

int
main(int argc, char *argv[])
{
	/* Though this program might perfectly work on other architectures,
	 * I just didn't test anything beyond x86 and ARM */
#if !defined(__x86_64__) && !defined(__i386__) && !defined(__ARM_ARCH)
	fprintf(stderr, "%s: Unsupported CPU architecture\n", PROGRAM_NAME);
	exit(EXIT_FAILURE);
#endif

#if !defined(__linux__) && !defined(__FreeBSD__) \
&& !defined(__NetBSD__) && !defined(__OpenBSD__) && !defined(__HAIKU__)
	fprintf(stderr, _("%s: Unsupported operating system\n"), PROGRAM_NAME);
	exit(EXIT_FAILURE);
#endif

	/* Make sure we are running on a supported terminal */
	check_term();

	/* Set the default color */
/*	fputs(DEF_DF_C, stdout);
	fflush(stdout); */

	/* If running the program locally, that is, not from a path in PATH,
	 * remove the leading "./" to get the correct program invocation
	 * name */
	if (*argv[0] == '.' && *(argv[0] + 1) == '/')
		argv[0] += 2;

	/* Use the locale specified by the environment */
	setlocale(LC_ALL, "");

	unicode = DEF_UNICODE;

	/* Store external arguments to be able to rerun external_arguments()
	 * in case the user edits the config file, in which case the program
	 * must rerun init_config(), get_aliases(), get_prompt_cmds(), and
	 * then external_arguments() */
	backup_argv(argc, argv);

	/* free_stuff does some cleaning */
	atexit(free_stuff);

	user = get_user();
	get_home();

	if (geteuid() == 0)
		flags |= ROOT_USR;

	/* Running in a graphical environment? */
#if __linux__
	if (getenv("DISPLAY") != NULL && strncmp(getenv("TERM"), "linux", 5) != 0)
#else
	if (getenv("DISPLAY") != NULL)
#endif
		flags |= GUI;

	/* Get paths from PATH environment variable. These paths will be
	 * used later by get_path_programs (for the autocomplete function)
	 * and get_cmd_path() */
	path_n = (size_t)get_path_env();

	init_workspaces();

	/* Set all external arguments flags to uninitialized state */
	unset_xargs();

	/* Manage external arguments, but only if any: argc == 1 equates to
	 * no argument, since this '1' is just the program invokation name.
	 * External arguments will override initialization values
	 * (init_config) */
	if (argc > 1)
		external_arguments(argc, argv);
	/* external_arguments is executed before init_config because, if
	 * specified (-P option), it sets the value of alt_profile, which
	 * is then checked by init_config */

	check_env_filter();
	get_data_dir();

	/* Initialize program paths and files, set options from the config
	 * file, if they were not already set via external arguments, and
	 * load sel elements, if any. All these configurations are made
	 * per user basis */
	init_config();
	check_options();

	set_sel_file();
	create_tmp_files();
	load_actions();
	get_aliases();

	/* Get the list of available applications in PATH to be used by my
	 * custom TAB-completion function */
	get_path_programs();

	/* Initialize gettext() for translations */
#ifndef _NO_GETTEXT
	init_gettext();
#endif

	cschemes_n = get_colorschemes();
	set_colors(usr_cscheme ? usr_cscheme : "default", 1);

	fputs(df_c, stdout);
	fflush(stdout);

	if (flags & ROOT_USR) {
		_err(0, PRINT_PROMPT, _("%s%s: %sRunning as root%s\n"),
			bold, PROGRAM_NAME, red, df_c);
	}

	free(usr_cscheme);
	usr_cscheme = (char *)NULL;

	load_remotes();
	automount_remotes();

	if (splash_screen) {
		splash();
		splash_screen = 0;
		CLEAR;
	}

	set_start_path();

	/* Set terminal window title */
	if (flags & GUI) {
		if (xargs.cwd_in_title == 0) {
			printf("\033]2;%s\007", PROGRAM_NAME);
			fflush(stdout);
		} else {
			set_term_title(ws[cur_ws].path);
		}
	}

	exec_profile();
	load_dirhist();
	add_to_dirhist(ws[cur_ws].path);
	get_sel_files();

	/* Start listing as soon as possible to speed up startup time */
	if (cd_lists_on_the_fly && isatty(STDIN_FILENO)) {
#ifdef LINUX_INOTIFY
		/* Initialize inotify */
		inotify_fd = inotify_init1(IN_NONBLOCK);
		if (inotify_fd < 0) {
			_err('w', PRINT_PROMPT, "%s: inotify: %s\n", PROGRAM_NAME,
				strerror(errno));
		}
#elif defined(BSD_KQUEUE)
		kq = kqueue();
		if (kq < 0) {
			_err('w', PRINT_PROMPT, "%s: kqueue: %s\n", PROGRAM_NAME,
				strerror(errno));
		}
#endif
		list_dir();
	}

	create_kbinds_file();
	load_bookmarks();
	load_keybinds();
	load_jumpdb();
	if (!jump_db || xargs.path == 1)
		add_to_jumpdb(ws[cur_ws].path);

	initialize_readline();

	/*Trim the directory history file if necessary */
	check_file_size(DIRHIST_FILE, max_dirhist);

	/* Check whether we have a working shell */
	if (access(user.shell, X_OK) == -1) {
		_err('w', PRINT_PROMPT, _("%s: %s: System shell not found. "
				"Please edit the configuration file to specify a working "
				"shell.\n"), PROGRAM_NAME, user.shell);
	}

	get_prompt_cmds();
#ifndef _NOTRASH
	if (trash_ok) {
		trash_n = count_dir(TRASH_FILES_DIR, NO_CPOP);
		if (trash_n <= 2)
			trash_n = 0;
	}
#endif
	if (gethostname(hostname, sizeof(hostname)) == -1) {
		hostname[0] = '?';
		hostname[1] = '\0';
		_err('e', PRINT_PROMPT, _("%s: Error getting hostname\n"), PROGRAM_NAME);
	}

	init_shell();

	if (config_ok)
		init_history();

	/* Store history into an array to be able to manipulate it */
	get_history();

	/* Check if the 'file' command is available: we need it for Lira */
/*	if (!opener)
		file_cmd_check(); */

	get_profile_names();
	load_pinned_dir();
	set_env();

				/* ###########################
				 * #   2) MAIN PROGRAM LOOP  #
				 * ########################### */

	/* This is the main structure of any basic shell
		 1 - Infinite loop
		 2 - Grab user input
		 3 - Parse user input
		 4 - Execute command
		 See https://brennan.io/2015/01/16/write-a-shell-in-c/
		 */

	int i;
	/* 1) Infinite loop to keep the program running */
	while (1) {
		/* 2) Grab input string from the prompt */
		char *input = prompt();
		if (!input)
			continue;

		/* 3) Parse input string */
		char **cmd = parse_input_str(input);
		free(input);
		input = (char *)NULL;

		if (!cmd)
			continue;

		/* 4) Execute input string */
		char **alias_cmd = check_for_alias(cmd);
		if (alias_cmd) {
			/* If an alias is found, check_for_alias() frees cmd
			 * and returns alias_cmd in its place to be executed by
			 * exec_cmd() */
			exec_cmd(alias_cmd);

			for (i = 0; alias_cmd[i]; i++)
				free(alias_cmd[i]);

			free(alias_cmd);
			alias_cmd = (char **)NULL;
		} else {
			exec_cmd(cmd);

			i = (int)args_n + 1;
			while (--i >= 0)
				free(cmd[i]);

			free(cmd);
			cmd = (char **)NULL;
		}
	}

	return exit_code; /* Never reached */
}
