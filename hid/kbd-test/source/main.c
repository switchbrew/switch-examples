// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

#define CLS() printf(CONSOLE_ESC(2J))
#define CHECK_KEY(k,t) if (KEY_CHECK_FUNC(k)) { \
    printf(t); \
}

#define CHECK_MOD(m,t) if (MOD_CHECK_FUNC(m)) { \
    printf(t); \
}
#define KEY_CHECK_FUNC (*key_check_ptr)
#define MOD_CHECK_FUNC (*mod_check_ptr)

void do_key_check(bool (*key_check_ptr)(HidKeyboardScancode));
void do_mod_check(bool (*mod_check_ptr)(HidKeyboardModifier));

// Main program entrypoint
int main(int argc, char* argv[])
{
    // This example uses a text console, as a simple way to output text to the screen.
    // If you want to write a software-rendered graphics application,
    //   take a look at the graphics/simplegfx example, which uses the libnx Framebuffer API instead.
    // If on the other hand you want to write an OpenGL based application,
    //   take a look at the graphics/opengl set of examples, which uses EGL instead.
    consoleInit(NULL);

    CLS();
    printf("This is a keyboard test!\n");
    printf("To exit: Press +\n");

    // Main loop
    while (appletMainLoop())
    {
        // Scan all the inputs. This should be done once for each frame
        hidScanInput();

        // hidKeysDown returns information about which buttons have been
        // just pressed in this frame compared to the previous one
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS)
            break; // break in order to return to hbmenu

        // Move the cursor to 0,4
        printf( CONSOLE_ESC(4;1H) );
        printf("hidKeyboardHeld: ");
        do_key_check(&hidKeyboardHeld);
        // Clear the rest of the line
        printf("                     \n");
        printf("hidKeyboardDown: ");
        do_key_check(&hidKeyboardDown);
        // Clear the rest of the line
        printf("                     \n");
        printf("hidKeyboardUp: ");
        do_key_check(&hidKeyboardUp);
        // Clear the rest of the line
        printf("                     \n");
        printf("\n");
        printf("hidKeyboardModifierHeld: ");
        do_mod_check(&hidKeyboardModifierHeld);
        // Clear the rest of the line
        printf("                     \n");
        printf("hidKeyboardModifierDown: ");
        do_mod_check(&hidKeyboardModifierDown);
        // Clear the rest of the line
        printf("                     \n");
        printf("hidKeyboardModifierUp: ");
        do_mod_check(&hidKeyboardModifierUp);
        // Clear the rest of the line
        printf("                     \n");

        // Update the console, sending a new frame to the display
        consoleUpdate(NULL);
    }

    // Deinitialize and clean up resources used by the console (important!)
    consoleExit(NULL); return 0;
}

void do_mod_check(bool (*mod_check_ptr)(HidKeyboardModifier)) {
    CHECK_MOD(KBD_MOD_LCTRL,"Left CTRL")
    CHECK_MOD(KBD_MOD_LSHIFT,"Left SHIFT")
    CHECK_MOD(KBD_MOD_LALT,"Left ALT")
    CHECK_MOD(KBD_MOD_LMETA,"Left META")

    CHECK_MOD(KBD_MOD_RCTRL,"Right CTRL")
    CHECK_MOD(KBD_MOD_RSHIFT,"Right SHIFT")
    CHECK_MOD(KBD_MOD_RALT,"Right ALT")
    CHECK_MOD(KBD_MOD_RMETA,"Right META")

    CHECK_MOD(KBD_MOD_CAPSLOCK,"CAPSLOCK")
    CHECK_MOD(KBD_MOD_SCROLLLOCK,"SCROLLOCK")
    CHECK_MOD(KBD_MOD_NUMLOCK,"NUMLOCK")
}

void do_key_check(bool (*key_check_ptr)(HidKeyboardScancode)) {
    // Special returns
    CHECK_KEY(KBD_NONE,"NONE")
    CHECK_KEY(KBD_ERR_OVF,"ERR")

    // Letters
    CHECK_KEY(KBD_A,"A")
    CHECK_KEY(KBD_B,"B")
    CHECK_KEY(KBD_C,"C")
    CHECK_KEY(KBD_D,"D")
    CHECK_KEY(KBD_E,"E")
    CHECK_KEY(KBD_F,"F")
    CHECK_KEY(KBD_G,"G")
    CHECK_KEY(KBD_H,"H")
    CHECK_KEY(KBD_I,"I")
    CHECK_KEY(KBD_J,"J")
    CHECK_KEY(KBD_K,"K")
    CHECK_KEY(KBD_L,"L")
    CHECK_KEY(KBD_M,"M")
    CHECK_KEY(KBD_N,"N")
    CHECK_KEY(KBD_O,"O")
    CHECK_KEY(KBD_P,"P")
    CHECK_KEY(KBD_Q,"Q")
    CHECK_KEY(KBD_R,"R")
    CHECK_KEY(KBD_S,"S")
    CHECK_KEY(KBD_T,"T")
    CHECK_KEY(KBD_U,"U")
    CHECK_KEY(KBD_V,"V")
    CHECK_KEY(KBD_W,"W")
    CHECK_KEY(KBD_X,"X")
    CHECK_KEY(KBD_Y,"Y")
    CHECK_KEY(KBD_Z,"Z")

    // Numbers
    CHECK_KEY(KBD_1,"1")
    CHECK_KEY(KBD_2,"2")
    CHECK_KEY(KBD_3,"3")
    CHECK_KEY(KBD_4,"4")
    CHECK_KEY(KBD_5,"5")
    CHECK_KEY(KBD_6,"6")
    CHECK_KEY(KBD_7,"7")
    CHECK_KEY(KBD_8,"8")
    CHECK_KEY(KBD_9,"9")
    CHECK_KEY(KBD_0,"0")

    // Special characters
    CHECK_KEY(KBD_ENTER,"ENTER")
    CHECK_KEY(KBD_ESC,"ESC")
    CHECK_KEY(KBD_BACKSPACE,"BACKSPACE")
    CHECK_KEY(KBD_DELETE,"DELETE")
    CHECK_KEY(KBD_END,"END")
    CHECK_KEY(KBD_TAB,"TAB")
    CHECK_KEY(KBD_SPACE,"SPACE")
    CHECK_KEY(KBD_MINUS,"-")
    CHECK_KEY(KBD_EQUAL,"=")
    CHECK_KEY(KBD_LEFTBRACE,"[")
    CHECK_KEY(KBD_RIGHTBRACE,"]")
    CHECK_KEY(KBD_BACKSLASH,"\\")
    CHECK_KEY(KBD_HASHTILDE,"~")
    CHECK_KEY(KBD_SEMICOLON,";")
    CHECK_KEY(KBD_APOSTROPHE,"'")
    CHECK_KEY(KBD_GRAVE,"`")
    CHECK_KEY(KBD_COMMA,",")
    CHECK_KEY(KBD_DOT,".")
    CHECK_KEY(KBD_SLASH,"/")

    // F Keys
    CHECK_KEY(KBD_F1,"F1")
    CHECK_KEY(KBD_F2,"F2")
    CHECK_KEY(KBD_F3,"F3")
    CHECK_KEY(KBD_F4,"F4")
    CHECK_KEY(KBD_F5,"F5")
    CHECK_KEY(KBD_F6,"F6")
    CHECK_KEY(KBD_F7,"F7")
    CHECK_KEY(KBD_F8,"F8")
    CHECK_KEY(KBD_F9,"F9")
    CHECK_KEY(KBD_F10,"F10")
    CHECK_KEY(KBD_F11,"F11")
    CHECK_KEY(KBD_F12,"F12")
    CHECK_KEY(KBD_F13,"F13")
    CHECK_KEY(KBD_F14,"F14")
    CHECK_KEY(KBD_F15,"F15")
    CHECK_KEY(KBD_F16,"F16")
    CHECK_KEY(KBD_F17,"F17")
    CHECK_KEY(KBD_F18,"F18")
    CHECK_KEY(KBD_F19,"F19")
    CHECK_KEY(KBD_F20,"F20")
    CHECK_KEY(KBD_F21,"F21")
    CHECK_KEY(KBD_F22,"F22")
    CHECK_KEY(KBD_F23,"F23")
    CHECK_KEY(KBD_F24,"F24")

    // Keypad keys
    CHECK_KEY(KBD_NUMLOCK,"NUMLOCK")
    CHECK_KEY(KBD_KPSLASH,"Keypad /")
    CHECK_KEY(KBD_KPASTERISK,"Keypad *")
    CHECK_KEY(KBD_KPMINUS,"Keypad -")
    CHECK_KEY(KBD_KPPLUS,"Keypad +")
    CHECK_KEY(KBD_KPENTER,"Keypad ENTER")
    CHECK_KEY(KBD_KP1,"Keypad 1")
    CHECK_KEY(KBD_KP2,"Keypad 2")
    CHECK_KEY(KBD_KP3,"Keypad 3")
    CHECK_KEY(KBD_KP4,"Keypad 4")
    CHECK_KEY(KBD_KP5,"Keypad 5")
    CHECK_KEY(KBD_KP6,"Keypad 6")
    CHECK_KEY(KBD_KP7,"Keypad 7")
    CHECK_KEY(KBD_KP8,"Keypad 8")
    CHECK_KEY(KBD_KP9,"Keypad 9")
    CHECK_KEY(KBD_KP0,"Keypad 0")
    CHECK_KEY(KBD_KPDOT,"Keypad .")
    CHECK_KEY(KBD_KPEQUAL,"Keypad =")
    CHECK_KEY(KBD_KPCOMMA,"Keypad ,")
    CHECK_KEY(KBD_KPLEFTPAREN,"Keypad (")
    CHECK_KEY(KBD_KPRIGHTPAREN,"Keypad )")

    // A bunch of other keys that I can't figure out a category for,
    // so I'll categorize it just as the enums in libnx does

    CHECK_KEY(KBD_SYSRQ,"SYSRQ")
    CHECK_KEY(KBD_SCROLLLOCK,"SCROLL LOCK")
    CHECK_KEY(KBD_PAUSE,"PAUSE")
    CHECK_KEY(KBD_CAPSLOCK_ACTIVE,"CAPSLOCK ACTIVE")
    CHECK_KEY(KBD_NUMLOCK_ACTIVE,"NUMLOCK ACTIVE")
    CHECK_KEY(KBD_SCROLLLOCK_ACTIVE,"SCROLL LOCK ACTIVE")

    // Modifier Keys
    CHECK_KEY(KBD_CAPSLOCK,"CAPSLOCK")
    CHECK_KEY(KBD_LEFTCTRL,"Left CTRL")
    CHECK_KEY(KBD_LEFTSHIFT,"Left SHIFT")
    CHECK_KEY(KBD_LEFTALT,"Left ALT")
    CHECK_KEY(KBD_LEFTMETA,"Left META")
    CHECK_KEY(KBD_RIGHTCTRL,"Right CTRL")
    CHECK_KEY(KBD_RIGHTSHIFT,"Right SHIFT")
    CHECK_KEY(KBD_RIGHTALT,"Right ALT")
    CHECK_KEY(KBD_RIGHTMETA,"Right META")

    // Movement Keys
    CHECK_KEY(KBD_INSERT,"INSERT")
    CHECK_KEY(KBD_HOME,"HOME")
    CHECK_KEY(KBD_PAGEUP,"PAGE UP")
    CHECK_KEY(KBD_PAGEDOWN,"PAGE DOWN")
    CHECK_KEY(KBD_RIGHT,"RIGHT")
    CHECK_KEY(KBD_LEFT,"LEFT")
    CHECK_KEY(KBD_DOWN,"DOWN")
    CHECK_KEY(KBD_UP,"UP")

    // What is this key?
    CHECK_KEY(KBD_102ND,"102ND ??")

    // System keys
    CHECK_KEY(KBD_OPEN,"Sys OPEN")
    CHECK_KEY(KBD_HELP,"Sys HELP")
    CHECK_KEY(KBD_PROPS,"Sys PROPS")
    CHECK_KEY(KBD_FRONT,"Sys FRONT")
    CHECK_KEY(KBD_UNDO,"Sys UNDO")
    CHECK_KEY(KBD_CUT,"Sys CUT")
    CHECK_KEY(KBD_COPY,"Sys COPY")
    CHECK_KEY(KBD_PASTE,"Sys PASTE")
    CHECK_KEY(KBD_FIND,"Sys FIND")
    CHECK_KEY(KBD_STOP,"Sys STOP")
    CHECK_KEY(KBD_VOLUMEUP,"Sys Vol UP")
    CHECK_KEY(KBD_VOLUMEDOWN,"Sys Vol DOWN")
    CHECK_KEY(KBD_MUTE,"Sys Mute")
    CHECK_KEY(KBD_POWER,"Sys POWER")
    CHECK_KEY(KBD_COMPOSE,"Sys COMPOSE")

    // Multimedia Keys
    CHECK_KEY(KBD_MEDIA_PLAYPAUSE,"Play/Pause")
    CHECK_KEY(KBD_MEDIA_STOPCD,"Stop CD")
    CHECK_KEY(KBD_MEDIA_PREVIOUSSONG,"Previous")
    CHECK_KEY(KBD_MEDIA_NEXTSONG,"Next")
    CHECK_KEY(KBD_MEDIA_EJECTCD,"Eject CD")
    CHECK_KEY(KBD_MEDIA_VOLUMEUP,"Vol UP")
    CHECK_KEY(KBD_MEDIA_VOLUMEDOWN,"Vol DOWN")
    CHECK_KEY(KBD_MEDIA_MUTE,"Mute")
    CHECK_KEY(KBD_MEDIA_WWW,"WWW")
    CHECK_KEY(KBD_MEDIA_BACK,"Back")
    CHECK_KEY(KBD_MEDIA_FORWARD,"Forward")
    CHECK_KEY(KBD_MEDIA_STOP,"Stop")
    CHECK_KEY(KBD_MEDIA_FIND,"Find")
    CHECK_KEY(KBD_MEDIA_SCROLLUP,"Scroll Up")
    CHECK_KEY(KBD_MEDIA_SCROLLDOWN,"Scroll Down")
    CHECK_KEY(KBD_MEDIA_EDIT,"Edit")
    CHECK_KEY(KBD_MEDIA_SLEEP,"Sleep")
    CHECK_KEY(KBD_MEDIA_COFFEE,"Coffee")
    CHECK_KEY(KBD_MEDIA_REFRESH,"Refresh")
    CHECK_KEY(KBD_MEDIA_CALC,"Calc")
}
