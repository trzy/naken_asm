#!/usr/bin/env python

import datetime

d = datetime.datetime.now().strftime("%Y-%b-%d")

print ";;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;"
print ";;"
print ";; ESP32 registers"
print ";; Part of the naken_asm assembler"
print ";;"
print ";; Generated by: Michael Kohn (mike@mikekohn.net)"
print ";;               http://www.mikekohn.net/"
print ";;         Date: " + d
print ";;"
print ";;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;"
print

registers = { }

fp = open("esp32.txt", "r")

for line in fp:
  line = line.strip()
  if not line: continue
  if line.startswith(";"): continue

  tokens = line.split()
  name = tokens[0]
  address = tokens[-2]
  mode = tokens[-1]

  registers[address] = [ name, mode ]

fp.close()

keys = registers.keys()
keys.sort()

base = 0
base_name = ""

for register in keys:
  i = int(register, 16)
  data = registers[register]

  if (base & 0xfffffc00) != (i & 0xfffffc00):
    name = data[0].split("_")[0]

    if name == "RTC":
      name = data[0].split("_")[0] + "_" + data[0].split("_")[1]

    if name != base_name:
      base = i & 0xfffffc00
      base_name = name
      print
      print ";; Base registers for " + name
      print name + "_BASE equ 0x%08x" % (base)

  print data[0] + " equ 0x%03x" % (i - base)

print
