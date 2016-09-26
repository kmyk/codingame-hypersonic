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
#include <sstream>
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
const int inf = 1e9+7;

struct point_t { int y, x; };
point_t point(int y, int x) { return (point_t) { y, x }; }
template <typename T>
point_t point(T const & p) { return (point_t) { p.y, p.x }; }
bool operator < (point_t a, point_t b) { return make_pair(a.y, a.x) < make_pair(b.y, b.x); }

namespace primitive {

    enum class player_id_t : int {
        id0 = 0,
        id1 = 1,
        id2 = 2,
        id3 = 3,
    };
    struct config_t {
        int height, width;
        player_id_t self_id;
    };
    istream & operator >> (istream & in, config_t & a) {
        int self_id;
        in >> a.width >> a.height >> self_id;
        a.self_id = player_id_t(self_id);
        return in;
    }

    enum class item_kind_t : int {
        extra_range = 1,
        extra_bomb = 2,
    };
    enum class entyty_type_t {
        player = 0,
        bomb = 1,
        item = 2,
    };
    struct player_t { entyty_type_t type; player_id_t id;    int x, y; int bomb, range; };
    struct bomb_t   { entyty_type_t type; player_id_t owner; int x, y; int time, range; };
    struct item_t   { entyty_type_t type; int dummy1;        int x, y; item_kind_t kind; int dummy2; };
    union entity_t {
        struct { entyty_type_t type; player_id_t owner; int x, y, param1, param2; };
        player_t player;
        bomb_t bomb;
        item_t item;
    };
    istream & operator >> (istream & in, entity_t & a) {
        return in >> (int &)(a.type) >> (int &)(a.owner) >> a.x >> a.y >> a.param1 >> a.param2;
    }
    entity_t entity_cast(player_t const & a) { entity_t b; b.player = a; return b; }
    entity_t entity_cast(bomb_t   const & a) { entity_t b; b.bomb   = a; return b; }
    entity_t entity_cast(item_t   const & a) { entity_t b; b.item   = a; return b; }
    const int bomb_time = 8;
    bomb_t place_bomb(player_t const & a) {
        bomb_t b = {};
        b.type = entyty_type_t::bomb;
        b.owner = a.id;
        b.y = a.y;
        b.x = a.x;
        b.time = bomb_time;
        b.range = a.range;
        return b;
    }
    item_t drop_item(int y, int x, item_kind_t kind) {
        item_t a = {};
        a.type = entyty_type_t::item;
        a.y = y;
        a.x = x;
        a.kind = kind;
        return a;
    }

    enum class cell_t {
        wall = -2,
        empty = -1,
        box = 0,
        box_extra_range = 1,
        box_extra_bomb = 2,
    };
    bool is_box(cell_t a) {
        return a != cell_t::wall and a != cell_t::empty;
    }
    struct turn_t {
        config_t config;
        vector<vector<cell_t> > field;
        vector<entity_t> entities;
    };
    istream & operator >> (istream & in, turn_t & a) {
        a.field = vectors(cell_t::empty, a.config.height, a.config.width);
        repeat (y, a.config.height) {
            repeat (x, a.config.width) {
                char c; in >> c;
                assert (c == '.' or c == 'X' or isdigit(c));
                a.field[y][x] =
                    c == '.' ? cell_t::empty :
                    c == 'X' ? cell_t::wall :
                    cell_t(c-'0');
            }
        }
        int n; in >> n;
        a.entities.resize(n);
        repeat (i,n) in >> a.entities[i];
        return in;
    }
    item_kind_t open_item_box(cell_t a) {
        switch (a) {
            case cell_t::box_extra_range: return item_kind_t::extra_range;
            case cell_t::box_extra_bomb:  return item_kind_t::extra_bomb;
            default: assert (false);
        }
    }

    enum class command_t {
        move = 0,
        bomb = 1,
    };
    struct output_t {
        command_t command;
        int y, x;
        string message;
    };
    ostream & operator << (ostream & out, output_t const & a) {
        const string table[] = { "MOVE", "BOMB" };
        return out << table[int(a.command)] << ' ' << a.x << ' ' << a.y << ' ' << a.message;
    }
    output_t default_output(entity_t const & self) {
        output_t output = {};
        output.command = command_t::move;
        output.y = self.y;
        output.x = self.x;
        return output;
    }

}
using namespace primitive;

vector<point_t> list_breakable_boxes(int y, int x, int range, vector<vector<cell_t> > const & field) {
    int h = field.size(), w = field.front().size();
    vector<point_t> breakable;
    if (is_box(field[y][x])) {
        breakable.push_back((point_t) { y, x });
    }
    if (field[y][x] == cell_t::empty) {
        repeat (i,4) {
            repeat_from (l,1,range) {
                int ny = y + l*dy[i];
                int nx = x + l*dx[i];
                if (is_on_field(ny, nx, h, w)) {
                    if (is_box(field[ny][nx])) {
                        breakable.push_back((point_t) { ny, nx });
                    }
                    if (field[ny][nx] != cell_t::empty) {
                        break;
                    }
                }
            }
        }
    }
    return breakable;
}
vector<vector<int> > breakable_boxes(int range, vector<vector<cell_t> > const & field) {
    int h = field.size(), w = field.front().size();
    vector<vector<int> > cnt = vectors(int(), h, w);
    repeat (y,h) {
        repeat (x,w) {
            cnt[y][x] = list_breakable_boxes(y, x, range, field).size();
        }
    }
    return cnt;
}
vector<vector<bool> > things_being_broken(vector<entity_t> const & entities , vector<vector<cell_t> > const & field) {
    int h = field.size(), w = field.front().size();
    vector<vector<bool> > broken = vectors(bool(), h, w);
    for (auto & ent : entities) if (ent.type == entyty_type_t::bomb) {
        for (auto p : list_breakable_boxes(ent.y, ent.x, ent.bomb.range, field)) {
            broken[p.y][p.x] = true;
        }
    }
    return broken;
}
vector<vector<cell_t> > remove_ineffective_things(vector<vector<cell_t> > const & field, vector<vector<bool> > const & removed) {
    int h = field.size(), w = field.front().size();
    vector<vector<cell_t> > nfield = field;
    repeat (y,h) {
        repeat (x,w) if (removed[y][x]) {
            nfield[y][x] = cell_t::empty;
        }
    }
    return nfield;
}
vector<vector<double> > boxes_potential(vector<vector<cell_t> > const & field, function<double (cell_t, int)> f) {
    int h = field.size(), w = field.front().size();
    vector<vector<double> > pot = vectors(double(), h, w);
    repeat (y,h) {
        repeat (x,w) if (field[y][x] == cell_t::empty) {
            repeat (ny,h) {
                repeat (nx,w) if (is_box(field[ny][nx])) {
                    pot[y][x] += f(field[ny][nx], abs(ny - y) + abs(nx - x));
                }
            }
        }
    }
    return pot;
}

int total_bomb(player_id_t id, vector<entity_t> & entities) {
    int placed = 0;
    int reserved = 0;
    for (auto & ent : entities) {
        if (ent.type == entyty_type_t::player) {
            if (ent.player.id == id) {
                reserved += ent.player.bomb;
            }
        } else if (ent.type == entyty_type_t::bomb) {
            if (ent.bomb.owner == id) {
                placed += 1;
            }
        }
    }
    return placed + reserved;
}

map<point_t,entity_t> entity_map(vector<entity_t> const & entities) {
    map<point_t,entity_t> ent_at;
    for (auto & ent : entities) {
        ent_at[point(ent)] = ent;
    }
    return ent_at;
}

multimap<point_t,entity_t> entity_multimap(vector<entity_t> const & entities) {
    multimap<point_t,entity_t> ent_at;
    for (auto & ent : entities) {
        ent_at.emplace(point(ent), ent);
    }
    return ent_at;
}

vector<vector<int> > distance_field(int sy, int sx, map<point_t,entity_t> const & ent_at, vector<vector<cell_t> > const & field) {
    int h = field.size(), w = field.front().size();
    vector<vector<int> > dist = vectors(inf, h, w);
    queue<point_t> que;
    dist[sy][sx] = 0;
    que.push(point(sy, sx));
    while (not que.empty()) {
        point_t p = que.front(); que.pop();
        repeat (i,4) {
            int ny = p.y + dy[i];
            int nx = p.x + dx[i];
            if (not is_on_field(ny, nx, h, w)) continue;
            if (dist[ny][nx] != inf) continue;
            if (field[ny][nx] != cell_t::empty) continue;
            if (ent_at.count(point(ny, nx))) {
                auto & ent = ent_at.at(point(ny, nx));
                if (ent.type == entyty_type_t::bomb) continue;
            }
            dist[ny][nx] = dist[p.y][p.x] + 1;
            que.push(point(ny, nx));
        }
    }
    return dist;
}
vector<vector<double> > item_potential(map<point_t,entity_t> const & ent_at, vector<vector<cell_t> > const & field, function<double (item_kind_t, int)> f) {
    int h = field.size(), w = field.front().size();
    vector<vector<double> > pot = vectors(0.0, h, w);
    for (auto & it : ent_at) {
        auto & ent = it.second;
        if (ent.type == entyty_type_t::item) {
            vector<vector<int> > dist = distance_field(ent.y, ent.x, ent_at, field);
            repeat (y,h) {
                repeat (x,w) if (dist[y][x] != inf) {
                    pot[y][x] += f(ent.item.kind, dist[y][x]);
                }
            }
        }
    }
    return pot;
}

vector<vector<int> > exploded_time(multimap<point_t,entity_t> const & ent_at, vector<vector<cell_t> > const & field) {
    int h = field.size(), w = field.front().size();
    vector<vector<int> > result = vectors(inf, h, w);
    function<void (entity_t const &, int)> explode = [&](entity_t const & ent, int time) {
        if (result[ent.y][ent.x] <= time) return;
        result[ent.y][ent.x] = time;
        repeat (i,4) {
            repeat_from (l, 1, ent.bomb.range) {
                int ny = ent.y + l*dy[i];
                int nx = ent.x + l*dx[i];
                if (not is_on_field(ny, nx, h, w)) continue;
                if (field[ny][nx] == cell_t::wall) break;
                setmin(result[ny][nx], time);
                for (auto rng = ent_at.equal_range(point(ny, nx)); rng.first != rng.second; ++ rng.first) {
                    auto & nent = rng.first->second;
                    // > Any bomb caught in an explosion is treated as if it had exploded at the very same moment.
                    if (nent.type == entyty_type_t::bomb and nent.bomb.time > time) explode(nent, time);
                    // > Explosions do not go through obstructions such as boxes, items or other bombs, but are included on the cells the obstruction occupies.
                    // > A single obstruction may block the explosions of several bombs that explode on the same turn.
                    if (nent.type != entyty_type_t::player) break;
                }
                if (field[ny][nx] != cell_t::empty) break;
            }
        }
    };
    for (auto & it : ent_at) {
        auto & ent = it.second;
        if (ent.type == entyty_type_t::bomb) {
            explode(ent, ent.bomb.time);
        }
    }
    return result;
}

entity_t *find_entity(vector<entity_t> & entities, entyty_type_t type, player_id_t owner) {
    for (entity_t & ent : entities) {
        if (ent.type == type and ent.owner == owner) {
            return &ent;
        }
    }
    return nullptr;
}
map<player_id_t,player_t> select_player(vector<entity_t> & entities) {
    map<player_id_t,player_t> player;
    for (auto & ent : entities) {
        if (ent.type == entyty_type_t::player) {
            player[ent.player.id] = ent.player;
        }
    }
    return player;
}

turn_t next_turn(turn_t const & cur, map<player_id_t,output_t> const & command) {
    int width  = cur.config.width;
    int height = cur.config.height;
    turn_t nxt = {};
    nxt.config = cur.config;
    nxt.field = cur.field;
    // bomb
    // > At the start of the round, all bombs have their countdown decreased by 1.
    // > Any bomb countdown that reaches 0 will cause the bomb to explode immediately, before players move.
    vector<vector<int> > exptime = exploded_time(entity_multimap(cur.entities), cur.field);
    map<point_t,item_t> items; // after explosion
    repeat (y,height) {
        repeat (x,width) {
            if (exptime[y][x]-1 == 0) {
                // > Once the explosions have been computed, any box hit is then removed. This means that the destruction of 1 box can count for 2 different players.
                if (is_box(cur.field[y][x])) {
                    nxt.field[y][x] = cell_t::empty;
                    // drop item
                    if (cur.field[y][x] != cell_t::box) {
                        item_kind_t kind = open_item_box(cur.field[y][x]);
                        items[point(y, x)] = drop_item(y, x, kind);
                    }
                }
            }
        }
    }
    // split entities
    map<player_id_t,player_t> players; // after explosion
    map<point_t,bomb_t> bombs; // after explosion, before placing
    for (entity_t ent : cur.entities) {
        if (exptime[ent.y][ent.x]-1 == 0) continue;
        switch (ent.type) {
            case entyty_type_t::player:
                players[ent.player.id] = ent.player;
                break;
            case entyty_type_t::bomb:
                ent.bomb.time -= 1;
                bombs[point(ent)] = ent.bomb;
                nxt.entities.push_back(ent);
                break;
            case entyty_type_t::item:
                items[point(ent)] = ent.item;
                break;
        }
    }
    // player
    // > Players then perform their actions simultaneously.
    // > Any bombs placed by a player appear at the end of the round.
    set<point_t> player_exists; // moved
    for (auto & it : players) {
        player_t ent = it.second;
        if (command.count(ent.id)) {
            output_t output = command.at(ent.id);
            // place bomb
            if (output.command == command_t::bomb) {
                assert (ent.bomb >= 1);
                assert (not bombs.count(point(ent)));
                ent.bomb -= 1;
                nxt.entities.push_back(entity_cast(place_bomb(ent))); // don't add to map<point_t,player_t> bombs
            }
            // move
            if (output.y != ent.y or output.x != ent.y) {
                assert (abs(output.y - ent.y) <= 1);
                assert (abs(output.x - ent.x) <= 1);
                assert (not bombs.count(point(output)));
                assert (nxt.field[output.y][output.x] == cell_t::empty);
                ent.y = output.y;
                ent.x = output.x;
                // get item
                if (items.count(point(ent))) {
                    switch (items[point(ent)].kind) {
                        case item_kind_t::extra_range: ent.range += 1; break;
                        case item_kind_t::extra_bomb:  ent.bomb  += 1; break;
                    }
                }
            }
        }
        player_exists.insert(point(ent));
        nxt.entities.push_back(entity_cast(ent));
    }
    // item
    for (auto & it : items) {
        item_t ent = it.second;
        if (player_exists.count(point(ent))) continue;
        nxt.entities.push_back(entity_cast(ent));
    }
    return nxt;
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
        player_id_t self_id = config.self_id;
        player_id_t opponent_id = player_id_t(1 - int(self_id));
        entity_t & self = *find_entity(turn.entities, entyty_type_t::player, self_id);
        entity_t & opponent = *find_entity(turn.entities, entyty_type_t::player, opponent_id);
        vector<vector<cell_t> > & field = turn.field;
        // cache
        vector<vector<bool> > being_broken = things_being_broken(turn.entities, field);
        vector<vector<cell_t> > effective_field = remove_ineffective_things(field, being_broken);
        vector<vector<int> > breakable = breakable_boxes(self.player.range, effective_field);
        vector<vector<double> > box_pot = boxes_potential(effective_field, [](cell_t type, int dist) { return dist == 0 ? 3. : 1./dist; });
        map<point_t,entity_t> ent_at = entity_map(turn.entities);
        vector<vector<int> > exptime = exploded_time(entity_multimap(turn.entities), field);
        vector<vector<int> > dist_self = distance_field(self.y, self.x, ent_at, field);
        vector<vector<double> > item_pot = item_potential(ent_at, field, [](item_kind_t kind, int dist) { return dist == 0 ? 3. : 1./dist; });

        // construct output
        output_t output = default_output(self);
        vector<int> dirs { 0, 1, 2, 3 };
        whole(shuffle, dirs, engine);
        for (int i : dirs) {
            int ny = self.y + dy[i];
            int nx = self.x + dx[i];
            if (not is_on_field(ny, nx, height, width)) continue;
            if (field[ny][nx] != cell_t::empty) continue;
            auto rank = [&](int y, int x) { return make_tuple(exptime[y][x], item_pot[y][x], breakable[y][x], box_pot[y][x]); };
            if (exptime[self.y][self.x] and output.y == self.y and output.x == self.x) {
                output.y = ny;
                output.x = nx;
            }
            if (rank(output.y, output.x) <= rank(ny, nx)) {
                output.y = ny;
                output.x = nx;
            }
        }
        if (self.player.bomb >= 3 * box_pot[self.y][self.x] ) {
            output.command = command_t::bomb;
        } else if (self.player.bomb >= 1 and breakable[self.y][self.x] >= 1) {
            if (self.player.bomb == 1 and breakable[output.y][output.x] > breakable[self.y][self.x]) {
                // cancel
            } else {
                output.command = command_t::bomb;
            }
        } else if (self.player.bomb >= 1 and box_pot[self.y][self.x] < 0.001) {
            output.command = command_t::bomb;
        }
        if (output.command == command_t::bomb) {
            bool escapable = false;
            repeat (i,4) {
                int ny = output.y + dy[i];
                int nx = output.x + dx[i];
                if (not is_on_field(ny, nx, height, width)) continue;
                if (ny == self.y and nx == self.x) continue;
                if (field[ny][nx] != cell_t::empty) continue;
                if (ent_at.count(point(ny, nx)) and ent_at[point(ny, nx)].type == entyty_type_t::bomb) continue;
                if (self.y != output.y or self.x != output.x) {
                    escapable = true;
                } else {
                    repeat (j,4) {
                        int nny = output.y + dy[i] + dy[j];
                        int nnx = output.x + dx[i] + dy[j];
                        if (not is_on_field(nny, nnx, height, width)) continue;
                        if (nny == self.y and nnx == self.x) continue;
                        if (field[nny][nnx] != cell_t::empty) continue;
                        if (ent_at.count(point(nny, nnx)) and ent_at[point(nny, nnx)].type == entyty_type_t::bomb) continue;
                        escapable = true;
                    }
                }
            }
            if (not escapable) {
                output.command = command_t::move;
            }
        }

        // hint message
        {
            ostringstream oss;
            oss << "R" << self.player.range << "/B" << total_bomb(config.self_id, turn.entities);;
            output.message = oss.str();
        }
        // return
        outputs.push_back(output);
        return output;
    }

    void debug_print() {
        vector<vector<char> > s = vectors<char>(' ', config.height, config.width);
        repeat (y,config.height) {
            repeat (x,config.width) {
                cell_t z = turn.field[y][x];
                if (z != cell_t::empty) {
                    s[y][x] =
                        z == cell_t::wall ? '#' :
                        z == cell_t::box ? 'Z' :
                        'A'+int(z)-1;
                }
            }
        }
        for (auto & ent : turn.entities) {
            char c = '?';
            if (ent.type == entyty_type_t::player) {
                c = (ent.player.id == config.self_id) ? '@' : '&';
            } else if (ent.type == entyty_type_t::bomb) {
                c = '*';
            } else if (ent.type == entyty_type_t::item) {
                c = 'a'+int(ent.item.kind)-1;
            }
            s[ent.y][ent.x] = c;
        }
        repeat (y,config.height) {
            repeat (x,config.width) {
                cerr << s[y][x];
            }
            cerr << endl;
        }
    }
};

int main() {
    config_t config; cin >> config;
    AI ai(config);
    while (true) {
        turn_t turn = { config }; cin >> turn;
        output_t output = ai.think(turn);
        cout << output << endl;
        ai.debug_print();
    }
}
