/* -*- indent-tabs-mode: nil; tab-width: 4; c-basic-offset: 4; -*-

   framerender.c for the Openbox window manager
   Copyright (c) 2006        Mikael Magnusson
   Copyright (c) 2003-2007   Dana Jansens

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   See the COPYING file for a copy of the GNU General Public License.
*/

#include "frame.h"
#include "openbox.h"
#include "screen.h"
#include "client.h"
#include "framerender.h"
#include "obrender/theme.h"
#include <Imlib2.h>

static void framerender_label(ObFrame *self, RrAppearance *a);
static void framerender_icon(ObFrame *self, RrAppearance *a);
static void framerender_max(ObFrame *self, RrAppearance *a);

/* macOS-style stoplight color circles for the title buttons.
   Draw a filled circle centered in the button appearance (green=max,
   yellow=iconify, red=close) via imlib2 so the window controls look like a
   Mac stoplight instead of openbox's single-color theme glyphs.
   Color is 0xAARRGGBB. */
#define STOP_RED    0xffff6052
#define STOP_YELLOW 0xffffbd45
#define STOP_GREEN  0xff29c86d

/* Draw a filled macOS stoplight disc directly onto the button's X pixmap via
   imlib2. RrPaint renders the theme glyph into a->pixmap; drawing a colored
   disc over it on that SAME pixmap is what openbox composites to the frame and
   to the screen (the twitter-verified root cause: writing to the title's CPU
   pixel_data never reaches an X pixmap because SOLID title surfaces skip
   pixel_data_to_pixmap). */
static void stoplight_paint_disc(RrAppearance *a, guint32 color) {
    static int dbg = 0;
    if (!a || a->pixmap == None) {
        if (!dbg) { FILE *f = fopen("/tmp/stoplight-debug.log", "w"); if (f) { fprintf(f, "paint SKIPPED a=%p pixmap=%lu\n", (void*)a, (unsigned long)(a?a->pixmap:None)); fclose(f); } dbg = 1; }
        return;
    }
    gint cx = a->w / 2, cy = a->h / 2;
    gint r = a->w / 2 - 1;
    if (r < 1) return;
    Display *dpy = RrDisplay(a->inst);

    if (!dbg) { FILE *f = fopen("/tmp/stoplight-debug.log", "w"); if (f) { fprintf(f, "paint a=%p w=%d h=%d pixmap=%lu color=%08x cx=%d cy=%d r=%d\n", (void*)a, a->w, a->h, (unsigned long)a->pixmap, color, cx, cy, r); fclose(f); } dbg = 1; }

    imlib_context_set_display(dpy);
    imlib_context_set_visual(RrVisual(a->inst));
    imlib_context_set_colormap(RrColormap(a->inst));

    Imlib_Image img = imlib_create_image(a->w, a->h);
    if (!img) return;
    imlib_context_set_image(img);
    imlib_image_clear();
    imlib_context_set_color((color>>16)&0xff, (color>>8)&0xff, color&0xff, (color>>24)&0xff);
    imlib_image_fill_ellipse(cx, cy, r, r);

    imlib_context_set_drawable(a->pixmap);
    imlib_context_set_image(img);
    imlib_context_set_blend(1);
    imlib_render_image_on_drawable(cx - r, cy - r);

    imlib_free_image();
    imlib_context_set_drawable(None);
}
static void framerender_desk(ObFrame *self, RrAppearance *a);
static void framerender_iconify(ObFrame *self, RrAppearance *a);
static void framerender_shade(ObFrame *self, RrAppearance *a);
static void framerender_close(ObFrame *self, RrAppearance *a);


void framerender_frame(ObFrame *self)
{
    if (frame_iconify_animating(self))
        return; /* delay redrawing until the animation is done */
    if (!self->need_render)
        return;
    if (!self->visible)
        return;
    self->need_render = FALSE;

    {
        gulong px;

        px = (self->focused ?
              RrColorPixel(ob_rr_theme->cb_focused_color) :
              RrColorPixel(ob_rr_theme->cb_unfocused_color));

        XSetWindowBackground(obt_display, self->backback, px);
        XClearWindow(obt_display, self->backback);
        XSetWindowBackground(obt_display, self->innerleft, px);
        XClearWindow(obt_display, self->innerleft);
        XSetWindowBackground(obt_display, self->innertop, px);
        XClearWindow(obt_display, self->innertop);
        XSetWindowBackground(obt_display, self->innerright, px);
        XClearWindow(obt_display, self->innerright);
        XSetWindowBackground(obt_display, self->innerbottom, px);
        XClearWindow(obt_display, self->innerbottom);
        XSetWindowBackground(obt_display, self->innerbll, px);
        XClearWindow(obt_display, self->innerbll);
        XSetWindowBackground(obt_display, self->innerbrr, px);
        XClearWindow(obt_display, self->innerbrr);
        XSetWindowBackground(obt_display, self->innerblb, px);
        XClearWindow(obt_display, self->innerblb);
        XSetWindowBackground(obt_display, self->innerbrb, px);
        XClearWindow(obt_display, self->innerbrb);

        px = RrColorPixel(self->focused ?
            (self->client->undecorated ?
             ob_rr_theme->frame_undecorated_focused_border_color :
             ob_rr_theme->frame_focused_border_color) :
            (self->client->undecorated ?
             ob_rr_theme->frame_undecorated_unfocused_border_color :
             ob_rr_theme->frame_unfocused_border_color));

        XSetWindowBackground(obt_display, self->left, px);
        XClearWindow(obt_display, self->left);
        XSetWindowBackground(obt_display, self->right, px);
        XClearWindow(obt_display, self->right);

        XSetWindowBackground(obt_display, self->titleleft, px);
        XClearWindow(obt_display, self->titleleft);
        XSetWindowBackground(obt_display, self->titletop, px);
        XClearWindow(obt_display, self->titletop);
        XSetWindowBackground(obt_display, self->titletopleft, px);
        XClearWindow(obt_display, self->titletopleft);
        XSetWindowBackground(obt_display, self->titletopright, px);
        XClearWindow(obt_display, self->titletopright);
        XSetWindowBackground(obt_display, self->titleright, px);
        XClearWindow(obt_display, self->titleright);

        XSetWindowBackground(obt_display, self->handleleft, px);
        XClearWindow(obt_display, self->handleleft);
        XSetWindowBackground(obt_display, self->handletop, px);
        XClearWindow(obt_display, self->handletop);
        XSetWindowBackground(obt_display, self->handleright, px);
        XClearWindow(obt_display, self->handleright);
        XSetWindowBackground(obt_display, self->handlebottom, px);
        XClearWindow(obt_display, self->handlebottom);

        XSetWindowBackground(obt_display, self->lgripleft, px);
        XClearWindow(obt_display, self->lgripleft);
        XSetWindowBackground(obt_display, self->lgriptop, px);
        XClearWindow(obt_display, self->lgriptop);
        XSetWindowBackground(obt_display, self->lgripbottom, px);
        XClearWindow(obt_display, self->lgripbottom);

        XSetWindowBackground(obt_display, self->rgripright, px);
        XClearWindow(obt_display, self->rgripright);
        XSetWindowBackground(obt_display, self->rgriptop, px);
        XClearWindow(obt_display, self->rgriptop);
        XSetWindowBackground(obt_display, self->rgripbottom, px);
        XClearWindow(obt_display, self->rgripbottom);

        /* don't use the separator color for shaded windows */
        if (!self->client->shaded)
            px = (self->focused ?
                  RrColorPixel(ob_rr_theme->title_separator_focused_color) :
                  RrColorPixel(ob_rr_theme->title_separator_unfocused_color));

        XSetWindowBackground(obt_display, self->titlebottom, px);
        XClearWindow(obt_display, self->titlebottom);
    }

    if (self->decorations & OB_FRAME_DECOR_TITLEBAR) {
        RrAppearance *t, *l, *m, *n, *i, *d, *s, *c, *clear;
        if (self->focused) {
            t = ob_rr_theme->a_focused_title;
            l = ob_rr_theme->a_focused_label;
            m = (!(self->decorations & OB_FRAME_DECOR_MAXIMIZE) ?
                 ob_rr_theme->btn_max->a_focused_disabled :
                 (self->client->max_vert || self->client->max_horz ?
                  (self->max_press ?
                   ob_rr_theme->btn_max->a_focused_pressed_toggled :
                   (self->max_hover ?
                    ob_rr_theme->btn_max->a_focused_hover_toggled :
                    ob_rr_theme->btn_max->a_focused_unpressed_toggled)) :
                  (self->max_press ?
                   ob_rr_theme->btn_max->a_focused_pressed :
                   (self->max_hover ?
                    ob_rr_theme->btn_max->a_focused_hover :
                    ob_rr_theme->btn_max->a_focused_unpressed))));
            n = ob_rr_theme->a_icon;
            i = (!(self->decorations & OB_FRAME_DECOR_ICONIFY) ?
                 ob_rr_theme->btn_iconify->a_focused_disabled :
                 (self->iconify_press ?
                  ob_rr_theme->btn_iconify->a_focused_pressed :
                  (self->iconify_hover ?
                   ob_rr_theme->btn_iconify->a_focused_hover :
                   ob_rr_theme->btn_iconify->a_focused_unpressed)));
            d = (!(self->decorations & OB_FRAME_DECOR_ALLDESKTOPS) ?
                 ob_rr_theme->btn_desk->a_focused_disabled :
                 (self->client->desktop == DESKTOP_ALL ?
                  (self->desk_press ?
                   ob_rr_theme->btn_desk->a_focused_pressed_toggled :
                   (self->desk_hover ?
                    ob_rr_theme->btn_desk->a_focused_hover_toggled :
                    ob_rr_theme->btn_desk->a_focused_unpressed_toggled)) :
                  (self->desk_press ?
                   ob_rr_theme->btn_desk->a_focused_pressed :
                   (self->desk_hover ?
                    ob_rr_theme->btn_desk->a_focused_hover :
                    ob_rr_theme->btn_desk->a_focused_unpressed))));
            s = (!(self->decorations & OB_FRAME_DECOR_SHADE) ?
                 ob_rr_theme->btn_shade->a_focused_disabled :
                 (self->client->shaded ?
                  (self->shade_press ?
                   ob_rr_theme->btn_shade->a_focused_pressed_toggled :
                   (self->shade_hover ?
                    ob_rr_theme->btn_shade->a_focused_hover_toggled :
                    ob_rr_theme->btn_shade->a_focused_unpressed_toggled)) :
                  (self->shade_press ?
                   ob_rr_theme->btn_shade->a_focused_pressed :
                   (self->shade_hover ?
                    ob_rr_theme->btn_shade->a_focused_hover :
                    ob_rr_theme->btn_shade->a_focused_unpressed))));
            c = (!(self->decorations & OB_FRAME_DECOR_CLOSE) ?
                 ob_rr_theme->btn_close->a_focused_disabled :
                 (self->close_press ?
                  ob_rr_theme->btn_close->a_focused_pressed :
                  (self->close_hover ?
                   ob_rr_theme->btn_close->a_focused_hover :
                   ob_rr_theme->btn_close->a_focused_unpressed)));
        } else {
            t = ob_rr_theme->a_unfocused_title;
            l = ob_rr_theme->a_unfocused_label;
            m = (!(self->decorations & OB_FRAME_DECOR_MAXIMIZE) ?
                 ob_rr_theme->btn_max->a_unfocused_disabled :
                 (self->client->max_vert || self->client->max_horz ?
                  (self->max_press ?
                   ob_rr_theme->btn_max->a_unfocused_pressed_toggled :
                   (self->max_hover ?
                    ob_rr_theme->btn_max->a_unfocused_hover_toggled :
                    ob_rr_theme->btn_max->a_unfocused_unpressed_toggled)) :
                  (self->max_press ?
                   ob_rr_theme->btn_max->a_unfocused_pressed :
                   (self->max_hover ?
                    ob_rr_theme->btn_max->a_unfocused_hover :
                    ob_rr_theme->btn_max->a_unfocused_unpressed))));
            n = ob_rr_theme->a_icon;
            i = (!(self->decorations & OB_FRAME_DECOR_ICONIFY) ?
                 ob_rr_theme->btn_iconify->a_unfocused_disabled :
                 (self->iconify_press ?
                  ob_rr_theme->btn_iconify->a_unfocused_pressed :
                  (self->iconify_hover ?
                   ob_rr_theme->btn_iconify->a_unfocused_hover :
                   ob_rr_theme->btn_iconify->a_unfocused_unpressed)));
            d = (!(self->decorations & OB_FRAME_DECOR_ALLDESKTOPS) ?
                 ob_rr_theme->btn_desk->a_unfocused_disabled :
                 (self->client->desktop == DESKTOP_ALL ?
                  (self->desk_press ?
                   ob_rr_theme->btn_desk->a_unfocused_pressed_toggled :
                   (self->desk_hover ?
                    ob_rr_theme->btn_desk->a_unfocused_hover_toggled :
                    ob_rr_theme->btn_desk->a_unfocused_unpressed_toggled)) :
                  (self->desk_press ?
                   ob_rr_theme->btn_desk->a_unfocused_pressed :
                   (self->desk_hover ?
                    ob_rr_theme->btn_desk->a_unfocused_hover :
                    ob_rr_theme->btn_desk->a_unfocused_unpressed))));
            s = (!(self->decorations & OB_FRAME_DECOR_SHADE) ?
                 ob_rr_theme->btn_shade->a_unfocused_disabled :
                 (self->client->shaded ?
                  (self->shade_press ?
                   ob_rr_theme->btn_shade->a_unfocused_pressed_toggled :
                   (self->shade_hover ?
                    ob_rr_theme->btn_shade->a_unfocused_hover_toggled :
                    ob_rr_theme->btn_shade->a_unfocused_unpressed_toggled)) :
                  (self->shade_press ?
                   ob_rr_theme->btn_shade->a_unfocused_pressed :
                   (self->shade_hover ?
                    ob_rr_theme->btn_shade->a_unfocused_hover :
                    ob_rr_theme->btn_shade->a_unfocused_unpressed))));
            c = (!(self->decorations & OB_FRAME_DECOR_CLOSE) ?
                 ob_rr_theme->btn_close->a_unfocused_disabled :
                 (self->close_press ?
                  ob_rr_theme->btn_close->a_unfocused_pressed :
                  (self->close_hover ?
                   ob_rr_theme->btn_close->a_unfocused_hover :
                   ob_rr_theme->btn_close->a_unfocused_unpressed)));
        }
        clear = ob_rr_theme->a_clear;

        RrPaint(t, self->title, self->width, ob_rr_theme->title_height);

        clear->surface.parent = t;
        clear->surface.parenty = 0;

        clear->surface.parentx = ob_rr_theme->grip_width;

        RrPaint(clear, self->topresize,
                self->width - ob_rr_theme->grip_width * 2,
                ob_rr_theme->paddingy + 1);

        clear->surface.parentx = 0;

        if (ob_rr_theme->grip_width > 0)
            RrPaint(clear, self->tltresize,
                    ob_rr_theme->grip_width, ob_rr_theme->paddingy + 1);
        if (ob_rr_theme->title_height > 0)
            RrPaint(clear, self->tllresize,
                    ob_rr_theme->paddingx + 1, ob_rr_theme->title_height);

        clear->surface.parentx = self->width - ob_rr_theme->grip_width;

        if (ob_rr_theme->grip_width > 0)
            RrPaint(clear, self->trtresize,
                    ob_rr_theme->grip_width, ob_rr_theme->paddingy + 1);

        clear->surface.parentx = self->width - (ob_rr_theme->paddingx + 1);

        if (ob_rr_theme->title_height > 0)
            RrPaint(clear, self->trrresize,
                    ob_rr_theme->paddingx + 1, ob_rr_theme->title_height);

        /* set parents for any parent relative guys */
        l->surface.parent = t;
        l->surface.parentx = self->label_x;
        l->surface.parenty = ob_rr_theme->paddingy;

        m->surface.parent = t;
        m->surface.parentx = self->max_x;
        m->surface.parenty = ob_rr_theme->paddingy + 1;

        n->surface.parent = t;
        n->surface.parentx = self->icon_x;
        n->surface.parenty = ob_rr_theme->paddingy;

        i->surface.parent = t;
        i->surface.parentx = self->iconify_x;
        i->surface.parenty = ob_rr_theme->paddingy + 1;

        d->surface.parent = t;
        d->surface.parentx = self->desk_x;
        d->surface.parenty = ob_rr_theme->paddingy + 1;

        s->surface.parent = t;
        s->surface.parentx = self->shade_x;
        s->surface.parenty = ob_rr_theme->paddingy + 1;

        c->surface.parent = t;
        c->surface.parentx = self->close_x;
        c->surface.parenty = ob_rr_theme->paddingy + 1;

        framerender_label(self, l);
        framerender_max(self, m);
        framerender_icon(self, n);
        framerender_iconify(self, i);
        framerender_desk(self, d);
        framerender_shade(self, s);
        framerender_close(self, c);
    }

    if (self->decorations & OB_FRAME_DECOR_HANDLE &&
        ob_rr_theme->handle_height > 0)
    {
        RrAppearance *h, *g;

        h = (self->focused ?
             ob_rr_theme->a_focused_handle : ob_rr_theme->a_unfocused_handle);

        RrPaint(h, self->handle, self->width, ob_rr_theme->handle_height);

        if (self->decorations & OB_FRAME_DECOR_GRIPS) {
            g = (self->focused ?
                 ob_rr_theme->a_focused_grip : ob_rr_theme->a_unfocused_grip);

            if (g->surface.grad == RR_SURFACE_PARENTREL)
                g->surface.parent = h;

            g->surface.parentx = 0;
            g->surface.parenty = 0;

            RrPaint(g, self->lgrip,
                    ob_rr_theme->grip_width, ob_rr_theme->handle_height);

            g->surface.parentx = self->width - ob_rr_theme->grip_width;
            g->surface.parenty = 0;

            RrPaint(g, self->rgrip,
                    ob_rr_theme->grip_width, ob_rr_theme->handle_height);
        }
    }

    XFlush(obt_display);
}

static void framerender_label(ObFrame *self, RrAppearance *a)
{
    if (!self->label_on) return;
    /* set the texture's text! */
    a->texture[0].data.text.string = self->client->title;
    RrPaint(a, self->label, self->label_width, ob_rr_theme->label_height);
}

static void framerender_icon(ObFrame *self, RrAppearance *a)
{
    RrImage *icon;

    if (!self->icon_on) return;

    icon = client_icon(self->client);

    if (icon) {
        RrAppearanceClearTextures(a);
        a->texture[0].type = RR_TEXTURE_IMAGE;
        a->texture[0].data.image.alpha = 0xff;
        a->texture[0].data.image.image = icon;
    } else {
        RrAppearanceClearTextures(a);
        a->texture[0].type = RR_TEXTURE_NONE;
    }

    RrPaint(a, self->icon,
            ob_rr_theme->button_size + 2, ob_rr_theme->button_size + 2);
}

static void framerender_max(ObFrame *self, RrAppearance *a)
{
    if (!self->max_on) return;
    RrPaint(a, self->max, ob_rr_theme->button_size, ob_rr_theme->button_size);
    stoplight_paint_disc(a, STOP_GREEN);
}

static void framerender_iconify(ObFrame *self, RrAppearance *a)
{
    if (!self->iconify_on) return;
    RrPaint(a, self->iconify,
            ob_rr_theme->button_size, ob_rr_theme->button_size);
    stoplight_paint_disc(a, STOP_YELLOW);
}

static void framerender_desk(ObFrame *self, RrAppearance *a)
{
    if (!self->desk_on) return;
    RrPaint(a, self->desk, ob_rr_theme->button_size, ob_rr_theme->button_size);
}

static void framerender_shade(ObFrame *self, RrAppearance *a)
{
    if (!self->shade_on) return;
    RrPaint(a, self->shade,
            ob_rr_theme->button_size, ob_rr_theme->button_size);
}

static void framerender_close(ObFrame *self, RrAppearance *a)
{
    if (!self->close_on) return;
    RrPaint(a, self->close,
            ob_rr_theme->button_size, ob_rr_theme->button_size);
    stoplight_paint_disc(a, STOP_RED);
}
