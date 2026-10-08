

local lgame = require("lgame")

Color = lgame.color.Color

local c1 = Color(1, 2, 3, 4)
assert(c1.r == 1, "c1.r == 1 assert failure")
assert(c1.g == 2, "c1.g == 2 assert failure")
assert(c1.b == 3, "c1.b == 3 assert failure")
assert(c1.a == 4, "c1.a == 4 assert failure")
print("-- OK")

local c2 = Color({ 54, 23, 98, 34 })
assert(c2.r == 54, "c2.r == 54 assert failure")
assert(c2.g == 23, "c2.g == 23 assert failure")
assert(c2.b == 98, "c2.b == 98 assert failure")
assert(c2.a == 34, "c2.a == 34 assert failure")
print("-- OK")

local c3 = Color(0xFF00FF00)
assert(c3.r == 0xFF, "c3.r == 0xFF assert failure")
assert(c3.g == 0x00, "c3.g == 0x00 assert failure")
assert(c3.b == 0xFF, "c3.b == 0xFF assert failure")
assert(c3.a == 0x00, "c3.a == 0x00 assert failure")
print("-- OK")

local c4 = Color({ 222, 223, 224 })
assert(c4.r == 222, "c4.r == 222 assert failure")
assert(c4.g == 223, "c4.g == 223 assert failure")
assert(c4.b == 224, "c4.b == 224 assert failure")
assert(c4.a == 255, "c4.a == 255 assert failure")
print("-- OK")

local c5 = Color(99, 100, 101)
assert(c5.r == 99, "c5.r == 99 assert failure")
assert(c5.g == 100, "c5.g == 100 assert failure")
assert(c5.b == 101, "c5.b == 101 assert failure")
assert(c5.a == 255, "c5.a == 255 assert failure")
print("-- OK")

local c6 = Color(Color(100, 200, 55, 33))
assert(c6.r == 100, "c6.r == 100 assert failure")
assert(c6.g == 200, "c6.g == 200 assert failure")
assert(c6.b == 55, "c6.b == 55 assert failure")
assert(c6.a == 33, "c6.a == 33 assert failure")
print("-- OK")

c1,c2,c3,c4,c5,c6 = nil,nil,nil,nil,nil,nil
collectgarbage()



local cf = Color(0,0,0,0)
cf.r = 1
cf.g = 2
cf.b = 3
cf.a = 4

assert(cf.r == 1, "cf.r == 1 assert failure")
assert(cf.g == 2, "cf.g == 2 assert failure")
assert(cf.b == 3, "cf.b == 3 assert failure")
assert(cf.a == 4, "cf.a == 4 assert failure")
print("-- OK")



cf = nil
collectgarbage()


local ok;
ok = pcall(Color, -1,0,0,0)
assert(not ok, "not ok failure")
ok = pcall(Color, 0,-1,0,0)
assert(not ok, "not ok failure")
ok = pcall(Color, 0,0,-1,0)
assert(not ok, "not ok failure")
ok = pcall(Color, 0,0,0,-1)
assert(not ok, "not ok failure")

ok = pcall(Color, 256,0,0,0)
assert(not ok, "not ok failure")
ok = pcall(Color, 0,256,0,0)
assert(not ok, "not ok failure")
ok = pcall(Color, 0,0,256,0)
assert(not ok, "not ok failure")
ok = pcall(Color, 0,0,0,256)
assert(not ok, "not ok failure")
print("-- OK")



print("test_color.lua successfuly complete")