#ifndef UI_CLIENT_FUNCTIONALITY
#define UI_CLIENT_FUNCTIONALITY

#include "../config.h"

#define OPTIONS_COUNT 8
#define QUIT_BUTTON 'q'

enum INPUT_OPTION {
	QUIT,
	CHAT,
	MESSAGE,
	MESSAGEX10,
	REGEN_OPENAI,
	EXPORT,
	DEEPRESEARCH,
	STARTSERVER,
};

typedef struct {
    enum INPUT_OPTION type;
    char key;
    const char* msg;
} InputOption;

InputOption options[] = {
    {QUIT, QUIT_BUTTON, "Exit client."},
    {CHAT, 'c', "Start a chat."},
    {MESSAGE, 'm', "Mock a message."},
    {MESSAGEX10, 'n', "Mock ..."},
    {REGEN_OPENAI, 'r', "Regen mocks with ChatGPT"},
    {EXPORT, 'e', "Export brain to json."},
    {DEEPRESEARCH, 'd', "Run deep research"},
    {STARTSERVER, 's', "Start HTTP Server"}
};

void UIStart();

void UILoop();

void UIKill();

#endif
