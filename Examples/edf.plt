import std/algo.plt
class Process
{
  var capacity = nil
  var period = nil
  var id = nil
  function __construct__(var id,var a,var b)
  {
    self.id = id
    self.capacity = a
    self.period = b
  }
  function __greaterthan__(var rhs)
  {
    return self.period > rhs.period
  }
}
function lcmPeriod(var p)
{
    #p is sorted
    var l = len(p)
    var max = algo::Max(p).period
    var i = max
    while(true)
    {
        var isLcm = true
        for(var j=0 to l-1 step 1)
        {
          if(i % p[j].period != 0)
          {
            isLcm = false
            break
          }
        }
        if(isLcm)
          return i
        i+=1
    }
    return nil
}
function scheduleEDF(var p)
{
    var lcm = lcmPeriod(p)
    var l = len(p)
    var exec = [0]*l # number of time units process
    var deadlines = [0]*l
    #executed in it's period
    var k = 0
    for(var i=0 to lcm-1 step 1)
    {
       for(var j=0 to l-1 step 1)
       {
        if(i% (p[j].period) == 0)
        {
          if(exec[j]!=p[j].capacity and i!=0)
          {
            printf("% missed deadline\n",p[j].id)
            return nil
          }
          exec[j] = 0 #reset period making process eligible for
          #execution again
          #calculate deadline
          deadlines[j] = i+p[j].period
        }
       }
       #pick a process
       var picked = nil
       var min = @INT_MAX
       for(var j=0 to l-1 step 1)
       {
         if(p[j].capacity > exec[j] and (deadlines[j] < min)) #process with ed
         {
            picked = j
            min = deadlines[j]
         }
       }
       if(picked == nil) #no process picked
         printf("%-%: IDLE\n",i,i+1)
       else
       {
         printf("%-%: %\n",i,i+1,p[picked].id)
         exec[picked]+=1
       }
    }
}
#[Process("T1",2,5),Process("T2",3,5)]#
#
var processes = [Process("T0",2,6),Process("T1",3,12),Process("T2",5,18),Process("T3",6,24)]#[Process("T1",3,20),Process("T2",2,5),Process("T3",2,10)]
scheduleEDF(processes)
