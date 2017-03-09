#include <string>
#include "feature.h"

using namespace std;

void split(const char * str, vector<uint64_t> & terms)
{
    const char *s = str, *e = NULL;
    char buf[32];

    while((e = strchr(s,'_')) != NULL) {
        assert(e-s < 32);

        strncpy(buf,s,e-s);
        buf[e-s] = '\0';
        terms.push_back(strtoull(buf,NULL,16));

        s = e+1;
    }
    // last one
    if(strlen(s)) {
        terms.push_back(strtoull(s,NULL,16));
    }
 
}

void ReadableIdiom(char * str) {
    vector<uint64_t> terms;
    split(str, terms);
    for (auto tit = terms.begin(); tit != terms.end(); ++tit) {
        uint64_t t = *tit;
        if(!(t & ENTRY_MASK)) {
            t = t << ARG_SIZE;
            t |= NOARG;
        }
        if(!(t & ENTRY_MASK)) {
            t = t << ARG_SIZE;
            t |= NOARG;
        }
        IdiomTerm term(t);
	printf("%s; ", term.human_format().c_str());
    }
    printf("\n");
}



int main(int argc, char **argv) {
    char str[1024];
    while (scanf("%s", str) != EOF) {
        ReadableIdiom(str);
    }
}


