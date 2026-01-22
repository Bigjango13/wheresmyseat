#pragma once

#include <vector>
#include <unordered_map>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 608
#define SCALE 16
#define UNUSED __attribute__((unused))

namespace Asset {
#define TEXTURE_X(t) SDL_Texture *t;
#include "textures.h"
#undef TEXTURE_X
}

#define C_CCLEAR  "\xA0"
#define C_CBLACK  "\xA1"
#define C_CRED    "\xA2"
#define C_CGREEN  "\xA3"
#define C_CPURPLE "\xA4"
#define C_CBLUE   "\xA5"

#define C_SSELF  "\x80"
#define C_SX     "\x81"
#define C_SY     "\x82"
#define C_SXY    "\x83"
#define C_SALL   "\x84"
#define C_SSEE   "\x85"
#define C_SDOWN   "\x86"
#define C_SCHESS "\x87"
#define C_SCHECKERS "\x88"

// TODO: Sleepy? (unknow how the imp would work, but funny image)
// TODO: Sodoku
// TODO: Checkers
// TODO: Friends (all in one row)
enum class Checks {
    None,
    Xside,
    Yside,
    XYside,
    All,
    See,
    InvSee,
    Down,
    Chess,
};

// TODO: Bad eyes (sit in front)
enum class Attributes {
    Stinky,
    Tall,
    Singing,
    Talkative,
};

enum class Constraints {
    Aisle,
    Row,
    Seat,

    StageLeft,
    StageRight,

    Love,
    Date,
    Hate,
    Prox,

    Introverted,
    Lonely,
    Extroverted,

    Chess,
    ChessHater,
    ChessProt,
    Checkers,
    CheckersHater,
    GameHater,

    Short,

    GoodNose,
    BadEars,

    Parent,
    Baby,

    Wheelchair,

    Group,
    Friends,
};

class Viewer;
struct Limit {
    Constraints constraint;
    int data = 0;
    bool check(Viewer *viewer, std::vector<Viewer*> &viewers);
};

enum Skins {
    KeySkin = 0,
    NormalSkin,
    ShortSkin,
    TallSkin,
    BabySkin,

    ChessRookSkin,
    ChessKnightSkin,
    ChessBishopSkin,
    ChessQueenSkin
};

class Game;
class Viewer {
public:
    Game *game;
    unsigned char id, level;

    // If either is negative, they are off-board cords
    int x, y;

    int skin = Skins::NormalSkin;
    int timer = 0;
    int color = 0;
    bool happy = false;
    std::string name = "Evil Bob";
    std::vector<Attributes> attrs = {};
    std::vector<Limit> limits = {};
    bool hasAttr(Attributes attribute);
    int getLimit(Constraints constraint);
    Limit *getLimitRef(Constraints constraint);

    void resetPos() {
        x = -((id % 3) + 1);
        y = -((id / 3) + 1);
    }

    int getSavePos() {
        if (x < 0 || y < 0) return -1;
        return y * 8 + x;
    }
    void loadPos(int pos) {
        if (pos == -1) {
            resetPos();
        } else {
            x = pos % 8;
            y = pos / 8;
        }
    }

    bool isChess() {
        return Skins::ChessRookSkin <= skin && skin <= Skins::ChessQueenSkin;
    }

    void setSkin(Skins nextSkin, int data = 0) {
        skin = nextSkin;
        if (skin == Skins::ShortSkin) {
            limits.push_back(Limit{Constraints::Short});
        } else if (skin == Skins::TallSkin) {
            attrs.push_back(Attributes::Tall);
        } else if (skin == Skins::BabySkin) {
            limits.push_back(Limit{Constraints::Baby, data});
        } else if (isChess()) {
            limits.push_back(Limit{Constraints::Chess});
        }
    }

    Viewer(Game *game, int color, std::string name);

    bool render();
    bool check_valid(std::vector<Viewer*> &viewers);
    std::string getStatusColor(Limit lim, std::vector<Viewer*> &viewers);
};

class Game {
public:
    // SDL
    SDL_Window *sdlwindow;
    SDL_Renderer *sdlrenderer;
    bool running = true;

    bool level_complete = false;
    int level_id = 0;
    int max_level_id = 0;
    int debug_state = 0;
    void setupLinks();
    void setupLevel();

    bool hasInfo();
    const char *getInfo();

    Game(SDL_Window *sdlwindow, SDL_Renderer *sdlrenderer) : sdlwindow(sdlwindow), sdlrenderer(sdlrenderer) {}
    Viewer *touching = NULL;
    Viewer *held = NULL;

    Viewer *getAt(int x, int y);
    Viewer *getViewer(int id);
    std::vector<Viewer*> viewers = {};
    std::unordered_map<int,std::vector<int>> links = {};
    std::vector<std::pair<int, int>> closed_sections = {};
    std::vector<std::vector<int>> saved_pos = {};
    bool chairEnabled(int id);

    void tick();
    void render();
    void run();
};
