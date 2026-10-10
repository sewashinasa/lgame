
local lgame = require("lgame")

local Surface = lgame.Surface

local SRCALPHA = 0x00000001

local format = string.format
local assert = assert
local print = print

local s;
local bypp, bipp;
local possible_depth, possible_masks;
local m, rm, gm, bm, am;

s = Surface({ 100, 100 })
assert(s:get_width() == 100, "s:get_width() assert failure")
assert(s:get_height() == 100, "s:get_height() assert failure")
print("-- OK")

lgame.display.init()
lgame.display.set_mode({1,1})
s = Surface({ 500, 500 }, SRCALPHA)
m = s:get_masks()
bypp = s:get_bytesize()
bipp = s:get_bitsize()
am = m[4]
assert(am > 0, "am > 0 assert failure")
assert(bypp == 4, "bypp == 4 assert failure")
assert(bipp == 32, "bipp == 32 assert failure")
print("-- OK")
lgame.display.quit()

possible_depth = 
{
  8,12,15,16,24,32,
}

for i=1,#possible_depth do
  s=Surface({0,0}, nil, possible_depth[i])
  bipp=s:get_bitsize()
  bypp=s:get_bytesize()
  if (possible_depth[i] == 32 and bipp == 24 and bypp == 4) then goto continue end
  assert(bipp == possible_depth[i], format("bipp == possible_depth[%d] assert failure", i))
  ::continue::
end
print("-- OK")


possible_masks = 
{
  -- 12 bit
  {0x0F00,0x00F0,0x000F,0}, -- XRGB4444
  {0x000F,0x00F0,0x0F00,0}, -- XBGR4444

  -- 16 bit
  {0x7C00,0x03E0,0x001F,0}, -- XRGB1555
  {0x001F,0x03E0,0x7C00,0}, -- XBGR1555
  {0x0F00,0x00F0,0x000F,0xF000}, -- ARGB4444
  {0xF000,0x0F00,0x00F0,0x000F}, -- RGBA4444
  {0x000F,0x00F0,0x0F00,0xF000}, -- ABGR4444
  {0x00F0,0x0F00,0xF000,0x000F}, -- BGRA4444

  -- 24-32 bit
  {0x00FF0000,0x0000FF00,0x000000FF,0}, -- XRGB8888
  {0xFF000000,0x00FF0000,0x0000FF00,0}, -- RGBX8888
  {0x000000FF,0x0000FF00,0x00FF0000,0}, -- XBGR8888
  {0x0000FF00,0x00FF0000,0xFF000000,0}, -- BGRX8888
  {0x00FF0000,0x0000FF00,0x000000FF,0xFF000000}, -- ARGB8888
  {0xFF000000,0x00FF0000,0x0000FF00,0x000000FF}, -- RGBA8888
  {0x000000FF,0x0000FF00,0x00FF0000,0xFF000000}, -- ABGR8888
  {0x0000FF00,0x00FF0000,0xFF000000,0x000000FF}, -- BGRA8888
}

for i=1,#possible_masks do
  s=Surface({0,0}, nil, nil, possible_masks[i])
  m=s:get_masks()
  rm=m[1]
  gm=m[2]
  bm=m[3]
  am=m[4]
  --bypp=s:get_bytesize()
  --bipp=s:get_bitsize()
  assert(rm == possible_masks[i][1], format("rm == possible_masks[%d][1] assert failure", i))
  assert(gm == possible_masks[i][2], format("rm == possible_masks[%d][2] assert failure", i))
  assert(bm == possible_masks[i][3], format("rm == possible_masks[%d][3] assert failure", i))
  assert(am == possible_masks[i][4], format("rm == possible_masks[%d][4] assert failure", i))
end
print("-- OK")



print("test_surface.lua successfuly complete")