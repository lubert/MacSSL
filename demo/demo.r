#include "Menus.r"

resource 'MENU' (4000) {
	4000,
	textMenuProc,
	0x7FFFFFFF,
	enabled,
	"HTTP Methods",
	{
		"GET", noIcon, noKey, noMark, plain,
		"POST", noIcon, noKey, noMark, plain,
		"PUT", noIcon, noKey, noMark, plain,
		"DELETE", noIcon, noKey, noMark, plain
	}
};