/**
 *  naken_asm assembler.
 *  Author: Michael Kohn
 *   Email: mike@mikekohn.net
 *     Web: http://www.mikekohn.net/
 * License: GPL
 *
 * Copyright 2010-2016 by Michael Kohn
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disasm/ps2_ee_vu.h"

#define READ_RAM(a) memory_read_m(memory, a)

int get_cycle_count_ps2_ee_vu(uint32_t opcode)
{
  return -1;
}

int disasm_ps2_ee_vu(struct _memory *memory, uint32_t address, char *instruction, int *cycles_min, int *cycles_max, int is_lower)
{
  struct _table_ps2_ee_vu *table_ps2_ee_vu;
  uint32_t opcode;
  int function, format, operation;
  int n, r;
  char temp[32];
  int ft, fs, fd, dest;
  char bc;

  *cycles_min = -1;
  *cycles_max = -1;
  opcode = memory_read32_m(memory, address);

  if (is_lower == 0)
  {
    table_ps2_ee_vu = table_ps2_ee_vu_upper;
  }
    else
  {
    table_ps2_ee_vu = table_ps2_ee_vu_lower;
  }

  instruction[0] = 0;

  n = 0;
  while(table_ps2_ee_vu[n].instr != NULL)
  {
    if (table_ps2_ee_vu[n].opcode == (opcode & table_ps2_ee_vu[n].mask))
    {
      strcpy(instruction, table_ps2_ee_vu[n].instr);

      dest = (opcode >> 21) & 0xf;
      ft = (opcode >> 16) & 0x1f;
      fs = (opcode >> 11) & 0x1f;
      fd = (opcode >> 6) & 0x1f;

#if 0
      if ((table_ps2_ee_vu[n].mask & 0x3) == 0)
      {
        char bc_xyzw[] = { 'x', 'y', 'z', 'w' };
        bc = bc_xyzw[opcode & 0x3];
      }
#endif

      if ((table_ps2_ee_vu[n].flags & FLAG_DEST) != 0)
      {
        strcat(instruction, ".");
        if ((dest & 8) != 0) { strcat(instruction, "x"); }
        if ((dest & 4) != 0) { strcat(instruction, "y"); }
        if ((dest & 2) != 0) { strcat(instruction, "z"); }
        if ((dest & 1) != 0) { strcat(instruction, "w"); }
      }

      for (r = 0; r < table_ps2_ee_vu[n].operand_count; r++)
      {
        if (r != 0) { strcat(instruction, ","); }

        switch(table_ps2_ee_vu[n].operand[r])
        {
          case EE_VU_OP_FT:
            sprintf(temp, " vf%d", ft);
            break;
          case EE_VU_OP_FS:
            sprintf(temp, " vf%d", fs);
            break;
          case EE_VU_OP_FD:
            sprintf(temp, " vf%d", fd);
            break;
          case EE_VU_OP_I:
            strcat(temp, " I");
            break;
          case EE_VU_OP_Q:
            strcat(temp, " Q");
            break;
          default:
            strcpy(temp, " ?");
            break;
        }

        strcat(instruction, temp);
      }

      return 4;
    }

    n++;
  }

  return 4;
}

void list_output_ps2_ee_vu(struct _asm_context *asm_context, uint32_t start, uint32_t end)
{
  int cycles_min,cycles_max;
  char instruction_upper[128];
  char instruction_lower[128];
  uint32_t opcode_upper;
  uint32_t opcode_lower;

  fprintf(asm_context->list, "\n");

  while(start < end)
  {
    opcode_upper = memory_read32_m(&asm_context->memory, start + 4);
    opcode_lower = memory_read32_m(&asm_context->memory, start);

    disasm_ps2_ee_vu(&asm_context->memory, start + 4, instruction_upper, &cycles_min, &cycles_max, 0);
    disasm_ps2_ee_vu(&asm_context->memory, start, instruction_lower, &cycles_min, &cycles_max, 1);

    fprintf(asm_context->list, "0x%08x: 0x%08x 0x%08x %-20s %-20s", start, opcode_upper, opcode_lower, instruction_upper, instruction_lower);

#if 0
    fprintf(asm_context->list, "0x%08x: 0x%08x %-40s cycles: ", start, opcode, instruction);

    if (cycles_min == cycles_max)
    { fprintf(asm_context->list, "%d\n", cycles_min); }
      else
    { fprintf(asm_context->list, "%d-%d\n", cycles_min, cycles_max); }

#endif
    start += 8;
  }
}

void disasm_range_ps2_ee_vu(struct _memory *memory, uint32_t start, uint32_t end)
{
  char instruction_upper[128];
  char instruction_lower[128];
  uint32_t opcode_upper;
  uint32_t opcode_lower;
  int cycles_min = 0,cycles_max = 0;

  printf("\n");

  printf("%-7s %-5s %-40s Cycles\n", "Addr", "Opcode", "Instruction");
  printf("------- ------ ----------------------------------       ------\n");

  while(start < end)
  {
    opcode_upper = memory_read32_m(memory, start + 4);
    opcode_lower = memory_read32_m(memory, start);

    disasm_ps2_ee_vu(memory, start + 4, instruction_upper, &cycles_min, &cycles_max, 0);
    disasm_ps2_ee_vu(memory, start, instruction_lower, &cycles_min, &cycles_max, 1);

    printf("0x%08x: 0x%08x 0x%08x %-20s %-20s", start, opcode_upper, opcode_lower, instruction_upper, instruction_lower);

#if 0
    if (cycles_min < 1)
    {
      printf("0x%04x: 0x%08x %-40s ?\n", start, num, instruction);
    }
      else
    if (cycles_min == cycles_max)
    {
      printf("0x%04x: 0x%08x %-40s %d\n", start, num, instruction, cycles_min);
    }
      else
    {
      printf("0x%04x: 0x%08x %-40s %d-%d\n", start, num, instruction, cycles_min, cycles_max);
    }
#endif

    start += 8;
  }
}
