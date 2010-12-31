#ifndef DATAJOCKEY_AUDIO_SMOKE_H
#define DATAJOCKEY_AUDIO_SMOKE_H

#include <smoke.h>
#include "/usr/share/qt4/include/QtCore/qglobal.h"
#include "/usr/include/qt4/QtCore/QRegExp"

// Defined in smokedata.cpp, initialized by init_qsci_Smoke(), used by all .cpp files
extern "C" SMOKE_EXPORT Smoke* datajockey_audio_Smoke;
extern "C" SMOKE_EXPORT void init_datajockey_audio_Smoke();
extern "C" SMOKE_EXPORT void delete_datajockey_audio_Smoke();

#ifndef QGLOBALSPACE_CLASS
#define QGLOBALSPACE_CLASS
class QGlobalSpace { };
#endif

#endif
