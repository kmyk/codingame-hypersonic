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

struct config_t {
    int height, width;
    int self_id;
};
istream & operator >> (istream & in, config_t & a) {
    return in >> a.width >> a.height >> a.self_id;
}
bool is_on_field(int y, int x, config_t const & g) { return is_on_field(y, x, g.height, g.width); }

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
    config_t *config;
    vector<vector<bool> > box;
    vector<entity_t> entities;
};
istream & operator >> (istream & in, turn_t & a) {
    a.box.resize(a.config->height, vector<bool>(a.config->width));
    repeat (y, a.config->height) {
        repeat (x, a.config->width) {
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

const int CMD_MOVE = 0;
const int CMD_BOMB = 1;
const string COMMAND_STRING[] = { "MOVE", "BOMB" };
struct output_t {
    int command;
    int y, x;
    string message;
};
ostream & operator << (ostream & out, output_t const & a) {
    return out << COMMAND_STRING[a.command] << ' ' << a.x << ' ' << a.y << ' ' << a.message;
}

int breakable_boxes(int y, int x, int range, vector<vector<bool> > const & box) {
    int h = box.size(), w = box.front().size();
    int cnt = 0;
    if (box[y][x]) {
        cnt += 1;
    } else {
        repeat (i,4) {
            repeat_from (l,1,range) {
                int ny = y + l*dy[i];
                int nx = x + l*dx[i];
                if (is_on_field(ny, nx, h, w)) {
                    if (box[ny][nx]) {
                        cnt += 1;
                        break;
                    }
                }
            }
        }
    }
    return cnt;
}
vector<vector<int> > breakable_boxes(int range, vector<vector<bool> > const & box) {
    int h = box.size(), w = box.front().size();
    vector<vector<int> > cnt = vectors(int(), h, w);
    repeat (y,h) {
        repeat (x,w) {
            cnt[y][x] = breakable_boxes(y, x, range, box);
        }
    }
    return cnt;
}

entity_t *find_entity(vector<entity_t> & entities, int type, int owner) {
    for (entity_t & ent : entities) {
        if (ent.type == type and ent.owner == owner) {
            return &ent;
        }
    }
    return nullptr;
}
output_t default_output(entity_t const & self) {
    output_t output = {};
    output.command = CMD_MOVE;
    output.y = self.y;
    output.x = self.x;
    return output;
}

class AI {
private:
    config_t config;
    vector<turn_t> turns; // history
    turn_t turn; // current
    vector<output_t> outputs;

private:
    default_random_engine engine;
    int randint(int a, int b) {
        uniform_int_distribution<int> dist(a, b);
        return dist(engine);
    }

public:
    AI(config_t const & a_config) {
        random_device device;
        engine = default_random_engine(device());
        config = a_config;
    }
    output_t think(turn_t const & a_turn) {
        // update info
        turns.push_back(turn);
        turn = a_turn;
        // aliases
        int width = config.width;
        int height = config.height;
        entity_t & self = *find_entity(turn.entities, ENT_PLAYER, config.self_id);
        entity_t & opponent = *find_entity(turn.entities, ENT_PLAYER, not config.self_id);
        vector<vector<bool> > & box = turn.box;
        // cache
        vector<vector<int> > breakable = breakable_boxes(self.params.player.range, box);

        // construct output
        output_t output = default_output(self);
        vector<int> dirs { 0, 1, 2, 3 };
        whole(shuffle, dirs, engine);
        for (int i : dirs) {
            int ny = self.y + dy[i];
            int nx = self.x + dx[i];
            if (not is_on_field(ny, nx, height, width)) continue;
            if (breakable[self.y][self.x] == 0 or breakable[self.y][self.x] <= breakable[ny][nx]) {
                output.y = ny;
                output.x = nx;
            }
        }
        if (self.params.player.count >= 1 and breakable[self.y][self.x] >= 1) {
            if (breakable[output.y][output.x] > breakable[self.y][self.x]) {
                // cancel
            } else {
                output.command = CMD_BOMB;
            }
        }

        // return
        outputs.push_back(output);
        return output;
    }
};

int main() {
    config_t config; cin >> config;
    AI ai(config);
    while (true) {
        turn_t turn = { &config }; cin >> turn;
        output_t output = ai.think(turn);
        cout << output << endl;
    }
}
