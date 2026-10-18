#include <switch.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "runtime_metadata.h"
#include "unpack.h"
#define ROOT "sdmc:/switch/papaspaleteria_nx"
#define WORKER ROOT "/cache/runtime.nro"
#define TEMP WORKER ".tmp"

static bool valid_runtime(const char *path) {
    FILE *file=fopen(path,"rb");if(!file)return false;
    uint8_t buffer[65536],digest[SHA256_HASH_SIZE];uint64_t total=0;
    Sha256Context hash;sha256ContextCreate(&hash);
    size_t count;
    while((count=fread(buffer,1,sizeof(buffer),file))!=0) {
        if(total>RUNTIME_SIZE||count>RUNTIME_SIZE-total){fclose(file);return false;}
        sha256ContextUpdate(&hash,buffer,count);total+=count;
    }
    bool good=!ferror(file)&&total==RUNTIME_SIZE;
    fclose(file);sha256ContextGetHash(&hash,digest);
    return good&&memcmp(digest,RUNTIME_SHA256,sizeof(digest))==0;
}
static bool directory(const char *path) {
    struct stat st;
    return mkdir(path,0777)==0||(errno==EEXIST&&stat(path,&st)==0&&S_ISDIR(st.st_mode));
}
static int failure(const char *reason) {
    consoleInit(NULL);
    printf("Papa's Paleteria To Go!\n\n%s\n\n",reason);
    printf("Install these files in sd:/switch/papaspaleteria_nx/:\n"
           "  papaspaleteria_nx.nro\n  game.apk\n\n"
           "Use title takeover (hold R while opening a game).\n"
           "Press B or Plus to return.\n");
    PadState pad;padConfigureInput(1,HidNpadStyleSet_NpadStandard);padInitializeDefault(&pad);
    while(appletMainLoop()) {
        padUpdate(&pad);if(padGetButtonsDown(&pad)&(HidNpadButton_B|HidNpadButton_Plus))break;
        consoleUpdate(NULL);
    }
    consoleExit(NULL);return 1;
}
int main(int argc,char **argv) {
    (void)argc;(void)argv;
    if(!envHasNextLoad())return failure("This homebrew loader does not support loading the runtime.");
    if(access(ROOT "/game.apk",R_OK)!=0)return failure("game.apk is missing or cannot be read.");
    if(!directory(ROOT)||!directory(ROOT "/cache"))return failure("Cannot create the port's runtime cache.");
    if(!valid_runtime(WORKER)) {
        consoleInit(NULL);
        printf("Papa's Paleteria To Go!\n\nPreparing runtime cache...\nThis may take a moment on the first launch or after an update.\n");
        consoleUpdate(NULL);
        Result rc=romfsInit();
        if(R_FAILED(rc)){consoleExit(NULL);return failure("Cannot read the runtime embedded in this NRO.");}
        FILE *input=fopen("romfs:/runtime.z","rb"),*output=NULL;
        bool good=false;
        if(input)output=fopen(TEMP,"wb");
        if(input&&output) {
            good=unpack_runtime(input,output,RUNTIME_SIZE);
            if(fflush(output)!=0||fsync(fileno(output))!=0)good=false;
        }
        if(input)fclose(input);
        if(output&&fclose(output)!=0)good=false;
        romfsExit();
        if(good)good=valid_runtime(TEMP);
        /* FAT rename need not replace an existing destination. The old file
         * has already failed this version's hash; only derived cache is removed. */
        if(good&&unlink(WORKER)!=0&&errno!=ENOENT)good=false;
        if(good)good=rename(TEMP,WORKER)==0;
        consoleExit(NULL);
        if(!good){unlink(TEMP);return failure("Runtime extraction failed. Check free SD space and recopy the NRO.");}
        rc=fsdevCommitDevice("sdmc");
        if(R_FAILED(rc))return failure("Cannot commit the extracted runtime to the SD card.");
    }
    /* Standard hbloader handoff; the worker has all platform data in RomFS. */
    Result rc=envSetNextLoad(WORKER,WORKER);
    if(R_FAILED(rc))return failure("The homebrew loader rejected the runtime handoff.");
    return 0;
}
