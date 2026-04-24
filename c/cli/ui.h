#ifndef UI_CLIENT_FUNCTIONALITY
#define UI_CLIENT_FUNCTIONALITY

#define QUIT_BUTTON 'q'

enum INPUT_OPTION {
	QUIT,
	MESSAGE,
	REGEN_OPENAI,
	EXPORT,
	DEEPRESEARCH,
	STARTSERVER,
	CREATEGOAL,
};

typedef struct {
    enum INPUT_OPTION type;
    char key;
    const char* msg;
} InputOption;

InputOption options[] = {
    {QUIT, QUIT_BUTTON, "Exit client."},
    {MESSAGE, 'u', "Write user input."},
    //{REGEN_OPENAI, 'r', "Regen mocks with ChatGPT"},
    {EXPORT, 'e', "Export brain to json."},
    {DEEPRESEARCH, 'd', "Run deep research"},
    {STARTSERVER, 's', "Start HTTP Server"},
    {CREATEGOAL, 'g', "Create a new Goal"}
};
#define OPTIONS_COUNT (sizeof(options) / sizeof(InputOption))

void UIStart();

void UILoop();

void UIKill();

#endif
