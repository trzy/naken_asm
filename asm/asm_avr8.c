/**
 *  naken_asm assembler.
 *  Author: Michael Kohn
 *   Email: mike@mikekohn.net
 *     Web: http://www.mikekohn.net/
 * License: GPL
 *
 * Copyright 2010-2013 by Michael Kohn
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "asm_common.h"
#include "asm_avr8.h"
#include "assembler.h"
#include "table_avr8.h"
#include "get_tokens.h"
#include "eval_expression.h"

enum
{
  OPERAND_NONE=0,
  OPERAND_NUMBER,
  OPERAND_REG,
  OPERAND_REG16,
  OPERAND_REG16_PLUS,
  OPERAND_MINUS_REG16,
};

enum
{
  REG16_X=0,
  REG16_Y,
  REG16_Z,
};

struct _operand
{
  int value;
  char type;
};

struct _alias
{
  char *instr;
  char *new;
};

static struct _alias alias[] =
{
  { "clr", "eor" },
  { "tst", "and" },
  { "lsl", "add" },
  { "rol", "adc" },
  { NULL, NULL },
};

static int get_register_avr8(char *token)
{
int n;
int r;

  if (token[0]=='r' || token[0]=='R')
  {
    if (token[1]==0) return -1;

    r=0; n=1;
    while(1)
    {
      if (token[n]==0) return r;
      if (token[n]<'0' || token[n]>'9') return -1;
      r=(r*10)+(token[n]-'0');
      n++;
    }
  }

  return -1;
}

int parse_instruction_avr8(struct _asm_context *asm_context, char *instr)
{
char token[TOKENLEN];
int token_type;
char instr_case_mem[TOKENLEN];
char *instr_case=instr_case_mem;
struct _operand operands[2];
int operand_count=0;
int matched=0;
int offset;
int n,num;
int rd,rr,k;

  lower_copy(instr_case, instr);

  //pushback(asm_context, token, token_type);

  while(1)
  {
    token_type=get_token(asm_context, token, TOKENLEN);
    if (token_type==TOKEN_EOL || token_type==TOKEN_EOF)
    {
      break;
    }

    if (operand_count>2)
    {
      print_error_opcount(instr, asm_context);
      return -1;
    }

    if ((n=get_register_avr8(token))!=-1)
    {
      operands[operand_count].type=OPERAND_REG;
      operands[operand_count].value=n;
    }
      else
    if (IS_TOKEN(token,'x') || IS_TOKEN(token,'X'))
    {
      operands[operand_count].type=OPERAND_REG16;
      operands[operand_count].value=REG16_X;
      token_type=get_token(asm_context, token, TOKENLEN);
      if (IS_TOKEN(token,'+')) { operands[operand_count].type=OPERAND_REG16_PLUS; }
      else { pushback(asm_context, token, token_type); }
    }
      else
    if (IS_TOKEN(token,'y') || IS_TOKEN(token,'Y'))
    {
      operands[operand_count].type=OPERAND_REG16;
      operands[operand_count].value=REG16_Y;
      token_type=get_token(asm_context, token, TOKENLEN);
      if (IS_TOKEN(token,'+')) { operands[operand_count].type=OPERAND_REG16_PLUS; }
      else { pushback(asm_context, token, token_type); }
    }
      else
    if (IS_TOKEN(token,'z') || IS_TOKEN(token,'Z'))
    {
      operands[operand_count].type=OPERAND_REG16;
      operands[operand_count].value=REG16_Z;
      token_type=get_token(asm_context, token, TOKENLEN);
      if (IS_TOKEN(token,'+')) { operands[operand_count].type=OPERAND_REG16_PLUS; }
      else { pushback(asm_context, token, token_type); }
    }
      else
    if (IS_TOKEN(token,'-'))
    {
      operands[operand_count].type=OPERAND_MINUS_REG16;
      token_type=get_token(asm_context, token, TOKENLEN);
      if (IS_TOKEN(token,'x') || IS_TOKEN(token,'X'))
      {
        operands[operand_count].value=REG16_X;
      }
        else
      if (IS_TOKEN(token,'y') || IS_TOKEN(token,'Y'))
      {
        operands[operand_count].value=REG16_Y;
      }
        else
      if (IS_TOKEN(token,'z') || IS_TOKEN(token,'Z'))
      {
        operands[operand_count].value=REG16_Z;
      }
        else
      {
        //double pushback, or?
        pushback(asm_context, token, token_type);
        if (asm_context->pass==1)
        {
          eat_operand(asm_context);
          num=0;
        }
          else
        if (eval_expression(asm_context, &num)!=0)
        {
          print_error_illegal_expression(instr, asm_context);
          return -1;
        }

        operands[operand_count].type=OPERAND_NUMBER;
        operands[operand_count].value=-num;
      }
    }
      else
    {
      pushback(asm_context, token, token_type);

      if (eval_expression(asm_context, &num)!=0)
      {
        if (asm_context->pass==1)
        {
          eat_operand(asm_context);
        }
          else
        {
          print_error_illegal_expression(instr, asm_context);
          return -1;
        }
      }

      operands[operand_count].type=OPERAND_NUMBER;
      operands[operand_count].value=num;
    }

    operand_count++;
    token_type=get_token(asm_context, token, TOKENLEN);
    if (token_type==TOKEN_EOL) break;
    if (IS_NOT_TOKEN(token,',') || operand_count==3)
    {
      print_error_unexp(token, asm_context);
      return -1;
    }
  }

  // Check for aliases:
  n=0;
  while(alias[n].instr!=NULL)
  {
    if (strcmp(alias[n].instr, instr_case)==0)
    {
      operand_count=2;
      operands[1].type=OPERAND_REG;
      operands[1].value=operands[0].value;
      instr_case=alias[n].new;
    }

    n++;
  }

  if (strcmp("cbr", instr_case)==0)
  {
    operands[1].value=(operands[1].value^0xff);
    instr_case="andi";
  }

  n=0;
  while(table_avr8[n].instr!=NULL)
  {
    if (strcmp(table_avr8[n].instr, instr_case)==0)
    {
      matched=1;

      switch(table_avr8[n].type)
      {
        case OP_NONE:
          if (operand_count==0)
          {
            add_bin16(asm_context, table_avr8[n].opcode, IS_OPCODE);
            return 2;
          }
          break;
        case OP_BRANCH_S_K:
          if (operand_count==2 &&
              operands[0].type==OPERAND_NUMBER &&
              operands[1].type==OPERAND_NUMBER)
          {
            if (asm_context->pass==1) { offset=0; }
            else { offset=operands[0].value-asm_context->address+2; }
            if (offset<-64 || offset>63)
            {
              print_error_range("Offset", -64, 63, asm_context);
              return -1;
            }
            if (operands[0].value>7)
            {
              print_error_range("Bit", 0, 7, asm_context);
              return -1;
            }
            add_bin16(asm_context, table_avr8[n].opcode|(offset&0x7f)|operands[0].value, IS_OPCODE);
            return 2;
          }
          break;
        case OP_BRANCH_K:
          if (operand_count==1 && operands[0].type==OPERAND_NUMBER)
          {
            if (asm_context->pass==1) { offset=0; }
            else { offset=operands[0].value-asm_context->address+2; }
            if (offset<-64 || offset>63)
            {
              print_error_range("Offset", -64, 63, asm_context);
              return -1;
            }
            add_bin16(asm_context, table_avr8[n].opcode|(offset&0x7f), IS_OPCODE);
            return 2;
          }
          break;
        case OP_TWO_REG:
          if (operand_count==2 &&
              operands[0].type==OPERAND_REG &&
              operands[1].type==OPERAND_REG)
          {
            rd=operands[0].value<<4;
            rr=((operands[1].value&0x10)<<5)|(operands[1].value&0xf);
            add_bin16(asm_context, table_avr8[n].opcode|rd|rr, IS_OPCODE);
            return 2;
          }
          break;
        case OP_REG_IMM:
          if (operand_count==2 &&
              operands[0].type==OPERAND_REG &&
              operands[1].type==OPERAND_NUMBER)
          {
            if (operands[0].value<16)
            {
              print_error_range("Register", 16, 31, asm_context);
              return -1;
            }

            if (operands[1].value<-128 || operands[1].value>255)
            {
              print_error_range("Constant", -128, 255, asm_context);
              return -1;
            }

            operands[1].value=operands[1].value&0xff;
            rd=(operands[0].value-16)<<4;
            k=((operands[1].value&0xf0)<<4)|(operands[1].value&0xf);
            add_bin16(asm_context, table_avr8[n].opcode|rd|k, IS_OPCODE);
            return 2;
          }
          break;
        case OP_ONE_REG:
          if (operand_count==1 &&
              operands[0].type==OPERAND_REG)
          {
            rd=(operands[0].value)<<4;
            add_bin16(asm_context, table_avr8[n].opcode|rd, IS_OPCODE);
            return 2;
          }
          break;
        case OP_REG_BIT:
          if (operand_count==2 &&
              operands[0].type==OPERAND_REG &&
              operands[1].type==OPERAND_NUMBER)
          {
            if (operands[1].value<0 || operands[1].value>7)
            {
              print_error_range("Bit", 0, 7, asm_context);
              return -1;
            }
            rd=(operands[0].value)<<4;
            k=operands[1].value;
            add_bin16(asm_context, table_avr8[n].opcode|rd|k, IS_OPCODE);
            return 2;
          }
          break;
        case OP_REG_IMM_WORD:
          if (operand_count==2 &&
              operands[0].type==OPERAND_REG &&
              operands[1].type==OPERAND_NUMBER)
          {
            if (operands[0].value<24 || operands[0].value>30 ||
                (operands[0].value&0x1)==1)
            {
              printf("Error: Register must be r24,r26,r28,r30 for '%s' at %s:%d.\n", instr, asm_context->filename, asm_context->line);
              return -1;
            }
            if (operands[1].value<0 || operands[1].value>63)
            {
              print_error_range("Constant", 0, 63, asm_context);
              return -1;
            }
            rd=((operands[0].value-24)>>1)<<4;
            k=((operands[1].value&0x30)<<2)|(operands[1].value&0xf);
            add_bin16(asm_context, table_avr8[n].opcode|rd|k, IS_OPCODE);
            return 2;
          }
          break;
        case OP_IOREG_BIT:
          if (operand_count==2 &&
              operands[0].type==OPERAND_NUMBER &&
              operands[1].type==OPERAND_NUMBER)
          {
            if (operands[0].value<0 || operands[0].value>31)
            {
              print_error_range("I/O Reg", 0, 31, asm_context);
              return -1;
            }

            if (operands[1].value<0 || operands[1].value>7)
            {
              print_error_range("Bit", 0, 7, asm_context);
              return -1;
            }
            rd=(operands[0].value<<3);
            k=operands[1].value;
            add_bin16(asm_context, table_avr8[n].opcode|rd|k, IS_OPCODE);
            return 2;
          }
          break;
        case OP_SREG_BIT:
          if (operand_count==1 && operands[0].type==OPERAND_NUMBER)
          {
            k=operands[0].value<<4;
            add_bin16(asm_context, table_avr8[n].opcode|k, IS_OPCODE);
            return 2;
          }
          break;
        case OP_REG_4:
          if (operand_count==1 && operands[0].type==OPERAND_REG)
          {
            if (operands[0].value<16)
            {
              print_error_range("Register", 16, 31, asm_context);
              return -1;
            }
            rd=(operands[0].value-16)<<4;
            add_bin16(asm_context, table_avr8[n].opcode|rd, IS_OPCODE);
            return 2;
          }
          break;
        case OP_IN:
          if (operand_count==2 &&
              operands[0].type==OPERAND_REG &&
              operands[1].type==OPERAND_NUMBER)
          {
            if (operands[1].value<0 || operands[1].value>63)
            {
              print_error_range("I/O Reg", 0, 63, asm_context);
              return -1;
            }
            rd=operands[0].value<<4;
            k=((operands[1].value&0x30)<<5)|(operands[1].value&0xf);
            add_bin16(asm_context, table_avr8[n].opcode|rd|k, IS_OPCODE);
            return 2;
          }
          break;
        case OP_OUT:
          if (operand_count==2 &&
              operands[0].type==OPERAND_NUMBER &&
              operands[1].type==OPERAND_REG)
          {
            if (operands[0].value<0 || operands[0].value>63)
            {
              print_error_range("I/O Reg", 0, 63, asm_context);
              return -1;
            }
            rd=operands[1].value<<4;
            k=((operands[0].value&0x30)<<5)|(operands[0].value&0xf);
            add_bin16(asm_context, table_avr8[n].opcode|rd|k, IS_OPCODE);
            return 2;
          }
          break;
        case OP_MOVW:
          if (operand_count==2 &&
              operands[0].type==OPERAND_REG &&
              operands[1].type==OPERAND_REG)
          {
            if ((operands[0].value&0x1)!=0 &&
                (operands[1].value&0x1)!=0)
            {
              printf("Error: Register must be even for '%s' at %s:%d.\n", instr, asm_context->filename, asm_context->line);
              return -1;
            }
            rd=(operands[0].value>>1)<<4;
            rr=operands[1].value>>1;
            add_bin16(asm_context, table_avr8[n].opcode|rd|rr, IS_OPCODE);
            return 2;
          }
          break;
        case OP_RELATIVE:
          if (operand_count==1 && operands[0].type==OPERAND_NUMBER)
          {
            if (asm_context->pass==1) { offset=0; }
            else { offset=operands[0].value-(asm_context->address+2); }

            if (offset<-2048 || offset>4096)
            {
              print_error_range("Offset", -2048, 2048, asm_context);
              return -1;
            }

            offset=offset&0xfff;
            add_bin16(asm_context, table_avr8[n].opcode|offset, IS_OPCODE);
            return 2;
          }
          break;
        case OP_JUMP:
          if (operand_count==1 && operands[0].type==OPERAND_NUMBER)
          {
            if (asm_context->pass==1) { k=0; }
            else { k=operands[0].value; }

            if (k<0 || k>((1<<22)-1))
            {
              print_error_range("Address", 0, ((1<<22)-1), asm_context);
              return -1;
            }

            rd=k>>8;
            rd=((k<<3)&0x1f0)|(k&0x1);
            add_bin16(asm_context, table_avr8[n].opcode|rd, IS_OPCODE);
            add_bin16(asm_context, k&0xffff, IS_OPCODE);
            return 4;
          }
          break;
        case OP_SPM_Z_PLUS:
          if (operand_count==1 &&
              operands[0].type==OPERAND_REG16_PLUS &&
              operands[0].value==REG16_Z)
          {
            add_bin16(asm_context, table_avr8[n].opcode, IS_OPCODE);
            return 2;
          }
          break;
        case OP_REG_X:
          if (operand_count==2 && operands[0].type==OPERAND_REG &&
              operands[1].type==OPERAND_REG16 && operands[1].value==REG16_X)
          {
            rd=operands[0].value<<4;
            add_bin16(asm_context, table_avr8[n].opcode|rd, IS_OPCODE);
            return 2;
          }
          break;
        case OP_REG_Y:
          if (operand_count==2 && operands[0].type==OPERAND_REG &&
              operands[1].type==OPERAND_REG16 && operands[1].value==REG16_Y)
          {
            rd=operands[0].value<<4;
            add_bin16(asm_context, table_avr8[n].opcode|rd, IS_OPCODE);
            return 2;
          }
          break;
        case OP_REG_Z:
          if (operand_count==2 && operands[0].type==OPERAND_REG &&
              operands[1].type==OPERAND_REG16 && operands[1].value==REG16_Z)
          {
            rd=operands[0].value<<4;
            add_bin16(asm_context, table_avr8[n].opcode|rd, IS_OPCODE);
            return 2;
          }
          break;
        case OP_REG_X_PLUS:
          if (operand_count==2 && operands[0].type==OPERAND_REG &&
              operands[1].type==OPERAND_REG16_PLUS &&
              operands[1].value==REG16_X)
          {
            rd=operands[0].value<<4;
            add_bin16(asm_context, table_avr8[n].opcode|rd, IS_OPCODE);
            return 2;
          }
          break;
        case OP_REG_Y_PLUS:
          if (operand_count==2 && operands[0].type==OPERAND_REG &&
              operands[1].type==OPERAND_REG16_PLUS &&
              operands[1].value==REG16_Y)
          {
            rd=operands[0].value<<4;
            add_bin16(asm_context, table_avr8[n].opcode|rd, IS_OPCODE);
            return 2;
          }
          break;
        case OP_REG_Z_PLUS:
          if (operand_count==2 && operands[0].type==OPERAND_REG &&
              operands[1].type==OPERAND_REG16_PLUS &&
              operands[1].value==REG16_Z)
          {
            rd=operands[0].value<<4;
            add_bin16(asm_context, table_avr8[n].opcode|rd, IS_OPCODE);
            return 2;
          }
          break;
        case OP_REG_MINUS_X:
          if (operand_count==2 && operands[0].type==OPERAND_REG &&
              operands[1].type==OPERAND_MINUS_REG16 &&
              operands[1].value==REG16_X)
          {
            rd=operands[0].value<<4;
            add_bin16(asm_context, table_avr8[n].opcode|rd, IS_OPCODE);
            return 2;
          }
          break;
        case OP_REG_MINUS_Y:
          if (operand_count==2 && operands[0].type==OPERAND_REG &&
              operands[1].type==OPERAND_MINUS_REG16 &&
              operands[1].value==REG16_Y)
          {
            rd=operands[0].value<<4;
            add_bin16(asm_context, table_avr8[n].opcode|rd, IS_OPCODE);
            return 2;
          }
          break;
        case OP_REG_MINUS_Z:
          if (operand_count==2 && operands[0].type==OPERAND_REG &&
              operands[1].type==OPERAND_MINUS_REG16 &&
              operands[1].value==REG16_Z)
          {
            rd=operands[0].value<<4;
            add_bin16(asm_context, table_avr8[n].opcode|rd, IS_OPCODE);
            return 2;
          }
          break;
        case OP_X_REG:
          if (operand_count==2 && operands[1].type==OPERAND_REG &&
              operands[0].type==OPERAND_REG16 && operands[0].value==REG16_X)
          {
            rd=operands[1].value<<4;
            add_bin16(asm_context, table_avr8[n].opcode|rd, IS_OPCODE);
            return 2;
          }
          break;
        case OP_Y_REG:
          if (operand_count==2 && operands[1].type==OPERAND_REG &&
              operands[0].type==OPERAND_REG16 && operands[0].value==REG16_Y)
          {
            rd=operands[1].value<<4;
            add_bin16(asm_context, table_avr8[n].opcode|rd, IS_OPCODE);
            return 2;
          }
          break;
        case OP_Z_REG:
          if (operand_count==2 && operands[1].type==OPERAND_REG &&
              operands[0].type==OPERAND_REG16 && operands[0].value==REG16_Z)
          {
            rd=operands[1].value<<4;
            add_bin16(asm_context, table_avr8[n].opcode|rd, IS_OPCODE);
            return 2;
          }
          break;
        case OP_X_PLUS_REG:
          if (operand_count==2 && operands[1].type==OPERAND_REG &&
              operands[0].type==OPERAND_REG16_PLUS &&
              operands[0].value==REG16_X)
          {
            rd=operands[1].value<<4;
            add_bin16(asm_context, table_avr8[n].opcode|rd, IS_OPCODE);
            return 2;
          }
          break;
        case OP_Y_PLUS_REG:
          if (operand_count==2 && operands[1].type==OPERAND_REG &&
              operands[0].type==OPERAND_REG16_PLUS &&
              operands[0].value==REG16_Y)
          {
            rd=operands[1].value<<4;
            add_bin16(asm_context, table_avr8[n].opcode|rd, IS_OPCODE);
            return 2;
          }
          break;
        case OP_Z_PLUS_REG:
          if (operand_count==2 && operands[1].type==OPERAND_REG &&
              operands[0].type==OPERAND_REG16_PLUS &&
              operands[0].value==REG16_Z)
          {
            rd=operands[1].value<<4;
            add_bin16(asm_context, table_avr8[n].opcode|rd, IS_OPCODE);
            return 2;
          }
          break;
        case OP_MINUS_X_REG:
          if (operand_count==2 && operands[1].type==OPERAND_REG &&
              operands[0].type==OPERAND_MINUS_REG16 &&
              operands[0].value==REG16_X)
          {
            rd=operands[1].value<<4;
            add_bin16(asm_context, table_avr8[n].opcode|rd, IS_OPCODE);
            return 2;
          }
          break;
        case OP_MINUS_Y_REG:
          if (operand_count==2 && operands[1].type==OPERAND_REG &&
              operands[0].type==OPERAND_MINUS_REG16 &&
              operands[0].value==REG16_Y)
          {
            rd=operands[1].value<<4;
            add_bin16(asm_context, table_avr8[n].opcode|rd, IS_OPCODE);
            return 2;
          }
          break;
        case OP_MINUS_Z_REG:
          if (operand_count==2 && operands[1].type==OPERAND_REG &&
              operands[0].type==OPERAND_MINUS_REG16 &&
              operands[0].value==REG16_Z)
          {
            rd=operands[1].value<<4;
            add_bin16(asm_context, table_avr8[n].opcode|rd, IS_OPCODE);
            return 2;
          }
          break;



#if 0
        case OP_REG_XYZ:
          rd=(operands[0].value<<4);
          if (operand_count==2 && operands[0].type==OPERAND_REG)
          {
            char regcode[] = { 0xc, 0x8, 0x0 };
            if (operands[1].type>=OPERAND_REG16 && 
                operands[1].type<=OPERAND_MINUS_REG16)
            {
              k=OPERAND_MINUS_REG16-operands[1].type;

            }
          }
#endif
        default:
          break;
      }
    }
    n++;
  }

  if (matched==1)
  {
    printf("Error: Unknown flag/operands combo for '%s' at %s:%d.\n", instr, asm_context->filename, asm_context->line);
  }
    else
  {
    printf("Error: Unknown instruction '%s' at %s:%d.\n", instr, asm_context->filename, asm_context->line);
  }

  return -1;
}


