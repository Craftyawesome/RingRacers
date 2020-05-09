// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2007-2016 by John "JTE" Muniz.
// Copyright (C) 2011-2018 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_grandprix.c
/// \brief Grand Prix mode specific code

#include "k_grandprix.h"
#include "doomdef.h"
#include "d_player.h"
#include "g_game.h"
#include "k_bot.h"
#include "k_kart.h"
#include "m_random.h"
#include "r_things.h"

UINT8 grandprixmatch = 0;
boolean initgpbots = false;

void K_InitGrandPrixBots(void)
{
	const UINT8 defaultbotskin = 9; // eggrobo

	// startingdifficulty: Easy = 3, Normal = 6, Hard = 9
	const UINT8 startingdifficulty = min(MAXBOTDIFFICULTY, (gamespeed + 1) * 3);
	UINT8 difficultylevels[MAXPLAYERS];

	UINT8 playercount = 8;
	UINT8 wantedbots = 0;

	UINT8 numplayers = 0;
	UINT8 competitors[4];

	boolean skinusable[MAXSKINS];
	UINT8 botskinlist[MAXPLAYERS];
	UINT8 botskinlistpos = 0;

	UINT8 newplayernum = 0;
	UINT8 i;

	if (initgpbots != true)
	{
		return;
	}

	memset(difficultylevels, MAXBOTDIFFICULTY, sizeof (difficultylevels));
	memset(competitors, MAXPLAYERS, sizeof (competitors));
	memset(botskinlist, defaultbotskin, sizeof (botskinlist));

	// init usable bot skins list
	for (i = 0; i < MAXSKINS; i++)
	{
		if (i < numskins)
		{
			skinusable[i] = true;
		}
		else
		{
			skinusable[i] = false;
		}
	}

#if MAXPLAYERS != 16
	I_Error("GP bot difficulty levels need rebalacned for the new player count!\n");
#endif

	// init difficulty levels list
	//if (!mastermodebots) { // leave as all level 9
	difficultylevels[0] = max(1, startingdifficulty);
	difficultylevels[1] = max(1, startingdifficulty-1);
	difficultylevels[2] = max(1, startingdifficulty-2);
	difficultylevels[3] = max(1, startingdifficulty-3);
	difficultylevels[4] = max(1, startingdifficulty-3);
	difficultylevels[5] = max(1, startingdifficulty-4);
	difficultylevels[6] = max(1, startingdifficulty-4);
	difficultylevels[7] = max(1, startingdifficulty-4);
	difficultylevels[8] = max(1, startingdifficulty-5);
	difficultylevels[9] = max(1, startingdifficulty-5);
	difficultylevels[10] = max(1, startingdifficulty-5);
	difficultylevels[11] = max(1, startingdifficulty-6);
	difficultylevels[12] = max(1, startingdifficulty-6);
	difficultylevels[13] = max(1, startingdifficulty-6);
	difficultylevels[14] = max(1, startingdifficulty-7);
	difficultylevels[15] = max(1, startingdifficulty-7);

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i])
		{
			if (numplayers < MAXSPLITSCREENPLAYERS && !players[i].spectator)
			{
				competitors[numplayers] = i;
				numplayers++;
			}
			else
			{
				players[i].spectator = true; // force spectate for all other players, if they happen to exist?
			}
		}
	}

	if (numplayers > 2)
	{
		// Add 3 bots per player beyond 2P
		playercount += (numplayers-2) * 3;
	}

	wantedbots = playercount - numplayers;

	// Create rival list

	// TODO: Use player skin's set rivals
	// Starting with P1's rival1, P2's rival1, P3's rival1, P4's rival1,
	// then P1's rival2, P2's rival2, etc etc etc etc.......
	// then skip over any duplicates.

	// Pad the remaining list with random skins if we need to
	if (botskinlistpos < wantedbots)
	{
		for (i = botskinlistpos; i < wantedbots; i++)
		{
			UINT8 val = M_RandomKey(numskins);
			UINT8 loops = 0;

			while (!skinusable[val])
			{
				if (loops >= numskins)
				{
					// no more skins
					break;
				}

				val++;

				if (val >= numskins)
				{
					val = 0;
				}

				loops++;
			}

			if (loops >= numskins)
			{
				// leave the rest of the table as the default skin
				break;
			}

			botskinlist[i] = val;
			skinusable[val] = false;
		}
	}

	for (i = 0; i < wantedbots; i++)
	{
		if (!K_AddBot(botskinlist[i], difficultylevels[i], &newplayernum))
		{
			break;
		}
	}

	initgpbots = false;
}
