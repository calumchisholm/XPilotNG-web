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

#ifndef PAINT_H
#define PAINT_H

/* constants begin */
#define MAX_COLORS		16	/* Max. switched colors ever */
#define MAX_COLOR_LEN		32	/* Max. length of a color name */

#define NUM_DASHES		2
#define NUM_CDASHES		2
#define DASHES_LENGTH		12

#define MIN_HUD_SIZE		90	/* Size/2 of HUD lines */
#define HUD_OFFSET		20	/* Hud line offset */
#define FUEL_GAUGE_OFFSET	6
#define HUD_FUEL_GAUGE_SIZE	(2*(MIN_HUD_SIZE-HUD_OFFSET-FUEL_GAUGE_OFFSET))

#define WARNING_DISTANCE	(VISIBILITY_DISTANCE*0.8)

#define TITLE_DELAY		500	/* Should probably change to seconds */
/* constants end */


/* which index a message actually has (consider reverse scroll) */
#define TALK_MSG_SCREENPOS(total, pos) \
    (instruments.showReverseScroll ? (total)-(pos) : (pos))


/*
 * Global objects.
 */

extern ipos	world;
extern ipos	realWorld;

extern char	dashes[NUM_DASHES];
extern char	cdashes[NUM_CDASHES];

extern int	hudColor;		/* Color index for HUD drawing */
extern int	hudHLineColor;		/* Color index for horiz. HUD line */
extern int	hudVLineColor;		/* Color index for vert. HUD line */
extern int	hudItemsColor;		/* Color index for HUD items drawing */
extern int	hudRadarEnemyColor;	/* Color index for enemy hudradar dots */
extern int	hudRadarOtherColor;	/* Color index for other hudradar dots */
extern int	hudRadarDotSize;	/* Size for hudradar dot drawing */
extern double	hudRadarScale;		/* Scale for hudradar drawing */
extern double	hudRadarLimit;		/* Limit for hudradar drawing */
extern int	hudSize;		/* size for hud-drawing */
extern int	hudLockColor;		/* Color index for lock on HUD drawing */
extern int	fuelGaugeColor;		/* Color index for fuel gauge drawing */
extern int	dirPtrColor;		/* Color index for dirptr-hack drawing */
extern int	shipShapesHackColor;	/* Color index for shipshapes-hack drawing */
extern int	zeroLivesColor;		/* Color to associate with 0 lives */
extern int	oneLifeColor;		/* Color to associate with 1 life */
extern int	twoLivesColor;		/* Color to associate with 2 lives */
extern int	manyLivesColor;		/* Color to associate with >2 lives */
extern int	team0Color;		/* Preferred color index for team 0 */
extern int	team1Color;		/* Preferred color index for team 1 */
extern int	team2Color;		/* Preferred color index for team 2 */
extern int	team3Color;		/* Preferred color index for team 3 */
extern int	team4Color;		/* Preferred color index for team 4 */
extern int	team5Color;		/* Preferred color index for team 5 */
extern int	team6Color;		/* Preferred color index for team 6 */
extern int	team7Color;		/* Preferred color index for team 7 */
extern int	team8Color;		/* Preferred color index for team 8 */
extern int	team9Color;		/* Preferred color index for team 9 */
extern int	msgScanBallColor;	/* Color index for ball msg */
extern int	msgScanSafeColor;	/* Color index for safe msg */
extern int	msgScanCoverColor;	/* Color index for cover msg */
extern int	msgScanPopColor;	/* Color index for pop msg */
extern int	selfLWColor;		/* Color index for selfLifeWarning */
extern int	enemyLWColor;		/* Color index for enemyLifeWarning */
extern int	teamLWColor;		/* Color index for teamLifeWarning */
extern int	shipNameColor;		/* Color index for ship name drawing */
extern int	baseNameColor;		/* Color index for base name drawing */
extern int	mineNameColor;		/* Color index for mine name drawing */
extern int	teamShotColor;		/* Color index for harmless shot drawing */
extern int	ballColor;		/* Color index for ball drawing */
extern int	connColor;		/* Color index for connector drawing */
extern int	fuelMeterColor;		/* Color index for fuel meter */
extern int	powerMeterColor;	/* Color index for power meter */
extern int	turnSpeedMeterColor;	/* Color index for turnspeed meter */
extern int	packetSizeMeterColor;	/* Color index for packet size meter */
extern int	packetLossMeterColor;	/* Color index for packet loss meter */
extern int	packetDropMeterColor;	/* Color index for packet drop meter */
extern int	packetLagMeterColor;	/* Color index for packet lag meter */
extern int	temporaryMeterColor;	/* Color index for temporary meters */
extern int	meterBorderColor;	/* Color index for meter borders */
extern int	baseWarningType;	/* Which type of base warning you prefer */
extern int	wallColor;		/* Color index for wall drawing */
extern int	fuelColor;		/* Color index for fuel box drawing */
extern int	backgroundPointColor;	/* Color index for background point drawing */
extern int	wallRadarColor;		/* Color index for walls on radar */
extern int	targetRadarColor;	/* Color index for targets on radar */
extern int	decorColor;		/* Color index for decoration drawing */
extern int	decorRadarColor;	/* Color index for decorations on radar */
extern int	visibilityBorderColor;	/* Color index for visibility border drawing */
extern int	messagesColor;		/* Color index for message strings */
extern int	oldMessagesColor;	/* Color index for old message strings */
extern int	clockColor;		/* Clock color index */
extern int	scoreColor;		/* Score list color index */
extern int	scoreSelfColor;		/* Score list own score color index */
extern int	scoreInactiveColor;	/* Score list inactive player color index */
extern int	scoreInactiveSelfColor;	/* Score list inactive self color index */
extern int	scoreOwnTeamColor;	/* Score list own team color index */
extern int	scoreEnemyTeamColor;	/* Score list enemy team color index */
extern int	scoreObjectColor;	/* Color index for map score objects */

extern bool	gotFocus;		/* Do we have the mouse pointer */
extern bool	talk_mapped;		/* Is talk window visible */
extern bool     radar_score_mapped;     /* Is the radar and score window mapped */
extern unsigned	draw_width, draw_height;

extern short	ext_view_width;		/* Width of extended visible area */
extern short	ext_view_height;	/* Height of extended visible area */
extern int	active_view_width;	/* Width of active map area displayed. */
extern int	active_view_height;	/* Height of active map area displayed. */
extern int	ext_view_x_offset;	/* Offset of ext_view_width */
extern int	ext_view_y_offset;	/* Offset of ext_view_height */
extern double	charsPerTick;		/* Output speed of messages */
extern bool	markingLights;		/* Marking lights on ships */
extern bool	titleFlip;		/* Do special titlebar flipping? */
extern int	shieldDrawMode;		/* How to draw players shield */

extern char	sparkColors[MSG_LEN];
extern int	spark_color[MAX_COLORS];
extern int	num_spark_colors;

extern long	loops;
extern long	loopsSlow;
extern double	timePerFrame;

extern double	scaleFactor;	/* scale the draw (main playfield) window */
extern double	scaleFactor_s;
extern short	scaleArray[];
extern void	Init_scale_array(void);
#define	WINSCALE(x)	((x) >= 0 ? scaleArray[(x)] : -scaleArray[-(x)])
#define	UWINSCALE(x)	((unsigned)(scaleArray[(x)]))

/* macros begin */

#define X(co)	((int) ((co) - world.x))
#define Y(co)	((int) (world.y + ext_view_height - (co)))

#define SCALEX(co) ((int) (WINSCALE(co) - WINSCALE(world.x)))
#define SCALEY(co) ((int) (WINSCALE(world.y + ext_view_height) - WINSCALE(co)))

/* macros end */

/*
 * Prototypes from the paint*.c files.
 */

int Paint_init(void);
void Paint_cleanup(void);
void Paint_shots(void);
void Paint_ships(void);
void Paint_radar(void);
void Paint_sliding_radar(void);
void Paint_world_radar(void);
void Radar_show_target(int x, int y);
void Radar_hide_target(int x, int y);
void Paint_vcannon(void);
void Paint_vfuel(void);
void Paint_vbase(void);
void Paint_vdecor(void);
void Paint_objects(void);
void Paint_world(void);
void Paint_score_table(void);
void Paint_score_entry(int entry_num, other_t *other, bool is_team);
void Paint_score_start(void);
void Paint_score_objects(void);
void Paint_meters(void);
void Paint_HUD(void);
int  Get_message(int *pos, char *message, int req_length, int key);
void Paint_messages(void);
void Paint_recording(void);
void Paint_client_fps(void);
void Paint_frame(void);
void Game_over_action(u_byte status);
int Team_color(int);
int Life_color(other_t *other);
int Life_color_by_life(int life);
void Play_beep(void);
int Check_view_dimensions(void);

#endif
