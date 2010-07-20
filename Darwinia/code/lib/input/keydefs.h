#ifndef INCLUDED_W32KEYS_H
#define INCLUDED_W32KEYS_H

#define KEY_BACKSPACE           8
#define KEY_TAB                 9
#define KEY_ENTER              13
#define KEY_SHIFT			   16
#define KEY_CONTROL			   17
#define KEY_ALT                18
#define KEY_PAUSE              19
#define KEY_CAPSLOCK           20
#define KEY_ESC                27
#define KEY_SPACE              32
#define KEY_PGUP               33
#define KEY_PGDN               34
#define KEY_END                35
#define KEY_HOME               36
#define KEY_LEFT               37
#define KEY_UP                 38
#define KEY_RIGHT              39
#define KEY_DOWN               40
#define KEY_INSERT             45
#define KEY_DEL                46
#define KEY_0                  48
#define KEY_1                  49
#define KEY_2                  50
#define KEY_3                  51
#define KEY_4                  52
#define KEY_5                  53
#define KEY_6                  54
#define KEY_7                  55
#define KEY_8                  56
#define KEY_9                  57
#define KEY_A                  65
#define KEY_B                  66
#define KEY_C                  67
#define KEY_D                  68
#define KEY_E                  69
#define KEY_F                  70
#define KEY_G                  71
#define KEY_H                  72
#define KEY_I                  73
#define KEY_J                  74
#define KEY_K                  75
#define KEY_L                  76
#define KEY_M                  77
#define KEY_N                  78
#define KEY_O                  79
#define KEY_P                  80
#define KEY_Q                  81
#define KEY_R                  82
#define KEY_S                  83
#define KEY_T                  84
#define KEY_U                  85
#define KEY_V                  86
#define KEY_W                  87
#define KEY_X                  88
#define KEY_Y                  89
#define KEY_Z                  90
#define KEY_0_PAD              96
#define KEY_1_PAD              97
#define KEY_2_PAD              98
#define KEY_3_PAD              99
#define KEY_4_PAD             100
#define KEY_5_PAD             101
#define KEY_6_PAD             102
#define KEY_7_PAD             103
#define KEY_8_PAD             104
#define KEY_9_PAD             105
#define KEY_ASTERISK          106
#define KEY_PLUS_PAD          107
#define KEY_MINUS_PAD         109
#define KEY_DEL_PAD           110
#define KEY_SLASH_PAD         111
#define KEY_F1                112
#define KEY_F2                113
#define KEY_F3                114
#define KEY_F4                115
#define KEY_F5                116
#define KEY_F6                117
#define KEY_F7                118
#define KEY_F8                119
#define KEY_F9                120
#define KEY_F10               121
#define KEY_F11               122
#define KEY_F12               123
#define KEY_NUMLOCK           144
#define KEY_SCRLOCK           145
#define KEY_COLON             186
#define KEY_EQUALS            187
#define KEY_COMMA             188
#define KEY_MINUS             189
#define KEY_STOP              190
#define KEY_SLASH             191
#define KEY_AT                192
#define KEY_QUOTE             192
#define KEY_OPENBRACE         219
#define KEY_BACKSLASH         220
#define KEY_CLOSEBRACE        221
#define KEY_TILDE             223
#define KEY_META              224

// This is not really a key
// TODO: Any key not implemented yet
#define KEY_ANY               255

#ifndef KEY_MAX
  #define KEY_MAX               256
#endif


#endif // INCLUDED_W32KEYS_H
