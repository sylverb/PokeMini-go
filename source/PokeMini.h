/*
  PokeMini - Pok�mon-Mini Emulator
  Copyright (C) 2009-2015  JustBurn

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef POKEMINI_EMU
#define POKEMINI_EMU

#include <stdint.h>
#include <stdlib.h>
#ifndef TARGET_GNW
#include <libretro.h>
#include <retro_inline.h>
#include <streams/memory_stream.h>
#else
#include <stdio.h>
#define memstream_t FILE
#define memstream_read(stream, buffer, size) fread(buffer, 1, size, stream)
#define memstream_write(stream, buffer, size) fwrite(buffer, 1, size, stream)
#define memstream_seek(stream, offset, whence) fseek(stream, offset, whence)
#endif

// Common functions
#include "PMCommon.h"

// Configuration Flags
// Generated sound only
#define POKEMINI_GENSOUND	0x02

extern int PokeMini_FreeBIOS;	// Using freebios?
extern int PokeMini_Flags;	// Configuration flags
extern int PokeMini_Rumbling;	// Pokemon-Mini is rumbling
extern uint8_t PM_BIOS[];	// Pokemon-Mini BIOS ($000000 to $000FFF, 4096)
extern uint8_t PM_RAM[];	// Pokemon-Mini RAM  ($001000 to $002100, 4096 + 256)
extern uint8_t *PM_ROM;		// Pokemon-Mini ROM  ($002100 to $1FFFFF, Up to 2MB)
extern int PM_ROM_Alloc;	// Pokemon-Mini ROM Allocated on memory?
extern int PM_ROM_Size;		// Pokemon-Mini ROM Size
extern int PM_ROM_Mask;		// Pokemon-Mini ROM Mask
extern int PokeMini_LCDMode;	// LCD Mode
extern int PokeMini_ColorFormat;	// Color Format (0 = 8x8, 1 = 4x4)
extern int PokeMini_HostBattStatus;	// Host battery status

// Number of cycles to process on hardware
extern int PokeHWCycles;

#ifndef TARGET_GNW
extern retro_log_printf_t log_cb;
#endif

enum {
	LCDMODE_ANALOG = 0,
	LCDMODE_3SHADES,
	LCDMODE_2SHADES,
	LCDMODE_COLORS
};

// Include Interfaces
#include "IOMap.h"
#include "Video.h"
#include "MinxCPU.h"
#include "MinxTimers.h"
#include "MinxIO.h"
#include "MinxIRQ.h"
#include "MinxPRC.h"
#include "MinxColorPRC.h"
#include "MinxLCD.h"
#include "MinxAudio.h"
#include "CommandLine.h"
#include "Multicart.h"

// PRC Read/Write
#ifdef PERFORMANCE

static INLINE uint8_t MinxPRC_OnRead(int cpu, uint32_t addr)
{
	if (addr >= 0x2100) {
		// ROM Read
		if (PM_ROM) return PM_ROM[addr & PM_ROM_Mask];
	} else if (addr >= 0x2000) {
		// I/O Read (Unused)
		return 0xFF;
	} else if (addr >= 0x1000) {
		// RAM Read
		return PM_RAM[addr-0x1000];
	} else {
		// BIOS Read
		return PM_BIOS[addr];
	}
	return 0xFF;
}

static INLINE void MinxPRC_OnWrite(int cpu, uint32_t addr, uint8_t data)
{
	// RAM Write
	if ((addr >= 0x1000) && (addr < 0x2000))
		PM_RAM[addr-0x1000] = data;
}

#else

#include "Multicart.h"

#define MinxPRC_OnRead	MinxCPU_OnRead
#define MinxPRC_OnWrite	MinxCPU_OnWrite

#endif

// LCD Framebuffer
#define MinxPRC_LCDfb	PM_RAM

// Load state safe variables
#define POKELOADSS_START(size)\
	uint32_t rsize = 0;\
	uint32_t tmp32;\
	uint16_t tmp16;\
	if (size != bsize) return 0;\
	{ tmp32 = 0; tmp16 = 0; }

// -- Stream versions START
#define POKELOADSS_STREAM_32(var) {\
	rsize += (uint32_t)memstream_read(stream, (void *)&tmp32, 4);\
	var = tmp32;\
}

#define POKELOADSS_STREAM_16(var) {\
	rsize += (uint32_t)memstream_read(stream, (void *)&tmp16, 2);\
	var = tmp16;\
}

#define POKELOADSS_STREAM_8(var) {\
	rsize += (uint32_t)memstream_read(stream, (void *)&var, 1);\
}

#define POKELOADSS_STREAM_A(array, size) {\
	rsize += (uint32_t)memstream_read(stream, (void *)array, size);\
}

#define POKE_SEEK_CUR 1

#define POKELOADSS_STREAM_X(size) {\
	rsize += memstream_seek(stream, size, POKE_SEEK_CUR) ? 0 : size;\
}
// -- Stream versions End

#define POKELOADSS_END(size) {\
	tmp32 = 0; tmp16 = 0;\
	return (rsize == size) + tmp32 + (uint32_t)tmp16;\
}

// -- Stream versions START
#define POKESAVESS_STREAM_START(size)\
	uint32_t wsize = 0;\
	uint32_t tmp32 = size;\
	uint16_t tmp16;\
	if (memstream_write(stream, (void *)&tmp32, 4) != 4) return 0;\
	{ tmp32 = 0; tmp16 = 0; }

#define POKESAVESS_STREAM_32(var) {\
	tmp32 = (uint32_t)var;\
	wsize += (uint32_t)memstream_write(stream, (void *)&tmp32, 4);\
}

#define POKESAVESS_STREAM_16(var) {\
	tmp16 = (uint16_t)var;\
	wsize += (uint32_t)memstream_write(stream, (void *)&tmp16, 2);\
}

#define POKESAVESS_STREAM_8(var) {\
	wsize += (uint32_t)memstream_write(stream, (void *)&var, 1);\
}

#define POKESAVESS_STREAM_A(array, size) {\
	wsize += (uint32_t)memstream_write(stream, (void *)array, size);\
}

#define POKESAVESS_STREAM_X(size) {\
	tmp16 = 0;\
	for (tmp32=0; tmp32<(uint32_t)size; tmp32++) {\
		wsize += (uint32_t)memstream_write(stream, (void *)&tmp16, 1);\
	}\
}
// -- Stream versions End

#define POKESAVESS_END(size) {\
	tmp32 = 0; tmp16 = 0;\
	return (wsize == size) + tmp32 + (uint32_t)tmp16;\
}

// Stream I/O
typedef int TPokeMini_StreamIO(void *data, int size, void *ptr);

// Create emulator and all interfaces
int PokeMini_Create(int flags, int soundfifo);

// Destroy emulator and all interfaces
void PokeMini_Destroy(void);

// Apply changes from command lines
void PokeMini_ApplyChanges(void);

// Low power battery emulation
void PokeMini_LowPower(int enable);

// Set LCD mode
void PokeMini_SetLCDMode(int mode);

// Generate rumble offset
int PokeMini_GenRumbleOffset(int pitch);

// Load BIOS file
int PokeMini_LoadBIOSFile(const char *filename);

// Load FreeBIOS
int PokeMini_LoadFreeBIOS(void);

// Check if file exist
int PokeMini_FileExist(const char *filename);

// New MIN ROM
int PokeMini_NewMIN(uint32_t size);

// Synchronize host time
int PokeMini_SyncHostTime(void);

// Load EEPROM data
int PokeMini_LoadEEPROMFile(const char *filename);

// Save EEPROM data
int PokeMini_SaveEEPROMFile(const char *filename);

#ifndef TARGET_GNW
// Load emulator state from memory stream
int PokeMini_LoadSSStream(uint8_t *buffer, uint64_t size);

// Save emulator state to memory stream
int PokeMini_SaveSSStream(uint8_t *buffer, uint64_t size);
#else
// Load emulator state from memory stream
int PokeMini_LoadSSStream(const char *filename, uint64_t size);

// Save emulator state to memory stream
int PokeMini_SaveSSStream(const char *filename, uint64_t size);
#endif

// Reset CPU
void PokeMini_Reset(int hardreset);

// Internals, do not call directly!
void PokeMini_FreeColorInfo(void);

#endif
