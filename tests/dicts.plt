var x = {"a": [5,3,1],"b" : {"a" : "b","c" : "d"},0xff : 0xf8}
println(x["a"])
println(x["b"]["a"])
println(x[0xff])

var dict = {1: "a",2: "b",3: "c",4: "d"}
dict.emplace(5,"e")
dict.emplace(5,"e") # won't do anything
var k = 1
println(len(dict))
while(k<=5)
{
  println(dict[k])
  k+=1
}
println(dict.hasKey(5))
println(dict.erase(5))
println(dict.hasKey(5))