/* -- user.c -- */

#include "x11vnc.h"
#include "solid.h"
#include "cleanup.h"
#include "scan.h"
#include "screen.h"
#include "unixpw.h"
#include "sslhelper.h"
#include "xwrappers.h"
#include "connections.h"
#include "inet.h"
#include "keyboard.h"
#include "cursor.h"
#include "remote.h"
#include "avahi.h"

void check_switched_user(void);
void lurk_loop(char *str);
int switch_user(char *user, int fb_mode);
int read_passwds(char *passfile);
void install_passwds(void);
void check_new_passwds(int force);
int wait_for_client(int *argc, char** argv, int http);
rfbBool custom_passwd_check(rfbClientPtr cl, const char *response, int len);
char *xdmcp_insert = NULL;

static void switch_user_task_dummy(void);
static void switch_user_task_solid_bg(void);
static char *get_login_list(int with_display);
static char **user_list(char *user_str);
static void user2uid(char *user, uid_t *uid, gid_t *gid, char **name, char **home);
static int lurk(char **users);
static int guess_user_and_switch(char *str, int fb_mode);
static int try_user_and_display(uid_t uid, gid_t gid, char *dpystr);
static int switch_user_env(uid_t uid, gid_t gid, char *name, char *home, int fb_mode);
static void try_to_switch_users(void);


/* tasks for after we switch */
static void switch_user_task_dummy(void) {
	;	/* dummy does nothing */
}
static void switch_user_task_solid_bg(void) {
	/* we have switched users, some things to do. */
	if (use_solid_bg && client_count) {
		solid_bg(0);
	}
}

void check_switched_user(void) {
	static time_t sched_switched_user = 0;
	static int did_solid = 0;
	static int did_dummy = 0;
	int delay = 15;
	time_t now = time(NULL);

	if (unixpw_in_progress) return;

	if (started_as_root == 1 && users_list) {
		try_to_switch_users();
		if (started_as_root == 2) {
			/*
			 * schedule the switch_user_tasks() call
			 * 15 secs is for piggy desktops to start up.
			 * might not be enough for slow machines...
			 */
			sched_switched_user = now;
			did_dummy = 0;
			did_solid = 0;
			/* add other activities */
		}
	}
	if (! sched_switched_user) {
		return;
	}

	if (! did_dummy) {
		switch_user_task_dummy();
		did_dummy = 1;
	}
	if (! did_solid) {
		int doit = 0;
		char *ss = solid_str;
		if (now >= sched_switched_user + delay) {
			doit = 1;
		} else if (ss && strstr(ss, "root:") == ss) {
		    	if (now >= sched_switched_user + 3) {
				doit = 1;
			}
		} else if (strcmp("root", guess_desktop())) {
			usleep(1000 * 1000);
			doit = 1;
		}
		if (doit) {
			switch_user_task_solid_bg();
			did_solid = 1;
		}
	}

	if (did_dummy && did_solid) {
		sched_switched_user = 0;
	}
}

/* utilities for switching users */
static char *get_login_list(int with_display) {
	char *out;
#if LIBVNCSERVER_HAVE_UTMPX_H
	int i, cnt, max = 200, ut_namesize = 32;
	int dpymax = 1000, sawdpy[1000];
	struct utmpx *utx;

	/* size based on "username:999," * max */
	out = (char *) malloc(max * (ut_namesize+1+3+1) + 1);
	out[0] = '\0';

	for (i=0; i<dpymax; i++) {
		sawdpy[i] = 0;
	}

	setutxent();
	cnt = 0;
	while (1) {
		char *user, *line, *host, *id;
		char tmp[10];
		int d = -1;
		utx = getutxent();
		if (! utx) {
			break;
		}
		if (utx->ut_type != USER_PROCESS) {
			continue;
		}
		user = lblanks(utx->ut_user);
		if (*user == '\0') {
			continue;
		}
		if (strchr(user, ',')) {
			continue;	/* unlikely, but comma is our sep. */
		}

		line = lblanks(utx->ut_line);
		host = lblanks(utx->ut_host);
		id   = lblanks(utx->ut_id);

		if (with_display) {
			if (0 && line[0] != ':' && strcmp(line, "dtlocal")) {
				/* XXX useful? */
				continue;
			}

			if (line[0] == ':') {
				if (sscanf(line, ":%d", &d) != 1)  {
					d = -1;
				}
			}
			if (d < 0 && host[0] == ':') {
				if (sscanf(host, ":%d", &d) != 1)  {
					d = -1;
				}
			}
			if (d < 0 && id[0] == ':') {
				if (sscanf(id, ":%d", &d) != 1)  {
					d = -1;
				}
			}

			if (d < 0 || d >= dpymax || sawdpy[d]) {
				continue;
			}
			sawdpy[d] = 1;
			sprintf(tmp, ":%d", d);
		} else {
			/* try to eliminate repeats */
			int repeat = 0;
			char *q;

			q = out;
			while ((q = strstr(q, user)) != NULL) {
				char *p = q + strlen(user) + strlen(":DPY");
				if (q == out || *(q-1) == ',') {
					/* bounded on left. */
					if (*p == ',' || *p == '\0') {
						/* bounded on right. */
						repeat = 1;
						break;
					}
				}
				q = p;
			}
			if (repeat) {
				continue;
			}
			sprintf(tmp, ":DPY");
		}

		if (*out) {
			strcat(out, ",");
		}
		strcat(out, user);
		strcat(out, tmp);

		cnt++;
		if (cnt >= max) {
			break;
		}
	}
	endutxent();
#else
	out = strdup("");
#endif
	return out;
}

static char **user_list(char *user_str) {
	int n, i;
	char *p, **list;
	
	p = user_str;
	n = 1;
	while (*p++) {
		if (*p == ',') {
			n++;
		}
	}
	list = (char **) malloc((n+1)*sizeof(char *));

	p = strtok(user_str, ",");
	i = 0;
	while (p) {
		list[i++] = p;
		p = strtok(NULL, ",");
	}
	list[i] = NULL;
	return list;
}

static void user2uid(char *user, uid_t *uid, gid_t *gid, char **name, char **home) {
	int numerical = 1, gotgroup = 0;
	char *q;

	*uid = (uid_t) -1;
	*name = NULL;
	*home = NULL;

	q = user;
	while (*q) {
		if (! isdigit((unsigned char) (*q++))) {
			numerical = 0;
			break;
		}
	}

	if (user2group != NULL) {
		static int *did = NULL;
		int i;

		if (did == NULL) {
			int n = 0;
			i = 0;
			while (user2group[i] != NULL) {
				n++;
				i++;
			}
			did = (int *) malloc((n+1) * sizeof(int)); 
			i = 0;
			for (i=0; i<n; i++) {
				did[i] = 0;
			}
		}
		i = 0;
		while (user2group[i] != NULL) {
			if (strstr(user2group[i], user) == user2group[i]) {
				char *w = user2group[i] + strlen(user);
				if (*w == '.') {
					struct group* gr = getgrnam(++w);
					if (! gr) {
						rfbLog("Invalid group: %s\n", w);
						clean_up_exit(1);
					}
					*gid = gr->gr_gid;
					if (! did[i]) {
						rfbLog("user2uid: using group %s (%d) for %s\n",
						    w, (int) *gid, user);
						did[i] = 1;
					}
					gotgroup = 1;
				}
			}
			i++;
		}
	}

	if (numerical) {
		int u = atoi(user);

		if (u < 0) {
			return;
		}
		*uid = (uid_t) u;
	}

#if LIBVNCSERVER_HAVE_PWD_H
	if (1) {
		struct passwd *pw;
		if (numerical) {
			pw = getpwuid(*uid);
		} else {
			pw = getpwnam(user);
		}
		if (pw) {
			*uid  = pw->pw_uid;
			if (! gotgroup) {
				*gid  = pw->pw_gid;
			}
			*name = pw->pw_name;	/* n.b. use immediately */
			*home = pw->pw_dir;
		}
	}
#endif
}


static int lurk(char **users) {
	uid_t uid;
	gid_t gid;
	int success = 0, dmin = -1, dmax = -1;
	char *p, *logins, **u;

	if ((u = users) != NULL && *u != NULL && *(*u) == ':') {
		int len;
		char *tmp;

		/* extract min and max display numbers */
		tmp = *u;
		if (strchr(tmp, '-')) {
			if (sscanf(tmp, ":%d-%d", &dmin, &dmax) != 2) {
				dmin = -1;
				dmax = -1;
			}
		}
		if (dmin < 0) {
			if (sscanf(tmp, ":%d", &dmin) != 1) {
				dmin = -1;
				dmax = -1;
			} else {
				dmax = dmin;
			}
		}
		if ((dmin < 0 || dmax < 0) || dmin > dmax || dmax > 10000) {
			dmin = -1;
			dmax = -1;
		}

		/* get user logins regardless of having a display: */
		logins = get_login_list(0);

		/*
		 * now we append the list in users (might as well try
		 * them) this will probably allow weird ways of starting
		 * xservers to work.
		 */
		len = strlen(logins);
		u++;
		while (*u != NULL) {
			len += strlen(*u) + strlen(":DPY,");
			u++;
		}
		tmp = (char *) malloc(len+1);
		strcpy(tmp, logins);

		/* now concatenate them: */
		u = users+1;
		while (*u != NULL) {
			char *q, chk[100];
			snprintf(chk, 100, "%s:DPY", *u);
			q = strstr(tmp, chk);
			if (q) {
				char *p = q + strlen(chk);
				
				if (q == tmp || *(q-1) == ',') {
					/* bounded on left. */
					if (*p == ',' || *p == '\0') {
						/* bounded on right. */
						u++;
						continue;
					}
				}
			}
			
			if (*tmp) {
				strcat(tmp, ",");
			}
			strcat(tmp, *u);
			strcat(tmp, ":DPY");
			u++;
		}
		free(logins);
		logins = tmp;
		
	} else {
		logins = get_login_list(1);
	}
	
	p = strtok(logins, ",");
	while (p) {
		char *user, *name, *home, dpystr[10];
		char *q, *t;
		int ok = 1, dn;
		
		t = strdup(p);	/* bob:0 */
		q = strchr(t, ':'); 
		if (! q) {
			free(t);
			break;
		}
		*q = '\0';
		user = t;
		snprintf(dpystr, 10, ":%s", q+1);

		if (users) {
			u = users;
			ok = 0;
			while (*u != NULL) {
				if (*(*u) == ':') {
					u++;
					continue;
				}
				if (!strcmp(user, *u++)) {
					ok = 1;
					break;
				}
			}
		}

		user2uid(user, &uid, &gid, &name, &home);
		free(t);

		if (! uid || ! gid) {
			ok = 0;
		}

		if (! ok) {
			p = strtok(NULL, ",");
			continue;
		}
		
		for (dn = dmin; dn <= dmax; dn++) {
			if (dn >= 0) {
				sprintf(dpystr, ":%d", dn);
			}
			if (try_user_and_display(uid, gid, dpystr)) {
				if (switch_user_env(uid, gid, name, home, 0)) {
					rfbLog("lurk: now user: %s @ %s\n",
					    name, dpystr);
					started_as_root = 2;
					success = 1;
				}
				set_env("DISPLAY", dpystr);
				break;
			}
		}
		if (success) {
			 break;
		}

		p = strtok(NULL, ",");
	}
	free(logins);
	return success;
}

void lurk_loop(char *str) {
	char *tstr = NULL, **users = NULL;

	if (strstr(str, "lurk=") != str) {
		exit(1);
	}
	rfbLog("lurking for logins using: '%s'\n", str);
	if (strlen(str) > strlen("lurk=")) {
		char *q = strchr(str, '=');
		tstr = strdup(q+1);
		users = user_list(tstr);
	}

	while (1) {
		if (lurk(users)) {
			break;
		}
		sleep(3);
	}
	if (tstr) {
		free(tstr);
	}
	if (users) {
		free(users);
	}
}

static int guess_user_and_switch(char *str, int fb_mode) {
	char *dstr, *d;
	char *p, *tstr = NULL, *allowed = NULL, *logins, **users = NULL;
	int dpy1, ret = 0;

	RAWFB_RET(0)

	d = DisplayString(dpy);
	/* pick out ":N" */
	dstr = strchr(d, ':');
	if (! dstr) {
		return 0;
	}
	if (sscanf(dstr, ":%d", &dpy1) != 1) {
		return 0;
	}
	if (dpy1 < 0) {
		return 0;
	}

	if (strstr(str, "guess=") == str && strlen(str) > strlen("guess=")) {
		allowed = strchr(str, '=');
		allowed++;

		tstr = strdup(allowed);
		users = user_list(tstr);
	}

	/* loop over the utmpx entries looking for this display */
	logins = get_login_list(1);
	p = strtok(logins, ",");
	while (p) {
		char *user, *q, *t;
		int dpy2, ok = 1;

		t = strdup(p);
		q = strchr(t, ':'); 
		if (! q) {
			free(t);
			break;
		}
		*q = '\0';
		user = t;
		dpy2 = atoi(q+1);

		if (users) {
			char **u = users;
			ok = 0;
			while (*u != NULL) {
				if (!strcmp(user, *u++)) {
					ok = 1;
					break;
				}
			}
		}
		if (dpy1 != dpy2) {
			ok = 0;
		}

		if (! ok) {
			free(t);
			p = strtok(NULL, ",");
			continue;
		}
		if (switch_user(user, fb_mode)) {
			rfbLog("switched to guessed user: %s\n", user);
			free(t);
			ret = 1;
			break;
		}

		p = strtok(NULL, ",");
	}
	if (tstr) {
		free(tstr);
	}
	if (users) {
		free(users);
	}
	if (logins) {
		free(logins);
	}
	return ret;
}

static int try_user_and_display(uid_t uid, gid_t gid, char *dpystr) {
	/* NO strtoks */
#if LIBVNCSERVER_HAVE_FORK && LIBVNCSERVER_HAVE_SYS_WAIT_H && LIBVNCSERVER_HAVE_PWD_H
	pid_t pid, pidw;
	char *home, *name;
	int st;
	struct passwd *pw;
	
	pw = getpwuid(uid);
	if (pw) {
		name = pw->pw_name;
		home = pw->pw_dir;
	} else {
		return 0;
	}

	/* 
	 * We fork here and try to open the display again as the
	 * new user.  Unreadable XAUTHORITY could be a problem...
	 * This is not really needed since we have DISPLAY open but:
	 * 1) is a good indicator this user owns the session and  2)
	 * some activities do spawn new X apps, e.g.  xmessage(1), etc.
	 */
	if ((pid = fork()) > 0) {
		;
	} else if (pid == -1) {
		fprintf(stderr, "could not fork\n");
		rfbLogPerror("fork");
		return 0;
	} else {
		/* child */
		Display *dpy2 = NULL;
		int rc;

		signal(SIGHUP,  SIG_DFL);
		signal(SIGINT,  SIG_DFL);
		signal(SIGQUIT, SIG_DFL);
		signal(SIGTERM, SIG_DFL);

		rc = switch_user_env(uid, gid, name, home, 0); 
		if (! rc) {
			exit(1);
		}

		fclose(stderr);
		dpy2 = XOpenDisplay_wr(dpystr);
		if (dpy2) {
			XCloseDisplay_wr(dpy2);
			exit(0);	/* success */
		} else {
			exit(2);	/* fail */
		}
	}

	/* see what the child says: */
	pidw = waitpid(pid, &st, 0);
	if (pidw == pid && WIFEXITED(st) && WEXITSTATUS(st) == 0) {
		return 1;
	}
#endif	/* LIBVNCSERVER_HAVE_FORK ... */
	return 0;
}

int switch_user(char *user, int fb_mode) {
	/* NO strtoks */
	int doit = 0;
	uid_t uid = 0;
	gid_t gid = 0;
	char *name, *home;

	if (*user == '+') {
		doit = 1;
		user++;
	}

	if (strstr(user, "guess=") == user) {
		return guess_user_and_switch(user, fb_mode);
	}

	user2uid(user, &uid, &gid, &name, &home);

	if (uid == (uid_t) -1 || uid == 0) {
		return 0;
	}
	if (gid == 0) {
		return 0;
	}

	if (! doit && dpy) {
		/* see if this display works: */
		char *dstr = DisplayString(dpy);
		doit = try_user_and_display(uid, gid, dstr);
	}

	if (doit) {
		int rc = switch_user_env(uid, gid, name, home, fb_mode);
		if (rc) {
			started_as_root = 2;
		}
		return rc;
	} else {
		return 0;
	}
}

static int switch_user_env(uid_t uid, gid_t gid, char *name, char *home, int fb_mode) {
	/* NO strtoks */
	char *xauth;
	int reset_fb = 0;
	int grp_ok = 0;

#if !LIBVNCSERVER_HAVE_SETUID
	return 0;
#else
	/*
	 * OK tricky here, we need to free the shm... otherwise
	 * we won't be able to delete it as the other user...
	 */
	if (fb_mode == 1 && (using_shm && ! xform24to32)) {
		reset_fb = 1;
		clean_shm(0);
		free_tiles();
	}
#if LIBVNCSERVER_HAVE_INITGROUPS
#if LIBVNCSERVER_HAVE_PWD_H
	if (getpwuid(uid) != NULL && getenv("X11VNC_SINGLE_GROUP") == NULL) {
		struct passwd *p = getpwuid(uid);
		if (initgroups(p->pw_name, gid) == 0)  {
			grp_ok = 1;
		} else {
			rfbLogPerror("initgroups");
		}
	}
#endif
#endif
	if (! grp_ok) {
		if (setgid(gid) == 0) {
			grp_ok = 1;
		}
	}
	if (! grp_ok) {
		if (reset_fb) {
			/* 2 means we did clean_shm and free_tiles */
			do_new_fb(2);
		}
		return 0;
	}

	if (setuid(uid) != 0) {
		if (reset_fb) {
			/* 2 means we did clean_shm and free_tiles */
			do_new_fb(2);
		}
		return 0;
	}
#endif
	if (reset_fb) {
		do_new_fb(2);
	}

	xauth = getenv("XAUTHORITY");
	if (xauth && access(xauth, R_OK) != 0) {
		*(xauth-2) = '_';	/* yow */
	}
	
	set_env("USER", name);
	set_env("LOGNAME", name);
	set_env("HOME", home);
	return 1;
}

static void try_to_switch_users(void) {
	static time_t last_try = 0;
	time_t now = time(NULL);
	char *users, *p;

	if (getuid() && geteuid()) {
		rfbLog("try_to_switch_users: not root\n");
		started_as_root = 2;
		return;
	}
	if (!last_try) {
		last_try = now;
	} else if (now <= last_try + 2) {
		/* try every 3 secs or so */
		return;
	}
	last_try = now;

	users = strdup(users_list);

	if (strstr(users, "guess=") == users) {
		if (switch_user(users, 1)) {
			started_as_root = 2;
		}
		free(users);
		return;
	}

	p = strtok(users, ",");
	while (p) {
		if (switch_user(p, 1)) {
			started_as_root = 2;
			rfbLog("try_to_switch_users: now %s\n", p);
			break;
		}
		p = strtok(NULL, ",");
	}
	free(users);
}

int read_passwds(char *passfile) {
	char line[1024];
	char *filename;
	char **old_passwd_list = passwd_list;
	int linecount = 0, i, remove = 0, read_mode = 0, begin_vo = -1;
	struct stat sbuf;
	static int max = -1;
	FILE *in = NULL;
	static time_t last_read = 0;
	static int read_cnt = 0;
	int db_passwd = 0;

	if (max < 0) {
		max = 1000;
		if (getenv("X11VNC_MAX_PASSWDS")) {
			max = atoi(getenv("X11VNC_MAX_PASSWDS"));
		}
	}

	filename = passfile;
	if (strstr(filename, "rm:") == filename) {
		filename += strlen("rm:");
		remove = 1;
	} else if (strstr(filename, "read:") == filename) {
		filename += strlen("read:");
		read_mode = 1;
		if (stat(filename, &sbuf) == 0) {
			if (sbuf.st_mtime <= last_read) {
				return 1;
			}
			last_read = sbuf.st_mtime;
		}
	} else if (strstr(filename, "cmd:") == filename) {
		int rc;

		filename += strlen("cmd:");
		read_mode = 1;
		in = tmpfile();
		if (in == NULL) {
			rfbLog("run_user_command tmpfile() failed: %s\n",
			    filename);
			clean_up_exit(1);
		}
		rc = run_user_command(filename, latest_client, "read_passwds",
		    NULL, 0, in);
		if (rc != 0) {
			rfbLog("run_user_command command failed: %s\n",
			    filename);
			clean_up_exit(1);
		}
		rewind(in);
	} else if (strstr(filename, "custom:") == filename) {
		return 1;
	}

	if (in == NULL && stat(filename, &sbuf) == 0) {
		/* (poor...) upper bound to number of lines */
		max = (int) sbuf.st_size;
		last_read = sbuf.st_mtime;
	}

	/* create 1 more than max to have it be the ending NULL */
	passwd_list = (char **) malloc( (max+1) * (sizeof(char *)) );
	for (i=0; i<max+1; i++) {
		passwd_list[i] = NULL;
	}
	
	if (in == NULL) {
		in = fopen(filename, "r");
	}
	if (in == NULL) {
		rfbLog("cannot open passwdfile: %s\n", passfile);
		rfbLogPerror("fopen");
		if (remove) {
			unlink(filename);
		}
		clean_up_exit(1);
	}

	if (getenv("DEBUG_PASSWDFILE") != NULL) {
		db_passwd = 1;
	}

	while (fgets(line, 1024, in) != NULL) {
		char *p;
		int blank = 1;
		int len = strlen(line); 

		if (db_passwd) {
			fprintf(stderr, "read_passwds: raw line: %s\n", line);
		}

		if (len == 0) {
			continue;
		} else if (line[len-1] == '\n') {
			line[len-1] = '\0';
		}
		if (line[0] == '\0') {
			continue;
		}
		if (strstr(line, "__SKIP__") != NULL) {
			continue;
		}
		if (strstr(line, "__COMM__") == line) {
			continue;
		}
		if (!strcmp(line, "__BEGIN_VIEWONLY__")) {
			if (begin_vo < 0) {
				begin_vo = linecount;
			}
			continue;
		}
		if (line[0] == '#') {
			/* commented out, cannot have password beginning with # */
			continue;
		}
		p = line;
		while (*p != '\0') {
			if (! isspace((unsigned char) (*p))) {
				blank = 0;
				break;
			}
			p++;
		}
		if (blank) {
			continue;
		}

		passwd_list[linecount++] = strdup(line);
		if (db_passwd) {
			fprintf(stderr, "read_passwds: keepline: %s\n", line);
			fprintf(stderr, "read_passwds: begin_vo: %d\n", begin_vo);
		}

		if (linecount >= max) {
			rfbLog("read_passwds: hit max passwd: %d\n", max);
			break;
		}
	}
	fclose(in);

	for (i=0; i<1024; i++) {
		line[i] = '\0';
	}

	if (remove) {
		unlink(filename);
	}

	if (! linecount) {
		rfbLog("cannot read a valid line from passwdfile: %s\n",
		    passfile);
		if (read_cnt == 0) {
			clean_up_exit(1);
		} else {
			return 0;
		}
	}
	read_cnt++;

	for (i=0; i<linecount; i++) {
		char *q, *p = passwd_list[i];
		if (!strcmp(p, "__EMPTY__")) {
			*p = '\0';
		} else if ((q = strstr(p, "__COMM__")) != NULL) {
			*q = '\0';
		}
		passwd_list[i] = strdup(p);
		if (db_passwd) {
			fprintf(stderr, "read_passwds: trimline: %s\n", p);
		}
		strzero(p);
	}

	begin_viewonly = begin_vo;
	if (read_mode && read_cnt > 1) {
		if (viewonly_passwd) {
			free(viewonly_passwd);
			viewonly_passwd = NULL;
		}
	}

	if (begin_viewonly < 0 && linecount == 2) {
		/* for compatibility with previous 2-line usage: */
		viewonly_passwd = strdup(passwd_list[1]);
		if (db_passwd) {
			fprintf(stderr, "read_passwds: linecount is 2.\n");
		}
		if (screen) {
			char **apd = (char **) screen->authPasswdData;
			if (apd) {
				if (apd[0] != NULL) {
					strzero(apd[0]);
				}
				apd[0] = strdup(passwd_list[0]);
			}
		}
		begin_viewonly = 1;
	}

	if (old_passwd_list != NULL) {
		char *p;
		i = 0;
		while (old_passwd_list[i] != NULL) {
			p = old_passwd_list[i];
			strzero(p);
			free(old_passwd_list[i]);
			i++;
		}
		free(old_passwd_list);
	}
	return 1;
}

void install_passwds(void) {
	if (viewonly_passwd) {
		/* append the view only passwd after the normal passwd */
		char **passwds_new = (char **) malloc(3*sizeof(char *));
		char **passwds_old = (char **) screen->authPasswdData;
		passwds_new[0] = passwds_old[0];
		passwds_new[1] = viewonly_passwd;
		passwds_new[2] = NULL;
		screen->authPasswdData = (void*) passwds_new;
	} else if (passwd_list) {
		int i = 0;
		while(passwd_list[i] != NULL) {
			i++;
		}
		if (begin_viewonly < 0) {
			begin_viewonly = i+1;
		}
		screen->authPasswdData = (void*) passwd_list;
		screen->authPasswdFirstViewOnly = begin_viewonly;
	}
}

void check_new_passwds(int force) {
	static time_t last_check = 0;
	time_t now;

	if (! passwdfile) {
		return;
	}
	if (strstr(passwdfile, "read:") != passwdfile) {
		return;
	}
	if (unixpw_in_progress) return;

	if (force) {
		last_check = 0;
	}

	now = time(NULL);
	if (now > last_check + 1) {
		if (read_passwds(passwdfile)) {
			install_passwds();
		}
		last_check = now;
	}
}

rfbBool custom_passwd_check(rfbClientPtr cl, const char *response, int len) {
	char *input, *cmd;
	char num[16];
	int j, i, n, rc;

	rfbLog("custom_passwd_check: len=%d\n", len);

	if (!passwdfile || strstr(passwdfile, "custom:") != passwdfile) {
		return FALSE;
	}
	cmd = passwdfile + strlen("custom:");

	sprintf(num, "%d\n", len);

	input = (char *) malloc(2 * len + 16 + 1);
	
	input[0] = '\0';
	strcat(input, num);
	n = strlen(num);

	j = n;
	for (i=0; i < len; i++) {
		input[j] = cl->authChallenge[i];
		j++;
	}
	for (i=0; i < len; i++) {
		input[j] = response[i];
		j++;
	}
	rc = run_user_command(cmd, cl, "custom_passwd", input, n+2*len, NULL);
	free(input);
	if (rc == 0) {
		return TRUE;
	} else {
		return FALSE;
	}
}

static void handle_one_http_request(void) {

	rfbLog("handle_one_http_request: begin.\n");
	if (inetd || screen->httpPort == 0) {
		int port = find_free_port(5800, 5860);
		if (port) {
			screen->httpPort = port;
		} else {
			rfbLog("handle_one_http_request: no http port.\n");
			clean_up_exit(1);
		}
	}
	screen->autoPort = FALSE;
	screen->port = 0;

	http_connections(1);
	rfbInitServer(screen);

	if (! inetd) {
		int conn = 0;
		while (1) {
			if (0) fprintf(stderr, "%d %d %d  %d\n", conn, screen->listenSock, screen->httpSock, screen->httpListenSock);
			usleep(10 * 1000);
			rfbHttpCheckFds(screen);
			if (conn) {
				if (screen->httpSock < 0) {
					break;
				}
			} else {
				if (screen->httpSock >= 0) {
					conn = 1;
				}
			}
			if (!screen->httpDir) {
				break;
			}
			if (screen->httpListenSock < 0) {
				break;
			}
		}
		rfbLog("handle_one_http_request: finished.\n");
		return;
	} else {
#if LIBVNCSERVER_HAVE_FORK
		pid_t pid;
		int s_in = screen->inetdSock;
		if (s_in < 0) {
			rfbLog("handle_one_http_request: inetdSock not set up.\n");
			clean_up_exit(1);
		}
		pid = fork();
		if (pid < 0) {
			rfbLog("handle_one_http_request: could not fork.\n");
			clean_up_exit(1);

		} else if (pid > 0) {
			int status;
			pid_t pidw;
			while (1) {
				rfbHttpCheckFds(screen);
				pidw = waitpid(pid, &status, WNOHANG); 
				if (pidw == pid && WIFEXITED(status)) {
					break;
				} else if (pidw < 0) {
					break;
				}
			}
			rfbLog("handle_one_http_request: finished.\n");
			return;
			
		} else {
			int sock = rfbConnectToTcpAddr("127.0.0.1",
			    screen->httpPort);
			if (sock < 0) {
				exit(1);
			}
			raw_xfer(sock, s_in, s_in);
			exit(0);
		}
#else
		rfbLog("handle_one_http_request: fork not supported.\n");
		clean_up_exit(1);
#endif
	}
}

void user_supplied_opts(char *opts) {
	char *p, *str;
	char *allow[] = {
		"skip-display", "skip-auth", "skip-shared",
		"scale", "scale_cursor", "sc", "solid", "so", "id",
		"clear_mods", "cm", "clear_keys", "ck", "repeat",
		"speeds", "sp", "readtimeout", "rd",
		"rotate", "ro",
		"geometry", "geom", "ge",
		"noncache", "nc",
		"nodisplay", "nd",
		NULL
	};

	if (getenv("X11VNC_NO_UNIXPW_OPTS")) {
		return;
	}

	str = strdup(opts);

	p = strtok(str, ",");
	while (p) {
		char *q;
		int i, n, m, ok = 0;

		i = 0;
		while (allow[i] != NULL) {
			if (strstr(allow[i], "skip-")) {
				i++;
				continue;
			}
			if (strstr(p, allow[i]) == p) 	{
				ok = 1;
				break;
			}
			i++;
		}

		if (! ok && strpbrk(p, "0123456789") == p &&
		    sscanf(p, "%d/%d", &n, &m) == 2) {
			if (scale_str) free(scale_str);
			scale_str = strdup(p);
		} else if (ok) {
			if (strstr(p, "display=") == p) {
				if (use_dpy) free(use_dpy);
				use_dpy = strdup(p + strlen("display="));
			} else if (strstr(p, "auth=") == p) {
				if (auth_file) free(auth_file);
				auth_file = strdup(p + strlen("auth="));
			} else if (!strcmp(p, "shared")) {
				shared = 1;
			} else if (strstr(p, "scale=") == p) {
				if (scale_str) free(scale_str);
				scale_str = strdup(p + strlen("scale="));
			} else if (strstr(p, "scale_cursor=") == p ||
			    strstr(p, "sc=") == p) {
				if (scale_cursor_str) free(scale_cursor_str);
				q = strchr(p, '=') + 1;
				scale_cursor_str = strdup(q);
			} else if (strstr(p, "rotate=") == p ||
			    strstr(p, "ro=") == p) {
				if (rotating_str) free(rotating_str);
				q = strchr(p, '=') + 1;
				rotating_str = strdup(q);
			} else if (!strcmp(p, "solid") || !strcmp(p, "so")) {
				use_solid_bg = 1;
				if (!solid_str) {
					solid_str = strdup(solid_default);
				}
			} else if (strstr(p, "solid=") == p ||
			    strstr(p, "so=") == p) {
				use_solid_bg = 1;
				if (solid_str) free(solid_str);
				q = strchr(p, '=') + 1;
				if (!strcmp(q, "R")) {
					solid_str = strdup("root:");
				} else {
					solid_str = strdup(q);
				}
			} else if (strstr(p, "id=") == p) {
				unsigned long win;
				q = p + strlen("id=");
				if (strcmp(q, "pick")) {
					if (scan_hexdec(q, &win)) {
						subwin = win;
					}
				}
			} else if (!strcmp(p, "clear_mods") ||
			    !strcmp(p, "cm")) {
				clear_mods = 1;
			} else if (!strcmp(p, "clear_keys") ||
			    !strcmp(p, "ck")) {
				clear_mods = 2;
			} else if (!strcmp(p, "noncache") ||
			    !strcmp(p, "nc")) {
				ncache  = 0;
				ncache0 = 0;
			} else if (strstr(p, "nc=") == p) {
				int n2 = atoi(p + strlen("nc="));
				if (nabs(n2) < nabs(ncache)) {
					if (ncache < 0) {
						ncache = -nabs(n2);
					} else {
						ncache = nabs(n2);
					}
				}
			} else if (!strcmp(p, "repeat")) {
				no_autorepeat = 0;
			} else if (strstr(p, "speeds=") == p ||
			    strstr(p, "sp=") == p) {
				if (speeds_str) free(speeds_str);
				q = strchr(p, '=') + 1;
				speeds_str = strdup(q);
				q = speeds_str;
				while (*q != '\0') {
					if (*q == '-') {
						*q = ',';
					}
					q++;
				}
			} else if (strstr(p, "readtimeout=") == p ||
			    strstr(p, "rd=") == p) {
				q = strchr(p, '=') + 1;
				rfbMaxClientWait = atoi(q) * 1000;
			}
		} else {
			rfbLog("skipping option: %s\n", p);
		}
		p = strtok(NULL, ",");
	}
	free(str);
}

static void vnc_redirect_timeout (int sig) {
	write(2, "timeout: no clients connected.\n", 31);
	exit(0);
}

extern char find_display[];
extern char create_display[];
static XImage ximage_struct;

int wait_for_client(int *argc, char** argv, int http) {
	XImage* fb_image;
	int w = 640, h = 480, b = 32;
	int w0, h0, i, chg_raw_fb = 0;
	char *str, *q, *cmd = NULL;
	int db = 0;
	char tmp[] = "/tmp/x11vnc-find_display.XXXXXX";
	int tmp_fd = -1, dt = 0;
	char *create_cmd = NULL;
	char *users_list_save = NULL;
	int created_disp = 0;
	int ncache_save;
	int did_client_connect = 0;
	int loop = 0;
	time_t start;
	char *vnc_redirect_host = "localhost";
	int vnc_redirect_port = -1;
	int vnc_redirect_cnt = 0;
	char vnc_redirect_test[10];

	vnc_redirect = 0;

	if (! use_dpy || strstr(use_dpy, "WAIT:") != use_dpy) {
		return 0;
	}

	if (getenv("WAIT_FOR_CLIENT_DB")) {
		db = 1;
	}

	for (i=0; i < *argc; i++) {
		if (!strcmp(argv[i], "-desktop")) {
			dt = 1;
		}
		if (db) fprintf(stderr, "args %d %s\n", i, argv[i]);
	}
	if (!quiet && !strstr(use_dpy, "FINDDISPLAY-run")) {
		rfbLog("wait_for_client: %s\n", use_dpy);
	}

	str = strdup(use_dpy);
	str += strlen("WAIT");

	xdmcp_insert = NULL;

	/* get any leading geometry: */
	q = strchr(str+1, ':');
	if (q) {
		*q = '\0';
		if (sscanf(str+1, "%dx%d", &w0, &h0) == 2)  {
			w = w0;
			h = h0;
			rfbLog("wait_for_client set: w=%d h=%d\n", w, h);
		}
		*q = ':';
		str = q;
	}

	/* str currently begins with a ':' */
	if (strstr(str, ":cmd=") == str) {
		/* cmd=/path/to/mycommand */
		str++;
	} else if (strpbrk(str, "0123456789") == str+1) {
		/* :0.0 */
		;
	} else {
		/* hostname:0.0 */
		str++;
	}

	if (db) fprintf(stderr, "str: %s\n", str);

	if (strstr(str, "cmd=") == str) {
		if (no_external_cmds || !cmd_ok("WAIT")) {
			rfbLog("wait_for_client external cmds not allowed:"
			    " %s\n", use_dpy);
			clean_up_exit(1);
		}

		cmd = str + strlen("cmd=");
		if (!strcmp(cmd, "FINDDISPLAY-print")) {
			fprintf(stdout, "%s", find_display);
			clean_up_exit(0);
		}
		if (!strcmp(cmd, "FINDDISPLAY-run")) {
			char tmp[] = "/tmp/fd.XXXXXX";
			char com[100];
			int fd = mkstemp(tmp);
			if (fd >= 0) {
				write(fd, find_display, strlen(find_display));
				close(fd);
				set_env("FINDDISPLAY_run", "1");
				sprintf(com, "/bin/sh %s -n; rm -f %s", tmp, tmp);
				system(com);
			}
			unlink(tmp);
			exit(0);
		}
		if (!strcmp(str, "FINDCREATEDISPLAY-print")) {
			fprintf(stdout, "%s", create_display);
			clean_up_exit(0);
		}
		if (db) fprintf(stderr, "cmd: %s\n", cmd);
		if (strstr(str, "FINDCREATEDISPLAY") || strstr(str, "FINDDISPLAY")) {
			if (strstr(str, "Xvnc.redirect") || strstr(str, "X.redirect")) {
				vnc_redirect = 1;
			}
		}
		if (strstr(cmd, "FINDDISPLAY-vnc_redirect") == cmd) {
			int p;
			char h[256];
			if (strlen(cmd) >= 256) {
				rfbLog("wait_for_client string too long: %s\n", str);
				clean_up_exit(1);
			}
			h[0] = '\0';
			if (sscanf(cmd, "FINDDISPLAY-vnc_redirect=%d", &p) == 1) {
				;
			} else if (sscanf(cmd, "FINDDISPLAY-vnc_redirect=%s %d", h, &p) == 2) {
				;
			} else {
				rfbLog("wait_for_client bad string: %s\n", cmd);
				clean_up_exit(1);
			}
			vnc_redirect_port = p;
			if (strcmp(h, "")) {
				vnc_redirect_host = strdup(h);
			}
			vnc_redirect = 2;
			rfbLog("wait_for_client: vnc_redirect: %s:%d\n", vnc_redirect_host, vnc_redirect_port);
		}
	}
	
	if (fake_fb) {
		free(fake_fb);
	}
	fake_fb = (char *) calloc(w*h*b/8, 1);

	fb_image = &ximage_struct;
	fb_image->data = fake_fb;
	fb_image->format = ZPixmap;
	fb_image->width  = w;
	fb_image->height = h;
	fb_image->bits_per_pixel = b;
	fb_image->bytes_per_line = w*b/8;
	fb_image->bitmap_unit = -1;
	fb_image->depth = 24;
	fb_image->red_mask   = 0xff0000;
	fb_image->green_mask = 0x00ff00;
	fb_image->blue_mask  = 0x0000ff;

	depth = fb_image->depth;

	dpy_x = wdpy_x = w;
	dpy_y = wdpy_y = h;
	off_x = 0;
	off_y = 0;

	if (! dt) {
		char *s;
		argv[*argc] = strdup("-desktop");
		*argc = (*argc) + 1;

		if (cmd) {
			char *q;
			s = choose_title(":0");
			q = strstr(s, ":0");
			if (q) {
				*q = '\0';
			}
		} else {
			s = choose_title(str);
		}
		rfb_desktop_name = strdup(s);
		argv[*argc] = s;
		*argc = (*argc) + 1;
	}

	ncache_save = ncache;
	ncache = 0;

	initialize_allowed_input();

	if (! multiple_cursors_mode) {
		multiple_cursors_mode = strdup("default");
	}
	initialize_cursors_mode();
	
	initialize_screen(argc, argv, fb_image);

	initialize_signals();

	if (! raw_fb) {
		chg_raw_fb = 1;
		/* kludge to get RAWFB_RET with dpy == NULL guards */
		raw_fb = (char *) 0x1;
	}

	if (cmd && !strcmp(cmd, "HTTPONCE")) {
		handle_one_http_request();	
		clean_up_exit(0);
	}

	if (http && check_httpdir()) {
		http_connections(1);
	}

	if (cmd && unixpw) {
		keep_unixpw = 1;
	}

	if (!inetd) {
		if (!use_openssl) {
			announce(screen->port, use_openssl, NULL);
			fprintf(stdout, "PORT=%d\n", screen->port);
		} else {
			fprintf(stdout, "PORT=%d\n", screen->port);
			if (stunnel_port) {
				fprintf(stdout, "SSLPORT=%d\n", stunnel_port);
			} else if (use_openssl) {
				fprintf(stdout, "SSLPORT=%d\n", screen->port);
			}
		}
		fflush(stdout);
	} else if (!use_openssl && avahi) {
		char *name = rfb_desktop_name;
		if (!name) {
			name = use_dpy;
		}
		avahi_initialise();
		avahi_advertise(name, this_host(), screen->port);
	}

	if (getenv("WAITBG")) {
#if LIBVNCSERVER_HAVE_FORK && LIBVNCSERVER_HAVE_SETSID
		int p, n;
		if ((p = fork()) > 0)  {
			exit(0);
		} else if (p == -1) {
			rfbLogEnable(1);
			fprintf(stderr, "could not fork\n");
			perror("fork");
			clean_up_exit(1);
		}
		if (setsid() == -1) {
			rfbLogEnable(1);
			fprintf(stderr, "setsid failed\n");
			perror("setsid");
			clean_up_exit(1);
		}
		/* adjust our stdio */
		n = open("/dev/null", O_RDONLY);
		dup2(n, 0);
		dup2(n, 1);
		if (! logfile) {
			dup2(n, 2);
		}
		if (n > 2) {
			close(n);
		}
#else
		clean_up_exit(1);
#endif
	}

	if (vnc_redirect) {
		if (unixpw) {
			rfbLog("wait_for_client: -unixpw and Xvnc.redirect not allowed\n");
			clean_up_exit(1);
		}
		if (client_connect) {
			rfbLog("wait_for_client: -connect and Xvnc.redirect not allowed\n");
			clean_up_exit(1);
		}
		if (inetd) {
			if (use_openssl) {
				accept_openssl(OPENSSL_INETD, -1);
			}
		} else {
			int gotone = 0;
			if (first_conn_timeout) {
				if (first_conn_timeout < 0) {
					first_conn_timeout = -first_conn_timeout;
				}
				signal(SIGALRM, vnc_redirect_timeout);
				alarm(first_conn_timeout);
			}
			if (use_openssl) {
				accept_openssl(OPENSSL_VNC, -1);
			} else {
				struct sockaddr_in addr;
#ifdef __hpux
				int addrlen = sizeof(addr);
#else
				socklen_t addrlen = sizeof(addr);
#endif
				if (screen->listenSock < 0) {
					rfbLog("wait_for_client: Xvnc.redirect not listening... sock=%d port=%d\n", screen->listenSock, screen->port);
					clean_up_exit(1);
				}
				vnc_redirect_sock = accept(screen->listenSock, (struct sockaddr *)&addr, &addrlen);
			}
			if (first_conn_timeout) {
				alarm(0);
			}
		}
		if (vnc_redirect_sock < 0) {
			rfbLog("wait_for_client: vnc_redirect failed.\n");
			clean_up_exit(1);
		}
		if (!inetd && use_openssl) {
			/* check for Fetch Cert closing */
			fd_set rfds;
			struct timeval tv;
			int nfds;

			usleep(300*1000);

			FD_ZERO(&rfds);
			FD_SET(vnc_redirect_sock, &rfds);

			tv.tv_sec = 0;
			tv.tv_usec = 200000;
			nfds = select(vnc_redirect_sock+1, &rfds, NULL, NULL, &tv);

			rfbLog("wait_for_client: vnc_redirect nfds: %d\n", nfds);
			if (nfds > 0) {
				int n;
				n = read(vnc_redirect_sock, vnc_redirect_test, 1);
				if (n <= 0) {
					close(vnc_redirect_sock);
					vnc_redirect_sock = -1;
					rfbLog("wait_for_client: waiting for 2nd connection (Fetch Cert?)\n");
					accept_openssl(OPENSSL_VNC, -1);
					if (vnc_redirect_sock < 0) {
						rfbLog("wait_for_client: vnc_redirect failed.\n");
						clean_up_exit(1);
					}
				} else {
					vnc_redirect_cnt = n;
				}
			}
		}
		goto vnc_redirect_place;
	}

	if (inetd && use_openssl) {
		accept_openssl(OPENSSL_INETD, -1);
	}

	if (client_connect != NULL) {
		char *remainder = NULL;
		if (inetd) {
			rfbLog("wait_for_client: -connect disallowed in inetd mode: %s\n",
			    client_connect);
		} else if (screen && screen->clientHead) {
			rfbLog("wait_for_client: -connect disallowed: client exists: %s\n",
			    client_connect);
		} else if (strchr(client_connect, '=')) {
			rfbLog("wait_for_client: invalid -connect string: %s\n",
			    client_connect);
		} else {
			char *q = strchr(client_connect, ',');
			if (q) {
				rfbLog("wait_for_client: only using first"
				    " connect host in: %s\n", client_connect);
				remainder = strdup(q+1);
				*q = '\0';
			}
			rfbLog("wait_for_client: reverse_connect(%s)\n",
			    client_connect);
			reverse_connect(client_connect);
			did_client_connect = 1;
		}
		free(client_connect);
		if (remainder != NULL) {
			/* reset to host2,host3,... */
			client_connect = remainder;
		} else {
			client_connect = NULL;
		}
	}

	if (first_conn_timeout < 0) {
		first_conn_timeout = -first_conn_timeout;
	}
	start = time(NULL);

	while (1) {
		loop++;
		if (first_conn_timeout && time(NULL) > start + first_conn_timeout) {
			rfbLog("no client connect after %d seconds.\n", first_conn_timeout);
			shut_down = 1;
		}
		if (shut_down) {
			clean_up_exit(0);
		}
		if (loop < 2) {
			if (did_client_connect) {
				goto screen_check;
			}
			if (inetd) {
				goto screen_check;
			}
			if (screen && screen->clientHead) {
				goto screen_check;
			}
		}
		if (use_openssl && !inetd) {
			check_openssl();
			/*
			 * This is to handle an initial verify cert from viewer,
			 * they disconnect right after fetching the cert.
			 */
			if (! use_threads) rfbPE(-1);
			if (screen && screen->clientHead) {
				int i;
				if (unixpw) {
					if (! unixpw_in_progress) {
						rfbLog("unixpw but no unixpw_in_progress\n");
						clean_up_exit(1);
					}
					if (unixpw_client && unixpw_client->onHold) {
						rfbLog("taking unixpw_client off hold\n");
						unixpw_client->onHold = FALSE;
					}
				}
				for (i=0; i<10; i++) {
					if (shut_down) {
						clean_up_exit(0);
					}
					usleep(20 * 1000);
					if (0) rfbLog("wait_for_client: %d\n", i);

					if (! use_threads) {
						if (unixpw) {
							unixpw_in_rfbPE = 1;
						}
						rfbPE(-1);
						if (unixpw) {
							unixpw_in_rfbPE = 0;
						}
					}

					if (unixpw && !unixpw_in_progress) {
						/* XXX too soon. */
						goto screen_check;
					}
					if (!screen->clientHead) {
						break;
					}
				}
			}
		} else if (use_openssl) {
			check_openssl();
		}

		if (! use_threads) {
			rfbPE(-1);
		}
		screen_check:
		if (! screen || ! screen->clientHead) {
			usleep(100 * 1000);
			continue;
		}
		rfbLog("wait_for_client: got client\n");
		break;
	}


	if (unixpw) {
		if (! unixpw_in_progress) {
			rfbLog("unixpw but no unixpw_in_progress\n");
			clean_up_exit(1);
		}
		if (unixpw_client && unixpw_client->onHold) {
			rfbLog("taking unixpw_client off hold.\n");
			unixpw_client->onHold = FALSE;
		}
		if (cmd && strstr(cmd, "FINDCREATEDISPLAY") == cmd) {
			if (users_list && strstr(users_list, "unixpw=") == users_list) {
				users_list_save = users_list;
				users_list = NULL;
			}
		}
		while (1) {
			if (shut_down) {
				clean_up_exit(0);
			}
			if (! use_threads) {
				unixpw_in_rfbPE = 1;
				rfbPE(-1);
				unixpw_in_rfbPE = 0;
			}
			if (unixpw_in_progress) {
				usleep(20 * 1000);
				continue;
			}
			rfbLog("wait_for_client: unixpw finished.\n");
			break;
		}
	}

if (0) db = 1;

	vnc_redirect_place:

	if (vnc_redirect == 2) {
		;
	} else if (cmd) {
		char line1[1024];
		char line2[16384];
		char *q;
		int n;
		int nodisp = 0;
		int saw_xdmcp = 0;
		char *usslpeer = NULL;

		memset(line1, 0, 1024);
		memset(line2, 0, 16384);

		if (users_list && strstr(users_list, "sslpeer=") == users_list) {
			int ok = 0;
			char *u = NULL, *upeer = NULL;

			if (certret_str) {
				char *q, *p, *str = strdup(certret_str);
				q = strstr(str, "Subject: ");
				if (! q) return 0;
				p = strstr(q, "\n");
				if (p) *p = '\0';
				q = strstr(q, "CN=");
				if (! q) return 0;
				if (! getenv("X11VNC_SSLPEER_CN")) {
					p = q;
					q = strstr(q, "/emailAddress=");
					if (! q) q = strstr(p, "/Email=");
					if (! q) return 0;
				}
				q = strstr(q, "=");
				if (! q) return 0;
				q++;
				p = strstr(q, " ");
				if (p) *p = '\0';
				p = strstr(q, "@");
				if (p) *p = '\0';
				p = strstr(q, "/");
				if (p) *p = '\0';
				upeer = strdup(q);
				if (strcmp(upeer, "")) {
					p = upeer;
					while (*p != '\0') {
						char c = *p;
						if (!isalnum((int) c)) {
							*p = '\0';
							break;
						}
						p++;
					}
					if (strcmp(upeer, "")) {
						ok = 1;
					}
				}
			}
			if (! ok || !upeer) {
				return 0;
			}
			rfbLog("sslpeer unix username extracted from x509 cert: %s\n", upeer);
			u = (char *) malloc(strlen(upeer+2));
			u[0] = '\0';
			if (!strcmp(users_list, "sslpeer=")) {
				sprintf(u, "+%s", upeer);
			} else {
				char *p, *str = strdup(users_list);
				p = strtok(str + strlen("sslpeer="), ",");
				while (p) {
					if (!strcmp(p, upeer)) {
						sprintf(u, "+%s", upeer);
						break;
					}
					p = strtok(NULL, ",");
				}
				free(str);
			}
			if (u[0] == '\0') {
				rfbLog("sslpeer cannot determine user: %s\n", upeer);
				free(u);
				return 0;
			}
			free(u);
			usslpeer = upeer;
		}

		/* only sets environment variables: */
		run_user_command("", latest_client, "env", NULL, 0, NULL);

		if (program_name) {
			set_env("X11VNC_PROG", program_name);
		} else {
			set_env("X11VNC_PROG", "x11vnc");
		}

		if (!strcmp(cmd, "FINDDISPLAY") ||
		    strstr(cmd, "FINDCREATEDISPLAY") == cmd) {
			char *nd = "";
			tmp_fd = mkstemp(tmp);
			if (tmp_fd < 0) {
				rfbLog("wait_for_client: open failed: %s\n", tmp);
				rfbLogPerror("mkstemp");
				clean_up_exit(1);
			}
			chmod(tmp, 0644);
			if (getenv("X11VNC_FINDDISPLAY_ALWAYS_FAILS")) {
				char *s = "#!/bin/sh\necho _FAIL_\nexit 1\n";
				write(tmp_fd, s, strlen(s));
			} else {
				write(tmp_fd, find_display, strlen(find_display));
			}
			close(tmp_fd);
			nodisp = 1;

			if (strstr(cmd, "FINDCREATEDISPLAY") == cmd) {
				char *opts = strchr(cmd, '-');
				char st[] = "";
				char fdgeom[128], fdsess[128], fdopts[128], fdprog[128];
				char fdxsrv[128], fdxdum[128], fdcups[128], fdesd[128];
				char fdnas[128], fdsmb[128], fdtag[128];
				if (opts) {
					opts++;
					if (strstr(opts, "xdmcp")) {
						saw_xdmcp = 1;
					}
				} else {
					opts = st;
				}
				sprintf(fdgeom, "NONE");
				fdsess[0] = '\0';
				fdgeom[0] = '\0';
				fdopts[0] = '\0';
				fdprog[0] = '\0';
				fdxsrv[0] = '\0';
				fdxdum[0] = '\0';
				fdcups[0] = '\0';
				fdesd[0]  = '\0';
				fdnas[0]  = '\0';
				fdsmb[0]  = '\0';
				fdtag[0]  = '\0';

				if (unixpw && keep_unixpw_opts && keep_unixpw_opts[0] != '\0') {
					char *q, *p, *t = strdup(keep_unixpw_opts);
					if (strstr(t, "gnome")) {
						sprintf(fdsess, "gnome");
					} else if (strstr(t, "kde")) {
						sprintf(fdsess, "kde");
					} else if (strstr(t, "twm")) {
						sprintf(fdsess, "twm");
					} else if (strstr(t, "fvwm")) {
						sprintf(fdsess, "fvwm");
					} else if (strstr(t, "mwm")) {
						sprintf(fdsess, "mwm");
					} else if (strstr(t, "cde")) {
						sprintf(fdsess, "cde");
					} else if (strstr(t, "dtwm")) {
						sprintf(fdsess, "dtwm");
					} else if (strstr(t, "xterm")) {
						sprintf(fdsess, "xterm");
					} else if (strstr(t, "wmaker")) {
						sprintf(fdsess, "wmaker");
					} else if (strstr(t, "xfce")) {
						sprintf(fdsess, "xfce");
					} else if (strstr(t, "enlightenment")) {
						sprintf(fdsess, "enlightenment");
					} else if (strstr(t, "Xsession")) {
						sprintf(fdsess, "Xsession");
					} else if (strstr(t, "failsafe")) {
						sprintf(fdsess, "failsafe");
					}

					q = strstr(t, "ge=");
					if (! q) q = strstr(t, "geom=");
					if (! q) q = strstr(t, "geometry=");
					if (q) {
						int ok = 1;
						q = strstr(q, "=");
						q++;
						p = strstr(q, ",");
						if (p) *p = '\0';
						p = q;
						while (*p) {
							if (*p == 'x') {
								;
							} else if (isdigit((int) *p)) {
								;
							} else {
								ok = 0;
								break;
							}
							p++;
						}
						if (ok && strlen(q) < 32) {
							sprintf(fdgeom, q);
							if (!quiet) {
								rfbLog("set create display geom: %s\n", fdgeom);
							}
						}
					}
					q = strstr(t, "cups=");
					if (q) {
						int p;
						if (sscanf(q, "cups=%d", &p) == 1) {
							sprintf(fdcups, "%d", p);
						}
					}
					q = strstr(t, "esd=");
					if (q) {
						int p;
						if (sscanf(q, "esd=%d", &p) == 1) {
							sprintf(fdesd, "%d", p);
						}
					}
					free(t);
				}
				if (fdgeom[0] == '\0' && getenv("FD_GEOM")) {
					snprintf(fdgeom,  120, "%s", getenv("FD_GEOM"));
				}
				if (fdsess[0] == '\0' && getenv("FD_SESS")) {
					snprintf(fdsess, 120, "%s", getenv("FD_SESS"));
				}
				if (fdopts[0] == '\0' && getenv("FD_OPTS")) {
					snprintf(fdopts, 120, "%s", getenv("FD_OPTS"));
				}
				if (fdprog[0] == '\0' && getenv("FD_PROG")) {
					snprintf(fdprog, 120, "%s", getenv("FD_PROG"));
				}
				if (fdxsrv[0] == '\0' && getenv("FD_XSRV")) {
					snprintf(fdxsrv, 120, "%s", getenv("FD_XSRV"));
				}
				if (fdcups[0] == '\0' && getenv("FD_CUPS")) {
					snprintf(fdcups, 120, "%s", getenv("FD_CUPS"));
				}
				if (fdesd[0] == '\0' && getenv("FD_ESD")) {
					snprintf(fdesd, 120, "%s", getenv("FD_ESD"));
				}
				if (fdnas[0] == '\0' && getenv("FD_NAS")) {
					snprintf(fdnas, 120, "%s", getenv("FD_NAS"));
				}
				if (fdsmb[0] == '\0' && getenv("FD_SMB")) {
					snprintf(fdsmb, 120, "%s", getenv("FD_SMB"));
				}
				if (fdtag[0] == '\0' && getenv("FD_TAG")) {
					snprintf(fdtag, 120, "%s", getenv("FD_TAG"));
				}
				if (fdxdum[0] == '\0' && getenv("FD_XDUMMY_NOROOT")) {
					snprintf(fdxdum, 120, "%s", getenv("FD_XDUMMY_NOROOT"));
				}

				set_env("FD_GEOM", fdgeom);
				set_env("FD_OPTS", fdopts);
				set_env("FD_PROG", fdprog);
				set_env("FD_XSRV", fdxsrv);
				set_env("FD_CUPS", fdcups);
				set_env("FD_ESD",  fdesd);
				set_env("FD_NAS",  fdnas);
				set_env("FD_SMB",  fdsmb);
				set_env("FD_TAG",  fdtag);
				set_env("FD_XDUMMY_NOROOT", fdxdum);
				set_env("FD_SESS", fdsess);

				if (usslpeer || (unixpw && keep_unixpw_user)) {
					char *uu = usslpeer;
					if (!uu) {
						uu = keep_unixpw_user;
					}
					create_cmd = (char *) malloc(strlen(tmp)+1
					    + strlen("env USER='' ")
					    + strlen("FD_GEOM='' ")
					    + strlen("FD_OPTS='' ")
					    + strlen("FD_PROG='' ")
					    + strlen("FD_XSRV='' ")
					    + strlen("FD_CUPS='' ")
					    + strlen("FD_ESD='' ")
					    + strlen("FD_NAS='' ")
					    + strlen("FD_SMB='' ")
					    + strlen("FD_TAG='' ")
					    + strlen("FD_XDUMMY_NOROOT='' ")
					    + strlen("FD_SESS='' /bin/sh ")
					    + strlen(uu) + 1
					    + strlen(fdgeom) + 1
					    + strlen(fdopts) + 1
					    + strlen(fdprog) + 1
					    + strlen(fdxsrv) + 1
					    + strlen(fdcups) + 1
					    + strlen(fdesd) + 1
					    + strlen(fdnas) + 1
					    + strlen(fdsmb) + 1
					    + strlen(fdtag) + 1
					    + strlen(fdxdum) + 1
					    + strlen(fdsess) + 1
					    + strlen(opts) + 1);
					sprintf(create_cmd, "env USER='%s' FD_GEOM='%s' FD_SESS='%s' "
					    "FD_OPTS='%s' FD_PROG='%s' FD_XSRV='%s' FD_CUPS='%s' "
					    "FD_ESD='%s' FD_NAS='%s' FD_SMB='%s' FD_TAG='%s' "
					    "FD_XDUMMY_NOROOT='%s' /bin/sh %s %s",
					    uu, fdgeom, fdsess, fdopts, fdprog, fdxsrv,
					    fdcups, fdesd, fdnas, fdsmb, fdtag, fdxdum, tmp, opts);
				} else {
					create_cmd = (char *) malloc(strlen(tmp)
					    + strlen("/bin/sh ") + 1 + strlen(opts) + 1);
					sprintf(create_cmd, "/bin/sh %s %s", tmp, opts);
				}

if (db) fprintf(stderr, "create_cmd: %s\n", create_cmd);
			}
			if (getenv("X11VNC_SKIP_DISPLAY")) {
				nd = strdup(getenv("X11VNC_SKIP_DISPLAY"));
			}
			if (unixpw && keep_unixpw_opts && keep_unixpw_opts[0] != '\0') {
				char *q, *t = keep_unixpw_opts;
				q = strstr(t, "nd=");
				if (! q) q = strstr(t, "nodisplay=");
				if (q) {
					char *t2;
					q = strchr(q, '=') + 1;
					t = strdup(q);
					q = t;
					t2 = strchr(t, ',');
					if (t2) *t2 = '\0';
					while (*t != '\0') {
						if (*t == '-') {
							*t = ',';
						}
						t++;
					}
					if (!strchr(q, '\'')) {
						if (! quiet) rfbLog("set X11VNC_SKIP_DISPLAY: %s\n", q);
						nd = q;
					}
				}
			}

			cmd = (char *) malloc(strlen("env X11VNC_SKIP_DISPLAY='' ")
			    + strlen(nd) + strlen(tmp) + strlen("/bin/sh ") + 1);
			sprintf(cmd, "env X11VNC_SKIP_DISPLAY='%s' /bin/sh %s", nd, tmp);
		}

		rfbLog("wait_for_client: running: %s\n", cmd);

		if (unixpw) {
			int res = 0, k, j, i;
			char line[18000];

			memset(line, 0, 18000);

			if (keep_unixpw_user && keep_unixpw_pass) {
				n = 18000;
				res = su_verify(keep_unixpw_user,
				    keep_unixpw_pass, cmd, line, &n, nodisp);
			}

if (db) {fprintf(stderr, "line: "); write(2, line, n); write(2, "\n", 1); fprintf(stderr, "res=%d n=%d\n", res, n);}
			if (! res) {
				rfbLog("wait_for_client: find display cmd failed\n");
			}

			if (! res && create_cmd) {
				FILE *mt = fopen(tmp, "w");
				if (! mt) {
					rfbLog("wait_for_client: open failed: %s\n", tmp);
					rfbLogPerror("fopen");
					clean_up_exit(1);
				}
				fprintf(mt, "%s", create_display);
				fclose(mt);

				findcreatedisplay = 1;

				if (getuid() != 0) {
					/* if not root, run as the other user... */
					n = 18000;
					close_exec_fds();
					res = su_verify(keep_unixpw_user,
					    keep_unixpw_pass, create_cmd, line, &n, nodisp);
if (db) fprintf(stderr, "c-res=%d n=%d line: '%s'\n", res, n, line);

				} else {
					FILE *p;
					close_exec_fds();
					rfbLog("wait_for_client: running: %s\n", create_cmd);
					p = popen(create_cmd, "r");
					if (! p) {
						rfbLog("wait_for_client: popen failed: %s\n", create_cmd);
						res = 0;
					} else if (fgets(line1, 1024, p) == NULL) {
						rfbLog("wait_for_client: read failed: %s\n", create_cmd);
						res = 0;
					} else {
						n = fread(line2, 1, 16384, p);
						if (pclose(p) != 0) {
							res = 0;
						} else {
							strncpy(line, line1, 100);
							memcpy(line + strlen(line1), line2, n);
if (db) fprintf(stderr, "line1: '%s'\n", line1);
							n += strlen(line1);
							created_disp = 1;
							res = 1;
						}
					}
				}
				if (res && saw_xdmcp) {
					xdmcp_insert = strdup(keep_unixpw_user);
				}
			}

			if (tmp_fd >= 0) {
				unlink(tmp);
			}

			if (! res) {
				rfbLog("wait_for_client: cmd failed: %s\n", cmd);
				unixpw_msg("No DISPLAY found.", 3);
				clean_up_exit(1);
			}

			/*
			 * we need to hunt for DISPLAY= since there may be
			 * a login banner or something at the beginning.
			 */
			q = strstr(line, "DISPLAY=");
			if (! q) {
				q = line;
			}
			n -= (q - line);

			for (k = 0; k < 1024; k++) {
				line1[k] = q[k];
				if (q[k] == '\n') {
					k++;
					break;
				}
			}
			n -= k;
			i = 0;
			for (j = 0; j < 16384; j++) {
				if (j < 16384 - 1) {
					/* xauth data, assume pty added CR */
					if (q[k+j] == '\r' && q[k+j+1] == '\n') {
						continue;
					}
				}
				
				line2[i] = q[k+j];
				i++;
			}
if (db) write(2, line, 100);
if (db) fprintf(stderr, "\n");
		} else {
			FILE *p;
			int rc;
			close_exec_fds();

			if (usslpeer) {
				char *c;
				if (getuid() == 0) {
					c = (char *) malloc(strlen("su - '' -c \"")
					    + strlen(usslpeer) + strlen(cmd) + 1 + 1);
					sprintf(c, "su - '%s' -c \"%s\"", usslpeer, cmd);
				} else {
					c = strdup(cmd);
				}
				p = popen(c, "r");
				free(c);
				
			} else {
				p = popen(cmd, "r");
			}
			if (! p) {
				rfbLog("wait_for_client: cmd failed: %s\n", cmd);
				rfbLogPerror("popen");
				if (tmp_fd >= 0) {
					unlink(tmp);
				}
				clean_up_exit(1);
			}
			if (fgets(line1, 1024, p) == NULL) {
				rfbLog("wait_for_client: read failed: %s\n", cmd);
				rfbLogPerror("fgets");
				if (tmp_fd >= 0) {
					unlink(tmp);
				}
				clean_up_exit(1);
			}
			n = fread(line2, 1, 16384, p);
			rc = pclose(p);

			if (rc != 0) {
				rfbLog("wait_for_client: find display cmd failed\n");
			}

			if (create_cmd && rc != 0) {
				FILE *mt = fopen(tmp, "w");
				if (! mt) {
					rfbLog("wait_for_client: open failed: %s\n", tmp);
					rfbLogPerror("fopen");
					if (tmp_fd >= 0) {
						unlink(tmp);
					}
					clean_up_exit(1);
				}
				fprintf(mt, "%s", create_display);
				fclose(mt);

				findcreatedisplay = 1;

				rfbLog("wait_for_client: FINDCREATEDISPLAY cmd: %s\n", create_cmd);

				p = popen(create_cmd, "r");
				if (! p) {
					rfbLog("wait_for_client: cmd failed: %s\n", create_cmd);
					rfbLogPerror("popen");
					if (tmp_fd >= 0) {
						unlink(tmp);
					}
					clean_up_exit(1);
				}
				if (fgets(line1, 1024, p) == NULL) {
					rfbLog("wait_for_client: read failed: %s\n", create_cmd);
					rfbLogPerror("fgets");
					if (tmp_fd >= 0) {
						unlink(tmp);
					}
					clean_up_exit(1);
				}
				n = fread(line2, 1, 16384, p);
			}
			if (tmp_fd >= 0) {
				unlink(tmp);
			}
		}

if (db) fprintf(stderr, "line1=%s\n", line1);

		if (strstr(line1, "DISPLAY=") != line1) {
			rfbLog("wait_for_client: bad reply '%s'\n", line1);
			if (unixpw) {
				unixpw_msg("No DISPLAY found.", 3);
			}
			clean_up_exit(1);
		}


		if (strstr(line1, ",VT=")) {
			int vt;
			char *t = strstr(line1, ",VT=");
			vt = atoi(t + strlen(",VT="));
			*t = '\0';
			if (7 <= vt && vt <= 15) {
				char chvt[100];
				sprintf(chvt, "chvt %d >/dev/null 2>/dev/null &", vt);
				rfbLog("running: %s\n", chvt);
				system(chvt);
				sleep(2);
			}
		} else if (strstr(line1, ",XPID=")) {
			int i, pvt, vt = -1;
			char *t = strstr(line1, ",XPID=");
			pvt = atoi(t + strlen(",XPID="));
			*t = '\0';
			if (pvt > 0) {
				for (i=3; i <= 10; i++) {
					int k;
					char proc[100];
					char buf[100];
					sprintf(proc, "/proc/%d/fd/%d", pvt, i);
if (db) fprintf(stderr, "%d -- %s\n", i, proc);
					for (k=0; k < 100; k++) {
						buf[k] = '\0';
					}
		
					if (readlink(proc, buf, 100) != -1) {
						buf[100-1] = '\0';
if (db) fprintf(stderr, "%d -- %s -- %s\n", i, proc, buf);
						if (strstr(buf, "/dev/tty") == buf) {
							vt = atoi(buf + strlen("/dev/tty"));
							if (vt > 0) {
								break;
							}
						}
					}
				}
			}
			if (7 <= vt && vt <= 12) {
				char chvt[100];
				sprintf(chvt, "chvt %d >/dev/null 2>/dev/null &", vt);
				rfbLog("running: %s\n", chvt);
				system(chvt);
				sleep(2);
			}
		}

		use_dpy = strdup(line1 + strlen("DISPLAY="));
		q = use_dpy;
		while (*q != '\0') {
			if (*q == '\n' || *q == '\r') *q = '\0';
			q++;
		}
		if (line2[0] != '\0') {
			if (strstr(line2, "XAUTHORITY=") == line2) {
				q = line2;
				while (*q != '\0') {
					if (*q == '\n' || *q == '\r') *q = '\0';
					q++;
				}
				if (auth_file) {
					free(auth_file);
				}
				auth_file = strdup(line2 + strlen("XAUTHORITY="));

			} else {
				xauth_raw_data = (char *)malloc(n);
				xauth_raw_len = n;
				memcpy(xauth_raw_data, line2, n);
if (db) {fprintf(stderr, "xauth_raw_len: %d\n", n);
write(2, xauth_raw_data, n);
fprintf(stderr, "\n");}
			}
		}

		if (usslpeer) {
			char *u = (char *) malloc(strlen(usslpeer+2));
			sprintf(u, "+%s", usslpeer);
			if (switch_user(u, 0)) {
				rfbLog("sslpeer switched to user: %s\n", usslpeer);
			} else {
				rfbLog("sslpeer failed to switch to user: %s\n", usslpeer);
			}
			free(u);
			
		} else if (users_list_save && keep_unixpw_user) {
			char *user = keep_unixpw_user;
			char *u = (char *)malloc(strlen(user)+1); 

			users_list = users_list_save;

			u[0] = '\0';
			if (!strcmp(users_list, "unixpw=")) {
				sprintf(u, "+%s", user);
			} else {
				char *p, *str = strdup(users_list);
				p = strtok(str + strlen("unixpw="), ",");
				while (p) {
					if (!strcmp(p, user)) {
						sprintf(u, "+%s", user);
						break;
					}
					p = strtok(NULL, ",");
				}
				free(str);
			}
			
			if (u[0] == '\0') {
				rfbLog("unixpw_accept skipping switch to user: %s\n", user);
			} else if (switch_user(u, 0)) {
				rfbLog("unixpw_accept switched to user: %s\n", user);
			} else {
				rfbLog("unixpw_accept failed to switch to user: %s\n", user);
			}
			free(u);
		}

		if (unixpw) {
			char str[32];

			if (keep_unixpw_user && keep_unixpw_pass) {
				strzero(keep_unixpw_user);
				strzero(keep_unixpw_pass);
				keep_unixpw = 0;
			}

			if (created_disp) {
				snprintf(str, 30, "Created DISPLAY %s", use_dpy);
			} else {
				snprintf(str, 30, "Using DISPLAY %s", use_dpy);
			}
			unixpw_msg(str, 2);
		}
	} else {
		use_dpy = strdup(str);
	}
	if (chg_raw_fb) {
		raw_fb = NULL;
	}

	ncache = ncache_save;

	if (unixpw && keep_unixpw_opts && keep_unixpw_opts[0] != '\0') {
		user_supplied_opts(keep_unixpw_opts);
	}
	if (create_cmd) {
		free(create_cmd);
	}

	if (vnc_redirect) {
		char *q = strchr(use_dpy, ':');
		int vdpy = -1, sock = -1;
		int s_in, s_out, i;
		if (vnc_redirect == 2) {
			char num[32];	
			sprintf(num, ":%d", vnc_redirect_port);
			q = num;
		}
		if (!q) {
			rfbLog("wait_for_client: can't find number in X display: %s\n", use_dpy);
			clean_up_exit(1);
		}
		if (sscanf(q+1, "%d", &vdpy) != 1) {
			rfbLog("wait_for_client: can't find number in X display: %s\n", q);
			clean_up_exit(1);
		}
		if (vdpy == -1 && vnc_redirect != 2) {
			rfbLog("wait_for_client: can't find number in X display: %s\n", q);
			clean_up_exit(1);
		}
		if (vnc_redirect == 2) {
			if (vdpy < 0) {
				vdpy = -vdpy;
			} else if (vdpy < 200) {
				vdpy += 5900;
			}
		} else {
			vdpy += 5900;
		}
		if (created_disp) {
			usleep(1000*1000);
		}
		for (i=0; i < 20; i++) {
			sock = rfbConnectToTcpAddr(vnc_redirect_host, vdpy);
			if (sock >= 0) {
				break;
			}
			rfbLog("wait_for_client: ...\n");
			usleep(500*1000);
		}
		if (sock < 0) {
			rfbLog("wait_for_client: could not connect to a VNC Server at %s:%d\n", vnc_redirect_host, vdpy);
			clean_up_exit(1);
		}
		if (inetd) {
			s_in  = fileno(stdin);
			s_out = fileno(stdout);
		} else {
			s_in = s_out = vnc_redirect_sock;
		}
		if (vnc_redirect_cnt > 0) {
			write(vnc_redirect_sock, vnc_redirect_test, vnc_redirect_cnt);
		}
		rfbLog("wait_for_client: switching control to VNC Server at %s:%d\n", vnc_redirect_host, vdpy);
		raw_xfer(sock, s_in, s_out);
		clean_up_exit(0);
	}

	return 1;
}

