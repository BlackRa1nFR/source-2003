typedef enum eOSVersion
{
    eUninitialized,
    eUnknown,
    eWin9x,
    eWinNT,
};

extern void       initOSVersion();
extern eOSVersion getOSVersion();
