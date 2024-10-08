/* Copyright 2011-2013 Bert Muennich
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
#define _MAPPINGS_CONFIG
#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <locale.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h>

typedef struct {
	struct timeval when;
	bool active;
	timeout_f handler;
} timeout_t;

/* timeout handler functions: */
void redraw(void);
void reset_cursor(void);
void animate(void);
void slideshow(void);
void cg_quit(void);     // dodalem bo jua tutaj uzywam a ona jest w commands.c
void clear_resize(void);
void show_top_bar(); // dodalem bo ta funkcje uzywam tutaj a ona jest w window.c
void draw_tags(); // dodalem bo ta funkcje uzywam tutaj a ona jest w window.c
appmode_t mode;
arl_t arl;
img_t img;
tns_t tns;
win_t win;

fileinfo_t *files;
int filecnt, fileidx;
int alternate;
int markcnt;
int markidx;

int prefix;
bool extprefix;

bool resized = false;
bool top_edge = false;
bool left_edge = false;
bool bottom_edge = false;

typedef struct {
	int err;
	char *cmd;
} extcmd_t;

struct {
	extcmd_t f;
	int fd;
	unsigned int i, lastsep;
	pid_t pid;
} info;

struct {
	extcmd_t f;
	bool warned;
} keyhandler;

timeout_t timeouts[] = {
	{ { 0, 0 }, false, redraw       },
	{ { 0, 0 }, false, reset_cursor },
	{ { 0, 0 }, false, animate      },
	{ { 0, 0 }, false, slideshow    },
	{ { 0, 0 }, false, clear_resize },
};

cursor_t imgcursor[3] = {
	CURSOR_ARROW, CURSOR_ARROW, CURSOR_ARROW
};

void cleanup(void)
{
	img_close(&img, false);
	arl_cleanup(&arl);
	tns_free(&tns);
	win_close(&win);
}

void check_add_file(char *filename, bool given)
{
	char *path;

	if (*filename == '\0')
		return;

	if (access(filename, R_OK) < 0 ||
	    (path = realpath(filename, NULL)) == NULL)
	{
		if (given)
			error(0, errno, "%s", filename);
		return;
	}

	if (fileidx == filecnt) {
		filecnt *= 2;
		files = erealloc(files, filecnt * sizeof(*files));
		memset(&files[filecnt/2], 0, filecnt/2 * sizeof(*files));
	}

	files[fileidx].name = estrdup(filename);
	files[fileidx].path = path;
	if (given)
		files[fileidx].flags |= FF_WARN;
	fileidx++;
}


void move_one_image(int n, int direction)
{
    fprintf(stderr, "MOVE:Selected: %s\n", files[n].path);
    
    if (direction == DIR_LEFT) {
        fprintf(stderr, "MOVE:Direction LEFT\n");
        files[filecnt+1] = files[n-1];	/* wczesniejszy zapisuje do tempa */
        files[n-1] = files[n];			/* przesuwa zaznaczony w jedo miejsce */
        files[n] = files[filecnt+1];	/* z tempa wstawia na miejsce zaznaczonego */
    } else if (direction == DIR_RIGHT) {
        fprintf(stderr, "MOVE:Direction RIGHT\n");
        files[filecnt+1] = files[n+1];	/* nastepny zapisuje do tempa */
        files[n+1] = files[n];			/* przesuwa zaznaczony w jedo miejsce */
        files[n] = files[filecnt+1];	/* z tempa wstawia na miejsce zaznaczonego */
    } else if (direction == DIR_DOWN) {
        fprintf(stderr, "MOVE:Direction DOWN\n");
        files[n+1].path = "/root/Downloads/0463824149471c93734b21551a6d6b4b.jpg";
        //files[filecnt+1] = files[n+5];	/* nastepny zapisuje do tempa */
        ///files[n+5] = files[n];			/* przesuwa zaznaczony w jedo miejsce */
        //files[n] = files[filecnt+1];	/* z tempa wstawia na miejsce zaznaczonego */
    }
    return; 

}


void remove_file(int n, bool manual)
{
	if (n < 0 || n >= filecnt)
		return;

	if (filecnt == 1) {
		if (!manual)
			fprintf(stderr, "sxiv: no more files to display, aborting\n");
		exit(manual ? EXIT_SUCCESS : EXIT_FAILURE);
	}
	if (files[n].flags & FF_MARK)
		markcnt--;

	if (files[n].path != files[n].name)
		 free((void*) files[n].path);
	free((void*) files[n].name); 

	if (n + 1 < filecnt) {
		if (tns.thumbs != NULL) {
			memmove(tns.thumbs + n, tns.thumbs + n + 1, (filecnt - n - 1) *
			        sizeof(*tns.thumbs));
			memset(tns.thumbs + filecnt - 1, 0, sizeof(*tns.thumbs));
		}
		memmove(files + n, files + n + 1, (filecnt - n - 1) * sizeof(*files));
	}
	filecnt--;
	if (fileidx > n || fileidx == filecnt)
		fileidx--;
	if (alternate > n || alternate == filecnt)
		alternate--;
	if (markidx > n || markidx == filecnt)
		markidx--;
}

void set_timeout(timeout_f handler, int time, bool overwrite)
{
	int i;

	for (i = 0; i < ARRLEN(timeouts); i++) {
		if (timeouts[i].handler == handler) {
			if (!timeouts[i].active || overwrite) {
				gettimeofday(&timeouts[i].when, 0);
				TV_ADD_MSEC(&timeouts[i].when, time);
				timeouts[i].active = true;
			}
			return;
		}
	}
}

void reset_timeout(timeout_f handler)
{
	int i;

	for (i = 0; i < ARRLEN(timeouts); i++) {
		if (timeouts[i].handler == handler) {
			timeouts[i].active = false;
			return;
		}
	}
}

bool check_timeouts(struct timeval *t)
{
	int i = 0, tdiff, tmin = -1;
	struct timeval now;

	while (i < ARRLEN(timeouts)) {
		if (timeouts[i].active) {
			gettimeofday(&now, 0);
			tdiff = TV_DIFF(&timeouts[i].when, &now);
			if (tdiff <= 0) {
				timeouts[i].active = false;
				if (timeouts[i].handler != NULL)
					timeouts[i].handler();
				i = tmin = -1;
			} else if (tmin < 0 || tdiff < tmin) {
				tmin = tdiff;
			}
		}
		i++;
	}
	if (tmin > 0 && t != NULL)
		TV_SET_MSEC(t, tmin);
	return tmin > 0;
}

void close_info(void)
{
	if (info.fd != -1) {
		kill(info.pid, SIGTERM);
		close(info.fd);
		info.fd = -1;
	}
}

// run external commannd on selected files
void run_ext_command(char *cmd)
{
close_info();
    // ta funkcja musi ocenic czy sa zaznaczone pliki i odpalic multi-files opcje
    // albo jak nie sa zaznaczone to one file opcje
    // mozna bedzie ja przerobic zeby odpalala takze del i move-files (marked or one)
	// musi odpalic script z parametrem "cala/sciezka/do/pliku"
    fprintf(stderr, "nowa funkcja do external commands. multifile opcja \n");
    fprintf(stderr, "cmd: %s\n",cmd);

    char** argv = (char**)malloc((markcnt + 2)* sizeof(char*));  // +2 is for command and NULL
	argv[0] = cmd;
	
    if (markcnt > 0) {
    fprintf(stderr, "marked files: %d\n",markcnt);
    unsigned int i;
    unsigned int x = 0;
    for (i = 0; i < filecnt; i++) {
	    if (files[i].flags & FF_MARK) {
            x += 1;
		    argv[x] = files[i].path;
        }
	}
    argv[x+1] = NULL;
    argv[x+2] = NULL;
    argv[x+3] = NULL;
	} else {
        fprintf(stderr, "single file\n");
	    argv[1] = files[fileidx].path;
        argv[2] = NULL;
        argv[3] = NULL;
        argv[4] = NULL;
    }

	printf("array loaded\n");

	int pfd[2];
	int status;
        if (access(cmd, X_OK) != 0) {
            fprintf(stderr, "no cmd exit\n");
            return;
        }
	info.fd = -1;

	if (info.f.err != 0 || info.fd >= 0){ 
		return;
	}
	win.bar.l.buf[0] = '\0';
	if (pipe(pfd) < 0) {
		return;
	}

	if ((info.pid = fork()) == 0) {
		close(pfd[0]);
		dup2(pfd[1], 1);
        //execl(cmd, cmd, files[fileidx].path, NULL);
        execvp(cmd, argv);
		error(EXIT_FAILURE, errno, "exec: %s", cmd);
	}
	close(pfd[1]);
	wait(&status);
	if (info.pid < 0) {
		close(pfd[0]);
	} else {
		fcntl(pfd[0], F_SETFL, O_NONBLOCK);
		info.fd = pfd[0];
		info.i = info.lastsep = 0;
	}
    free(argv);
    fprintf(stderr, "koniec fork. free array\n");
}

void run_ext_command_current_file(char *cmd)
{
close_info();
    // cmd = "/root/.config/sxiv/exec/edit-tags.sh";
	// musi odpalic script z parametrem "cala/sciezka/do/pliku"
    fprintf(stderr, "funkcja external command on current file only\n");

	int pfd[2];
	int status;
    if (access(cmd, X_OK) != 0) {
        fprintf(stderr, "no cmd exit\n");
        return;
    }
	info.fd = -1;

	if (info.f.err != 0 || info.fd >= 0){ 
		return;
	}
	win.bar.l.buf[0] = '\0';
	if (pipe(pfd) < 0) {
		return;
	}

	if ((info.pid = fork()) == 0) {
		close(pfd[0]);
		dup2(pfd[1], 1);
        execl(cmd, cmd, files[fileidx].path, NULL);
		error(EXIT_FAILURE, errno, "exec: %s", cmd);
	}
	close(pfd[1]);
	wait(&status);
	if (info.pid < 0) {
		close(pfd[0]);
	} else {
		fcntl(pfd[0], F_SETFL, O_NONBLOCK);
		info.fd = pfd[0];
		info.i = info.lastsep = 0;
	}
    fprintf(stderr, "koniec fork.\n");
}

void read_tags(void)
{
close_info();
	int pfd[2];
	int status;
    //this is path to script that can get tags but this whole thing is not working yet
	char *cmd = "/root/.config/sxiv/exec/get-tags.sh";
	//fprintf(stderr,"cmd --> %s\n",cmd);
	//fprintf(stderr, "test if cmd exists -> %d\n",access(cmd, X_OK)); 
		if (access(cmd, X_OK) != 0) {
		    fprintf(stderr,"nie istnieje %s\n",cmd);
		    fprintf(stderr,"exit\n");
		    return;
		}
	info.fd = -1;

		//fprintf(stderr,"wykonauje bo plik cmd isnieje \n");
		//fprintf(stderr,"test info.fd -->  %d\n",info.fd);
	fprintf(stderr,"continue\n");
		//fprintf(stderr, "test  pfd[0] (before) %d\n",pfd[0]); 
		//fprintf(stderr, "test  pfd[1] (before) %d\n",pfd[1]); 
		//fprintf(stderr, "test  pfd[2] (before) %d\n",pfd[2]); 
	if (pipe(pfd) < 0) {
		//fprintf(stderr,"pipe error cos jest zle koniec\n");
		return;
	}
	if ((info.pid = fork()) == 0) {
		close(pfd[0]);
		dup2(pfd[1], 1);
		//close(pfd[1]);
		fprintf(stderr,"exec forked cmd -->%s\n",cmd);
		execl(cmd, cmd, files[fileidx].path, NULL);
		error(EXIT_FAILURE, errno, "exec: %s", cmd);
	}
	close(pfd[1]);
	wait(&status);
	if (info.pid < 0) {
		close(pfd[0]);
	} else {
		fcntl(pfd[0], F_SETFL, O_NONBLOCK);
		info.fd = pfd[0];
		fprintf(stderr,"test parent finish\n");
		info.i = info.lastsep = 0;
	}

	ssize_t i, n;
	//char buf[BAR_L_LEN];
	char buf[2048];

	while (true) {
		n = read(info.fd, buf, sizeof(buf));
        fprintf(stderr,"Size of buf N=%d\n",n);
		fprintf(stderr,"Readed %d bytes from info.fd -> %d\n", n, info.fd);
		fprintf(stderr,"Raw: %s\n",buf);
		if (n < 0 && errno == EAGAIN) {
			fprintf(stderr,"koniec. N<0\n");
			return;
		}
		else if (n == 0)
			goto end;
		//fprintf(stderr,"moj wykonuke --> %d\n",n);
		for (i = 0; i < n; i++) {
            //fprintf(stderr,"%02x ",buf[i]);
			fprintf(stderr,"buf[%d] -> %02x %c\n",i, buf[i], buf[i]);
			if ((buf[i] == 0x0a) || (buf[i] == '\n')) {
			    fprintf(stderr,"Got new line character\n");
	            fprintf(stderr,"Added NULL to string\n");
			    buf[i] = 0x00;  // indicate end of string
			    goto end;
            }
		}
	}
end:
	for (i = 0; i < n; i++) {
        fprintf(stderr,"%02x ",buf[i]);
    }
	fprintf(stderr,"\nbuf now: %s\n",buf);
    draw_tags(&win, &buf);  // wyswietla tagi
	win_draw(&win);
    fprintf(stderr,"koniec tagow\n");
	close_info();
}

void open_info(void)
{
	int pfd[2];
	char w[12], h[12];

	if (info.f.err != 0 || info.fd >= 0 || win.bar.h == 0)
		return;
	win.bar.l.buf[0] = '\0';
		//fprintf(stderr, "OOOooo  pfd[0] (before) %d\n",pfd[0]); 
		//fprintf(stderr, "OOOooo  pfd[1] (before) %d\n",pfd[1]); 
		//fprintf(stderr, "OOOooo  pfd[2] (before) %d\n",pfd[2]); 
	if (pipe(pfd) < 0)
		return;
	//fprintf(stderr, "OOOooo  pfd[0] (after) %d\n",pfd[0]); 
	//fprintf(stderr, "OOOooo  pfd[1] (after) %d\n",pfd[1]); 
	//fprintf(stderr, "OOOooo  pfd[2] (after) %d\n",pfd[2]); 
	if ((info.pid = fork()) == 0) {
		close(pfd[0]);
		dup2(pfd[1], 1);
		snprintf(w, sizeof(w), "%d", img.w);
		snprintf(h, sizeof(h), "%d", img.h);
		//fprintf(stderr," OOOoo run cmd\n");
		execl(info.f.cmd, info.f.cmd, files[fileidx].name, w, h, NULL);
		error(EXIT_FAILURE, errno, "exec: %s", info.f.cmd);
	}
	close(pfd[1]);
	if (info.pid < 0) {
		//fprintf(stderr," closed forked\n");
		close(pfd[0]);
	} else {
		//fprintf(stderr," OOOoo parent finish\n");
		fcntl(pfd[0], F_SETFL, O_NONBLOCK);
		info.fd = pfd[0];
		//fprintf(stderr,"tutaj %s\n",info.f.cmd);
		info.i = info.lastsep = 0;
	}
}

void read_info(void)
{
	ssize_t i, n;
	char buf[BAR_L_LEN];

		//fprintf(stderr,"OOooo info.fd %d\n",info.fd);
		//fprintf(stderr,"OOooo error --> %d\n",errno);
		//fprintf(stderr,"OOooo info.fd  %d\n",info.fd);
	while (true) {
		n = read(info.fd, buf, sizeof(buf));
		//fprintf(stderr,"OOOooo readed  %d from info.fd -> %d\n",n, info.fd);
		//fprintf(stderr,"OOOooo  %s\n",buf);
		if (n < 0 && errno == EAGAIN)
			return;
		else if (n == 0)
			goto end;
		for (i = 0; i < n; i++) {
		//fprintf(stderr,"OOOooo buf[%d]  -> %d\n",i, buf[i]);
			if (buf[i] == '\n') {
				if (info.lastsep == 0) {
					win.bar.l.buf[info.i++] = ' ';
					info.lastsep = 1;
				}
			} else {
				win.bar.l.buf[info.i++] = buf[i];
				info.lastsep = 0;
			}
			if (info.i + 1 == win.bar.l.size)
				goto end;
		}
	}
end:
	info.i -= info.lastsep;
	win.bar.l.buf[info.i] = '\0';
	//fprintf(stderr,"%s\n",win.bar.l.buf);  // tu jest to co cmd przekazal   image-info
	win_draw(&win);
	close_info();
}

void load_image(int new)
{
	bool prev = new < fileidx;
	static int current;

	if (new < 0 || new >= filecnt)
		return;

	if (win.xwin != None)
		win_set_cursor(&win, CURSOR_WATCH);
	reset_timeout(slideshow);

	if (new != current)
		alternate = current;

	img_close(&img, false);
	while (!img_load(&img, &files[new])) {
		remove_file(new, false);
		if (new >= filecnt)
			new = filecnt - 1;
		else if (new > 0 && prev)
			new--;
	}
	files[new].flags &= ~FF_WARN;
	fileidx = current = new;

	close_info();
	open_info();
	arl_setup(&arl, files[fileidx].path);

	if (img.multi.cnt > 0 && img.multi.animate)
		set_timeout(animate, img.multi.frames[img.multi.sel].delay, true);
	else
		reset_timeout(animate);
}

bool mark_image(int n, bool on)
{
	markidx = n;
	if (!!(files[n].flags & FF_MARK) != on) {
		files[n].flags ^= FF_MARK;
		markcnt += on ? 1 : -1;
		if (mode == MODE_THUMB)
			tns_mark(&tns, n, on);
		return true;
	}
	return false;
}

void bar_put(win_bar_t *bar, const char *fmt, ...)
{
	size_t len = bar->size - (bar->p - bar->buf), n;
	va_list ap;

	va_start(ap, fmt);
	n = vsnprintf(bar->p, len, fmt, ap);
	bar->p += MIN(len, n);
	va_end(ap);
}

#define BAR_SEP "  "

void update_info(void)
{
	unsigned int i, fn, fw;
	const char * mark;
	win_bar_t *l = &win.bar.l, *r = &win.bar.r;

	/* update bar contents */
	if (win.bar.h == 0)
		return;
	for (fw = 0, i = filecnt; i > 0; fw++, i /= 10);
	mark = files[fileidx].flags & FF_MARK ? "* " : "";
	l->p = l->buf;
	r->p = r->buf;
	if (mode == MODE_THUMB) {
		if (tns.loadnext < tns.end)
			bar_put(l, "Loading... %0*d", fw, tns.loadnext + 1);
		else if (tns.initnext < filecnt)
			bar_put(l, "Caching... %0*d", fw, tns.initnext + 1);
		else
			strncpy(l->buf, files[fileidx].name, l->size);
		bar_put(r, "%s[%d] %0*d/%d", mark, markcnt, fw, fileidx + 1, filecnt);
	} else {
		bar_put(r, "%s", mark);
		if (img.ss.on) {
			if (img.ss.delay % 10 != 0)
				bar_put(r, "%2.1fs" BAR_SEP, (float)img.ss.delay / 10);
			else
				bar_put(r, "%ds" BAR_SEP, img.ss.delay / 10);
		}
		if (img.gamma != 0)
			bar_put(r, "G%+d" BAR_SEP, img.gamma);
		bar_put(r, "%3d%%" BAR_SEP, (int) (img.zoom * 100.0));
		if (img.multi.cnt > 0) {
			for (fn = 0, i = img.multi.cnt; i > 0; fn++, i /= 10);
			bar_put(r, "%0*d/%d" BAR_SEP, fn, img.multi.sel + 1, img.multi.cnt);
		}
		bar_put(r, "[%d] %0*d/%d", markcnt, fw, fileidx + 1, filecnt);
		if (info.f.err)
			strncpy(l->buf, files[fileidx].name, l->size);
	}
}

void cursor_track()
{
//read_tags();
	int x, y;
	win_bar_t *l = &win.bar.l, *r = &win.bar.r;
	l->p = l->buf;
	r->p = r->buf;
	win_cursor_pos(&win, &x, &y);
		//fprintf(stderr,"przed %d\n",bottom_edge); 
       //fprintf(stderr,"Mouse motion: (%d,%d)\n", x, y);
	if (( y > win.h-20 ) && (bottom_edge == false)) { // pokazuje statusbar when mouseover
		fprintf(stderr,"statusbar on mouseover\n"); 
		bar_put(l, "info "); // add text on the bar l=left or r=right
		win_toggle_bar(&win);
		//redraw(); // nie moze tak byc bo resetuje variables np. bottom_edge
		update_info();	// mark fileind/filecnt  on the right side of bar
		win_draw(&win);
		bottom_edge = true;
	}
	if ((y < win.h-20) && (bottom_edge == true)) {
		fprintf(stderr,"set zero\n"); 
		win_toggle_bar(&win);
		img.checkpan = img.dirty = true;  // potrzebne zeby img_render zadzialalo
		img_render(&img);
		win_draw(&win);
		bottom_edge = false;
	}  // koniec statusbar when mouseover

	if (( y < 20 ) && (top_edge == false)) {  // top bar when mouseover
		fprintf(stderr,"siema\n"); 
		show_top_bar(&win);
		win_draw(&win);
		top_edge = true;
	}
	if ((y > 20) && (top_edge == true)) {
		fprintf(stderr,"set zero\n"); 
		img.checkpan = img.dirty = true;  // potrzebne zeby img_render zadzialalo
		img_render(&img);
		win_draw(&win);
		top_edge = false;
	}
	if ((top_edge == true ) && ( x > 308 ) && (x < 433 )) {  // jest na napisie
		fprintf(stderr,"xxxx ooodddppppaaallllaa  xxxx\n"); 
		read_tags();
	}
	if ((x < 43) && (left_edge == false)) {  // jest z lewej
		fprintf(stderr,"pokazuje tagi bo cursor jest z lewej strony\n"); 
		read_tags();
        //char *menu = "/root/.config/sxiv/exec/context_menu.sh ";
        //run_ext_command(menu);
		left_edge = true;
	}
	if ((x > 43) && (left_edge == true)) {  // jest z lewej
		left_edge = false;
	}
	if ((top_edge == true ) && ( x > win.w-10 ) && (x < win.w )) {  // jest na napisie
		fprintf(stderr,"odpalaa exit bo jest myszka w rogu\n"); 
        cg_quit();
	
	}
}

int ptr_third_x(void)
{
	int x, y;

	win_cursor_pos(&win, &x, &y);
	return MAX(0, MIN(2, (x / (win.w * 0.63))));
}

void redraw(void)
{
	int t;

	if (mode == MODE_IMAGE) {
		img_render(&img);
		if (img.ss.on) {
			t = img.ss.delay * 100;
			if (img.multi.cnt > 0 && img.multi.animate)
				t = MAX(t, img.multi.length);
			set_timeout(slideshow, t, false);
		}
	} else {
		tns_render(&tns);
	}
	update_info();
	win_draw(&win);
	reset_timeout(redraw);
	reset_cursor();
}

void reset_cursor(void)
{
	int i;
	//int c, i;
	cursor_t cursor = CURSOR_NONE;

	if (mode == MODE_IMAGE) {
		for (i = 0; i < ARRLEN(timeouts); i++) {
			if (timeouts[i].handler == reset_cursor) {
				if (timeouts[i].active) {
				//	c = ptr_third_x();
				//	c = MAX(fileidx > 0 ? 0 : 1, c);
				//	c = MIN(fileidx + 1 < filecnt ? 2 : 1, c);
				//	cursor = imgcursor[c];
					cursor = imgcursor[0];
					cursor_track();
				}
				break;
			}
		}
	} else {
		if (tns.loadnext < tns.end || tns.initnext < filecnt)
			cursor = CURSOR_WATCH;
		else
			cursor = CURSOR_ARROW;
	}
	win_set_cursor(&win, cursor);
}

void animate(void)
{
	if (img_frame_animate(&img)) {
		redraw();
		set_timeout(animate, img.multi.frames[img.multi.sel].delay, true);
	}
}

void slideshow(void)
{
	load_image(fileidx + 1 < filecnt ? fileidx + 1 : 0);
	redraw();
}

void clear_resize(void)
{
	resized = false;
}

Bool is_input_ev(Display *dpy, XEvent *ev, XPointer arg)
{
	return ev->type == ButtonPress || ev->type == KeyPress;
}

void run_key_handler(const char *key, unsigned int mask)
{
	pid_t pid;
	FILE *pfs;
	bool marked = mode == MODE_THUMB && markcnt > 0;
	bool changed = false;
	int f, i, pfd[2];
	int fcnt = marked ? markcnt : 1;
	char kstr[32];
	struct stat *oldst, st;
	XEvent dump;

	if (keyhandler.f.err != 0) {
		if (!keyhandler.warned) {
			error(0, keyhandler.f.err, "%s", keyhandler.f.cmd);
			keyhandler.warned = true;
		}
		return;
	}
	if (key == NULL)
		return;

	if (pipe(pfd) < 0) {
		error(0, errno, "pipe");
		return;
	}
	if ((pfs = fdopen(pfd[1], "w")) == NULL) {
		error(0, errno, "open pipe");
		close(pfd[0]), close(pfd[1]);
		return;
	}
	oldst = emalloc(fcnt * sizeof(*oldst));

	close_info();
	strncpy(win.bar.l.buf, "Running key handler...", win.bar.l.size);
	win_draw(&win);
	win_set_cursor(&win, CURSOR_WATCH);

	snprintf(kstr, sizeof(kstr), "%s%s%s%s",
	         mask & ControlMask ? "C-" : "",
	         mask & Mod1Mask    ? "M-" : "",
	         mask & ShiftMask   ? "S-" : "", key);

	if ((pid = fork()) == 0) {
		close(pfd[1]);
		dup2(pfd[0], 0);
		execl(keyhandler.f.cmd, keyhandler.f.cmd, kstr, NULL);
		error(EXIT_FAILURE, errno, "exec: %s", keyhandler.f.cmd);
	}
	close(pfd[0]);
	if (pid < 0) {
		error(0, errno, "fork");
		fclose(pfs);
		goto end;
	}

	for (f = i = 0; f < fcnt; i++) {
		if ((marked && (files[i].flags & FF_MARK)) || (!marked && i == fileidx)) {
			stat(files[i].path, &oldst[f]);
			fprintf(pfs, "%s\n", files[i].name);
			f++;
		}
	}
	fclose(pfs);
	while (waitpid(pid, NULL, 0) == -1 && errno == EINTR);

	for (f = i = 0; f < fcnt; i++) {
		if ((marked && (files[i].flags & FF_MARK)) || (!marked && i == fileidx)) {
			if (stat(files[i].path, &st) != 0 ||
				  memcmp(&oldst[f].st_mtime, &st.st_mtime, sizeof(st.st_mtime)) != 0)
			{
				if (tns.thumbs != NULL) {
					tns_unload(&tns, i);
					tns.loadnext = MIN(tns.loadnext, i);
				}
				changed = true;
			}
			f++;
		}
	}
	/* drop user input events that occurred while running the key handler */
	while (XCheckIfEvent(win.env.dpy, &dump, is_input_ev, NULL));

end:
	if (mode == MODE_IMAGE) {
		if (changed) {
			img_close(&img, true);
			load_image(fileidx);
		} else {
			open_info();
		}
	}
	free(oldst);
	reset_cursor();
	redraw();
}

#define MODMASK(mask) ((mask) & (ShiftMask|ControlMask|Mod1Mask))

void on_keypress(XKeyEvent *kev)
{
	int i;
	unsigned int sh = 0;
	KeySym ksym, shksym;
	char dummy, key;
	bool dirty = false;

	XLookupString(kev, &key, 1, &ksym, NULL);

	if (kev->state & ShiftMask) {
		kev->state &= ~ShiftMask;
		XLookupString(kev, &dummy, 1, &shksym, NULL);
		kev->state |= ShiftMask;
		if (ksym != shksym)
			sh = ShiftMask;
	}
	if (IsModifierKey(ksym))
		return;
	if (ksym == XK_Escape && MODMASK(kev->state) == 0) {
		extprefix = False;
	} else if (extprefix) {
		run_key_handler(XKeysymToString(ksym), kev->state & ~sh);
		extprefix = False;
	} else if (key >= '0' && key <= '9') {
		/* number prefix for commands */
		prefix = prefix * 10 + (int) (key - '0');
		return;
	} else for (i = 0; i < ARRLEN(keys); i++) {
		if (keys[i].ksym == ksym &&
		    MODMASK(keys[i].mask | sh) == MODMASK(kev->state) &&
		    keys[i].cmd >= 0 && keys[i].cmd < CMD_COUNT &&
		    (cmds[keys[i].cmd].mode < 0 || cmds[keys[i].cmd].mode == mode))
		{
			if (cmds[keys[i].cmd].func(keys[i].arg))
				dirty = true;
		}
	}
	if (dirty)
		redraw();
	prefix = 0;
}

void on_buttonpress(XButtonEvent *bev)
{
	int i, sel;
	bool dirty = false;
	static Time firstclick;

	if (mode == MODE_IMAGE) {
		set_timeout(reset_cursor, TO_CURSOR_HIDE, true);
		reset_cursor();
		if (bev->button == Button1) {  // zrobilem double click on image daje thumbsy
			if (bev->time - firstclick <= TO_DOUBLE_CLICK) {
				tns_init(&tns, files, &filecnt, &fileidx, &win);
				img_close(&img, false);
				tns.dirty = true;
				mode = MODE_THUMB;
			} else {
				firstclick = bev->time;
			}
		}

		for (i = 0; i < ARRLEN(buttons); i++) {
			if (buttons[i].button == bev->button &&
			    MODMASK(buttons[i].mask) == MODMASK(bev->state) &&
			    buttons[i].cmd >= 0 && buttons[i].cmd < CMD_COUNT &&
			    (cmds[buttons[i].cmd].mode < 0 || cmds[buttons[i].cmd].mode == mode))
			{
                const struct timespec ten_ms = {0, 100000000};
				nanosleep(&ten_ms, NULL);
                // I added delay here becose xmenu do not show up
				if (cmds[buttons[i].cmd].func(buttons[i].arg))
					dirty = true;
			}
		}
		if (dirty)
			redraw();
	} else {
		/* thumbnail mode (hard-coded) */
		switch (bev->button) {
			case Button1:
				if ((sel = tns_translate(&tns, bev->x, bev->y)) >= 0) {
					if (sel != fileidx) {
						tns_highlight(&tns, fileidx, false);
						tns_highlight(&tns, sel, true);
						fileidx = sel;
						firstclick = bev->time;
						redraw();
					} else if (bev->time - firstclick <= TO_DOUBLE_CLICK) {
						mode = MODE_IMAGE;
						set_timeout(reset_cursor, TO_CURSOR_HIDE, true);
						load_image(fileidx);
						redraw();
					} else {
						firstclick = bev->time;
					}
				}
				break;
			case Button2:
				if ((sel = tns_translate(&tns, bev->x, bev->y)) >= 0) {
					bool on = !(files[sel].flags & FF_MARK);
					XEvent e;

					for (;;) {
						if (sel >= 0 && mark_image(sel, on))
							redraw();
						XMaskEvent(win.env.dpy,
						           ButtonPressMask | ButtonReleaseMask | PointerMotionMask, &e);
						if (e.type == ButtonPress || e.type == ButtonRelease)
							break;
						while (XCheckTypedEvent(win.env.dpy, MotionNotify, &e));
						sel = tns_translate(&tns, e.xbutton.x, e.xbutton.y);
					}
				}
				break;
			case Button4:
			//case Button3:
              //  fprintf(stderr,"Button 3 pressed\n"); 
                //char *menu = "/root/.config/sxiv/exec/context_menu.sh";
                //char *menu = "/root/.config/sxiv/exec/full-info.sh";
                //const struct timespec ten_ms = {0, 100000000};
				//nanosleep(&ten_ms, NULL);
                //run_ext_command(menu);

                //break;
			case Button5:
				if (tns_scroll(&tns, bev->button == Button4 ? DIR_UP : DIR_DOWN,
				               (bev->state & ControlMask) != 0))
					redraw();
				break;
		}
	}
	prefix = 0;
}

const struct timespec ten_ms = {0, 10000000};

void run(void)
{
	int xfd;
	fd_set fds;
	struct timeval timeout;
	bool discard, init_thumb, load_thumb, to_set;
	XEvent ev, nextev;

	while (true) {
		to_set = check_timeouts(&timeout);
		init_thumb = mode == MODE_THUMB && tns.initnext < filecnt;
		load_thumb = mode == MODE_THUMB && tns.loadnext < tns.end;

		if ((init_thumb || load_thumb || to_set || info.fd != -1 ||
			   arl.fd != -1) && XPending(win.env.dpy) == 0)
		{
			if (load_thumb) {
				set_timeout(redraw, TO_REDRAW_THUMBS, false);
				if (!tns_load(&tns, tns.loadnext, false, false)) {
					remove_file(tns.loadnext, false);
					tns.dirty = true;
				}
				if (tns.loadnext >= tns.end)
					redraw();
			} else if (init_thumb) {
				set_timeout(redraw, TO_REDRAW_THUMBS, false);
				if (!tns_load(&tns, tns.initnext, false, true))
					remove_file(tns.initnext, false);
			} else {
				xfd = ConnectionNumber(win.env.dpy);
				FD_ZERO(&fds);
				FD_SET(xfd, &fds);
				if (info.fd != -1) {
					FD_SET(info.fd, &fds);
					xfd = MAX(xfd, info.fd);
				}
				if (arl.fd != -1) {
					FD_SET(arl.fd, &fds);
					xfd = MAX(xfd, arl.fd);
				}
				select(xfd + 1, &fds, 0, 0, to_set ? &timeout : NULL);
				if (info.fd != -1 && FD_ISSET(info.fd, &fds))
					read_info();
				if (arl.fd != -1 && FD_ISSET(arl.fd, &fds)) {
					if (arl_handle(&arl)) {
						/* when too fast, imlib2 can't load the image */
						nanosleep(&ten_ms, NULL);
						img_close(&img, true);
						load_image(fileidx);
						redraw();
					}
				}
			}
			continue;
		}

		do {
			XNextEvent(win.env.dpy, &ev);
			discard = false;
			if (XEventsQueued(win.env.dpy, QueuedAlready) > 0) {
				XPeekEvent(win.env.dpy, &nextev);
				switch (ev.type) {
					case ConfigureNotify:
					case MotionNotify:
						discard = ev.type == nextev.type;
						break;
					case KeyPress:
						discard = (nextev.type == KeyPress || nextev.type == KeyRelease)
						          && ev.xkey.keycode == nextev.xkey.keycode;
						break;
				}
			}
		} while (discard);

		switch (ev.type) {
			/* handle events */
			case ButtonPress:
				on_buttonpress(&ev.xbutton);
				break;
			case ClientMessage:
				if ((Atom) ev.xclient.data.l[0] == atoms[ATOM_WM_DELETE_WINDOW])
					cmds[g_quit].func(0);
				break;
			case ConfigureNotify:
				if (win_configure(&win, &ev.xconfigure)) {
					if (mode == MODE_IMAGE) {
						img.dirty = true;
						img.checkpan = true;
					} else {
						tns.dirty = true;
					}
					if (!resized) {
						redraw();
						set_timeout(clear_resize, TO_REDRAW_RESIZE, false);
						resized = true;
					} else {
						set_timeout(redraw, TO_REDRAW_RESIZE, false);
					}
				}
				break;
			case KeyPress:
				on_keypress(&ev.xkey);
				break;
			case MotionNotify:
				if (mode == MODE_IMAGE) {
					set_timeout(reset_cursor, TO_CURSOR_HIDE, true);
					reset_cursor();
				}
				break;
		}
	}
}

int fncmp(const void *a, const void *b)
{
	return strcoll(((fileinfo_t*) a)->name, ((fileinfo_t*) b)->name);
}

void sigchld(int sig)
{
	while (waitpid(-1, NULL, WNOHANG) > 0);
}

void setup_signal(int sig, void (*handler)(int sig))
{
	struct sigaction sa;

	sa.sa_handler = handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
	if (sigaction(sig, &sa, 0) == -1)
		error(EXIT_FAILURE, errno, "signal %d", sig);
}

int main(int argc, char **argv)
{
	int i, start;
	size_t n;
	ssize_t len;
	char *filename;
	const char *homedir, *dsuffix = "";
	struct stat fstats;
	r_dir_t dir;

	setup_signal(SIGCHLD, sigchld);
	setup_signal(SIGPIPE, SIG_IGN);

	setlocale(LC_COLLATE, "");

	parse_options(argc, argv);

	if (options->clean_cache) {
		tns_init(&tns, NULL, NULL, NULL, NULL);
		tns_clean_cache(&tns);
		exit(EXIT_SUCCESS);
	}

	if (options->filecnt == 0 && !options->from_stdin) {
		print_usage();
		exit(EXIT_FAILURE);
	}

	if (options->recursive || options->from_stdin)
		filecnt = 1024;
	else
		filecnt = options->filecnt;

	files = emalloc(filecnt * sizeof(*files));
	memset(files, 0, filecnt * sizeof(*files));
	fileidx = 0;

	if (options->from_stdin) {
		n = 0;
		filename = NULL;
		while ((len = getline(&filename, &n, stdin)) > 0) {
			if (filename[len-1] == '\n')
				filename[len-1] = '\0';
			check_add_file(filename, true);
		}
		free(filename);
	}

	for (i = 0; i < options->filecnt; i++) {
		filename = options->filenames[i];

		if (stat(filename, &fstats) < 0) {
			error(0, errno, "%s", filename);
			continue;
		}
		if (!S_ISDIR(fstats.st_mode)) {
			check_add_file(filename, true);
		} else {
			if (r_opendir(&dir, filename, options->recursive) < 0) {
				error(0, errno, "%s", filename);
				continue;
			}
			start = fileidx;
			while ((filename = r_readdir(&dir, true)) != NULL) {
				check_add_file(filename, false);
				free((void*) filename);
			}
			r_closedir(&dir);
			if (fileidx - start > 1)
				qsort(files + start, fileidx - start, sizeof(fileinfo_t), fncmp);
		}
	}

	if (fileidx == 0)
		error(EXIT_FAILURE, 0, "No valid image file given, aborting");

	filecnt = fileidx;
	fileidx = options->startnum < filecnt ? options->startnum : 0;

	for (i = 0; i < ARRLEN(buttons); i++) {
		if (buttons[i].cmd == i_cursor_navigate) {
			imgcursor[0] = CURSOR_LEFT;
			imgcursor[2] = CURSOR_RIGHT;
			break;
		}
	}

	win_init(&win);
	img_init(&img, &win);
	arl_init(&arl);

	if ((homedir = getenv("XDG_CONFIG_HOME")) == NULL || homedir[0] == '\0') {
		homedir = getenv("HOME");
		dsuffix = "/.config";
	}
	if (homedir != NULL) {
		extcmd_t *cmd[] = { &info.f, &keyhandler.f };
		const char *name[] = { "image-info", "key-handler" };

		for (i = 0; i < ARRLEN(cmd); i++) {
			n = strlen(homedir) + strlen(dsuffix) + strlen(name[i]) + 12;
			cmd[i]->cmd = (char*) emalloc(n);
			snprintf(cmd[i]->cmd, n, "%s%s/sxiv/exec/%s", homedir, dsuffix, name[i]);
			if (access(cmd[i]->cmd, X_OK) != 0)
				cmd[i]->err = errno;
		}
	} else {
		error(0, 0, "Exec directory not found");
	}
	info.fd = -1;

	if (options->thumb_mode) {
		mode = MODE_THUMB;
		tns_init(&tns, files, &filecnt, &fileidx, &win);
		while (!tns_load(&tns, fileidx, false, false))
			remove_file(fileidx, false);
	} else {
		mode = MODE_IMAGE;
		tns.thumbs = NULL;
		load_image(fileidx);
        //read_tags();
	}
	win_open(&win);
	win_set_cursor(&win, CURSOR_WATCH);

	atexit(cleanup);

	set_timeout(redraw, 25, false);

	run();

	return 0;
}
