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

#ifndef SERVER_H
#define SERVER_H

#ifndef OBJECT_H
# include "object.h"
#endif

#ifndef LIST_H
# include "list.h"
#endif

#ifndef DEFAULTS_H
# include "defaults.h"
#endif

#ifndef WALLS_H
# include "walls.h"
#endif

#ifndef MAX
#define MAX(a,b)  ((a) > (b) ? (a) : (b))
#define MIN(a,b)  ((a) < (b) ? (a) : (b))
#endif

/*
 * When using this, add a final realloc later to free wasted memory
 */
#define STORE(T,P,N,M,V)						\
    if (N >= M && ((M <= 0)						\
	? (P = (T *) malloc((M = 1) * sizeof(*P)))			\
	: (P = (T *) realloc(P, (M += M) * sizeof(*P)))) == NULL) {	\
	warn("No memory");						\
	exit(1);							\
    } else								\
	(P[N++] = V)

typedef struct {
    char	owner[80];
    char	host[80];
} server;

/*
 * Global data.
 */

#define FPS		framesPerSecond
#define NumObjs		(ObjCount + 0)

extern player		**PlayersArray;
extern object		*Obj[];
extern ecm_t		*Ecms[];
extern trans_t		*Transporters[];
extern long		frame_loops;
extern long		frame_loops_slow;
extern double		frame_time;
extern int		observerStart;
extern int		NumPlayers;
extern int		NumObservers;
extern int		NumOperators;
extern int		NumPseudoPlayers;
extern int		NumQueuedPlayers;
extern int		ObjCount;
extern int		NumEcms;
extern int		NumTransporters;
extern int		NumAlliances;
extern int		NumRobots, maxRobots, minRobots;
extern int		login_in_progress;
extern char		ShutdownReason[];
extern sock_t		contactSocket;
extern time_t		serverTime;
extern char		*robotFile;
extern int		robotsTalk, robotsLeave, robotLeaveLife;
extern int		robotLeaveScore, robotLeaveRatio;
extern int		robotTeam;
extern bool		restrictRobots, reserveRobotTeam;
extern int 		robotTicksPerSecond;
extern world_t		World;
extern server		Server;
extern list_t		expandList;
extern double		ShotsMass, ShipMass, ShotsSpeed, Gravity;
extern double		ballMass;
extern double		minItemMass;
extern int		ShotsMax;
extern double		ShotsLife;
extern double		pulseSpeed, pulseLength;
extern double		pulseLife;
extern bool		ShotsGravity;
extern bool		shotHitFuelDrainUsesKineticEnergy;
extern double		fireRepeatRate;
extern double		laserRepeatRate;
extern long		DEF_BITS, KILL_BITS, DEF_HAVE, DEF_USED, USED_KILL;
extern int		GetIndArray[];
extern int		ShutdownServer, ShutdownDelay;
extern bool		Log;
extern bool		silent;
extern bool		RawMode;
extern bool		NoQuit;
extern bool		logRobots;
extern int		framesPerSecond;
extern long		main_loops;
extern char		*mapFileName;
extern int		mapRule;
extern char		*mapData;
extern int		mapWidth;
extern int		mapHeight;
extern char		*mapName;
extern char		*mapAuthor;
extern int 		contactPort;
extern char		*serverHost;
extern char		*serverAddr;
extern char		*greeting;
extern bool		crashWithPlayer;
extern bool		bounceWithPlayer;
extern bool		playerKillings;
extern bool		playerShielding;
extern bool		playerStartsShielded;
extern bool		shotsWallBounce;
extern bool		ballsWallBounce;
extern bool		ballCollisions;
extern bool		ballSparkCollisions;
extern bool		minesWallBounce;
extern bool		itemsWallBounce;
extern bool		missilesWallBounce;
extern bool		sparksWallBounce;
extern bool		debrisWallBounce;
extern bool		asteroidsWallBounce;
extern bool		pulsesWallBounce;
extern bool		cloakedExhaust;
extern bool		cloakedShield;
extern bool		ecmsReprogramMines;
extern bool		ecmsReprogramRobots;
extern double		maxObjectWallBounceSpeed;
extern double		maxShieldedWallBounceSpeed;
extern double		maxUnshieldedWallBounceSpeed;
extern double		maxShieldedWallBounceAngle;
extern double		maxUnshieldedWallBounceAngle;
extern double		playerWallBrakeFactor;
extern double		objectWallBrakeFactor;
extern double		objectWallBounceLifeFactor;
extern double		wallBounceFuelDrainMult;
extern double		wallBounceDestroyItemProb;

extern bool		limitedVisibility;
extern double		minVisibilityDistance;
extern double		maxVisibilityDistance;
extern bool		limitedLives;
extern int		worldLives;
extern bool		endOfRoundReset;
extern int		resetOnHuman;
extern bool		allowAlliances;
extern bool		announceAlliances;
extern bool		teamPlay;
extern bool		teamFuel;
extern bool		teamCannons;
extern int		cannonSmartness;
extern bool		cannonsUseItems;
extern bool		cannonsDefend;
extern bool		cannonFlak;
extern int		cannonDeadTime;
extern bool		keepShots;
extern bool		teamAssign;
extern bool		teamImmunity;
extern bool		teamShareScore;
extern bool		tagGame;
extern int		tag;
extern bool		timing;
extern bool		ballrace;
extern bool		ballrace_connect;
extern bool		edgeWrap;
extern bool		edgeBounce;
extern bool		extraBorder;
extern ipos		gravityPoint;
extern double		gravityAngle;
extern bool		gravityPointSource;
extern bool		gravityClockwise;
extern bool		gravityAnticlockwise;
extern bool		gravityVisible;
extern bool		wormholeVisible;
extern bool		itemConcentratorVisible;
extern bool		asteroidConcentratorVisible;
extern int		wormTime;
extern int		nukeMinSmarts;
extern int		nukeMinMines;
extern double		nukeClusterDamage;
extern int		mineFuseTime;
extern int		mineLife;
extern double		minMineSpeed;
extern double		missileLife;
extern int		baseMineRange;
extern int		mineShotDetonateDistance;

extern double		shotKillScoreMult;
extern double		torpedoKillScoreMult;
extern double		smartKillScoreMult;
extern double		heatKillScoreMult;
extern double		clusterKillScoreMult;
extern double		laserKillScoreMult;
extern double		tankKillScoreMult;
extern double		runoverKillScoreMult;
extern double		ballKillScoreMult;
extern double		explosionKillScoreMult;
extern double		shoveKillScoreMult;
extern double		crashScoreMult;
extern double		mineScoreMult;
extern double		selfKillScoreMult;
extern double		selfDestructScoreMult;
extern double		unownedKillScoreMult;
extern double		asteroidPoints;
extern double		cannonPoints;
extern double		asteroidMaxScore;
extern double		cannonMaxScore;

extern double		destroyItemInCollisionProb;
extern bool		updateScores;
extern bool 		allowSmartMissiles;
extern bool 		allowHeatSeekers;
extern bool 		allowTorpedoes;
extern bool 		allowNukes;
extern bool		allowClusters;
extern bool		allowModifiers;
extern bool		allowLaserModifiers;
extern bool		allowShipShapes;
extern bool		allowPlayerPasswords;

extern bool		shieldedItemPickup;
extern bool		shieldedMining;
extern bool		laserIsStunGun;
extern bool		targetKillTeam;
extern bool		targetSync;
extern int		targetDeadTime;
extern bool		reportToMetaServer;
extern bool		searchDomainForXPilot;
extern char		*denyHosts;

extern bool		playersOnRadar;
extern bool		missilesOnRadar;
extern bool		minesOnRadar;
extern bool		nukesOnRadar;
extern bool		treasuresOnRadar;
extern bool		asteroidsOnRadar;
extern bool 		identifyMines;
extern bool		distinguishMissiles;
extern int		maxMissilesPerPack;
extern int		maxMinesPerPack;
extern bool		targetTeamCollision;
extern bool		treasureKillTeam;
extern bool		captureTheFlag;
extern bool		treasureCollisionDestroys;
extern bool		treasureCollisionMayKill;
extern bool		wreckageCollisionMayKill;
extern bool		asteroidCollisionMayKill;

extern double		ballConnectorSpringConstant;
extern double		ballConnectorDamping;
extern double		maxBallConnectorRatio;
extern double		ballConnectorLength;
extern bool		connectorIsString;
extern double		ballRadius;

extern double 		dropItemOnKillProb;
extern double		detonateItemOnKillProb;
extern double 		movingItemProb;
extern double		randomItemProb;
extern double            rogueHeatProb;
extern double            rogueMineProb;
extern double		itemProbMult;
extern double		cannonItemProbMult;
extern double		asteroidItemProb;
extern int		asteroidMaxItems;
extern double		maxItemDensity;
extern double		maxAsteroidDensity;
extern int		itemConcentratorRadius;
extern double		itemConcentratorProb;
extern int		asteroidConcentratorRadius;
extern double		asteroidConcentratorProb;
extern double		gameDuration;
extern bool		baselessPausing;
extern double		pauseTax;
extern int		pausedFPS;
extern int		waitingFPS;
extern int		game_lock;
extern int		mute_baseless;

extern char		*motdFileName;
extern char	       	*scoreTableFileName;
extern char		*adminMessageFileName;
extern int		adminMessageFileSizeLimit;
extern time_t		gameOverTime;

extern double		friction;
extern double		blockFriction;
extern bool		blockFrictionVisible;
extern double		coriolis, coriolisCosine, coriolisSine;
extern double		checkpointRadius;
extern int		raceLaps;
extern bool		lockOtherTeam;
extern bool 		loseItemDestroys;
extern int		maxOffensiveItems;
extern int		maxDefensiveItems;

extern int		maxVisibleObject;
extern bool		pLockServer;
extern int		timerResolution;

extern int		roundDelaySeconds;
extern int		round_delay;
extern int		round_delay_send;
extern int		maxRoundTime;
extern int		roundtime;
extern int		roundsToPlay;
extern int		roundsPlayed;

extern bool		useWreckage;
extern bool		ignore20MaxFPS;
extern char		*password;

extern char		*robotUserName;
extern char		*robotHostName;

extern char		*tankUserName;
extern char		*tankHostName;
extern int		tankScoreDecrement;

extern bool		selfImmunity;

extern char		*defaultShipShape;
extern char		*tankShipShape;

extern int		clientPortStart;
extern int		clientPortEnd;

extern int		maxPauseTime;
extern int		maxClientsPerIP;

extern long		KILLING_SHOTS;
extern unsigned		SPACE_BLOCKS;

extern int		numberOfRounds;
extern int		playerLimit;
extern int		playerLimit_orig;
extern int		recordMode;
extern int		recordFlushInterval;
extern int		constantScoring;
extern int		eliminationRace;
extern char		*dataURL;
extern char		*recordFileName;
extern double		gameSpeed;
extern double		timeStep;
extern double		ecmSizeFactor;
extern char		*playerPasswordsFileName;
extern int		playerPasswordsFileSizeLimit;
extern bool		maraTurnqueue;
extern bool		ngControls;
extern bool		ignoreMaxFPS;
extern bool		polygonMode;

#define Bases(ind)		(&World.bases[(ind)])
#define Fuels(ind)		(&World.fuels[(ind)])
#define Cannons(ind)		(&World.cannons[(ind)])
#define Checks(ind)		(&World.checks[(ind)])
#define Gravs(ind)		(&World.gravs[(ind)])
#define Targets(ind)		(&World.targets[(ind)])
#define Treasures(ind)		(&World.treasures[(ind)])
#define Wormholes(ind)		(&World.wormholes[(ind)])
#define AsteroidConcs(ind)	(&World.asteroidConcs[(ind)])
#define ItemConcs(ind)		(&World.itemConcs[(ind)])
#define Teams(team)		(&World.teams[(team)])

/* determine if a block is one of SPACE_BLOCKS */
#define EMPTY_SPACE(s)	BIT(1U << (s), SPACE_BLOCKS)

#define Player_by_id(id)	Players(GetInd(id))

static inline vector World_gravity(clpos pos)
{
    return World.gravity[CLICK_TO_BLOCK(pos.cx)][CLICK_TO_BLOCK(pos.cy)];
}

enum TeamPickType {
    PickForHuman	= 1,
    PickForRobot	= 2
};

#ifndef	_WINDOWS
#define	APPNAME	"xpilots"
#else
#define	APPNAME	"XPilotServer"
#endif


/*
 * Prototypes for cell.c
 */
void Free_cells(void);
void Alloc_cells(void);
void Cell_init_object(object *obj);
void Cell_add_object(object *obj);
void Cell_remove_object(object *obj);
void Cell_get_objects(int x, int y, int r, int max, object ***list, int *count);

/*
 * Prototypes for collision.c
 */
void Check_collision(void);
int IsOffensiveItem(enum Item i);
int IsDefensiveItem(enum Item i);
int CountOffensiveItems(player *pl);
int CountDefensiveItems(player *pl);

/*
 * Prototypes for id.c
 */
int peek_ID(void);
int request_ID(void);
void release_ID(int id);

/*
 * Prototypes for walls.c
 */
void Groups_init(void);
void Walls_init(void);
void Treasure_init(void);
void Move_init(void);
void Move_object(object *obj);
void Move_player(player *pl);
void Turn_player(player *pl);
int is_inside(int x, int y, int hitmask, const object *obj);
int shape_is_inside(int cx, int cy, int hitmask, const object *obj,
		    const shape_t *s, int dir);
int Polys_to_client(unsigned char **);
void Ball_line_init(void);
void Player_crash(player *pl, int crashtype, int mapobj_ind, int pt);
void Object_crash(object *obj, int crashtype, int mapobj_ind);

/*
 * Prototypes for event.c
 */
int Handle_keyboard(player *pl);
void Pause_player(player *pl, bool on);
int Player_lock_closest(player *pl, bool next);
bool team_dead(int team);
void filter_mods(modifiers *mods);

/*
 * Prototypes for map.c
 */
void Free_map(void);
bool Grok_map(void);
bool Grok_map_options(void);

int Map_place_base(clpos pos, int dir, int team);
int Map_place_cannon(clpos pos, int dir, int team);
int Map_place_check(clpos pos, int ind);
int Map_place_fuel(clpos pos, int team);
int Map_place_grav(clpos pos, double force, int type);
int Map_place_target(clpos pos, int team);
int Map_place_treasure(clpos pos, int team, bool empty);
int Map_place_wormhole(clpos pos, wormType type);
int Map_place_item_concentrator(clpos pos);
int Map_place_asteroid_concentrator(clpos pos);

void Find_base_direction(void);
void Compute_gravity(void);
double Wrap_findDir(double dx, double dy);
double Wrap_cfindDir(int dx, int dy);
double Wrap_length(int dx, int dy);
int Find_closest_team(clpos pos);


/*
 * Prototypes for xpmap.c
 */
void Xpmap_print(void);
void Xpmap_grok_map_data(void);
void Xpmap_allocate_checks(void);
void Xpmap_tags_to_internal_data(bool create_objects);
void Xpmap_find_map_object_teams(void);
void Xpmap_find_base_direction(void);
void Xpmap_blocks_to_polygons(void);


/*
 * Prototypes for xp2map.c
 */
bool isXp2MapFile(int fd);
bool parseXp2MapFile(int fd, optOrigin opt_origin);


/*
 * Prototypes for cmdline.c
 */
void tuner_none(void);
void tuner_dummy(void);
bool Init_options(void);
void Free_options(void);
void Check_playerlimit(void);
void Timing_setup(void);

/*
 * Prototypes for play.c
 */
void Thrust(player *pl);
void Record_shove(player *pl, player *pusher, long shove_time);
void Delta_mv(object *ship, object *obj);
void Delta_mv_elastic(object *obj1, object *obj2);
void Obj_repel(object *obj1, object *obj2, int repel_dist);
void Item_damage(player *pl, double prob);
void Tank_handle_detach(player *pl);
void Add_fuel(pl_fuel_t *, double);
void Update_tanks(pl_fuel_t *);
void Place_item(player *pl, int type);
int Choose_random_item(void);
void Tractor_beam(player *pl);
void General_tractor_beam(player *pl, clpos pos,
			  int items, player *victim, bool pressor);
void Place_mine(player *pl);
void Place_moving_mine(player *pl);
void Place_general_mine(player *pl, int team, long status,
			clpos pos, vector vel, modifiers mods);
void Detonate_mines(player *pl);
char *Describe_shot(int type, long status, modifiers mods, int hit);
void Fire_ecm(player *pl);
void Fire_general_ecm(player *pl, int team, clpos pos);
void Update_connector_force(ballobject *ball);
void Fire_shot(player *pl, int type, int dir);
void Fire_general_shot(player *pl, int team, bool cannon,
		       clpos pos, int type, int dir,
		       modifiers mods, int target_id);
void Fire_normal_shots(player *pl);
void Fire_main_shot(player *pl, int type, int dir);
void Fire_left_shot(player *pl, int type, int dir, int gun);
void Fire_right_shot(player *pl, int type, int dir, int gun);
void Fire_left_rshot(player *pl, int type, int dir, int gun);
void Fire_right_rshot(player *pl, int type, int dir, int gun);
void Make_treasure_ball(treasure_t *t);

void Ball_hits_goal(ballobject *ball, group_t *groupptr);
void Ball_is_replaced(ballobject *ball);
void Ball_is_destroyed(ballobject *ball);

bool Balltarget_hitfunc(group_t *groupptr, move_t *move);

int Cannon_hitmask(cannon_t *cannon);
bool Cannon_hitfunc(group_t *groupptr, move_t *move);
void Cannon_restore_on_map(cannon_t *cannon);
void Cannon_remove_from_map(cannon_t *cannon);

int Target_hitmask(target_t *targ);
void Target_init(void);
void Target_restore_on_map(target_t *targ);
void Target_remove_from_map(target_t *targ);

int Wormhole_hitmask(wormhole_t *wormhole);
bool Wormhole_hitfunc(group_t *groupptr, move_t *move);
void Wormhole_remove_from_map(wormhole_t *wormhole);

void Team_immunity_init(void);
void Hitmasks_init(void);
void Transfer_tag(player *oldtag_pl, player *newtag_pl);
/*double Handle_tag(double score, player *victim_pl, player* killer_pl);*/
void Check_tag(void);
void Delete_shot(int ind);
void Fire_laser(player *pl);
void Fire_general_laser(player *pl, int team, clpos pos,
			int dir, modifiers mods);
void Do_deflector(player *pl);
void Do_transporter(player *pl);
void Do_general_transporter(player *pl, clpos pos, player *victim,
			    int *item, double *amount);
bool Do_hyperjump(player *pl);
void do_lose_item(player *pl);
void Update_torpedo(torpobject *torp);
void Update_missile(missileobject *shot);
void Update_mine(mineobject *mine);
void Make_debris(
    /* pos            */ clpos  pos,
    /* vel            */ vector vel,
    /* owner id       */ int    id,
    /* owner team     */ int    team,
    /* type           */ int    type,
    /* mass           */ double mass,
    /* status         */ long   status,
    /* color          */ int    color,
    /* radius         */ int    radius,
    /* num debris     */ int    num_debris,
    /* min,max dir    */ int    min_dir,    int    max_dir,
    /* min,max speed  */ double min_speed,  double max_speed,
    /* min,max life   */ double min_life,   double max_life
    );
void Make_wreckage(
    /* pos            */ clpos  pos,
    /* vel            */ vector vel,
    /* owner id       */ int    id,
    /* owner team     */ int    team,
    /* min,max mass   */ double min_mass,   double max_mass,
    /* total mass     */ double total_mass,
    /* status         */ long   status,
    /* color          */ int    color,
    /* max wreckage   */ int    max_wreckage,
    /* min,max dir    */ int    min_dir,    int    max_dir,
    /* min,max speed  */ double min_speed,  double max_speed,
    /* min,max life   */ double min_life,   double max_life
    );
void Make_item(clpos pos,
	       vector vel,
	       int item, int num_per_pack,
	       long status);
void Explode_fighter(player *pl);
void Throw_items(player *pl);
void Detonate_items(player *pl);
void add_temp_wormholes(int xin, int yin, int xout, int yout);
void remove_temp_wormhole(int ind);


/*
 * Prototypes for asteroid.c
 */
void Break_asteroid(wireobject *asteroid);
void Asteroid_update(void);
list_t Asteroid_get_list(void);


/*
 * Prototypes for cannon.c
 */
void Cannon_init(cannon_t *cannon);
void Cannon_add_item(cannon_t *cannon, int item, double amount);
void Cannon_throw_items(cannon_t *cannon);
void Cannon_check_defense(cannon_t *cannon);
void Cannon_check_fire(cannon_t *cannon);
void Cannon_dies(cannon_t *cannon, player *pl);

/*
 * Prototypes for command.c
 */
void Handle_player_command(player *pl, char *cmd);
player *Get_player_by_name(char *str, int *errcode, const char **errorstr_p);

/*
 * Prototypes for player.c
 */
player *Players(int ind);
int GetInd(int id);
void Pick_startpos(player *pl);
void Go_home(player *pl);
void Compute_sensor_range(player *pl);
void Player_add_tank(player *pl, double tank_fuel);
void Player_remove_tank(player *pl, int which_tank);
static inline void Player_add_fuel(player *pl, double amount)
{
    Add_fuel(&(pl->fuel), amount);
}
static inline bool Player_used_emergency_shield(player *pl)
{
    if (BIT(pl->used, (HAS_SHIELD|HAS_EMERGENCY_SHIELD)) ==
	(HAS_SHIELD|HAS_EMERGENCY_SHIELD))
	return true;
    return false;
}
void Player_hit_armor(player *pl);
void Player_used_kill(player *pl);
void Player_set_mass(player *pl);
int Init_player(int ind, shipshape_t *ship);
void Alloc_players(int number);
void Free_players(void);
void Update_score_table(void);
void Reset_all_players(void);
void Check_team_members(int);
void Compute_game_status(void);
void Delete_player(player *pl);
void Detach_ball(player *pl, ballobject *ball);
void Kill_player(player *pl, bool add_rank_death);
void Player_death_reset(player *pl, bool add_rank_death);
void Team_game_over(int winning_team, const char *reason);
void Individual_game_over(int winner);
void Race_game_over(void);
int Team_immune(int id1, int id2);
static inline void Player_set_float_dir(player *pl, double new_float_dir)
{
    if (ngControls && new_float_dir != pl->float_dir) {
	pl->float_dir = new_float_dir;
	pl->float_dir_cos = cos(pl->float_dir * 2.0 * PI / RES);
	pl->float_dir_sin = sin(pl->float_dir * 2.0 * PI / RES);
    } else
	pl->float_dir = new_float_dir;
}

/*
 * Prototypes for robot.c
 */
void Parse_robot_file(void);
void Robot_init(void);
void Robot_delete(player *robot, bool kicked);
void Robot_destroy(player *robot);
void Robot_update(void);
void Robot_invite(player *robot, player *inviter);
void Robot_war(player *robot, player *killer);
void Robot_reset_war(player *robot);
int Robot_war_on_player(player *robot);
void Robot_go_home(player *robot);
void Robot_program(player *robot, int victim_id);
void Robot_message(player *robot, const char *message);

/*
 * Prototypes for rules.c
 */
void Tune_item_probs(void);
void Tune_item_packs(void);
void Set_initial_resources(void);
void Set_world_items(void);
void Set_world_rules(void);
void Set_world_asteroids(void);
void Set_misc_item_limits(void);
void Tune_asteroid_prob(void);

/*
 * Prototypes for server.c
 */
int End_game(void);
int Pick_team(int pick_for_type);
void Server_info(char *str, size_t max_size);
void Log_game(const char *heading);
void Game_Over(void);
void Server_log_admin_message(player *pl, const char *str);
int plock_server(bool on);
void Main_loop(void);


/*
 * Prototypes for contact.c
 */
void Contact_cleanup(void);
int Contact_init(void);
void Contact(int fd, void *arg);
void Queue_kick(const char *nick);
void Queue_loop(void);
int Queue_advance_player(char *name, char *msg);
int Queue_show_list(char *msg);
void Set_deny_hosts(void);

/*
 * Prototypes for metaserver.c
 */
void Meta_send(char *mesg, size_t len);
int Meta_from(char *addr, int port);
void Meta_gone(void);
void Meta_init(void);
void Meta_update(int change);

/*
 * Prototypes for frame.c
 */
void Frame_update(void);
void Set_message(const char *message);
void Set_player_message(player *pl, const char *message);

/*
 * Prototypes for update.c
 */
void Update_objects(void);
void Autopilot(player *pl, bool on);
void Cloak(player *pl, bool on);
void Deflector(player *pl, bool on);
void Emergency_thrust(player *pl, bool on);
void Emergency_shield(player *pl, bool on);
void Phasing(player *pl, bool on);

/*
 * Prototypes for option.c
 */
void Options_parse(void);
void Options_free(void);
bool Convert_string_to_int(const char *value_str, int *int_ptr);
bool Convert_string_to_float(const char *value_str, double *float_ptr);
bool Convert_string_to_bool(const char *value_str, bool *bool_ptr);
void Convert_list_to_string(list_t list, char **string);
void Convert_string_to_list(const char *value, list_t *list_ptr);

/*
 * Prototypes for parser.c
 */
int Parser_list_option(int *ind, char *buf);
bool Parser(int argc, char **argv);
int Tune_option(char *name, char *val);
int Get_option_value(const char *name, char *value, size_t size);

/*
 * Prototypes for fileparser.c
 */
bool parseDefaultsFile(const char *filename);
bool parsePasswordFile(const char *filename);
bool parseMapFile(const char *filename);
void expandKeyword(const char *keyword);

/*
 * Prototypes for laser.c
 */
void Laser_pulse_collision(void);
void Laser_pulse_hits_player(player *pl, pulseobject *pulse);

/*
 * Prototypes for alliance.c
 */
int Invite_player(player *pl, player *ally);
int Cancel_invitation(player *pl);
int Refuse_alliance(player *pl, player *ally);
int Refuse_all_alliances(player *pl);
int Accept_alliance(player *pl, player *ally);
int Accept_all_alliances(player *pl);
int Get_alliance_member_count(int id);
void Player_join_alliance(player *pl, player *ally);
void Dissolve_all_alliances(void);
int Leave_alliance(player *pl);
void Alliance_player_list(player *pl);

/*
 * Prototypes for object.c
 */
object *Object_allocate(void);
void Object_free_ind(int ind);
void Object_free_ptr(object *obj);
void Alloc_shots(int number);
void Free_shots(void);
const char *Object_typename(object *obj);

/*
 * Prototypes for polygon.c
 */
void P_edgestyle(const char *id, int width, int color, int style);
void P_polystyle(const char *id, int color, int texture_id, int defedge_id,
		 int flags);
void P_bmpstyle(const char *id, const char *filename, int flags);
void P_start_polygon(clpos pos, int style);
void P_offset(clpos offset, int edgestyle);
void P_vertex(clpos pos, int edgestyle);
void P_end_polygon(void);
int P_start_ballarea(void);
void P_end_ballarea(void);
int P_start_balltarget(int team, int treasure_ind);
void P_end_balltarget(void);
int P_start_target(int target_ind);
void P_end_target(void);
int P_start_cannon(int cannon_ind);
void P_end_cannon(void);
int P_start_wormhole(int wormhole_ind);
void P_end_wormhole(void);
void P_start_decor(void);
void P_end_decor(void);
int P_start_frictionarea(void);
void P_end_frictionarea(void);
int P_get_bmp_id(const char *s);
int P_get_edge_id(const char *s);
int P_get_poly_id(const char *s);
/*void P_grouphack(int type, void (*f)(int group, void *mapobj));*/
void P_set_hitmask(int group, int hitmask);

/*
 * Prototypes for showtime.c
 */
char *showtime(void);

/*
 * Prototypes for srecord.c
 */
void Init_recording(void);
void Handle_recording_buffers(void);
void Get_recording_data(void);

#endif


