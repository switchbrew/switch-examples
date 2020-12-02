/*
** deko3d Examples - Main Menu
*/

// Sample Framework headers
#include "SampleFramework/CApplication.h"

// C++ standard library headers
#include <array>

void Example01(void);
void Example02(void);
void Example03(void);
void Example04(void);
void Example05(void);
void Example06(void);
void Example07(void);
void Example08(void);
void Example09(void);

namespace
{
    using ExampleFunc = void(*)(void);
    struct Example
    {
        ExampleFunc mainfunc;
        const char* name;
    };

    constexpr std::array Examples =
    {
        Example{ Example01, "01: Simple Setup"                                            },
        Example{ Example02, "02: Triangle"                                                },
        Example{ Example03, "03: Cube"                                                    },
        Example{ Example04, "04: Textured Cube"                                           },
        Example{ Example05, "05: Simple Tessellation"                                     },
        Example{ Example06, "06: Simple Multisampling"                                    },
        Example{ Example07, "07: Mesh Loading and Lighting (sRGB)"                        },
        Example{ Example08, "08: Deferred Shading (Multipass Rendering with Tiled Cache)" },
        Example{ Example09, "09: Simple Compute Shader (Geometry Generation)"             },
    };
}

class CMainMenu final : public CApplication
{
    static constexpr unsigned EntriesPerScreen = 39;
    static constexpr unsigned EntryPageLength = 10;

    PadState pad;

    int screenPos;
    int selectPos;

    void renderMenu()
    {
        printf("\x1b[2J\n");
        printf("  deko3d Examples\n");
        printf("  Press PLUS(+) to exit; A to select an example to run\n");
        printf("\n");
        printf("--------------------------------------------------------------------------------");
        printf("\n");

        for (unsigned i = 0; i < (Examples.size() - screenPos) && i < EntriesPerScreen; i ++)
        {
            unsigned id = screenPos+i;
            printf("  %c %s\n", id==unsigned(selectPos) ? '*' : ' ', Examples[id].name);
        }
    }

    CMainMenu() : screenPos{}, selectPos{}
    {
        consoleInit(NULL);
        renderMenu();

        padConfigureInput(1, HidNpadStyleSet_NpadStandard);
        padInitializeDefault(&pad);
        padUpdate(&pad);
    }

    ~CMainMenu()
    {
        consoleExit(NULL);
    }

    bool onFrame(u64 ns) override
    {
        int oldPos = selectPos;
        padUpdate(&pad);

        u64 kDown = padGetButtonsDown(&pad);
        if (kDown & HidNpadButton_Plus)
        {
            selectPos = -1;
            return false;
        }
        if (kDown & HidNpadButton_A)
            return false;
        if (kDown & HidNpadButton_AnyUp)
            selectPos -= 1;
        if (kDown & HidNpadButton_AnyDown)
            selectPos += 1;
        if (kDown & HidNpadButton_AnyLeft)
            selectPos -= EntryPageLength;
        if (kDown & HidNpadButton_AnyRight)
            selectPos += EntryPageLength;

        if (selectPos < 0)
            selectPos = 0;
        if (unsigned(selectPos) >= Examples.size())
            selectPos = Examples.size()-1;

        if (selectPos != oldPos)
        {
            if (selectPos < screenPos)
                screenPos = selectPos;
            else if (selectPos >= screenPos + int(EntriesPerScreen))
                screenPos = selectPos - EntriesPerScreen + 1;
            renderMenu();
        }

        consoleUpdate(NULL);
        return true;
    }

public:
    static ExampleFunc Display()
    {
        CMainMenu app;
        app.run();
        return app.selectPos >= 0 ? Examples[app.selectPos].mainfunc : nullptr;
    }
};

int main(int argc, char* argv[])
{
    for (;;)
    {
        ExampleFunc func = CMainMenu::Display();
        if (!func) break;
        func();
    }
    return 0;
}
