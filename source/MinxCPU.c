/*
  PokeMini - Pok�mon-Mini Emulator
  Copyright (C) 2009-2012  JustBurn

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

#include "PokeMini.h"
#include "MinxCPU.h"

TMinxCPU MinxCPU;

//
// Functions
//

int MinxCPU_Create(void)
{
	// Init variables
	MinxCPU.BA.D = 0;
	MinxCPU.HL.D = 0;
	MinxCPU.X.D = 0;
	MinxCPU.Y.D = 0;
	MinxCPU.SP.D = 0;
	MinxCPU.PC.D = 0;
	MinxCPU.N.D = 0;
	MinxCPU.E = 0;
	MinxCPU.F = 0xC0;
	Set_U(0);
	MinxCPU.Status = MINX_STATUS_NORMAL;
	return 1;
}

void MinxCPU_Destroy(void)
{
	// Nothing...
}

// Reset core, call it after OnRead/OnWrite point to the right BIOS
void MinxCPU_Reset(int hardreset)
{
	MinxCPU.Status = MINX_STATUS_NORMAL;
	MinxCPU.PC.W.L = ReadMem16(hardreset ? 0 : 2);
	MinxCPU.E = 0x1F;
	MinxCPU.F = 0xC0;
	Set_U(0);
	MinxCPU_OnIRQHandle(MinxCPU.F, MinxCPU.Shift_U);
}

// Load State from stream
int MinxCPU_LoadStateStream(memstream_t *stream, uint32_t bsize)
{
	POKELOADSS_START(64);
	POKELOADSS_STREAM_32(MinxCPU.BA.D);
	POKELOADSS_STREAM_32(MinxCPU.HL.D);
	POKELOADSS_STREAM_32(MinxCPU.X.D);
	POKELOADSS_STREAM_32(MinxCPU.Y.D);
	POKELOADSS_STREAM_32(MinxCPU.SP.D);
	POKELOADSS_STREAM_32(MinxCPU.PC.D);
	POKELOADSS_STREAM_32(MinxCPU.N.D);
	POKELOADSS_STREAM_8(MinxCPU.U1);
	POKELOADSS_STREAM_8(MinxCPU.U2);
	POKELOADSS_STREAM_8(MinxCPU.F);
	POKELOADSS_STREAM_8(MinxCPU.E);
	POKELOADSS_STREAM_8(MinxCPU.IR);
	POKELOADSS_STREAM_8(MinxCPU.Shift_U);
	POKELOADSS_STREAM_8(MinxCPU.Status);
	POKELOADSS_STREAM_8(MinxCPU.IRQ_Vector);
	POKELOADSS_STREAM_A(MinxCPU.Reserved, 28);
	POKELOADSS_END(64);
}

// Save State to stream
int MinxCPU_SaveStateStream(memstream_t *stream)
{
	POKESAVESS_STREAM_START(64);
	POKESAVESS_STREAM_32(MinxCPU.BA.D);
	POKESAVESS_STREAM_32(MinxCPU.HL.D);
	POKESAVESS_STREAM_32(MinxCPU.X.D);
	POKESAVESS_STREAM_32(MinxCPU.Y.D);
	POKESAVESS_STREAM_32(MinxCPU.SP.D);
	POKESAVESS_STREAM_32(MinxCPU.PC.D);
	POKESAVESS_STREAM_32(MinxCPU.N.D);
	POKESAVESS_STREAM_8(MinxCPU.U1);
	POKESAVESS_STREAM_8(MinxCPU.U2);
	POKESAVESS_STREAM_8(MinxCPU.F);
	POKESAVESS_STREAM_8(MinxCPU.E);
	POKESAVESS_STREAM_8(MinxCPU.IR);
	POKESAVESS_STREAM_8(MinxCPU.Shift_U);
	POKESAVESS_STREAM_8(MinxCPU.Status);
	POKESAVESS_STREAM_8(MinxCPU.IRQ_Vector);
	POKESAVESS_STREAM_A(MinxCPU.Reserved, 28);
	POKESAVESS_END(64);
}

// Force call Interrupt by the address
int MinxCPU_CallIRQ(uint8_t addr)
{
	if (MinxCPU.Status == MINX_STATUS_IRQ) return 0;
	MinxCPU.Status = MINX_STATUS_IRQ;
	MinxCPU.IRQ_Vector = addr;
	return 1;
}
