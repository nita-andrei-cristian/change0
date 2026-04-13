#ifndef UI_CLIENT_FUNCTIONALITY
#define UI_CLIENT_FUNCTIONALITY

#include "../config.h"

#define OPTIONS_COUNT 7
#define QUIT_BUTTON 'q'

enum INPUT_OPTION {
	QUIT,
	CHAT,
	MESSAGE,
	MESSAGEX10,
	REGEN_OPENAI,
	EXPORT,
	DEEPRESEARCH,
};

enum INPUT_OPTION INPUT_TYPE[OPTIONS_COUNT] = {QUIT, CHAT, MESSAGE, MESSAGEX10, EXPORT, REGEN_OPENAI, DEEPRESEARCH};
const char INPUT_CHAR[OPTIONS_COUNT] = {QUIT_BUTTON, 'c', 'm', 'n', 'r', 'e', 'd'};
const char* INPUT_MSG[OPTIONS_COUNT] = {
	"Exit client.",
	"Start a chat.",
	"Mock a message.",
	("Mock " CONFIG_XSTR(DEFAULT_MOCK_NODES_COUNT) " messages."),
	"Regen mocks with ChatGPT",
	"Export brain to json.",
	"Run deep research",
};

void UIStart();

void UILoop();

void UIKill();

#endif
