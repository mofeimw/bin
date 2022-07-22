/* ==================================== mofei ===================================== */
/* a ratpoison inspired minimal hotkey launcher for personal use on macOS with skhd */
/* -------------------------------------------------------------------------------- */
/** gcc mousetoxin.c -framework ApplicationServices -framework Carbon -o mousetoxin */
/* ================================================================================ */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>

// declare globals
char INPUT[512] = "";
CFMachPortRef eventTap;

/* === simulate pressing a key === */
void pressKey(int keyCode) {
    // kill the event tap
    CGEventTapEnable(eventTap, false);

    // create keydown & keyup events
    // using HID hardware event source
    CGEventRef down = CGEventCreateKeyboardEvent(CGEventSourceCreate(kCGEventSourceStateHIDSystemState), (CGKeyCode) keyCode, true);
    CGEventRef up = CGEventCreateKeyboardEvent(CGEventSourceCreate(kCGEventSourceStateHIDSystemState), (CGKeyCode) keyCode, false);

    // post the keyboard events
    CGEventPost(kCGHIDEventTap, down);
    CGEventPost(kCGHIDEventTap, up);
}

/* === search ~/.zsh_local === */
char *search(char *word) {
    // get path with $HOME
    char path[64];
    strcat(strcpy(path, getenv("HOME")), "/.zsh_local");

    // open file
    FILE *file;
    file = fopen(path, "r");

    // exit on error opening file
    if (!file) {
        fprintf(stderr, "[mousetoxin] error opening ~/.zsh_local");
        exit(1);
    }

    // construct search string for alias
    char search[256] = "alias ";
    strcat(search, word);
    strcat(search, "=");

    // read file line by line
    char line[128];
    while (fgets(line, 128, file)) {
        // check if line contains search string
        if (strstr(line, search)) {
            char *alias = line;

            // check if the alias declaration uses single/double quotes
            if (alias[strlen(search)] == '"' || alias[strlen(search)] == '\'') {
                // remove leading declaration, quotes, and trailing newline
                alias += strlen(search) + 1;
                alias[strlen(alias) - 2] = '\0';
            } else {
                // remove leading declaration & trailing newline
                alias += strlen(search);
                alias[strlen(alias) - 1] = '\0';
            }

            // return contents of stripped alias declaration
            return alias;
        }
    }

    // close file
    fclose(file);

    // return NULL if match is not found
    return NULL;
}

/* ====== convert an int keycode to a key character ====== */
const char *fromKeyCode(int keyCode, bool shift, bool caps) {
    switch (keyCode) {
        case 0:   return shift || caps ? "A" : "a";
        case 11:  return shift || caps ? "B" : "b";
        case 8:   return shift || caps ? "C" : "c";
        case 2:   return shift || caps ? "D" : "d";
        case 14:  return shift || caps ? "E" : "e";
        case 3:   return shift || caps ? "F" : "f";
        case 5:   return shift || caps ? "G" : "g";
        case 4:   return shift || caps ? "H" : "h";
        case 34:  return shift || caps ? "I" : "i";
        case 38:  return shift || caps ? "J" : "j";
        case 40:  return shift || caps ? "K" : "k";
        case 37:  return shift || caps ? "L" : "l";
        case 46:  return shift || caps ? "M" : "m";
        case 45:  return shift || caps ? "N" : "n";
        case 31:  return shift || caps ? "O" : "o";
        case 35:  return shift || caps ? "P" : "p";
        case 12:  return shift || caps ? "Q" : "q";
        case 15:  return shift || caps ? "R" : "r";
        case 1:   return shift || caps ? "S" : "s";
        case 17:  return shift || caps ? "T" : "t";
        case 32:  return shift || caps ? "U" : "u";
        case 9:   return shift || caps ? "V" : "v";
        case 13:  return shift || caps ? "W" : "w";
        case 7:   return shift || caps ? "X" : "x";
        case 16:  return shift || caps ? "Y" : "y";
        case 6:   return shift || caps ? "Z" : "z";

        case 49:  return " ";                                     // space
        case 36:  return "";                                      // enter
        case 51:  INPUT[strlen(INPUT) - 1] = '\0'; return ""; // backspace
        case 53:  pressKey(53); exit(0);                         // escape

        case 18:  return shift ? "!" : "1";
        case 19:  return shift ? "@" : "2";
        case 20:  return shift ? "#" : "3";
        case 21:  return shift ? "$" : "4";
        case 22:  return shift ? "^" : "6";
        case 23:  return shift ? "%" : "5";
        case 24:  return shift ? "+" : "=";
        case 25:  return shift ? "(" : "9";
        case 26:  return shift ? "&" : "7";
        case 27:  return shift ? "_" : "-";
        case 28:  return shift ? "*" : "8";
        case 29:  return shift ? ")" : "0";
        case 30:  return shift ? "}" : "]";

        case 33:  return shift ? "{" : "[";
        case 39:  return shift ? "\"" : "'";
        case 41:  return shift ? ":" : ";";
        case 42:  return shift ? "|" : "\\";
        case 43:  return shift ? "<" : ",";
        case 44:  return shift ? "?" : "/";
        case 47:  return shift ? ">" : ".";
        case 50:  return shift ? "~" : "`";
    }

    return "";                   // unknown
}

/* =================================== keyboard event handler ==================================== */
CGEventRef CGEventCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon) {
    // get flags & keycode
    CGEventFlags flags = CGEventGetFlags(event);
    CGKeyCode keyCode = (CGKeyCode) CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);

    // check shift & caps lock
    bool shift = flags & kCGEventFlagMaskShift;
    bool caps = flags & kCGEventFlagMaskAlphaShift;

    // append key to the end of INPUT
    strcat(INPUT, fromKeyCode(keyCode, shift, caps));

    // check if the first key is occupied by skhd
    if (strlen(INPUT) == 1) {
        // skhd BLACKLIST
        const char BLACKLIST[] = {
            'z',    // kitty
            'f',   // finder
            'n',    // notes
            'c',  // firefox
            'x',  // spotify
            'v',    // skype
        };

        for (int i = 0; i < strlen(BLACKLIST); i++) {
            if (INPUT[0] == BLACKLIST[i]) {
                // send the original key
                pressKey((int) keyCode);
                exit(0);
            }
        }
    }

    // search ~/.zsh_local
    char *alias = search(INPUT);
    if (alias) {
        // run the alias if found
        system(strdup(alias));

        // escape & exit
        pressKey(53);
        exit(0);
    }

    // prevent keypress from going through
    return NULL;
}

/* === 6 second suicide thread === */
void *suicide() {
    while (1) {
        sleep(6);
        pressKey(53);
        exit(0);
    }
}

int main(int argc, const char *argv[]) {
    // spawn a new thread to limit lifetime
    pthread_t tid;
    pthread_create(&tid, NULL, suicide, NULL);

    // create an event mask & event tap to grab keyboard events
    CGEventMask eventMask = CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventFlagsChanged);
    eventTap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, 0, eventMask, CGEventCallback, NULL);

    // exit if unable to create the event tap
    // permissions missing in system preferences
    if (!eventTap) {
        fprintf(stderr, "[mousetoxin] unable to create event tap - please check permissions");
        exit(1);
    }

    // create a run loop source
    CFRunLoopSourceRef runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);

    // enable the event tap
    CGEventTapEnable(eventTap, true);

    // start the loop
    CFRunLoopRun();

    return 0;
}
