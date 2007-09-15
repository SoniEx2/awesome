/* 
 * tile.c - tile layout
 *
 * Copyright © 2007 Julien Danjou <julien@danjou.info> 
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
 * You should have received a copy of the GNU General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. 
 * 
 */

#include <stdio.h>

#include "screen.h"
#include "awesome.h"
#include "tag.h"
#include "layout.h"
#include "layouts/tile.h"

/* extern */
extern Client *sel, *clients;

/* static */

static double mwfact = -1;
static int nmaster = -1;
static int ncols = -1;

static void
init_static_var_layout(awesome_config *awesomeconf)
{
    if(mwfact == -1)
        mwfact = awesomeconf->mwfact;
    if(nmaster == -1)
        nmaster = awesomeconf->nmaster;
    if(ncols == -1)
        ncols = awesomeconf->ncols;
}

void
uicb_setnmaster(Display *disp,
                DC * drawcontext,
                awesome_config *awesomeconf,
                const char * arg)
{
    if(!IS_ARRANGE(tile) && !IS_ARRANGE(tileleft))
        return;

    if(!arg)
        nmaster = awesomeconf->nmaster;
    else if((nmaster = (int) compute_new_value_from_arg(arg, (double) nmaster)) < 0)
            nmaster = 0;

    if(sel)
        arrange(disp, drawcontext, awesomeconf);
    else
        drawstatusbar(disp, drawcontext, awesomeconf);
}

void
uicb_setncols(Display *disp,
              DC * drawcontext,
              awesome_config *awesomeconf,
              const char * arg)
{
    if(!IS_ARRANGE(tile) && !IS_ARRANGE(tileleft))
        return;

    if(!arg)
        ncols = awesomeconf->ncols;
    else if((ncols = (int) compute_new_value_from_arg(arg, (double) ncols)) < 1)
            ncols = 1;

    if(sel)
        arrange(disp, drawcontext, awesomeconf);
    else
        drawstatusbar(disp, drawcontext, awesomeconf);
}

void
uicb_setmwfact(Display *disp,
                DC *drawcontext,
               awesome_config * awesomeconf,
               const char *arg)
{
    if(!IS_ARRANGE(tile) && !IS_ARRANGE(tileleft))
        return;

    if(!arg)
        mwfact = awesomeconf->mwfact;
    else
    {
        if((mwfact = compute_new_value_from_arg(arg, mwfact)) < 0.1)
            mwfact = 0.1;
        else if(mwfact > 0.9)
            mwfact = 0.9;
    }
    arrange(disp, drawcontext, awesomeconf);
}

static void
_tile(Display *disp, awesome_config *awesomeconf, const Bool right)
{
    /* windows area geometry */
    int wah = 0, waw = 0, wax = 0, way = 0;
    /* new coordinates */
    unsigned int nx, ny, nw, nh;
    /* master size */
    unsigned int mw = 0, mh = 0;
    int n, i, li, last_i = 0, nmaster_screen = 0, otherwin_screen = 0;
    int screen_numbers = 1, use_screen = -1;
    int real_ncols = 1, win_by_col = 1, current_col = 0;
    ScreenInfo *screens_info = NULL;
    Client *c;

    init_static_var_layout(awesomeconf);
    screens_info = get_screen_info(disp, awesomeconf->statusbar, &screen_numbers);
 
    for(n = 0, c = clients; c; c = c->next)
        if(IS_TILED(c, awesomeconf->selected_tags, awesomeconf->ntags))
            n++;

    for(i = 0, c = clients; c; c = c->next)
    {
        if(!IS_TILED(c, awesomeconf->selected_tags, awesomeconf->ntags))
            continue;

        if(use_screen == -1 || (screen_numbers > 1 && i && ((i - last_i) >= nmaster_screen + otherwin_screen || n == screen_numbers)))
        {
            use_screen++;
            last_i = i;

            wah = screens_info[use_screen].height;
            waw = screens_info[use_screen].width;
            wax = screens_info[use_screen].x_org;
            way = screens_info[use_screen].y_org;

            if(n >= nmaster * screen_numbers)
            {
                nmaster_screen = nmaster;
                otherwin_screen = (n - (nmaster * screen_numbers)) / screen_numbers;
                if(use_screen == 0)
                    otherwin_screen += (n - (nmaster * screen_numbers)) % screen_numbers;
            }
            else
            {
                nmaster_screen = n / screen_numbers;
                /* first screen takes more master */
                if(use_screen == 0)
                    nmaster_screen += n % screen_numbers;
                otherwin_screen = 0;
            }

            if(nmaster)
            {
                mh = nmaster_screen ? wah / nmaster_screen : waw;
                mw = otherwin_screen ? waw * mwfact : waw;
            }
            else
                mh = mw = 0;

            if(otherwin_screen < ncols)
                real_ncols = otherwin_screen;
            else
                real_ncols = ncols;
        }

        c->ismax = False;
        li = last_i ? i - last_i : i;
        if(li < nmaster)
        {                       /* master */
            ny = way + li * mh;
            if(right)
                nx = wax;
            else
                nx = wax + (waw - mw);
            nw = mw - 2 * c->border;
            nh = mh - 2 * c->border;
        }
        else
        {                       /* tile window */
            win_by_col = otherwin_screen / real_ncols;

            if((li - nmaster) && (li - nmaster) % win_by_col == 0 && current_col < real_ncols - 1)
                current_col++;

            if(current_col == real_ncols - 1)
                win_by_col += otherwin_screen % real_ncols;

            if(otherwin_screen <= real_ncols)
                nh = wah - 2 * c->border;
            else
                nh = (wah / win_by_col) - 2 * c->border;

            nw = (waw - mw) / real_ncols - 2 * c->border;

            if(li == nmaster || otherwin_screen <= real_ncols || (li - nmaster) % win_by_col == 0)
                ny = way;
            else
                ny = way + ((li - nmaster) % win_by_col) * (nh + 2 * c->border);

            nx = wax + current_col * nw + (right ? mw : 0);
        }
        resize(c, nx, ny, nw, nh, awesomeconf->resize_hints);
        i++;
    }
    XFree(screens_info);
}

void
tile(Display *disp, awesome_config *awesomeconf)
{
    _tile(disp, awesomeconf, True);
}

void
tileleft(Display *disp, awesome_config *awesomeconf)
{
    _tile(disp, awesomeconf, False);
}
