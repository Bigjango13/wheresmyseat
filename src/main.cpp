#include <cstdio>
#include <cassert>
#include <string>

#include <SDL2/SDL.h>
#include <SDL_image.h>

#include "game.h"

//#define DEBUG_MODE
#define COLOR(asset, HEX) SDL_SetTextureColorMod(asset, HEX >> 16, (HEX >> 8) & 0xFF, (HEX) & 0xFF)
#define RENDER(asset, src, dst) \
    SDL_RenderCopyEx(game->sdlrenderer, asset, src, dst, 0, NULL, SDL_FLIP_NONE)
#define RENDER_G(asset, src, dst) \
    SDL_RenderCopyEx(sdlrenderer, asset, src, dst, 0, NULL, SDL_FLIP_NONE)
constexpr int wd16 = SCREEN_WIDTH / SCALE, hd16 = SCREEN_HEIGHT / SCALE;
#define IMPOSSIBLE \
    puts("ERROR: We've reached an unreachable state. Anything is possible. The limits were in our heads all along. Follow your dreams."); \
    exit(1);

#define MAX_LEVEL 20

#define BOTTOM_Y 551
#define NEXT_BUTTON_X 673
#define PREV_BUTTON_X 177
#define NEXT_BUTTON_POS {NEXT_BUTTON_X, BOTTOM_Y, 64, 32}
#define PREV_BUTTON_POS {PREV_BUTTON_X, BOTTOM_Y, 64, 32}

static inline constexpr bool inbounds(int x, int y) {
    return 0 <= x && x <= 7 && 0 <= y && y <= 8;
}

#define FONT_S 16
int colors[] = {0x000000, 0xFF0000, 0x09550C, 0x8E03FC, 0x0323FC};
void render_text(
    Game *game,
    int x, int y,
    const char *text,
    int color = 0, int s = 1
) {
    SDL_Rect dstRect = {.x = 0, .y = 0, .w = FONT_S * s, .h = FONT_S * s};
    SDL_Rect srcRect = {.x = 0, .y = 0, .w = FONT_S,     .h = FONT_S    };
    COLOR(Asset::font, color);
    int i, ox = x; unsigned char c;
    for (i = 0; (c = text[i]) != 0; i++) {
        if (c == '\n') {
            // Newline
            y += (FONT_S + 4) * s;
            x = ox;
            COLOR(Asset::font, color);
        } else if (c == 0xA0) {
            // Reset color
            COLOR(Asset::font, color);
        } else if (0xA1 <= c && c <= 0xA5) {
            // Set new color
            COLOR(Asset::font, colors[c - 0xA1]);
        } else if (0x80 <= c && c < 0x87) {
            // Render base glyph
            dstRect.x = x; dstRect.y = y;
            srcRect.x = FONT_S * (c & 15);
            srcRect.y = FONT_S * (c >> 4);
            RENDER(Asset::font, &srcRect, &dstRect);
            // Locational chars have special rendering
            Uint8 orr,og,ob;
            SDL_GetTextureColorMod(Asset::font, &orr, &og, &ob);
            COLOR(Asset::font, colors[4]);
            srcRect.x = FONT_S * 0;
            srcRect.y = FONT_S * 8;
            RENDER(Asset::font, &srcRect, &dstRect);
            SDL_SetTextureColorMod(Asset::font, orr, og, ob);
            // Next char
            x += (FONT_S-2)*s;
        } else {
            // Normal char
            dstRect.x = x; dstRect.y = y;
            srcRect.x = FONT_S * (c & 15);
            srcRect.y = FONT_S * (c >> 4);
            RENDER(Asset::font, &srcRect, &dstRect);
            // Next char
            x += (FONT_S-2)*s;
        }
    }
    COLOR(Asset::font, 0xFFFFFF);
}
void render_text_bubble(Game *game, std::string name, const char *text, int x, int y, int w, int h) {
    // Upper left
    SDL_Rect dstRect = {x - wd16/4, y - hd16/4, wd16/4, hd16/4};
    SDL_Rect srcRect = {0, 0, 4, 4};
    RENDER(Asset::text, &srcRect, &dstRect);
    // Upper right
    dstRect.x = x + w;
    srcRect.x = 8;
    RENDER(Asset::text, &srcRect, &dstRect);
    // Bottom right
    dstRect.y = y + h;
    srcRect.y = 8;
    RENDER(Asset::text, &srcRect, &dstRect);
    // Bottom left
    dstRect.x = x - wd16/4;
    srcRect.x = 0;
    RENDER(Asset::text, &srcRect, &dstRect);
    // Connect top
    dstRect = {x, y - hd16/4, w, hd16/4};
    srcRect.x = 4;
    srcRect.y = 0;
    RENDER(Asset::text, &srcRect, &dstRect);
    // Connect bottom
    dstRect.y = y + h;
    srcRect.y = 8;
    RENDER(Asset::text, &srcRect, &dstRect);
    // Connect left
    dstRect = {x - wd16/4, y, wd16/4, h};
    srcRect.x = 0;
    srcRect.y = 4;
    RENDER(Asset::text, &srcRect, &dstRect);
    // Connect right
    dstRect.x = x + w;
    srcRect.x = 8;
    RENDER(Asset::text, &srcRect, &dstRect);
    // Middle
    dstRect = {x, y, w, h};
    srcRect = {4, 4, 4, 4};
    RENDER(Asset::text, &srcRect, &dstRect);
    // At last, text!
    render_text(game, x + w - (name.size()-1) * (FONT_S-2), y, name.c_str(), 0, 1);
    render_text(game, x, y, text, 0, 1);
}

std::string getStrForAttr(UNUSED Game *game, UNUSED Viewer *viewer, Attributes attr) {
    switch (attr) {
        case Attributes::Stinky:
            return C_CBLUE "Stinka stonka\n";
        case Attributes::Tall:
            return C_CBLUE "I'm tall btw (6'7)\n";
        case Attributes::Singing:
            return C_CBLUE "I will be singing along\n";
        case Attributes::Talkative:
            return C_CBLUE "I can be chatty " C_SX "\n";
    }
    IMPOSSIBLE
}

std::string getStrForLim(UNUSED Game *game, Viewer *viewer, Limit lim, std::string color) {
    switch (lim.constraint) {
        // Aisle/row
        // I messed the text up first try :')
        case Constraints::Aisle:
            return color +
                "I want to be on " C_CPURPLE "Row " + std::string(1, 'A' + lim.data)
                + "\n";
        case Constraints::Row:
            return color +
                "I want to be in " C_CPURPLE "Column " + std::to_string(lim.data + 1)
                + "\n";

        // Stage l/r
        case Constraints::StageLeft:
            return color + "I want to be " C_CPURPLE "Stage Left\n";
        case Constraints::StageRight:
            return color + "I want to be " C_CPURPLE "Stage Right\n";

        // Wheelchair
        case Constraints::Wheelchair:
            return color + "I'm in a wheelchair\n";

        // Love/hate/date
        case Constraints::Date:
            return color +
                "I'm on a date with " C_CPURPLE
                + viewer->game->viewers[lim.data]->name
                + color + ":" C_SX "\n";
        case Constraints::Love:
            return color +
                "I want to sit with " C_CPURPLE
                + viewer->game->viewers[lim.data]->name
                + color + ":" C_SX "\n";
        case Constraints::Prox:
            return color +
                "I want to sit near " C_CPURPLE
                + viewer->game->viewers[lim.data]->name
                + color + ":" C_SALL "\n";
        case Constraints::Hate:
            return color
                + "Don't put me near " C_CPURPLE
                + viewer->game->viewers[lim.data]->name
                + color + ":" C_SALL "\n";

        // Intro/lonely/extro
        case Constraints::Introverted:
            return color + "I want to be alone:" C_SALL "\n";
        case Constraints::Lonely:
            return color + "I could use a friend:" C_SALL "\n";
        case Constraints::Extroverted:
            return color + "I'm pretty extroverted:" C_SALL "\n";

        // Chess/Checkers
        case Constraints::ChessHater:
            // Unsure of the wording that should be used
            //return color + "I don't trust chess pieces:" C_SCHESS "\n";
            return color + "I'm scared of chess pieces:" C_SCHESS "\n";
        case Constraints::Chess:
            return color + "I'm a chess piece!" C_SCHESS "\n";
        case Constraints::ChessProt:
            return C_CPURPLE
                + viewer->game->viewers[lim.data]->name
                + color + " will protect me"
                + color + ":" C_SALL "\n";
        case Constraints::CheckersHater:
            return color + "I don't trust checkers pieces:" C_SCHECKERS "\n";
        case Constraints::Checkers:
            return color + "I'm a checkers piece!" C_SCHECKERS "\n";
        case Constraints::GameHater:
            return color + "I don't trust game pieces:" C_SCHESS C_SCHECKERS "\n";

        // Short
        case Constraints::Short:
            return color + "I'm short (5'3):" C_SDOWN "\n";

        // Nose
        case Constraints::GoodNose:
            return color + "I have a good nose:" C_SALL "\n";
        // Ears
        case Constraints::BadEars:
            return color + "I dislike loud noises:" C_SALL "\n";

        // Baby/parent
        case Constraints::Parent:
            if (lim.data == 1) {
                return color + "I have one child:" C_SSEE "\n";
            }
            return color + "I have "
                + std::to_string(lim.data) + " children:" C_SSEE "\n";

        case Constraints::Baby:
            return color
                + "My parent is " C_CPURPLE
                + viewer->game->viewers[lim.data]->name + "\n";
    }
    IMPOSSIBLE
}

// Checks
inline std::vector<Viewer*> _get_checks(Viewer *viewer, std::vector<std::pair<int,int>> pos) {
    std::vector<Viewer *> ret = {};
    for (std::pair<int, int> xy : pos) {
        // Don't cross aisles
        if (
            (viewer->x <= 3 && viewer->x + xy.first > 3)
            || (viewer->x >= 4 && viewer->x + xy.first < 4)
        ) continue;
        // Get the person
        int x = viewer->x + xy.first, y = viewer->y + xy.second;
        Viewer *next = viewer->game->getAt(x, y);
        if (next != NULL) ret.push_back(next);
    }
    return ret;
}
std::vector<Viewer*> get_checks(Viewer *viewer, Checks checks) {
    switch (checks) {
        case Checks::None:
            return {};
        case Checks::Xside:
            return _get_checks(viewer, {{-1, 0}, {1, 0}});
        case Checks::Yside:
            return _get_checks(viewer, {{0, -1}, {0, 1}});
        case Checks::XYside:
            return _get_checks(viewer, {{-1, 0}, {1, 0}, {0, -1}, {0, 1}});
        case Checks::All:
            return _get_checks(viewer, {
                {-1, -1}, {0, -1}, {1, -1},
                {-1,  0},          {1,  0},
                {-1,  1}, {0,  1}, {1, 1}
            });
        case Checks::See:
            return _get_checks(viewer, {
                {-1, 0},         {1, 0},
                {-1, 1}, {0, 1}, {1, 1}
            });
        case Checks::InvSee:
            return _get_checks(viewer, {
                {-1, -1}, {0, -1}, {1, -1},
                {-1,  0},          {1,  0}
            });
        case Checks::Down:
            return _get_checks(viewer, {{0, 1}});
        case Checks::Chess:
            assert(0);
            return {};
    }
    return {};
}

std::vector<Viewer*> check_for_constraints(Viewer *viewer, Checks checks, Constraints constraint) {
    std::vector<Viewer*> viewers = get_checks(viewer, checks), ret = {};
    for (Viewer *v : viewers) {
        for (Limit lim : v->limits) {
            if (lim.constraint == constraint) {
                ret.push_back(v);
                break;
            }
        }
    }
    return ret;
}
std::vector<Viewer*> check_for_limit(Viewer *viewer, Checks checks, Limit lim) {
    std::vector<Viewer*> viewers = get_checks(viewer, checks), ret = {};
    for (Viewer *v : viewers) {
        for (Limit vlim : v->limits) {
            if (vlim.constraint == lim.constraint && vlim.data == lim.data) {
                ret.push_back(v);
                break;
            }
        }
    }
    return ret;
}
std::vector<Viewer*> check_for_attr(Viewer *viewer, Checks checks, Attributes attr) {
    std::vector<Viewer*> viewers = get_checks(viewer, checks), ret = {};
    for (Viewer *v : viewers) {
        for (Attributes a : v->attrs) {
            if (a == attr) {
                ret.push_back(v);
                break;
            }
        }
    }
    return ret;
}
std::vector<Viewer*> check_for_person(Viewer *viewer, Checks checks, int id) {
    std::vector<Viewer*> viewers = get_checks(viewer, checks);
    for (Viewer *v : viewers) {
        if (v == viewer->game->viewers[id]) return {v};
    }
    return {};
}
#define I std::vector<std::pair<int, int>>
void add_chess_rook(Viewer *viewer, std::vector<Viewer*> &viewers, bool add_all = false) {
    // Rook/queen
    for (std::pair<int, int> offset : I{{0, 1}, {0, -1}, {1, 0}, {-1, 0}}) {
        int m = 0;
        while (true) {
            m += 1;
            int x = viewer->x + offset.first  * m,
                y = viewer->y + offset.second * m;
            if (!inbounds(x, y)) break;
            // Check
            Viewer *v = viewer->game->getAt(x, y);
            if (
                v != NULL
                && (((v->skin == Skins::ChessRookSkin
                ||  v->skin == Skins::ChessQueenSkin)
                && viewer->color != v->color)
                || add_all)
            ) {
                viewers.push_back(v);
            }
            if (v != NULL) break;
        }
    }
}
void add_chess_bishop(Viewer *viewer, std::vector<Viewer*> &viewers, bool add_all = false) {
    // Bishop/queen
    for (std::pair<int, int> offset : I{{1, 1}, {1, -1}, {-1, 1}, {-1, -1}}) {
        int m = 0;
        while (true) {
            m += 1;
            int x = viewer->x + offset.first  * m,
                y = viewer->y + offset.second * m;
            // +-1 penalty for crossing aisle
            if ((viewer->x <= 3 && 4 <= x) || (x <= 3 && 4 <= viewer->x)) {
                y += offset.second;
            }
            if (!inbounds(x, y)) break;
            // Check
            Viewer *v = viewer->game->getAt(x, y);
            if (
                v != NULL
                && (((v->skin == Skins::ChessBishopSkin
                ||  v->skin == Skins::ChessQueenSkin)
                && viewer->color != v->color)
                || add_all)
            ) {
                viewers.push_back(v);
            }
            if (v != NULL) break;
        }
    }
}
#undef I
void add_chess_knight(Viewer *viewer, std::vector<Viewer*> &viewers, bool add_all = false) {
    // Knight
    for (Viewer *v : _get_checks(viewer, {
         {1,  2}, {-1,  2},
       {2,  1},     {-2,  1},
       {2, -1},     {-2, -1},
         {1, -2}, {-1, -2},
    })) {
        if ((v->skin == Skins::ChessKnightSkin && viewer->color != v->color) || add_all) {
            viewers.push_back(v);
        }
    }
}
void add_chess_queen(Viewer *viewer, std::vector<Viewer*> &viewers, bool add_all = false) {
    add_chess_rook(viewer, viewers, add_all);
    add_chess_bishop(viewer, viewers, add_all);
}

bool Limit::check(Viewer *viewer, std::vector<Viewer*> &viewers) {
    viewers = {};
    switch (constraint) {
        // Aisle/row
        case Constraints::Aisle:
            return viewer->y == 8 - data;
        case Constraints::Row:
            return viewer->x == data;

        // Stage l/r
        case Constraints::StageLeft:
            return viewer->x <= 3;
        case Constraints::StageRight:
            return viewer->x > 3;

        // Wheelchair
        case Constraints::Wheelchair:
            return viewer->x == 3 || viewer->x == 4;

        // Love/hate/date
        case Constraints::Date:
        case Constraints::Love:
            return check_for_person(viewer, Checks::Xside, data).size() != 0;
        case Constraints::Prox:
            return check_for_person(viewer, Checks::All, data).size() != 0;
        case Constraints::Hate:
            viewers = check_for_person(viewer, Checks::All, data);
            return viewers.size() == 0;

        // Intro/lonely/extro
        case Constraints::Introverted:
            viewers = get_checks(viewer, Checks::All);
            return viewers.size() < 1;
        case Constraints::Lonely:
            viewers = get_checks(viewer, Checks::All);
            if (viewers.size() == 1) {
                viewers = {};
                return true;
            }
            return false;
        case Constraints::Extroverted:
            return get_checks(viewer, Checks::All).size() > 1;

        // Baby
        case Constraints::Baby:
            if (check_for_person(viewer, Checks::InvSee, data).size() == 0) {
                return false;
            }
            [[fallthrough]];
        // Short
        case Constraints::Short:
            viewers = check_for_attr(viewer, Checks::Down, Attributes::Tall);
            return viewers.size() == 0;

        // Parent
        case Constraints::Parent: {
            return (int) check_for_limit(
                viewer,
                Checks::See,
                Limit{Constraints::Baby, viewer->id}
            ).size() == data;
        }

        // Nose
        case Constraints::GoodNose:
            viewers = check_for_attr(viewer, Checks::All, Attributes::Stinky);
            return viewers.size() == 0;

        // Ears
        case Constraints::BadEars: {
            // Singers are always loud
            viewers = check_for_attr(viewer, Checks::All, Attributes::Singing);
            // Talkers are only loud when next to talkers
            std::vector<Viewer*> talkers = get_checks(viewer, Checks::All);
            for (Viewer *v : talkers) {
                for (Attributes attr : v->attrs) {
                    if (attr == Attributes::Talkative) {
                        std::vector<Viewer*> ntalk = check_for_attr(v, Checks::Xside, Attributes::Talkative);
                        // Talking to others
                        if (ntalk.size() > 0) {
                            viewers.push_back(v);
                            viewers.insert(viewers.end(), ntalk.begin(), ntalk.end());
                        }
                    }
                }
            }
            // Silence!
            return viewers.size() == 0;
        }

        // Chess protect
        case Constraints::ChessProt: {
            Viewer *protector = viewer->game->viewers[data];
            if (protector->skin == Skins::ChessRookSkin) add_chess_rook(protector, viewers, true);
            if (protector->skin == Skins::ChessKnightSkin) add_chess_knight(protector, viewers, true);
            if (protector->skin == Skins::ChessBishopSkin) add_chess_bishop(protector, viewers, true);
            if (protector->skin == Skins::ChessQueenSkin) add_chess_queen(protector, viewers, true);
            for (Viewer *v : viewers) {
                if (v->id == viewer->id) {
                    viewers = {};
                    return true;
                }
            }
            viewers = {};
            return false;
        }

        // Chess/checkers
        case Constraints::ChessHater:
        case Constraints::GameHater:
        case Constraints::Chess: {
            add_chess_queen(viewer, viewers);
            add_chess_knight(viewer, viewers);
            if (constraint != Constraints::GameHater)
                return viewers.size() == 0;
            [[fallthrough]];
        }
        case Constraints::Checkers:
        case Constraints::CheckersHater:
            // TODO: Checkers
            IMPOSSIBLE
    }
    return false;
}

// Viewer
bool mouse_touching(SDL_Rect dstRect) {
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    return dstRect.x <= mx && mx <= dstRect.x + dstRect.w
        && dstRect.y <= my && my <= dstRect.y + dstRect.h;
}

constexpr inline int getChairX(int x) {return x * 62 + 159 + 94 * (x >= 4); }
constexpr inline int getChairY(int y) {return y * 46 + 3; }
constexpr inline int getViewerX(int x, bool bleachers) {
    if (bleachers) return (abs(x)-1)*36 + 10;
    return getChairX(x) + 8*2;
}
constexpr inline int getViewerY(int y, bool bleachers) {
    if (bleachers) return (abs(y)-1)*36 + 12;
    return getChairY(y) + 6*2;
}
Viewer::Viewer(Game *game, int color, std::string name) : game(game), color(color), name(name) {
    id = game->viewers.size();
    level = game->level_id;
    resetPos();
    if (game->saved_pos[level].size() == id) {
        game->saved_pos[level].push_back(-1);
    } else {
        loadPos(game->saved_pos[level][id]);
    }
}
bool Viewer::render() {
    int angle = 0;
    if (game->held == this) {
        timer += 10;
        timer &= 0xFF;
        constexpr int MAXANGLE = 20;
        if (timer < 0x7F) {
            angle = MAXANGLE - ((MAXANGLE*2)*timer)/0x7F;
        } else {
            angle = ((MAXANGLE*2)*(timer - 0x7F))/0x7F - MAXANGLE;
        }
    } else {
        timer = 0x7F;
    }
    int gx, gy;
    if (game->held == this) {
        int mx, my;
        SDL_GetMouseState(&mx, &my);
        gx = mx - 16;
        gy = my - 16;
    } else {
        gx = getViewerX(x, x < 0 || y < 0);
        gy = getViewerY(y, x < 0 || y < 0);
    }
    SDL_Rect dstRect = {gx, gy, 32, 32};
    SDL_Rect srcRect = {0, skin * 16, 16, 16};
    // Uncolored base layer
    COLOR(Asset::duck, 0xFFFFFF);
    SDL_RendererFlip flip = (happy && game->held != this && x >= 0 && y >= 0)
        ? SDL_FLIP_HORIZONTAL
        : SDL_FLIP_NONE;
    if (isChess()) {
        flip = (happy || x < 0 || y < 0)
            ? SDL_FLIP_NONE
            : SDL_FLIP_VERTICAL;
    }
    SDL_RenderCopyEx(
        game->sdlrenderer, Asset::duck, &srcRect, &dstRect, angle, NULL, flip
    );
    // Colored overlay
    COLOR(Asset::duck, color);
    srcRect.x += srcRect.w;
    SDL_RenderCopyEx(
        game->sdlrenderer, Asset::duck, &srcRect, &dstRect, angle, NULL, flip
    );

    COLOR(Asset::duck, 0xFFFFFF);
    bool ret = mouse_touching(dstRect);

    // Special limits effecting rendering
    for (Limit lim : limits) {
        if (lim.constraint == Constraints::Wheelchair) {
            if (y >= 0 && (x == 3 || x == 4)) {
                dstRect = {getChairX(x), getChairY(y), 64, 64};
                if (x == 3) dstRect.x += 52;
                if (x == 4) dstRect.x -= 52;
                RENDER(Asset::wheelchair, NULL, &dstRect);
            }
        }
    }

    return ret;
}
std::string Viewer::getStatusColor(Limit lim, std::vector<Viewer*> &viewers) {
    if (x < 0 || y < 0 || game->held == this) {
        return C_CBLACK;
    } else if (lim.check(this, viewers)) {
        return C_CGREEN;
    } else {
        return C_CRED;
    }
}
bool Viewer::check_valid(std::vector<Viewer*> &viewers) {
    if (x < 0 || y < 0) {
        happy = false;
        return happy;
    }
    for (Limit lim : limits) {
        if (!lim.check(this, viewers)) {
            happy = false;
            return happy;
        }
    }
    happy = true;
    return happy;
};

// Game
static int chaircolor[8 * 9] = {0};
void update_chairs_shades(Game *game, std::vector<Viewer*> &redviewers) {
    for (int i = 0; i < 8*9; i++) chaircolor[i] = 0;
    if (game->touching == NULL) return;
    if (game->touching->x < 0 || game->touching->y < 0) return;
    Viewer *v = game->touching;
#define I std::vector<std::pair<int, int>>
    // Rook/queen
    if (v->skin == Skins::ChessRookSkin || v->skin == Skins::ChessQueenSkin) {
        for (std::pair<int, int> offset : I{{0, 1}, {0, -1}, {1, 0}, {-1, 0}}) {
            int m = 0;
            while (true) {
                m += 1;
                int x = v->x + offset.first  * m,
                    y = v->y + offset.second * m;
                if (!inbounds(x, y)) break;
                chaircolor[y * 8 + x] = 1;
                if (game->getAt(x, y) != NULL) break;
            }
        }
        chaircolor[v->y * 8 + v->x] = 1;
    }
    // Bishop/queen
    if (v->skin == Skins::ChessBishopSkin || v->skin == Skins::ChessQueenSkin) {
        for (std::pair<int, int> offset : I{{1, 1}, {1, -1}, {-1, 1}, {-1, -1}}) {
            int m = 0;
            while (true) {
                m += 1;
                int x = v->x + offset.first  * m,
                    y = v->y + offset.second * m;
                // +-1 penalty for crossing aisle
                if ((v->x <= 3 && 4 <= x) || (x <= 3 && 4 <= v->x)) {
                    y += offset.second;
                }
                if (!inbounds(x, y)) break;
                chaircolor[y * 8 + x] = 1;
                if (game->getAt(x, y) != NULL) break;
            }
        }
        chaircolor[v->y * 8 + v->x] = 1;
    }
    // Knight
    if (v->skin == Skins::ChessKnightSkin) {
        for (std::pair<int, int> offset :  I{
            {1,2},{-1,2},{2,1},{-2,1},{2,-1},{-2,-1},{1,-2},{-1,-2}
        }) {
            int x = v->x + offset.first,
                y = v->y + offset.second;
            if ((v->x <= 3 && x > 3) || (v->x >= 4 && x < 4)) continue;
            if (inbounds(x, y)) chaircolor[y * 8 + x] = 1;
        }
    }
#undef I
    // Red chairs
    for (Viewer *v : redviewers) {
        chaircolor[v->y * 8 + v->x] = 2;
    }
}

void draw_chair(Game *game, int id) {
    int x = getChairX(id % 8), y = getChairY(id / 8);
    int color = chaircolor[id];
    if (game->chairEnabled(id)) {
        COLOR(Asset::chair, 0xFFFFFF);
        if (color == 1) COLOR(Asset::chair, 0x47FF47);
        if (color == 2) COLOR(Asset::chair, 0xFF0000);
    } else {
        COLOR(Asset::chair, 0x474747);
        if (color == 1) COLOR(Asset::chair, 0x47AA47);
        if (color == 2) COLOR(Asset::chair, 0xAA4747);
    }
    SDL_Rect dstRect = {x, y, 64, 64};
    RENDER(Asset::chair, NULL, &dstRect);
}

int getChairId(int x, int y) {
    constexpr int CS = 64;
    if (y > (8 * 46 + 3) + 64 || y < 5) return -1;
    y -= 5;
    y /= 46;
    if (159 <= x && x <= 3 * 62 + 159 + CS) {
        x -= 159;
        x /= 62;
    } else if (4 * 62 + 159 + 94 <= x && x <= 7 * 62 + 159 + 94 + CS) {
        x -= 4 * 62 + 159 + 94;
        x /= 62;
        x += 4;
    } else {
        return -1;
    }
    x = std::min(x, 7);
    y = std::min(y, 8);

    return x+y*8;
}

void Game::tick() {
    bool needs_update = false;
    // Exit events
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            running = false;
        }
    }
    // Mouse events
    int mx, my;
    int mouse = SDL_GetMouseState(&mx, &my);
    static int oms = 0;
    if ((mouse & SDL_BUTTON(1)) && !(oms & SDL_BUTTON(1))) {
        if (level_id > 0 && mouse_touching(PREV_BUTTON_POS)) {
            // Prev button
            level_id--;
            setupLevel();
        } else if (
            (level_complete || level_id < max_level_id)
            && mouse_touching(NEXT_BUTTON_POS)
            && level_id < MAX_LEVEL - 1
        ) {
            // Next button
            level_id++;
            max_level_id = std::max(max_level_id, level_id);
            setupLevel();
        } else if (held == NULL) {
            // Pick up
            held = touching;
            if (held != NULL) held->resetPos();
        } else if (touching != NULL) {
            // Swap
            Viewer *t = touching;
            touching = held;
            held = t;
            if (held->x >= 0 && held->y >= 0) {
                touching->x = held->x;
                touching->y = held->y;
            }
            held->resetPos();
        } else {
            // Set down
            int cid = getChairId(mx, my);
            if (cid == -1) {
                held->resetPos();
                held = NULL;
            } else if (chairEnabled(cid) && getAt(cid % 8, cid / 8) == NULL) {
                int nx = cid % 8, ny = cid / 8;
                Viewer *v = getAt(nx, ny);
                if (v == NULL || v == held) {
                    held->x = nx;
                    held->y = ny;
                    held = NULL;
                }
            }
        }
        needs_update = true;
    }
    oms = mouse;

    if (needs_update) {
        level_complete = true;
        for (Viewer *viewer : viewers) {
            std::vector<Viewer*> _v = {};
            level_complete = viewer->check_valid(_v) && level_complete;
        }
    }
}

bool Game::chairEnabled(int id) {
    for (std::pair<int, int> closed_section : closed_sections) {
        if (closed_section.first <= id && id <= closed_section.second) {
            return false;
        }
    }
    return true;
}

void Game::render() {
    // BG
    SDL_SetRenderDrawColor(sdlrenderer, 0x29, 0x0F, 0x02, 255);
    SDL_RenderClear(sdlrenderer);
    SDL_Rect dstRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    RENDER_G(Asset::bg, NULL, &dstRect);

    // Chairs
    for (int cid = 0; cid < 8*9; cid++) {
        draw_chair(this, cid);
    }

    // Info button
    bool info = false;
    if (getInfo() != NULL) {
        dstRect = {439, 6, 32, 32};
        RENDER_G(Asset::info, NULL, &dstRect);
        info = mouse_touching(dstRect);
    }

    // Next button
    if (level_id < MAX_LEVEL - 1) {
        dstRect = NEXT_BUTTON_POS;
        if (!level_complete && level_id == max_level_id) COLOR(Asset::next, 0x474747);
        else COLOR(Asset::next, 0xFFFFFF);
        RENDER_G(Asset::next, NULL, &dstRect);
    }

    // Level Number
    std::string level_name = "Level " + std::to_string(level_id + 1);
    int color = 0xFFFFFF;
    if (level_complete) color = 0x47FF47;
    #ifdef DEBUG_MODE
    else if (level_id < max_level_id) color = 0xFF47FF;
    #else
    else if (level_id < max_level_id) color = 0xFF4747;
    #endif
    render_text(
        this,
        (NEXT_BUTTON_X + PREV_BUTTON_X)/2 - (level_name.size() * (FONT_S-2)) + 32,
        BOTTOM_Y - 4,
        level_name.c_str(),
        color,
        2
    );

    // Prev button
    dstRect = PREV_BUTTON_POS;
    if (level_id <= 0) COLOR(Asset::next, 0x474747);
    else COLOR(Asset::next, 0xFFFFFF);
    SDL_RenderCopyEx(sdlrenderer, Asset::next, NULL, &dstRect, 0, NULL, SDL_FLIP_HORIZONTAL);

    // Text
    std::vector<Viewer *> redent = {};
    if (touching == NULL) touching = held;
    if (touching != NULL) {
        std::string text = "";
        for (Attributes attr : touching->attrs) {
            text += getStrForAttr(this, touching, attr);
        }
        for (Limit lim : touching->limits) {
            std::vector<Viewer *> nredent = {};
            text += getStrForLim(
                this, touching, lim, touching->getStatusColor(lim, nredent)
            );
            redent.insert(redent.end(), nredent.begin(), nredent.end());
        }
        if (text.size() == 0) text = "I'm not picky!";
        render_text_bubble(this,
            C_CPURPLE + touching->name,
            text.c_str(),
            114 + (SCREEN_WIDTH-114)/2 - 512/2,
            SCREEN_HEIGHT - 112,
            512, 96
        );
        // Update invalid entities
        if (touching == held) redent = {};
    } else if (info) {
        // Check info box
        render_text_bubble(this, "",
            getInfo(),
            114 + (SCREEN_WIDTH-114)/2 - 512/2,
            SCREEN_HEIGHT - 112,
            512, 96
        );
    }
    if (held == touching) touching = NULL;

    // Draw entities
    touching = NULL;
    for (Viewer *viewer : viewers) {
        // Doing game logic in rendering loop, sue me
        if (viewer != held && viewer->render()) touching = viewer;
    }
    update_chairs_shades(this, redent);
    if (held != NULL) held->render();

    // Present
    SDL_RenderPresent(sdlrenderer);
}

Viewer *Game::getAt(int x, int y) {
    if (!inbounds(x, y)) return NULL;
    for (Viewer *v : viewers) {
        if (v->x == x && v->y == y) return v;
    }
    return NULL;
}

const char *Game::getInfo() {
    if (level_id == 0) {
        return "The " C_CGREEN "goal\xA0 is to seat all " C_CPURPLE "viewers\n"
               C_CPURPLE "Ducks\xA0 facing "
               C_CGREEN "left\xA0 are " C_CGREEN "happy";
    } else if (level_id == 1) {
        return "Some " C_CPURPLE"ducks" C_CGREEN " like\xA0 other " C_CPURPLE "ducks\n"
               "Some " C_CPURPLE"ducks" C_CRED " dislike\xA0 other " C_CPURPLE "ducks\n\n"
               "The dots show what " C_CPURPLE "seats\xA0 are checked\n"
               "IE:" C_CPURPLE C_SALL "\xA0 is all surrounding seats";
    } else if (level_id == 2) {
        return "Sometimes " C_CPURPLE "ducks\xA0 have bad hygiene\n"
               "This can be really gross\n\n"
               "Hovering over ducks highlights seats that need to move\n";
    } else if (level_id == 3) {
        return "Not all " C_CPURPLE "ducks\xA0 like company\n"
               "\"Wants to be alone\"= "  C_CGREEN"0 people\n"
               "\"Could use a friend\"= " C_CGREEN"1 person\n"
               "\"Pretty extroverted\"= " C_CGREEN "2 or more\n";
    } else if (level_id == 4) {
        return "Some " C_CPURPLE "ducks\xA0 are picky about seating\n"
               C_CPURPLE "Columns\xA0 are labled " C_CPURPLE "1234 5678\n"
               C_CPURPLE "Rows\xA0 are labled from front to back\n";
    } else if (level_id == 5) {
        return C_CPURPLE "Tall ducks\xA0 block " C_CPURPLE "short ducks\xA0 view\n"
               C_CPURPLE "Wheelchair users\xA0 must be on an aisle";
    } else if (level_id == 6) {
        return C_CPURPLE "Parent ducks\xA0 need to see their "
               C_CPURPLE "babies\xA0\n"
               C_CPURPLE "Baby ducks\xA0 are short\n"
               C_CPURPLE "Singing ducks\xA0 are always loud\n"
               C_CPURPLE "Chatty ducks\xA0 are loud when together\n";
    } else if (level_id == 7) {
        return C_CPURPLE "Chess pieces\xA0 can also enjoy movies\n"
               "Unhappy " C_CPURPLE "chess pieces\xA0 are inverted\n"
               "Hovering over them shows their range";
    } else if (level_id == 8) {
        return C_CPURPLE "Chess pieces\xA0 can enjoy company too:\n"
               "-By sitting side by side\n"
               "-By sitting near each other\n"
               "-By protecting each other\n";
    } else if (level_id == 9) {
        return "Some " C_CPURPLE "ducks\xA0 distrust " C_CPURPLE "chess pieces\n"
               "Some " C_CPURPLE "viewers\xA0 prefer a side\n";
    } else if (level_id == 10) {
        return "Some " C_CPURPLE "baby ducks\xA0 come without a "
               C_CPURPLE "parent\n"
               "They do come with friends though!";
    } else if (level_id == MAX_LEVEL - 1) {
        // Final level
        return "Good luck!";
    }
    return NULL;
}

#define DC(c) (0xE0C812 & (c))
#define CHESSW 0xEEEEEE
#define CHESSB 0x676767
void Game::setupLevel() {
    // Reset level
    touching = NULL;
    held = NULL;
    closed_sections = {};
    for (Viewer *v : viewers) {
        saved_pos[v->level][v->id] = v->getSavePos();
        delete v;
    }
    viewers = {};

    // Init new level
    if (saved_pos.size() == (size_t) level_id) saved_pos.push_back({});
    if (level_id == 0) {
        // Level 1
        viewers.push_back(new Viewer(this, 0xDB0067, "Julia"));

        closed_sections.push_back({0, 66});
        closed_sections.push_back({68, 71});
    } else if (level_id == 1) {
        // Level 2
        viewers.push_back(new Viewer(this, 0xDB0067, "Julia"));
        viewers[0]->limits.push_back(Limit{Constraints::Love, 1});
        viewers.push_back(new Viewer(this, 0x9000CE, "Max"));
        viewers[1]->limits.push_back(Limit{Constraints::Love, 0});
        viewers[1]->limits.push_back(Limit{Constraints::Hate, 2});
        viewers.push_back(new Viewer(this, 0x6CAEED, "Buttercup"));
        viewers[2]->limits.push_back(Limit{Constraints::Hate, 1});

        closed_sections.push_back({0, 64});
        closed_sections.push_back({68, 71});
    } else if (level_id == 2) {
        // Level 3
        viewers.push_back(new Viewer(this, DC(0x00FF00), "Isabel"));
        viewers[0]->limits.push_back(Limit{Constraints::GoodNose});

        viewers.push_back(new Viewer(this, DC(0x4747FF), "Davenport"));
        viewers[1]->attrs.push_back(Attributes::Stinky);

        closed_sections.push_back({0, 64});
        closed_sections.push_back({68, 71});
    } else if (level_id == 3) {
        // Level 4
        viewers.push_back(new Viewer(this, DC(0xFFFFFF), "Kate"));
        viewers[0]->limits.push_back(Limit{Constraints::Extroverted});

        viewers.push_back(new Viewer(this, DC(0xFF0000), "Murgatroyd"));
        viewers[1]->limits.push_back(Limit{Constraints::Hate, 0});
        viewers[1]->limits.push_back(Limit{Constraints::Lonely});

        viewers.push_back(new Viewer(this, DC(0x00FF00), "Isabel"));
        viewers[2]->limits.push_back(Limit{Constraints::GoodNose});
        viewers[2]->limits.push_back(Limit{Constraints::Lonely});

        viewers.push_back(new Viewer(this, DC(0x4747FF), "Davenport"));
        viewers[3]->attrs.push_back(Attributes::Stinky);

        viewers.push_back(new Viewer(this, DC(0xFF4700), "Alice"));
        viewers[4]->limits.push_back(Limit{Constraints::Introverted});

        closed_sections.push_back({0, 63});
        closed_sections.push_back({69, 71});
    } else if (level_id == 4) {
        // Level 5
        viewers.push_back(new Viewer(this, 0xE62025, "Samuel"));
        viewers[0]->limits.push_back(Limit{Constraints::Aisle, 0});
        viewers[0]->limits.push_back(Limit{Constraints::Date, 1});

        viewers.push_back(new Viewer(this, 0x64A649, "Richard"));
        viewers[1]->limits.push_back(Limit{Constraints::Row, 0});
        viewers[1]->limits.push_back(Limit{Constraints::Date, 0});

        closed_sections.push_back({0, 55});
        closed_sections.push_back({60, 63});
        closed_sections.push_back({68, 71});
    } else if (level_id == 5) {
        // Level 6
        viewers.push_back(new Viewer(this, 0xE62025, "Samuel"));
        viewers[0]->setSkin(Skins::TallSkin);
        viewers[0]->limits.push_back(Limit{Constraints::Date, 1});
        viewers[0]->limits.push_back(Limit{Constraints::Aisle, 0});

        viewers.push_back(new Viewer(this, 0x64A649, "Richard"));
        viewers[1]->setSkin(Skins::TallSkin);
        viewers[1]->limits.push_back(Limit{Constraints::Date, 0});
        viewers[1]->attrs.push_back(Attributes::Stinky);

        viewers.push_back(new Viewer(this, 0x6CAEED, "Buttercup"));
        viewers[2]->setSkin(Skins::ShortSkin);
        viewers[2]->limits.push_back(Limit{Constraints::GoodNose});

        viewers.push_back(new Viewer(this, 0xDE5833, "Hebe"));
        viewers[3]->setSkin(Skins::ShortSkin);
        viewers[3]->limits.push_back(Limit{Constraints::Love, 4});
        viewers[3]->limits.push_back(Limit{Constraints::Wheelchair});

        viewers.push_back(new Viewer(this, 0xDE5833, "Joseph"));
        viewers[4]->limits.push_back(Limit{Constraints::Love, 3});

        closed_sections.push_back({0, 56});
        closed_sections.push_back({60, 64});
        closed_sections.push_back({68, 71});
    } else if (level_id == 6) {
        // Level 7
        viewers.push_back(new Viewer(this, 0xFFFFFF, "MajGen.Stanley"));
        viewers[0]->limits.push_back(Limit{Constraints::Parent, 4});

        viewers.push_back(new Viewer(this, 0xFF0067, "Mabel"));
        viewers[1]->setSkin(Skins::BabySkin, 0);
        viewers[1]->attrs.push_back(Attributes::Singing);
        viewers[1]->limits.push_back(Limit{Constraints::Love, 5});

        viewers.push_back(new Viewer(this, DC(0xFFFFFF), "Kate"));
        viewers[2]->setSkin(Skins::BabySkin, 0);
        viewers[2]->attrs.push_back(Attributes::Talkative);
        viewers[2]->limits.push_back(Limit{Constraints::GoodNose, 1});

        viewers.push_back(new Viewer(this, DC(0x00FF00), "Isabel"));
        viewers[3]->setSkin(Skins::BabySkin, 0);
        viewers[3]->limits.push_back(Limit{Constraints::BadEars, 1});

        viewers.push_back(new Viewer(this, 0xABDAD1, "Edith"));
        viewers[4]->setSkin(Skins::BabySkin, 0);
        viewers[4]->attrs.push_back(Attributes::Talkative);
        viewers[4]->limits.push_back(Limit{Constraints::Hate, 1});

        viewers.push_back(new Viewer(this, 0xAB3C3C, "Frederic"));
        viewers[5]->attrs.push_back(Attributes::Stinky);
        viewers[5]->limits.push_back(Limit{Constraints::Love, 1});

        viewers.push_back(new Viewer(this, 0xE62025, "Samuel"));
        viewers[6]->setSkin(Skins::TallSkin);
        viewers[6]->limits.push_back(Limit{Constraints::Row, 2});

        closed_sections.push_back({0, 55});
        closed_sections.push_back({60, 63});
        closed_sections.push_back({68, 71});
    } else if (level_id == 7) {
        // Level 8
        viewers.push_back(new Viewer(this, 0x9000CE, "Queen (P)"));
        viewers[0]->setSkin(Skins::ChessQueenSkin);
        viewers[0]->attrs.push_back(Attributes::Singing);
        viewers[0]->limits.push_back(Limit{Constraints::Wheelchair});
        viewers.push_back(new Viewer(this, 0xFFFFFF, "Rook (W)"));
        viewers[1]->setSkin(Skins::ChessRookSkin);
        viewers.push_back(new Viewer(this, 0xFFFFFF, "Knight (W)"));
        viewers[2]->setSkin(Skins::ChessKnightSkin);
        viewers[2]->attrs.push_back(Attributes::Tall);
        viewers.push_back(new Viewer(this, 0xE62022, "Bishop (R)"));
        viewers[3]->setSkin(Skins::ChessBishopSkin);
        viewers[3]->limits.push_back(Limit{Constraints::Short});
        viewers[3]->limits.push_back(Limit{Constraints::BadEars});
        viewers.push_back(new Viewer(this, 0xE62022, "Rook (R)"));
        viewers[4]->setSkin(Skins::ChessRookSkin);

        closed_sections.push_back({0, 47});
        closed_sections.push_back({52, 55});
        closed_sections.push_back({60, 63});
        closed_sections.push_back({68, 71});
    } else if (level_id == 8) {
        // Level 9
        viewers.push_back(new Viewer(this, 0x9000CE, "Knight (P)"));
        viewers[0]->setSkin(Skins::ChessKnightSkin);
        viewers[0]->attrs.push_back(Attributes::Tall);
        viewers.push_back(new Viewer(this, 0xE62022, "Knight (R)"));
        viewers[1]->setSkin(Skins::ChessKnightSkin);
        viewers[1]->attrs.push_back(Attributes::Tall);
        viewers[1]->limits.push_back(Limit{Constraints::Love, 4});
        viewers.push_back(new Viewer(this, 0xE62022, "Rook (R)"));
        viewers[2]->setSkin(Skins::ChessRookSkin);
        viewers[2]->limits.push_back(Limit{Constraints::Prox, 5});
        viewers.push_back(new Viewer(this, 0xFFFFFF, "Bishop (W)"));
        viewers[3]->setSkin(Skins::ChessBishopSkin);
        viewers[3]->limits.push_back(Limit{Constraints::Short});
        viewers[3]->limits.push_back(Limit{Constraints::Row, 1});
        viewers.push_back(new Viewer(this, 0xFFFFFF, "Knight (W)"));
        viewers[4]->setSkin(Skins::ChessKnightSkin);
        viewers[4]->attrs.push_back(Attributes::Tall);
        viewers[4]->limits.push_back(Limit{Constraints::Love, 1});
        viewers.push_back(new Viewer(this, 0xFFFFFF, "Rook (W)"));
        viewers[5]->setSkin(Skins::ChessRookSkin);
        viewers[5]->limits.push_back(Limit{Constraints::Prox, 2});
        viewers[5]->limits.push_back(Limit{Constraints::ChessProt, 4});

        closed_sections.push_back({0, 55});
        closed_sections.push_back({60, 63});
        closed_sections.push_back({68, 71});
    } else if (level_id == 9) {
        // Level 10
        viewers.push_back(new Viewer(this, 0x9000CE, "Queen (P)"));
        viewers[0]->setSkin(Skins::ChessQueenSkin);
        viewers[0]->attrs.push_back(Attributes::Singing);
        viewers[0]->limits.push_back(Limit{Constraints::Wheelchair});
        viewers.push_back(new Viewer(this, 0x9000CE, "Bishop (P)"));
        viewers[1]->setSkin(Skins::ChessBishopSkin);
        viewers[1]->limits.push_back(Limit{Constraints::BadEars});
        viewers.push_back(new Viewer(this, 0x9000CE, "Knight (P)"));
        viewers[2]->setSkin(Skins::ChessKnightSkin);
        viewers[2]->limits.push_back(Limit{Constraints::GoodNose});
        viewers[2]->limits.push_back(Limit{Constraints::Love, 4});
        viewers.push_back(new Viewer(this, 0xFFFFFF, "Rook (W)"));
        viewers[3]->setSkin(Skins::ChessRookSkin);
        viewers.push_back(new Viewer(this, 0xFFFFFF, "Knight (W)"));
        viewers[4]->setSkin(Skins::ChessKnightSkin);
        viewers[4]->limits.push_back(Limit{Constraints::GoodNose});
        viewers[4]->limits.push_back(Limit{Constraints::Love, 2});
        viewers.push_back(new Viewer(this, 0xE62022, "Rook (R)"));
        viewers[5]->setSkin(Skins::ChessRookSkin);
        viewers.push_back(new Viewer(this, 0xE62022, "Bishop (R)"));
        viewers[6]->setSkin(Skins::ChessBishopSkin);
        viewers[6]->limits.push_back(Limit{Constraints::BadEars});
        viewers[6]->limits.push_back(Limit{Constraints::StageLeft});
        viewers.push_back(new Viewer(this, 0x64A649, "Richard"));
        viewers[7]->setSkin(Skins::TallSkin);
        viewers[7]->limits.push_back(Limit{Constraints::ChessHater});
        viewers[7]->attrs.push_back(Attributes::Stinky);
        closed_sections.push_back({0, 63});
    } else if (level_id == 10) {
        // Level 11
        viewers.push_back(new Viewer(this, 0x9000CE, "Bishop (P)"));
        viewers[0]->setSkin(Skins::ChessBishopSkin);
        viewers.push_back(new Viewer(this, 0x9000CE, "Knight (P)"));
        viewers[1]->setSkin(Skins::ChessKnightSkin);
        viewers[1]->limits.push_back(Limit{Constraints::GoodNose});
        viewers.push_back(new Viewer(this, 0x9000CE, "Knight 2 (P)"));
        viewers[2]->setSkin(Skins::ChessKnightSkin);
        viewers[2]->limits.push_back(Limit{Constraints::GoodNose});
        viewers.push_back(new Viewer(this, 0xFFFFFF, "Knight (R)"));
        viewers[3]->setSkin(Skins::ChessKnightSkin);

        viewers.push_back(new Viewer(this, 0xE62025, "Samuel"));
        viewers[4]->setSkin(Skins::TallSkin);
        viewers[4]->limits.push_back(Limit{Constraints::Date, 5});
        viewers[4]->limits.push_back(Limit{Constraints::Aisle, 0});
        viewers[4]->limits.push_back(Limit{Constraints::ChessHater});
        viewers.push_back(new Viewer(this, 0x64A649, "Richard"));
        viewers[5]->setSkin(Skins::TallSkin);
        viewers[5]->attrs.push_back(Attributes::Stinky);
        viewers[5]->limits.push_back(Limit{Constraints::Date, 4});
        viewers[5]->limits.push_back(Limit{Constraints::ChessHater});

        viewers.push_back(new Viewer(this, 0xE679A6, "Rosemary"));
        viewers[6]->limits.push_back(Limit{Constraints::Parent, 1});
        viewers[6]->limits.push_back(Limit{Constraints::Row, 2});
        viewers[6]->limits.push_back(Limit{Constraints::ChessHater});
        viewers.push_back(new Viewer(this, 0x6CAEED, "Wendy"));
        viewers[7]->setSkin(Skins::BabySkin, 6);
        viewers[7]->limits.push_back(Limit{Constraints::GoodNose});
        viewers[7]->limits.push_back(Limit{Constraints::Love, 8});
        viewers[7]->limits.push_back(Limit{Constraints::ChessHater});
        viewers.push_back(new Viewer(this, 0x8B0000, "Ruby"));
        viewers[8]->skin = Skins::BabySkin;
        viewers[8]->limits.push_back(Limit{Constraints::Love, 7});
        viewers[8]->limits.push_back(Limit{Constraints::ChessHater});

        closed_sections.push_back({0, 47});
        closed_sections.push_back({52, 55});
        closed_sections.push_back({60, 63});
        closed_sections.push_back({68, 71});
    }

    printf("Level %i\n", level_id + 1);
}

void Game::run() {
    setupLevel();
    // Continue
    while (running) {
        tick();
        if (!running) return;
        render();
        // ~60 fps
        SDL_Delay(16);
    }
}


void load_assets(SDL_Renderer *sdlrenderer) {
#define TEXTURE_X(name) \
    Asset::name = IMG_LoadTexture(sdlrenderer, "assets/" #name ".png"); \
    if (Asset::name == NULL) { printf("Failed to load " #name "!\n"); }
#include "textures.h"
#undef TEXTURE_X
}
void unload_assets() {
#define TEXTURE_X(name) SDL_DestroyTexture(Asset::name);
#include "textures.h"
#undef TEXTURE_X
}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO)) {
        printf("Couldn't initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Where's My Seat?",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN
    );
    if (!window) {
        printf("Couldn't initialize SDL window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    load_assets(renderer);

    Game game = Game(window, renderer);
#ifdef DEBUG_MODE
    game.max_level_id = 99;
#endif
    game.run();

    unload_assets();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
