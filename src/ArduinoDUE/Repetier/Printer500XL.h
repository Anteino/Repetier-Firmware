#include "Repetier.h"

class Printer500XL
{
  public:
    static void StartBedLevelWizard();
    static void MenuTest2();
    static void ParkExtruder1();
    static void ParkExtruder2();
    static bool ParkExtruder(uint8_t extruderId);
}; 
