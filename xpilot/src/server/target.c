/* 
 * XPilotNG, an XPilot-like multiplayer space war game.
 *
 * Copyright (C) 1991-2001 by
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "xpserver.h"

static char msg[MSG_LEN];

void Target_update(world_t *world)
{
    int i, j;

    for (i = 0; i < world->NumTargets; i++) {
	target_t *targ = Targets(world, i);

	if (targ->dead_ticks > 0) {
	    if ((targ->dead_ticks -= timeStep) <= 0) {
		World_restore_target(world, targ);

		if (options.targetSync) {
		    for (j = 0; j < world->NumTargets; j++) {
			target_t *t = Targets(world, j);

			if (t->team == targ->team)
			    World_restore_target(world, t);
		    }
		}
	    }
	    continue;
	}
	else if (targ->damage == TARGET_DAMAGE)
	    continue;

	targ->damage += TARGET_REPAIR_PER_FRAME * timeStep;
	if (targ->damage >= TARGET_DAMAGE)
	    targ->damage = TARGET_DAMAGE;
	else if (targ->last_change + TARGET_UPDATE_DELAY < frame_loops)
	    /*
	     * We don't send target info to the clients every frame
	     * if the latest repair wouldn't change their display.
	     */
	    continue;

	targ->conn_mask = 0;
	targ->last_change = frame_loops;
    }
}

void Object_hits_target(object_t *obj, target_t *targ, double player_cost)
{
    int			j;
    player_t		*kp;
    double		sc, por,
			win_score = 0.0,
			lose_score = 0.0;
    int			win_team_members = 0,
			lose_team_members = 0,
			somebody_flag = 0,
			targets_remaining = 0,
			targets_total = 0;
    double 		drainfactor;
    vector_t 		zero_vel = {0.0, 0.0};
    world_t *world = &World;

    /* a normal shot or a direct mine hit work, cannons don't */
    /* KK: should shots/mines by cannons of opposing teams work? */
    /* also players suiciding on target will cause damage */
    if (!BIT(obj->type, KILLING_SHOTS|OBJ_MINE|OBJ_PULSE|OBJ_PLAYER))
	return;

    if (obj->id == NO_ID)
	return;
    assert(obj->id >= 0);
    kp = Player_by_id(obj->id);

    /* Targets are always team immune. */
    if (targ->team != TEAM_NOT_SET && targ->team == obj->team)
	return;

    switch(obj->type) {
    case OBJ_SHOT:
	if (options.shotHitFuelDrainUsesKineticEnergy) {
	    drainfactor = VECTOR_LENGTH(obj->vel);
	    drainfactor = (drainfactor * drainfactor * ABS(obj->mass))
			  / (options.shotSpeed * options.shotSpeed
			     * options.shotMass);
	} else
	    drainfactor = 1.0;
	targ->damage += ED_SHOT_HIT * drainfactor * SHOT_MULT(obj);
	break;
    case OBJ_PULSE:
	targ->damage += ED_LASER_HIT;
	break;
    case OBJ_SMART_SHOT:
    case OBJ_TORPEDO:
    case OBJ_HEAT_SHOT:
	if (!obj->mass)
	    /* happens at end of round reset. */
	    return;
	if (BIT(obj->mods.nuclear, NUCLEAR))
	    targ->damage = 0.0;
	else
	    targ->damage += ED_SMART_SHOT_HIT / (obj->mods.mini + 1);
	break;
    case OBJ_MINE:
	if (!obj->mass)
	    /* happens at end of round reset. */
	    return;
	targ->damage -= TARGET_DAMAGE / (obj->mods.mini + 1);
	break;
    case OBJ_PLAYER:
	if (player_cost <= 0.0 || player_cost > TARGET_DAMAGE / 4.0)
	    player_cost = TARGET_DAMAGE / 4.0;
	targ->damage -= player_cost;
	break;

    default:
	/*???*/
	break;
    }

    targ->conn_mask = 0;
    targ->last_change = frame_loops;
    if (targ->damage > 0.0)
	return;

    World_remove_target(world, targ);

    Make_debris(world,
		targ->pos,
		zero_vel,
		NO_ID,
		targ->team,
		OBJ_DEBRIS,
		4.5,
		GRAVITY,
		RED,
		6,
		(int)(75 + 75 * rfrac()),
		0, RES-1,
		20.0, 70.0,
		10.0, 100.0);

    if (BIT(world->rules->mode, TEAM_PLAY)) {
	for (j = 0; j < NumPlayers; j++) {
	    player_t *pl_j = Players(j);

	    if (Player_is_tank(pl_j)
		|| (BIT(pl_j->status, PAUSE) && pl_j->pause_count <= 0)
		|| Player_is_waiting(pl_j))
		continue;

	    if (pl_j->team == targ->team) {
		lose_score += pl_j->score;
		lose_team_members++;
		if (BIT(pl_j->status, GAME_OVER) == 0)
		    somebody_flag = 1;
	    }
	    else if (pl_j->team == kp->team) {
		win_score += pl_j->score;
		win_team_members++;
	    }
	}
    }
    if (somebody_flag) {
	for (j = 0; j < world->NumTargets; j++) {
	    target_t *t = Targets(world, j);

	    if (t->team == targ->team) {
		targets_total++;
		if (t->dead_ticks <= 0)
		    targets_remaining++;
	    }
	}
    }
    if (!somebody_flag)
	return;

    sound_play_sensors(targ->pos, DESTROY_TARGET_SOUND);

    if (targets_remaining > 0) {
	sc = Rate(kp->score, CANNON_SCORE)/4;
	sc = sc * (targets_total - targets_remaining) / (targets_total + 1);
	if (sc >= 0.01)
	    Score(kp, sc, targ->pos, "Target: ");
	/*
	 * If players can't collide with their own targets, we
	 * assume there are many used as shields.  Don't litter
	 * the game with the message below.
	 */
	if (options.targetTeamCollision && targets_total < 10) {
	    sprintf(msg, "%s blew up one of team %d's targets.",
		    kp->name, targ->team);
	    Set_message(msg);
	}
	return;
    }

    sprintf(msg, "%s blew up team %d's %starget.",
	    kp->name, targ->team, (targets_total > 1) ? "last " : "");
    Set_message(msg);

    if (options.targetKillTeam)
	Rank_add_target_kill(kp);

    sc  = Rate(win_score, lose_score);
    por = (sc * lose_team_members) /win_team_members;

    for (j = 0; j < NumPlayers; j++) {
	player_t *pl = Players(j);

	if (Player_is_tank(pl)
	    || (BIT(pl->status, PAUSE) && pl->pause_count <= 0)
	    || Player_is_waiting(pl))
	    continue;

	if (pl->team == targ->team) {
	    if (options.targetKillTeam
		&& targets_remaining == 0
		&& !BIT(pl->status, KILLED|PAUSE|GAME_OVER))
		SET_BIT(pl->status, KILLED);
	    Score(pl, -sc, targ->pos, "Target: ");
	}
	else if (pl->team == kp->team &&
		 (pl->team != TEAM_NOT_SET || pl->id == kp->id))
	    Score(pl, por, targ->pos, "Target: ");
    }
}

hitmask_t Target_hitmask(target_t *targ)
{
    /* target is destroyed - nothing hits */
    if (targ->dead_ticks > 0)
	return ALL_BITS;

    /* everything hits targets that don't belong to a team */
    if (targ->team == TEAM_NOT_SET)
	return 0;

    /* if options.targetTeamCollision is true, everything hits a target */
    if (options.targetTeamCollision)
	return 0;

    /* note that targets are always team immune */
    return HITMASK(targ->team);
}

void Target_set_hitmask(int group, target_t *targ)
{
    assert(targ->group == group);
    P_set_hitmask(targ->group, Target_hitmask(targ));
}

void Target_init(world_t *world)
{
    int group;

    for (group = 0; group < num_groups; group++) {
	group_t *gp = groupptr_by_id(group);

	if (gp->type == TARGET)
	    Target_set_hitmask(group, Targets(world, gp->mapobj_ind));
    }

#if 0
    P_grouphack(TARGET, Target_set_hitmask);
#endif
}

void World_restore_target(world_t *world, target_t *targ)
{
    blkpos_t blk = Clpos_to_blkpos(targ->pos);

    World_set_block(world, blk, TARGET);

    targ->conn_mask = 0;
    targ->update_mask = (unsigned)-1;
    targ->last_change = frame_loops;
    targ->dead_ticks = 0;
    targ->damage = TARGET_DAMAGE;

    P_set_hitmask(targ->group, Target_hitmask(targ));
}

void World_remove_target(world_t *world, target_t *targ)
{
    blkpos_t blk = Clpos_to_blkpos(targ->pos);

    targ->update_mask = (unsigned) -1;
    /* is this necessary? (done also in Target_restore_on_map() ) */
    targ->damage = TARGET_DAMAGE;
    targ->dead_ticks = options.targetDeadTicks;

    /*
     * Destroy target.
     * Turn it into a space to simplify other calculations.
     */
    World_set_block(world, blk, SPACE);

    /*P_set_hitmask(targ->group, ALL_BITS);*/
    P_set_hitmask(targ->group, Target_hitmask(targ));
}