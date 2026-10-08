
local lf = {
	"./test_color.lua",
	"./test_rect.lua",
}

for i=1, #lf do
  print(string.format("Start %s test", string.sub(lf[i], 3, #lf[i])))
	dofile(lf[i])
  print()
end

print()
print()
print()
print("All tests passed successfully")