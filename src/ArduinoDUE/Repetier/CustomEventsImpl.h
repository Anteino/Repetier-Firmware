#include "Repetier.h"

uint8_t currentExtruder = -1;
char command[128];

Extruder *ext;

bool EventUnhandledGCode(GCode *com)
{
    if (com->hasG())
    {
        switch (com->G)
        {
        case 28:
            if(Extruder::current->id == 1 && Printer::currentPositionSteps[X_AXIS] * Printer::invAxisStepsPerMM[X_AXIS] >= 574 + ext->xOffset * Printer::invAxisStepsPerMM[X_AXIS])
            {
              Printer::moveToReal(574.0 + ext->xOffset * Printer::invAxisStepsPerMM[X_AXIS], IGNORE_COORDINATE, IGNORE_COORDINATE, IGNORE_COORDINATE, Printer::homingFeedrate[X_AXIS]);
              Commands::waitUntilEndOfAllMoves();
              Printer::updateCurrentPosition(true);
            }
            Extruder::current = &extruder[0];
            Printer::offsetX = Extruder::current->xOffset * Printer::invAxisStepsPerMM[X_AXIS];
            if (com->hasX() || com->hasY())
            {
                GCode::executeFString(PSTR("G50028 Y\n"));
            }
            if (com->hasX())
            {
                GCode::executeFString(PSTR("G50028 X\n"));
            }
            if (com->hasZ())
            {
                GCode::executeFString(PSTR("G50028 Z\n"));
            }
            previousMillisCmd = HAL::timeInMilliseconds();
            return true;
            break;
        default:
            return false;
            break;
        }
        return false;
    }
    return true;
}

void SelectExtruder500XL(uint8_t t) {
    if (currentExtruder == t)
    {
        return;
    }

    currentExtruder = t;
    Extruder::current = &extruder[t];
    ext = &extruder[1];

    switch (t)
    {
    case 0:
//        Serial.print("Offset: ");
//        Serial.println(ext->xOffset * Printer::invAxisStepsPerMM[X_AXIS]);
        GCode::executeFString(PSTR("G1 X574 F6000\n"));
        GCode::executeFString(PSTR("G1 Y506 S1\n"));
        GCode::executeFString(PSTR("G1 X564\n"));
        GCode::executeFString(PSTR("G1 Y496\n"));
        GCode::executeFString(PSTR("G1 X0 S0\n"));
        GCode::executeFString(PSTR("G92 E0\n"));
        break;
    case 1:
//        Serial.print("Offset: ");
//        Serial.println(ext->xOffset * Printer::invAxisStepsPerMM[X_AXIS]);
        sprintf(command, "G1 X%f F6000\n", -ext->xOffset * Printer::invAxisStepsPerMM[X_AXIS]);
        GCode::executeFString(PSTR(command));
        GCode::executeFString(PSTR("G0 Y506 S1\n"));
        sprintf(command, "G1 X%f F6000\n", 10 - ext->xOffset * Printer::invAxisStepsPerMM[X_AXIS]);
        GCode::executeFString(PSTR(command));
        GCode::executeFString(PSTR("G0 Y496\n"));
        sprintf(command, "G1 X%f F6000\n", 574 - ext->xOffset * Printer::invAxisStepsPerMM[X_AXIS]);
        GCode::executeFString(PSTR(command));
        break;
    default:
        break;
    }
}
