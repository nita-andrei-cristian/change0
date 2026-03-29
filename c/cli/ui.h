#ifndef UI_CLIENT_FUNCTIONALITY
#define UI_CLIENT_FUNCTIONALITY

#define OPTIONS_COUNT 6
#define QUIT_BUTTON 'q'

enum INPUT_OPTION {
	QUIT,
	CHAT,
	MESSAGE,
	MESSAGEX10,
	EXPORT,
	DEEPRESEARCH,
};

enum INPUT_OPTION INPUT_TYPE[OPTIONS_COUNT] = {QUIT, CHAT, MESSAGE, MESSAGEX10, EXPORT, DEEPRESEARCH};
const char INPUT_CHAR[OPTIONS_COUNT] = {QUIT_BUTTON, 'c', 'm', 'n', 'e', 'd'};
const char* INPUT_MSG[OPTIONS_COUNT] = {
	"Exit client.",
	"Start a chat.",
	"Mock a message.",
	"Mock 10 messages.",
	"Export brain to json.",
	"Run deep research",
};

void UIStart();

void UILoop();

void UIKill();

#endif
