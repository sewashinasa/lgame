
local lgame = require("lgame")

Rect = lgame.rect.Rect

local assert = assert
local print = print
local collectgarbage = collectgarbage

local r1,r2,r3,r4,rf;


r1 = Rect(1, 2, 3, 4)
assert(r1.x == 1, "r1.x == 1 assert failure")
assert(r1.y == 2, "r1.y == 2 assert failure")
assert(r1.w == 3, "r1.w == 3 assert failure")
assert(r1.h == 4, "r1.h == 4 assert failure")
print("-- OK")

r2 = Rect({ 54, 23 }, { 23, 44 })
assert(r2.x == 54, "r2.x == 54 assert failure")
assert(r2.y == 23, "r2.y == 23 assert failure")
assert(r2.w == 23, "r2.w == 23 assert failure")
assert(r2.h == 44, "r2.h == 44 assert failure")
print("-- OK")

r3 = Rect({ 215, 321, 56, 23 })
assert(r3.x == 215, "r3.x == 215 assert failure")
assert(r3.y == 321, "r3.y == 321 assert failure")
assert(r3.w == 56, "r3.w == 56 assert failure")
assert(r3.h == 23, "r3.h == 23 assert failure")
print("-- OK")

r4 = Rect({ 100, 1000, 10000, 100000 })
assert(r4.x == 100, "r4.x == 100 assert failure")
assert(r4.y == 1000, "r4.y == 1000 assert failure")
assert(r4.w == 10000, "r4.w == 10000 assert failure")
assert(r4.h == 100000, "r4.h == 100000 assert failure")
print("-- OK")

r1,r2,r3,r4 = nil,nil,nil,nil



rf = Rect(0,0,0,0)
rf.x = 1
rf.y = 2
rf.w = 3
rf.h = 4

assert(rf.x == 1, "rf.x == 1 assert failure")
assert(rf.y == 2, "rf.y == 2 assert failure")
assert(rf.w == 3, "rf.w == 3 assert failure")
assert(rf.h == 4, "rf.h == 4 assert failure")
print("-- OK")



rf = nil
collectgarbage()



print("test_rect.lua successfuly complete")