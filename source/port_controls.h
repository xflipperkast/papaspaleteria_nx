#pragma once
#include <math.h>
#include <string.h>

/* Standalone policy: invoke the original Decline action, never accept consent. */
static int paleteria_launcher_action(const char *package, const char *activity,
                                    const Apk64DialogItem *items, size_t count) {
    if (strcmp(package,"air.com.flipline.papaspaleteriatogo") ||
        strcmp(activity,"com/unity/android/sdk/PlayerLauncher")) return -1;
    bool consent=false,accept=false; int decline=-1;
    for(size_t i=0;i<count;++i) {
        const char *s=items[i].text;
        if(!s)continue;
        if(strstr(s,"bandwidth")&&strstr(s,"Privacy Policy"))consent=true;
        if(items[i].actionable&&items[i].enabled) {
            if(!strcmp(s,"\xe2\x9c\x94 ACCEPT"))accept=true;
            if(!strcmp(s,"\xe2\x9c\x98 DECLINE"))decline=(int)i;
        }
    }
    return consent&&accept?decline:-1;
}

typedef struct PaleteriaCursor {
    float x,y;
    bool enabled,blocked;
} PaleteriaCursor;

static void paleteria_cursor_move(PaleteriaCursor *c,float x,float y,float dt) {
    if(!c->enabled)return;
    if(dt<0)dt=0;
    if(dt>0.1f)dt=0.1f;
    float length=sqrtf(x*x+y*y);
    if(length>0.18f) {
        float speed=fminf(1,(length-0.18f)/0.82f)*800*dt/length;
        c->x=fminf(1279,fmaxf(0,c->x+x*speed));
        c->y=fminf(719,fmaxf(0,c->y-y*speed));
    }
}
static void paleteria_cursor_move_buttons(PaleteriaCursor *c,float x,float y,uint64_t buttons,float dt) {
    if(buttons&(APK64_BTN_DLEFT|APK64_BTN_DRIGHT|APK64_BTN_DUP|APK64_BTN_DDOWN)) {
        x=!!(buttons&APK64_BTN_DRIGHT)-!!(buttons&APK64_BTN_DLEFT);
        y=!!(buttons&APK64_BTN_DUP)-!!(buttons&APK64_BTN_DDOWN);
    }
    paleteria_cursor_move(c,x,y,dt);
}

static void paleteria_cursor_update(PaleteriaCursor *c,Apk64InputSnapshot *in,float dt) {
    if(in->buttons_down&APK64_BTN_MINUS) {
        c->enabled=!c->enabled;
        c->blocked=true; /* A held while toggling must not create a new touch. */
    }
    in->buttons&=~APK64_BTN_MINUS;
    in->buttons_down&=~APK64_BTN_MINUS;
    in->buttons_up&=~APK64_BTN_MINUS;
    if(!in->controller_connected) { c->enabled=false;c->blocked=true; }
    if(!c->enabled) {
        if(c->blocked) {
            if(!(in->buttons&APK64_BTN_A))c->blocked=false;
            in->buttons&=~APK64_BTN_A;
            in->buttons_down&=~APK64_BTN_A;
            in->buttons_up&=~APK64_BTN_A;
        }
        return;
    }
    paleteria_cursor_move_buttons(c,in->left_x,in->left_y,in->buttons,dt);
    if(in->touch_count)c->blocked=true;
    if(!(in->buttons&APK64_BTN_A))c->blocked=false;
    if(!in->touch_count&&!c->blocked&&(in->buttons&APK64_BTN_A)) {
        in->touch_count=1;
        /* AIR uses finger 0 for mouse clicks. Physical touches have priority,
         * so the cursor never adds this ID beside a real touchscreen finger. */
        in->touches[0]=(Apk64TouchPoint){.id=0,.x=c->x,.y=c->y,.diameter_x=6,.diameter_y=6};
    }
    in->buttons&=~APK64_BTN_A;
    in->buttons_down&=~APK64_BTN_A;
    in->buttons_up&=~APK64_BTN_A;
    in->left_x=in->left_y=0;
    uint64_t arrows=APK64_BTN_DLEFT|APK64_BTN_DRIGHT|APK64_BTN_DUP|APK64_BTN_DDOWN;
    in->buttons&=~arrows;in->buttons_down&=~arrows;in->buttons_up&=~arrows;
}

/* A small outlined ice-blue pointer, with a darker shadow and pressed tint. */
static const char *const paleteria_cursor_pixels[]={
    "o               ","owo             ","owwo            ","owwwo           ",
    "owwwwo          ","owwwwwos        ","owwwwwwos       ","owwwwwwwos      ",
    "owwwwwwwwos     ","owwwwwwwwwos    ","owwwwwwwwwwos   ","owwwwwwwwwwwos  ",
    "owwwwwwwooooos  ","owwwwowwoossss  ","owwwosowwos     ","owwossowwos     ",
    "owoss  owwos    ","ooss   owwos    "," ss     ooos    ","         ss     "
};
