/*
    This file is part of Repetier-Firmware.

    Repetier-Firmware is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Repetier-Firmware is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Repetier-Firmware.  If not, see <http://www.gnu.org/licenses/>.

    This firmware is a nearly complete rewrite of the sprinter firmware
    by kliment (https://github.com/kliment/Sprinter)
    which based on Tonokip RepRap firmware rewrite based off of Hydra-mmm firmware.
*/

#include "Repetier.h"
#include <avr/dtostrf.h>
#include "floatToString.h"

#if SDSUPPORT

char tempLongFilename[LONG_FILENAME_LENGTH + 1];
char fullName[LONG_FILENAME_LENGTH * SD_MAX_FOLDER_DEPTH + SD_MAX_FOLDER_DEPTH + 1];

SDCard sd;

SDCard::SDCard()
{
    sdmode = 0;
    sdactive = false;
    savetosd = false;
    Printer::setAutomount(false);
}

void SDCard::automount()
{
#if SDCARDDETECT > -1
    if(READ(SDCARDDETECT) != SDCARDDETECTINVERTED)
    {
        if(sdactive || sdmode == 100)   // Card removed
        {
            Com::printFLN(PSTR("SD card removed"));
#if UI_DISPLAY_TYPE != NO_DISPLAY
            uid.executeAction(UI_ACTION_TOP_MENU, true);
#endif
            unmount();
            UI_STATUS_UPD_F(Com::translatedF(UI_TEXT_SD_REMOVED_ID));
        }
    }
    else
    {
        if(!sdactive && sdmode != 100)
        {
            UI_STATUS_UPD_F(Com::translatedF(UI_TEXT_SD_INSERTED_ID));
            mount();
      if(sdmode != 100) // send message only if we have success
              Com::printFLN(PSTR("SD card inserted")); // Not translatable or host will not understand signal
#if UI_DISPLAY_TYPE != NO_DISPLAY
            if(sdactive && !uid.isWizardActive()) { // Wizards have priority
                Printer::setAutomount(true);
                uid.executeAction(UI_ACTION_SD_PRINT + UI_ACTION_TOPMENU, true);
            }
#endif
        }
    }
#endif
}

void SDCard::initsd()
{
    sdactive = false;
#if SDSS > -1
#if SDCARDDETECT > -1
    if(READ(SDCARDDETECT) != SDCARDDETECTINVERTED)
        return;
#endif
  HAL::pingWatchdog();
  HAL::delayMilliseconds(50); // wait for stabilization of contacts, bootup ...
    fat.begin(SDSS, SPI_FULL_SPEED);  // dummy init of SD_CARD
    HAL::delayMilliseconds(50);       // wait for init end
  HAL::pingWatchdog();
    /*if(dir[0].isOpen())
        dir[0].close();*/
    if(!fat.begin(SDSS, SPI_FULL_SPEED))
    {
        Com::printFLN(Com::tSDInitFail);
    sdmode = 100; // prevent automount loop!
        return;
    }
    sdactive = true;
    Printer::setMenuMode(MENU_MODE_SD_MOUNTED, true);
  HAL::pingWatchdog();

    fat.chdir();
    if(selectFile("init.g", true))
    {
        startPrint();
    }
#endif
}

void SDCard::mount()
{
    sdmode = 0;
    initsd();
}

void SDCard::unmount()
{
    sdmode = 0;
    sdactive = false;
    savetosd = false;
    Printer::setAutomount(false);
    Printer::setMenuMode(MENU_MODE_SD_MOUNTED + MENU_MODE_SD_PAUSED + MENU_MODE_SD_PRINTING, false);
#if UI_DISPLAY_TYPE != NO_DISPLAY && SDSUPPORT
    uid.cwd[0] = '/';
    uid.cwd[1] = 0;
    uid.folderLevel = 0;
#endif
}

void SDCard::startPrint()
{
    if(!sdactive) return;
    sdmode = 1;
    Printer::setMenuMode(MENU_MODE_SD_PRINTING, true);
    Printer::setMenuMode(MENU_MODE_SD_PAUSED, false);
}
void SDCard::pausePrint(bool intern)
{
    if(!sd.sdactive) return;
    sdmode = 2; // finish running line
    Printer::setMenuMode(MENU_MODE_SD_PAUSED, true);
    if(intern) {
        Commands::waitUntilEndOfAllBuffers();
        sdmode = 0;
        Printer::MemoryPosition();
        Printer::moveToReal(IGNORE_COORDINATE, IGNORE_COORDINATE, IGNORE_COORDINATE,
                            Printer::memoryE - RETRACT_ON_PAUSE,
                            Printer::maxFeedrate[E_AXIS]);
        if(Extruder::current->id == 0)
        {
          Printer::moveToReal(Printer::xMin, Printer::yMin + Printer::yLength, IGNORE_COORDINATE, IGNORE_COORDINATE, Printer::maxFeedrate[X_AXIS]);
        }
        else
        {
          Printer::moveToReal(604.0, Printer::yMin + Printer::yLength, IGNORE_COORDINATE, IGNORE_COORDINATE, Printer::maxFeedrate[X_AXIS]);
        }
        Printer::lastCmdPos[X_AXIS] = Printer::currentPosition[X_AXIS];
        Printer::lastCmdPos[Y_AXIS] = Printer::currentPosition[Y_AXIS];
        Printer::lastCmdPos[Z_AXIS] = Printer::currentPosition[Z_AXIS];
        GCode::executeFString(PSTR(PAUSE_START_COMMANDS));
    }
    int actualCurrentExtruder = Extruder::current->id;
    float T_temp[2];  //  index 0 for extruder0 and 1 for extruder1
    
    Extruder::current = &extruder[0];
    T_temp[0] = Extruder::current->tempControl.targetTemperatureC;
    Extruder::current = &extruder[1];
    T_temp[1] = Extruder::current->tempControl.targetTemperatureC;
    Extruder::current = &extruder[actualCurrentExtruder];

    if(T_temp[0] > 0)
    {
      Extruder::setTemperatureForExtruder(T_temp[0] - COOLDOWN_ON_PAUSE, 0);
    }
    if(T_temp[1] > 0)
    {
      Extruder::setTemperatureForExtruder(T_temp[1] - COOLDOWN_ON_PAUSE, 1);
    }
}

void SDCard::continuePrint(bool intern)
{
    int actualCurrentExtruder = Extruder::current->id;
    float T_temp[2];  //  index 0 for extruder0 and 1 for extruder1

    Extruder::current = &extruder[0];
    T_temp[0] = Extruder::current->tempControl.targetTemperatureC;
    Extruder::current = &extruder[1];
    T_temp[1] = Extruder::current->tempControl.targetTemperatureC;
    Extruder::current = &extruder[actualCurrentExtruder];

    if(T_temp[0] > 0)
    {
      Extruder::setTemperatureForExtruder(T_temp[0] + COOLDOWN_ON_PAUSE, 0, false, true);
    }
    if(T_temp[1] > 0)
    {
      Extruder::setTemperatureForExtruder(T_temp[1] + COOLDOWN_ON_PAUSE, 1, false, true);
    }
    
    if(!sd.sdactive) return;
    if(intern) {
        GCode::executeFString(PSTR(PAUSE_END_COMMANDS));
        Printer::GoToMemoryPosition(true, true, false, false, Printer::maxFeedrate[X_AXIS]);
        Printer::GoToMemoryPosition(false, false, true, false, Printer::maxFeedrate[Z_AXIS] / 2.0f);
        Printer::GoToMemoryPosition(false, false, false, true, Printer::maxFeedrate[E_AXIS] / 2.0f);
    }
    Printer::setMenuMode(MENU_MODE_SD_PAUSED, false);
    sdmode = 1;
}

void SDCard::stopPrint(bool keepHeat)
{
    if(!sd.sdactive) return;
    if(sdmode)
        Com::printFLN(PSTR("SD print stopped by user."));
    sdmode = 0;
    Printer::setMenuMode(MENU_MODE_SD_PRINTING,false);
    Printer::setMenuMode(MENU_MODE_SD_PAUSED,false);
    GCode::executeFString(PSTR(SD_RUN_ON_STOP));
    Serial.println("Not dead yet.");
    Printer::updateCurrentPosition(true);
    Printer::moveTo(IGNORE_COORDINATE, IGNORE_COORDINATE, IGNORE_COORDINATE, IGNORE_COORDINATE, Printer::homingFeedrate[X_AXIS]);
    Printer::homeAxis(true, true, false);
    Serial.println("Still not dead yet.");
//    Commands::waitUntilEndOfAllMoves();
    if(!keepHeat)
    {
        Extruder::setTemperatureForExtruder(0, 0);
        Extruder::setTemperatureForExtruder(0, 1);
        Extruder::setHeatedBedTemperature(0);
    }
    Serial.println("this can't be happening");
}

//  This function stops the current print and creates a resume.g. Function was first created
//  by TripodMaker and edited by Anteino. Inspired by Gemma.
void SDCard::saveStopPrint(bool keepHeat)
{
  SdBaseFile parent;
  parent = *fat.vwd();

  this->createResumeGemma();

  float file_open = file.open(&parent, "resume.g", O_WRITE);
  Com::printFLN(PSTR("file_open_success: "), file_open);
  file.truncate(0);

  char msg[128];
  char buff[25];
  
  char bufX[11];
  char bufY[11];
  char bufZ[11];
  char bufE[11];
  char bufCoord[50];

  dtostrf(Printer::realXPosition(),1,3,bufX);
  dtostrf(Printer::realYPosition(),1,3,bufY);
  dtostrf(Printer::realZPosition(),1,3,bufZ);
  dtostrf(Printer::currentPositionSteps[E_AXIS]/Printer::axisStepsPerMM[E_AXIS],1,3,bufE);
  
  strcpy(bufCoord,"G1 X");
  strcat(bufCoord,bufX);
  strcat(bufCoord," Y");
  strcat(bufCoord,bufY);
  strcat(bufCoord," F4000\nG92 Z");
  strcat(bufCoord,bufZ);
  strcat(bufCoord,"\n");

  int actualCurrentExtruder = Extruder::current->id;
  float T_temp[2],  //  index 0 for extruder0 and 1 for extruder1
        B_temp;
  
  Extruder::current = &extruder[0];
  T_temp[0] = Extruder::current->tempControl.targetTemperatureC;
  Extruder::current = &extruder[1];
  T_temp[1] = Extruder::current->tempControl.targetTemperatureC;
  Extruder::current = &extruder[actualCurrentExtruder];
  B_temp = heatedBedController.targetTemperatureC;

  for(int i = 0; i < 2; i++)
  {
    sprintf(msg, "M104 T%i S%s\n", i, floatToString(buff, T_temp[i], 2));
    file.write(msg);
  }
  sprintf(msg, "M190 S%s\n", floatToString(buff, B_temp, 2));
  file.write(msg);
  for(int i = 0; i < 2; i++)
  {
    if(T_temp[i] > 0)
    {
      sprintf(msg, "M109 T%i S%s\n", i, floatToString(buff, T_temp[i], 2));
      file.write(msg);
    }
  }
  file.write("G28 XY\n");
  sprintf(msg, "T%d\n", Extruder::current->id);
  file.write(msg);
  sprintf(msg, "M106 S%d\n", Printer::getFanSpeed());
  file.write(msg);
  sprintf(msg, "G92 E0\nG1 E13.5 F F150\nG92 E%s\n", floatToString(buff, Printer::currentPositionSteps[E_AXIS]/Printer::axisStepsPerMM[E_AXIS], 2));
  file.write(msg);
  file.write(bufCoord);
  sprintf(msg, "M23 %s\n", Printer::current_filename);
  file.write(msg);
  sprintf(msg, "M26 S%lu\n", (unsigned long)sdpos);
  file.write(msg);
//  file.write("M24\n");

  file.sync();
  file.close();

  Printer::updateCurrentPosition(true);
  Printer::moveTo(IGNORE_COORDINATE, IGNORE_COORDINATE, IGNORE_COORDINATE, Printer::currentPositionSteps[E_AXIS] / Printer::axisStepsPerMM[E_AXIS] - 10.0, Printer::maxFeedrate[E_AXIS]);

  Serial.println("Stopping print");
  this->stopPrint(keepHeat);
  Serial.println("Print actually stopped");
//  if(keepHeat == false)
//  {
//    Printer::kill(false);
//  }
}

//  Creates the resume.g file needed for saving before stopping a print. Function name
//  inspired by Gemma.
void SDCard::createResumeGemma()
{
  SdBaseFile parent;
  file.sync();
  file.close();
  parent = *fat.vwd();
  char nameSav[15];
  strcpy(nameSav, "resume.g");
  bool nameSavCheck = file.exists(nameSav);
  Com::printFLN(PSTR("name'SavCheck boolean "), nameSavCheck);
  if(!nameSavCheck)
  {
    Com::printFLN(PSTR("File does not exist yet."));
    file.createContiguous(&parent, nameSav, 1);
  }
  else
  {
    Com::printFLN(PSTR("File already exists."));
  }
  file.close();
}

/*
 * Anteino 2-2-2018:  This function reads the resume.g file and prints it to the screen
 * before executing the contents. It is needed to have this as a function on the LCD
 * because you can't open another GCode file while currently having one opened. This
 * function will also be called upon sending M50024. This function will however only read
 * the first 1024 characters of the file, which should be enough for any resume.g
 */
void SDCard::resumeG_resume()
{
	SdBaseFile parent;
	parent = *fat.vwd();
  file.close();
	float file_open = file.open(&parent, "resume.g", O_READ);
  Com::printFLN(PSTR("file_open_success: "), file_open);
  file.truncate(0);
	Serial.println("Resuming print.");
	if(file.isOpen())
	{
    char buffer_[1024];
    if(file.available())
    {
      file.read(buffer_, 1024);
    }
    file.close();
    Serial.write(buffer_);
    GCode::executeFString(PSTR(buffer_));
	}
	else
	{
	  Serial.println("Opening resume.g failed");
	}
}

void SDCard::writeCommand(GCode *code)
{
    unsigned int sum1 = 0, sum2 = 0; // for fletcher-16 checksum
    uint8_t buf[100];
    uint8_t p = 2;
    file.writeError = false;
    uint16_t params = 128 | (code->params & ~1);
    memcopy2(buf,&params);
    //*(int*)buf = params;
    if(code->isV2())   // Read G,M as 16 bit value
    {
        memcopy2(&buf[p],&code->params2);
        //*(int*)&buf[p] = code->params2;
        p += 2;
        if(code->hasString())
            buf[p++] = strlen(code->text);
        if(code->hasM())
        {
            memcopy2(&buf[p],&code->M);
            //*(int*)&buf[p] = code->M;
            p += 2;
        }
        if(code->hasG())
        {
            memcopy2(&buf[p],&code->G);
            //*(int*)&buf[p]= code->G;
            p += 2;
        }
    }
    else
    {
        if(code->hasM())
        {
            buf[p++] = (uint8_t)code->M;
        }
        if(code->hasG())
        {
            buf[p++] = (uint8_t)code->G;
        }
    }
    if(code->hasX())
    {
        memcopy4(&buf[p],&code->X);
        //*(float*)&buf[p] = code->X;
        p += 4;
    }
    if(code->hasY())
    {
        memcopy4(&buf[p],&code->Y);
        //*(float*)&buf[p] = code->Y;
        p += 4;
    }
    if(code->hasZ())
    {
        memcopy4(&buf[p],&code->Z);
        //*(float*)&buf[p] = code->Z;
        p += 4;
    }
    if(code->hasE())
    {
        memcopy4(&buf[p],&code->E);
        //*(float*)&buf[p] = code->E;
        p += 4;
    }
    if(code->hasF())
    {
        memcopy4(&buf[p],&code->F);
        //*(float*)&buf[p] = code->F;
        p += 4;
    }
    if(code->hasT())
    {
        buf[p++] = code->T;
    }
    if(code->hasS())
    {
        memcopy4(&buf[p],&code->S);
        //*(int32_t*)&buf[p] = code->S;
        p += 4;
    }
    if(code->hasP())
    {
        memcopy4(&buf[p],&code->P);
        //*(int32_t*)&buf[p] = code->P;
        p += 4;
    }
    if(code->hasI())
    {
        memcopy4(&buf[p],&code->I);
        //*(float*)&buf[p] = code->I;
        p += 4;
    }
    if(code->hasJ())
    {
        memcopy4(&buf[p],&code->J);
        //*(float*)&buf[p] = code->J;
        p += 4;
    }
    if(code->hasR())
    {
        memcopy4(&buf[p],&code->R);
        //*(float*)&buf[p] = code->R;
        p += 4;
    }
    if(code->hasD())
    {
        memcopy4(&buf[p],&code->D);
        //*(float*)&buf[p] = code->D;
        p += 4;
    }
    if(code->hasC())
    {
        memcopy4(&buf[p],&code->C);
        //*(float*)&buf[p] = code->C;
        p += 4;
    }
    if(code->hasH())
    {
        memcopy4(&buf[p],&code->H);
        //*(float*)&buf[p] = code->H;
        p += 4;
    }
    if(code->hasA())
    {
        memcopy4(&buf[p],&code->A);
        //*(float*)&buf[p] = code->A;
        p += 4;
    }
    if(code->hasB())
    {
        memcopy4(&buf[p],&code->B);
        //*(float*)&buf[p] = code->B;
        p += 4;
    }
    if(code->hasK())
    {
        memcopy4(&buf[p],&code->K);
        //*(float*)&buf[p] = code->K;
        p += 4;
    }
    if(code->hasL())
    {
        memcopy4(&buf[p],&code->L);
        //*(float*)&buf[p] = code->L;
        p += 4;
    }
    if(code->hasO())
    {
        memcopy4(&buf[p],&code->O);
        //*(float*)&buf[p] = code->O;
        p += 4;
    }
    if(code->hasString())   // read 16 uint8_t into string
    {
        char *sp = code->text;
        if(code->isV2())
        {
            uint8_t i = strlen(code->text);
            for(; i; i--) buf[p++] = *sp++;
        }
        else
        {
            for(uint8_t i = 0; i < 16; ++i) buf[p++] = *sp++;
        }
    }
    uint8_t *ptr = buf;
    uint8_t len = p;
    while (len)
    {
        uint8_t tlen = len > 21 ? 21 : len;
        len -= tlen;
        do
        {
            sum1 += *ptr++;
            if(sum1 >= 255) sum1 -= 255;
            sum2 += sum1;
            if(sum2 >= 255) sum2 -= 255;
        }
        while (--tlen);
    }
    buf[p++] = sum1;
    buf[p++] = sum2;
    // Debug
    /*Com::printF(PSTR("Buf: "));
    for(int i=0;i<p;i++)
    Com::printF(PSTR(" "),(int)buf[i]);
    Com::println();*/
    if(params == 128)
    {
        Com::printErrorFLN(Com::tAPIDFinished);
    }
    else
        file.write(buf,p);
    if (file.writeError)
    {
        Com::printFLN(Com::tErrorWritingToFile);
    }
}

void SDCard::saveCurrentPosition()
{
  sdpos_saved = sdpos;
  Serial.println("Saving current position: " + String(sdpos) + ".");
}

char *SDCard::createFilename(char *buffer,const dir_t &p)
{
    char *pos = buffer,*src = (char*)p.name;
    for (uint8_t i = 0; i < 11; i++,src++)
    {
        if (*src == ' ') continue;
        if (i == 8)
            *pos++ = '.';
        *pos++ = *src;
    }
    *pos = 0;
    return pos;
}

bool SDCard::showFilename(const uint8_t *name)
{
    if (*name == DIR_NAME_DELETED || *name == '.') return false;
    return true;
}

int8_t RFstricmp(const char* s1, const char* s2)
{
    while(*s1 && (tolower(*s1) == tolower(*s2)))
        s1++,s2++;
    return (const uint8_t)tolower(*s1)-(const uint8_t)tolower(*s2);
}

int8_t RFstrnicmp(const char* s1, const char* s2, size_t n)
{
    while(n--)
    {
        if(tolower(*s1)!=tolower(*s2))
            return (uint8_t)tolower(*s1) - (uint8_t)tolower(*s2);
        s1++;
        s2++;
    }
    return 0;
}

void SDCard::ls()
{
    SdBaseFile file;

    Com::printFLN(Com::tBeginFileList);
    fat.chdir();

    file.openRoot(fat.vol());
    file.ls(0, 0);
    Com::printFLN(Com::tEndFileList);
}

#if JSON_OUTPUT
void SDCard::lsJSON(const char *filename)
{
    SdBaseFile dir;
    fat.chdir();
    if (*filename == 0) {
        dir.openRoot(fat.vol());
    } else {
        if (!dir.open(fat.vwd(), filename, O_READ) || !dir.isDir()) {
            Com::printF(Com::tJSONErrorStart);
            Com::printF(Com::tFileOpenFailed);
            Com::printFLN(Com::tJSONErrorEnd);
            return;
        }
    }

    Com::printF(Com::tJSONDir);
    SDCard::printEscapeChars(filename);
    Com::printF(Com::tJSONFiles);
    dir.lsJSON();
    Com::printFLN(Com::tJSONArrayEnd);
}

void SDCard::printEscapeChars(const char *s) {
    for (unsigned int i = 0; i < strlen(s); ++i) {
        switch (s[i]) {
            case '"':
            case '/':
            case '\b':
            case '\f':
            case '\n':
            case '\r':
            case '\t':
            case '\\':
                Com::print('\\');
                break;
        }
        Com::print(s[i]);
    }
}

void SDCard::JSONFileInfo(const char* filename) {
    SdBaseFile targetFile;
    GCodeFileInfo *info,tmpInfo;
    if (strlen(filename) == 0)  {
        targetFile = file;
        info = &fileInfo;
    } else {
        if (!targetFile.open(fat.vwd(), filename, O_READ) || targetFile.isDir()) {
            Com::printF(Com::tJSONErrorStart);
            Com::printF(Com::tFileOpenFailed);
            Com::printFLN(Com::tJSONErrorEnd);
            return;
        }
        info = &tmpInfo;
        info->init(targetFile);
    }
    if (!targetFile.isOpen()) {
        Com::printF(Com::tJSONErrorStart);
        Com::printF(Com::tNotSDPrinting);
        Com::printFLN(Com::tJSONErrorEnd);
        return;
    }

    // {"err":0,"size":457574,"height":4.00,"layerHeight":0.25,"filament":[6556.3],"generatedBy":"Slic3r 1.1.7 on 2014-11-09 at 17:11:32"}
    Com::printF(Com::tJSONFileInfoStart);
    Com::print(info->fileSize);
    Com::printF(Com::tJSONFileInfoHeight);
    Com::print(info->objectHeight);
    Com::printF(Com::tJSONFileInfoLayerHeight);
    Com::print(info->layerHeight);
    Com::printF(Com::tJSONFileInfoFilament);
    Com::print(info->filamentNeeded);
    Com::printF(Com::tJSONFileInfoGeneratedBy);
    Com::print(info->generatedBy);
    Com::print('"');
    if (strlen(filename) == 0) {
        Com::printF(Com::tJSONFileInfoName);
        file.printName();
        Com::print('"');
    }
    Com::print('}');
    Com::println();
};

#endif

/* Anteino 4-1-2018: This function checks if the char array and the string are equal for at
 *  least the length of the string. Thereby resume.g and resume.gcode can both be compared
 *  to resume.g
 */
bool compareCharString(const char *charArray, String string_)
{
  for(int i = 0; i < string_.length(); i++)
  {
    if(charArray[i] != string_[i])
    {
      return false;
    }
  }
  return true;
}

bool SDCard::selectFile(const char *filename, bool silent)
{
    for(int i=0;i<(LONG_FILENAME_LENGTH+1);i++)
    {
      Printer::current_filename[i] = filename[i];
    }
    SdBaseFile parent;
    const char *oldP = filename;

    if(compareCharString(filename, "RESUME.G"))
    {
      Com::printFLN(PSTR("RESUME.G file detected, starting resume script."));
      GCode::executeFString(PSTR("M50024\n"));
      sdpos = 0;
      filesize = file.fileSize();
      return true;
    }

    if(!sdactive) return false;
    sdmode = 0;

    file.close();

    parent = *fat.vwd();
    if (file.open(&parent, filename, O_READ))
      {
      if ((oldP = strrchr(filename, '/')) != NULL)
          oldP++;
      else
          oldP = filename;

        if(!silent)
        {
            Com::printF(Com::tFileOpened, oldP);
            Com::printFLN(Com::tSpaceSizeColon,file.fileSize());
        }
#if JSON_OUTPUT
        fileInfo.init(file);
#endif
        sdpos = 0;
        filesize = file.fileSize();
        Com::printFLN(Com::tFileSelected);
        return true;
    }
    else
    {
        if(!silent)
            Com::printFLN(Com::tFileOpenFailed);
        return false;
    }
}

void SDCard::printStatus()
{
    if(sdactive)
    {
        Com::printF(Com::tSDPrintingByte, sdpos);
        Com::printFLN(Com::tSlash, filesize);
    }
    else
    {
        Com::printFLN(Com::tNotSDPrinting);
    }
}

void SDCard::startWrite(char *filename)
{
    if(!sdactive) return;
    file.close();
    sdmode = 0;
    fat.chdir();
    if(!file.open(filename, O_CREAT | O_APPEND | O_WRITE | O_TRUNC))
    {
        Com::printFLN(Com::tOpenFailedFile,filename);
    }
    else
    {
        UI_STATUS_F(Com::translatedF(UI_TEXT_UPLOADING_ID));
        savetosd = true;
        Com::printFLN(Com::tWritingToFile,filename);
    }
}

void SDCard::finishWrite()
{
    if(!savetosd) return; // already closed or never opened
    file.sync();
    file.close();
    savetosd = false;
    Com::printFLN(Com::tDoneSavingFile);
    UI_CLEAR_STATUS;
}

void SDCard::deleteFile(char *filename)
{
    if(!sdactive) return;
    sdmode = 0;
    file.close();
    if(fat.remove(filename))
    {
        Com::printFLN(Com::tFileDeleted);
    }
    else
    {
        if(fat.rmdir(filename))
            Com::printFLN(Com::tFileDeleted);
        else
            Com::printFLN(Com::tDeletionFailed);
    }
}

void SDCard::makeDirectory(char *filename)
{
    if(!sdactive) return;
    sdmode = 0;
    file.close();
    if(fat.mkdir(filename))
    {
        Com::printFLN(Com::tDirectoryCreated);
    }
    else
    {
        Com::printFLN(Com::tCreationFailed);
    }
}

#ifdef GLENN_DEBUG
void SDCard::writeToFile()
{
  size_t nbyte;
  char szName[10];

  strcpy(szName, "Testing\r\n");
  nbyte = file.write(szName, strlen(szName));
  Com::print("L=");
  Com::print((long)nbyte);
  Com::println();
}

#endif

#endif

