
extern bool CustomMCodeHandler(GCode *com);

#undef EVENT_UNHANDLED_M_CODE

#define EVENT_UNHANDLED_M_CODE(c) CustomMCodeHandler(c)
