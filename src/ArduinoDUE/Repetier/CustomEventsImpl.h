
#include "Repetier.h"
#include "Printer500XL.h"


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
		return Printer500XL::ParkExtruder(extruderId);
	default:
		return false;
	}
}
