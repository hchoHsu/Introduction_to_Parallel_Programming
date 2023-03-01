#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <set>
#include <map>
#include <omp.h>
#include <tbb/concurrent_hash_map.h>
#include <boost/functional/hash.hpp>
using namespace std;
using pss = pair<string, string>;
using chmap = oneapi::tbb::concurrent_hash_map<string, string>;

/* 
 * + 待更新:
 * 勝利判定改用數數兒
 * 加速點: DeadState, UpdateState
 */

char **MAP = NULL;
int WIDTH=1, HEIGHT=1;
map<string, string> visited;
const char dirsym[4] = {'W', 'A', 'S', 'D'};
const int dirmat[4][2] = {{0, -1},
                 {-1, 0}, {0,  1}, {1, 0}};

void printState(string &state, bool endline=true) {
    for(int i=0; i<state.size(); i+=2){
        cout << '(' << (int)state[i] << ',' << (int)state[i+1] << ')' << '-';
    }
    if (endline)
        cout << endl;
}

int findBox(string &state, int &a, int &b) {
    int rtval = 0;
    for (int i=2; i<state.size(); i+=2) {
        if (state[i] == a && state[i+1] == b) {
            rtval = i;
            break;
        }
    }
    return rtval;
}

bool isSolved(string &state) {
    bool result = true;
    for(int i=2; i<state.size(); i+=2) {
        if (MAP[state[i+1]][state[i]] != '.')
            result = false;
    }
    return result;
}

bool isDeadState(string &state) {
    int a1 = 0, b1 = 0, a2 = 0, b2 = 0, cnt = 0;
    for (int i=2; i<state.size(); i+=2) { // 對每個箱箱
        if (MAP[state[i+1]][state[i]] == '.')
            continue;
        cnt = 0;
        for (int t=0; t<2; t++) { // 左右 上下
            a1 = state[i] + dirmat[t][0];
            b1 = state[i+1] + dirmat[t][1];
            a2 = state[i] + dirmat[t+2][0];
            b2 = state[i+1] + dirmat[t+2][1];
            if (MAP[b1][a1] == '%') {// 只有一側有%
                if (MAP[b2][a2] != ' ' && MAP[b2][a2] != '.') {
                    cnt++;
                    continue;
                }
                else if (findBox(state, a2, b2)) {
                    cnt++;
                    continue;
                }
            }
            if (MAP[b2][a2] == '%') { // 只有一側有%
                if (MAP[b1][a1] != ' ' && MAP[b1][a1] != '.') {
                    cnt++;
                    continue;
                }
                else if (findBox(state, a1, b1)) {
                    cnt++;
                    continue;
                }
            }

            if (MAP[b1][a1] != ' ' && MAP[b1][a1] != '.') {// 兩側都是空的
                if (MAP[b2][a2] != ' ' && MAP[b2][a2] != '.') {
                    cnt++;
                    continue;
                }
                else if (findBox(state, a2, b2)) {
                    cnt++;
                    continue;
                }
            }

            if (MAP[b2][a2] != ' ' && MAP[b2][a2] != '.') {// 兩側都是空的
                if (MAP[b2][a1] != ' ' && MAP[b1][a1] != '.') {
                    cnt++;
                    continue;
                }
                else if (findBox(state, a1, b1)) {
                    cnt++;
                    continue;
                }
            }
            
        }
    }
    return false;
}

// only update boxes, won't update player.
bool UpdateState(string &cur, int dir, bool &ispush) {
    if (visited.find(cur) != visited.end())
        return false;
    
    int a = cur[0];
    int b = cur[1];
    ispush = false;
    // 如果那不是牆
    if (MAP[b][a] != '#') {
        for (int i=2; i<cur.size(); i+=2) {
            // 如果推箱箱 
            if (a == (int)cur[i] && b == (int)cur[i+1]) {
                int ba = (int)cur[i] + dirmat[dir][0];
                int bb = (int)cur[i+1] + dirmat[dir][1];
                // 如果箱箱前面能推
                if (MAP[bb][ba] == ' ' || MAP[bb][ba] == '.') {
                    // 如果箱箱前面沒有箱箱
                    for (int j=2; j<cur.size(); j+=2) {
                        if (cur[j] == ba && cur[j+1] == bb) {
                            return false;
                        }
                    }
                    cur[i] += dirmat[dir][0];
                    cur[i+1] += dirmat[dir][1];
                    ispush = true;
                    return true;
                }
                else {
                    return false;
                }
            }
        }
        return true;
    }
    return false;
}

// only restore boxes, won't restore player.
void RestoreState(string &cur, int dir) {
    int a = cur[0] + (int)dirmat[dir][0];
    int b = cur[1] + (int)dirmat[dir][1];
    
    for (int i=2; i<cur.size(); i+=2) {
        // 如果有箱箱 
        if (a == (int)cur[i] && b == (int)cur[i+1]) {
            cur[i] = cur[i] - (int)dirmat[dir][0];
            cur[i+1] = cur[i+1] - (int)dirmat[dir][1];
            return;
        }
    }

    return;
}

void solver(string state, string &track) {
    queue<string> BFS;
    BFS.push(state);
    visited.insert(pss(state, ""));
    
    while(!BFS.empty()) {
        string original_cur = BFS.front();
        BFS.pop();

        // #pragma omp parallel for num_threads(6)
        for(int i=0; i<4; i++) {
            string cur = new string(original_cur);
            string curTrack = new string(visited[threadcur]);
            
            // update player
            (*cur)[0] += dirmat[i][0];
            (*cur)[1] += dirmat[i][1];

            // if player move is valid
            bool ispush = false;
            if (UpdateState(*cur, i, ispush)) {
                (*curTrack) += dirsym[i];

                if(isSolved(*cur)) {
                    track = curTrack;
                    return;
                }
                
                // #pragma omp critical
                // {
                if (!isDeadState(*cur))
                    BFS.push(*cur);

                visited.insert(pss(*cur, *curTrack));
                // }
                if (ispush)
                    RestoreState(*cur, i);
            }
        }
    }
}

string loadfile(string filename)
{
    string strMAP;

    ifstream f(filename);
    f.seekg(0, ios::end);
    strMAP.resize(f.tellg());
    f.seekg(0);
    f.read(strMAP.data(), strMAP.size());

    string cur = "";
    cur += (char)0;
    cur += (char)0;
    for(int i=0; i<strMAP.size(); i++){
        if(strMAP[i] == '\n') {
            WIDTH = i/HEIGHT;
            HEIGHT++;
        }
        else if(strMAP[i] == 'x') {
            cur += i%(WIDTH+1);
            cur += HEIGHT-1;
            strMAP[i] = ' ';
        }
        else if(strMAP[i] == 'X') {
            cur += i%(WIDTH+1);
            cur += HEIGHT-1;
            strMAP[i] = '.';
        }
        else if(strMAP[i] == 'o') {
            cur[0] = i%(WIDTH+1);
            cur[1] = HEIGHT-1;
            strMAP[i] = ' ';
        }
        else if(strMAP[i] == 'O') {
            cur[0] = i%(WIDTH+1);
            cur[1] = HEIGHT-1;
            strMAP[i] = '.';
        }
        else if(strMAP[i] == '!') {
            cur[0] = i%(WIDTH+1);
            cur[1] = HEIGHT-1;
            strMAP[i] = '@';
        }
    }
    HEIGHT--;

    MAP = new char*[HEIGHT];
    for(int i=0; i<HEIGHT; i++)
        MAP[i] = new char[WIDTH];
    int i=0, j=0;
    for(int t=0; t<strMAP.size(); t++){
        if (strMAP[t] != '\n') {
            MAP[j][i] = strMAP[t];
            i++;
        }
        else {
            i = 0;
            j++;
        }
    }

    return cur;
}

int main(int argc, char** argv)
{
    string init_state = loadfile(argv[1]);
    string ans = NULL;
    solver(init_state, ans);
    cout << "end\n";
    cout << ans << endl;
}