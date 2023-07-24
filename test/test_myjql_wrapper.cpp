#include <map>
#include <vector>
#include <time.h>
#include <string>
using namespace std;

#ifdef _WIN32
typedef long long my_off_t;
#else
typedef long my_off_t;
#endif

map<string, string> m;

vector<string> key;
map<string,pair<int,bool>> idx;


extern "C" void init() {
    m.clear();
}

extern "C" void insert(char *k, char *v) {
    if (m.count(k)) {
        terminate();
    }
    else{
        key.emplace_back(k);
        idx[k] = {key.size()-1,true};
        m[k] = v;
    }
    
}

extern "C" void erase(char *k) {
    if (!m.count(k)) {
        terminate();
    }
    else{
        auto i = idx[k];
        if(i.second){
            idx[k].second = false;
        }
        m.erase(k);
    }
    
}

extern "C" void get_value(char *k, char *v) {
    if (!m.count(k)) {
        terminate();
    }
    else{
        string str = m[k];
        for(int i=0;i<str.size();++i){
            v[i] = str[i];
        }
        v[str.size()] = 0;
    }
    
}

extern "C" int contain(char *k) {
    return m.count(k) != 0;
}

extern "C" size_t get_total() {
    return m.size();
}

extern "C" int get_rand_key(char *k){
    if(key.size() == 0) return -1;
    srand(time(NULL));
    int idx = rand() % key.size();
    for(int i=0;i<key[idx].size();++i){
        k[i] = key[idx][i];
    }
    k[key[idx].size()] = 0;
    return key[idx].size();
}