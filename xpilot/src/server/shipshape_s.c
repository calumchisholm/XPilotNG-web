/*
 * XPilot, a multiplayer gravity war game.  Copyright (C) 1991-2001 by
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

#include "xpserver.h"

char shipshape_s_version[] = VERSION;

extern void	Make_table(void);

void Rotate_point(shapepos pt[RES])
{
    int			i;

    /*pt[0].clk.cx *= CLICK;
      pt[0].clk.cy *= CLICK;*/
    for (i = 1; i < RES; i++) {
	pt[i].clk.cx = (tcos(i) * pt[0].clk.cx - tsin(i) * pt[0].clk.cy) + .5;
	pt[i].clk.cy = (tsin(i) * pt[0].clk.cx + tcos(i) * pt[0].clk.cy) + .5;
    }
}

/* kps - tmp hack */
shapepos *Shape_get_points(shape *s, int dir)
{
    int i;

    /* kps - optimize if cashed_dir == dir */
    for (i = 0; i < s->num_points; i++)
	s->cashed_pts[i] = s->pts[i][dir];

    return s->cashed_pts;
}
