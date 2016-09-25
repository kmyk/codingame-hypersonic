#include <iostream>
#include <vector>
#include <algorithm>
#include <array>
#include <set>
#include <map>
#include <queue>
#include <tuple>
#include <random>
#include <unordered_set>
#include <unordered_map>
#include <functional>
#include <cassert>
#define repeat(i,n) for (int i = 0; (i) < (n); ++(i))
#define repeat_from(i,m,n) for (int i = (m); (i) < (n); ++(i))
#define repeat_reverse(i,n) for (int i = (n)-1; (i) >= 0; --(i))
#define repeat_from_reverse(i,m,n) for (int i = (n)-1; (i) >= (m); --(i))
#define whole(f,x,...) ([&](decltype((x)) y) { return (f)(begin(y), end(y), ## __VA_ARGS__); })(x)
typedef long long ll;
using namespace std;
template <class T> void setmax(T & a, T const & b) { if (a < b) a = b; }
template <class T> void setmin(T & a, T const & b) { if (b < a) a = b; }
template <typename T> vector<vector<T> > vectors(T a, size_t h, size_t w) { return vector<vector<T> >(h, vector<T>(w, a)); }
template <typename T> T input(istream & in) { T a; in >> a; return a; }
const int dy[] = { -1, 1, 0, 0 };
const int dx[] = { 0, 0, 1, -1 };
bool is_on_field(int y, int x, int h, int w) { return 0 <= y and y < h and 0 <= x and x < w; }
int randint(int a, int b) {
    static random_device device;
    static default_random_engine engine(device());
    uniform_int_distribution<int> dist(a, b);
    return dist(engine);
}

struct game_t {
    int height, width;
    int id;
};
istream & operator >> (istream & in, game_t & a) {
    return in >> a.width >> a.height >> a.id;
}
bool is_on_field(int y, int x, game_t const & g) { return is_on_field(y, x, g.height, g.width); }

struct common_params_t { int param1, param2; };
struct player_params_t { int count, range; };
struct bomb_params_t { int count, range; };
union params_t {
    common_params_t common;
    player_params_t player;
    bomb_params_t bomb;
};
const int ENT_PLAYER = 0;
const int ENT_BOMB = 1;
struct entity_t {
    int type;
    int owner;
    int y, x;
    params_t params;
};
istream & operator >> (istream & in, entity_t & a) {
    return in >> a.type >> a.owner >> a.x >> a.y >> a.params.common.param1 >> a.params.common.param2;
}

struct turn_t {
    game_t *game;
    vector<vector<bool> > box;
    vector<entity_t> entities;
};
istream & operator >> (istream & in, turn_t & a) {
    a.box.resize(a.game->height, vector<bool>(a.game->width));
    repeat (y, a.game->height) {
        repeat (x, a.game->width) {
            char c; in >> c;
            assert (c == '.' or c == '0');
            a.box[y][x] = c == '0';
        }
    }
    int n; in >> n;
    a.entities.resize(n);
    repeat (i,n) in >> a.entities[i];
    return in;
}

const string CMD_MOVE = "MOVE";
const string CMD_BOMB = "BOMB";
struct output_t {
    string command;
    int y, x;
    string message;
};
ostream & operator << (ostream & out, output_t const & a) {
    return out << a.command << ' ' << a.x << ' ' << a.y << ' ' << a.message;
}

entity_t *find_entity(vector<entity_t> & entities, int type, int owner) {
    for (entity_t & ent : entities) {
        if (ent.type == type and ent.owner == owner) {
            return &ent;
        }
    }
    return nullptr;
}
int breakable_boxes(int y, int x, int range, vector<vector<bool> > const & box) {
    int h = box.size(), w = box.front().size();
    int cnt = 0;
    cnt += box[y][x];
    repeat (i,4) {
        repeat_from (l,1,range) {
            int ny = y + l*dy[i];
            int nx = x + l*dx[i];
            if (is_on_field(ny, nx, h, w)) {
                cnt += box[ny][nx];
            }
        }
    }
    return cnt;
}

output_t think(turn_t & a) {
    entity_t *self = find_entity(a.entities, ENT_PLAYER, a.game->id);
    entity_t *opponent = find_entity(a.entities, ENT_PLAYER, not a.game->id);
    output_t output = {};
    output.command = CMD_MOVE;
    if (self->params.player.count and breakable_boxes(self->y, self->x, self->params.player.range, a.box)) {
        output.command = CMD_BOMB;
    }
    output.y = self->y;
    output.x = self->x;
    int i = randint(0, 4-1);
    int ny = self->y + dy[i];
    int nx = self->x + dx[i];
    if (is_on_field(ny, nx, *a.game)) {
        output.y = ny;
        output.x = nx;
    }
    return output;
}

int main() {
    game_t game; cin >> game;
    while (true) {
        turn_t turn = { &game }; cin >> turn;
        output_t output = think(turn);
        cout << output << endl;
    }
}
