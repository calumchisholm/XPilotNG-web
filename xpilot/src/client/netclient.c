/* $Id$
 *
 * XPilot, a multiplayer gravity war game.  Copyright (C) 1991-98 by
 *
 *      Bj�rn Stabell        <bjoern@xpilot.org>
 *      Ken Ronny Schouten   <ken@xpilot.org>
 *      Bert Gijsbers        <bert@xpilot.org>
 *      Dick Balaska         <dick@xpilot.org>
 *
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#ifdef	_WINDOWS
#include "NT/winclient.h"
#include "NT/winNet.h"
#include "NT/winAudio.h"
#include "NT/winX.h"
#include "NT/winXThread.h"
#include "NT/winXXPilot.h"
#endif

#ifndef INT_MAX
#define INT_MAX 4294967295
#endif

#include "types.h"

#ifndef	_WINDOWS
#include <unistd.h>
#ifndef VMS
#include <sys/param.h>
#endif
#endif
#if defined(__hpux) || defined(_WINDOWS)
#include <time.h>
#else
#include <sys/time.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#ifdef VMS
#include <socket.h>
#include <in.h>
#else
#ifndef	_WINDOWS
#include <sys/socket.h>
#include <netinet/in.h>
#endif
#endif
#ifndef	_WINDOWS
#include <netdb.h>
#endif

#include "version.h"
#include "config.h"
#include "const.h"
#include "error.h"
#include "net.h"
#include "netclient.h"
#include "setup.h"
#include "packet.h"
#include "bit.h"
#include "paint.h"
#include "paintdata.h"
#include "xinit.h"
#include "pack.h"
#include "socklib.h"
#include "protoclient.h"
#include "portability.h"
#include "talk.h"

#ifdef	SOUND
#include "audio.h"
#endif

char netclient_version[] = VERSION;

#define TALK_RETRY	2
#define MAX_MAP_ACK_LEN	500

/*
 * Type definitions.
 */
typedef struct {
    long		loops;
    sockbuf_t		sbuf;
} frame_buf_t;

/*
 * Exported variables.
 */
setup_t			*Setup;
int			receive_window_size;
long			last_loops;
#ifdef	_WINDOWS
int			received_self = FALSE;
#endif
/*
 * Local variables.
 */
static sockbuf_t	rbuf,
			cbuf,
			wbuf;
static frame_buf_t	*Frames;
static int		(*receive_tbl[256])(void),
			(*reliable_tbl[256])(void);
static unsigned		magic;
static long		last_keyboard_change,
			last_keyboard_ack,
			last_keyboard_update,
			last_send_anything,
			reliable_offset,
			talk_pending,
			talk_sequence_num,
			talk_last_send;
static char		talk_str[MAX_CHARS];


/*
 * Initialize the function dispatch tables.
 * There are two tables.  One for the semi-important unreliable
 * data like frame updates.
 * The other one is for the reliable data stream, which is
 * received as part of the unreliable data packets.
 */
static void Receive_init(void)
{
    int i;

    for (i = 0; i < 256; i++) {
	receive_tbl[i] = NULL;
	reliable_tbl[i] = NULL;
    }

    receive_tbl[PKT_EYES]	= Receive_eyes;
    receive_tbl[PKT_TIME_LEFT]	= Receive_time_left;
    receive_tbl[PKT_AUDIO]	= Receive_audio;
    receive_tbl[PKT_START]	= Receive_start;
    receive_tbl[PKT_END]	= Receive_end;
    receive_tbl[PKT_SELF]	= Receive_self;
    receive_tbl[PKT_DAMAGED]	= Receive_damaged;
    receive_tbl[PKT_CONNECTOR]	= Receive_connector;
    receive_tbl[PKT_LASER]	= Receive_laser;
    receive_tbl[PKT_REFUEL]	= Receive_refuel;
    receive_tbl[PKT_SHIP]	= Receive_ship;
    receive_tbl[PKT_ECM]	= Receive_ecm;
    receive_tbl[PKT_TRANS]	= Receive_trans;
    receive_tbl[PKT_PAUSED]	= Receive_paused;
    receive_tbl[PKT_ITEM]	= Receive_item;
    receive_tbl[PKT_MINE]	= Receive_mine;
    receive_tbl[PKT_BALL]	= Receive_ball;
    receive_tbl[PKT_MISSILE]	= Receive_missile;
    receive_tbl[PKT_SHUTDOWN]	= Receive_shutdown;
    receive_tbl[PKT_DESTRUCT]	= Receive_destruct;
    receive_tbl[PKT_SELF_ITEMS]	= Receive_self_items;
    receive_tbl[PKT_FUEL]	= Receive_fuel;
    receive_tbl[PKT_CANNON]	= Receive_cannon;
    receive_tbl[PKT_TARGET]	= Receive_target;
    receive_tbl[PKT_RADAR]	= Receive_radar;
    receive_tbl[PKT_RELIABLE]	= Receive_reliable;
    receive_tbl[PKT_QUIT]	= Receive_quit;
    receive_tbl[PKT_MODIFIERS]  = Receive_modifiers;
    receive_tbl[PKT_FASTSHOT]	= Receive_fastshot;
    receive_tbl[PKT_THRUSTTIME] = Receive_thrusttime;
    receive_tbl[PKT_SHIELDTIME] = Receive_shieldtime;
    receive_tbl[PKT_PHASINGTIME]= Receive_phasingtime;
    receive_tbl[PKT_ROUNDDELAY] = Receive_rounddelay;
    receive_tbl[PKT_LOSEITEM]	= Receive_loseitem;
    receive_tbl[PKT_WRECKAGE]	= Receive_wreckage;
    for (i = 0; i < DEBRIS_TYPES; i++) {
	receive_tbl[PKT_DEBRIS + i] = Receive_debris;
    }

    reliable_tbl[PKT_MOTD]	= Receive_motd;
    reliable_tbl[PKT_MESSAGE]	= Receive_message;
    reliable_tbl[PKT_PLAYER]	= Receive_player;
    reliable_tbl[PKT_SCORE]	= Receive_score;
    reliable_tbl[PKT_TIMING]	= Receive_timing;
    reliable_tbl[PKT_LEAVE]	= Receive_leave;
    reliable_tbl[PKT_WAR]	= Receive_war;
    reliable_tbl[PKT_SEEK]	= Receive_seek;
    reliable_tbl[PKT_BASE]	= Receive_base;
    reliable_tbl[PKT_QUIT]	= Receive_quit;
    reliable_tbl[PKT_STRING]	= Receive_string;
    reliable_tbl[PKT_SCORE_OBJECT] = Receive_score_object;
    reliable_tbl[PKT_TALK_ACK]	= Receive_talk_ack;
}

/*
 * Uncompress the map which is compressed using a simple
 * Run-Length-Encoding algorithm.
 * The map object type is encoded in the lower seven bits
 * of a byte.
 * If the high bit of a byte is set then the next byte
 * means the number of contiguous map data bytes that
 * have the same type.  Otherwise only one map byte
 * has this type.
 * Because we uncompress the map backwards to save on
 * memory usage there is some complexity involved.
 */
static int Uncompress_map(void)
{
    u_byte	*cmp,		/* compressed map pointer */
		*ump,		/* uncompressed map pointer */
		*p;		/* temporary search pointer */
    int		i,
		count;

    if (Setup->map_order != SETUP_MAP_ORDER_XY) {
	errno = 0;
	error("Unknown map ordering in setup (%d)", Setup->map_order);
	return -1;
    }

    /* Point to last compressed map byte */
    cmp = Setup->map_data + Setup->map_data_len - 1;

    /* Point to last uncompressed map byte */
    ump = Setup->map_data + Setup->x * Setup->y - 1;

    while (cmp >= Setup->map_data) {
	for (p = cmp; p > Setup->map_data; p--) {
	    if ((p[-1] & SETUP_COMPRESSED) == 0) {
		break;
	    }
	}
	if (p == cmp) {
	    *ump-- = *cmp--;
	    continue;
	}
	if ((cmp - p) % 2 == 0) {
	    *ump-- = *cmp--;
	}
	while (p < cmp) {
	    count = *cmp--;
	    if (count < 2) {
		errno = 0;
		error("Map compress count error %d", count);
		return -1;
	    }
	    *cmp &= ~SETUP_COMPRESSED;
	    for (i = 0; i < count; i++) {
		*ump-- = *cmp;
	    }
	    cmp--;
	    if (ump < cmp) {
		errno = 0;
		error("Map uncompression error (%d,%d)",
		    cmp - Setup->map_data, ump - Setup->map_data);
		return -1;
	    }
	}
    }
    if (ump != cmp) {
	errno = 0;
	error("map uncompress error (%d,%d)",
	    cmp - Setup->map_data, ump - Setup->map_data);
	return -1;
    }
    Setup->map_order = SETUP_MAP_UNCOMPRESSED;
    return 0;
}


/* Get signed short and advance ptr */
static int get_short(char **ptr)
{
    *ptr += 2;
    return ((signed char)*(*ptr - 2) << 8) + (unsigned char)(*(*ptr - 1));
}

/* Unsigned version */
static unsigned int get_ushort(char **ptr)
{
    *ptr += 2;
    return ((unsigned char)*(*ptr - 2) << 8) + (unsigned char)*(*ptr - 1);
}

static int get_32bit(char **ptr)
{
    int res;

    res = get_ushort(ptr) << 16;
    return res + get_ushort(ptr);
}


static void Create_polygon_styles_for_testing ()
{
    int i;
    extern int Bitmap_add(const char *, int, bool);

    char *textureNames[] = {
        "texture0.ppm",
        "texture1.ppm",
        "texture2.ppm",
        "texture3.ppm",
        "texture4.ppm",
        "texture5.ppm",
        "texture6.ppm",
        "texture7.ppm",
    };


    /* a few polygon styles with a texture */
    for (i = 0; i < 8; i++) {
        polygon_styles[2 + i].visible = true;
        polygon_styles[2 + i].visible_in_radar = true;
        polygon_styles[2 + i].method = TEXTURED;
        polygon_styles[2 + i].def_edge_style = (i < 6) ? 0 : 1;
        polygon_styles[2 + i].texture =
            Bitmap_add(textureNames[i], 1, true);
    }
}

/*
 * Receive the map data and some game parameters from
 * the server.  The map data may be in compressed form.
 */
int Net_setup(void)
{
    int		n,
		len,
		size,
		done = 0,
		retries;
    long	todo = sizeof(setup_t);
    char	*ptr;

    if ((Setup = (setup_t *) malloc(sizeof(setup_t))) == NULL) {
	error("No memory for setup data");
	return -1;
    }
    ptr = (char *) Setup;
    while (todo > 0) {
	if (cbuf.ptr != cbuf.buf) {
	    Sockbuf_advance(&cbuf, cbuf.ptr - cbuf.buf);
	}
	len = cbuf.len;
	if (len > todo) {
	    len = todo;
	}
	if (len > 0) {
	    if (done == 0) {
		n = Packet_scanf(&cbuf,
				 "%ld" "%ld%hd" "%hd%hd" "%hd%hd" "%s%s",
				 &Setup->map_data_len,
				 &Setup->mode, &Setup->lives,
				 &Setup->width, &Setup->height,
				 &Setup->frames_per_second, &Setup->map_order,
				 Setup->name, Setup->author);
		if (Setup->map_order != SETUP_MAP_XY_WITH_LINES) {
		    oldServer = 1;
		    Setup->x = Setup->width; /* received size in blocks */
		    Setup->width *= BLOCK_SZ;
		    Setup->y = Setup->height;
		    Setup->height *= BLOCK_SZ;
		}
		else
		    oldServer = 0;
		if (n <= 0) {
		    errno = 0;
		    error("Can't read setup info from reliable data buffer");
		    return -1;
		}

		/*
		 * Do some consistency checks on the server setup structure.
		 */
		if (Setup->map_data_len <= 0
		    || Setup->width <= 0
		    || Setup->height <= 0
		    || Setup->map_data_len > Setup->x * Setup->y
		    && Setup->map_order != SETUP_MAP_XY_WITH_LINES) {
		    errno = 0;
		    error("Got bad map specs from server (%d,%d,%d)",
			Setup->map_data_len, Setup->x, Setup->y);
		    return -1;
		}
		if (oldServer && Setup->map_order != SETUP_MAP_ORDER_XY
		    && Setup->map_order != SETUP_MAP_UNCOMPRESSED) {
		    errno = 0;
		    error("Unknown map order type (%d)", Setup->map_order);
		    return -1;
		}
		size = sizeof(setup_t) + Setup->map_data_len;
		if (oldServer)
		    size = sizeof(setup_t) + Setup->x * Setup->y;
		if ((Setup = (setup_t *) realloc(ptr, size)) == NULL) {
		    error("No memory for setup and map");
		    return -1;
		}
		ptr = (char *) Setup;
		done = (char *) &Setup->map_data[0] - ptr;
		todo = Setup->map_data_len;
	    } else {
		memcpy(&ptr[done], cbuf.ptr, len);
		Sockbuf_advance(&cbuf, len + cbuf.ptr - cbuf.buf);
		done += len;
		todo -= len;
	    }
	}
	if (todo > 0) {
	    if (rbuf.ptr != rbuf.buf) {
		Sockbuf_advance(&rbuf, rbuf.ptr - rbuf.buf);
	    }
	    if (rbuf.len > 0) {
		if (rbuf.ptr[0] != PKT_RELIABLE) {
		    if (rbuf.ptr[0] == PKT_QUIT) {
			errno = 0;
			error("Server closed connection");
			return -1;
		    } else {
			errno = 0;
			error("Not a reliable packet (%d) in setup",
			    rbuf.ptr[0]);
			return -1;
		    }
		}
		if (Receive_reliable() == -1) {
		    return -1;
		}
		if (Sockbuf_flush(&wbuf) == -1) {
		    return -1;
		}
	    }
	    if (cbuf.ptr != cbuf.buf) {
		Sockbuf_advance(&cbuf, cbuf.ptr - cbuf.buf);
	    }
	    if (cbuf.len > 0) {
		continue;
	    }
	    for (retries = 0;; retries++) {
		if (retries >= 10) {
		    errno = 0;
		    error("Can't read setup after %d retries "
			  "(todo=%d, left=%d)",
			  retries, todo, cbuf.len - (cbuf.ptr - cbuf.buf));
		    return -1;
		}
		SetTimeout(2, 0);
		while (SocketReadable(rbuf.sock) > 0) {
		    Sockbuf_clear(&rbuf);
		    if (Sockbuf_read(&rbuf) == -1) {
			error("Can't read all setup data");
			return -1;
		    }
		    if (rbuf.len > 0) {
			break;
		    }
		    SetTimeout(0, 0);
		}
		if (rbuf.len > 0) {
		    break;
		}
	    }
	}
    }

    if (oldServer) {
	num_checks = 26; /* even if there really aren't any */
	checks = malloc(num_checks * sizeof(checkpoint_t));
    }
    else {
	int i, j, startx, starty, polyc, hidcount, nexthid;
	int dx, dy, cx, cy, pc;
        int *styles;
	xp_polygon_t poly;
	ipos *points, min, max;
	char *ptr, *hidptr;

	oldServer = 0;
	ptr = Setup->map_data;
	Setup->map_order = SETUP_MAP_UNCOMPRESSED;


        /* TODO Get these from server somehow */
        edge_styles = malloc(2 * sizeof(edge_style_t));
        if (edge_styles == NULL) {
            error("no memory for edge styles");
            return -1;
        }

        /* default edge style */
        edge_styles[0].width = 3;
        edge_styles[0].color = wallColor;
        edge_styles[0].style = LineSolid;

        /* hidden edge style */
        edge_styles[1].width = -1;

        polygon_styles = malloc(10 * sizeof(polygon_style_t));
        if (polygon_styles == NULL) {
            error("no memory for polygon styles");
            return -1;
        }

        /* default polygon style */
        polygon_styles[0].visible = true;
        polygon_styles[0].visible_in_radar = true;
        polygon_styles[0].method = NOFILL;
        polygon_styles[0].def_edge_style = 0;

        /* hidden polygon style */
        polygon_styles[1].visible = false;

	polyc = get_ushort(&ptr);
	for (i = 0; i < polyc; i++) {
	    dx = 0;
	    dy = 0;
	    hidcount = get_ushort(&ptr);
	    hidptr = ptr;
	    if (hidcount) {
		nexthid = get_ushort(&hidptr);
	    }
	    else
		nexthid = INT_MAX;
	    ptr += hidcount * 2;
	    pc = get_ushort(&ptr);
	    if ((points = malloc(pc * sizeof(ipos))) == NULL) {
		error("no memory for points");
		return -1;
	    }
            if (hidcount) {
                if ((styles = malloc(pc * sizeof(int))) == NULL) {
                    error("no memory for special edges");
                    return -1;
                }
            } else {
                styles = NULL;
            }
	    startx = get_ushort(&ptr);
	    starty = get_ushort(&ptr);
	    points[0].x = cx = min.x = max.x = startx;
	    points[0].y = cy = min.y = max.y = starty;

	    if (!nexthid) {
		styles[0] = 1; /* -1 means default */
		hidcount--;
		if (hidcount) {
		    nexthid = get_ushort(&hidptr);
		}
	    }
	    else {
		if (styles) styles[0] = -1;
            }

	    for (j = 1; j < pc; j++) {
		dx = get_short(&ptr);
		dy = get_short(&ptr);
		cx += dx;
		cy += dy;
		if (min.x > cx) min.x = cx;
		if (min.y > cy) min.y = cy;
		if (max.x < cx) max.x = cx;
		if (max.y < cy) max.y = cy;
		points[j].x = dx;
		points[j].y = dy;

		if (nexthid == j) {
		    styles[j] = 1;
		    hidcount--;
		    if (hidcount) {
			nexthid = get_ushort(&hidptr);
		    }
		}
		else {
                    if (styles) styles[j] = -1;
                }
	    }
	    poly.points = points;
            poly.edge_styles = styles;
            poly.style = 0;
	    poly.num_points = pc;
	    poly.bounds.x = min.x;
	    poly.bounds.y = min.y;
	    poly.bounds.w = max.x - min.x;
	    poly.bounds.h = max.y - min.y;
	    STORE(xp_polygon_t, polygons, num_polygons, max_polygons, poly);
	}
	num_bases = *ptr++;
	bases = (homebase_t *) malloc(num_bases * sizeof(homebase_t));
	if (bases == NULL) {
	    error("No memory for Map bases (%d)", num_bases);
	    exit(1);
	}
	for (i = 0; i < num_bases; i++) {
	    /* base.pos is not used */
	    bases[i].id = -1;
	    bases[i].team = *ptr++;
	    cx = get_ushort(&ptr);
	    cy = get_ushort(&ptr);
            bases[i].bounds.x = cx - BLOCK_SZ / 2;
            bases[i].bounds.y = cy - BLOCK_SZ / 2;
            bases[i].bounds.w = BLOCK_SZ;
            bases[i].bounds.h = BLOCK_SZ;
	    if (*ptr < 16)
		bases[i].type = SETUP_BASE_RIGHT;
	    else if (*ptr < 48)
		bases[i].type = SETUP_BASE_UP;
	    else if (*ptr < 80)
		bases[i].type = SETUP_BASE_LEFT;
	    else if (*ptr < 112)
		bases[i].type = SETUP_BASE_DOWN;
	    else
		bases[i].type = SETUP_BASE_RIGHT;
	    ptr++;
	}
	num_fuels = get_ushort(&ptr);
	if (num_fuels != 0) {
	    fuels = (fuelstation_t *)malloc(num_fuels * sizeof(fuelstation_t));
	    if (fuels == NULL) {
		error("No memory for Map fuels (%d)", num_fuels);
		exit(1);
	    }
	}
	for (i = 0; i < num_fuels; i++) {
	    cx = get_ushort(&ptr);
	    cy = get_ushort(&ptr);
	    fuels[i].fuel = MAX_STATION_FUEL;
            fuels[i].bounds.x = cx - BLOCK_SZ / 2;
            fuels[i].bounds.y = cy - BLOCK_SZ / 2;
            fuels[i].bounds.w = BLOCK_SZ;
            fuels[i].bounds.h = BLOCK_SZ;
	}
	num_checks = *ptr++;
	if (num_checks != 0) {

	    checks = malloc(num_checks * sizeof(checkpoint_t));
	    if (checks == NULL) {
		error("No memory for checkpoints (%d)", num_checks);
		exit(1);
	    }
	}
	for (i = 0; i < num_checks; i++) {
	    cx = get_ushort(&ptr);
	    cy = get_ushort(&ptr);
            checks[i].bounds.x = cx - BLOCK_SZ / 2;
            checks[i].bounds.y = cy - BLOCK_SZ / 2;
            checks[i].bounds.w = BLOCK_SZ;
            checks[i].bounds.h = BLOCK_SZ;
	}
	if (0) /* commented out to avoid problems outside testing */
	{
            extern int Mapdata_setup (const char *);

            if (Mapdata_setup(Setup->author)) {

                Create_polygon_styles_for_testing();

                for (i = 0; i < polyc; i++) {
                    int sid = *ptr++;
                    if (sid == 100 || sid < 0 || sid >= 9) sid = rand() % 9;
                    polygons[i].style = sid;
                }
            }
	}
	{
	    struct polystyle {
		int color;
		int texture_id;
		int defedge_id;
		int flags;
	    };

	    struct edgestyle {
		int width;
		int color;
		int style;
	    };

	    struct bmpstyle {
		char filename[30];
		int flags;
	    };

	    struct polydata {
		int style;
	    };

	    struct polystyle pstyles[256];
	    struct edgestyle estyles[256];
	    struct bmpstyle  bstyles[256];
	    struct polydata  pdata[1000];

	    int num_pstyles, num_estyles, num_bstyles;

	    num_pstyles = *ptr++;
	    num_estyles = *ptr++;
	    num_bstyles = *ptr++;
	    for (i = 0; i < num_pstyles; i++) {
		pstyles[i].color = get_32bit(&ptr);
		pstyles[i].texture_id = *ptr++;
		pstyles[i].defedge_id = *ptr++;
		pstyles[i].flags = *ptr++;
	    }
	    for (i = 0; i < num_estyles; i++) {
		estyles[i].width = *ptr++;
		estyles[i].color = get_32bit(&ptr);
		estyles[i].style = *ptr++;
	    }
	    for (i = 0; i < num_bstyles; i++) {
		strncpy(bstyles[i].filename, ptr,
			sizeof(bstyles[0].filename - 1));
		bstyles[i].filename[sizeof(bstyles[0].filename) - 1] = 0;
		ptr += strlen(bstyles[i].filename) + 1;
		bstyles[i].flags = *ptr++;
	    }
	    for (i = 0; i < polyc; i++) {
		pdata[i].style = *ptr++;
	    }
	}
    }

    if (Setup->map_order != SETUP_MAP_UNCOMPRESSED) {
	if (Uncompress_map() == -1) {
	    return -1;
	}
    }
    return 0;
}

/*
 * Send the first packet to the server with our name,
 * nick and display contained in it.
 * The server uses this data to verify that the packet
 * is from the right UDP connection, it already has
 * this info from the ENTER_GAME_pack.
 */
#define	MAX_VERIFY_RETRIES	5
int Net_verify(char *real, char *nick, char *disp, int my_team)
{
    int		n,
		type,
		result,
		retries;
    time_t	last;

    team = my_team;

    for (retries = 0;;) {
	if (retries == 0
	    || time(NULL) - last >= 3) {
	    if (retries++ >= MAX_VERIFY_RETRIES) {
		errno = 0;
		error("Can't connect to server after %d retries", retries);
		return -1;
	    }
	    Sockbuf_clear(&wbuf);
/*		IFWINDOWS( Trace("Verifying to sock=%d\n", wbuf.sock); ) */
	    n = Packet_printf(&wbuf, "%c%s%s%s", PKT_VERIFY, real, nick, disp);
	    if (n <= 0
		|| Sockbuf_flush(&wbuf) <= 0) {
		error("Can't send verify packet");
		return -1;
	    }
	    time(&last);
#ifndef SILENT
	    if (retries > 1) {
		printf("Waiting for verify response\n");
		IFWINDOWS( Progress("Waiting for verify response"); )
	    }
#endif
	}
	SetTimeout(1, 0);
	if (SocketReadable(rbuf.sock) == 0) {
	    continue;
	}
	Sockbuf_clear(&rbuf);
	if (Sockbuf_read(&rbuf) == -1) {
	    error("Can't read verify reply packet");
	    return -1;
	}
	if (rbuf.len <= 0) {
	    continue;
	}
	if (rbuf.ptr[0] != PKT_RELIABLE) {
	    if (rbuf.ptr[0] == PKT_QUIT) {
		errno = 0;
		error("Server closed connection");
		return -1;
	    } else {
		errno = 0;
		error("Bad packet type when verifying (%d)", rbuf.ptr[0]);
		return -1;
	    }
	}
	if (Receive_reliable() == -1) {
	    return -1;
	}
	if (Sockbuf_flush(&wbuf) == -1) {
	    return -1;
	}
	if (cbuf.len == 0) {
	    continue;
	}
	if (Receive_reply(&type, &result) <= 0) {
	    errno = 0;
	    error("Can't receive verify reply packet");
	    return -1;
	}
	if (type != PKT_VERIFY) {
	    errno = 0;
	    error("Verify wrong reply type (%d)", type);
	    return -1;
	}
	if (result != PKT_SUCCESS) {
	    errno = 0;
	    error("Verification failed (%d)", result);
	    return -1;
	}
	if (Receive_magic() <= 0) {
	    error("Can't receive magic packet after verify");
	    return -1;
	}
	break;
    }
#ifndef SILENT
    if (retries > 1) {
	printf("Verified correctly\n");
	IFWINDOWS( Progress("Verified correctly"); )
    }
#endif
    return 0;
}

/*
 * Open the datagram socket and allocate the network data
 * structures like buffers.
 * Currently there are three different buffers used:
 * 1) wbuf is used only for sending packets (write/printf).
 * 2) rbuf is used for receiving packets in (read/scanf).
 * 3) cbuf is used to copy the reliable data stream
 *    into from the raw and unreliable rbuf packets.
 */
int Net_init(char *server, int port)
{
    int			i,
			sock;
    unsigned		size;

#ifndef	_WINDOWS
    signal(SIGPIPE, SIG_IGN);
#endif

    Receive_init();

    if ((sock = CreateDgramSocket(0)) == -1) {
	error("Can't create datagram socket");
	return -1;
    }
    if (DgramConnect(sock, server, port) == -1) {
	error("Can't connect to server %s on port %d", server, port);
	DgramClose(sock);
	return -1;
    }
    wbuf.sock = sock;
    if (SetSocketNonBlocking(sock, 1) == -1) {
	error("Can't make socket non-blocking");
	return -1;
    }
    if (SetSocketSendBufferSize(sock, CLIENT_SEND_SIZE + 256) == -1) {
	error("Can't set send buffer size to %d", CLIENT_SEND_SIZE + 256);
    }
    if (SetSocketReceiveBufferSize(sock, CLIENT_RECV_SIZE + 256) == -1) {
	error("Can't set receive buffer size to %d", CLIENT_RECV_SIZE + 256);
    }

    size = receive_window_size * sizeof(frame_buf_t);
    if ((Frames = (frame_buf_t *) malloc(size)) == NULL) {
	error("No memory (%u)", size);
	return -1;
    }
    for (i = 0; i < receive_window_size; i++) {
	Frames[i].loops = 0;
	if (Sockbuf_init(&Frames[i].sbuf, sock, CLIENT_RECV_SIZE,
			 SOCKBUF_READ | SOCKBUF_DGRAM) == -1) {
	    error("No memory for read buffer (%u)", CLIENT_RECV_SIZE);
	    return -1;
	}
    }

    /* reliable data buffer, not a valid socket filedescriptor needed */
    if (Sockbuf_init(&cbuf, -1, CLIENT_RECV_SIZE,
		     SOCKBUF_WRITE | SOCKBUF_READ | SOCKBUF_LOCK) == -1) {
	error("No memory for control buffer (%u)", CLIENT_RECV_SIZE);
	return -1;
    }

    /* write buffer */
    if (Sockbuf_init(&wbuf, sock, CLIENT_SEND_SIZE,
		     SOCKBUF_WRITE | SOCKBUF_DGRAM) == -1) {
	error("No memory for write buffer (%u)", CLIENT_SEND_SIZE);
	return -1;
    }

    /* read buffer */
    rbuf = Frames[0].sbuf;

    /* reliable data byte stream offset */
    reliable_offset = 0;

    /* reset talk status */
    talk_sequence_num = 0;
    talk_pending = 0;

    return 0;
}

/*
 * Cleanup all the network buffers and close the datagram socket.
 * Also try to send the server a quit packet if possible.
 * Because this quit packet may get lost we send one at the
 * beginning and one at the end.
 */
void Net_cleanup(void)
{
    int		i,
		sock = wbuf.sock;
    char	ch;

    if (sock > 2) {
	ch = PKT_QUIT;
	if (DgramWrite(sock, &ch, 1) != 1) {
	    GetSocketError(sock);
	    DgramWrite(sock, &ch, 1);
	}
	micro_delay((unsigned)50*1000);
    }
    if (Frames != NULL) {
	for (i = 0; i < receive_window_size; i++) {
	    if (Frames[i].sbuf.buf != NULL) {
		Sockbuf_cleanup(&Frames[i].sbuf);
	    } else {
		break;
	    }
	}
	free(Frames);
	Frames = NULL;
    }
    Sockbuf_cleanup(&cbuf);
    Sockbuf_cleanup(&wbuf);
    if (Setup != NULL) {
	free(Setup);
	Setup = NULL;
    }
    if (sock > 2) {
	ch = PKT_QUIT;
	if (DgramWrite(sock, &ch, 1) != 1) {
	    GetSocketError(sock);
	    DgramWrite(sock, &ch, 1);
	}
	micro_delay((unsigned)50*1000);
	if (DgramWrite(sock, &ch, 1) != 1) {
	    GetSocketError(sock);
	    DgramWrite(sock, &ch, 1);
	}
	DgramClose(sock);
    }
}

/*
 * Calculate a new `keyboard-changed-id' which the server has to ack.
 */
void Net_key_change(void)
{
    last_keyboard_change++;
    Key_update();
}

/*
 * Flush the network output buffer if it has some data in it.
 * Called by the main loop before blocking on a select(2) call.
 */
int Net_flush(void)
{
    if (wbuf.len == 0) {
	wbuf.ptr = wbuf.buf;
	return 0;
    }
    if (last_keyboard_ack != last_keyboard_change) {
	/*
	 * Since 3.2.10: just call Key_update to add our keyboard vector.
	 * Key_update will call Send_keyboard to flush our buffer.
	 */
	return Key_update();
    }
    Send_talk();
    if (Sockbuf_flush(&wbuf) == -1) {
	return -1;
    }
    Sockbuf_clear(&wbuf);
    last_send_anything = last_loops;
    return 1;
}

/*
 * Return the socket filedescriptor for use in a select(2) call.
 */
int Net_fd(void)
{
    return rbuf.sock;
}

/*
 * Try to send a `start play' packet to the server and get an
 * acknowledgement from the server.  This is called after
 * we have initialized all our other stuff like the user interface
 * and we also have the map already.
 */
int Net_start(void)
{
    int			retries,
			type,
			result;
    time_t		last;

    for (retries = 0;;) {
	if (retries == 0
	    || (time(NULL) - last) > 1) {
	    if (retries++ >= 10) {
		errno = 0;
		error("Can't start play after %d retries", retries);
		return -1;
	    }
	    Sockbuf_clear(&wbuf);
	    /*
	     * Some networks have trouble receiving big packets.
	     * Therefore we don't transmit the shipshape when
	     * we have had 5 unsuccesful attempts.
	     */
	    if ((retries < 5 && Send_shape(shipShape) == -1)
		|| Packet_printf(&wbuf, "%c", PKT_PLAY) <= 0
		|| Client_power() == -1
#ifdef SOUND
		|| Send_audio_request(1) == -1
#endif
		|| Client_fps_request() == -1
		|| Sockbuf_flush(&wbuf) == -1) {
		error("Can't send start play packet");
		return -1;
	    }
	    time(&last);
	}
	if (cbuf.ptr > cbuf.buf) {
	    Sockbuf_advance(&cbuf, cbuf.ptr - cbuf.buf);
	}
	SetTimeout(2, 0);
	while (cbuf.len <= 0
	    && SocketReadable(rbuf.sock) != 0) {
	    Sockbuf_clear(&rbuf);
	    if (Sockbuf_read(&rbuf) == -1) {
		error("Error reading play reply");
		return -1;
	    }
	    if (rbuf.len <= 0) {
		continue;
	    }
	    if (rbuf.ptr[0] != PKT_RELIABLE) {
		if (rbuf.ptr[0] == PKT_QUIT) {
		    errno = 0;
		    error("Server closed connection");
		    return -1;
		}
		else if (rbuf.ptr[0] == PKT_START) {
		    /*
		     * Packet out of order, drop it or...
		     * Skip the frame and check for a Reliable Data Packet.
		     * (HACK)
		     * In a future version we may not want a reply to
		     * the PKT_PLAY request and accept frames immediately.
		     */
		    while (++rbuf.ptr < rbuf.buf + rbuf.len) {
			if (rbuf.ptr[0] == PKT_END
			    && rbuf.ptr + 5 < rbuf.buf + rbuf.len
			    && rbuf.ptr[5] == PKT_RELIABLE) {
			    rbuf.ptr += 5;
			    break;
			}
		    }
		    if (rbuf.ptr + 11 >= rbuf.buf + rbuf.len) {
			printf("skipping unexpected frame while starting\n");
			continue;
		    }
		    printf("abusing unexpected frame while starting\n");
		} else {
		    printf("strange packet type while starting (%d)\n",
			rbuf.ptr[0]);
		    /*
		     * What the hack do we care when we wanna play.
		     * Just drop the packet for now.
		     */
		    Sockbuf_clear(&rbuf);
		    continue;
		}
	    }
	    if (Receive_reliable() == -1) {
		return -1;
	    }
	    if (Sockbuf_flush(&wbuf) == -1) {
		return -1;
	    }
	}
	if (cbuf.ptr - cbuf.buf >= cbuf.len) {
	    continue;
	}
	if (cbuf.ptr[0] != PKT_REPLY) {
	    errno = 0;
	    error("Not a reply packet after play (%d,%d,%d)",
		cbuf.ptr[0], cbuf.ptr - cbuf.buf, cbuf.len);
	    return -1;
	}
	if (Receive_reply(&type, &result) <= 0) {
	    errno = 0;
	    error("Can't receive reply packet after play");
	    return -1;
	}
	if (type != PKT_PLAY) {
	    errno = 0;
	    error("Can't receive reply packet after play");
	    return -1;
	}
	if (result != PKT_SUCCESS) {
	    errno = 0;
	    error("Start play not allowed (%d)", result);
	    return -1;
	}
	break;
    }
    packet_measure = NULL;
    Net_init_measurement();
    errno = 0;
    return 0;
}

void Net_init_measurement(void)
{
    packet_loss = 0;
    packet_drop = 0;
    packet_loop = 0;
    if (BIT(instruments, SHOW_PACKET_LOSS_METER|SHOW_PACKET_DROP_METER) != 0) {
	if (packet_measure == NULL) {
	    if ((packet_measure = (char *) malloc(FPS)) == NULL) {
		error("No memory for packet measurement");
		CLR_BIT(instruments,
			SHOW_PACKET_LOSS_METER|SHOW_PACKET_DROP_METER);
	    } else {
		memset(packet_measure, PACKET_DRAW, FPS);
	    }
	}
    }
    else if (packet_measure != NULL) {
	free(packet_measure);
	packet_measure = NULL;
    }
}

/*
 * Process a packet which most likely is a frame update,
 * perhaps with some reliable data in it.
 */
static int Net_packet(void)
{
    int		type,
		prev_type = 0,
		result,
		replyto,
		status;

    while (rbuf.buf + rbuf.len > rbuf.ptr) {
	type = (*rbuf.ptr & 0xFF);

	if (receive_tbl[type] == NULL) {
	    errno = 0;
	    IFNWINDOWS(error("Received unknown packet type (%d, %d), dropping frame.",
			type, prev_type);)
	    Sockbuf_clear(&rbuf);
	    break;
	}
	else if ((result = (*receive_tbl[type])()) <= 0) {
	    if (result == -1) {
		if (type != PKT_QUIT) {
		    errno = 0;
		    error("Processing packet type (%d, %d) failed", type, prev_type);
		}
		return -1;
	    }
	    /* Drop rest of incomplete packet */
	    Sockbuf_clear(&rbuf);
	    break;
	}
	prev_type = type;
    }
    while (cbuf.buf + cbuf.len > cbuf.ptr) {
	type = (*cbuf.ptr & 0xFF);
	if (type == PKT_REPLY) {
	    if ((result = Receive_reply(&replyto, &status)) <= 0) {
		if (result == 0) {
		    break;
		}
		return -1;
	    }
	    /* should do something more appropriate than this with the reply */
	    errno = 0;
	    error("Got reply packet (%d,%d)", replyto, status);
	}
	else if (reliable_tbl[type] == NULL) {
	    int i;
	    errno = 0;
	    IFNWINDOWS(error("Received unknown reliable data packet type (%d,%d,%d)",
					type, cbuf.ptr - cbuf.buf, cbuf.len);)
	    printf("\tdumping buffer for debugging:\n");
	    for (i = 0; i < cbuf.len; i++) {
		printf("%3d", cbuf.buf[i] & 0xFF);
		if (i % 20 == 0) {
		    printf("\n");
		} else {
		    printf(" ");
		}
	    }
	    printf("\n");
	    return -1;
	}
	else if ((result = (*reliable_tbl[type])()) <= 0) {
	    if (result == 0) {
		break;
	    }
	    return -1;
	}
    }

    return 0;
}

/*
 * Do some (simple) packet loss/drop measurement
 * the results of which can be drawn on the display.
 * This is mainly for debugging and analysis.
 */
static void Net_measurement(long loop, int status)
{
    int		i;
    long	delta;

    if (packet_measure == NULL) {
	return;
    }
    if ((delta = loop - packet_loop) < 0) {
	/*
	 * Duplicate or out of order.
	 */
	return;
    }
    if (delta >= FPS) {
	if (packet_loop == 0) {
	    packet_loop = loop - (loop % FPS);
	    return;
	}
	packet_loop = loop - (loop % FPS);
	packet_loss = 0;
	packet_drop = 0;
	for (i = 0; i < FPS; i++) {
	    switch (packet_measure[i]) {
	    case PACKET_LOSS:
		packet_loss++;
		continue;
	    case PACKET_DROP:
		packet_drop++;
		break;
	    }
	    packet_measure[i] = PACKET_LOSS;
	}
	delta = loop - packet_loop;
    }
   if (packet_measure[(int)delta] != PACKET_DRAW) {
       packet_measure[(int)delta] = status;
   }
}

/*
 * Read a packet into one of the input buffers.
 * If it is a frame update then we check to see
 * if it is an old or duplicate one.  If it isn't
 * a new frame then the packet is discarded and
 * we retry to read a packet once more.
 * It's a non-blocking read.
 */
static int Net_read(frame_buf_t *frame)
{
    int		n;
    long	loop;
    u_byte	ch;

    frame->loops = 0;
    for (;;) {
	Sockbuf_clear(&frame->sbuf);
	if (Sockbuf_read(&frame->sbuf) == -1) {
	    error("Net input error");
	    return -1;
	}
	if (frame->sbuf.len <= 0) {
	    Sockbuf_clear(&frame->sbuf);
	    return 0;
	}
	/*IFWINDOWS( Trace("Net_read: read %d bytes type=%d\n", frame->sbuf.len, frame->sbuf.ptr[0]); ) */
	if (frame->sbuf.ptr[0] != PKT_START) {
	    /*
	     * Don't know which type of packet this is
	     * and if it contains a frame at all (not likely).
	     * It could be a quit packet.
	     */
	    return 1;
	}
	/* Peek at the frame loop number. */
	n = Packet_scanf(&frame->sbuf, "%c%ld", &ch, &loop);
	/*IFWINDOWS( Trace("Net_read: frame # %d\n", loop); )*/
	frame->sbuf.ptr = frame->sbuf.buf;
	if (n <= 0) {
	    if (n == -1) {
		Sockbuf_clear(&frame->sbuf);
		return -1;
	    }
	    continue;
	}
	else if (loop > last_loops) {
	    frame->loops = loop;
	    return 2;
	} else {
	    /*
	     * Packet out of order.  Drop it.
	     * We may have already drawn it if it is duplicate.
	     * Perhaps we should try to extract any reliable data
	     * from it before dropping it.
	     */
	}
    }
	/*IFWINDOWS( Trace("Net_read: wbuf->len=%d\n", wbuf.len); )*/
}

/*
 * Read frames from the net until there are no more available.
 * If the server has floaded us with frame updates then we should
 * discard everything except the most recent ones.  The X server
 * may be too slow to keep up with the rate of the XPilot server
 * or there may have been a network hickup if the net is overloaded.
 */
int Net_input(void)
{
    int		i,
		j,
		n;
    frame_buf_t	*frame,
		*last_frame,
		*oldest_frame = &Frames[0],
		tmpframe;

    for (i = 0; i < receive_window_size; i++) {
	frame = &Frames[i];
	if (!frame)
		continue;
	if (frame->loops != 0) {
	    /*
	     * Already contains a frame.
	     */
	    if (frame->loops < oldest_frame->loops
		|| oldest_frame->loops == 0) {
		oldest_frame = frame;
	    }
	}
	else if (frame->sbuf.len > 0
	    && frame->sbuf.ptr == frame->sbuf.buf) {
	    /*
	     * Contains an unidentifiable packet.
	     * No more input until this one is processed.
	     */
	    break;
	} else {
	    /*
	     * Empty buffer.  Read a frame.
	     */
	    if ((n = Net_read(frame)) <= 0) {
		if (n == 0) {
		    /*
		     * No more new packets available.
		     */
		    if (i == 0) {
			/*
			 * No frames to be processed.
			 */
			return 0;
		    }
		    break;
		} else {
		    return n;
		}
	    }
	    else if (n == 1) {
		/*
		 * Contains an unidentifiable packet.
		 * No more input until this one is processed.
		 */
		break;
	    } else {
		/*
		 * Check for duplicate packets.
		 */
		for (j = i - 1; j >= 0; j--) {
		    if (frame->loops == Frames[j].loops) {
			break;
		    }
		}
		if (j >= 0) {
		    /*
		     * Duplicate.  Drop it.
		     */
		    Net_measurement(frame->loops, PACKET_DROP);
		    Sockbuf_clear(&frame->sbuf);
		    frame->loops = 0;
		    i--;	/* correct for for loop increment. */
		    continue;
		}
		if (frame->loops < oldest_frame->loops) {
		    oldest_frame = frame;
		}
	    }
	}
	if ((i == receive_window_size - 1 && i > 0)
#ifdef	_WINDOWS
		|| drawPending
		|| (ThreadedDraw &&
				!WaitForSingleObject(dinfo.eventNotDrawing, 0) == WAIT_OBJECT_0)
#endif
		) {
	    /*
	     * Drop oldest packet.
	     */
	    if (oldest_frame->loops < frame->loops) {
		/*
		 * Switch buffers to prevent gaps.
		 */
		tmpframe = *oldest_frame;
		*oldest_frame = *frame;
		*frame = tmpframe;
	    }
	    Net_measurement(frame->loops, PACKET_DROP);
	    Sockbuf_clear(&frame->sbuf);
	    frame->loops = 0;
	    oldest_frame = &Frames[0];
	    /*
	     * Reset loop index.
	     */
	    i = -1;	/* correct for for loop increment. */
	    continue;
	}
    }

    /*
     * Find oldest packet.
     */
    last_frame = oldest_frame = &Frames[0];
    for (i = 1; i < receive_window_size; i++, last_frame++) {
	frame = &Frames[i];
	if (frame->loops == 0) {
	    if (frame->sbuf.len > 0) {
		/*
		 * This is an unidentifiable packet.
		 * Process it last, because it arrived last.
		 */
		continue;
	    } else {
		/*
		 * Empty.  The rest should be empty too,
		 * because we have taken care not to have gaps.
		 */
		break;
	    }
	}
	else if (frame->loops < oldest_frame->loops
	    || oldest_frame->loops == 0) {
	    oldest_frame = frame;
	}
    }

    if (oldest_frame->sbuf.len <= 0) {
	/*
	 * Couldn't find a non-empty packet.
	 */
	if (oldest_frame->loops > 0) {
	    errno = 0;
	    error("bug %s,%d", __FILE__, __LINE__);
	    oldest_frame->loops = 0;
	}
	return 0;
    }

    /*
     * Let the packet processing routines know which
     * packet they should process.
     */
    rbuf = oldest_frame->sbuf;

    /*
     * Process the packet.
     */
    n = Net_packet();

    if (last_frame > oldest_frame) {
	/*
	 * Switch buffers to prevent gaps.
	 */
	tmpframe = *oldest_frame;
	*oldest_frame = *last_frame;
	*last_frame = tmpframe;
    }
    Sockbuf_clear(&last_frame->sbuf);
    last_frame->loops = 0;
    rbuf = last_frame->sbuf;

    if (n == -1) {
	return -1;
    }

    /*
     * If the server hasn't yet acked our last keyboard change
     * and we haven't updated it in the (previous) current time frame
     * or if we haven't sent anything for a while (keepalive)
     * then we send our current keyboard state.
     */
    if ((last_keyboard_ack != last_keyboard_change
	    && last_keyboard_update /*+ 1*/ < last_loops)
	|| last_loops - last_send_anything > 5 * Setup->frames_per_second) {
	Key_update();
	last_send_anything = last_loops;
    }

    return 1 + (last_frame > oldest_frame);
}

/*
 * Receive the beginning of a new frame update packet,
 * which contains the loops number.
 */
int Receive_start(void)
{
    int		n;
    long	loops;
    u_byte	ch;
    long	key_ack;

    if ((n = Packet_scanf(&rbuf,
			  "%c%ld%ld",
			  &ch, &loops, &key_ack)) <= 0) {
	return n;
    }
    if (last_loops >= loops) {
	/*
	 * Packet is duplicate or out of order.
	 */
	Net_measurement(loops, PACKET_DROP);
	printf("ignoring frame (%ld)\n", last_loops - loops);
	return 0;
    }
    last_loops = loops;
    if (key_ack > last_keyboard_ack) {
	if (key_ack > last_keyboard_change) {
	    printf("Premature keyboard ack by server (%ld,%ld,%ld)\n",
		   last_keyboard_change, last_keyboard_ack, key_ack);
	    /*
	     * Packet could be corrupt???
	     * Some blokes turn off checksumming.
	     */
	    return 0;
	}
	else {
	    last_keyboard_ack = key_ack;
	}
    }
    if ((n = Handle_start(loops)) == -1) {
	return -1;
    }
    return 1;
}

/*
 * Receive the end of a new frame update packet,
 * which should contain the same loops number
 * as the frame head.  If this terminating packet
 * is missing then the packet is corrupt or incomplete.
 */
int Receive_end(void)
{
    int		n;
    long	loops;
    u_byte	ch;

    if ((n = Packet_scanf(&rbuf, "%c%ld", &ch, &loops)) <= 0) {
	return n;
    }
    Net_measurement(loops, PACKET_DRAW);
    if ((n = Handle_end(loops)) == -1) {
	return -1;
    }
    return 1;
}

/*
 * Receive a message string.  This currently is rather
 * inefficiently encoded as an ascii string.
 */
int Receive_message(void)
{
    int		n;
    u_byte	ch;
    char	msg[MSG_LEN];

    if ((n = Packet_scanf(&cbuf, "%c%S", &ch, msg)) <= 0) {
	return n;
    }
    if ((n = Handle_message(msg)) == -1) {
	return -1;
    }
    return 1;
}

/*
 * Receive the remaining playing time.
 */
int Receive_time_left(void)
{
    int		n;
    u_byte	ch;
    long	sec;

    if ((n = Packet_scanf(&rbuf, "%c%ld", &ch, &sec)) <= 0) {
	return n;
    }
    if ((n = Handle_time_left(sec)) == -1) {
	return -1;
    }
    return 1;
}

/*
 * Receive the id of the player we get frame updates for (game over mode).
 */
int Receive_eyes(void)
{
    int			n,
			id;
    u_byte		ch;

    if ((n = Packet_scanf(&rbuf, "%c%hd", &ch, &id)) <= 0) {
	return n;
    }
    if ((n = Handle_eyes(id)) == -1) {
	return -1;
    }
    return 1;
}

/*
 * Receive the server MOTD.
 */
int Receive_motd(void)
{
    u_byte		ch;
    long		off,
			size;
    short		len;
    int			n;
    char		*cbuf_ptr = cbuf.ptr;

    if ((n = Packet_scanf(&cbuf,
			  "%c%ld%hd%ld",
			  &ch, &off, &len, &size)) <= 0) {
	return n;
    }
    if (cbuf.ptr + len > &cbuf.buf[cbuf.len]) {
	cbuf.ptr = cbuf_ptr;
	return 0;
    }
    Handle_motd(off, cbuf.ptr, (int)len, size);
    cbuf.ptr += len;

    return 1;
}

/*
 * Ask the server to send us the server MOTD.
 */
int Net_ask_for_motd(long offset, long maxlen)
{
    if (offset < 0 || maxlen <= 0) {
	errno = 0;
	error("Bad motd request (%ld, %ld)", offset, maxlen);
	return -1;
    }
    if (Packet_printf(&wbuf, "%c%ld%ld", PKT_MOTD, offset, maxlen) <= 0) {
	errno = 0;
	error("Can't ask motd");
	return -1;
    }

    return 0;
}


static void Check_view_dimensions(void)
{
    int			width_wanted = draw_width;
    int			height_wanted = draw_height;
    int			srv_width, srv_height;

#ifdef	WINDOWSCALING
    width_wanted = (int)(width_wanted * scaleFactor + 0.5);
    height_wanted = (int)(height_wanted * scaleFactor + 0.5);
#endif

    srv_width = width_wanted;
    srv_height = height_wanted;
    LIMIT(srv_height, MIN_VIEW_SIZE, MAX_VIEW_SIZE);
    LIMIT(srv_width, MIN_VIEW_SIZE, MAX_VIEW_SIZE);
    if (view_width != srv_width ||
	view_height != srv_height) {
	Send_display();
    }

    real_view_width = view_width;
    real_view_height = view_height;
    view_x_offset = 0;
    view_y_offset = 0;
    if (width_wanted > view_width) {
	view_width = width_wanted;
	view_x_offset = (width_wanted - real_view_width) / 2;
    }
    if (height_wanted > view_height) {
	view_height = height_wanted;
	view_y_offset = (height_wanted - real_view_height) / 2;
    }
}


/*
 * Receive the packet with counts for all the items.
 * New since pack version 4203.
 */
int Receive_self_items(void)
{
    unsigned		mask;
    int			i, n;
    u_byte		ch;
    char		*rbuf_ptr_start = rbuf.ptr;
    u_byte		num_items[NUM_ITEMS];

    n = Packet_scanf(&rbuf, "%c%u", &ch, &mask);
    if (n <= 0) {
	return n;
    }
    memset(num_items, 0, sizeof num_items);
    for (i = 0; mask != 0; i++) {
	if (mask & (1 << i)) {
	    mask ^= (1 << i);
	    if (rbuf.ptr - rbuf.buf < rbuf.len) {
		if (i < NUM_ITEMS) {
		    num_items[i] = *rbuf.ptr++;
		} else {
		    rbuf.ptr++;
		}
	    }
	}
    }
    Handle_self_items(num_items);
    return (rbuf.ptr - rbuf_ptr_start);
}

/*
 * Receive the packet with all player information for the HUD.
 * If this packet is missing from the frame update then the player
 * isn't actively playing, which means he's either damaged, dead,
 * paused or has game over.
 */
int Receive_self(void)
{
    int		n;
    short	x, y, vx, vy, lockId, lockDist,
		fuelSum, fuelMax;
    u_byte	ch, heading, power, turnspeed, turnresistance,
		nextCheckPoint, lockDir, autopilotLight, currentTank, stat;
    u_byte	num_items[NUM_ITEMS];

    n = Packet_scanf(&rbuf,
		     "%c"
		     "%hd%hd%hd%hd%c"
		     "%c%c%c"
		     "%hd%hd%c%c"
		     ,
		     &ch,
		     &x, &y, &vx, &vy, &heading,
		     &power, &turnspeed, &turnresistance,
		     &lockId, &lockDist, &lockDir, &nextCheckPoint);
    if (n <= 0) {
	return n;
    }

    if (version >= 0x4203) {
	memset(num_items, 0, sizeof num_items);
    }
    else {
	n = Packet_scanf(&rbuf,
			 "%c%c%c%c%c"
			 "%c%c%c%c%c"
			 "%c%c%c%c"
			 ,
			 &(num_items[ITEM_CLOAK]),
			 &(num_items[ITEM_SENSOR]),
			 &(num_items[ITEM_MINE]),
			 &(num_items[ITEM_MISSILE]),
			 &(num_items[ITEM_ECM]),

			 &(num_items[ITEM_TRANSPORTER]),
			 &(num_items[ITEM_WIDEANGLE]),
			 &(num_items[ITEM_REARSHOT]),
			 &(num_items[ITEM_AFTERBURNER]),
			 &(num_items[ITEM_TANK]),

			 &(num_items[ITEM_LASER]),
			 &(num_items[ITEM_EMERGENCY_THRUST]),
			 &(num_items[ITEM_TRACTOR_BEAM]),
			 &(num_items[ITEM_AUTOPILOT]));
	if (n <= 0) {
	    return n;
	}
    }
    n = Packet_scanf(&rbuf,
		     "%c%hd%hd"
		     "%hd%hd%c"
		     "%c%c"
		     ,
		     &currentTank, &fuelSum, &fuelMax,
		     &view_width, &view_height, &debris_colors,
		     &stat, &autopilotLight
		     );
    if (n <= 0) {
	return n;
    }

    if (version < 0x4203) {
	if (version >= 0x3720) {
	    n = Packet_scanf(&rbuf, "%c%c%c%c",
			     &(num_items[ITEM_EMERGENCY_SHIELD]),
			     &(num_items[ITEM_DEFLECTOR]),
			     &(num_items[ITEM_HYPERJUMP]),
			     &(num_items[ITEM_PHASING])
			     );
	    if (n <= 0) {
		return n;
	    }
	    if (version >= 0x4100) {
		n = Packet_scanf(&rbuf, "%c", &(num_items[ITEM_MIRROR]));
		if (n <= 0) {
		    return n;
		}
		if (version >= 0x4201) {
		    n = Packet_scanf(&rbuf, "%c", &(num_items[ITEM_ARMOR]));
		    if (n <= 0) {
			return n;
		    }
		} else {
		    num_items[ITEM_ARMOR] = 0;
		}
	    }
	    else {
		num_items[ITEM_MIRROR] = 0;
		num_items[ITEM_ARMOR] = 0;
	    }
	}
	else {
	    if (version >= 0x3200) {
		n = Packet_scanf(&rbuf, "%c",
				 &(num_items[ITEM_EMERGENCY_SHIELD]));
		if (n <= 0) {
		    return n;
		}
	    }
	    else {
		num_items[ITEM_EMERGENCY_SHIELD] = 0;
	    }
	    num_items[ITEM_DEFLECTOR] = 0;
	    num_items[ITEM_HYPERJUMP] = 0;
	    num_items[ITEM_PHASING] = 0;
	    num_items[ITEM_MIRROR] = 0;
	    num_items[ITEM_ARMOR] = 0;
	}
    }

    if (debris_colors > num_spark_colors) {
	debris_colors = num_spark_colors;
    }

    Check_view_dimensions();

    Game_over_action(stat);
    Handle_self(x, y, vx, vy, heading,
		(float) power,
		(float) turnspeed,
		(float) turnresistance / 255.0F,
		lockId, lockDist, lockDir,
		nextCheckPoint, autopilotLight,
		num_items,
		currentTank, fuelSum, fuelMax, rbuf.len);

#ifdef	_WINDOWS
	received_self = TRUE;
#endif
    return 1;
}

int Receive_modifiers(void)
{
    int		n;
    char	mods[MAX_CHARS];
    u_byte	ch;

    if ((n = Packet_scanf(&rbuf, "%c%s", &ch, mods)) <= 0) {
	return n;
    }
    if ((n = Handle_modifiers(mods)) == -1)
	return -1;
    return 1;
}

int Receive_refuel(void)
{
    int		n;
    short	x0, y0, x1, y1;
    u_byte	ch;

    if ((n = Packet_scanf(&rbuf, "%c%hd%hd%hd%hd",
			  &ch, &x0, &y0, &x1, &y1)) <= 0) {
	return n;
    }
    if ((n = Handle_refuel(x0, y0, x1, y1)) == -1) {
	return -1;
    }
    return 1;
}

int Receive_connector(void)
{
    int		n;
    short	x0, y0, x1, y1;
    u_byte	ch, tractor;

    n = Packet_scanf(&rbuf, "%c%hd%hd%hd%hd%c",
		     &ch, &x0, &y0, &x1, &y1, &tractor);
    if (n <= 0)
	return n;
    if ((n = Handle_connector(x0, y0, x1, y1, tractor)) == -1) {
	return -1;
    }
    return 1;
}

int Receive_laser(void)
{
    int		n;
    short	x, y, len;
    u_byte	ch, color, dir;

    if ((n = Packet_scanf(&rbuf, "%c%c%hd%hd%hd%c",
			  &ch, &color, &x, &y, &len, &dir)) <= 0) {
	return n;
    }
    if ((n = Handle_laser(color, x, y, len, dir)) == -1) {
	return -1;
    }
    return 1;
}

int Receive_missile(void)
{
    int		n;
    short	x, y;
    u_byte	ch, dir, len;

    if ((n = Packet_scanf(&rbuf, "%c%hd%hd%c%c", &ch, &x, &y, &len, &dir)) <= 0) {
	return n;
    }
    if ((n = Handle_missile(x, y, len, dir)) == -1) {
	return -1;
    }
    return 1;
}

int Receive_ball(void)
{
    int		n;
    short	x, y, id;
    u_byte	ch;

    if ((n = Packet_scanf(&rbuf, "%c%hd%hd%hd", &ch, &x, &y, &id)) <= 0) {
	return n;
    }
    if ((n = Handle_ball(x, y, id)) == -1) {
	return -1;
    }
    return 1;
}

int Receive_ship(void)
{
    int		n, shield, cloak, eshield, phased, deflector;
    short	x, y, id;
    u_byte	ch, dir, flags;

    if ((n = Packet_scanf(&rbuf,
			  "%c%hd%hd%hd" "%c%c",
			  &ch, &x, &y, &id,
			  &dir, &flags)) <= 0) {
	return n;
    }
    shield = ((flags & 1) != 0);
    cloak = ((flags & 2) != 0);
    eshield = ((flags & 4) != 0);
    phased = ((flags & 8) != 0);
    deflector = ((flags & 0x10) != 0);

    if ((n = Handle_ship(x, y, id, dir, shield, cloak, eshield, phased, deflector)) == -1) {
	return -1;
    }
    return 1;
}

int Receive_mine(void)
{
    int		n;
    short	x, y, id;
    u_byte	ch, teammine;

    n = Packet_scanf(&rbuf, "%c%hd%hd%c%hd", &ch, &x, &y, &teammine, &id);
    if (n <= 0) {
	return n;
    }
    if ((n = Handle_mine(x, y, teammine, id)) == -1) {
	return -1;
    }
    return 1;
}

int Receive_item(void)
{
    int		n;
    short	x, y;
    u_byte	ch, type;

    if ((n = Packet_scanf(&rbuf, "%c%hd%hd%c", &ch, &x, &y, &type)) <= 0) {
	return n;
    }
    if (type < NUM_ITEMS) {
	if ((n = Handle_item(x, y, type)) == -1) {
	    return -1;
	}
    }
    return 1;
}

int Receive_destruct(void)
{
    int		n;
    short	count;
    u_byte	ch;

    if ((n = Packet_scanf(&rbuf, "%c%hd", &ch, &count)) <= 0) {
	return n;
    }
    if ((n = Handle_destruct(count)) == -1) {
	return -1;
    }
    return 1;
}

int Receive_shutdown(void)
{
    int		n;
    short	count, delay;
    u_byte	ch;

    if ((n = Packet_scanf(&rbuf, "%c%hd%hd", &ch, &count, &delay)) <= 0) {
	return n;
    }
    if ((n = Handle_shutdown(count, delay)) == -1) {
	return -1;
    }
    return 1;
}

int Receive_thrusttime(void)
{
    int		n;
    short	count, max;
    u_byte	ch;

    if ((n = Packet_scanf(&rbuf, "%c%hd%hd", &ch, &count, &max)) <= 0) {
	return n;
    }
    if ((n = Handle_thrusttime(count, max)) == -1) {
	return -1;
    }
    return 1;
}

int Receive_shieldtime(void)
{
    int		n;
    short	count, max;
    u_byte	ch;

    if ((n = Packet_scanf(&rbuf, "%c%hd%hd", &ch, &count, &max)) <= 0) {
	return n;
    }
    if ((n = Handle_shieldtime(count, max)) == -1) {
	return -1;
    }
    return 1;
}

int Receive_phasingtime(void)
{
    int		n;
    short	count, max;
    u_byte	ch;

    if ((n = Packet_scanf(&rbuf, "%c%hd%hd", &ch, &count, &max)) <= 0) {
	return n;
    }
    if ((n = Handle_phasingtime(count, max)) == -1) {
	return -1;
    }
    return 1;
}

int Receive_rounddelay(void)
{
    int		n;
    short	count, max;
    u_byte	ch;

    if ((n = Packet_scanf(&rbuf, "%c%hd%hd", &ch, &count, &max)) <= 0) {
	return n;
    }
    if ((n = Handle_rounddelay(count, max)) == -1) {
	return -1;
    }
    return 1;
}

int Receive_fastshot(void)
{
    int			n, r, type;

    rbuf.ptr++;	/* skip PKT_FASTSHOT packet id */

    if (rbuf.ptr - rbuf.buf + 2 >= rbuf.len) {
	return 0;
    }
    type = (*rbuf.ptr++ & 0xFF);
    n = (*rbuf.ptr++ & 0xFF);
    if (rbuf.ptr - rbuf.buf + (n * 2) > rbuf.len) {
	return 0;
    }
    r = Handle_fastshot(type, (u_byte*)rbuf.ptr, n);
    rbuf.ptr += n * 2;

    return (r == -1) ? -1 : 1;
}

int Receive_debris(void)
{
    int			n, r, type;

    if (rbuf.ptr - rbuf.buf + 2 >= rbuf.len) {
	return 0;
    }
    type = (*rbuf.ptr++ & 0xFF);
    n = (*rbuf.ptr++ & 0xFF);
    if (rbuf.ptr - rbuf.buf + (n * 2) > rbuf.len) {
	return 0;
    }
    r = Handle_debris(type - PKT_DEBRIS, (u_byte*)rbuf.ptr, n);
    rbuf.ptr += n * 2;

    return (r == -1) ? -1 : 1;
}

int Receive_wreckage(void)	/* since 3.8.0 */
{
    int			n;
    short		x, y;
    u_byte		ch, wrecktype, size, rot;

    if ((n = Packet_scanf(&rbuf, "%c%hd%hd%c%c%c", &ch, &x, &y,
			  &wrecktype, &size, &rot)) <= 0) {
	return n;
    }
    if (version < 0x4202) {
	/* always color red. */
	wrecktype &= 0x7F;
    }
    if ((n = Handle_wreckage(x, y, wrecktype, size, rot)) == -1) {
	return -1;
    }
    return 1;
}

int Receive_ecm(void)
{
    int			n;
    short		x, y, size;
    u_byte		ch;

    if ((n = Packet_scanf(&rbuf, "%c%hd%hd%hd", &ch, &x, &y, &size)) <= 0) {
	return n;
    }
    if ((n = Handle_ecm(x, y, size)) == -1) {
	return -1;
    }
    return 1;
}

int Receive_trans(void)
{
    int			n;
    short		x1, y1, x2, y2;
    u_byte		ch;

    if ((n = Packet_scanf(&rbuf, "%c%hd%hd%hd%hd",
			  &ch, &x1, &y1, &x2, &y2)) <= 0) {
	return n;
    }
    if ((n = Handle_trans(x1, y1, x2, y2)) == -1) {
	return -1;
    }
    return 1;
}

int Receive_paused(void)
{
    int			n;
    short		x, y, count;
    u_byte		ch;

    if ((n = Packet_scanf(&rbuf, "%c%hd%hd%hd", &ch, &x, &y, &count)) <= 0) {
	return n;
    }
    if ((n = Handle_paused(x, y, count)) == -1) {
	return -1;
    }
    return 1;
}

int Receive_radar(void)
{
    int			n;
    short		x, y;
    u_byte		ch, size;

    if ((n = Packet_scanf(&rbuf, "%c%hd%hd%c", &ch, &x, &y, &size)) <= 0) {
	return n;
    }

    if ((n = Handle_radar(x, y, size)) == -1) {
		    return -1;
    }
    return 1;
}

int Receive_damaged(void)
{
    int			n;
    u_byte		ch, damaged;

    if ((n = Packet_scanf(&rbuf, "%c%c", &ch, &damaged)) <= 0) {
	return n;
    }
    if ((n = Handle_damaged(damaged)) == -1) {
	return -1;
    }
    return 1;
}

int Receive_leave(void)
{
    int			n;
    short		id;
    u_byte		ch;

    if ((n = Packet_scanf(&cbuf, "%c%hd", &ch, &id)) <= 0) {
	return n;
    }
    if ((n = Handle_leave(id)) == -1) {
	return -1;
    }
    return 1;
}

int Receive_war(void)
{
    int			n;
    short		robot_id, killer_id;
    u_byte		ch;

    if ((n = Packet_scanf(&cbuf, "%c%hd%hd",
			  &ch, &robot_id, &killer_id)) <= 0) {
	return n;
    }
    if ((n = Handle_war(robot_id, killer_id)) == -1) {
	return -1;
    }
    return 1;
}

int Receive_seek(void)
{
    int			n;
    short		programmer_id, robot_id, sought_id;
    u_byte		ch;

    if ((n = Packet_scanf(&cbuf, "%c%hd%hd%hd", &ch,
			  &programmer_id, &robot_id, &sought_id)) <= 0) {
	return n;
    }
    if ((n = Handle_seek(programmer_id, robot_id, sought_id)) == -1) {
	return -1;
    }
    return 1;
}

int Receive_player(void)
{
    int			n;
    short		id;
    u_byte		ch, myteam, mychar;
    char		name[MAX_CHARS],
			real[MAX_CHARS],
			host[MAX_CHARS],
			shape[2*MSG_LEN],
			*cbuf_ptr = cbuf.ptr;

    if ((n = Packet_scanf(&cbuf,
			  "%c%hd%c%c" "%s%s%s" "%S",
			  &ch, &id, &myteam, &mychar,
			  name, real, host,
			  shape)) <= 0) {
	return n;
    }
    name[MAX_NAME_LEN - 1] = '\0';
    real[MAX_NAME_LEN - 1] = '\0';
    host[MAX_HOST_LEN - 1] = '\0';
    if (version > 0x3200) {
	if ((n = Packet_scanf(&cbuf, "%S", &shape[strlen(shape)])) <= 0) {
	    cbuf.ptr = cbuf_ptr;
	    return n;
	}
    }
    if ((n = Handle_player(id, myteam, mychar, name, real, host, shape)) == -1) {
	return -1;
    }
    return 1;
}

int Receive_score_object(void)
{
    int			n;
    unsigned short	x, y;
    short		score;
    char		msg[MAX_CHARS];
    u_byte		ch;

    if ((n = Packet_scanf(&cbuf, "%c%hd%hu%hu%s",
			  &ch, &score, &x, &y, msg)) <= 0) {
	return n;
    }
    if ((n = Handle_score_object(score, x, y, msg)) == -1) {
	return -1;
    }

    return 1;
}

int Receive_score(void)
{
    int			n;
    short		id, score, life;
    u_byte		ch, mychar;

    n = Packet_scanf(&cbuf, "%c%hd%hd%hd%c", &ch,
		     &id, &score, &life, &mychar);
    if (n <= 0) {
	return n;
    }
    if ((n = Handle_score(id, score, life, mychar)) == -1) {
	return -1;
    }
    return 1;
}

int Receive_timing(void)
{
    int			n,
			check,
			round;
    short		id;
    unsigned short	timing;
    u_byte		ch;

    n = Packet_scanf(&cbuf, "%c%hd%hu", &ch, &id, &timing);
    if (n <= 0) {
	return n;
    }
    check = timing % num_checks;
    round = timing / num_checks;
    if ((n = Handle_timing(id, check, round)) == -1) {
	return -1;
    }
    return 1;
}

int Receive_fuel(void)
{
    int			n;
    unsigned short	num, fuel;
    u_byte		ch;

    if ((n = Packet_scanf(&rbuf, "%c%hu%hu", &ch, &num, &fuel)) <= 0) {
	return n;
    }
    if ((n = Handle_fuel(num, fuel << FUEL_SCALE_BITS)) == -1) {
	return -1;
    }
    if (wbuf.len < MAX_MAP_ACK_LEN) {
	Packet_printf(&wbuf, "%c%ld%hu", PKT_ACK_FUEL, last_loops, num);
    }
    return 1;
}

int Receive_cannon(void)
{
    int			n;
    unsigned short	num, dead_time;
    u_byte		ch;

    if ((n = Packet_scanf(&rbuf, "%c%hu%hu", &ch, &num, &dead_time)) <= 0) {
	return n;
    }
    if ((n = Handle_cannon(num, dead_time)) == -1) {
	return -1;
    }
    if (wbuf.len < MAX_MAP_ACK_LEN) {
	Packet_printf(&wbuf, "%c%ld%hu", PKT_ACK_CANNON, last_loops, num);
    }
    return 1;
}

int Receive_target(void)
{
    int			n;
    unsigned short	num,
			dead_time,
			damage;
    u_byte		ch;

    if ((n = Packet_scanf(&rbuf, "%c%hu%hu%hu", &ch,
			  &num, &dead_time, &damage)) <= 0) {
	return n;
    }
    if ((n = Handle_target(num, dead_time, damage)) == -1) {
	return -1;
    }
    if (wbuf.len < MAX_MAP_ACK_LEN) {
	Packet_printf(&wbuf, "%c%ld%hu", PKT_ACK_TARGET, last_loops, num);
    }
    return 1;
}

int Receive_base(void)
{
    int			n;
    short		id;
    unsigned short	num;
    u_byte		ch;

    if ((n = Packet_scanf(&cbuf, "%c%hd%hu", &ch, &id, &num)) <= 0) {
	return n;
    }
    if ((n = Handle_base(id, num)) == -1) {
	return -1;
    }
    return 1;
}

int Receive_magic(void)
{
    int			n;
    u_byte		ch;

    if ((n = Packet_scanf(&cbuf, "%c%u", &ch, &magic)) <= 0) {
	return n;
    }
    return 1;
}

int Receive_string(void)
{
    int			n;
    u_byte		ch,
			type;
    unsigned short	arg1,
			arg2;

    if ((n = Packet_scanf(&cbuf, "%c%c%hu%hu",
			  &ch, &type, &arg1, &arg2)) <= 0) {
	return n;
    }
    /*
     * Not implemented yet.
     */
    return 1;
}

int Receive_loseitem(void)
{
    int		n;
    u_byte	pkt;
				/* Most of the Receive_ funcs call a */
				/* Handle_ func but that seems */
				/* unecessary here */
    if ((n = Packet_scanf(&rbuf, "%c%c", &pkt, &lose_item)) <= 0) {
	return n;
    }
    return 1;
}

int Send_ack(long rel_loops)
{
    int			n;

    if ((n = Packet_printf(&wbuf, "%c%ld%ld", PKT_ACK,
			   reliable_offset, rel_loops)) <= 0) {
	if (n == 0) {
	    return 0;
	}
	error("Can't ack reliable data");
	return -1;
    }
    return 1;
}

int Receive_reliable(void)
{
    int			n;
    short		len;
    u_byte		ch;
    long		rel,
			rel_loops;

    if ((n = Packet_scanf(&rbuf, "%c%hd%ld%ld",
			  &ch, &len, &rel, &rel_loops)) == -1) {
	return -1;
    }
    if (n == 0) {
	errno = 0;
	error("Incomplete reliable data packet");
	return 0;
    }
#if DEBUG
    if (reliable_offset >= rel + len) {
	printf("Reliable my=%ld pkt=%ld len=%d loops=%ld\n",
	       reliable_offset, rel, len, rel_loops);
    }
#endif
    if (len <= 0) {
	errno = 0;
	error("Bad reliable data length (%d)", len);
	return -1;
    }
    if (rbuf.ptr + len > rbuf.buf + rbuf.len) {
	errno = 0;
	error("Not all reliable data in packet (%d,%d,%d)",
	      rbuf.ptr - rbuf.buf, len, rbuf.len);
	rbuf.ptr += len;
	Sockbuf_advance(&rbuf, rbuf.ptr - rbuf.buf);
	return -1;
    }
    if (rel > reliable_offset) {
	/*
	 * We miss one or more packets.
	 * For now we drop this packet.
	 * We could have kept it until the missing packet(s) arrived.
	 */
	rbuf.ptr += len;
	Sockbuf_advance(&rbuf, rbuf.ptr - rbuf.buf);
	if (Send_ack(rel_loops) == -1) {
	    return -1;
	}
	return 1;
    }
    if (rel + len <= reliable_offset) {
	/*
	 * Duplicate data.  Probably an ack got lost.
	 * Send an ack for our current stream position.
	 */
	rbuf.ptr += len;
	Sockbuf_advance(&rbuf, rbuf.ptr - rbuf.buf);
	if (Send_ack(rel_loops) == -1) {
	    return -1;
	}
	return 1;
    }
    if (rel < reliable_offset) {
	len -= (short)(reliable_offset - rel);
	rbuf.ptr += reliable_offset - rel;
	rel = reliable_offset;
    }
    if (cbuf.ptr > cbuf.buf) {
	Sockbuf_advance(&cbuf, cbuf.ptr - cbuf.buf);
    }
    if (Sockbuf_write(&cbuf, rbuf.ptr, len) != len) {
	errno = 0;
	error("Can't copy reliable data to buffer");
	rbuf.ptr += len;
	Sockbuf_advance(&rbuf, rbuf.ptr - rbuf.buf);
	return -1;
    }
    reliable_offset += len;
    rbuf.ptr += len;
    Sockbuf_advance(&rbuf, rbuf.ptr - rbuf.buf);
    if (Send_ack(rel_loops) == -1) {
	return -1;
    }
    return 1;
}

int Receive_reply(int *replyto, int *result)
{
    int		n;
    u_byte	type, ch1, ch2;

    n = Packet_scanf(&cbuf, "%c%c%c", &type, &ch1, &ch2);
    if (n <= 0) {
	return n;
    }
    if (n != 3 || type != PKT_REPLY) {
	error("Can't receive reply packet");
	return -1;
    }
    *replyto = ch1;
    *result = ch2;
    return 1;
}

int Send_keyboard(u_byte *keyboard_vector)
{
    int		size = KEYBOARD_SIZE;

    if (version < 0x3800) {
	/* older servers have a keyboard_size of 8 bytes instead of 9. */
	size--;
    }
    if (wbuf.size - wbuf.len < size + 1 + 4) {
	/* Not enough write buffer space for keyboard state */
	return 0;
    }
    Packet_printf(&wbuf, "%c%ld", PKT_KEYBOARD, last_keyboard_change);
    memcpy(&wbuf.buf[wbuf.len], keyboard_vector, size);
    wbuf.len += size;
    last_keyboard_update = last_loops;
    Send_talk();
    if (Sockbuf_flush(&wbuf) == -1) {
	error("Can't send keyboard update");
	return -1;
    }

    return 0;
}

int Send_shape(char *str)
{
    wireobj		*w;
    char		buf[MSG_LEN], ext[MSG_LEN];

    w = Convert_shape_str(str);
    Convert_ship_2_string(w, buf, ext, (version < 0x3200) ? 0x3100 : 0x3200);
    Free_ship_shape(w);
    if (Packet_printf(&wbuf, "%c%S", PKT_SHAPE, buf) <= 0) {
	return -1;
    }
    if (version > 0x3200) {
	if (Packet_printf(&wbuf, "%S", ext) <= 0) {
	    return -1;
	}
    }
    return 0;
}

int Send_power(DFLOAT power)
{
    if (Packet_printf(&wbuf, "%c%hd", PKT_POWER,
		      (int) (power * 256.0)) == -1) {
	return -1;
    }
    return 0;
}

int Send_power_s(DFLOAT power_s)
{
    if (Packet_printf(&wbuf, "%c%hd", PKT_POWER_S,
		      (int) (power_s * 256.0)) == -1) {
	return -1;
    }
    return 0;
}

int Send_turnspeed(DFLOAT turnspeed)
{
    if (Packet_printf(&wbuf, "%c%hd", PKT_TURNSPEED,
		      (int) (turnspeed * 256.0)) == -1) {
	return -1;
    }
    return 0;
}

int Send_turnspeed_s(DFLOAT turnspeed_s)
{
    if (Packet_printf(&wbuf, "%c%hd", PKT_TURNSPEED_S,
		      (int) (turnspeed_s * 256.0)) == -1) {
	return -1;
    }
    return 0;
}

int Send_turnresistance(DFLOAT turnresistance)
{
    if (Packet_printf(&wbuf, "%c%hd", PKT_TURNRESISTANCE,
		      (int) (turnresistance * 256.0)) == -1) {
	return -1;
    }
    return 0;
}

int Send_turnresistance_s(DFLOAT turnresistance_s)
{
    if (Packet_printf(&wbuf, "%c%hd", PKT_TURNRESISTANCE_S,
		      (int) (turnresistance_s * 256.0)) == -1) {
	return -1;
    }
    return 0;
}

int Receive_quit(void)
{
    unsigned char	pkt;
    sockbuf_t		*sbuf;
    char		reason[MAX_CHARS];

    if (rbuf.ptr < rbuf.buf + rbuf.len) {
	sbuf = &rbuf;
    } else {
	sbuf = &cbuf;
    }
    if (Packet_scanf(sbuf, "%c", &pkt) != 1) {
	errno = 0;
	error("Can't read quit packet");
    } else {
	if (Packet_scanf(sbuf, "%s", reason) <= 0) {
	    strcpy(reason, "unknown reason");
	}
	errno = 0;
	error("Got quit packet: \"%s\"", reason);
    }
    return -1;
}


int Receive_audio(void)
{
    int			n;
    unsigned char	pkt, type, vol;

    if ((n = Packet_scanf(&rbuf, "%c%c%c", &pkt, &type, &vol)) <= 0) {
	return n;
    }
#ifdef SOUND
    if ((n = Handle_audio(type, vol)) == -1) {
	return -1;
    }
#endif /* SOUND */
    return 1;
}


int Receive_talk_ack(void)
{
    int			n;
    unsigned char	pkt;
    long		talk_ack;

    if ((n = Packet_scanf(&cbuf, "%c%ld", &pkt, &talk_ack)) <= 0) {
	return n;
    }
    if (talk_ack >= talk_pending) {
	talk_pending = 0;
    }
    return 1;
}


int Net_talk(char *str)
{
    strncpy(talk_str, str, sizeof talk_str - 1);
    talk_pending = ++talk_sequence_num;
    talk_last_send = last_loops - TALK_RETRY;
    return 0;
}


int Send_talk(void)
{
    if (talk_pending == 0) {
	return 0;
    }
    if (last_loops - talk_last_send < TALK_RETRY) {
	return 0;
    }
    if (Packet_printf(&wbuf, "%c%ld%s", PKT_TALK,
		      talk_pending, talk_str) == -1) {
	return -1;
    }
    talk_last_send = last_loops;
    return 0;
}


int Send_display(void)
{
    int			width_wanted = draw_width;
    int			height_wanted = draw_height;

#ifdef	WINDOWSCALING
    width_wanted = (int)(width_wanted * scaleFactor + 0.5);
    height_wanted = (int)(height_wanted * scaleFactor + 0.5);
#endif

    LIMIT(width_wanted, MIN_VIEW_SIZE, MAX_VIEW_SIZE);
    LIMIT(height_wanted, MIN_VIEW_SIZE, MAX_VIEW_SIZE);

    if (width_wanted == view_width &&
	height_wanted == view_height &&
	debris_colors == num_spark_colors &&
	spark_rand == old_spark_rand &&
	last_loops != 0) {
	return 0;
    }

    if (Packet_printf(&wbuf, "%c%hd%hd%c%c", PKT_DISPLAY,
		      width_wanted, height_wanted,
		      num_spark_colors, spark_rand) == -1) {
	return -1;
    }

    old_spark_rand = spark_rand;

    return 0;
}


int Send_modifier_bank(int bank)
{
    if (bank < 0 || bank >= NUM_MODBANKS)
	return -1;
    if (Packet_printf(&wbuf, "%c%c%s", PKT_MODIFIERBANK,
		      bank, modBankStr[bank]) == -1)
	return -1;
    return 0;
}

int Send_pointer_move(int movement)
{
    if (version > 0x3201) {
	if (Packet_printf(&wbuf, "%c%hd", PKT_POINTER_MOVE,
			  movement) == -1) {
	    return -1;
	}
    }
    return 0;
}

int Send_audio_request(int onoff)
{
#ifdef DEBUG_SOUND
    printf("Send_audio_request %d\n", onoff);
#endif

    if (version < 0x3250) {
	return 0;
    }
#ifndef SOUND
    onoff = 0;
#endif
    if (Packet_printf(&wbuf, "%c%c", PKT_REQUEST_AUDIO, (onoff != 0)) == -1) {
	return -1;
    }
    return 0;
}

int Send_fps_request(int fps)
{
    if (version < 0x3280) {
	return 0;
    }
    if (Packet_printf(&wbuf, "%c%c", PKT_ASYNC_FPS, fps) == -1) {
	return -1;
    }
    return 0;
}
