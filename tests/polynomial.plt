class polynomial
{
  private var coeff = nil
  private var exp = nil
  function __construct__(var c,var e)
  {
    self.coeff = c
    self.exp = e
  }
  function add(var rhs)
  {
    var c = self.coeff
    var e = self.exp
    var k = 0
    foreach(var E: rhs.exp)
    {
      var z = e.find(E)
      if(z==nil)
      {
        c.push(rhs.coeff[k])
        e.push(E)
      }
      else
      {
        c[z]+=rhs.coeff[k]
      }
      k+=1
    }
    self.coeff = c
    self.exp = e
  }
  function print()
  {
    var k = 0
    while(k<len(self.coeff))
    {
      if(self.exp[k]==0)
        print(self.coeff[k])
      else
      {
        if(self.coeff[k]==1)
        {
          if(self.exp[k]==1)
            print("x")
          else
            print("x^",self.exp[k])
        }
        else
        {
          if(self.exp[k]==1)
          print(self.coeff[k],"x")
          else
            print(self.coeff[k],"x^",self.exp[k])
        }
      }
      if(k!=len(self.coeff)-1)
        print(" + ")
      k+=1
    }
    println()
  }
}
var p1 = polynomial([1,2],[2,1])
var p2 = polynomial([1],[0])
p1.print()
p2.print()
p1.add(p2)
p1.print()