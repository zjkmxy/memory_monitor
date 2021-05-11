#coding:utf-8
import sys
import re
import os.path
import subprocess

argv = sys.argv
argc = len(argv)
if(argc != 3):
  print('Usage: python %s <input file> <output file>\n' % argv[0])
  quit()

symDict = {}
def getSymbolAddr(filepath, funcname):
  if not symDict.has_key(filepath):
    status, output = subprocess.getstatusoutput('nm -p %s' % filepath)
    if status != 0:
      symDict[filepath] = ('!! %s\n' % output)
      return False, ('!! %s\n' % output)
    symLst = output.split('\n')
    symDict[filepath] = {}
    for row in symLst:
      mat = re.match('^(?P<addr>[0-9A-Fa-f]+) [TtWw] (?P<name>.+)$', row)
      if not (mat is None):
        symDict[filepath][mat.group('name')] = mat.group('addr')
    if len(symDict[filepath]) == 0:
      symDict[filepath] = ('!! %s\n' % output)
  if not isinstance(symDict[filepath], dict):
    return False, symDict[filepath]
  if not symDict[filepath].has_key(funcname):
    return False, ('!! Function not found: %s\n' % filepath)
  else:
    return True, symDict[filepath][funcname]

pattern1 = '^(?P<file>.+)\((?P<function>.*)\+0x(?P<offset>[0-9A-Fa-f]+)\)\[0x(?P<address>[0-9A-Fa-f]+)\]$'
pattern2 = '^(?P<file>.+)\[0x(?P<address>[0-9A-Fa-f]+)\]$'

with open(argv[1], 'r') as fin:
  with open(argv[2], 'w') as fout:
    for row in fin:
      mat = re.match(pattern1, row)
      if mat is None:
        mat = re.match(pattern2, row)
        if mat is None:
          fout.write(row)
          continue
        else:
          funcname = ''
          offset = ''
      else:
        funcname = mat.group('function')
        offset = mat.group('offset')
      filepath = mat.group('file')
      address = mat.group('address')
      if not os.path.exists(filepath):
        fout.write('!! File not found: %s\n' % filepath)
        continue
      if len(funcname) > 0:
        # status, output = commands.getstatusoutput('nm -p %s | grep %s' % (filepath, funcname))
        status, output = getSymbolAddr(filepath, funcname)
        if not status:
          fout.write(output)
          continue
        addrbase = output.split(' ')[0]
        addrfinal = int(addrbase, 16) + int(offset, 16)
      elif len(offset) > 0:
        addrfinal = int(offset, 16)
      else:
        addrfinal = int(address, 16)
      status, output = subprocess.getstatusoutput('addr2line -e %s %x' % (filepath, addrfinal))
      if status != 0:
        fout.write('!! %s\n' % output)
      else:
        fout.write('%s\n' % output)

