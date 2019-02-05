// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

// See also libnx nfc.h.

static char *get_amiibo_char(const NfpuModelInfo model_info);

#define PRINT_UPD(fmt, ...) ({   \
    printf(fmt, ## __VA_ARGS__); \
    consoleUpdate(NULL);         \
})

// Indefinitely wait for an event to be signaled
// Break when + is pressed, or if the application should quit (in this case, return value will be non-zero)
Result eventWaitLoop(Event *event) {
    Result rc = 1;
    while (appletMainLoop()) {
        rc = eventWait(event, 0);
        hidScanInput();
        if (R_SUCCEEDED(rc) || (hidKeysDown(CONTROLLER_P1_AUTO) & KEY_PLUS))
            break;
    }
    return rc;
}

// Main program entrypoint
int main(int argc, char* argv[])
{
    Result rc = 0;

    // This example uses a text console, as a simple way to output text to the screen.
    // If you want to write a software-rendered graphics application,
    //   take a look at the graphics/simplegfx example, which uses the libnx Framebuffer API instead.
    // If on the other hand you want to write an OpenGL based application,
    //   take a look at the graphics/opengl set of examples, which uses EGL instead.
    consoleInit(NULL);

    PRINT_UPD("NFC example program.\n");

    PRINT_UPD("Scan an amiibo tag to display its character.\n");
    PRINT_UPD("Press + to exit.\n\n");

    // Initialize the nfp:user and nfc:user services.
    rc = nfpuInitialize();

    // Check if NFC is enabled. If not, wait until it is.
    if (R_SUCCEEDED(rc)) {
        bool nfc_enabled = false;
        rc = nfpuIsNfcEnabled(&nfc_enabled);

        if (R_SUCCEEDED(rc) && !nfc_enabled) {
            // Get the availability change event. This is signaled when a change in NFC availability happens.
            Event availability_change_event = {0};
            rc = nfpuAttachAvailabilityChangeEvent(&availability_change_event);

            // Wait for a change in availability.
            if (R_SUCCEEDED(rc)) {
                PRINT_UPD("NFC is disabled. Please turn off plane mode via the quick settings to continue.\n");
                rc = eventWaitLoop(&availability_change_event);
            }

            eventClose(&availability_change_event);
        }
    }

    // Get the handle of the first controller with NFC capabilities.
    HidControllerID controller = 0;
    if (R_SUCCEEDED(rc)) {
        u32 device_count;
        rc = nfpuListDevices(&device_count, &controller, 1);
    }

    if (R_FAILED(rc))
        goto fail_0;

    // Get the activation event. This is signaled when a tag is detected.
    Event activate_event = {0};
    if (R_FAILED(nfpuAttachActivateEvent(controller, &activate_event)))
        goto fail_1;

    // Get the deactivation event. This is signaled when a tag is removed.
    Event deactivate_event = {0};
    if (R_FAILED(nfpuAttachDeactivateEvent(controller, &deactivate_event)))
        goto fail_2;

    // Start the detection of tags.
    rc = nfpuStartDetection(controller);
    if (R_SUCCEEDED(rc))
        PRINT_UPD("Scanning for a tag...\n");

    // Wait until a tag is detected.
    if (R_SUCCEEDED(rc)) {
        rc = eventWaitLoop(&activate_event);
        if (R_SUCCEEDED(rc))
            PRINT_UPD("A tag was detected, please do not remove it from the NFC spot.\n");
    }

    // If a tag was successfully detected, load it into memory.
    if (R_SUCCEEDED(rc))
        rc = nfpuMount(controller, NfpuDeviceType_Amiibo, NfpuMountTarget_All);

    // Retrieve the model info data, which contains the amiibo id.
    if (R_SUCCEEDED(rc)) {
        NfpuModelInfo model_info = {0};
        rc = nfpuGetModelInfo(controller, &model_info);
        
        if (R_SUCCEEDED(rc))
            PRINT_UPD("Amiibo character: %s.\n", get_amiibo_char(model_info));
    }
    
    if (R_SUCCEEDED(rc)) {
        PRINT_UPD("You can now remove the tag.\n");
        eventWaitLoop(&deactivate_event);
    }

    // If an error happened during detection/reading, print it.
    if (R_FAILED(rc))
        PRINT_UPD("Error: 0x%x.\n", rc);
    
    // Unmount the tag.
    nfpuUnmount(controller);

    // Stop the detection of tags.
    nfpuStopDetection(controller);

    // Wait for the user to explicitely exit.
    PRINT_UPD("Press + to exit.\n");
    while (appletMainLoop()) {
        hidScanInput();
        if (hidKeysDown(CONTROLLER_P1_AUTO) & KEY_PLUS)
            break;
    }

    // Cleanup.
fail_2: 
    eventClose(&deactivate_event);
fail_1:
    eventClose(&activate_event);
fail_0:
    nfpuExit();
    // Deinitialize and clean up resources used by the console (important!)
    consoleExit(NULL);
    return 0;
}

static char *get_amiibo_char(const NfpuModelInfo model_info) {
    u16 char_id = model_info.amiibo_id[0] << 8 | model_info.amiibo_id[1]; // Reverse byte order for convenience
    switch (char_id) {
        case 0x0000: return "Mario";
        case 0x0001: return "Luigi";
        case 0x0002: return "Peach";
        case 0x0003: return "Yoshi";
        case 0x0004: return "Rosalina";
        case 0x0005: return "Bowser";
        case 0x0006: return "Bowser Jr.";
        case 0x0007: return "Wario";
        case 0x0008: return "Donkey Kong";
        case 0x0009: return "Diddy Kong";
        case 0x000a: return "Toad";
        case 0x0013: return "Daisy";
        case 0x0014: return "Waluigi";
        case 0x0015: return "Goomba";
        case 0x0017: return "Boo";
        case 0x0023: return "Koopa Troopa";
        case 0x0080: return "Poochy";
        case 0x0100: return "Link";
        case 0x0101: return "Zelda";
        case 0x0102: return "Ganon";
        case 0x0103: return "Midna";
        case 0x0105: return "Daruk";
        case 0x0106: return "Urbosa";
        case 0x0107: return "Mipha";
        case 0x0108: return "Revali";
        case 0x0140: return "Guardian";
        case 0x0141: return "Bokoblin";
        case 0x0180: return "Villager";
        case 0x0181: return "Isabelle";
        case 0x0182: return "K.K. Slider";
        case 0x0183: return "Tom Nook";
        case 0x0184: return "Timmy & Tommy";
        case 0x0185: return "Timmy";
        case 0x0186: return "Tommy";
        case 0x0187: return "Sable";
        case 0x0188: return "Mabel";
        case 0x0189: return "Labelle";
        case 0x018a: return "Reese";
        case 0x018b: return "Cyrus";
        case 0x018c: return "Digby";
        case 0x018d: return "Rover";
        case 0x018e: return "Mr. Resetti";
        case 0x018f: return "Don Resetti";
        case 0x0190: return "Brewster";
        case 0x0191: return "Harriet";
        case 0x0192: return "Blathers";
        case 0x0193: return "Celeste";
        case 0x0194: return "Kicks";
        case 0x0195: return "Porter";
        case 0x0196: return "Kapp'n";
        case 0x0197: return "Leilani";
        case 0x0198: return "Lelia";
        case 0x0199: return "Grams";
        case 0x019a: return "Chip";
        case 0x019b: return "Nat";
        case 0x019c: return "Phineas";
        case 0x019d: return "Copper";
        case 0x019e: return "Booker";
        case 0x019f: return "Pete";
        case 0x01a0: return "Pelly";
        case 0x01a1: return "Phyllis";
        case 0x01a2: return "Gulliver";
        case 0x01a3: return "Joan";
        case 0x01a4: return "Pascal";
        case 0x01a5: return "Katarina";
        case 0x01a6: return "Sahara";
        case 0x01a7: return "Wendell";
        case 0x01a8: return "Redd";
        case 0x01a9: return "Gracie";
        case 0x01aa: return "Lyle";
        case 0x01ab: return "Pave";
        case 0x01ac: return "Zipper";
        case 0x01ad: return "Jack";
        case 0x01ae: return "Franklin";
        case 0x01af: return "Jingle";
        case 0x01b0: return "Tortimer";
        case 0x01b1: return "Dr. Shrunk";
        case 0x01b3: return "Blanca";
        case 0x01b4: return "Leif";
        case 0x01b5: return "Luna";
        case 0x01b6: return "Katie";
        case 0x01c1: return "Lottie";
        case 0x0200: return "Cyrano";
        case 0x0201: return "Antonio";
        case 0x0202: return "Pango";
        case 0x0203: return "Anabelle";
        case 0x0206: return "Snooty";
        case 0x0208: return "Annalisa";
        case 0x0209: return "Olaf";
        case 0x0214: return "Teddy";
        case 0x0215: return "Pinky";
        case 0x0216: return "Curt";
        case 0x0217: return "Chow";
        case 0x0219: return "Nate";
        case 0x021a: return "Groucho";
        case 0x021b: return "Tutu";
        case 0x021c: return "Ursala";
        case 0x021d: return "Grizzly";
        case 0x021e: return "Puala";
        case 0x021f: return "Ike";
        case 0x0220: return "Charlise";
        case 0x0221: return "Beardo";
        case 0x0222: return "Klaus";
        case 0x022d: return "Jay";
        case 0x022e: return "Robin";
        case 0x022f: return "Anchovy";
        case 0x0230: return "Twiggy";
        case 0x0231: return "Jitters";
        case 0x0232: return "Piper";
        case 0x0233: return "Admiral";
        case 0x0235: return "Midge";
        case 0x0238: return "Jacob";
        case 0x023c: return "Lucha";
        case 0x023d: return "Jacques";
        case 0x023e: return "Peck";
        case 0x023f: return "Sparro";
        case 0x024a: return "Angus";
        case 0x024b: return "Rodeo";
        case 0x024d: return "Stu";
        case 0x024f: return "T-Bone";
        case 0x0251: return "Coach";
        case 0x0252: return "Vic";
        case 0x025d: return "Bob";
        case 0x025e: return "Mitzi";
        case 0x025f: return "Rosie";
        case 0x0260: return "Olivia";
        case 0x0261: return "Kiki";
        case 0x0262: return "Tangy";
        case 0x0263: return "Punchy";
        case 0x0264: return "Purrl";
        case 0x0265: return "Moe";
        case 0x0266: return "Kabuki";
        case 0x0267: return "Kid Cat";
        case 0x0268: return "Monique";
        case 0x0269: return "Tabby";
        case 0x026a: return "Stinky";
        case 0x026b: return "Kitty";
        case 0x026c: return "Tom";
        case 0x026d: return "Merry";
        case 0x026e: return "Felicity";
        case 0x026f: return "Lolly";
        case 0x0270: return "Ankha";
        case 0x0271: return "Rudy";
        case 0x0272: return "Katt";
        case 0x027d: return "Bluebear";
        case 0x027e: return "Maple";
        case 0x027f: return "Poncho";
        case 0x0280: return "Pudge";
        case 0x0281: return "Kody";
        case 0x0282: return "Stitches";
        case 0x0283: return "Vladimir";
        case 0x0284: return "Murphy";
        case 0x0286: return "Olive";
        case 0x0287: return "Cheri";
        case 0x028a: return "June";
        case 0x028b: return "Pekoe";
        case 0x028c: return "Chester";
        case 0x028d: return "Barold";
        case 0x028e: return "Tammy";
        case 0x028f: return "Marty";
        case 0x0299: return "Goose";
        case 0x029a: return "Benedict";
        case 0x029b: return "Egbert";
        case 0x029e: return "Ava";
        case 0x02a2: return "Becky";
        case 0x02a3: return "Plucky";
        case 0x02a4: return "Knox";
        case 0x02a5: return "Broffina";
        case 0x02a6: return "Ken";
        case 0x02b1: return "Patty";
        case 0x02b2: return "Tipper";
        case 0x02b7: return "Norma";
        case 0x02b8: return "Naomi";
        case 0x02c3: return "Alfonso";
        case 0x02c4: return "Alli";
        case 0x02c5: return "Boots";
        case 0x02c7: return "Del";
        case 0x02c9: return "Sly";
        case 0x02ca: return "Gayle";
        case 0x02cb: return "Drago";
        case 0x02d6: return "Fauna";
        case 0x02d7: return "Bam";
        case 0x02d8: return "Zell";
        case 0x02d9: return "Bruce";
        case 0x02da: return "Deidre";
        case 0x02db: return "Lopez";
        case 0x02dc: return "Fuchsia";
        case 0x02dd: return "Beau";
        case 0x02de: return "Diana";
        case 0x02df: return "Erik";
        case 0x02e0: return "Chelsea";
        case 0x02ea: return "Goldie";
        case 0x02eb: return "Butch";
        case 0x02ec: return "Lucky";
        case 0x02ed: return "Biskit";
        case 0x02ee: return "Bones";
        case 0x02ef: return "Portia";
        case 0x02f0: return "Walker";
        case 0x02f1: return "Daisy";
        case 0x02f2: return "Cookie";
        case 0x02f3: return "Maddie";
        case 0x02f4: return "Bea";
        case 0x02f8: return "Mac";
        case 0x02f9: return "Marcel";
        case 0x02fa: return "Benjamin";
        case 0x02fb: return "Cherry";
        case 0x02fc: return "Shep";
        case 0x0307: return "Bill";
        case 0x0308: return "Joey";
        case 0x0309: return "Pate";
        case 0x030a: return "Maelle";
        case 0x030b: return "Deena";
        case 0x030c: return "Pompom";
        case 0x030d: return "Mallary";
        case 0x030e: return "Freckles";
        case 0x030f: return "Derwin";
        case 0x0310: return "Drake";
        case 0x0311: return "Scoot";
        case 0x0312: return "Weber";
        case 0x0313: return "Miranda";
        case 0x0314: return "Ketchup";
        case 0x0316: return "Gloria";
        case 0x0317: return "Molly";
        case 0x0318: return "Quillson";
        case 0x0323: return "Opal";
        case 0x0324: return "Dizzy";
        case 0x0325: return "Big Top";
        case 0x0326: return "Eloise";
        case 0x0327: return "Margie";
        case 0x0328: return "Paolo";
        case 0x0329: return "Axel";
        case 0x032a: return "Ellie";
        case 0x032c: return "Tucker";
        case 0x032d: return "Tia";
        case 0x032e: return "Chai";
        case 0x0338: return "Lily";
        case 0x0339: return "Ribbot";
        case 0x033a: return "Frobert";
        case 0x033b: return "Camofrog";
        case 0x033c: return "Drift";
        case 0x033d: return "Wart Jr.";
        case 0x033e: return "Puddies";
        case 0x033f: return "Jeremiah";
        case 0x0341: return "Tad";
        case 0x0342: return "Cousteau";
        case 0x0343: return "Huck";
        case 0x0344: return "Prince";
        case 0x0345: return "Jambette";
        case 0x0347: return "Raddle";
        case 0x0348: return "Gigi";
        case 0x0349: return "Croque";
        case 0x034a: return "Diva";
        case 0x034b: return "Henry";
        case 0x0356: return "Chevre";
        case 0x0357: return "Nan";
        case 0x0358: return "Billy";
        case 0x035a: return "Gruff";
        case 0x035c: return "Velma";
        case 0x035d: return "Kidd";
        case 0x035e: return "Pashmina";
        case 0x0369: return "Cesar";
        case 0x036a: return "Peewee";
        case 0x036b: return "Boone";
        case 0x036d: return "Louie";
        case 0x036e: return "Maddie";
        case 0x0370: return "Violet";
        case 0x0371: return "Al";
        case 0x0372: return "Rocket";
        case 0x0373: return "Hans";
        case 0x0374: return "Rilla";
        case 0x037e: return "Hamlet";
        case 0x037f: return "Apple";
        case 0x0380: return "Graham";
        case 0x0381: return "Rodney";
        case 0x0382: return "Soleil";
        case 0x0383: return "Clay";
        case 0x0384: return "Flurry";
        case 0x0385: return "Hamphrey";
        case 0x0390: return "Rocco";
        case 0x0392: return "Bubbles";
        case 0x0393: return "Bertha";
        case 0x0394: return "Biff";
        case 0x0395: return "Bitty";
        case 0x0398: return "Harry";
        case 0x0399: return "Hippeux";
        case 0x03a4: return "Buck";
        case 0x03a5: return "Victoria";
        case 0x03a6: return "Savannah";
        case 0x03a7: return "Elmer";
        case 0x03a8: return "Rosco";
        case 0x03a9: return "Winnie";
        case 0x03aa: return "Ed";
        case 0x03ab: return "Cleo";
        case 0x03ac: return "Peaches";
        case 0x03ad: return "Annalise";
        case 0x03ae: return "Clyde";
        case 0x03af: return "Colton";
        case 0x03b0: return "Papi";
        case 0x03b1: return "Julian";
        case 0x03bc: return "Yuka";
        case 0x03bd: return "Alice";
        case 0x03be: return "Melba";
        case 0x03bf: return "Sydney";
        case 0x03c0: return "Gonzo";
        case 0x03c1: return "Ozzie";
        case 0x03c4: return "Canberra";
        case 0x03c5: return "Lyman";
        case 0x03c6: return "Eugene";
        case 0x03d1: return "Kitt";
        case 0x03d2: return "Mathilda";
        case 0x03d3: return "Carrie";
        case 0x03d6: return "Astrid";
        case 0x03d7: return "Sylvia";
        case 0x03d9: return "Walt";
        case 0x03da: return "Rodney";
        case 0x03db: return "Marcie";
        case 0x03e6: return "Bud";
        case 0x03e7: return "Elvis";
        case 0x03e8: return "Rex";
        case 0x03ea: return "Leopold";
        case 0x03ec: return "Mott";
        case 0x03ed: return "Rory";
        case 0x03ee: return "Lionel";
        case 0x03fa: return "Nana";
        case 0x03fb: return "Simon";
        case 0x03fc: return "Tammi";
        case 0x03fd: return "Monty";
        case 0x03fe: return "Elise";
        case 0x03ff: return "Flip";
        case 0x0400: return "Shari";
        case 0x0401: return "Deli";
        case 0x040c: return "Dora";
        case 0x040d: return "Limberg";
        case 0x040e: return "Bella";
        case 0x040f: return "Bree";
        case 0x0410: return "Samson";
        case 0x0411: return "Rod";
        case 0x0414: return "Candi";
        case 0x0415: return "Rizzo";
        case 0x0416: return "Anicotti";
        case 0x0418: return "Broccolo";
        case 0x041a: return "Moose";
        case 0x041b: return "Bettina";
        case 0x041c: return "Greta";
        case 0x041d: return "Penelope";
        case 0x041e: return "Chadder";
        case 0x0429: return "Octavian";
        case 0x042a: return "Marina";
        case 0x042b: return "Zucker";
        case 0x0436: return "Queenie";
        case 0x0437: return "Gladys";
        case 0x0438: return "Sandy";
        case 0x0439: return "Sprocket";
        case 0x043b: return "Julia";
        case 0x043c: return "Cranston";
        case 0x043d: return "Phil";
        case 0x043e: return "Blanche";
        case 0x043f: return "Flora";
        case 0x0440: return "Phoebe";
        case 0x044b: return "Apollo";
        case 0x044c: return "Amelia";
        case 0x044d: return "Pierce";
        case 0x044e: return "Buzz";
        case 0x0450: return "Avery";
        case 0x0451: return "Frank";
        case 0x0452: return "Sterling";
        case 0x0453: return "Keaton";
        case 0x0454: return "Celia";
        case 0x045f: return "Aurora";
        case 0x0460: return "Roald";
        case 0x0461: return "Cube";
        case 0x0462: return "Hopper";
        case 0x0463: return "Friga";
        case 0x0464: return "Gwen";
        case 0x0465: return "Puck";
        case 0x0468: return "Wade";
        case 0x0469: return "Boomer";
        case 0x046a: return "Iggly";
        case 0x046b: return "Tex";
        case 0x046c: return "Flo";
        case 0x046d: return "Sprinkle";
        case 0x0478: return "Curly";
        case 0x0479: return "Truffles";
        case 0x047a: return "Rasher";
        case 0x047b: return "Hugh";
        case 0x047c: return "Lucy";
        case 0x047d: return "Spork/Crackle";
        case 0x0480: return "Cobb";
        case 0x0481: return "Boris";
        case 0x0482: return "Maggie";
        case 0x0483: return "Peggy";
        case 0x0485: return "Gala";
        case 0x0486: return "Chops";
        case 0x0487: return "Kevin";
        case 0x0488: return "Pancetti";
        case 0x0489: return "Agnes";
        case 0x0494: return "Bunnie";
        case 0x0495: return "Dotty";
        case 0x0496: return "Coco";
        case 0x0497: return "Snake";
        case 0x0498: return "Gaston";
        case 0x0499: return "Gabi";
        case 0x049a: return "Pippy";
        case 0x049b: return "Tiffany";
        case 0x049c: return "Genji";
        case 0x049d: return "Ruby";
        case 0x049e: return "Doc";
        case 0x049f: return "Claude";
        case 0x04a0: return "Francine";
        case 0x04a1: return "Chrissy";
        case 0x04a2: return "Hopkins";
        case 0x04a3: return "OHare";
        case 0x04a4: return "Carmen";
        case 0x04a5: return "Bonbon";
        case 0x04a6: return "Cole";
        case 0x04a7: return "Mira";
        case 0x04a8: return "Toby";
        case 0x04b2: return "Tank";
        case 0x04b3: return "Rhonda";
        case 0x04b4: return "Spike";
        case 0x04b6: return "Hornsby";
        case 0x04b9: return "Merengue";
        case 0x04ba: return "Renée";
        case 0x04c5: return "Vesta";
        case 0x04c6: return "Baabara";
        case 0x04c7: return "Eunice";
        case 0x04c8: return "Stella";
        case 0x04c9: return "Cashmere";
        case 0x04cc: return "Willow";
        case 0x04cd: return "Curlos";
        case 0x04ce: return "Wendy";
        case 0x04cf: return "Timbra";
        case 0x04d0: return "Frita";
        case 0x04d1: return "Muffy";
        case 0x04d2: return "Pietro";
        case 0x04d3: return "Étoile";
        case 0x04dd: return "Peanut";
        case 0x04de: return "Blaire";
        case 0x04df: return "Filbert";
        case 0x04e0: return "Pecan";
        case 0x04e1: return "Nibbles";
        case 0x04e2: return "Agent S";
        case 0x04e3: return "Caroline";
        case 0x04e4: return "Sally";
        case 0x04e5: return "Static";
        case 0x04e6: return "Mint";
        case 0x04e7: return "Ricky";
        case 0x04e8: return "Cally";
        case 0x04ea: return "Tasha";
        case 0x04eb: return "Sylvana";
        case 0x04ec: return "Poppy";
        case 0x04ed: return "Sheldon";
        case 0x04ee: return "Marshal";
        case 0x04ef: return "Hazel";
        case 0x04fa: return "Rolf";
        case 0x04fb: return "Rowan";
        case 0x04fc: return "Tybalt";
        case 0x04fd: return "Bangle";
        case 0x04fe: return "Leonardo";
        case 0x04ff: return "Claudia";
        case 0x0500: return "Bianca";
        case 0x050b: return "Chief";
        case 0x050c: return "Lobo";
        case 0x050d: return "Wolfgang";
        case 0x050e: return "Whitney";
        case 0x050f: return "Dobie";
        case 0x0510: return "Freya";
        case 0x0511: return "Fang";
        case 0x0513: return "Vivian";
        case 0x0514: return "Skye";
        case 0x0515: return "Kyle";
        case 0x0580: return "Fox";
        case 0x0581: return "Falco";
        case 0x0584: return "Wolf";
        case 0x05c0: return "Samus";
        case 0x05c1: return "Metroid";
        case 0x05c2: return "Ridley";
        case 0x0600: return "Captain Falcon";
        case 0x0640: return "Olimar";
        case 0x0642: return "Pikmin";
        case 0x06c0: return "Little Mac";
        case 0x0700: return "Wii Fit Trainer";
        case 0x0740: return "Pit";
        case 0x0741: return "Dark Pit";
        case 0x0742: return "Palutena";
        case 0x0780: return "Mr. G&W";
        case 0x0781: return "R.O.B.";
        case 0x0782: return "Duck Hunt";
        case 0x07c0: return "Mii";
        case 0x0800: return "Inkling";
        case 0x0801: return "Callie";
        case 0x0802: return "Marie";
        case 0x0803: return "Pearl";
        case 0x0804: return "Marina";
        case 0x0805: return "Octoling";
        case 0x09c0: return "Mario";
        case 0x09c1: return "Luigi";
        case 0x09c2: return "Peach";
        case 0x09c3: return "Daisy";
        case 0x09c4: return "Yoshi";
        case 0x09c5: return "Wario";
        case 0x09c6: return "Waluigi";
        case 0x09c7: return "Donkey Kong";
        case 0x09c8: return "Diddy Kong";
        case 0x09c9: return "Bowser";
        case 0x09ca: return "Bowser Jr.";
        case 0x09cb: return "Boo";
        case 0x09cc: return "Baby Mario";
        case 0x09cd: return "Baby Luigi";
        case 0x09ce: return "Birdo";
        case 0x09cf: return "Rosalina";
        case 0x09d0: return "Metal Mario";
        case 0x09d1: return "Pink Gold Peach";
        case 0x1906: return "Charizard";
        case 0x1919: return "Pikachu";
        case 0x1927: return "Jigglypuff";
        case 0x1996: return "Mewtwo";
        case 0x1ac0: return "Lucario";
        case 0x1b92: return "Greninja";
        case 0x1d00: return "Shadow Mewtwo";
        case 0x1d01: return "Detective Pikachu";
        case 0x1f00: return "Kirby";
        case 0x1f01: return "Meta Knight";
        case 0x1f02: return "King Dedede";
        case 0x1f03: return "Waddle Dee";
        case 0x1f40: return "Qbby";
        case 0x2100: return "Marth";
        case 0x2101: return "Ike";
        case 0x2102: return "Lucina";
        case 0x2103: return "Robin";
        case 0x2104: return "Roy";
        case 0x2105: return "Corrin";
        case 0x2106: return "Alm";
        case 0x2107: return "Celica";
        case 0x2108: return "Chrom";
        case 0x2109: return "Tiki";
        case 0x2240: return "Shulk";
        case 0x2280: return "Ness";
        case 0x2281: return "Lucas";
        case 0x22c0: return "Chibi-Robo";
        case 0x3200: return "Sonic";
        case 0x3240: return "Bayonetta";
        case 0x3340: return "PAC-MAN";
        case 0x3380: return "Solaire of Astora";
        case 0x3480: return "Mega Man";
        case 0x34c0: return "Ryu";
        case 0x3500: return "One-Eyed Rathalos";
        case 0x3501: return "Nabiru";
        case 0x3502: return "Rathian";
        case 0x3503: return "Barioth";
        case 0x3504: return "Qurupeco";
        case 0x35c0: return "Shovel Knight";
        case 0x3600: return "Cloud Strife";
        case 0x3740: return "Mario Cereal";
        default: return "Unknown";
    }
}
