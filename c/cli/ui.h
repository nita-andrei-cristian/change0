#ifndef UI_CLIENT_FUNCTIONALITY
#define UI_CLIENT_FUNCTIONALITY

#define OPTIONS_COUNT 5
#define QUIT_BUTTON 'q'

enum INPUT_OPTION {
	QUIT,
	CHAT,
	MESSAGE,
	MESSAGEX10,
	EXPORT,
};

enum INPUT_OPTION INPUT_TYPE[OPTIONS_COUNT] = {QUIT, CHAT, MESSAGE, MESSAGEX10, EXPORT};
const char INPUT_CHAR[OPTIONS_COUNT] = {QUIT_BUTTON, 'c', 'm', 'n', 'e'};
const char* INPUT_MSG[OPTIONS_COUNT] = {
	"Exit client.",
	"Start a chat.",
	"Mock a message.",
	"Mock 10 messages.",
	"Export brain to json."
};

void UIStart();

void UILoop();

void UIKill();

#endif
