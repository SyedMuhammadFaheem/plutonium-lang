var x = 30
namespace std
{
    var x = 40
    function f1()
    {
        println(x) #40
        println(std::x) # 40
    }
}
namespace std
{
    class Demo
    {
        var id = 0
    }
}

std::f1()
println(x)
println(std::Demo.id)