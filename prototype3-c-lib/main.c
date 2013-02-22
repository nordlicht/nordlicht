#include "vidcode.h"
#include "stdio.h"

int main(int argv, char** argc) {
    vidcode *code;
    vidcode_create(&code, 1024, 128);

    vidcode_input(code, "/home/seb/tmp/[UTWoots] Sword Art Online [Complete]/[UTWoots]_Sword_Art_Online_-_23_[720p][45F8C851].mkv");

    while (!vidcode_is_done(code)) {
    }
    vidcode_free(code);
}
