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


char laser_version[] = VERSION;


/*
 * Do what needs to be done when a laser pulse
 * actually hits a player.
 */
void Laser_pulse_hits_player(player *pl, pulseobject *pulse)
{
    player		*kp;
    double		sc;
    char		msg[MSG_LEN];

    kp = Player_by_id(pulse->id);

    pl->forceVisible += 1;
    if (BIT(pl->have, HAS_MIRROR)
	&& (rfrac() * (2 * pl->item[ITEM_MIRROR])) >= 1) {
	/*pulse->pos.cx = cx - tcos(pulse->dir) * 0.5 * PULSE_SAMPLE_DISTANCE;
	  pulse->pos.cy = cy - tsin(pulse->dir) * 0.5 * PULSE_SAMPLE_DISTANCE;*/
	/* is this ok ? */
	pulse->dir = (int)(Wrap_cfindDir(pl->pos.cx - pulse->pos.cx,
					 pl->pos.cy - pulse->pos.cy)
			   * 2 - RES / 2 - pulse->dir);
	pulse->dir = MOD2(pulse->dir, RES);
	
	pulse->vel.x = pulseSpeed * tcos(pulse->dir);
	pulse->vel.y = pulseSpeed * tsin(pulse->dir);

	pulse->life += pl->item[ITEM_MIRROR];
	pulse->len = 0 /*PULSE_LENGTH*/;
	pulse->refl = true;
	/**refl = true;*/
	return;
    }

    sound_play_sensors(pl->pos, PLAYER_EAT_LASER_SOUND);
    if (Player_used_emergency_shield(pl))
	return;
    if (pulse->type != OBJ_PULSE) {
	/* kps -remove */
	warn("Player_collides_with_laser_pulse: "
	     "(pulse->type != OBJ_PULSE)\n");
	return;
    }
#if 0
    if (!BIT(pulse->type, KILLING_SHOTS))
	return;
#endif
    /* kps - do we need some hack so that the laser pulse is
     * not removed in the same frame that its life ends ?? */
    pulse->life = 0;
    if (BIT(pulse->mods.laser, STUN)
	|| (laserIsStunGun == true
	    && allowLaserModifiers == false)) {
	if (BIT(pl->used, HAS_SHIELD|HAS_LASER|HAS_SHOT)
	    || BIT(pl->status, THRUSTING)) {
	    if (kp) {
		sprintf(msg,
			"%s got paralysed by %s's stun laser.",
			pl->name, kp->name);
		if (pl->id == kp->id)
		    strcat(msg, " How strange!");
	    } else {
		sprintf(msg,
			"%s got paralysed by a stun laser.",
			pl->name);
	    }
	    Set_message(msg);
	    CLR_BIT(pl->used,
		    HAS_SHIELD|HAS_LASER|OBJ_SHOT);
	    CLR_BIT(pl->status, THRUSTING);
	    pl->stunned += 5;
	}
    } else if (BIT(pulse->mods.laser, BLIND)) {
	pl->damaged += (12 + 6);
	pl->forceVisible += (12 + 6);
	if (kp)
	    Record_shove(pl, kp, frame_loops + 12 + 6);
    } else {
	Player_add_fuel(pl, ED_LASER_HIT);
	if (!BIT(pl->used, HAS_SHIELD)
	    && !BIT(pl->have, HAS_ARMOR)) {
	    SET_BIT(pl->status, KILLED);
	    if (kp) {
		sprintf(msg,
			"%s got roasted alive by %s's laser.",
			pl->name, kp->name);
		if (pl->id == kp->id) {
		    sc = Rate(0.0, kp->score)
			* laserKillScoreMult
			* selfKillScoreMult;
		    Score(kp, -sc, kp->pos, kp->name);
		    strcat(msg, " How strange!");
		} else {
		    sc = Rate(kp->score, pl->score)
			* laserKillScoreMult;
		    Score_players(kp, sc, pl->name, pl, -sc, kp->name);
		}
	    } else {
		sc = Rate(CANNON_SCORE, pl->score) / 4;
		Score(pl, -sc, pl->pos, "Cannon");
		if (BIT(World.rules->mode, TEAM_PLAY)
		    && pl->team != pulse->team)
		    TEAM_SCORE(pulse->team, sc);
		sprintf(msg,
			"%s got roasted alive by cannonfire.",
			pl->name);
	    }
	    sound_play_sensors(pl->pos, PLAYER_ROASTED_SOUND);
	    Set_message(msg);
	    if (kp && kp->id != pl->id) {
		Rank_AddLaserKill(kp);
		Robot_war(pl, kp);
	    }
	}
	if (!BIT(pl->used, HAS_SHIELD)
	    && BIT(pl->have, HAS_ARMOR)) {
	    Player_hit_armor(pl);
	}
    }
}
