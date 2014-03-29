// Alits-Life-Hive.cpp : Definiert die exportierten Funktionen für die DLL-Anwendung.
//

#include "stdafx.h"
#include "HiveLib\HiveLib.hpp"

HiveLib *HiveLibrary;

extern "C" {
	__declspec (dllexport) void __stdcall RVExtension(char *output, int outputSize, const char *function);
}

void __stdcall RVExtension(char *output, int outputSize, const char *function) {
//	strncpy_s(output, outputSize, "IT WORKS!", _TRUNCATE);
//	if (function == 100) {
//		HiveLibrary->getPlayer(123456789);
//	}
//	else if (function == 0) {
		HiveLibrary = new HiveLib();
//	}
}