class A
{
  private var x = nil
  function __construct__(var x)
    self.x = x
  function f1()
  {
    println("this is A.f1()")
  }
  function f2()
  {
    println("this is A.f2()")
  }
}
class B extends A
{
  function f2()
  {
    println("this is B.f2()")
  }
}
var a = A(10)
var b = B(30)
a.f1()
a.f2()
b.f1()
b.f2()