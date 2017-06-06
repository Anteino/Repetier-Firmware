
#include "Repetier.h"

inline void ParkExtruder1() {
	//TODO:RM: Move positions to config
	Com::printFLN("Park extruder 1");
	GCode::executeFString(PSTR("G1 X574 Y496 F4000\n"));
	GCode::executeFString(PSTR("G1 Y506 F4000 S1\n"));
	GCode::executeFString(PSTR("G1 X564 F4000\n"));
	GCode::executeFString(PSTR("G1 Y0 F4000 S0\n"));
}

inline void ParkExtruder2() {
	//TODO:RM: Move positions to config
	Com::printFLN("Park extruder 2");
	GCode::executeFString(PSTR("G1 X0 Y496 F4000\n"));
	GCode::executeFString(PSTR("G1 Y506 F4000 S1\n"));
	GCode::executeFString(PSTR("G1 X10 F4000\n"));
	GCode::executeFString(PSTR("G1 Y0 F4000 S0\n"));
}

bool ParkExtruder(uint8_t extruderId) {
	switch (extruderId)
	{
	case 1: ParkExtruder1();
		return true;
	case 2: ParkExtruder2();
		return true;
	default:
		return false;
	}
}

inline uint8_t GetExtruderId(GCode *com) {
	if (com->hasT() && com->T > 0 && com->T <= NUM_EXTRUDER)
		return com->T;
	return 0;
}

bool CustomMCodeHandler(GCode *com)
{
  uint8_t extruderId;
  
	switch (com->M)
	{
	case 700:
		// Example. To park extruder 1: M700 T1
		extruderId = GetExtruderId(com);
		if (extruderId == 0)
			return false;
		return ParkExtruder(extruderId);
	default:
		return false;
	}
}
