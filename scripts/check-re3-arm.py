#!/usr/bin/env python3
"""Audit static ARM kernel and its actual compiled streaming object."""
import argparse,hashlib,pathlib,struct
p=argparse.ArgumentParser();p.add_argument('binary',type=pathlib.Path);p.add_argument('--stream-object',required=True,type=pathlib.Path);a=p.parse_args();b=a.binary.read_bytes()
def arm(data):
 assert data[:6]==b'\x7fELF\x01\x01' and struct.unpack_from('<H',data,18)[0]==40,'not ARM32 little-endian ELF'
arm(b)
phoff=struct.unpack_from('<I',b,28)[0];phsize,phnum=struct.unpack_from('<HH',b,42)
for i in range(phnum):assert struct.unpack_from('<I',b,phoff+i*phsize)[0] not in (2,3),'not static'
obj=a.stream_object.read_bytes();arm(obj)
shoff=struct.unpack_from('<I',obj,32)[0];shsize,shnum=struct.unpack_from('<HH',obj,46)
sections=[struct.unpack_from('<IIIIIIIIII',obj,shoff+i*shsize) for i in range(shnum)]
names=set()
for section in sections:
 if section[1]!=2:continue
 strings=sections[section[6]];table=obj[strings[4]:strings[4]+strings[5]]
 for offset in range(section[4],section[4]+section[5],section[9]):
  name,value,size,info,other,index=struct.unpack_from('<IIIBBH',obj,offset)
  if name:names.add(table[name:table.index(b'\0',name)].decode('ascii'))
assert {'sem_init','sem_wait','sem_post','sem_destroy','_Z12re3_sem_openv'}<=names,'wrong unnamed semaphore implementation'
assert not ({'sem_open','sem_unlink','sem_close'}&names),'named semaphore still referenced by streaming'
assert b'CPU streaming sync: process-local unnamed semaphores' in b,'wrong linked streaming path'
print('PASS static ARM32 kernel and compiled CdStreamPosix object: unnamed sem_init/wait/post/destroy; no sem_open/unlink/close references')
print('SHA256',hashlib.sha256(b).hexdigest(),'bytes',len(b))
