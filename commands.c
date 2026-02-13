/* Copyright 2011, 2012, 2014 Bert Muennich
 *
 * This file is part of sxiv.
 *
 * sxiv is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * sxiv is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with sxiv.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "sxiv.h"
#define _IMAGE_CONFIG
#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

// commands used ind cg_run_cmd()
char *delfiles = "del-file.sh";
char *edittags = "edit-tags.sh";
char *editmultitags = "edit-tags-multi.sh";
char *openfolder = "open-folder.sh";
char *spawnshell = "spawn-shell.sh";
char *movefiles = "move-file.sh";
char *fullinfo = "full-info.sh";
char *menu = "context_menu.sh";

// tu sa zadelkarowane funkcje ktore beda uzywane ale sa w innych plikach
void remove_file(int, bool);
void move_one_image(int, int);
void move_img(int);
void show_left_panel();
//void edit_tags(void);
//void edit_tags(char*);
void set_color_for_bg(win_t*,const char*);
void run_ext_command(char*);
void run_ext_command_current_file(char*);
void read_tags(void);
void load_image(int);
bool mark_image(int, bool);
void close_info(void);
void open_info(void);
int ptr_third_x(void);
void redraw(void);
void reset_cursor(void);
void animate(void);
void slideshow(void);
void set_timeout(timeout_f, int, bool);
void reset_timeout(timeout_f);

extern appmode_t mode;
extern img_t img;
extern tns_t tns;
extern win_t win;

extern fileinfo_t *files;
extern int filecnt, fileidx;
extern int alternate;
extern int markcnt;
extern int markidx;

extern int prefix;
char *opt = "navigate";
//typedef enum wheel_mode { NAVIGATE, ZOOM } wh_mod;
//wh_mod wheel_mode;
wh_mod wheel_mode = NAVIGATE;
extern bool extprefix;
extern bool extractor;
extern bool quicktags;

bool cg_edit_tags(arg_t _) {
    fprintf(stderr, "Edit tags in commands.c\n");
    //edit_tags(edittags);
    run_ext_command_current_file(edittags);
return true;
}

bool cg_show_tags(arg_t _) {
    //read_tags();
    show_left_panel();
return true;
}

bool cg_run_cmd(arg_t _) {
    fprintf(stderr, "run_cmd in commands.c\n");
	fprintf(stderr, "param:%d\n",_);

		switch (_) {
			case 1:
	            fprintf(stderr, "case 1:%d\n",_);
                run_ext_command(editmultitags);
				break;
			case 2:
	            fprintf(stderr, "case 2:%d\n",_);
	            fprintf(stderr, "Pressed Delete");
                run_ext_command(delfiles);
                load_image(fileidx);    // refresh after remove 
				break;
			case 3:
	            fprintf(stderr, "case 3:%d\n",_);
                run_ext_command_current_file(openfolder);
				break;
			case 4:
	            fprintf(stderr, "case 4:%d\n",_);
                run_ext_command_current_file(spawnshell);
				break;
			case 5:
	            fprintf(stderr, "case 5:%d\n",_);
                run_ext_command(movefiles);
                load_image(fileidx);    // refresh after remove 
				break;
			case 6:
	            fprintf(stderr, "case 6:%d\n",_);
                run_ext_command_current_file(fullinfo);
				break;
			case 7:
	            fprintf(stderr, "case 7:%d\n",_);
                run_ext_command_current_file(menu);
				break;
	    }
		tns.dirty = true;
	    img.dirty = true;



return true;
}

bool ct_move_img(arg_t direction)
{
	fprintf(stderr, "Direction:%d\n",direction);
	/*fprintf(stderr, "index:%d\n",fileidx);
	fprintf(stderr, "total:%d\n",filecnt);*/
	if (fileidx == 0 && direction == DIR_LEFT) {
		fprintf(stderr, "Nie da sie\n");
		return true;
	}	
	if (fileidx +1  == filecnt && direction == DIR_RIGHT) {
		fprintf(stderr, "Nie da sie\n");
		return true;
	}
	move_one_image(fileidx, direction);
	
	/* te kolejne 3 odswierzaja thubbsy */
	tns_free(&tns);
	tns_init(&tns, files, &filecnt, &fileidx, &win);
	tns.dirty = true;
	tns_move_selection(&tns, direction, 1);
	return true;
}

bool cg_make_index(arg_t _)
{
	unsigned int i = 1;
	FILE *fh;
	char filename[20] = "index.idx";
	fh=fopen(filename, "w");
	if (fh == NULL) {
		fprintf(stderr, "%s : couldn't open.", filename);
		return -1;
	}
    for (i = 0; i < filecnt; i++) {
        //printf("%s\n", files[i].name);
        fprintf(fh, "%s\n", files[i].name);
    }
	fclose(fh);

	return true;
}

bool cg_dump_files(arg_t _)
{
	unsigned int i = 1;
	fprintf(stderr, "\n1 - save selected\n2 - save all files");
	fprintf(stderr, "\nOption: %d\n", _);

	FILE *fh;
	/*char buf[80];

	if (mkdir("/tmp/sxiv", 0755) == -1) {
		fprintf(stderr, "error");
		return -1;
	}

	fprintf(stderr, "created dir");
	
	sprintf(buf, "/tmp/sxiv/dump-%d", getpid());
	fprintf(stderr, "created file"); */
	
	char filename[20] = "selection-1";

	while (true) {
		fprintf(stderr, "TEST %s", filename);
		fprintf(stderr, " --> %d --> ", access(filename, F_OK));
		if (access(filename, F_OK) == 0 ) 
		{
			fprintf(stderr, "EXISTS: %d\n", i);
			sprintf(filename, "selection-%d", i);
			i++;
		} else {
			fprintf(stderr, "\nOK\n");
			fprintf(stderr, "Created: %s\n", filename);
			break;
		}
	}

	fh=fopen(filename, "w");
	if (fh == NULL) {
		fprintf(stderr, "%s : couldn't open.", filename);
		return -1;
	}

	if (_ == SELECTED) {  /* only selected */
		for (i = 0; i < filecnt; i++) {
			if (files[i].flags & FF_MARK) {
				printf("%s\n", files[i].name);
				fprintf(fh, "%s\n", files[i].name);
			}
		}
	} else { 	/* all file list */
		for (i = 0; i < filecnt; i++) {
			printf("%s\n", files[i].name);
			fprintf(fh, "%s\n", files[i].name);
		}
	}
	fclose(fh);

	return true;
}

bool cg_del_selected()
{
	int i;

		for (i = filecnt - 1; i > -1; i--) {
			printf("Sprawdzam: %d %s\n", i, files[i].name);
			if (files[i].flags & FF_MARK) {
				printf("Removing: %s\n", files[i].name);
				remove_file(i, true);
			}
	}
	if (mode == MODE_IMAGE)
		load_image(i);
	else
		tns.dirty = true;

	return true;
}

bool cg_quit(arg_t _)
{
	unsigned int i;

	//if (options->to_stdout && markcnt > 0) {
	if (markcnt > 0) {
		for (i = 0; i < filecnt; i++) {
			if (files[i].flags & FF_MARK)
				printf("%s\n", files[i].path);
		}
	} else printf("%s\n", files[fileidx].path);
	exit(EXIT_SUCCESS);
}

bool cg_switch_mode(arg_t _)
{
	if (mode == MODE_IMAGE) {
		if (tns.thumbs == NULL)
			tns_init(&tns, files, &filecnt, &fileidx, &win);
		img_close(&img, false);
		reset_timeout(reset_cursor);
		if (img.ss.on) {
			img.ss.on = false;
			reset_timeout(slideshow);
		}
		tns.dirty = true;
		mode = MODE_THUMB;
	} else {
		load_image(fileidx);
		mode = MODE_IMAGE;
	}
	return true;
}

bool cg_toggle_fullscreen(arg_t _)
{
	win_toggle_fullscreen(&win);
	/* redraw after next ConfigureNotify event */
	set_timeout(redraw, TO_REDRAW_RESIZE, false);
	if (mode == MODE_IMAGE)
		img.checkpan = img.dirty = true;
	else
		tns.dirty = true;
	return false;
}

bool cg_set_bg_color(arg_t _)
{
    static const char * const RED = "#FF0000";
	set_color_for_bg(&win, RED);
	load_image(fileidx);
    redraw();
    return false;
}

bool cg_toggle_bar(arg_t _)
{
	win_toggle_bar(&win);
	if (mode == MODE_IMAGE) {
		if (win.bar.h > 0)
			open_info();
		else
			close_info();
		img.checkpan = img.dirty = true;
	} else {
		tns.dirty = true;
	}
	return true;
}

bool cg_prefix_external(arg_t _)
{
	extprefix = true;
	return false;
}

bool ci_set_mode_extractor(arg_t _)
{
	extractor = true;
    //cg_unmark_all();
    fprintf(stderr, "SET MODE: Extractor\n");
	return false;
}

bool ci_set_mode_taging(arg_t _)
{
	quicktags = true;
    //cg_unmark_all();
    fprintf(stderr, "SET MODE: Quick Taging\n");
	return false;
}

bool cg_reload_image(arg_t _)
{
	if (mode == MODE_IMAGE) {
		load_image(fileidx);
	} else {
		win_set_cursor(&win, CURSOR_WATCH);
		if (!tns_load(&tns, fileidx, true, false)) {
			remove_file(fileidx, false);
			tns.dirty = true;
		}
	}
	return true;
}

bool cg_remove_image(arg_t _)
{
	remove_file(fileidx, true);
	if (mode == MODE_IMAGE)
		load_image(fileidx);
	else
		tns.dirty = true;
	return true;
}

bool cg_first(arg_t _)
{
	if (mode == MODE_IMAGE && fileidx != 0) {
		load_image(0);
		return true;
	} else if (mode == MODE_THUMB && fileidx != 0) {
		fileidx = 0;
		tns.dirty = true;
		return true;
	} else {
		return false;
	}
}

bool cg_n_or_last(arg_t _)
{
	int n = prefix != 0 && prefix - 1 < filecnt ? prefix - 1 : filecnt - 1;

	if (mode == MODE_IMAGE && fileidx != n) {
		load_image(n);
		return true;
	} else if (mode == MODE_THUMB && fileidx != n) {
		fileidx = n;
		tns.dirty = true;
		return true;
	} else {
		return false;
	}
}

bool cg_scroll_screen(arg_t dir)
{
	if (mode == MODE_IMAGE)
		return img_pan(&img, dir, -1);
	else
		return tns_scroll(&tns, dir, true);
}

bool cg_zoom(arg_t d)
{
	if (mode == MODE_THUMB)
		return tns_zoom(&tns, d);
	else if (d > 0)
		return img_zoom_in(&img);
	else if (d < 0)
		return img_zoom_out(&img);
	else
		return false;
}

bool cg_toggle_image_mark(arg_t _)
{
	return mark_image(fileidx, !(files[fileidx].flags & FF_MARK));
}

bool cg_reverse_marks(arg_t _)
{
	int i;

	for (i = 0; i < filecnt; i++) {
		files[i].flags ^= FF_MARK;
		markcnt += files[i].flags & FF_MARK ? 1 : -1;
	}
	if (mode == MODE_THUMB)
		tns.dirty = true;
	return true;
}

bool cg_mark_range(arg_t _)
{
	int d = markidx < fileidx ? 1 : -1, end, i;
	bool dirty = false, on = !!(files[markidx].flags & FF_MARK);

	for (i = markidx + d, end = fileidx + d; i != end; i += d)
		dirty |= mark_image(i, on);
	return dirty;
}

bool cg_unmark_all(arg_t _)
{
	int i;

	for (i = 0; i < filecnt; i++)
		files[i].flags &= ~FF_MARK;
	markcnt = 0;
	if (mode == MODE_THUMB)
		tns.dirty = true;
	return true;
}

bool cg_navigate_marked(arg_t n)
{
	int d, i;
	int new = fileidx;
	
	if (prefix > 0)
		n *= prefix;
	d = n > 0 ? 1 : -1;
	for (i = fileidx + d; n != 0 && i >= 0 && i < filecnt; i += d) {
		if (files[i].flags & FF_MARK) {
			n -= d;
			new = i;
		}
	}
	if (new != fileidx) {
		if (mode == MODE_IMAGE) {
			load_image(new);
		} else {
			fileidx = new;
			tns.dirty = true;
		}
		return true;
	} else {
		return false;
	}
}

bool cg_change_gamma(arg_t d)
{
	if (img_change_gamma(&img, d * (prefix > 0 ? prefix : 1))) {
		if (mode == MODE_THUMB)
			tns.dirty = true;
		return true;
	} else {
		return false;
	}
}

bool ci_change_mode(arg_t n)
{

    if (n == 1) {  // force mode navigate
        wheel_mode = NAVIGATE;
	    img_fit_win(&img, SCALE_FIT);
	    fprintf(stderr, "SET IMAGE:SCALE_FIT\n");
	    fprintf(stderr, "SET MODE:navigate\n");
        return true;
    }

    // swap modes
    if (wheel_mode == NAVIGATE) {
        wheel_mode = ZOOM;
	    fprintf(stderr, "SET MODE:zoom\n");
    } else {
        wheel_mode = NAVIGATE;
	    fprintf(stderr, "SET MODE:navigate\n");
    }
    return true;
}

bool ci_navigate(arg_t n)
{
	if (prefix > 0)
		n *= prefix;
	n += fileidx;
	if (n < 0)
		n = 0;
	if (n >= filecnt)
		n = filecnt - 1;

	if (n != fileidx) {
		load_image(n);
		return true;
	} else {
		return false;
	}
}

bool ci_nav_or_zoom(arg_t n)
{
    fprintf(stderr, "WHEEL_MODE:%d\n",wheel_mode);
    if (wheel_mode == NAVIGATE) {
        ci_navigate(n);
    } else { 
	    //reverse to make wheel up makes zoom in
        cg_zoom(n*-1);
    }
    return true;
}

bool ci_cursor_navigate(arg_t _)
{
	return ci_navigate(ptr_third_x() - 1);
}

bool ci_alternate(arg_t _)
{
	load_image(alternate);
	return true;
}

bool ci_navigate_frame(arg_t d)
{
	if (prefix > 0)
		d *= prefix;
	return !img.multi.animate && img_frame_navigate(&img, d);
}

bool ci_toggle_animation(arg_t _)
{
	bool dirty = false;

	if (img.multi.cnt > 0) {
		img.multi.animate = !img.multi.animate;
		if (img.multi.animate) {
			dirty = img_frame_animate(&img);
			set_timeout(animate, img.multi.frames[img.multi.sel].delay, true);
		} else {
			reset_timeout(animate);
		}
	}
	return dirty;
}

bool ci_scroll(arg_t dir)
{
	return img_pan(&img, dir, prefix);
}

bool ci_scroll_to_edge(arg_t dir)
{
	return img_pan_edge(&img, dir);
}

bool ci_drag(arg_t mode)
{
	int x, y, ox, oy;
	float px, py;
	XEvent e;

	if ((int)(img.w * img.zoom) <= win.w && (int)(img.h * img.zoom) <= win.h)
		return false;
	
	win_set_cursor(&win, CURSOR_DRAG);

	win_cursor_pos(&win, &x, &y);
	ox = x;
	oy = y;

	for (;;) {
		if (mode == DRAG_ABSOLUTE) {
			px = MIN(MAX(0.0, x - win.w*0.1), win.w*0.8) / (win.w*0.8)
			   * (win.w - img.w * img.zoom);
			py = MIN(MAX(0.0, y - win.h*0.1), win.h*0.8) / (win.h*0.8)
			   * (win.h - img.h * img.zoom);
		} else {
			px = img.x + x - ox;
			py = img.y + y - oy;
		}

		if (img_pos(&img, px, py)) {
			img_render(&img);
			win_draw(&win);
		}
		XMaskEvent(win.env.dpy,
		           ButtonPressMask | ButtonReleaseMask | PointerMotionMask, &e);
		if (e.type == ButtonPress || e.type == ButtonRelease)
			break;
		while (XCheckTypedEvent(win.env.dpy, MotionNotify, &e));
		ox = x;
		oy = y;
		x = e.xmotion.x;
		y = e.xmotion.y;
	}
	set_timeout(reset_cursor, TO_CURSOR_HIDE, true);
	reset_cursor();

	return true;
}

bool ci_set_zoom(arg_t zl)
{
	return img_zoom(&img, (prefix ? prefix : zl) / 100.0);
}

bool ci_fit_to_win(arg_t sm)
{
	return img_fit_win(&img, sm);
}

bool ci_rotate(arg_t degree)
{
	img_rotate(&img, degree);
	return true;
}

bool ci_flip(arg_t dir)
{
	img_flip(&img, dir);
	return true;
}

bool ci_stretch(arg_t _)
{
    if (_ == DIR_RIGHT) img_stretch(&img, 1.01, 1.0);    // expand horizontaly
    if (_ == DIR_LEFT) img_stretch(&img, 0.99, 1.0);     // squeeze hor
    if (_ == DIR_DOWN) img_stretch(&img, 1.0, 1.01);     // expant verticaly
    if (_ == DIR_UP) img_stretch(&img, 1.0, 0.99);       // squeeze ver
	return true;
}
bool ci_toggle_antialias(arg_t _)
{
	img_toggle_antialias(&img);
	return true;
}

bool ci_toggle_alpha(arg_t _)
{
	img.alpha = !img.alpha;
	img.dirty = true;
	return true;
}

bool ci_slideshow(arg_t _)
{
	if (prefix > 0) {
		img.ss.on = true;
		img.ss.delay = prefix * 10;
		set_timeout(slideshow, img.ss.delay * 100, true);
	} else if (img.ss.on) {
		img.ss.on = false;
		reset_timeout(slideshow);
	} else {
		img.ss.on = true;
	}
	return true;
}

bool ct_move_sel(arg_t dir)
{
	return tns_move_selection(&tns, dir, prefix);
}

bool ct_reload_all(arg_t _)
{
	tns_free(&tns);
	tns_init(&tns, files, &filecnt, &fileidx, &win);
	tns.dirty = true;
	return true;
}


#undef  G_CMD
#define G_CMD(c) { -1, cg_##c },
#undef  I_CMD
#define I_CMD(c) { MODE_IMAGE, ci_##c },
#undef  T_CMD
#define T_CMD(c) { MODE_THUMB, ct_##c },

const cmd_t cmds[CMD_COUNT] = {
#include "commands.lst"
};

