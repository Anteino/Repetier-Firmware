#include "Repetier.h"
#include "Printer500XL.h"

void Printer500XL::StartBedLevelWizard()
{
    Com::printFLN("500XL bed level wizard start");
    // HOME Y, X
    GCode::executeFString(PSTR("G28 Y\n"));
    GCode::executeFString(PSTR("G28 X\n"));
    // Park extruder 1 and 2
    GCode::executeFString(PSTR("M700 T1\n"));
    GCode::executeFString(PSTR("M700 T2\n"));
    // Home Z and move bed down
    GCode::executeFString(PSTR("G28 Z\n"));
    GCode::executeFString(PSTR("G1 Z10\n"));
    // Fetch extruder 1 and move to level position 1
    GCode::executeFString(PSTR("G1 X0\n"));
    GCode::executeFString(PSTR("G1 X310 Y398 F4000\n"));
    //TODO: Allow user to move bed to nozzle

    Com::printFLN("500XL bed level wizard end");
}

void Printer500XL::MenuTest2() {
    Com::printFLN("500XL test 2");
    UI_STATUS_F(Com::translatedF(UI_TEXT_YES_ID));
}

void Printer500XL::ParkExtruder1() {
	Com::printFLN("Park extruder 1");
	GCode::executeFString(PSTR("G1 X574 Y496 F4000\n"));
	GCode::executeFString(PSTR("G1 Y506 F4000 S1\n"));
	GCode::executeFString(PSTR("G1 X564 F4000\n"));
	GCode::executeFString(PSTR("G1 Y0 F4000 S0\n"));
}

void Printer500XL::ParkExtruder2() {
	Com::printFLN("Park extruder 2");
	GCode::executeFString(PSTR("G1 X0 Y496 F4000\n"));
	GCode::executeFString(PSTR("G1 Y506 F4000 S1\n"));
	GCode::executeFString(PSTR("G1 X10 F4000\n"));
	GCode::executeFString(PSTR("G1 Y0 F4000 S0\n"));
}

bool Printer500XL::ParkExtruder(uint8_t extruderId) {
	switch (extruderId)
	{
	case 1: Printer500XL::ParkExtruder1();
		return true;
	case 2: Printer500XL::ParkExtruder2();
		return true;
	default:
		return false;
	}    
}