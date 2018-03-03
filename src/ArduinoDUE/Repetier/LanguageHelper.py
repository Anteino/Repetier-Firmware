import wx
import sys
reload(sys)
sys.setdefaultencoding('utf8')

SPACER = 30
AMOUNT_ENTRIES = 500
WINDOW_SIZE = (1200, 800)
LANGUAGES = ["EN", "NL", "DE"]
LABEL_WIDTH = 200
MARGIN = 10
LABEL_HEIGHT = 20

class LanguageHelper(wx.Frame):
	def __init__(self, parent, title):
		wx.Frame.__init__(self, None, -1, title, size = WINDOW_SIZE)
		self.panel = wx.ScrolledWindow(self, -1)
		self.panel.SetScrollbars(0, 1, WINDOW_SIZE[0], (AMOUNT_ENTRIES + 1) * SPACER)
		self.saveButton = wx.Button(self.panel, -1, "Save", pos = ((len(LANGUAGES) + 2) * (MARGIN + LABEL_WIDTH) + MARGIN, MARGIN))
		self.saveButton.Bind(wx.EVT_BUTTON, self.saveButton_clicked)
		
		self.languageEntries = []
		
		self.openUiLangRead()
		self.importLanguageEntries()
		self.showLanguageEntries()
		
		self.Show()
	
	def openUiLangRead(self):
		self.uilangh = open("uilang.h", "r")
		self.uilanghText = self.uilangh.read()
		self.uilanghTextLength = len(self.uilanghText)
		self.uilangh.close()
		
		self.uilangcpp = open("uilang.cpp", "r")
		self.uilangcppText = self.uilangcpp.read()
		self.uilangcppTextLength = len(self.uilangcppText)
		self.uilangcpp.close()
		
	def openUiLangWrite(self):
		self.uilangh = open("uilang.h", "w")
		self.uilangcpp = open("uilang.cpp", "w")
		self.uilangh.write(self.uilanghText)
		self.uilangcpp.write(self.uilangcppText)
		self.uilangh.close()
		self.uilangcpp.close()
	
	def importLanguageEntries(self):
		index = 0
		indexEnter = 0
		indexID = 0
		self.count = 0
		
		while(True):
			index = self.uilanghText.find("#define UI_TEXT_", index, self.uilanghTextLength)
			if( (index < 0) | (index >= self.uilanghTextLength - 1) ):
				break
				
			indexEnter = self.uilanghText.find("\n", index, self.uilanghTextLength)
			indexID = self.uilanghText.find("_ID	", index, indexEnter)
			
			if( (indexID <= index) | (indexID >= indexEnter) ):
				index = indexEnter + 1
				continue
			
			remark = self.extractLanguageEntryRemark(index, indexEnter)
			indexUI = self.uilanghText.find("UI_TEXT", index, indexEnter)
			name = self.uilanghText[indexUI:indexID + 3]
			
			values = []
			for i in range(0, len(LANGUAGES)):
				tmp = "{}{}".format(name[0:len(name) - 2], LANGUAGES[i])
				value = self.findLanguageValueEntry(tmp, indexEnter + 1)
				values.append([LANGUAGES[i], value])
				
			self.languageEntries.append(LanguageEntry(name, remark, values))
			self.count = self.count + 1
			
			index = indexEnter + 1
		
		values = []
		for i in range(0, len(LANGUAGES)):
			values.append([LANGUAGES[i], ""])
		for i in range(self.count, AMOUNT_ENTRIES):
			self.languageEntries.append(LanguageEntry("", "", values))
	
	def showLanguageEntries(self):
		nameLabel = wx.StaticText(self.panel, -1, "Name id of text element", pos = (MARGIN, MARGIN), size = (LABEL_WIDTH, LABEL_HEIGHT))
		remarkLabel = wx.StaticText(self.panel, -1, "Remark", pos = (2 * MARGIN + LABEL_WIDTH, MARGIN), size = (LABEL_WIDTH, LABEL_HEIGHT))
		for i in range(0, len(LANGUAGES)):
			wx.StaticText(self.panel, -1, LANGUAGES[i], pos = ((3 + i) * MARGIN + (i + 2) * LABEL_WIDTH, MARGIN), size = (LABEL_WIDTH, LABEL_HEIGHT))
		for i in range(0, AMOUNT_ENTRIES):
			self.languageEntries[i].showEntry(i, self.panel)
	
	def extractLanguageEntryRemark(self, index, indexEnter):
		indexBackslash = self.uilanghText.find("//", index, indexEnter)
		
		if ( (indexBackslash <= index) | (indexBackslash >= indexEnter) ):
			return ""
		
		textIndex = indexBackslash + 2
		while(self.uilanghText[textIndex] == ' '):
			textIndex = textIndex + 1
		
		return self.uilanghText[textIndex:indexEnter]
	
	def findLanguageValueEntry(self, valueName, startIndex):
		index = startIndex
		while True:
			index = self.uilanghText.find(valueName, index, self.uilangcppTextLength)
			
			nextChar = ord(self.uilanghText[index + len(valueName)])
			if( ( (nextChar >= 65) & (nextChar <= 90) ) | ( (nextChar >= 97) & (nextChar <= 122) ) ):
				print "char: {}, sentence: {}".format(self.uilanghText[index + len(valueName)], self.uilanghText[index:index + len(valueName) + 2])
				index = index + len(valueName) + 1
				continue
			
			if( (index <= startIndex) | (index >= self.uilanghTextLength - 1) ):
				return ""
				
			indexEnter = self.uilanghText.find("\n", index, self.uilanghTextLength)
			
			firstIndex = -1
			lastIndex = -1
			for i in range(index + len(valueName), indexEnter):
				if( (self.uilanghText[i] != " ") & (self.uilanghText[i] != "\t") ):
					firstIndex = i
					break
			
			return self.uilanghText[firstIndex:indexEnter]
	
	def saveButton_clicked(self, event):
		self.updateEntries()
		self.setNumTranslatedWords()
		self.writeTextEntries()
		self.openUiLangWrite()
	
	def updateEntries(self):
		self.count = 0
		
		for i in range(0, AMOUNT_ENTRIES):
			self.languageEntries[i].name = self.languageEntries[i].nameLabel.GetLineText(0)
			self.languageEntries[i].nameNoId = self.languageEntries[i].name[0:len(self.languageEntries[i].name) - 2]
			self.languageEntries[i].remark = self.languageEntries[i].remarkLabel.GetLineText(0)
			for j in range(0, len(LANGUAGES)):
				self.languageEntries[i].values[j][1] = self.languageEntries[i].languageLabels[j].GetLineText(0)
			
			if(len(self.languageEntries[i].name) != 0):
				self.count = self.count + 1
	
	def setNumTranslatedWords(self):
		index = self.uilanghText.find("NUM_TRANSLATED_WORDS", 0, self.uilanghTextLength)
		indexEnter = self.uilanghText.find("\n", index, self.uilanghTextLength)
		self.uilanghText = self.uilanghText[0:index + len("NUM_TRANSLATED_WORDS")] + " {}".format(self.count) + self.uilanghText[indexEnter:self.uilanghTextLength]
		self.uilanghTextLength = len(self.uilanghText)
	
	def writeTextEntries(self):
		startIndex = self.uilanghText.find("//	Start of text elements", 0, self.uilanghTextLength)
		startIndex = self.uilanghText.find("\n", startIndex, self.uilanghTextLength)
		endIndex = self.uilanghText.find("//	End of text elements", startIndex, self.uilanghTextLength)
				
		textEntries = "\n"
		for i in range(0, AMOUNT_ENTRIES):
			if(len(self.languageEntries[i].name) != 0):
				textEntries = textEntries + "#define {}	{}  //  {}\n".format(self.languageEntries[i].name, i, self.languageEntries[i].remark)
		
		self.uilanghText = self.uilanghText[0:startIndex] + textEntries + self.uilanghText[endIndex:self.uilanghTextLength]
		self.uilanghTextLength = len(self.uilanghText)
		
		for i in range(0, len(LANGUAGES)):
			language = LANGUAGES[i]
			startIndex = self.uilanghText.find("//	Start of {} translations".format(language), 0, self.uilanghTextLength)
			startIndex = self.uilanghText.find("\n", startIndex, self.uilanghTextLength)
			endIndex = self.uilanghText.find("//	End of {} translations".format(language), startIndex, self.uilanghTextLength)
		
			textEntries = "\n"
			for j in range(0, AMOUNT_ENTRIES):
				if(len(self.languageEntries[j].name) != 0):
					if(len(self.languageEntries[j].values[i][1]) == 0):
						self.languageEntries[j].values[i][1] = "\"-\""
					textEntries = textEntries + "#define {}{}	{}\n".format(self.languageEntries[j].nameNoId, language, self.languageEntries[j].values[i][1].decode('utf-8'))
		
			self.uilanghText = self.uilanghText[0:startIndex] + textEntries + self.uilanghText[endIndex:self.uilanghTextLength]
			self.uilanghTextLength = len(self.uilanghText)
			
			startIndex = self.uilangcppText.find("#if LANGUAGE_{}_ACTIVE\n".format(language), 0, self.uilangcppTextLength)
			startIndex = self.uilangcppText.find("\n", startIndex, self.uilangcppTextLength)
			endIndex = self.uilangcppText.find("#endif", startIndex, self.uilangcppTextLength)
			
			textEntries = "\n"
			for j in range(0, AMOUNT_ENTRIES):
				if(len(self.languageEntries[j].name) != 0):
					textEntries = textEntries + "TRANS({}{});\n".format(self.languageEntries[j].nameNoId, language)
			textEntries = textEntries + "\nPGM_P const translations_{}[NUM_TRANSLATED_WORDS] PROGMEM = {{\n".format(language)
			for j in range(0, AMOUNT_ENTRIES):
				if(len(self.languageEntries[j].name) != 0):
					textEntries = textEntries + "	F{}{},\n".format(self.languageEntries[j].nameNoId, language)
			textEntries = textEntries + "}};\n#define LANG_{}_TABLE translations_{}\n#else\n#define LANG_{}_TABLE NULL\n".format(language, language, language)
			
			self.uilangcppText = self.uilangcppText[0:startIndex] + textEntries + self.uilangcppText[endIndex:self.uilangcppTextLength]
			self.uilangcppTextLength = len(self.uilangcppText)

class LanguageEntry:
	def __init__(self, name, remark, values):
		self.name = name
		self.nameNoId = name[0:len(name) - 2]
		self.remark = remark
		self.values = values
	
	def showEntry(self, y, panel):
		self.nameLabel = wx.TextCtrl(panel, -1, self.name, pos = (MARGIN, MARGIN + (y + 1) * SPACER), size = (LABEL_WIDTH, LABEL_HEIGHT))
		self.remarkLabel = wx.TextCtrl(panel, -1, self.remark, pos = (2 * MARGIN + LABEL_WIDTH, MARGIN + (y + 1) * SPACER), size = (LABEL_WIDTH, LABEL_HEIGHT))
		self.languageLabels = []
		for i in range(0, len(LANGUAGES)):
			self.languageLabels.append(wx.TextCtrl(panel, -1, self.values[i][1], pos = ((3 + i) * MARGIN + (i + 2) * LABEL_WIDTH, MARGIN + (y + 1) * SPACER), size = (LABEL_WIDTH, LABEL_HEIGHT)))

gui = wx.App()
LanguageHelper(None, 'Language helper')
gui.MainLoop()