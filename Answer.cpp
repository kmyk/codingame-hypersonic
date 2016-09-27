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
#include <memory>
#include <functional>
#include <chrono>
#include <sstream>
#include <cassert>
#define repeat(i,n) for (int i = 0; (i) < (n); ++(i))
#define repeat_from(i,m,n) for (int i = (m); (i) < (n); ++(i))
#define repeat_reverse(i,n) for (int i = (n)-1; (i) >= 0; --(i))
#define repeat_from_reverse(i,m,n) for (int i = (n)-1; (i) >= (m); --(i))
#define whole(f,x,...) ([&](decltype((x)) y) { return (f)(begin(y), end(y), ## __VA_ARGS__); })(x)
typedef long long ll;
using namespace std;
using namespace std::chrono;
template <class T> void setmax(T & a, T const & b) { if (a < b) a = b; }
template <class T> void setmin(T & a, T const & b) { if (b < a) a = b; }
template <typename T> vector<vector<T> > vectors(T a, size_t h, size_t w) { return vector<vector<T> >(h, vector<T>(w, a)); }
template <typename T> T input(istream & in) { T a; in >> a; return a; }
const int dy[] = { -1, 1, 0, 0, 0 };
const int dx[] = { 0, 0, 1, -1, 0 };
bool is_on_field(int y, int x, int h, int w) { return 0 <= y and y < h and 0 <= x and x < w; }
const int inf = 1e9+7;

struct point_t { int y, x; };
point_t point(int y, int x) { return (point_t) { y, x }; }
template <typename T>
point_t point(T const & p) { return (point_t) { p.y, p.x }; }
bool operator < (point_t a, point_t b) { return make_pair(a.y, a.x) < make_pair(b.y, b.x); }

namespace primitive {

    const int player_number = 4;
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
    output_t default_output(player_t const & self) {
        output_t output = {};
        output.command = command_t::move;
        output.y = self.y;
        output.x = self.x;
        return output;
    }

}
using namespace primitive;

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

struct exploded_time_info_t { int time; bool owner[player_number]; };
exploded_time_info_t default_explosion_info() { exploded_time_info_t a = { inf }; return a; }
vector<vector<exploded_time_info_t> > exploded_time(multimap<point_t,entity_t> const & ent_at, vector<vector<cell_t> > const & field) {
    // TODO: 箱が壊れて爆風が通れるようになる作用が考慮できていない
    int h = field.size(), w = field.front().size();
    vector<vector<exploded_time_info_t> > result = vectors(default_explosion_info(), h, w);
    auto update = [&](int y, int x, int time, player_id_t owner) {
        if (result[y][x].time < time) return;
        if (result[y][x].time > time) result[y][x] = default_explosion_info();
        result[y][x].time = time;
        result[y][x].owner[int(owner)] = true;
    };
    map<point_t,int> used;
    function<void (bomb_t const &, int)> explode = [&](bomb_t const & ent, int time) {
        if (used.count(point(ent)) and used[point(ent)] <= time) return;
        used[point(ent)] = time;
        update(ent.y, ent.x, time, ent.owner);
        repeat (i,4) {
            repeat_from (l, 1, ent.range) {
                int ny = ent.y + l*dy[i];
                int nx = ent.x + l*dx[i];
                if (not is_on_field(ny, nx, h, w)) continue;
                if (field[ny][nx] == cell_t::wall) break;
                update(ny, nx, time, ent.owner);
                for (auto rng = ent_at.equal_range(point(ny, nx)); rng.first != rng.second; ++ rng.first) {
                    auto & nent = rng.first->second;
                    // > Any bomb caught in an explosion is treated as if it had exploded at the very same moment.
                    if (nent.type == entyty_type_t::bomb and nent.bomb.time > time) explode(nent.bomb, time);
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
            explode(ent.bomb, ent.bomb.time);
        }
    }
    return result;
}

player_t const *find_player(vector<entity_t> const & entities, player_id_t id) {
    for (auto & ent : entities) {
        if (ent.type == entyty_type_t::player and ent.player.id == id) {
            return &ent.player;
        }
    }
    return nullptr;
}

struct next_turn_info_t {
    bool killed[player_number];
    int box[player_number];
    int range[player_number];
    int bomb[player_number];
};
shared_ptr<turn_t> next_turn(turn_t const & cur, map<player_id_t,output_t> const & command, next_turn_info_t & info) {
    info = {};
    shared_ptr<turn_t> nxt = make_shared<turn_t>();
    nxt->config = cur.config;
    nxt->field = cur.field;
    // bomb
    // > At the start of the round, all bombs have their countdown decreased by 1.
    // > Any bomb countdown that reaches 0 will cause the bomb to explode immediately, before players move.
    vector<vector<exploded_time_info_t> > exptime = exploded_time(entity_multimap(cur.entities), cur.field);
    map<point_t,item_t> items; // after explosion
    repeat (y, cur.config.height) {
        repeat (x, cur.config.width) {
            if (exptime[y][x].time-1 == 0) {
                // > Once the explosions have been computed, any box hit is then removed. This means that the destruction of 1 box can count for 2 different players.
                if (is_box(cur.field[y][x])) {
                    nxt->field[y][x] = cell_t::empty;
                    // drop item
                    if (cur.field[y][x] != cell_t::box) {
                        item_kind_t kind = open_item_box(cur.field[y][x]);
                        items[point(y, x)] = drop_item(y, x, kind);
                    }
                    repeat (i,player_number) {
                        if (exptime[y][x].owner[i]) {
                            info.box[i] += 1;
                        }
                    }
                }
            }
        }
    }
    // split entities
    map<player_id_t,player_t> players; // after explosion
    map<point_t,bomb_t> bombs; // after explosion, before placing
    for (entity_t ent : cur.entities) {
        if (exptime[ent.y][ent.x].time-1 == 0) {
            if (ent.type == entyty_type_t::player) {
                info.killed[int(ent.player.id)] = true;
                if (ent.player.id == cur.config.self_id) return nullptr;
            }
            continue;
        }
        switch (ent.type) {
            case entyty_type_t::player:
                players[ent.player.id] = ent.player;
                break;
            case entyty_type_t::bomb:
                ent.bomb.time -= 1;
                bombs[point(ent)] = ent.bomb;
                nxt->entities.push_back(ent);
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
                if (ent.bomb == 0) return nullptr;
                if (bombs.count(point(ent))) return nullptr;
                ent.bomb -= 1;
                nxt->entities.push_back(entity_cast(place_bomb(ent))); // don't add to map<point_t,player_t> bombs
            }
            // move
            if (output.y != ent.y or output.x != ent.y) {
                if (not is_on_field(output.y, output.x, cur.config.height, cur.config.width)) return nullptr;
                if (abs(output.y - ent.y) >= 2) return nullptr;
                if (abs(output.x - ent.x) >= 2) return nullptr;
                if (bombs.count(point(output))) return nullptr;
                if (nxt->field[output.y][output.x] != cell_t::empty) return nullptr;
                if ( cur.field[output.y][output.x] != cell_t::empty) return nullptr; // It seems that they cannot move onto a box, even if the box is broken in the turn.
                ent.y = output.y;
                ent.x = output.x;
                // get item
                if (items.count(point(ent))) {
                    switch (items[point(ent)].kind) {
                        case item_kind_t::extra_range:
                            ent.range += 1;
                            info.range[int(ent.id)] += 1;
                            break;
                        case item_kind_t::extra_bomb:
                            ent.bomb += 1;
                            info.bomb[int(ent.id)] += 1;
                            break;
                    }
                }
            }
        }
        player_exists.insert(point(ent));
        nxt->entities.push_back(entity_cast(ent));
    }
    // item
    for (auto & it : items) {
        item_t ent = it.second;
        if (player_exists.count(point(ent))) continue;
        nxt->entities.push_back(entity_cast(ent));
    }
    return nxt;
}

struct photon_t {
    turn_t turn;
    output_t initial_output;
    int age;
    int box, range, bomb; // difference
    double score;
};
double evaluate_photon(photon_t const & pho) {
    return 4*pho.box + pho.range + pho.bomb; // very magic
}
photon_t default_photon(turn_t const & turn) {
    photon_t pho = {};
    pho.turn = turn;
    pho.score = evaluate_photon(pho);
    return pho;
}
shared_ptr<photon_t> update_photon(photon_t const & pho, int dy, int dx, command_t command) {
    auto self = find_player(pho.turn.entities, pho.turn.config.self_id);
    assert (self);
    output_t output = default_output(*self);
    output.y += dy;
    output.x += dx;
    output.command = command;
    map<player_id_t,output_t> outputs;
    outputs[self->id] = output;
    shared_ptr<photon_t> npho = make_shared<photon_t>(pho);
    next_turn_info_t info;
    auto next_turn_ptr = next_turn(pho.turn, outputs, info);
    if (not next_turn_ptr) return nullptr;
    npho->turn = *next_turn_ptr;
    if (pho.age == 0) npho->initial_output = output;
    npho->age += 1;
    npho->box   += info.box[  int(self->id)];
    npho->range += info.range[int(self->id)];
    npho->bomb  += info.bomb[ int(self->id)];
    npho->score = evaluate_photon(*npho);
    return npho;
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

        // output
        player_t const & self = *find_player(turn.entities, config.self_id);
        output_t output = default_output(self);
        string message = "";
        vector<shared_ptr<photon_t> > beam;
        beam.emplace_back(make_shared<photon_t>(default_photon(turn)));
        const int beam_width = 32;
        repeat (age,14) {
            map<tuple<int,int,command_t>, shared_ptr<photon_t> > used;
            for (auto const & pho : beam) {
                repeat (i,5) {
                    repeat (j,2) {
                        command_t command = j == 0 ? command_t::move : command_t::bomb;
                        if (command == command_t::bomb and pho->age >= 3) continue;
                        shared_ptr<photon_t> npho = update_photon(*pho, dy[i], dx[i], command);
                        if (not npho) continue;
                        auto & self = *find_player(npho->turn.entities, npho->turn.config.self_id);
                        auto key = make_tuple(self.y, self.x, npho->initial_output.command);
                        if (used.count(key) and used[key]->score >= npho->score) continue;
                        used[key] = npho;
                    }
                }
            }
            beam.clear();
            for (auto & it : used) beam.emplace_back(it.second);
            whole(shuffle, beam, engine);
            whole(sort, beam, [&](shared_ptr<photon_t> const & a, shared_ptr<photon_t> const & b) { return a->score > b->score; }); // reversed
            if (beam.size() > beam_width) beam.resize(beam_width);
            if (not beam.empty()) {
                output = beam.front()->initial_output;
            }
        }

        // message
        if (message.empty()) {
            ostringstream oss;
            oss << "R" << self.range << "/B" << total_bomb(self.id, turn.entities);
            message = oss.str();
        }
        output.message = message;
        // return
        outputs.push_back(output);
        return output;
    }
};

int main() {
    config_t config; cin >> config;
    AI ai(config);
    while (true) {
        turn_t turn = { config }; cin >> turn;
        high_resolution_clock::time_point begin = high_resolution_clock::now();
        output_t output = ai.think(turn);
        cout << output << endl;
        high_resolution_clock::time_point end = high_resolution_clock::now();
        ll count = duration_cast<microseconds>(end - begin).count();
        cerr << "elapsed time: " << count/1000 << "." << count%1000 << "msec" << endl;
    }
}
