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

/*
 * This file is an awful hack which is used only until someone gets
 * the time to rewrite it from scratch. It uses the X resource manager
 * which is something we don't want to use in an SDL/OpenGL based client.
 * So if you are feeling bored why not rewrite this thing so that it 
 * does not depend on X.
 */

#ifndef _WINDOWS
#include <unistd.h>
#include <X11/Xos.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <sys/param.h>
#else
#include "NT/winX.h"
#include "NT/winXXPilot.h"
#endif
#include "xpclient.h"
#include "sdlkeys.h"
#include "default.h"
#include "SDL.h"


char default_version[] = VERSION;



#define DISPLAY_ENV	"DISPLAY"
#define DISPLAY_DEF	":0.0"
#define KEYBOARD_ENV	"KEYBOARD"

#ifndef PATH_MAX
#define PATH_MAX	1023
#endif

char			myName[] = "xpilot";
char			myClass[] = "XPilot";


extern char *talk_fast_msgs[];	/* talk macros */
char talk_fast_temp_buf[7];		/* can handle up to 999 fast msgs */
char *talk_fast_temp_buf_big;


static Display *dpy;

/* from common/config.c */
extern char conf_ship_file_string[];
extern char conf_texturedir_string[];
extern char conf_soundfile_string[];

/* from sdlevent.c */
extern keys_t keyMap[SDLK_LAST];   /* maps SDLKeys to keys_t */
extern keys_t buttonMap[5];        /* maps mouse buttons to keys_t */


/*
 * Structure to store all the client options.
 * The most important field is the help field.
 * It is used to self-document the client to
 * the user when "xpilot -help" is issued.
 * Help lines can span multiple lines, but for
 * the key help window only the first line is used.
 */
option options[] = {
    {
	"help",
	"Yes",
	"",
	KEY_DUMMY,
	"Display this help message.\n",
	0
    },
    {
	"version",
	"Yes",
	"",
	KEY_DUMMY,
	"Show the source code version.\n",
	0
    },
    {
	"name",
	NULL,
	"",
	KEY_DUMMY,
	"Set the nickname.\n",
	0
    },
    {
	"user",
	NULL,
	"",
	KEY_DUMMY,
	"Set the username.\n",
	0
    },
    {
	"host",
	NULL,
	"",
	KEY_DUMMY,
	"Set the hostname.\n",
	0
    },
    {
	"join",
	"Yes",
	"",
	KEY_DUMMY,
	"Join the game immediately, no questions asked.\n",
	0
    },
    {
	"noLocalMotd",
	"Yes",
	"",
	KEY_DUMMY,
	"Do not display the local Message Of The Day.\n",
	0
    },
    {
	"autoServerMotdPopup",
	NULL,
#ifdef _WINDOWS
	"No",	/* temporary till i straighten out the motd woes. */
#else
	"Yes",
#endif
	KEY_DUMMY,
	"Automatically popup the MOTD of the server on startup.\n",
	0
    },
    {
	"refreshMotd",
	NULL,
	"No",
	KEY_DUMMY,
	"Get a fresh copy of the server MOTD every time it is displayed.\n",
	0
    },
    {
	"text",
	"Yes",
	"No",
	KEY_DUMMY,
	"Use the simple text interface to contact a server\n"
	"instead of the graphical user interface.\n",
	0
    },
    {
	"list",
	"Yes",
	"",
	KEY_DUMMY,
	"List all servers running on the local network.\n",
	0
    },
    {
	"team",
	NULL,
	TEAM_NOT_SET_STR,
	KEY_DUMMY,
	"Set the team to join.\n",
	0
    },
    {
	"display",
	NULL,
	"",
	KEY_DUMMY,
	"Set the X display.\n",
	0
    },
    {
	"keyboard",
	NULL,
	"",
	KEY_DUMMY,
	"Set the X keyboard input if you want keyboard input from\n"
	"another display.  The default is to use the keyboard input from\n"
	"the X display.\n",
	0
    },
    {
	"geometry",
	NULL,
	"1024x768",
	KEY_DUMMY,
	"Set the window size and position in standard X geometry format.\n"
	"The maximum allowed window size is 1922x1440.\n",
	0
    },
    {
	"ignoreWindowManager",
	NULL,
	"",
	KEY_DUMMY,
	"Ignore the window manager when opening the top level player window.\n"
	"This can be handy if you want to have your XPilot window on a preferred\n"
	"position without window manager borders.  Also sometimes window managers\n"
	"may interfere when switching colormaps.  This option may prevent that.\n",
	0
    },
    {
	"shutdown",
	NULL,
	"",
	KEY_DUMMY,
	"Shutdown the server with a message.\n"
	"The message used is the first argument to this option.\n",
	0
    },
    {
	"port",
	NULL,
	SERVER_PORT_STR,
	KEY_DUMMY,
	"Set the port number of the server.\n"
	"Almost all servers use the default port, which is the\n"
	"recommended policy.  You can find out about which port\n"
	"is used by a server by querying the XPilot Meta server.\n",
	0
    },
    {
	"shipShape",
	NULL,
	"",
	KEY_DUMMY,
	"Define the ship shape to use.  Because the argument to this option\n"
	"is rather large (up to 500 bytes) the recommended way to set\n"
	"this option is in the .xpilotrc file in your home directory.\n"
	"The exact format is defined in the file doc/README.SHIPS in the XPilot\n"
	"distribution.  Note that there is a nifty Unix tool called editss for\n"
	"easy ship creation.  There is XPShipEditor for Windows\n"
	"and Ship Shaper for Java.  See the XPilot FAQ for details.\n"
	"See also the \"shipShapeFile\" option below.\n",
	0
    },
    {
	"shipShapeFile",
	NULL,
	conf_ship_file_string,
	KEY_DUMMY,
	"An optional file where shipshapes can be stored.\n"
	"If this resource is defined and it refers to an existing file\n"
	"then shipshapes can be referenced to by their name.\n"
	"For instance if you define shipShapeFile to be\n"
	"/home/myself/.shipshapes and this file contains one or more\n"
	"shipshapes then you can select the shipshape by starting xpilot as:\n"
	"	xpilot -shipShape myshipshapename\n"
	"Where \"myshipshapename\" should be the \"name:\" or \"NM:\" of\n"
	"one of the shipshapes defined in /home/myself/.shipshapes.\n"
	"Each shipshape definition should be defined on only one line,\n"
	"where all characters up to the first left parenthesis don't matter.\n"
	/* shipshopshapshepshit getting nuts from all these shpshp-s. */,
	0
    },
    {
	"power",
	NULL,
	"55.0",
	KEY_DUMMY,
	"Set the engine power.\n"
	"Valid values are in the range 5-55.\n",
	0
    },
    {
	"turnSpeed",
	NULL,
	"35.0",
	KEY_DUMMY,
	"Set the ship's turn speed.\n"
	"Valid values are in the range 4-64.\n"
	"See also turnResistance.\n",
	0
    },
    {
	"turnResistance",
	NULL,
	"0",
	KEY_DUMMY,
	"Set the ship's turn resistance.\n"
	"This determines the speed at which a ship stops turning.\n"
	"Valid values are in the range 0.0-1.0.\n"
	"This should always be 0, other values are for compatibility.\n"
	"See also turnSpeed.\n",
	0
    },
    {
	"altPower",
	NULL,
	"35.0",
	KEY_DUMMY,
	"Set the alternate engine power.\n"
	"See also the keySwapSettings option.\n",
	0
    },
    {
	"altTurnSpeed",
	NULL,
	"25.0",
	KEY_DUMMY,
	"Set the alternate ship's turn speed.\n"
	"See also the keySwapSettings option.\n",
	0
    },
    {
	"altTurnResistance",
	NULL,
	"0",
	KEY_DUMMY,
	"Set the alternate ship's turn resistance.\n"
	"See also the keySwapSettings option.\n",
	0
    },
    {
	"mapRadar",
	NULL,
	"Yes",
	KEY_DUMMY,
	"Paint radar dots' position on the map \n",
	0
    },
    {
	"showShipShapes",
	NULL,
	"Yes",
	KEY_DUMMY,
	"Should others' shipshapes be displayed or not.\n",
	0
    },
    {
	"showMyShipShape",
	NULL,
	"Yes",
	KEY_DUMMY,
	"Should your own shipshape be displayed or not.\n",
	0
    },
    {
	"ballMsgScan",
	NULL,
	"Yes",
	KEY_DUMMY,
	"Scan messages for BALL, SAFE, COVER and POP and paint \n"
	"warning circles inside ship.\n",
	0
    },
    /* these 2 should really be color options */
    {
	"showLivesByShip",
	NULL,
	"No",
	KEY_DUMMY,
	"Paint remaining lives next to ships.\n",
	0
    },
    {
	"fuelNotify",
	NULL,
	"500.0",
	KEY_DUMMY,
	"The limit when the HUD fuel bar will become visible.\n",
	0
    },
    {
	"fuelWarning",
	NULL,
	"200.0",
	KEY_DUMMY,
	"The limit when the HUD fuel bar will start flashing.\n",
	0
    },
    {
	"fuelCritical",
	NULL,
	"100.0",
	KEY_DUMMY,
	"The limit when the HUD fuel bar will flash faster.\n",
	0
    },
    {
	"speedFactHUD",
	NULL,
	"0.0",
	KEY_DUMMY,
	"Should the HUD be moved, to indicate the current velocity?\n",
	0
    },
    {
	"speedFactPTR",
	NULL,
	"0.0",
	KEY_DUMMY,
	"Uses a red line to indicate the current velocity and direction.\n",
	0
    },
    {
	"slidingRadar",
	NULL,
	"Yes",
	KEY_DUMMY,
	"If the game is in edgewrap mode then the radar will keep your\n"
	"position on the radar in the center and raw the rest of the radar\n"
	"around it.  Note that this requires a fast graphics system.\n",
	0
    },
    {
	"outlineWorld",
	NULL,
	"No",
	KEY_DUMMY,
	"Draws only the outline of all the blue map constructs.\n",
	0
    },
    {
	"filledWorld",
	NULL,
	"No",
	KEY_DUMMY,
	"Draws the walls solid, filled with one color,\n"
	"unless overridden by texture.\n"
	"Be warned that this option needs fast graphics.\n",
	0
    },
    {
	"texturedWalls",
	NULL,
	"Yes",
	KEY_DUMMY,
	"Allows drawing polygon bitmaps specified by the (new-style) map\n"
	"See also the wallTextureFile option.\n"
	"Be warned that this needs a reasonably fast graphics system.\n",
	0
    },
    {
	"wallTextureFile",
	NULL,
	"",
	KEY_DUMMY,
	"Specify a XPM format pixmap file to load wall texture from.\n"
	"(this only affects old-style maps, generally useless)\n",
	0
    },
    {
	"texturePath",
	NULL,
	conf_texturedir_string,
	KEY_DUMMY,
	"Search path for texture files.\n"
	"This is a list of one or more directories separated by colons.\n",
	0
    },
    {
	"fullColor",
	NULL,
	"Yes",
	KEY_DUMMY,
	"Whether to use a colors as close as possible to the specified ones\n"
	"or use a few standard colors for everything. May require more\n"
	"resources from your system.\n",
	0
    },
    {
	"texturedObjects",
	NULL,
	"Yes",
	KEY_DUMMY,
	"Whether to draw certain game objects with textures.\n"
	"Be warned that this requires more graphics speed.\n"
	"fullColor must be on for this to work.\n"
	"You may also need to enable multibuffering or double-buffering.\n",
	0
    },
    {
	"showID",
	NULL,
	"No",
	KEY_DUMMY,
	"Should ID numbers be shown instead of ship names on the playfield?\n",
	0
    },
    {
	"markingLights",
	NULL,
	"No",
	KEY_DUMMY,
	"Should the fighters have marking lights, just like airplanes?\n",
	0
    },
    {
	"sparkProb",
	NULL,
	"0.25",
	KEY_DUMMY,
	"The chance that sparks are drawn or not.\n"
	"This gives a sparkling effect.\n"
	"Valid values are in the range [0.0-1.0]\n",
	0
    },
    {
	"sparkSize",
	NULL,
	"2",
	KEY_DUMMY,
	"Size of sparks in pixels.\n",
	0
    },
    {
	"charsPerSecond",
	NULL,
	"50",
	KEY_DUMMY,
	"Speed in which messages appear on screen in characters per second.\n",
	0
    },
    {
	"clockAMPM",
	NULL,
	"No",
	KEY_DUMMY,
	"12 or 24 hour format for clock display.\n",
	0
    },
    {
	"pointerControl",
	NULL,
	"No",
	KEY_DUMMY,
	"Enable mouse control.  This allows ship direction control by\n"
	"moving the mouse to the left for an anti-clockwise turn and\n"
	"moving the mouse to the right for a clockwise turn.\n"
	"Also see the pointerButton options for use of the mouse buttons.\n",
	0
    },
    {
	"maxMessages",
	NULL,
	"8",
	KEY_DUMMY,
	"The maximum number of messages to display.\n",
	0
    },
    {
	"messagesToStdout",
	NULL,
	"0",
	KEY_DUMMY,
	"Send messages to standard output.\n"
	"0: Don't.\n"
	"1: Only player messages.\n"
	"2: Player and status messages.\n",
	0
    },
    {
	"reverseScroll",
	NULL,
	"No",
	KEY_DUMMY,
	"Reverse scroll direction of messages.\n",
	0
    },
#ifndef _WINDOWS
    {
	"selectionAndHistory",
	NULL,
	"Yes",
	KEY_DUMMY,
	"Provide cut&paste for the player messages and the talk window and\n"
	"a `history' for the talk window.\n",
	0
    },
    {
	"maxLinesInHistory",
	NULL,
	"32",
	KEY_DUMMY,
	"Number of your messages saved in the `history' of the talk window.\n"
	"`history' is accessible with `keyTalkCursorUp/Down'.\n",
	0
    },
#endif
    {
	"shotSize",
	NULL,
	"5",
	KEY_DUMMY,
	"The size of shots in pixels.\n",
	0
    },
    {
	"teamShotSize",
	NULL,
	"3",
	KEY_DUMMY,
	"The size of team shots in pixels.\n"
	"Note that team shots are drawn in teamShotColor.\n",
	0
    },
    {
	"teamShotColor",
	NULL,
	"2",
	KEY_DUMMY,
	"Which color number to use for drawing harmless shots.\n",
	0
    },
    {
	"showNastyShots",
	NULL,
	"No",
	KEY_DUMMY,
	"Use the new Nasty Looking Shots or the original rectangle shots,\n"
	"You will probably want to increase your shotSize if you use this.\n",
	0
    },
    {
	"backgroundPointDist",
	NULL,
	"8",
	KEY_DUMMY,
	"The distance between points in the background measured in blocks.\n"
	"These are drawn in empty map regions to keep feeling for which\n"
	"direction the ship is moving to.\n",
	0
    },
    {
	"backgroundPointSize",
	NULL,
	"2",
	KEY_DUMMY,
	"Specifies the size of the background points.  0 means no points.\n",
	0
    },
    {
	"titleFlip",
	NULL,
	"Yes",
	KEY_DUMMY,
	"Should the title bar change or not.\n"
	"Some window managers like twm may have problems with\n"
	"flipping title bars.  Hence this option to turn it off.\n",
	0
    },
    {
	"toggleShield",
	NULL,
	"No",
	KEY_DUMMY,
	"Are shields toggled by a keypress only?\n",
	0
    },
    {
	"autoShield", /* Don auto-shield hack */
	NULL,
	"Yes",
	KEY_DUMMY,
	"Are shields lowered automatically for weapon fire?\n",
	0
    },
    {
	"shieldDrawSolid",
	NULL,
	"Default",
	KEY_DUMMY,
	"Are shields drawn in a solid line.\n"
	"Not setting a value for this option will select the best value\n"
	"automatically for your particular display system.\n",
	0
    },
    {
	"showMessages",
	NULL,
	"Yes",
	KEY_DUMMY,
	"Should messages appear on screen.\n",
	0
    },
    {
	"showItems",
	NULL,
	"Yes",
	KEY_DUMMY,
	"Should owned items be displayed permanently on the HUD?\n",
	0
    },
    {
	"showItemsTime",
	NULL,
	"3.0",
	KEY_DUMMY,
	"If showItems is false, the time in seconds to display item\n"
	"information on the HUD when it has changed.\n",
	0
    },
    {
	"showScoreDecimals",
	NULL,
	"1",
	KEY_DUMMY,
	"The number of decimals to use when displaying scores.\n",
	0
    },
    {
	"receiveWindowSize",
	NULL,
	"3",
	KEY_DUMMY,
	"Too complicated.  Keep it on 3.\n",
	0
    },
    {
	"visual",
	NULL,
	"",
	KEY_DUMMY,
	"Specify which visual to use for allocating colors.\n"
	"To get a listing of all possible visuals on your dislay\n"
	"set the argument for this option to list.\n",
	0
    },
    {
	"colorSwitch",
	NULL,
	"Yes",
	KEY_DUMMY,
	"Use color buffering or not.\n"
	"Usually color buffering is faster, especially on 8-bit\n"
	"PseudoColor displays.\n",
	0
    },
    {
	"multibuffer",
	NULL,
	"No",
	KEY_DUMMY,
	"Use the X windows multibuffer extension if present.\n",
	0
    },
    {
	"maxColors",
	NULL,
	"16",
	KEY_DUMMY,
	"The number of colors to use.  Valid values are 4, 8 and 16.\n",
	0
    },
    {
	"color0",
	NULL,
	"",
	KEY_DUMMY,
	"The color value for the first color.\n",
	0
    },
    {
	"color1",
	NULL,
	"",
	KEY_DUMMY,
	"The color value for the second color.\n",
	0
    },
    {
	"color2",
	NULL,
	"",
	KEY_DUMMY,
	"The color value for the third color.\n",
	0
    },
    {
	"color3",
	NULL,
	"",
	KEY_DUMMY,
	"The color value for the fourth color.\n",
	0
    },
    {
	"color4",
	NULL,
	"",
	KEY_DUMMY,
	"The color value for the fifth color.\n"
	"This is only used if maxColors is set to 8 or 16.\n",
	0
    },
    {
	"color5",
	NULL,
	"",
	KEY_DUMMY,
	"The color value for the sixth color.\n"
	"This is only used if maxColors is set to 8 or 16.\n",
	0
    },
    {
	"color6",
	NULL,
	"",
	KEY_DUMMY,
	"The color value for the seventh color.\n"
	"This is only used if maxColors is set to 8 or 16.\n",
	0
    },
    {
	"color7",
	NULL,
	"",
	KEY_DUMMY,
	"The color value for the eighth color.\n"
	"This is only used if maxColors is set to 8 or 16.\n",
	0
    },
    {
	"color8",
	NULL,
	"",
	KEY_DUMMY,
	"The color value for the nineth color.\n"
	"This is only used if maxColors is set to 16.\n",
	0
    },
    {
	"color9",
	NULL,
	"",
	KEY_DUMMY,
	"The color value for the tenth color.\n"
	"This is only used if maxColors is set to 16.\n",
	0
    },
    {
	"color10",
	NULL,
	"",
	KEY_DUMMY,
	"The color value for the eleventh color.\n"
	"This is only used if maxColors is set to 16.\n",
	0
    },
    {
	"color11",
	NULL,
	"",
	KEY_DUMMY,
	"The color value for the twelfth color.\n"
	"This is only used if maxColors is set to 16.\n",
	0
    },
    {
	"color12",
	NULL,
	"",
	KEY_DUMMY,
	"The color value for the thirteenth color.\n"
	"This is only used if maxColors is set to 16.\n",
	0
    },
    {
	"color13",
	NULL,
	"",
	KEY_DUMMY,
	"The color value for the fourteenth color.\n"
	"This is only used if maxColors is set to 16.\n",
	0
    },
    {
	"color14",
	NULL,
	"",
	KEY_DUMMY,
	"The color value for the fifteenth color.\n"
	"This is only used if maxColors is set to 16.\n",
	0
    },
    {
	"color15",
	NULL,
	"",
	KEY_DUMMY,
	"The color value for the sixteenth color.\n"
	"This is only used if maxColors is set to 16.\n",
	0
    },
    {
	"hudColor",
	NULL,
	"2",
	KEY_DUMMY,
	"Which color number to use for drawing the HUD.\n",
	0
    },
    {
	"hudHLineColor",
	NULL,
	"2",
	KEY_DUMMY,
	"Which color number to use for drawing the horizontal lines\n"
	"in the HUD.\n",
	0
    },
    {
	"hudVLineColor",
	NULL,
	"0",
	KEY_DUMMY,
	"Which color number to use for drawing the vertical lines\n"
	"in the HUD.\n",
	0
    },
    {
	"hudItemsColor",
	NULL,
	"2",
	KEY_DUMMY,
	"Which color number to use for drawing owned items on the HUD.\n",
	0
    },
    {
	"hudRadarEnemyColor",
	NULL,
	"1",
	KEY_DUMMY,
	"Which color number to use for drawing hudradar dots\n"
	"that represent enemy ships.\n",
	0
    },
    {
	"hudRadarOtherColor",
	NULL,
	"2",
	KEY_DUMMY,
	"Which color number to use for drawing hudradar dots\n"
	"that represent friendly ships or other objects.\n",
	0
    },
    {
	"hudRadarDotSize",
	NULL,
	"8",
	KEY_DUMMY,
	"Which size to use for drawing the hudradar dots.\n",
	0
    },
    {
	"hudRadarScale",
	NULL,
	"1.5",
	KEY_DUMMY,
	"The relative size of the hudradar.\n",
	0
    },
    {
	"hudRadarLimit",
	NULL,
	"0.05",
	KEY_DUMMY,
	"Hudradar dots closer than this to your ship are not drawn.\n"
	"A value of 1.0 means that the dots are not drawn for ships in\n"
	"your active view area.\n",
	0
    },
    {
	"hudSize",
	NULL,
	"90",
	KEY_DUMMY,
	"Which size to use for drawing the hud.\n",
	0
    },
    {
	"dirPtrColor",
	NULL,
	"0",
	KEY_DUMMY,
	"Which color number to use for drawing the direction pointer hack.\n",
	0
    },
    {
	"shipShapesHackColor",
	NULL,
	"0",
	KEY_DUMMY,
	"Which color number to use for drawing the shipshapes hack.\n",
	0
    },
    {
	"hudLockColor",
	NULL,
	"0",
	KEY_DUMMY,
	"Which color number to use for drawing the lock on the HUD.\n",
	0
    },
    {
	"visibilityBorderColor",
	NULL,
	"2",
	KEY_DUMMY,
	"Which color number to use for drawing the visibility border.\n",
	0
    },
    {
	"fuelGaugeColor",
	NULL,
	"0",
	KEY_DUMMY,
	"Which color number to use for drawing the fuel gauge.\n",
	0
    },
    {
	"msgScanBallColor",
	NULL,
	"3",
	KEY_DUMMY,
	"Which color number to use for drawing ball message warning.\n",
	0
    },
    {
	"msgScanSafeColor",
	NULL,
	"4",
	KEY_DUMMY,
	"Which color number to use for drawing safe message.\n",
	0
    },
    {
	"msgScanCoverColor",
	NULL,
	"2",
	KEY_DUMMY,
	"Which color number to use for drawing cover message.\n",
	0
    },
    {
	"msgScanPopColor",
	NULL,
	"11",
	KEY_DUMMY,
	"Which color number to use for drawing pop message.\n",
	0
    },
    {
	"zeroLivesColor",
	NULL,
	"1",
	KEY_DUMMY,
	"Which color to associate with ships with zero lives left.\n"
	"This can be used to paint for example ship and base names.\n",
	0
    },
    {
	"oneLifeColor",
	NULL,
	"3",
	KEY_DUMMY,
	"Which color to associate with ships with one life left.\n"
	"This can be used to paint for example ship and base names.\n",
	0
    },
    {
	"twoLivesColor",
	NULL,
	"11",
	KEY_DUMMY,
	"Which color to associate with ships with two lives left.\n"
	"This can be used to paint for example ship and base names.\n",
	0
    },
    {
	"manyLivesColor",
	NULL,
	"4",
	KEY_DUMMY,
	"Which color to associate with ships with more than two lives left.\n"
	"This can be used to paint for example ship and base names.\n",
	0
    },
    {
	"selfLWColor",
	NULL,
	"1",
	KEY_DUMMY,
	"Which color to use to paint your ship in when on last life.\n"
	"Original color for this is red.\n",
	0
    },
    {
	"enemyLWColor",
	NULL,
	"1",
	KEY_DUMMY,
	"Which color to use to paint enemy ships in when on last life.\n"
	"Original color for this is red.\n",
	0
    },
    {
	"teamLWColor",
	NULL,
	"2",
	KEY_DUMMY,
	"Which color to use to paint teammate ships in when on last life.\n"
	"Original color for this is green.\n",
	0
    },
    {
	"shipNameColor",
	NULL,
	"2",
	KEY_DUMMY,
	"Which color number to use for drawing names of ships.\n",
	0
    },
    {
	"baseNameColor",
	NULL,
	"1",
	KEY_DUMMY,
	"Which color number to use for drawing names of bases.\n",
	0
    },
    {
	"mineNameColor",
	NULL,
	"2",
	KEY_DUMMY,
	"Which color number to use for drawing names of mines.\n",
	0
    },
    {
	"ballColor",
	NULL,
	"1",
	KEY_DUMMY,
	"Which color number to use for drawing balls.\n",
	0
    },
    {
	"connColor",
	NULL,
	"2",
	KEY_DUMMY,
	"Which color number to use for drawing connectors.\n",
	0
    },
    {
	"fuelMeterColor",
	NULL,
	"0",
	KEY_DUMMY,
	"Which color number to use for drawing the fuel meter.\n",
	0
    },
    {
	"powerMeterColor",
	NULL,
	"0",
	KEY_DUMMY,
	"Which color number to use for drawing the power meter.\n",
	0
    },
    {
	"turnSpeedMeterColor",
	NULL,
	"0",
	KEY_DUMMY,
	"Which color number to use for drawing the turn speed meter.\n",
	0
    },
    {
	"packetSizeMeterColor",
	NULL,
	"0",
	KEY_DUMMY,
	"Which color number to use for drawing the packet size meter.\n"
	"Each bar is equavalent to 1024 bytes, for a maximum of 4096 bytes.\n",
	0
    },
    {
	"packetLossMeterColor",
	NULL,
	"3",
	KEY_DUMMY,
	"Which color number to use for drawing the packet loss meter.\n"
	"This gives the percentage of lost frames due to network failure.\n",
	0
    },
    {
	"packetDropMeterColor",
	NULL,
	"0",
	KEY_DUMMY,
	"Which color number to use for drawing the packet drop meter.\n"
	"This gives the percentage of dropped frames due to display slowness.\n",
	0
    },
    {
	"packetLagMeterColor",
	NULL,
	"0",
	KEY_DUMMY,
	"Which color number to use for drawing the packet lag meter.\n"
	"This gives the amount of lag in frames over the past one second.\n",
	0
    },
    {
	"temporaryMeterColor",
	NULL,
	"3",
	KEY_DUMMY,
	"Which color number to use for drawing temporary meters.\n",
	0
    },
    {
	"meterBorderColor",
	NULL,
	"2",
	KEY_DUMMY,
	"Which color number to use for drawing borders of meters.\n",
	0
    },
    {
	"windowColor",
	NULL,
	"8",
	KEY_DUMMY,
	"Which color number to use for drawing windows.\n",
	0
    },
    {
	"buttonColor",
	NULL,
	"2",
	KEY_DUMMY,
	"Which color number to use for drawing buttons.\n",
	0
    },
    {
	"borderColor",
	NULL,
	"1",
	KEY_DUMMY,
	"Which color number to use for drawing borders.\n",
	0
    },
    {
	"clockColor",
	NULL,
	"1",
	KEY_DUMMY,
	"Which color number to use for drawing the clock.\n"
	"The clock is displayed in the top right of the score window.\n",
	0
    },
    {
	"scoreColor",
	NULL,
	"1",
	KEY_DUMMY,
	"Which color number to use for drawing score list entries.\n",
	0
    },
    {
	"scoreSelfColor",
	NULL,
	"3",
	KEY_DUMMY,
	"Which color number to use for drawing your own score.\n",
	0
    },
    {
	"scoreInactiveColor",
	NULL,
	"12",
	KEY_DUMMY,
	"Which color number to use for drawing inactive players's scores.\n",
	0
    },
    {
	"scoreInactiveSelfColor",
	NULL,
	"12",
	KEY_DUMMY,
	"Which color number to use for drawing your score when inactive.\n",
	0
    },
    {
	"scoreOwnTeamColor",
	NULL,
	"4",
	KEY_DUMMY,
	"Which color number to use for drawing your own team score.\n",
	0
    },

    {
	"scoreEnemyTeamColor",
	NULL,
	"11",
	KEY_DUMMY,
	"Which color number to use for drawing enemy team score.\n",
	0
    },
    {
	"scoreObjectColor",
	NULL,
	"4",
	KEY_DUMMY,
	"Which color number to use for drawing score objects.\n",
	0
    },
    {
	"scoreObjectTime",
	NULL,
	"3.0",
	KEY_DUMMY,
	"How many seconds score objects remain visible on the map.\n",
	0
    },
    {
	"baseWarningType",
	NULL,
	"2",
	KEY_DUMMY,
	"Which type of base warning you prefer.\n"
	"A value of 0 disables base warning.\n"
	"A value of 1 draws a red box on a base when someone has died.\n"
	"A value of 2 makes the base name flash when someone has died.\n"
	"A value of 3 combines the effects of values 1 and 2.\n",
	0
    },
    {
	"wallColor",
	NULL,
	"2",
	KEY_DUMMY,
	"Which color number to use for drawing walls.\n",
	0
    },
    {
	"fuelColor",
	NULL,
	"3",
	KEY_DUMMY,
	"Which color number to use for drawing fuel stations.\n",
	0
    },
    {
	"wallRadarColor",
	NULL,
	"8",
	KEY_DUMMY,
	"Which color number to use for drawing walls on the radar.\n"
	"Valid values all even numbers smaller than maxColors.\n",
	0
    },
    {
	"decorColor",
	NULL,
	"6",
	KEY_DUMMY,
	"Which color number to use for drawing decorations.\n",
	0
    },
    {
	"backgroundPointColor",
	NULL,
	"2",
	KEY_DUMMY,
	"Which color number to use for drawing background points.\n",
	0
    },
    {
	"team0Color",
	NULL,
	"0",
	KEY_DUMMY,
	"Which color number to use for drawing team 0 objects.\n",
	0
    },
    {
	"team1Color",
	NULL,
	"0",
	KEY_DUMMY,
	"Which color number to use for drawing team 1 objects.\n",
	0
    },
    {
	"team2Color",
	NULL,
	"0",
	KEY_DUMMY,
	"Which color number to use for drawing team 2 objects.\n",
	0
    },
    {
	"team3Color",
	NULL,
	"0",
	KEY_DUMMY,
	"Which color number to use for drawing team 3 objects.\n",
	0
    },
    {
	"team4Color",
	NULL,
	"0",
	KEY_DUMMY,
	"Which color number to use for drawing team 4 objects.\n",
	0
    },
    {
	"team5Color",
	NULL,
	"0",
	KEY_DUMMY,
	"Which color number to use for drawing team 5 objects.\n",
	0
    },
    {
	"team6Color",
	NULL,
	"0",
	KEY_DUMMY,
	"Which color number to use for drawing team 6 objects.\n",
	0
    },
    {
	"team7Color",
	NULL,
	"0",
	KEY_DUMMY,
	"Which color number to use for drawing team 7 objects.\n",
	0
    },
    {
	"team8Color",
	NULL,
	"0",
	KEY_DUMMY,
	"Which color number to use for drawing team 8 objects.\n",
	0
    },
    {
	"team9Color",
	NULL,
	"0",
	KEY_DUMMY,
	"Which color number to use for drawing team 9 objects.\n",
	0
    },
    {
	"showDecor",
	NULL,
	"Yes",
	KEY_DUMMY,
	"Should decorations be displayed on the screen and radar?\n",
	0
    },
    {
	"decorRadarColor",
	NULL,
	"6",
	KEY_DUMMY,
	"Which color number to use for drawing decorations on the radar.\n"
	"Valid values are all even numbers smaller than maxColors.\n",
	0
    },
    {
	"messagesColor",
	NULL,
	"12",
	KEY_DUMMY,
	"Which color number to use for drawing messages.\n",
	0
    },
    {
	"oldMessagesColor",
	NULL,
	"13",
	KEY_DUMMY,
	"Which color number to use for drawing old messages.\n",
	0
    },
    {
	"clientRanker",
	NULL,
	"No",
	KEY_DUMMY,
	"Scan messages and make personal kill/death ranking.\n",
	0
    },
    {
	"outlineDecor",
	NULL,
	"No",
	KEY_DUMMY,
	"Draws only the outline of the map decoration.\n",
	0
    },
    {
	"filledDecor",
	NULL,
	"No",
	KEY_DUMMY,
	"Draws filled decorations.\n",
	0
    },
    {
	"texturedDecor",
	NULL,
	"No",
	KEY_DUMMY,
	"Draws the map decoration filled with a texture pattern.\n"
	"See also the decorTextureFile and texturedWalls options.\n",
	0
    },
    {
	"decorTextureFile",
	NULL,
	"",
	KEY_DUMMY,
	"Specify a XPM format pixmap file to load the decor texture from.\n",
	0
    },
    {
	"targetRadarColor",
	NULL,
	"4",
	KEY_DUMMY,
	"Which color number to use for drawing targets on the radar.\n"
	"Valid values are all even numbers smaller than maxColors.\n",
	0
    },
    {
	"sparkColors",
	NULL,
	"5,6,7,3",
	KEY_DUMMY,
	"Which color numbers to use for spark and debris particles.\n",
	0
    },
    {
	"modifierBank1",
	NULL,
	"",
	KEY_DUMMY,
	"The default weapon modifier values for the first modifier bank.\n",
	0
    },
    {
	"modifierBank2",
	NULL,
	"",
	KEY_DUMMY,
	"The default weapon modifier values for the second modifier bank.\n",
	0
    },
    {
	"modifierBank3",
	NULL,
	"",
	KEY_DUMMY,
	"The default weapon modifier values for the third modifier bank.\n",
	0
    },
    {
	"modifierBank4",
	NULL,
	"",
	KEY_DUMMY,
	"The default weapon modifier values for the fourth modifier bank.\n",
	0
    },
    {
	"keyTurnLeft",
	NULL,
	"a",
	KEY_TURN_LEFT,
	"Turn left (anti-clockwise).\n",
	0
    },
    {
	"keyTurnRight",
	NULL,
	"s",
	KEY_TURN_RIGHT,
	"Turn right (clockwise).\n",
	0
    },
    {
	"keyThrust",
	NULL,
	"Shift_R Shift_L",
	KEY_THRUST,
	"Thrust.\n",
	0
    },
    {
	"keyShield",
	NULL,
	"space Caps_Lock",
	KEY_SHIELD,
	"Raise or toggle the shield.\n",
	0
    },
    {
	"keyFireShot",
	NULL,
	"Return Linefeed",
	KEY_FIRE_SHOT,
	"Fire shot.\n"
	"Note that shields must be down to fire.\n",
	0
    },
    {
	"keyFireMissile",
	NULL,
	"backslash",
	KEY_FIRE_MISSILE,
	"Fire smart missile.\n",
	0
    },
    {
	"keyFireTorpedo",
	NULL,
	"quoteright",
	KEY_FIRE_TORPEDO,
	"Fire unguided torpedo.\n",
	0
    },
    {
	"keyFireHeat",
	NULL,
	"semicolon",
	KEY_FIRE_HEAT,
	"Fire heatseeking missile.\n",
	0
    },
    {
	"keyFireLaser",
	NULL,
	"slash",
	KEY_FIRE_LASER,
	"Activate laser beam.\n",
	0
    },
    {
	"keyDropMine",
	NULL,
	"Tab",
	KEY_DROP_MINE,
	"Drop a stationary mine.\n",
	0
    },
    {
	"keyDetachMine",
	NULL,
	"bracketright",
	KEY_DETACH_MINE,
	"Detach a moving mine.\n",
	0
    },
    {
	"keyDetonateMines",
	NULL,
	"equal",
	KEY_DETONATE_MINES,
	"Detonate the mine you have dropped or thrown, which is closest to you.\n",
	0
    },
    {
	"keyLockClose",
	NULL,
	"Select Up",
	KEY_LOCK_CLOSE,
	"Lock on closest player.\n",
	0
    },
    {
	"keyLockNextClose",
	NULL,
	"Down",
	KEY_LOCK_NEXT_CLOSE,
	"Lock on next closest player.\n",
	0
    },
    {
	"keyLockNext",
	NULL,
	"Next Right",
	KEY_LOCK_NEXT,
	"Lock on next player.\n",
	0
    },
    {
	"keyLockPrev",
	NULL,
	"Prior Left",
	KEY_LOCK_PREV,
	"Lock on previous player.\n",
	0
    },
    {
	"keyRefuel",
	NULL,
	"f Control_L Control_R",
	KEY_REFUEL,
	"Refuel.\n",
	0
    },
    {
	"keyRepair",
	NULL,
	"f",
	KEY_REPAIR,
	"Repair target.\n",
	0
    },
    {
	"keyCloak",
	NULL,
	"Delete BackSpace",
	KEY_CLOAK,
	"Toggle cloakdevice.\n",
	0
    },
    {
	"keyEcm",
	NULL,
	"bracketleft",
	KEY_ECM,
	"Use ECM.\n",
	0
    },
    {
	"keySelfDestruct",
	NULL,
	"End",
	KEY_SELF_DESTRUCT,
	"Toggle self destruct.\n",
	0
    },
    {
	"keyIdMode",
	NULL,
	"u",
	KEY_ID_MODE,
	"Toggle User mode (show real names).\n",
	0
    },
    {
	"keyPause",
	NULL,
	"Pause",
	KEY_PAUSE,
	"Toggle pause mode.\n"
	"When the ship is stationary on its homebase.\n",
	0
    },
    {
	"keySwapSettings",
	NULL,
	"Escape",
	KEY_SWAP_SETTINGS,
	"Swap control settings.\n"
	"These are the power, turn speed and turn resistance settings.\n",
	0
    },
    {
        "keySwapScaleFactor",
        NULL,
        "",
        KEY_SWAP_SCALEFACTOR,
        "Swap scalefactor settings.\n"
        "These are the scalefactor settings.\n",
	0
    },
    {
	"keyChangeHome",
	NULL,
	"Home h",
	KEY_CHANGE_HOME,
	"Change home base.\n"
	"When the ship is stationary on a new homebase.\n",
	0
    },
    {
	"keyConnector",
	NULL,
	"Control_L",
	KEY_CONNECTOR,
	"Connect to a ball.\n",
	0
    },
    {
	"keyDropBall",
	NULL,
	"d",
	KEY_DROP_BALL,
	"Drop a ball.\n",
	0
    },
    {
	"keyTankNext",
	NULL,
	"e",
	KEY_TANK_NEXT,
	"Use the next tank.\n",
	0
    },
    {
	"keyTankPrev",
	NULL,
	"w",
	KEY_TANK_PREV,
	"Use the the previous tank.\n",
	0
    },
    {
	"keyTankDetach",
	NULL,
	"r",
	KEY_TANK_DETACH,
	"Detach the current tank.\n",
	0
    },
    {
	"keyIncreasePower",
	NULL,
	"KP_Multiply",
	KEY_INCREASE_POWER,
	"Increase engine power.\n",
	0
    },
    {
	"keyDecreasePower",
	NULL,
	"KP_Divide",
	KEY_DECREASE_POWER,
	"Decrease engine power.\n",
	0
    },
    {
	"keyIncreaseTurnspeed",
	NULL,
	"KP_Add",
	KEY_INCREASE_TURNSPEED,
	"Increase turnspeed.\n",
	0
    },
    {
	"keyDecreaseTurnspeed",
	NULL,
	"KP_Subtract",
	KEY_DECREASE_TURNSPEED,
	"Decrease turnspeed.\n",
	0
    },
    {
	"keyTransporter",
	NULL,
	"t",
	KEY_TRANSPORTER,
	"Use transporter to steal an item.\n",
	0
    },
    {
	"keyDeflector",
	NULL,
	"o",
	KEY_DEFLECTOR,
	"Toggle deflector.\n",
	0
    },
    {
	"keyHyperJump",
	NULL,
	"q",
	KEY_HYPERJUMP,
	"Teleport.\n",
	0
    },
    {
	"keyPhasing",
	NULL,
	"p",
	KEY_PHASING,
	"Use phasing device.\n",
	0
    },
    {
	"keyTalk",
	NULL,
	"m",
	KEY_TALK,
	"Toggle talk window.\n",
	0
    },
    {
	"keyToggleNuclear",
	NULL,
	"n",
	KEY_TOGGLE_NUCLEAR,
	"Toggle nuclear weapon modifier.\n",
	0
    },
    {
	"keyToggleCluster",
	NULL,
	"c",
	KEY_TOGGLE_CLUSTER,
	"Toggle cluster weapon modifier.\n",
	0
    },
    {
	"keyToggleImplosion",
	NULL,
	"i",
	KEY_TOGGLE_IMPLOSION,
	"Toggle implosion weapon modifier.\n",
	0
    },
    {
	"keyToggleVelocity",
	NULL,
	"v",
	KEY_TOGGLE_VELOCITY,
	"Toggle explosion velocity weapon modifier.\n",
	0
    },
    {
	"keyToggleMini",
	NULL,
	"x",
	KEY_TOGGLE_MINI,
	"Toggle mini weapon modifier.\n",
	0
    },
    {
	"keyToggleSpread",
	NULL,
	"z",
	KEY_TOGGLE_SPREAD,
	"Toggle spread weapon modifier.\n",
	0
    },
    {
	"keyTogglePower",
	NULL,
	"b",
	KEY_TOGGLE_POWER,
	"Toggle power weapon modifier.\n",
	0
    },
    {
	"keyToggleCompass",
	NULL,
	"KP_7",
	KEY_TOGGLE_COMPASS,
	"Toggle HUD/radar compass lock.\n",
	0
    },
    {
	"keyToggleAutoPilot",
	NULL,
	"h",
	KEY_TOGGLE_AUTOPILOT,
	"Toggle automatic pilot mode.\n",
	0
    },
    {
	"keyToggleLaser",
	NULL,
	"l",
	KEY_TOGGLE_LASER,
	"Toggle laser modifier.\n",
	0
    },
    {
	"keyEmergencyThrust",
	NULL,
	"j",
	KEY_EMERGENCY_THRUST,
	"Pull emergency thrust handle.\n",
	0
    },
    {
	"keyEmergencyShield",
	NULL,
	"g",
	KEY_EMERGENCY_SHIELD,
	"Toggle emergency shield power.\n",
	0
    },
    {
	"keyTractorBeam",
	NULL,
	"comma",
	KEY_TRACTOR_BEAM,
	"Use tractor beam in attract mode.\n",
	0
    },
    {
	"keyPressorBeam",
	NULL,
	"period",
	KEY_PRESSOR_BEAM,
	"Use tractor beam in repulse mode.\n",
	0
    },
    {
	"keyClearModifiers",
	NULL,
	"k",
	KEY_CLEAR_MODIFIERS,
	"Clear current weapon modifiers.\n",
	0
    },
    {
	"keyLoadModifiers1",
	NULL,
	"1",
	KEY_LOAD_MODIFIERS_1,
	"Load the weapon modifiers from bank 1.\n",
	0
    },
    {
	"keyLoadModifiers2",
	NULL,
	"2",
	KEY_LOAD_MODIFIERS_2,
	"Load the weapon modifiers from bank 2.\n",
	0
    },
    {
	"keyLoadModifiers3",
	NULL,
	"3",
	KEY_LOAD_MODIFIERS_3,
	"Load the weapon modifiers from bank 3.\n",
	0
    },
    {
	"keyLoadModifiers4",
	NULL,
	"4",
	KEY_LOAD_MODIFIERS_4,
	"Load the weapon modifiers from bank 4.\n",
	0
    },
    {
	"keyToggleOwnedItems",
	NULL,
	"KP_8",
	KEY_TOGGLE_OWNED_ITEMS,
	"Toggle list of owned items on HUD.\n",
	0
    },
    {
	"keyToggleMessages",
	NULL,
	"KP_9",
	KEY_TOGGLE_MESSAGES,
	"Toggle showing of messages.\n",
	0
    },
    {
	"keyReprogram",
	NULL,
	"quoteleft",
	KEY_REPROGRAM,
	"Reprogram modifier or lock bank.\n",
	0
    },
    {
	"keyLoadLock1",
	NULL,
	"5",
	KEY_LOAD_LOCK_1,
	"Load player lock from bank 1.\n",
	0
    },
    {
	"keyLoadLock2",
	NULL,
	"6",
	KEY_LOAD_LOCK_2,
	"Load player lock from bank 2.\n",
	0
    },
    {
	"keyLoadLock3",
	NULL,
	"7",
	KEY_LOAD_LOCK_3,
	"Load player lock from bank 3.\n",
	0
    },
    {
	"keyLoadLock4",
	NULL,
	"8",
	KEY_LOAD_LOCK_4,
	"Load player lock from bank 4.\n",
	0
    },
    {
	"keyToggleRecord",
	NULL,
	"KP_5",
	KEY_TOGGLE_RECORD,
	"Toggle recording of session (see recordFile).\n",
	0
    },
    {
        "keyToggleRadarScore",
        NULL,
        "",
        KEY_TOGGLE_RADAR_SCORE,
        "Toggles the radar and score windows on and off.\n",
	0
    },
    {
	"keySelectItem",
	NULL,
	"KP_0 KP_Insert",
	KEY_SELECT_ITEM,
	"Select an item to lose.\n",
	0
    },
    {
	"keyLoseItem",
	NULL,
	"KP_Delete KP_Decimal",
	KEY_LOSE_ITEM,
	"Lose the selected item.\n",
	0
    },
#ifndef _WINDOWS
    {
	"keyPrintMessagesStdout",
	NULL,
	"Print",
	KEY_PRINT_MSGS_STDOUT,
	"Print the current messages to stdout.\n",
	0
    },
    {
	"keyTalkCursorLeft",
	NULL,
	"Left",
	KEY_TALK_CURSOR_LEFT,
	"Move Cursor to the left in the talk window.\n",
	0
    },
    {
	"keyTalkCursorRight",
	NULL,
	"Right",
	KEY_TALK_CURSOR_RIGHT,
	"Move Cursor to the right in the talk window.\n",
	0
    },
    {
	"keyTalkCursorUp",
	NULL,
	"Up",
	KEY_TALK_CURSOR_UP,
	"Browsing in the history of the talk window.\n",
	0
    },
    {
	"keyTalkCursorDown",
	NULL,
	"Down",
	KEY_TALK_CURSOR_DOWN,
	"Browsing in the history of the talk window.\n",
	0
    },
#endif
    {
	"keyPointerControl",
	NULL,
	"KP_Enter",
	KEY_POINTER_CONTROL,
	"Toggle pointer control.\n",
	0
    },
    {
	"pointerButton1",
	NULL,
	"keyFireShot",
	KEY_DUMMY,
	"The key to activate when pressing the first mouse button.\n",
	0
    },
    {
	"pointerButton2",
	NULL,
	"keyThrust",
	KEY_DUMMY,
	"The key to activate when pressing the second mouse button.\n",
	0
    },
    {
	"pointerButton3",
	NULL,
	"keyDropBall",
	KEY_DUMMY,
	"The key to activate when pressing the third mouse button.\n",
	0
    },
    {
	"pointerButton4",
	NULL,
	"",
	KEY_DUMMY,
	"The key to activate when pressing the fourth mouse button.\n",
	0
    },
    {
	"pointerButton5",
	NULL,
	"",
	KEY_DUMMY,
	"The key to activate when pressing the fifth mouse button.\n",
	0
    },
    {
	"maxFPS",
	NULL,
	"100",
	KEY_DUMMY,
	"Set client's maximum FPS supported.\n",
	0
    },
    {
	"recordFile",
	NULL,
	"",
	KEY_DUMMY,
	"An optional file where a recording of a game can be made.\n"
	"If this file is undefined then recording isn't possible.\n",
	0
    },
    {
	"clientRankFile",
	NULL,
	"",
	KEY_DUMMY,
	"An optional file where clientside kill/death rank is stored.\n",
	0
    },
    {
	"clientRankHTMLFile",
	NULL,
	"",
	KEY_DUMMY,
	"An optional file where clientside kill/death rank is\n"
	"published in HTML format. \n",
	0
    },
    {
	"clientRankHTMLNOJSFile",
	NULL,
	"",
	KEY_DUMMY,
	"An optional file where clientside kill/death rank is\n"
	"published in HTML format, w/o JavaScript. \n",
	0
    },
    {
	"clientPortStart",
	NULL,
	"0",
	KEY_DUMMY,
	"Use UDP ports clientPortStart - clientPortEnd (for firewalls).\n",
	0
    },
    {
	"clientPortEnd",
	NULL,
	"0",
	KEY_DUMMY,
	"Use UDP ports clientPortStart - clientPortEnd (for firewalls).\n",
	0
    },
#ifdef _WINDOWS
    {
	"threadedDraw",
	NULL,
	"No",
	KEY_DUMMY,
	"Tell Windows to do the heavy BitBlt in another thread\n",
	0
	},
    {
	"radarDivisor",
	NULL,
	"1",
	KEY_DUMMY,
	"Specifies how many frames between radar window updates.\n",
	0
	},
#endif
    {
	"scaleFactor",
	NULL,
	"1.0",
	KEY_DUMMY,
	"Specifies scaling factor for the drawing window.\n",
	0
    },
    {
        "altScaleFactor",
        NULL,
        "2.0",
        KEY_DUMMY,
        "Specifies alternative scaling factor for the drawing window.\n",
	0
    },
#ifdef SOUND
    {
	"sounds",
	NULL,
	conf_soundfile_string,
	KEY_DUMMY,
	"Specifies the sound file.\n",
	0
    },
    {
	"maxVolume",
	NULL,
	"100",
	KEY_DUMMY,
	"Specifies the volume to play sounds with.\n",
	0
    },
    {
	"audioServer",
	NULL,
	"",
	KEY_DUMMY,
	"Specifies the audio server to use.\n",
	0
    },
#endif
#ifdef DEVELOPMENT
    {
        "test",
        NULL,
        "",
        KEY_DUMMY,
        "Which development testing parameters to use?\n",
	0
    },
#endif
/* talk macros: */
    {
	"keySendMsg1",
	NULL,
	"F1",
	KEY_MSG_1,
	"Sends the talkmessage stored in msg1.\n",
	0
    },
    {
	"msg1",
	NULL,
	"",
	KEY_DUMMY,
	"Talkmessage 1.\n",
	0
    },
    {
	"keySendMsg2",
	NULL,
	"F2",
	KEY_MSG_2,
	"Sends the talkmessage stored in msg2.\n",
	0
    },
    {
	"msg2",
	NULL,
	"",
	KEY_DUMMY,
	"Talkmessage 2.\n",
	0
    },
    {
	"keySendMsg3",
	NULL,
	"F3",
	KEY_MSG_3,
	"Sends the talkmessage stored in msg3.\n",
	0
    },
    {
	"msg3",
	NULL,
	"",
	KEY_DUMMY,
	"Talkmessage 3.\n",
	0
    },
    {
	"keySendMsg4",
	NULL,
	"F4",
	KEY_MSG_4,
	"Sends the talkmessage stored in msg4.\n",
	0
    },
    {
	"msg4",
	NULL,
	"",
	KEY_DUMMY,
	"Talkmessage 4.\n",
	0
    },
    {
	"keySendMsg5",
	NULL,
	"F5",
	KEY_MSG_5,
	"Sends the talkmessage stored in msg5.\n",
	0
    },
    {
	"msg5",
	NULL,
	"",
	KEY_DUMMY,
	"Talkmessage 5.\n",
	0
    },
    {
	"keySendMsg6",
	NULL,
	"F6",
	KEY_MSG_6,
	"Sends the talkmessage stored in msg6.\n",
	0
    },
    {
	"msg6",
	NULL,
	"",
	KEY_DUMMY,
	"Talkmessage 6.\n",
	0
    },
    {
	"keySendMsg7",
	NULL,
	"F7",
	KEY_MSG_7,
	"Sends the talkmessage stored in msg7.\n",
	0
    },
    {
	"msg7",
	NULL,
	"",
	KEY_DUMMY,
	"Talkmessage 7.\n",
	0
    },
    {
	"keySendMsg8",
	NULL,
	"F8",
	KEY_MSG_8,
	"Sends the talkmessage stored in msg8.\n",
	0
    },
    {
	"msg8",
	NULL,
	"",
	KEY_DUMMY,
	"Talkmessage 8.\n",
	0
    },
    {
	"keySendMsg9",
	NULL,
	"F9",
	KEY_MSG_9,
	"Sends the talkmessage stored in msg9.\n",
	0
    },
    {
	"msg9",
	NULL,
	"",
	KEY_DUMMY,
	"Talkmessage 9.\n",
	0
    },
    {
	"keySendMsg10",
	NULL,
	"F10",
	KEY_MSG_10,
	"Sends the talkmessage stored in msg10.\n",
	0
    },
    {
	"msg10",
	NULL,
	"",
	KEY_DUMMY,
	"Talkmessage 10.\n",
	0
    },
    {
	"keySendMsg11",
	NULL,
	"F11",
	KEY_MSG_11,
	"Sends the talkmessage stored in msg11.\n",
	0
    },
    {
	"msg11",
	NULL,
	"",
	KEY_DUMMY,
	"Talkmessage 11.\n",
	0
    },
    {
	"keySendMsg12",
	NULL,
	"F12",
	KEY_MSG_12,
	"Sends the talkmessage stored in msg12.\n",
	0
    },
    {
	"msg12",
	NULL,
	"",
	KEY_DUMMY,
	"Talkmessage 12.\n",
	0
    },
    {
	"keySendMsg13",
	NULL,
	"",
	KEY_MSG_13,
	"Sends the talkmessage stored in msg13.\n",
	0
    },
    {
	"msg13",
	NULL,
	"",
	KEY_DUMMY,
	"Talkmessage 13.\n",
	0
    },
    {
	"keySendMsg14",
	NULL,
	"",
	KEY_MSG_14,
	"Sends the talkmessage stored in msg14.\n",
	0
    },
    {
	"msg14",
	NULL,
	"",
	KEY_DUMMY,
	"Talkmessage 14.\n",
	0
    },
    {
	"keySendMsg15",
	NULL,
	"",
	KEY_MSG_15,
	"Sends the talkmessage stored in msg15.\n",
	0
    },
    {
	"msg15",
	NULL,
	"",
	KEY_DUMMY,
	"Talkmessage 15.\n",
	0
    },
    {
	"keySendMsg16",
	NULL,
	"",
	KEY_MSG_16,
	"Sends the talkmessage stored in msg16.\n",
	0
    },
    {
	"msg16",
	NULL,
	"",
	KEY_DUMMY,
	"Talkmessage 16.\n",
	0
    },
    {
	"keySendMsg17",
	NULL,
	"",
	KEY_MSG_17,
	"Sends the talkmessage stored in msg17.\n",
	0
    },
    {
	"msg17",
	NULL,
	"",
	KEY_DUMMY,
	"Talkmessage 17.\n",
	0
    },
    {
	"keySendMsg18",
	NULL,
	"",
	KEY_MSG_18,
	"Sends the talkmessage stored in msg18.\n",
	0
    },
    {
	"msg18",
	NULL,
	"",
	KEY_DUMMY,
	"Talkmessage 18.\n",
	0
    },
    {
	"keySendMsg19",
	NULL,
	"",
	KEY_MSG_19,
	"Sends the talkmessage stored in msg19.\n",
	0
    },
    {
	"msg19",
	NULL,
	"",
	KEY_DUMMY,
	"Talkmessage 19.\n",
	0
    },
    {
	"keySendMsg20",
	NULL,
	"",
	KEY_MSG_20,
	"Sends the talkmessage stored in msg20.\n",
	0
    },
    {
	"msg20",
	NULL,
	"",
	KEY_DUMMY,
	"Talkmessage 20.\n",
	0
    },
};

int optionsCount = NELEM(options);

unsigned String_hash(const char *s)
{
    unsigned		hash = 0;

    for (; *s; s++)
	hash = (((hash >> 29) & 7) | (hash << 3)) ^ *s;
    return hash;
}


char* Get_keyHelpString(keys_t key)
{
    int			i;
    char		*nl;
    static char		buf[MAX_CHARS];

    for (i = 0; i < NELEM(options); i++) {
	if (options[i].key == key) {
	    strlcpy(buf, options[i].help, sizeof buf);
	    if ((nl = strchr(buf, '\n')) != NULL)
		*nl = '\0';
	    return buf;
	}
    }

    return NULL;
}


const char* Get_keyResourceString(keys_t key)
{
    int			i;

    for (i = 0; i < NELEM(options); i++) {
	if (options[i].key == key)
	    return options[i].name;
    }

    return NULL;
}


static void Usage(void)
{
    int			i;

    printf(
"Usage: xpilot [-options ...] [server]\n"
"Where options include:\n"
"\n"
	  );
    for (i = 0; i < NELEM(options); i++) {
	printf("    -%s %s\n", options[i].name,
	       (options[i].noArg == NULL) ? "<value>" : "");
	if (options[i].help && options[i].help[0]) {
	    const char *str;
	    printf("        ");
	    for (str = options[i].help; *str; str++) {
		putchar(*str);
		if (*str == '\n' && str[1])
		    printf("        ");
	    }
	    if (str[-1] != '\n')
		putchar('\n');
	}
	if (options[i].fallback && options[i].fallback[0]) {
	    printf("        The default %s: %s.\n",
		   (options[i].key == KEY_DUMMY)
		       ? "value is"
		       : (strchr(options[i].fallback, ' ') == NULL)
			   ? "key is"
			   : "keys are",
		   options[i].fallback);
	}
	printf("\n");
    }
    printf(
"Most of these options can also be set in the .xpilotrc file\n"
"in your home directory.\n"
"Each key option may have multiple keys bound to it and\n"
"one key may be used by multiple key options.\n"
"If no server is specified then xpilot will search\n"
"for servers on your local network.\n"
"For a listing of remote servers try: telnet meta.xpilot.org 4400 \n"
	  );

    exit(1);
}


static int Find_resource(XrmDatabase db, const char *resource,
			 char *result, unsigned size, int *ind)
{
#ifndef _WINDOWS
    int			i;
    int			len;
    char		str_name[80],
			str_class[80],
			*str_type[10];
    XrmValue		rmValue;
    unsigned		hash = String_hash(resource);

    for (i = 0;;) {
	if (hash == options[i].hash && !strcmp(resource, options[i].name)) {
	    *ind = i;
	    break;
	}
	if (++i >= NELEM(options)) {
	    warn("BUG: Can't find option \"%s\"", resource);
	    exit(1);
	}
    }
    sprintf(str_name, "%s.%s", myName, resource);
    sprintf(str_class, "%s.%c%s", myClass, toupper(*resource), resource + 1);

    if (XrmGetResource(db, str_name, str_class, str_type, &rmValue) == True) {
	if (rmValue.addr == NULL)
	    len = 0;
	else {
	    len = MIN(rmValue.size, size - 1);
	    memcpy(result, rmValue.addr, len);
	}
	result[len] = '\0';
	return 1;
    }
    strlcpy(result, options[*ind].fallback, size);

    return 0;

#else	/* _WINDOWS */
    Config_get_resource(resource, result, size, ind);

    return 1;
#endif
}


static int Get_resource(XrmDatabase db,
			const char *resource, char *result, unsigned size)
{
    int			ind;

    return Find_resource(db, resource, result, size, &ind);
}


static int Get_string_resource(XrmDatabase db,
			       const char *resource, char *result,
			       unsigned size)
{
    char		*src, *dst;
    int			ind, val;

    val = Find_resource(db, resource, result, size, &ind);
    src = dst = result;
    while ((*src & 0x7f) == *src && isgraph(*src) == 0 && *src != '\0')
	src++;

    while ((*src & 0x7f) != *src || isgraph(*src) != 0)
	*dst++ = *src++;

    *dst = '\0';

    return val;
}


static void Get_int_resource(XrmDatabase db,
			     const char *resource, int *result)
{
    int			ind;
    char		resValue[MAX_CHARS];

    Find_resource(db, resource, resValue, sizeof resValue, &ind);
    if (sscanf(resValue, "%d", result) <= 0) {
	warn("Bad value \"%s\" for option \"%s\", using default...",
	     resValue, resource);
	sscanf(options[ind].fallback, "%d", result);
    }
}


static void Get_float_resource(XrmDatabase db,
			       const char *resource, double *result)
{
    int			ind;
    double		temp_result;
    char		resValue[MAX_CHARS];

    temp_result = 0.0;
    Find_resource(db, resource, resValue, sizeof resValue, &ind);
    if (sscanf(resValue, "%lf", &temp_result) <= 0) {
	warn("Bad value \"%s\" for option \"%s\", using default...",
	     resValue, resource);
	sscanf(options[ind].fallback, "%lf", &temp_result);
    }
    *result = temp_result;
}


static void Get_bool_resource(XrmDatabase db, const char *resource,
			      bool *result)
{
    int			ind;
    char		resValue[MAX_CHARS];

    Find_resource(db, resource, resValue, sizeof resValue, &ind);
    *result = (ON(resValue) ? true : false);
}


static void Get_bit_resource(XrmDatabase db, const char *resource,
			     long *mask, int bit)
{
    int			ind;
    char		resValue[MAX_CHARS];

    Find_resource(db, resource, resValue, sizeof resValue, &ind);
    if (ON(resValue))
	SET_BIT(*mask, bit);
}

static void Get_shipshape_resource(XrmDatabase db, char **ship_shape)
{
    char		resValue[MAX(2*MSG_LEN, PATH_MAX + 1)];

    Get_resource(db, "shipShape", resValue, sizeof resValue);
    *ship_shape = xp_strdup(resValue);
    if (*ship_shape && **ship_shape && !strchr(*ship_shape, '(' )) {
	/* so it must be the name of shipshape defined in the shipshapefile. */
	Get_resource(db, "shipShapeFile", resValue, sizeof resValue);
	if (resValue[0] != '\0') {
	    FILE *fp = fopen(resValue, "r");
	    if (!fp)
		perror(resValue);
	    else {
		char *ptr;
		char *str;
		char line[1024];
		while (fgets(line, sizeof line, fp)) {
		    if ((str = strstr(line, "(name:" )) != NULL
			|| (str = strstr(line, "(NM:" )) != NULL) {
			str = strchr(str, ':');
			while (*++str == ' ');
			if ((ptr = strchr(str, ')' )) != NULL)
			    *ptr = '\0';
			if (!strcmp(str, *ship_shape)) {
			    /* Gotcha */
			    free(*ship_shape);
			    if (ptr != NULL)
				*ptr = ')';
			    *ship_shape = xp_strdup(line);
			    break;
			}
		    }
		}
		fclose(fp);
	    }
	}
    }
}


#ifndef _WINDOWS
void Get_xpilotrc_file(char *path, unsigned size)
{
    const char		*home = getenv("HOME");
    const char		*defaultFile = ".xpilotrc";
    const char		*optionalFile = getenv("XPILOTRC");

    if (optionalFile != NULL)
	strlcpy(path, optionalFile, size);
    else if (home != NULL) {
	strlcpy(path, home, size);
	strlcat(path, "/", size);
	strlcat(path, defaultFile, size);
    } else
	strlcpy(path, "", size);
}
#endif


#ifndef _WINDOWS
static void Get_file_defaults(XrmDatabase *rDBptr)
{
    int			len;
    char		*ptr,
			*lang = getenv("LANG"),
			*home = getenv("HOME"),
			path[PATH_MAX + 1];
    XrmDatabase		tmpDB;

    sprintf(path, "%s%s", Conf_libdir(), myClass);
    *rDBptr = XrmGetFileDatabase(path);

    if (lang != NULL) {
	sprintf(path, "/usr/lib/X11/%s/app-defaults/%s", lang, myClass);
	if (access(path, 0) == -1)
	    sprintf(path, "/usr/lib/X11/app-defaults/%s", myClass);
    } else
	sprintf(path, "/usr/lib/X11/app-defaults/%s", myClass);
    tmpDB = XrmGetFileDatabase(path);
    XrmMergeDatabases(tmpDB, rDBptr);

    if ((ptr = getenv("XUSERFILESEARCHPATH")) != NULL) {
	sprintf(path, "%s/%s", ptr, myClass);
	tmpDB = XrmGetFileDatabase(path);
	XrmMergeDatabases(tmpDB, rDBptr);
    }
    else if ((ptr = getenv("XAPPLRESDIR")) != NULL) {
	if (lang != NULL) {
	    sprintf(path, "%s/%s/%s", ptr, lang, myClass);
	    if (access(path, 0) == -1)
		sprintf(path, "%s/%s", ptr, myClass);
	} else
	    sprintf(path, "%s/%s", ptr, myClass);
	tmpDB = XrmGetFileDatabase(path);
	XrmMergeDatabases(tmpDB, rDBptr);
    }
    else if (home != NULL) {
	if (lang != NULL) {
	    sprintf(path, "%s/app-defaults/%s/%s", home, lang, myClass);
	    if (access(path, 0) == -1)
		sprintf(path, "%s/app-defaults/%s", home, myClass);
	} else
	    sprintf(path, "%s/app-defaults/%s", home, myClass);
	tmpDB = XrmGetFileDatabase(path);
	XrmMergeDatabases(tmpDB, rDBptr);
    }

    if ((ptr = XResourceManagerString(dpy)) != NULL) {
	tmpDB = XrmGetStringDatabase(ptr);
	XrmMergeDatabases(tmpDB, rDBptr);
    }
    else if (home != NULL) {
	sprintf(path, "%s/.Xdefaults", home);
	tmpDB = XrmGetFileDatabase(path);
	XrmMergeDatabases(tmpDB, rDBptr);
    }

    if ((ptr = getenv("XENVIRONMENT")) != NULL) {
	tmpDB = XrmGetFileDatabase(ptr);
	XrmMergeDatabases(tmpDB, rDBptr);
    }
    else if (home != NULL) {
	sprintf(path, "%s/.Xdefaults-", home);
	len = strlen(path);
	gethostname(&path[len], sizeof path - len);
	path[sizeof path - 1] = '\0';
	tmpDB = XrmGetFileDatabase(path);
	XrmMergeDatabases(tmpDB, rDBptr);
    }

    Get_xpilotrc_file(path, sizeof(path));
    if (path[0] != '\0') {
	tmpDB = XrmGetFileDatabase(path);
	XrmMergeDatabases(tmpDB, rDBptr);
    }
}
#endif	/* _WINDOWS*/


void Parse_options(int *argcp, char **argvp, char *realName, int *port,
		   int *my_team, bool *text, bool *list,
		   bool *join, bool *noLocalMotd,
		   char *nickName, char *dispName, char *hostName,
		   char *shut_msg)
{
    char		*ptr, *str;
    int			i, j;
    int			num;
    int			firstKeyDef;
    keys_t		key;
    SDLKey		ks;

    char		resValue[MAX(2*MSG_LEN, PATH_MAX + 1)];
    XrmDatabase		argDB = 0, rDB = 0;

#ifndef _WINDOWS
    XrmOptionDescRec	*xopt;
    int			size;


    XrmInitialize();

    /*
     * Construct a Xrm Option table from our options array.
     */
    size = sizeof(*xopt) * NELEM(options);
    for (i = 0; i < NELEM(options); i++)
	size += 2 * (strlen(options[i].name) + 2);

    if ((ptr = (char *)malloc(size)) == NULL) {
	error("No memory for options");
	exit(1);
    }
    xopt = (XrmOptionDescRec *)ptr;
    ptr += sizeof(*xopt) * NELEM(options);
    for (i = 0; i < NELEM(options); i++) {
	options[i].hash = String_hash(options[i].name);
	xopt[i].option = ptr;
	xopt[i].option[0] = '-';
	strcpy(&xopt[i].option[1], options[i].name);
	size = strlen(options[i].name) + 2;
	ptr += size;
	xopt[i].specifier = ptr;
	xopt[i].specifier[0] = '.';
	strcpy(&xopt[i].specifier[1], options[i].name);
	ptr += size;
	if (options[i].noArg) {
	    xopt[i].argKind = XrmoptionNoArg;
	    xopt[i].value = (char *)options[i].noArg;
	}
	else {
	    xopt[i].argKind = XrmoptionSepArg;
	    xopt[i].value = NULL;
	}
    }

    XrmParseCommand(&argDB, xopt, NELEM(options), myName, argcp, argvp);

    /*
     * Check for bad arguments.
     */
    for (i = 1; i < *argcp; i++) {
	if (argvp[i][0] == '-' || argvp[i][0] == '+') {
	    warn("Unknown or incomplete option '%s'", argvp[i]);
	    warn("Type: %s -help to see a list of options", argvp[0]);
	    exit(1);
	}
	/* The rest of the arguments are hostnames of servers. */
    }

    if (Get_resource(argDB, "help", resValue, sizeof resValue) != 0)
	Usage();

    if (Get_resource(argDB, "version", resValue, sizeof resValue) != 0) {
	puts(TITLE);
	exit(0);
    }

    if (Get_string_resource(argDB, "display", dispName, MAX_DISP_LEN) == 0
	|| dispName[0] == '\0') {
	if ((ptr = getenv(DISPLAY_ENV)) != NULL)
	    strlcpy(dispName, ptr, MAX_DISP_LEN);
	else
	    strlcpy(dispName, DISPLAY_DEF, MAX_DISP_LEN);
    }
    if ((dpy = XOpenDisplay(dispName)) == NULL) {
	error("Can't open display '%s'", dispName);
	exit(1);
    }

    Get_resource(argDB, "shutdown", shut_msg, MAX_CHARS);

    Get_file_defaults(&rDB);

    XrmMergeDatabases(argDB, &rDB);

#endif

    if ((talk_fast_temp_buf_big
	 = (char *)malloc(TALK_FAST_MSG_SIZE)) != NULL) {
        for (i = 0; i < TALK_FAST_NR_OF_MSGS; ++i) {
            sprintf (talk_fast_temp_buf, "msg%d", i + 1);
            Get_resource(rDB, talk_fast_temp_buf, talk_fast_temp_buf_big,
			 TALK_FAST_MSG_SIZE);
            talk_fast_msgs[i] = xp_strdup (talk_fast_temp_buf_big);
        }
        free (talk_fast_temp_buf_big);
    }
    else {
	for (i = 0; i < TALK_FAST_NR_OF_MSGS; ++i)
	    talk_fast_msgs[i] = NULL;
    }

    Get_resource(rDB, "user", resValue, MAX_NAME_LEN);
    if (resValue[0])
	strlcpy(realName, resValue, MAX_NAME_LEN);

    if (Check_real_name(realName) == NAME_ERROR) {
	xpprintf("Fixing realname from \"%s\" ", realName);
	Fix_real_name(realName);
	xpprintf("to \"%s\".\n", realName);
    }

    Get_resource(rDB, "host", resValue, MAX_HOST_LEN);
    if (resValue[0])
	strlcpy(hostName, resValue, MAX_HOST_LEN);

    if (Check_host_name(hostName) == NAME_ERROR) {
	xpprintf("Fixing host from \"%s\" ", hostName);
	Fix_host_name(hostName);
	xpprintf("to \"%s\".\n", hostName);
    }


    Get_resource(rDB, "name", nickName, MAX_NAME_LEN);
    if (!nickName[0])
	strlcpy(nickName, realName, MAX_NAME_LEN);
    CAP_LETTER(nickName[0]);
    if (nickName[0] < 'A' || nickName[0] > 'Z') {
	warn("Your player name \"%s\" should start with an uppercase letter",
	     nickName);
	exit(1);
    }
    if (Check_nick_name(nickName) == NAME_ERROR) {
	xpprintf("Fixing nick from \"%s\" ", nickName);
	Fix_nick_name(nickName);
	xpprintf("to \"%s\".\n", nickName);
    }

    strlcpy(realname, realName, sizeof(realname));
    strlcpy(nickname, nickName, sizeof(nickname));

    Get_int_resource(rDB, "team", my_team);

    IFWINDOWS( Config_get_name(nickname) );
    IFWINDOWS( Config_get_team(my_team) );

    if (*my_team < 0 || *my_team > 9)
	*my_team = TEAM_NOT_SET;

    Get_int_resource(rDB, "port", port);
    Get_bool_resource(rDB, "text", text);
    Get_bool_resource(rDB, "list", list);
    Get_bool_resource(rDB, "join", join);
    Get_bool_resource(rDB, "noLocalMotd", noLocalMotd);

    Get_shipshape_resource(rDB, &shipShape);
    Validate_shape_str(shipShape);

    Get_float_resource(rDB, "power", &power);
    Get_float_resource(rDB, "turnSpeed", &turnspeed);
    Get_float_resource(rDB, "turnResistance", &turnresistance);
    Get_float_resource(rDB, "altPower", &power_s);
    Get_float_resource(rDB, "altTurnSpeed", &turnspeed_s);
    Get_float_resource(rDB, "altTurnResistance", &turnresistance_s);

    Get_float_resource(rDB, "sparkProb", &spark_prob);
    spark_rand = (int)(spark_prob * MAX_SPARK_RAND + 0.5f);
    Get_int_resource(rDB, "charsPerSecond", &charsPerSecond);

    Get_int_resource(rDB, "backgroundPointDist", &map_point_distance);
    Get_int_resource(rDB, "backgroundPointSize", &map_point_size);
    LIMIT(map_point_size, MIN_MAP_POINT_SIZE, MAX_MAP_POINT_SIZE);
    Get_int_resource(rDB, "sparkSize", &spark_size);
    LIMIT(spark_size, MIN_SPARK_SIZE, MAX_SPARK_SIZE);
    Get_int_resource(rDB, "shotSize", &shot_size);
    LIMIT(shot_size, MIN_SHOT_SIZE, MAX_SHOT_SIZE);
    Get_int_resource(rDB, "teamShotSize", &teamshot_size);
    LIMIT(teamshot_size, MIN_TEAMSHOT_SIZE, MAX_TEAMSHOT_SIZE);
    Get_bool_resource(rDB, "showNastyShots", &showNastyShots);
    Get_bool_resource(rDB, "toggleShield", &toggle_shield);
    Get_bool_resource(rDB, "autoShield", &auto_shield);

    Get_int_resource(rDB, "clientPortStart", &clientPortStart);
    Get_int_resource(rDB, "clientPortEnd", &clientPortEnd);


    Get_resource(rDB, "modifierBank1", modBankStr[0], sizeof modBankStr[0]);
    Get_resource(rDB, "modifierBank2", modBankStr[1], sizeof modBankStr[1]);
    Get_resource(rDB, "modifierBank3", modBankStr[2], sizeof modBankStr[2]);
    Get_resource(rDB, "modifierBank4", modBankStr[3], sizeof modBankStr[3]);

    Get_float_resource(rDB, "scoreObjectTime", &scoreObjectTime);

    instruments = 0;

    Get_bit_resource(rDB, "showMessages", &instruments, SHOW_MESSAGES);

    Get_bit_resource(rDB, "mapRadar", &instruments, MAP_RADAR);
    Get_bit_resource(rDB, "clientRanker", &instruments, CLIENT_RANKER);
    Get_bit_resource(rDB, "showShipShapes", &instruments, SHOW_SHIP_SHAPES);
    Get_bit_resource(rDB, "showMyShipShape", &instruments,
		     SHOW_MY_SHIP_SHAPE);
    Get_bit_resource(rDB, "ballMsgScan", &instruments, BALL_MSG_SCAN);
    Get_bit_resource(rDB, "showLivesByShip", &instruments,
		     SHOW_LIVES_BY_SHIP);
    Get_bit_resource(rDB, "slidingRadar", &instruments, SHOW_SLIDING_RADAR);
    Get_bit_resource(rDB, "showItems", &instruments, SHOW_ITEMS);
    Get_bit_resource(rDB, "clockAMPM", &instruments, SHOW_CLOCK_AMPM_FORMAT);
    Get_bit_resource(rDB, "outlineWorld", &instruments, SHOW_OUTLINE_WORLD);
    Get_bit_resource(rDB, "filledWorld", &instruments, SHOW_FILLED_WORLD);
    Get_bit_resource(rDB, "texturedWalls", &instruments, SHOW_TEXTURED_WALLS);
    Get_bit_resource(rDB, "showDecor", &instruments, SHOW_DECOR);
    Get_bit_resource(rDB, "outlineDecor", &instruments, SHOW_OUTLINE_DECOR);
    Get_bit_resource(rDB, "filledDecor", &instruments, SHOW_FILLED_DECOR);
    Get_bit_resource(rDB, "texturedDecor", &instruments, SHOW_TEXTURED_DECOR);
    Get_bit_resource(rDB, "reverseScroll", &instruments, SHOW_REVERSE_SCROLL);
    Get_bit_resource(rDB, "showID", &instruments, SHOW_SHIP_ID);

    Get_bool_resource(rDB, "pointerControl", &initialPointerControl);
    Get_float_resource(rDB, "showItemsTime", &showItemsTime);
    LIMIT(showItemsTime, MIN_SHOW_ITEMS_TIME, MAX_SHOW_ITEMS_TIME);

    Get_int_resource(rDB, "showScoreDecimals", &showScoreDecimals);
    LIMIT(showScoreDecimals, 0, 2);

    Get_float_resource(rDB, "speedFactHUD", &hud_move_fact);
    Get_float_resource(rDB, "speedFactPTR", &ptr_move_fact);
    Get_float_resource(rDB, "fuelNotify", &fuelLevel3);
    Get_float_resource(rDB, "fuelWarning", &fuelLevel2);
    Get_float_resource(rDB, "fuelCritical", &fuelLevel1);

    Get_int_resource(rDB, "maxMessages", &maxMessages);
    Get_int_resource(rDB, "messagesToStdout", &messagesToStdout);
#ifndef _WINDOWS
    Get_bool_resource(rDB, "selectionAndHistory", &selectionAndHistory);
    Get_int_resource(rDB, "maxLinesInHistory", &maxLinesInHistory);
    LIMIT(maxLinesInHistory, 1, MAX_HIST_MSGS);
#endif

    Get_int_resource(rDB, "receiveWindowSize", &receive_window_size);
    LIMIT(receive_window_size,
	  MIN_RECEIVE_WINDOW_SIZE, MAX_RECEIVE_WINDOW_SIZE);

    Get_resource(rDB, "clientRankFile", resValue, sizeof resValue);
    clientRankFile = xp_strdup(resValue);
    Get_resource(rDB, "clientRankHTMLFile", resValue, sizeof resValue);
    clientRankHTMLFile = xp_strdup(resValue);
    Get_resource(rDB, "clientRankHTMLNOJSFile", resValue, sizeof resValue);
    clientRankHTMLNOJSFile = xp_strdup(resValue);
    Get_resource(rDB, "texturePath", resValue, sizeof resValue);
    texturePath = xp_strdup(resValue);

    Get_int_resource(rDB, "maxFPS", &maxFPS);
    oldMaxFPS = maxFPS;

    IFWINDOWS( Get_int_resource(rDB, "radarDivisor", &RadarDivisor) );
    IFWINDOWS( Get_bool_resource(rDB, "threadedDraw", &ThreadedDraw) );

#ifdef SOUND
    Get_string_resource(rDB, "sounds", sounds, sizeof sounds);
    Get_int_resource(rDB, "maxVolume", &maxVolume);
    Get_resource(rDB, "audioServer", audioServer, sizeof audioServer);
#endif

    /*
     * Key bindings
     */
    num = 0;
    for (i = 0; i < NELEM(options); i++) {
	if ((key = options[i].key) == KEY_DUMMY)
	    continue;
	Get_resource(rDB, options[i].name, resValue, sizeof resValue);
	firstKeyDef = num;
	for (str = strtok(resValue, " \t\r\n");
	     str != NULL;
	     str = strtok(NULL, " \t\r\n")) {
	    if ((ks = Get_key_by_name(str)) == SDLK_UNKNOWN) {
		printf("Invalid keysym \"%s\" for key \"%s\".\n",
		       str, options[i].name);
		continue;
	    }
	    keyMap[ks] = key;
	}
    }

    /*
     * Pointer button bindings
     */
    for (i = 0; i < 5; i++) {
	sprintf(resValue, "pointerButton%d", i+1);
	Get_resource(rDB, resValue, resValue, sizeof resValue);
	ptr = resValue;
	if (*ptr != '\0') {
	    for (ptr = strtok(resValue, " \t\r\n");
		 ptr != NULL;
		 ptr = strtok(NULL, " \t\r\n")) {
		if (!strncasecmp(ptr, "key", 3))
		    ptr += 3;
		for (j = 0; j < NELEM(options); j++) {
		    if (options[j].key != KEY_DUMMY) {
			if (!strcasecmp(ptr, options[j].name + 3)) {
			    buttonMap[i] = options[j].key;
			    break;
			}
		    }
		}
		if (j == NELEM(options))
		    warn("Unknown key \"%s\" for pointer button %d", ptr, i);
	    }
	}
    }

#ifndef _WINDOWS
    XrmDestroyDatabase(rDB);

    free(xopt);
#endif

#ifdef SOUND
    audioInit(dispName);
#endif /* SOUND */
}

void Options_cleanup(void)
{
    IFWINDOWS( Get_xpilotini_file(-1) );

    if (texturePath) {
	free(texturePath);
	texturePath = NULL;
    }
    if (shipShape) {
	free(shipShape);
	shipShape = NULL;
    }

#ifdef SOUND
    audioCleanup();
#endif /* SOUND */
}