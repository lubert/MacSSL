#include "Menus.r"
#include "Processes.r"

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

resource 'SIZE' (-1) {
	reserved,
	acceptSuspendResumeEvents,
	reserved,
	canBackground,
	doesActivateOnFGSwitch,
	backgroundAndForeground,
	dontGetFrontClicks,
	ignoreChildDiedEvents,
	is32BitCompatible,
	notHighLevelEventAware,
	onlyLocalHLEvents,
	notStationeryAware,
	dontUseTextEditServices,
	reserved,
	reserved,
	reserved,
	4096 * 1024,    // 4MB minimum memory
	4096 * 1024     // 4MB preferred memory
};