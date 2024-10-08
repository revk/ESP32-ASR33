/*
 * Initialisation
 *
 * Copyright (c) 1977, 2005 by Will Crowther and Don Woods
 * Copyright (c) 2017 by Eric S. Raymond
 * SPDX-License-Identifier: BSD-2-clause
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#include "advent.h"

struct settings_t settings;

struct game_t game;

int
initialise (void)
{
   srand (time (NULL));
   int seedval = (int) rand ();
   set_seed (seedval);

   memset (&settings, 0, sizeof (settings));
   settings.logfp = NULL;
   settings.oldstyle = false;
   settings.prompt = true;

   memset (&game, 0, sizeof (game));
   game.dloc[1] = LOC_KINGHALL;
   game.dloc[2] = LOC_WESTBANK;
   game.dloc[3] = LOC_Y2;
   game.dloc[4] = LOC_ALIKE3;
   game.dloc[5] = LOC_COMPLEX;

   /*  Sixth dwarf is special (the pirate).  He always starts at his
    *  chest's eventual location inside the maze. This loc is saved
    *  in chloc for ref. The dead end in the other maze has its
    *  loc stored in chloc2. */
   game.dloc[6] = LOC_DEADEND12;
   game.chloc = LOC_DEADEND12;
   game.chloc2 = LOC_DEADEND13;
   game.abbnum = 5;
   game.clock1 = WARNTIME;
   game.clock2 = FLASHTIME;
   game.newloc = LOC_START;
   game.loc = LOC_START;
   game.limit = GAMELIMIT;
   game.foobar = WORD_EMPTY;

   for (int i = 1; i <= NOBJECTS; i++)
   {
      game.place[i] = LOC_NOWHERE;
   }

   for (int i = 1; i <= NLOCATIONS; i++)
   {
      if (!(locations[i].description.big == 0 || tkey[i] == 0))
      {
         int k = tkey[i];
         if (T_TERMINATE (travel[k]))
            conditions[i] |= (1 << COND_FORCED);
      }
   }

   /*  Set up the game.atloc and game.link arrays.
    *  We'll use the DROP subroutine, which prefaces new objects on the
    *  lists.  Since we want things in the other order, we'll run the
    *  loop backwards.  If the object is in two locs, we drop it twice.
    *  Also, since two-placed objects are typically best described
    *  last, we'll drop them first. */
   for (int i = NOBJECTS; i >= 1; i--)
   {
      if (objects[i].fixd > 0)
      {
         drop (i + NOBJECTS, objects[i].fixd);
         drop (i, objects[i].plac);
      }
   }

   for (int i = 1; i <= NOBJECTS; i++)
   {
      int k = NOBJECTS + 1 - i;
      game.fixed[k] = objects[k].fixd;
      if (objects[k].plac != 0 && objects[k].fixd <= 0)
         drop (k, objects[k].plac);
   }

   /*  Treasure props are initially -1, and are set to 0 the first time
    *  they are described.  game.tally keeps track of how many are
    *  not yet found, so we know when to close the cave. */
   for (int treasure = 1; treasure <= NOBJECTS; treasure++)
   {
      if (objects[treasure].is_treasure)
      {
         if (objects[treasure].inventory != 0)
            game.prop[treasure] = STATE_NOTFOUND;
         game.tally = game.tally - game.prop[treasure];
      }
   }
   game.conds = setbit (11);

   return seedval;
}
