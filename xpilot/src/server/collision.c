/*
 *
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <limits.h>
#include <assert.h>

#ifdef _WINDOWS
# include "NT/winServer.h"
#endif

#define SERVER
#include "version.h"
#include "config.h"
#include "serverconst.h"
#include "list.h"
#include "global.h"
#include "proto.h"
#include "map.h"
#include "score.h"
#include "saudio.h"
#include "item.h"
#include "netserver.h"
#include "pack.h"
#include "error.h"
#include "portability.h"
#include "objpos.h"
#include "asteroid.h"
#include "commonproto.h"

char collision_version[] = VERSION;


/*
 * The very first "analytical" collision patch, XPilot 3.6.2
 * Faster than other patches and accurate below half warp-speed
 * Trivial common subexpressions are eliminated by any reasonable compiler,
 * and kept here for readability.
 * Written by Pontus (Rakk, Kepler) pontus@ctrl-c.liu.se Jan 1998
 * Kudos to Svenske and Mad Gurka for beta testing, and Murx for
 * invaluable insights.
 */

/* kps - note: the arguments are all given in clicks here */
static int in_range_acd_old(
	int p1x, int p1y, int p2x, int p2y,
	int q1x, int q1y, int q2x, int q2y,
	int r)
{
    long	fac1, fac2;
    double	tmin, fminx, fminy;
    long	top, bot;
    bool	mpx, mpy, mqx, mqy;

    /*
     * Get the wrapped coordinates straight
     */
    if (BIT(World.rules->mode, WRAP_PLAY)) {
	if ((mpx = (ABS(p2x - p1x) > World.cwidth / 2))) {
	    if (p1x > p2x)
		p1x -= World.cwidth;
	    else
		p2x -= World.cwidth;
	}
	if ((mpy = (ABS(p2y - p1y) > World.cheight / 2))) {
	    if (p1y > p2y)
		p1y -= World.cheight;
	    else
		p2y -= World.cheight;
	}
	if ((mqx = (ABS(q2x - q1x) > World.cwidth / 2))) {
	    if (q1x > q2x)
		q1x -= World.cwidth;
	    else
		q2x -= World.cwidth;
	}
	if ((mqy = (ABS(q2y - q1y) > World.cheight / 2))) {
	    if (q1y > q2y)
		q1y -= World.cheight;
	    else
		q2y -= World.cheight;
	}

	if (mpx && !mqx && (q2x > World.cwidth / 2 || q1x > World.cwidth / 2)) {
	    q1x -= World.cwidth;
	    q2x -= World.cwidth;
	}

	if (mqy && !mpy && (q2y > World.cheight / 2 || q1y > World.cheight / 2)) {
	    q1y -= World.cheight;
	    q2y -= World.cheight;
	}

	if (mqx && !mpx && (p2x > World.cwidth / 2 || p1x > World.cwidth / 2)) {
	    p1x -= World.cwidth;
	    p2x -= World.cwidth;
	}

	if (mqy && !mpy && (p2y > World.cheight / 2 || p1y > World.cheight / 2)) {
	    p1y -= World.cheight;
	    p2y -= World.cheight;
	}
    }

    /* kps hack - change to pixels to not get overflows */
    p1x = CLICK_TO_PIXEL(p1x);
    p1y = CLICK_TO_PIXEL(p1y);
    p2x = CLICK_TO_PIXEL(p2x);
    p2y = CLICK_TO_PIXEL(p2y);
    q1x = CLICK_TO_PIXEL(q1x);
    q1y = CLICK_TO_PIXEL(q1y);
    q2x = CLICK_TO_PIXEL(q2x);
    q2y = CLICK_TO_PIXEL(q2y);
    r   = CLICK_TO_PIXEL(r);
    /* kps hack end */

    /*
     * Do the detection
     */
    if ((p2x - q2x) * (p2x - q2x) + (p2y - q2y) * (p2y - q2y) < r * r)
	return 1;
    fac1 = -p1x + p2x + q1x - q2x;
    fac2 = -p1y + p2y + q1y - q2y;
    top = -(fac1 * (-p2x + q2x) + fac2 * (-p2y + q2y));
    bot = (fac1 * fac1 + fac2 * fac2);
    if (top < 0 || bot < 1 || top > bot)
	return 0;
    tmin = ((double)top) / ((double)bot);
    fminx = -p2x + q2x + fac1 * tmin;
    fminy = -p2y + q2y + fac2 * tmin;
    if (fminx * fminx + fminy * fminy < r * r)
	return 1;
    else
	return 0;
}

/* new acd functions */
/* doubles because the multiplies might overflow ints */
int in_range_acd(double dx, double dy, double dvx, double dvy, double r)
{
    double	tmin, fminx, fminy;
    double	top, bot;

    /*
     * Get the wrapped coordinates straight
     */
    if (BIT(World.rules->mode, WRAP_PLAY)) {
	if (dx > World.cwidth / 2)
	    dx -= World.cwidth;
	else if (dx < -World.cwidth / 2)
	    dx += World.cwidth;
	if (dy > World.cheight / 2)
	    dy -= World.cheight;
	else if (dy < -World.cheight / 2)
	    dy += World.cheight;
    }

    if (dx * dx + dy * dy < r * r)
	return 1;
    top = -(dvx * dx + dvy * dy);
    bot = dvx * dvx + dvy * dvy;
    if (top < 0 || bot < CLICK * CLICK || top > bot)
	return 0;
    tmin = top / bot;
    fminx = dx + dvx * tmin;
    fminy = dy + dvy * tmin;
    if (fminx * fminx + fminy * fminy < r * r)
	return 1;
    else
	return 0;
}

int in_range_simple(int px, int py, int qx, int qy, int r)
{
    int dx = px - qx, dy = py - qy;

    if (dx > World.cwidth >> 1)
	dx -= World.cwidth;
    else if (dx < -World.cwidth >> 1)
	dx += World.cwidth;
    if (dy > World.cheight >> 1)
	dy -= World.cheight;
    else if (dy < -World.cheight >> 1)
	dy += World.cheight;
    if ((double)dx * dx + (double)dy * dy < r * r)
	return 1;
    else
	return 0;
}

int in_range_partial(double dx, double dy, double dvx, double dvy,
		     double r, DFLOAT wall_time)
{
    double	tmin, fminx, fminy;
    double	top, bot;

    /*
     * Get the wrapped coordinates straight
     */
    if (BIT(World.rules->mode, WRAP_PLAY)) {
	if (dx > World.cwidth / 2)
	    dx -= World.cwidth;
	else if (dx < -World.cwidth / 2)
	    dx += World.cwidth;
	if (dy > World.cheight / 2)
	    dy -= World.cheight;
	else if (dy < -World.cheight / 2)
	    dy += World.cheight;
    }

    top = -(dvx * dx + dvy * dy);
    bot = dvx * dvx + dvy * dvy;
    if (top <= 0)
	return 0;
    if (bot < 5 * CLICK * CLICK || top >= bot)
	tmin = wall_time;
    else
	tmin = top / bot;
    fminx = dx + dvx * tmin;
    fminy = dy + dvy * tmin;
    if (fminx * fminx + fminy * fminy < r * r)
	return 1;
    else
	return 0;
}



/*
 * Globals
 */
static char msg[MSG_LEN];

static void PlayerCollision(void);
static void PlayerObjectCollision(int ind);
static void AsteroidCollision(void);
static void BallCollision(void);
static void MineCollision(void);
static void Player_collides_with_ball(int ind, object *obj, int radius);
static void Player_collides_with_item(int ind, object *obj);
static void Player_collides_with_mine(int ind, object *obj);
static void Player_collides_with_debris(int ind, object *obj);
static void Player_collides_with_asteroid(int ind, wireobject *obj);
static void Player_collides_with_killing_shot(int ind, object *obj);
static void Player_pass_checkpoint(int ind);



void Check_collision(void)
{
    BallCollision();
    MineCollision();

    if (round_delay == 0) {
	PlayerCollision();
    }

    AsteroidCollision();
}

static void PlayerCollision(void)
{
    int			i, j;
    DFLOAT		sc, sc2;
    player		*pl;

    /* Player - player, checkpoint, treasure, object and wall */
    for (i=0; i<NumPlayers; i++) {
	pl = Players[i];
	if (BIT(pl->status, PLAYING|PAUSE|GAME_OVER|KILLED) != PLAYING)
	    continue;

	if (!INSIDE_MAP(pl->pos.cx, pl->pos.cy)) {
	    SET_BIT(pl->status, KILLED);
	    sprintf(msg, "%s left the known universe.", pl->name);
	    Set_message(msg);
	    sc = Rate(WALL_SCORE, pl->score);
	    SCORE(i, -sc, pl->pos.cx, pl->pos.cy, pl->name);
	    continue;
	}

	if (BIT(pl->used, HAS_PHASING_DEVICE))
	    continue;

	/* Player - player */
	if (BIT(World.rules->mode, CRASH_WITH_PLAYER | BOUNCE_WITH_PLAYER)) {
	    for (j=i+1; j<NumPlayers; j++) {
		if (BIT(Players[j]->status, PLAYING|PAUSE|GAME_OVER|KILLED)
		    != PLAYING) {
		    continue;
		}
		if (BIT(Players[j]->used, HAS_PHASING_DEVICE))
		    continue;
		if (is_polygon_map || !useOldCode) {
		    if (!in_range_acd(pl->prevpos.cx - Players[j]->prevpos.cx,
				      pl->prevpos.cy - Players[j]->prevpos.cy,
				      pl->extmove.cx - Players[j]->extmove.cx,
				      pl->extmove.cy - Players[j]->extmove.cy,
				      (2*SHIP_SZ-6) * CLICK)) {
 			continue;
		    }
		} else {
		    if (!in_range_acd_old(pl->prevpos.cx, pl->prevpos.cy,
					  pl->pos.cx, pl->pos.cy,
					  Players[j]->prevpos.cx,
					  Players[j]->prevpos.cy,
					  Players[j]->pos.cx,
					  Players[j]->pos.cy,
					  PIXEL_TO_CLICK(2*SHIP_SZ-6))) {
			continue;
		    }
		}

		/*
		 * Here we can add code to do more accurate player against
		 * player collision detection.
		 * A new algorithm could be based on the following idea:
		 *
		 * - If we can draw an uninterupted line between two players:
		 *   - Then test for both ships:
		 *     - For the three points which make up a ship:
		 *       - If we can draw a line between its previous
		 *         position and its current position which does not
		 *         cross the first line.
		 * Then the ships have not collided even though they may be
		 * very close to one another.
		 * The choosing of the first line may not be easy however.
		 */

		if (Team_immune(pl->id, Players[j]->id)
		    || PSEUDO_TEAM(i, j)) {
		    continue;
		}
		sound_play_sensors(pl->pos.cx, pl->pos.cy, PLAYER_HIT_PLAYER_SOUND);
		if (BIT(World.rules->mode, BOUNCE_WITH_PLAYER)) {
		    if (BIT(pl->used, (HAS_SHIELD|HAS_EMERGENCY_SHIELD)) !=
			(HAS_SHIELD|HAS_EMERGENCY_SHIELD)) {
			Add_fuel(&(pl->fuel), (long)ED_PL_CRASH);
			Item_damage(i, destroyItemInCollisionProb);
		    }
		    if (BIT(Players[j]->used, (HAS_SHIELD|
					       HAS_EMERGENCY_SHIELD)) !=
			(HAS_SHIELD|HAS_EMERGENCY_SHIELD)) {
			Add_fuel(&(Players[j]->fuel), (long)ED_PL_CRASH);
			Item_damage(j, destroyItemInCollisionProb);
		    }
		    pl->forceVisible = 20;
		    Players[j]->forceVisible = 20;
		    Obj_repel((object *)pl, (object *)Players[j],
			      PIXEL_TO_CLICK(2*SHIP_SZ));
		}
		if (!BIT(World.rules->mode, CRASH_WITH_PLAYER)) {
		    continue;
		}

		if (pl->fuel.sum <= 0
		    || (!BIT(pl->used, HAS_SHIELD)
			&& !BIT(pl->have, HAS_ARMOR))) {
		    SET_BIT(pl->status, KILLED);
		}
		if (Players[j]->fuel.sum <= 0
		    || (!BIT(Players[j]->used, HAS_SHIELD)
			&& !BIT(Players[j]->have, HAS_ARMOR))) {
		    SET_BIT(Players[j]->status, KILLED);
		}

		if (!BIT(pl->used, HAS_SHIELD)
		    && BIT(pl->have, HAS_ARMOR)) {
		    Player_hit_armor(i);
		}
		if (!BIT(Players[j]->used, HAS_SHIELD)
		    && BIT(Players[j]->have, HAS_ARMOR)) {
		    Player_hit_armor(j);
		}

		if (BIT(Players[j]->status, KILLED)) {
		    if (BIT(pl->status, KILLED)) {
			sprintf(msg, "%s and %s crashed.",
				pl->name, Players[j]->name);
			Set_message(msg);
			if (!IS_TANK_IND(i) && !IS_TANK_IND(j)) {
			    sc = Rate(Players[j]->score, pl->score)
			 		    * crashScoreMult;
			    sc2 = Rate(pl->score, Players[j]->score)
					     * crashScoreMult;
			    Score_players(i, -sc, Players[j]->name,
					  j, -sc2, pl->name);
			} else if (IS_TANK_IND(i)) {
			    int i_tank_owner = GetInd[Players[i]->lock.pl_id];
			    sc = Rate(Players[i_tank_owner]->score,
						 Players[j]->score)
					    * tankKillScoreMult;
			    Score_players(i_tank_owner, sc, Players[j]->name,
					  j, -sc, pl->name);
			} else if (IS_TANK_IND(j)) {
			    int j_tank_owner = GetInd[Players[j]->lock.pl_id];
			    sc = Rate(Players[j_tank_owner]->score,
						 pl->score)
					    * tankKillScoreMult;
			    Score_players(j_tank_owner, sc, pl->name,
					  i, -sc, Players[j]->name);
			} /* don't bother scoring two tanks */
		    } else {
			int i_tank_owner = i;
			if (IS_TANK_IND(i)) {
			    i_tank_owner = GetInd[Players[i]->lock.pl_id];
			    if (i_tank_owner == j) {
				i_tank_owner = i;
			    }
			}
			sprintf(msg, "%s ran over %s.",
				pl->name, Players[j]->name);
			Set_message(msg);
			sound_play_sensors(Players[j]->pos.cx,
					   Players[j]->pos.cy,
					   PLAYER_RAN_OVER_PLAYER_SOUND);
			Rank_AddKill(pl);
			if (IS_TANK_IND(i)) {
			    sc = Rate(Players[i_tank_owner]->score,
						 Players[j]->score)
					    * tankKillScoreMult;
			} else {
			    sc = Rate(pl->score, Players[j]->score)
					    * runoverKillScoreMult;
			}
			Score_players(i_tank_owner, sc, Players[j]->name,
				      j, -sc, pl->name);
		    }

		} else {
		    if (BIT(pl->status, KILLED)) {
			int j_tank_owner = j;
			if (IS_TANK_IND(j)) {
			    j_tank_owner = GetInd[Players[j]->lock.pl_id];
			    if (j_tank_owner == i) {
				j_tank_owner = j;
			    }
			}
			sprintf(msg, "%s ran over %s.",
				Players[j]->name, pl->name);
			Set_message(msg);
			sound_play_sensors(pl->pos.cx, pl->pos.cy,
					   PLAYER_RAN_OVER_PLAYER_SOUND);
			Rank_AddKill(Players[j]);
			if (IS_TANK_IND(j)) {
			    sc = Rate(Players[j_tank_owner]->score, pl->score)
				   * tankKillScoreMult;
			} else {
			    sc = Rate(Players[j]->score, pl->score)
				   * runoverKillScoreMult;
			}
			Score_players(j_tank_owner, sc, pl->name,
				      i, -sc, Players[j]->name);
		    }
		}

		if (BIT(Players[j]->status, KILLED)) {
		    if (IS_ROBOT_IND(j)
			&& Robot_war_on_player(j) == pl->id) {
			Robot_reset_war(j);
		    }
		}

		if (BIT(pl->status, KILLED)) {
		    if (IS_ROBOT_PTR(pl)
			&& Robot_war_on_player(i) == Players[j]->id) {
			Robot_reset_war(i);
		    }
		    /* cannot crash with more than one player at the same time? */
		    /* hmm, if 3 players meet at the same point at the same time? */
		    /* break; */
		}
	    }
	}

	/* Player picking up ball/treasure */
	if (!BIT(pl->used, HAS_CONNECTOR)
	    || BIT(pl->used, HAS_PHASING_DEVICE)) {
	    pl->ball = NULL;
	} else if (pl->ball != NULL) {
	    ballobject *ball = pl->ball;

	    if (ball->life <= 0 || ball->id != NO_ID)
		pl->ball = NULL;
	    else {
		DFLOAT distance = Wrap_length(pl->pos.cx - ball->pos.cx,
					      pl->pos.cy - ball->pos.cy);
		int group;

		if (distance >= ballConnectorLength * CLICK) {
		    ball->id = pl->id;
		    /* this is only the team of the owner of the ball,
		     * not the team the ball belongs to. the latter is
		     * found through the ball's treasure */
		    ball->team = pl->team;
		    if (ball->owner == NO_ID)
			ball->life = 1e6;  /* for frame counter */
		    ball->owner = pl->id;
		    SET_BIT(ball->status, GRAVITY);
		    World.treasures[ball->treasure].have = false;
		    SET_BIT(pl->have, HAS_BALL);
		    pl->ball = NULL;
		    sound_play_sensors(pl->pos.cx, pl->pos.cy,
				       CONNECT_BALL_SOUND);
		    pl->grabbedBallFrame = main_loops;
		    if (is_polygon_map || !useOldCode) {
			/* The ball might already be inside the team's ball
			 * target. */
			extern shipobj ball_wire;
			group = shape_is_inside(ball->pos.cx,
						ball->pos.cy,
						BALL_BIT | HITMASK(pl->team),
						(object *)ball,
						&ball_wire, 0);
			if (group != NO_GROUP) {
			    Ball_hits_goal(ball, group);
			    ball->life = 0;
			}
		    }
		}
	    }
	} else {
	    /*
	     * We want a separate list of balls to avoid searching
	     * the object list for balls.
	     */
	    int dist, mindist = ballConnectorLength * CLICK;
	    for (j = 0; j < NumObjs; j++) {
		if (BIT(Obj[j]->type, OBJ_BALL) && Obj[j]->id == NO_ID) {
		    dist = Wrap_length(pl->pos.cx - Obj[j]->pos.cx,
				       pl->pos.cy - Obj[j]->pos.cy);
		    if (dist < mindist) {
			ballobject *ball = BALL_PTR(Obj[j]);
			int bteam = World.treasures[ball->treasure].team;

			/*
			 * The treasure's team cannot connect before
			 * somebody else has owned the ball.
			 * This was done to stop team members
			 * taking and hiding with the ball... this was
			 * considered bad gamesmanship.
			 */
			/* mara: this also causes balls to be impossible to
			   grab when the owner leaves...*/
			if (!BIT(World.rules->mode, TEAM_PLAY)
			    || ball->owner != NO_ID
			    || pl->team != bteam) {
			    pl->ball = BALL_PTR(Obj[j]);
			    mindist = dist;
			}
		    }
		}
	    }
	}

	PlayerObjectCollision(i);

	/* Player checkpoint */
	if (BIT(World.rules->mode, TIMING)
	    && BIT(pl->status, PAUSE|GAME_OVER) == 0) {
	    if (pl->round != 0) {
		pl->time++;
	    }
	    if (BIT(pl->status, PLAYING|KILLED) == PLAYING
		&& Wrap_length(pl->pos.cx - World.check[pl->check].cx,
			       pl->pos.cy - World.check[pl->check].cy)
		    < checkpointRadius * BLOCK_CLICKS
		&& !IS_TANK_PTR(pl)
		&& !ballrace) {
		Player_pass_checkpoint(i);
	    }
	}
    }
}

int IsOffensiveItem(enum Item i)
{
    if (BIT(1 << i,
	    ITEM_BIT_WIDEANGLE |
	    ITEM_BIT_REARSHOT |
	    ITEM_BIT_MINE |
	    ITEM_BIT_MISSILE |
	    ITEM_BIT_LASER)) {
	return true;
    }
    return false;
}

int IsDefensiveItem(enum Item i)
{
    if (BIT(1 << i,
	    ITEM_BIT_CLOAK |
	    ITEM_BIT_ECM |
	    ITEM_BIT_TRANSPORTER |
	    ITEM_BIT_TRACTOR_BEAM |
	    ITEM_BIT_EMERGENCY_SHIELD |
	    ITEM_BIT_MIRROR |
	    ITEM_BIT_DEFLECTOR |
	    ITEM_BIT_HYPERJUMP |
	    ITEM_BIT_PHASING |
	    ITEM_BIT_TANK |
	    ITEM_BIT_ARMOR)) {
	return true;
    }
    return false;
}

int CountOffensiveItems(player *pl)
{
    return (pl->item[ITEM_WIDEANGLE] + pl->item[ITEM_REARSHOT] +
	    pl->item[ITEM_MINE] + pl->item[ITEM_MISSILE] +
	    pl->item[ITEM_LASER]);
}

int CountDefensiveItems(player *pl)
{
    int count;

    count = pl->item[ITEM_CLOAK] + pl->item[ITEM_ECM] + pl->item[ITEM_ARMOR] +
	    pl->item[ITEM_TRANSPORTER] + pl->item[ITEM_TRACTOR_BEAM] +
	    pl->item[ITEM_EMERGENCY_SHIELD] + pl->fuel.num_tanks +
	    pl->item[ITEM_DEFLECTOR] + pl->item[ITEM_HYPERJUMP] +
	    pl->item[ITEM_PHASING] + pl->item[ITEM_MIRROR];
    if (pl->emergency_shield_left > 0)
 	count++;
    if (pl->phasing_left > 0)
	count++;
    return count;
}

/* Collmodes:
   0 - Object has not been moved in walls.c after it was created.
       Check only whether end-of-frame position is on top of a
       player.
   1 - Object was moved in walls.c. It did not hit walls and
       therefore moved at constant speed from obj->prevpos to
       obj->pos. Check whether it was within range during movement
       using analytical collision detection.
   2 - Object was moving from obj->prevpos by obj->extmove but it
       hit a wall and was destroyed after completing obj->wall_time
       of the distance. Check whether it was within range similarly
       to case 1 but only consider hits at the beginning of the
       frame before obj->wall_time.
   3 - Object bounced off a wall at least once without getting
       destroyed. Checking all the linear parts of movement
       separately is not implemented yet so we don't detect any
       possible collisions. Note that it would already be possible
       to check the first linear part until obj->wall_time similarly
       to case 2. This is not done because we lack the information
       needed to calculate the effect of non-fatal hits. The
       direction and speed of the object at the moment of impact
       were likely completely different from the end-of-frame values
       we have now.

       Different collision modes for players have not been implemented
       yet. It's supposed that they move in a straight line from
       prevpos to pos. This can lead to some erroneous hits.
*/
static void PlayerObjectCollision(int ind)
{
    int		j, range, radius, hit, obj_count;
    player	*pl = Players[ind];
    object	*obj, **obj_list;


    /*
     * Collision between a player and an object.
     */
    if (BIT(pl->status, PLAYING|PAUSE|GAME_OVER|KILLED) != PLAYING)
	return;

    Cell_get_objects(OBJ_X_IN_BLOCKS(pl), OBJ_Y_IN_BLOCKS(pl),
		     4, 500,
		     &obj_list, &obj_count);

    for (j = 0; j < obj_count; j++) {
	obj = obj_list[j];

	if (is_polygon_map || !useOldCode) {
	    range = SHIP_SZ + obj->pl_range;
	    switch (obj->collmode) {
	    case 0:
		hit = in_range_simple(pl->pos.cx, pl->pos.cy,
				      obj->pos.cx, obj->pos.cy,
				      range * CLICK);
		break;
	    case 1:
		hit = in_range_acd(pl->prevpos.cx - obj->prevpos.cx,
				   pl->prevpos.cy - obj->prevpos.cy,
				   pl->extmove.cx - obj->extmove.cx,
				   pl->extmove.cy - obj->extmove.cy,
				   range * CLICK);
		break;
	    case 2:
		hit = in_range_partial(pl->prevpos.cx - obj->prevpos.cx,
				       pl->prevpos.cy - obj->prevpos.cy,
				       pl->extmove.cx - obj->extmove.cx,
				       pl->extmove.cy - obj->extmove.cy,
				       range * CLICK, obj->wall_time);
		break;
	    case 3:
	    default:
#if 0
		warn("Unimplemented collision mode %d", obj->collmode);
#endif
		continue;
	    }
	    if (!hit)
		continue;
	} else {
	    if (obj->life <= 0)
		continue;

	    range = SHIP_SZ + obj->pl_range;
	    if (!in_range_acd_old(pl->prevpos.cx, pl->prevpos.cy,
				  pl->pos.cx, pl->pos.cy,
				  obj->prevpos.cx, obj->prevpos.cy,
				  obj->pos.cx, obj->pos.cy,
				  range * CLICK)) {
		continue;
	    }
	}

	if (obj->id != NO_ID) {
	    if (obj->id == pl->id) {
		if (BIT(obj->type, OBJ_SPARK|OBJ_MINE)
		    && BIT(obj->status, OWNERIMMUNE)) {
		    continue;
		}
		else if (selfImmunity)
		    continue;
	    } else if (selfImmunity &&
		       IS_TANK_PTR(pl) &&
		       (pl->lock.pl_id == obj->id)) {
		continue;
	    } else if (Team_immune(obj->id, pl->id))
		continue;
	    else if (BIT(Players[GetInd[obj->id]]->status, PAUSE))
		continue;
	} else if (BIT(World.rules->mode, TEAM_PLAY)
		   && teamImmunity
		   && obj->team == pl->team
		   /* allow players to destroy their team's unowned balls */
		   && obj->type != OBJ_BALL) {
	    continue;
	}

	if (obj->type == OBJ_ITEM) {
	    if (BIT(pl->used, HAS_SHIELD) && !shieldedItemPickup) {
		SET_BIT(obj->status, GRAVITY);
		Delta_mv((object *)pl, obj);
		continue;
	    }
	}
	else if (BIT(obj->type, OBJ_HEAT_SHOT | OBJ_SMART_SHOT | OBJ_TORPEDO
				| OBJ_SHOT | OBJ_CANNON_SHOT)) {
	    if (pl->id == obj->id && frame_time < obj->fusetime)
		continue;
	}
	else if (BIT(obj->type, OBJ_MINE)) {
	    if (BIT(obj->status, CONFUSED))
		continue;
	}
	else if (BIT(obj->type, OBJ_BALL) && obj->id != NO_ID) {
	    if (BIT(Players[GetInd[obj->id]]->used, HAS_PHASING_DEVICE))
		continue;
	}
	else if (BIT(obj->type, OBJ_PULSE)) {
	    pulseobject *pulse = PULSE_PTR(obj);
	    if (pl->id == pulse->id && !pulse->refl)
		continue;
	}
	/*
	 * Objects actually only hit the player if they are really close.
	 */
	radius = SHIP_SZ + obj->pl_radius;
	if (radius >= range) {
	    hit = 1;
	} else {
	    if (is_polygon_map || !useOldCode) {
		switch (obj->collmode) {
		case 0:
		    hit = in_range_simple(pl->pos.cx, pl->pos.cy,
					  obj->pos.cx, obj->pos.cy,
					  radius * CLICK);
		    break;
		case 1:
		    hit = in_range_acd(pl->prevpos.cx - obj->prevpos.cx,
				       pl->prevpos.cy - obj->prevpos.cy,
				       pl->extmove.cx - obj->extmove.cx,
				       pl->extmove.cy - obj->extmove.cy,
				       radius * CLICK);
		    break;
		case 2:
		    hit = in_range_partial(pl->prevpos.cx - obj->prevpos.cx,
					   pl->prevpos.cy - obj->prevpos.cy,
					   pl->extmove.cx - obj->extmove.cx,
					   pl->extmove.cy - obj->extmove.cy,
					   radius * CLICK, obj->wall_time);
		    break;
		default:
		    warn("Unimplemented collision mode %d", obj->collmode);
		    continue;
		}
	    } else {
		hit = in_range_acd_old(pl->prevpos.cx, pl->prevpos.cy,
				       pl->pos.cx, pl->pos.cy,
				       obj->prevpos.cx, obj->prevpos.cy,
				       obj->pos.cx, obj->pos.cy,
				       range * CLICK);


	    }
	}

#if 0
	if ((is_polygon_map || !useOldCode) && obj->collmode != 1) {
	    char MSG[80];
	    sprintf(MSG, "Collision type=%d, hit=%d, cm=%d, time=%f, "
		    "frame=%ld [*DEBUG*]", obj->type, hit, obj->collmode,
		    obj->wall_time, frame_loops);
	    Set_message(MSG);
	}
#endif

	/*
	 * Object collision.
	 */
	switch (obj->type) {
	case OBJ_BALL:
	    if (! hit) {
		continue;
	    }
	    Player_collides_with_ball(ind, obj, radius);
	    if (BIT(pl->status, KILLED)) {
		return;
	    }
	    continue;

	case OBJ_ITEM:
	    Player_collides_with_item(ind, obj);
	    /* if life is non-zero then no collision occurred */
	    if (obj->life != 0) {
		continue;
	    }
	    break;

	case OBJ_MINE:
	    Player_collides_with_mine(ind, obj);
	    break;

	case OBJ_WRECKAGE:
	case OBJ_DEBRIS:
	    Player_collides_with_debris(ind, obj);
	    if (BIT(pl->status, KILLED)) {
		return;
	    }
	    break;

	case OBJ_ASTEROID:
	    if (hit) {
		Player_collides_with_asteroid(ind, WIRE_PTR(obj));
		Delta_mv_elastic((object *)pl, (object *)obj);
	    }
	    if (BIT(pl->status, KILLED)) {
		return;
	    }
	    continue;

	case OBJ_CANNON_SHOT:
	    /* don't explode cannon flak if it hits directly*/
	    CLR_BIT(obj->mods.warhead, CLUSTER);
	    break;

	case OBJ_PULSE:
	    Laser_pulse_hits_player(ind, PULSE_PTR(obj));
	    if (BIT(pl->status, KILLED)) {
		return;
	    }
	    continue;

	default:
	    break;
	}

	obj->life = 0;

	if (BIT(obj->type, KILLING_SHOTS)) {
	    Player_collides_with_killing_shot(ind, obj);
	    if (BIT(pl->status, KILLED)) {
		return;
	    }
	}

	if (hit)
	    Delta_mv((object *)pl, (object *)obj);
    }
}


static void Player_collides_with_ball(int ind, object *obj, int radius)
{
    player	*pl = Players[ind];
    DFLOAT	sc;
    int		killer;
    ballobject	*ball = BALL_PTR(obj);

    /*
     * The ball is special, usually players bounce off of it with
     * shields up, or die with shields down.  The treasure may
     * be destroyed.
     */
    Obj_repel((object *)pl, obj, PIXEL_TO_CLICK(radius));
    if (BIT(pl->used, (HAS_SHIELD|HAS_EMERGENCY_SHIELD))
	!= (HAS_SHIELD|HAS_EMERGENCY_SHIELD)) {
	Add_fuel(&(pl->fuel), (long)ED_BALL_HIT);
	if (treasureCollisionDestroys) {
	    ball->life = 0;
	}
    }
    if (pl->fuel.sum > 0) {
	if (!treasureCollisionMayKill || BIT(pl->used, HAS_SHIELD))
	    return;
	if (!BIT(pl->used, HAS_SHIELD) && BIT(pl->have, HAS_ARMOR)) {
	    Player_hit_armor(ind);
	    return;
	}
    }
    if (ball->owner == NO_ID) {
	sprintf(msg, "%s was killed by a ball.", pl->name);
	sc = Rate(0, pl->score)
		* ballKillScoreMult
		* unownedKillScoreMult;
	SCORE(ind, -sc, pl->pos.cx, pl->pos.cy, "Ball");
    } else {
	killer = GetInd[ball->owner];

	sprintf(msg, "%s was killed by a ball owned by %s.",
		pl->name, Players[killer]->name);

	if (killer == ind) {
	    strcat(msg, "  How strange!");
	    sc = Rate(0, pl->score)
		   * ballKillScoreMult
		   * selfKillScoreMult;
	    SCORE(ind, -sc, pl->pos.cx, pl->pos.cy, Players[killer]->name);
	} else {
	    Rank_AddKill(Players[killer]);
	    sc = Rate(Players[killer]->score, pl->score)
		       * ballKillScoreMult;
	    Score_players(killer, sc, pl->name,
			  ind, -sc, Players[killer]->name);
	    Robot_war(ind, killer);
	}
    }
    Set_message(msg);
    SET_BIT(pl->status, KILLED);
}


static void Player_collides_with_item(int ind, object *obj)
{
    player	*pl = Players[ind];
    int		old_have;
    enum Item	item_index;

    if (IsOffensiveItem((enum Item) obj->info)) {
	int off_items = CountOffensiveItems(pl);
	if (off_items >= maxOffensiveItems) {
	    /* Set_player_message(pl, "No space left for offensive items."); */
	    Delta_mv((object *)pl, obj);
	    return;
	}
	else if (obj->count > 1
		 && off_items + obj->count > maxOffensiveItems)
	    obj->count = maxOffensiveItems - off_items;
    }
    else if (IsDefensiveItem((enum Item) obj->info)) {
	int def_items = CountDefensiveItems(pl);
	if (def_items >= maxDefensiveItems) {
	    /* Set_player_message(pl, "No space for left for defensive items."); */
	    Delta_mv((object *)pl, obj);
	    return;
	}
	else if (obj->count > 1
		 && def_items + obj->count > maxDefensiveItems)
	    obj->count = maxDefensiveItems - def_items;
    }

    item_index = (enum Item) obj->info;

    switch (item_index) {
    case ITEM_WIDEANGLE:
	pl->item[item_index] += obj->count;
	LIMIT(pl->item[item_index], 0, World.items[item_index].limit);
	sound_play_sensors(pl->pos.cx, pl->pos.cy,
			   WIDEANGLE_SHOT_PICKUP_SOUND);
	break;
    case ITEM_ECM:
	pl->item[item_index] += obj->count;
	LIMIT(pl->item[item_index], 0, World.items[item_index].limit);
	sound_play_sensors(pl->pos.cx, pl->pos.cy, ECM_PICKUP_SOUND);
	break;
    case ITEM_ARMOR:
	pl->item[item_index]++;
	LIMIT(pl->item[item_index], 0, World.items[item_index].limit);
	if (pl->item[item_index] > 0)
	    SET_BIT(pl->have, HAS_ARMOR);
	sound_play_sensors(pl->pos.cx, pl->pos.cy,
			   ARMOR_PICKUP_SOUND);
	break;
    case ITEM_TRANSPORTER:
	pl->item[item_index] += obj->count;
	LIMIT(pl->item[item_index], 0, World.items[item_index].limit);
	sound_play_sensors(pl->pos.cx, pl->pos.cy, TRANSPORTER_PICKUP_SOUND);
	break;
    case ITEM_MIRROR:
	pl->item[ITEM_MIRROR] += obj->count;
	LIMIT(pl->item[item_index], 0, World.items[item_index].limit);
	if (pl->item[item_index] > 0)
	    SET_BIT(pl->have, HAS_MIRROR);
	sound_play_sensors(pl->pos.cx, pl->pos.cy, MIRROR_PICKUP_SOUND);
	break;
    case ITEM_DEFLECTOR:
	pl->item[ITEM_DEFLECTOR] += obj->count;
	LIMIT(pl->item[item_index], 0, World.items[item_index].limit);
	if (pl->item[item_index] > 0)
	    SET_BIT(pl->have, HAS_DEFLECTOR);
	sound_play_sensors(pl->pos.cx, pl->pos.cy, DEFLECTOR_PICKUP_SOUND);
	break;
    case ITEM_HYPERJUMP:
	pl->item[item_index] += obj->count;
	LIMIT(pl->item[item_index], 0, World.items[item_index].limit);
	sound_play_sensors(pl->pos.cx, pl->pos.cy, HYPERJUMP_PICKUP_SOUND);
	break;
    case ITEM_PHASING:
	pl->item[item_index] += obj->count;
	LIMIT(pl->item[item_index], 0, World.items[item_index].limit);
	if (pl->item[item_index] > 0)
	    SET_BIT(pl->have, HAS_PHASING_DEVICE);
	sound_play_sensors(pl->pos.cx, pl->pos.cy, PHASING_DEVICE_PICKUP_SOUND);
	break;
    case ITEM_SENSOR:
	pl->item[item_index] += obj->count;
	LIMIT(pl->item[item_index], 0, World.items[item_index].limit);
	pl->updateVisibility = 1;
	sound_play_sensors(pl->pos.cx, pl->pos.cy, SENSOR_PACK_PICKUP_SOUND);
	break;
    case ITEM_AFTERBURNER:
	pl->item[item_index] += obj->count;
	LIMIT(pl->item[item_index], 0, World.items[item_index].limit);
	if (pl->item[item_index] > 0)
	    SET_BIT(pl->have, HAS_AFTERBURNER);
	sound_play_sensors(pl->pos.cx, pl->pos.cy, AFTERBURNER_PICKUP_SOUND);
	break;
    case ITEM_REARSHOT:
	pl->item[item_index] += obj->count;
	LIMIT(pl->item[item_index], 0, World.items[item_index].limit);
	sound_play_sensors(pl->pos.cx, pl->pos.cy, BACK_SHOT_PICKUP_SOUND);
	break;
    case ITEM_MISSILE:
	pl->item[item_index] += obj->count;
	LIMIT(pl->item[item_index], 0, World.items[item_index].limit);
	sound_play_sensors(pl->pos.cx, pl->pos.cy, ROCKET_PACK_PICKUP_SOUND);
	break;
    case ITEM_CLOAK:
	pl->item[item_index] += obj->count;
	LIMIT(pl->item[item_index], 0, World.items[item_index].limit);
	if (pl->item[item_index] > 0)
	    SET_BIT(pl->have, HAS_CLOAKING_DEVICE);
	pl->updateVisibility = 1;
	sound_play_sensors(pl->pos.cx, pl->pos.cy, CLOAKING_DEVICE_PICKUP_SOUND);
	break;
    case ITEM_FUEL:
	Add_fuel(&(pl->fuel), ENERGY_PACK_FUEL);
	sound_play_sensors(pl->pos.cx, pl->pos.cy, ENERGY_PACK_PICKUP_SOUND);
	break;
    case ITEM_MINE:
	pl->item[item_index] += obj->count;
	LIMIT(pl->item[item_index], 0, World.items[item_index].limit);
	sound_play_sensors(pl->pos.cx, pl->pos.cy, MINE_PACK_PICKUP_SOUND);
	break;
    case ITEM_LASER:
	pl->item[item_index] += obj->count;
	LIMIT(pl->item[item_index], 0, World.items[item_index].limit);
	sound_play_sensors(pl->pos.cx, pl->pos.cy, LASER_PICKUP_SOUND);
	break;
    case ITEM_EMERGENCY_THRUST:
	pl->item[item_index] += obj->count;
	LIMIT(pl->item[item_index], 0, World.items[item_index].limit);
	if (pl->item[item_index] > 0)
	    SET_BIT(pl->have, HAS_EMERGENCY_THRUST);
	sound_play_sensors(pl->pos.cx, pl->pos.cy,
			   EMERGENCY_THRUST_PICKUP_SOUND);
	break;
    case ITEM_EMERGENCY_SHIELD:
	old_have = pl->have;
	pl->item[item_index] += obj->count;
	LIMIT(pl->item[item_index], 0, World.items[item_index].limit);
	if (pl->item[item_index] > 0)
	    SET_BIT(pl->have, HAS_EMERGENCY_SHIELD);
	sound_play_sensors(pl->pos.cx, pl->pos.cy,
			   EMERGENCY_SHIELD_PICKUP_SOUND);
	/*
	 * New feature since 3.2.7:
	 * If we're playing in a map where shields are not allowed
	 * and a player picks up her first emergency shield item
	 * then we'll immediately turn on emergency shield.
	 */
	if (!BIT(old_have, HAS_SHIELD | HAS_EMERGENCY_SHIELD)
	    && pl->item[ITEM_EMERGENCY_SHIELD] == 1) {
	    Emergency_shield(ind, true);
	}
	break;
    case ITEM_TRACTOR_BEAM:
	pl->item[item_index] += obj->count;
	LIMIT(pl->item[item_index], 0, World.items[item_index].limit);
	if (pl->item[item_index] > 0)
	    SET_BIT(pl->have, HAS_TRACTOR_BEAM);
	sound_play_sensors(pl->pos.cx, pl->pos.cy,
			   TRACTOR_BEAM_PICKUP_SOUND);
	break;
    case ITEM_AUTOPILOT:
	pl->item[item_index] += obj->count;
	LIMIT(pl->item[item_index], 0, World.items[item_index].limit);
	if (pl->item[item_index] > 0)
	    SET_BIT(pl->have, HAS_AUTOPILOT);
	sound_play_sensors(pl->pos.cx, pl->pos.cy,
			   AUTOPILOT_PICKUP_SOUND);
	break;

    case ITEM_TANK:
	if (pl->fuel.num_tanks < World.items[ITEM_TANK].limit) {
	    Player_add_tank(ind, TANK_FUEL(pl->fuel.num_tanks + 1));
	} else {
	    Add_fuel(&(pl->fuel), TANK_FUEL(MAX_TANKS));
	}
	sound_play_sensors(pl->pos.cx, pl->pos.cy, TANK_PICKUP_SOUND);
	break;
    case NUM_ITEMS:
	/* impossible */
	break;
    }

    obj->life = 0;
}


static void Player_collides_with_mine(int ind, object *obj)
{
    player	*pl = Players[ind];
    DFLOAT	sc;
    int		killer;
    mineobject	*mine = MINE_PTR(obj);

    sound_play_sensors(pl->pos.cx, pl->pos.cy, PLAYER_HIT_MINE_SOUND);
    killer = -1;
    if (mine->id == NO_ID && mine->owner == NO_ID) {
	sprintf(msg, "%s hit %s.",
		pl->name,
		Describe_shot(mine->type, mine->status, mine->mods, 1));
    }
    else if (mine->owner == mine->id) {
	killer = GetInd[mine->owner];
	sprintf(msg, "%s hit %s %s by %s.", pl->name,
		Describe_shot(mine->type, mine->status, mine->mods,1),
		BIT(mine->status, GRAVITY) ? "thrown " : "dropped ",
		Players[killer]->name);
    }
    else if (mine->owner == NO_ID) {
	const char *reprogrammer_name = "some jerk";
	if (mine->id != NO_ID) {
	    killer = GetInd[mine->id];
	    reprogrammer_name = Players[killer]->name;
	}
	sprintf(msg, "%s hit %s reprogrammed by %s.",
		pl->name,
		Describe_shot(mine->type, mine->status, mine->mods, 1),
		reprogrammer_name);
    }
    else {
	const char *reprogrammer_name = "some jerk";
	if (mine->id != NO_ID) {
	    killer = GetInd[mine->id];
	    reprogrammer_name = Players[killer]->name;
	}
	sprintf(msg, "%s hit %s %s by %s and reprogrammed by %s.",
		pl->name,
		Describe_shot(mine->type, mine->status, mine->mods,1),
		BIT(mine->status, GRAVITY) ? "thrown " : "dropped ",
		Players[GetInd[mine->owner]]->name,
		reprogrammer_name);
    }
    if (killer != -1) {
	/*
	 * Question with this is if we want to give the same points for
	 * a high-scored-player hitting a low-scored-player's mine as
	 * for a low-scored-player hitting a high-scored-player's mine.
	 * Maybe not.
	 */
	sc = Rate(Players[killer]->score, pl->score)
		   * mineScoreMult;
	Score_players(killer, sc, pl->name,
		      ind, -sc, Players[killer]->name);
    }
    Set_message(msg);
}


static void Player_collides_with_debris(int ind, object *obj)
{
    player		*pl = Players[ind];
    DFLOAT		v = VECTOR_LENGTH(obj->vel);
    long		tmp = (long) (2 * obj->mass * v);
    long		cost = ABS(tmp);
    int			killer;
    DFLOAT		sc;

    if (BIT(pl->used, (HAS_SHIELD|HAS_EMERGENCY_SHIELD))
	!= (HAS_SHIELD|HAS_EMERGENCY_SHIELD))
	Add_fuel(&pl->fuel, - cost);
    if (pl->fuel.sum == 0
	|| (obj->type == OBJ_WRECKAGE
	    && wreckageCollisionMayKill
	    && !BIT(pl->used, HAS_SHIELD)
	    && !BIT(pl->have, HAS_ARMOR))) {
	SET_BIT(pl->status, KILLED);
	sprintf(msg, "%s succumbed to an explosion.", pl->name);
	killer = -1;
	if (obj->id != NO_ID) {
	    killer = GetInd[obj->id];
	    sprintf(msg + strlen(msg) - 1, " from %s.",
		    Players[killer]->name);
	    if (obj->id == pl->id) {
		sprintf(msg + strlen(msg), "  How strange!");
	    }
	}
	Set_message(msg);
	if (killer == -1 || killer == ind) {
	    sc = Rate(0, pl->score)
		   * explosionKillScoreMult
		   * selfKillScoreMult;
	    SCORE(ind, -sc, pl->pos.cx, pl->pos.cy,
		  (killer == -1) ? "[Explosion]" : pl->name);
	} else {
	    Rank_AddKill(Players[killer]);
	    sc = Rate(Players[killer]->score, pl->score)
		       * explosionKillScoreMult;
	    Score_players(killer, sc, pl->name,
			  ind, -sc, Players[killer]->name);
	}
	obj->life = 0;
	return;
    }
    if (obj->type == OBJ_WRECKAGE
	&& wreckageCollisionMayKill
	&& !BIT(pl->used, HAS_SHIELD)
	&& BIT(pl->have, HAS_ARMOR)) {
	Player_hit_armor(ind);
    }
}


static void Player_collides_with_asteroid(int ind, wireobject *ast)
{
    player	*pl = Players[ind];
    DFLOAT	v = VECTOR_LENGTH(ast->vel);
    long	tmp = (long) (2 * ast->mass * v);
    long	cost = ABS(tmp);

    ast->life += ASTEROID_FUEL_HIT(ED_PL_CRASH, ast->size);
    if (ast->life < 0)
	ast->life = 0;
    if (ast->life == 0
	&& asteroidPoints > 0
	&& pl->score <= asteroidMaxScore) {
	SCORE(ind, asteroidPoints, ast->pos.cx, ast->pos.cy, "");
    }
    if (BIT(pl->used, (HAS_SHIELD|HAS_EMERGENCY_SHIELD))
	!= (HAS_SHIELD|HAS_EMERGENCY_SHIELD)) {
	Add_fuel(&pl->fuel, -cost);
    }
    if (asteroidCollisionMayKill
	&& (pl->fuel.sum == 0
	    || (!BIT(pl->used, HAS_SHIELD)
		&& !BIT(pl->have, HAS_ARMOR)))) {
	DFLOAT sc;
	SET_BIT(pl->status, KILLED);
	if (pl->velocity > v) {
	    /* player moves faster than asteroid */
	    sprintf(msg, "%s smashed into an asteroid.", pl->name);
	} else {
	    sprintf(msg, "%s was hit by an asteroid.", pl->name);
	}
	Set_message(msg);
	sc = Rate(0, pl->score) * unownedKillScoreMult;
	SCORE(ind, -sc, pl->pos.cx, pl->pos.cy, "[Asteroid]");
	if (IS_TANK_PTR(pl) && asteroidPoints > 0) {
	    int owner = GetInd[pl->lock.pl_id];
	    if (Players[owner]->score <= asteroidMaxScore) {
		SCORE(owner, asteroidPoints, ast->pos.cx, ast->pos.cy, "");
	    }
	}
	return;
    }
    if (asteroidCollisionMayKill
	&& !BIT(pl->used, HAS_SHIELD)
	&& BIT(pl->have, HAS_ARMOR)) {
	Player_hit_armor(ind);
    }
}


static void Player_collides_with_killing_shot(int ind, object *obj)
{
    player	*pl = Players[ind];
    DFLOAT	sc;
    DFLOAT   	drainfactor;
    long	drain;
    int		killer = NO_ID;

    /*
     * Player got hit by a potentially deadly object.
     *
     * When a player has shields up, and not enough fuel
     * to `absorb' the shot then shields are lowered.
     * This is not very logical, rather in this case
     * the shot should be considered to be deadly too.
     *
     * Sound effects are missing when shot is deadly.
     */

    if (BIT(pl->used, HAS_SHIELD)
	|| BIT(pl->have, HAS_ARMOR)
	|| (obj->type == OBJ_TORPEDO
	    && BIT(obj->mods.nuclear, NUCLEAR)
	    && (int)(rfrac() >= 0.25f))) {
	switch (obj->type) {
	case OBJ_TORPEDO:
	    sound_play_sensors(pl->pos.cx, pl->pos.cy,
			       PLAYER_EAT_TORPEDO_SHOT_SOUND);
	    break;
	case OBJ_HEAT_SHOT:
	    sound_play_sensors(pl->pos.cx, pl->pos.cy,
			       PLAYER_EAT_HEAT_SHOT_SOUND);
	    break;
	case OBJ_SMART_SHOT:
	    sound_play_sensors(pl->pos.cx, pl->pos.cy,
			       PLAYER_EAT_SMART_SHOT_SOUND);
	    break;
	}

	switch(obj->type) {
	case OBJ_TORPEDO:
	case OBJ_HEAT_SHOT:
	case OBJ_SMART_SHOT:
	    if (obj->id == NO_ID)
		sprintf(msg, "%s ate %s.", pl->name,
			Describe_shot(obj->type, obj->status,
				      obj->mods, 1));
	    else
		sprintf(msg, "%s ate %s from %s.", pl->name,
			Describe_shot(obj->type, obj->status,
				      obj->mods, 1),
			Players[ killer=GetInd[obj->id] ]->name);
	    drain = (long)(ED_SMART_SHOT_HIT /
		((obj->mods.mini + 1) * (obj->mods.power + 1)));
	    if (BIT(pl->used, (HAS_SHIELD|HAS_EMERGENCY_SHIELD))
		!= (HAS_SHIELD|HAS_EMERGENCY_SHIELD))
		Add_fuel(&(pl->fuel), drain);
	    pl->forceVisible += 2;
	    Set_message(msg);
	    break;

	case OBJ_SHOT:
	case OBJ_CANNON_SHOT:
	    sound_play_sensors(pl->pos.cx, pl->pos.cy,
			       PLAYER_EAT_SHOT_SOUND);
	    if (BIT(pl->used, (HAS_SHIELD|HAS_EMERGENCY_SHIELD))
		!= (HAS_SHIELD|HAS_EMERGENCY_SHIELD)) {
		if (shotHitFuelDrainUsesKineticEnergy) {
		    DFLOAT rel_velocity = LENGTH(pl->vel.x - obj->vel.x,
						 pl->vel.y - obj->vel.y);
		    drainfactor = (rel_velocity * rel_velocity * ABS(obj->mass))
				  / (ShotsSpeed * ShotsSpeed * ShotsMass);
		} else {
		   drainfactor = 1.0f;
		}
		drain = (long)(ED_SHOT_HIT * drainfactor * SHOT_MULT(obj));
		Add_fuel(&(pl->fuel), drain);
	    }
	    pl->forceVisible += SHOT_MULT(obj);
	    break;

	default:
	    xpprintf("%s You were hit by what?\n", showtime());
	    break;
	}
	if (pl->fuel.sum <= 0) {
	    CLR_BIT(pl->used, HAS_SHIELD);
	}
	if (!BIT(pl->used, HAS_SHIELD) && BIT(pl->have, HAS_ARMOR)) {
	    Player_hit_armor(ind);
	}
    } else {
	DFLOAT factor;
	switch (obj->type) {
	case OBJ_TORPEDO:
	case OBJ_SMART_SHOT:
	case OBJ_HEAT_SHOT:
	case OBJ_SHOT:
	case OBJ_CANNON_SHOT:
	    if (BIT(obj->status, FROMCANNON)) {
		sound_play_sensors(pl->pos.cx, pl->pos.cy,
				   PLAYER_HIT_CANNONFIRE_SOUND);
		sprintf(msg, "%s was hit by cannonfire.", pl->name);
		sc = Rate(CANNON_SCORE, pl->score)/4;
	    } else if (obj->id == NO_ID) {
		sprintf(msg, "%s was killed by %s.", pl->name,
			Describe_shot(obj->type, obj->status,
				      obj->mods, 1));
		sc = Rate(0, pl->score) * unownedKillScoreMult;
	    } else {
		sprintf(msg, "%s was killed by %s from %s.", pl->name,
			Describe_shot(obj->type, obj->status,
				      obj->mods, 1),
			Players[killer=GetInd[obj->id]]->name);
		if (killer == ind) {
		    sound_play_sensors(pl->pos.cx, pl->pos.cy,
				       PLAYER_SHOT_THEMSELF_SOUND);
		    strcat(msg, "  How strange!");
		    sc = Rate(0, pl->score) * selfKillScoreMult;
		} else {
		    Rank_AddKill(Players[killer]);
		    sc = Rate(Players[killer]->score, pl->score);
		}
	    }
	    switch (obj->type) {
	    case OBJ_SHOT:
		if (BIT(obj->mods.warhead, CLUSTER)) {
		    factor = clusterKillScoreMult;
		} else {
		    factor = shotKillScoreMult;
		}
		break;
	    case OBJ_TORPEDO:
		factor = torpedoKillScoreMult;
		break;
	    case OBJ_SMART_SHOT:
		factor = smartKillScoreMult;
		break;
	    case OBJ_HEAT_SHOT:
		factor = heatKillScoreMult;
		break;
	    default:
		factor = shotKillScoreMult;
		break;
	    }
	    sc *= factor;
	    if (BIT(obj->status, FROMCANNON)) {
		SCORE(ind, -sc, pl->pos.cx, pl->pos.cy, "Cannon");
		if (BIT(World.rules->mode, TEAM_PLAY)
		    && pl->team != obj->team)
		    TEAM_SCORE(obj->team, sc);
	    } else if (obj->id == NO_ID || killer == ind) {
		SCORE(ind, -sc, pl->pos.cx, pl->pos.cy,
		      (obj->id == NO_ID ? "" : pl->name));
	    } else {
		Score_players(killer, sc, pl->name,
			      ind, -sc, Players[killer]->name);
		Robot_war(ind, killer);
	    }
	    Set_message(msg);
	    SET_BIT(pl->status, KILLED);
	    return;

	default:
	    break;
	}
    }
}

static void Player_pass_checkpoint(int ind)
{
    player	*pl = Players[ind];
    int		j;

    if (pl->check == 0) {
	pl->round++;
#if 1
	pl->last_lap_time = pl->time - pl->last_lap;
	if ((pl->best_lap > pl->last_lap_time
	     || pl->best_lap == 0)
	    && pl->time != 0
	    && pl->round != 1) {
	    pl->best_lap = pl->last_lap_time;
	}
	pl->last_lap = pl->time;
	if (pl->round > raceLaps) {
	    if (ballrace) {
		/* Balls are made unowned when their owner finishes the race
		   This way, they can be reused by other players */
		for (j = 0; j < NumObjs; j++) {
		    if (Obj[j]->type == OBJ_BALL) {
			ballobject	*ball = BALL_PTR(Obj[j]);
			if (ball->owner == pl->id)
			    ball->owner = NO_ID;
		    }
		}
	    }
	    Player_death_reset(ind);
	    pl->mychar = 'D';
	    SET_BIT(pl->status, GAME_OVER|FINISH);
	    sprintf(msg,
		    "%s finished the race. Last lap time: %.2fs. "
		    "Personal race best lap time: %.2fs.",
		    pl->name,
		    (DFLOAT) pl->last_lap_time / FPS,
		    (DFLOAT) pl->best_lap / FPS);
	} else if (pl->round > 1) {
	    sprintf(msg,
		    "%s completes lap %d in %.2fs. "
		    "Personal race best lap time: %.2fs.",
		    pl->name,
		    pl->round-1,
		    (DFLOAT) pl->last_lap_time / FPS,
		    (DFLOAT) pl->best_lap / FPS);
	} else {
	    sprintf(msg, "%s starts lap 1 of %d", pl->name, raceLaps);
	}
	Set_message(msg);
#else
	/* this is how 4.3.1X did this */
	SET_BIT(pl->status, FINISH);
	/* Rest done in Compute_game_status() */
#endif
    }

    if (++pl->check == World.NumChecks)
	pl->check = 0;
    pl->last_check_dir = pl->dir;

    updateScores = true;
}


static void AsteroidCollision(void)
{
    int		j, radius, obj_count, hit;
    object	*ast;
    object	*obj = NULL, **obj_list;
    list_t	list;
    list_iter_t	iter;
    DFLOAT	damage = 0;
    bool	sound = false;

    list = Asteroid_get_list();
    if (!list) {
	return;
    }

    for (iter = List_begin(list); iter != List_end(list); LI_FORWARD(iter)) {
	ast = LI_DATA(iter);

	assert(BIT(ast->type, OBJ_ASTEROID));

	if (ast->life <= 0) {
	    continue;
	}

	assert(OBJ_X_IN_BLOCKS(ast) >= 0);
	assert(OBJ_X_IN_BLOCKS(ast) < World.x);
	assert(OBJ_Y_IN_BLOCKS(ast) >= 0);
	assert(OBJ_Y_IN_BLOCKS(ast) < World.y);

	Cell_get_objects(OBJ_X_IN_BLOCKS(ast), OBJ_Y_IN_BLOCKS(ast),
			 ast->pl_radius / BLOCK_SZ + 1, 300,
			 &obj_list, &obj_count);

	for (j = 0; j < obj_count; j++) {
	    obj = obj_list[j];
	    assert(obj != NULL);

	    if (is_polygon_map || !useOldCode) {
		;
	    } else {
		if (obj->life <= 0)
		    continue;
	    }

	    /* asteroids don't hit these objects */
	    if (BIT(obj->type, OBJ_ITEM|OBJ_DEBRIS|OBJ_SPARK|OBJ_WRECKAGE)
		&& obj->id == NO_ID
		&& !BIT(obj->status, FROMCANNON))
		continue;
	    /* don't collide while still overlapping  after breaking */
	    if (obj->type == OBJ_ASTEROID && frame_time < ast->fusetime)
		continue;
	    /* don't collide with self */
	    if (obj == ast)
		continue;
	    /* don't collide with phased balls */
	    if (BIT(obj->type, OBJ_BALL)
		&& obj->id != NO_ID
		&& BIT(Players[GetInd[obj->id]]->used, HAS_PHASING_DEVICE))
		continue;

	    radius = ast->pl_radius + obj->pl_radius;
	    if (is_polygon_map || !useOldCode) {
		switch (obj->collmode) {
		case 0:
		    hit = in_range_simple(ast->pos.cx, ast->pos.cy,
					  obj->pos.cx, obj->pos.cy,
					  radius * CLICK);
		    break;
		case 1:
		    hit = in_range_acd(ast->prevpos.cx - obj->prevpos.cx,
				       ast->prevpos.cy - obj->prevpos.cy,
				       ast->extmove.cx - obj->extmove.cx,
				       ast->extmove.cy - obj->extmove.cy,
				       radius * CLICK);
		    break;
		case 2:
		    hit = in_range_partial(ast->prevpos.cx - obj->prevpos.cx,
					   ast->prevpos.cy - obj->prevpos.cy,
					   ast->extmove.cx - obj->extmove.cx,
					   ast->extmove.cy - obj->extmove.cy,
					   radius * CLICK, obj->wall_time);
		    break;
		case 3:
		default:
#if 0
		    warn("Unimplemented collision mode %d", obj->collmode);
#endif
		    continue;
		}
		if (!hit)
		    continue;
	    } else {
		if (!in_range_acd_old(ast->prevpos.cx, ast->prevpos.cy,
				      ast->pos.cx, ast->pos.cy,
				      obj->prevpos.cx, obj->prevpos.cy,
				      obj->pos.cx, obj->pos.cy,
				      radius * CLICK)) {
		    continue;
		}
	    }

	    switch (obj->type) {
	    case OBJ_BALL:
		Obj_repel(ast, obj, PIXEL_TO_CLICK(radius));
		if (treasureCollisionDestroys)
		    obj->life = 0;
		damage = ED_BALL_HIT;
		sound = true;
		break;
	    case OBJ_ASTEROID:
		obj->life -= ASTEROID_FUEL_HIT(ABS(2 * ast->mass
					           * VECTOR_LENGTH(ast->vel)),
				               WIRE_PTR(obj)->size);
		damage = -ABS(2 * obj->mass * VECTOR_LENGTH(obj->vel));
		Delta_mv_elastic(ast, obj);
		/* avoid doing collision twice */
		obj->fusetime = frame_time + timeStep;
		sound = true;
		break;
	    case OBJ_SPARK:
		obj->life = 0;
		Delta_mv(ast, obj);
		damage = 0;
		break;
	    case OBJ_DEBRIS:
	    case OBJ_WRECKAGE:
		obj->life = 0;
		damage = -ABS(2 * obj->mass * VECTOR_LENGTH(obj->vel));
		Delta_mv(ast, obj);
		break;
	    case OBJ_MINE:
		if (!BIT(obj->status, CONFUSED))
		    obj->life = 0;
		break;
	    case OBJ_SHOT:
	    case OBJ_CANNON_SHOT:
		obj->life = 0;
		Delta_mv(ast, obj);
		damage = ED_SHOT_HIT;
		sound = true;
		break;
	    case OBJ_SMART_SHOT:
	    case OBJ_TORPEDO:
	    case OBJ_HEAT_SHOT:
		obj->life = 0;
		Delta_mv(ast, obj);
		damage = ED_SMART_SHOT_HIT
			 / ((obj->mods.mini + 1) * (obj->mods.power + 1));
		sound = true;
		break;
	    case OBJ_PULSE:
		obj->life = 0;
		damage = ED_LASER_HIT;
		sound = true;
		break;
	    default:
		Delta_mv(ast, obj);
		damage = 0;
		break;
	    }

	    if (ast->life > 0) {
		/* kps - this is some strange sort of hack - fix it*/
		/*if (ast->life <= ast->fuselife) {*/
		ast->life += ASTEROID_FUEL_HIT(damage, WIRE_PTR(ast)->size);
		/*}*/
		if (sound) {
		    sound_play_sensors(ast->pos.cx, ast->pos.cy,
				       ASTEROID_HIT_SOUND);
		}
		if (ast->life < 0) {
		    ast->life = 0;
		}
		if (ast->life == 0) {
		    if (asteroidPoints > 0
			&& (obj->id != NO_ID
			    || (obj->type == OBJ_BALL
				&& BALL_PTR(obj)->owner != NO_ID))) {
			int owner_id = ((obj->type == OBJ_BALL)
					? BALL_PTR(obj)->owner
					: obj->id);
			int ind = GetInd[owner_id];
			if (Players[ind]->score <= asteroidMaxScore) {
			    SCORE(ind, asteroidPoints,
				  ast->pos.cx, ast->pos.cy, "");
			}
		    }

		    /* break; */
		}
	    }
	}
    }
}


/* do ball - object and ball - checkpoint collisions */
static void BallCollision(void)
{
    int         i, j, obj_count;
    int		ignored_object_types;
    object    **obj_list;
    object     *obj;
    ballobject *ball;

    /*
     * These object types ignored;
     * some are handled by other code,
     * some don't interact.
     */
    ignored_object_types = OBJ_PLAYER | OBJ_ASTEROID | OBJ_MINE | OBJ_ITEM;
    if (!ballSparkCollisions) {
	ignored_object_types |= OBJ_SPARK;
    }

    for (i = 0; i < NumObjs; i++) {
	ball = BALL_IND(i);

	/* ignore if: */
	if (ball->type != OBJ_BALL ||	/* not a ball */
	    ball->life <= 0 ||		/* dying ball */
	    (ball->id != NO_ID
	     && BIT(Players[GetInd[ball->id]]->used, HAS_PHASING_DEVICE)) ||
					/* phased ball */
	    World.treasures[ball->treasure].have) {
					/* safe in a treasure */
	    continue;
	}

	/* Ball - checkpoint */
	if (BIT(World.rules->mode, TIMING)
	    && ballrace
	    && ball->owner != NO_ID) {
	    int owner_ind = GetInd[ball->owner];
	    player *owner = Players[owner_ind];

	    if (!ballrace_connect || ball->id == owner->id) {
		if (Wrap_length(ball->pos.cx - World.check[owner->check].cx,
				ball->pos.cy - World.check[owner->check].cy)
		    < checkpointRadius * BLOCK_CLICKS) {
		    Player_pass_checkpoint(owner_ind);
		}
	    }
	}

	/* Ball - object */
	if (!ballCollisions)
	    continue;

	Cell_get_objects(OBJ_X_IN_BLOCKS(ball), OBJ_Y_IN_BLOCKS(ball),
			 4, 300,
			 &obj_list, &obj_count);

	for (j = 0; j < obj_count; j++) {
	    int r, hit;

	    obj = obj_list[j];

	    if (BIT(obj->type, ignored_object_types))
		continue;

	    if (is_polygon_map || !useOldCode) {
		;
	    } else {
		if (obj->life <= 0)
		    continue;
	    }

	    /* have we already done this ball pair? */
	    if (obj->type == OBJ_BALL && obj <= OBJ_PTR(ball)) {
		continue;
	    }

	    r = (ball->pl_radius + obj->pl_radius) * CLICK;
	    if (is_polygon_map || !useOldCode) {
		switch (obj->collmode) {
		case 0:
		    hit = in_range_simple(ball->pos.cx, ball->pos.cy,
					  obj->pos.cx, obj->pos.cy,
					  r);
		    break;
		case 1:
		    hit = in_range_acd(ball->prevpos.cx - obj->prevpos.cx,
				       ball->prevpos.cy - obj->prevpos.cy,
				       ball->extmove.cx - obj->extmove.cx,
				       ball->extmove.cy - obj->extmove.cy,
				       r);
		    break;
		case 2:
		    hit = in_range_partial(ball->prevpos.cx - obj->prevpos.cx,
					   ball->prevpos.cy - obj->prevpos.cy,
					   ball->extmove.cx - obj->extmove.cx,
					   ball->extmove.cy - obj->extmove.cy,
					   r, obj->wall_time);
		    break;
		default:
		    warn("Unimplemented collision mode %d", obj->collmode);
		    continue;
		}
		if (!hit)
		    continue;
	    } else {
		if (!in_range_acd_old(ball->prevpos.cx, ball->prevpos.cy,
				      ball->pos.cx, ball->pos.cy,
				      obj->prevpos.cx, obj->prevpos.cy,
				      obj->pos.cx, obj->pos.cy,
				      r)) {
		    continue;
		}
	    }

	    /* bang! */

	    switch (obj->type) {
	    case OBJ_BALL:
		/* Balls bounce off other balls that aren't safe in
		 * the treasure: */
		{
		    ballobject *b2 = BALL_PTR(obj);
		    if (World.treasures[b2->treasure].have) {
			break;
		    }
		    if (b2->id != NO_ID
			&& BIT(Players[GetInd[b2->id]]->used, HAS_PHASING_DEVICE)) {
			break;
		    }
		}

		/* if the collision was too violent, destroy ball and object */
		if ((sqr(ball->vel.x - obj->vel.x) +
		     sqr(ball->vel.y - obj->vel.y)) >
		    sqr(maxObjectWallBounceSpeed)) {
		    ball->life = 0;
		    obj->life  = 0;
		} else {
		    /* they bounce */
		    Obj_repel((object*)ball, obj,
			      PIXEL_TO_CLICK(ball->pl_radius + obj->pl_radius));
		}
		break;

	    /* balls absorb and destroy all other objects: */
	    case OBJ_SPARK:
	    case OBJ_TORPEDO:
	    case OBJ_SMART_SHOT:
	    case OBJ_HEAT_SHOT:
	    case OBJ_SHOT:
	    case OBJ_CANNON_SHOT:
	    case OBJ_DEBRIS:
	    case OBJ_WRECKAGE:
		Delta_mv(OBJ_PTR(ball), obj);
		obj->life = 0;
		break;
	    }
	}
    }
}


/* do mine - object collisions */
static void MineCollision(void)
{
    int		i, j, obj_count;
    object	**obj_list;
    object	*obj;
    mineobject	*mine;
    int		collide_object_types;

    if (!mineShotDetonateDistance)
	return;

    /*
     * These object types ignored;
     * some are handled by other code,
     * some don't interact.
     */
    collide_object_types = OBJ_SHOT |
			   OBJ_TORPEDO |
			   OBJ_SMART_SHOT |
			   OBJ_HEAT_SHOT |
			   OBJ_CANNON_SHOT;

    for (i = 0; i < NumObjs; i++) {
	mine = MINE_IND(i);

	/* ignore if: */
	if (mine->type != OBJ_MINE ||	/* not a mine */
	    mine->life <= 0) {		/* dying mine */
	    continue;
	}

	Cell_get_objects(OBJ_X_IN_BLOCKS(mine), OBJ_Y_IN_BLOCKS(mine),
			 4, 300,
			 &obj_list, &obj_count);

	for (j = 0; j < obj_count; j++) {
	    int r, hit;

	    obj = obj_list[j];

	    if (!BIT(obj->type, collide_object_types))
		continue;

	    r = PIXEL_TO_CLICK(mineShotDetonateDistance + obj->pl_radius);

	    if (is_polygon_map || !useOldCode) {
		switch (obj->collmode) {
		case 0:
		    hit = in_range_simple(mine->pos.cx, mine->pos.cy,
					  obj->pos.cx, obj->pos.cy,
					  r);
		    break;
		case 1:
		    hit = in_range_acd(mine->prevpos.cx - obj->prevpos.cx,
				       mine->prevpos.cy - obj->prevpos.cy,
				       mine->extmove.cx - obj->extmove.cx,
				       mine->extmove.cy - obj->extmove.cy,
				       r);
		    break;
		case 2:
		    hit = in_range_partial(mine->prevpos.cx - obj->prevpos.cx,
					   mine->prevpos.cy - obj->prevpos.cy,
					   mine->extmove.cx - obj->extmove.cx,
					   mine->extmove.cy - obj->extmove.cy,
					   r, obj->wall_time);
		    break;
		case 3:
		default:
#if 0
		    warn("Unimplemented collision mode %d", obj->collmode);
#endif
		    continue;
		}
		if (!hit)
		    continue;
	    } else {
		if (obj->life <= 0)
		    continue;

		if (!in_range_acd_old(mine->prevpos.cx, mine->prevpos.cy,
				      mine->pos.cx, mine->pos.cy,
				      obj->prevpos.cx, obj->prevpos.cy,
				      obj->pos.cx, obj->pos.cy, r)) {
		    continue;
		}
	    }

	    /* bang! */
	    obj->life = 0;
	    mine->life = 0;
	    break;
	}
    }
}


int wormXY(int x, int y)
{
    return Map_get_itemid(x, y);
}
