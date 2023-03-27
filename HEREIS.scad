// ASR33 HERE-IS answerback wheel
// Expects to have a 50mm x 4mm diameter rod inserted

tol=0.4; // allow for rod clerance for print tollerance
outer=25.4;     // Main outer size
inner=0.6*25.4; // Smallest inner for stop control
ratchet=0.93*25.4; // ratchet size (smaller than outer)
gap=2.5; // Gap between rackets
dip=0.75*25.4;  // Depth of dip for contacts
dipsize=3; // Size of dip
spacing=1.75*25.4/11; // spacing between cylinders
chars=21; // characters (including start) should be 21 - may work with 22
rod=4+tol;  // diameter of rod through the middle (hole)
collar=4.7; // Far end collar (usually too small to print)
slot=5; // Size of stop slot
$fn=200;

// ASCII character codes for start and 20 characters
START=chr(256); // suppress, and a cam stop at +6
SKIP=chr(256+255); // suppress and all bits, not a cam stop
CR=chr(13);
LF=chr(10);
WRU=chr(5);
ACK=chr(6);
RO=chr(127);
// Stadard format starts CR/LF/RO, has CR/LF at end and ACK
codes=str(START, // Non coded start (and cam stop at +6)
    CR,LF,RO, // Standard header
    // 14 chars available
    "asr33.revk.uk",
    CR,LF,ACK, // Standard trailer
    SKIP,SKIP,SKIP,SKIP,SKIP,SKIP,SKIP,SKIP,SKIP,SKIP,SKIP,SKIP,SKIP,SKIP); // Padding for shorter name
// Sending WRU instead of ACK would be evil

// 4th pos is suppress
// 5th pos is cam stop but has to offset 6 characters
// 6th pos is driving lever 
map=[128,64,32,256,0,0,16,8,4,2,1];
function getbit(v,s)=((v/s)%2>=1?1:0);
function parity(v)=(getbit(v,64)+getbit(v,32)+getbit(v,16)+getbit(v,8)+getbit(v,4)+getbit(v,2)+getbit(v,1))%2;
function code(p)=ord(codes[p%chars]);
function bit(p,b)=(b?map[b]?!getbit(code(p),map[b]):1:!parity(code(p)));

translate([0,0,spacing*11])
rotate([180,0,0])
difference()
{
 union()
 {
  cylinder(h=spacing*11,d=ratchet);
  hull()
  {
   cylinder(h=spacing,d=outer);
   translate([0,0,spacing*5-(outer-ratchet)/4])
   cylinder(h=(outer-ratchet)/4,d1=outer,d2=ratchet);
  }
  hull()
  {
   translate([0,0,spacing*6])
   cylinder(h=(outer-ratchet)/4,d1=ratchet,d2=outer);
   translate([0,0,spacing*10])
   cylinder(h=spacing,d=outer);
  }
  translate([0,0,-spacing])
  cylinder(h=spacing*2,d=collar);
 }
 for(a=[0:chars-1])
 if(code(a)==256)
 rotate([0,0,360*(a+6)/21]) // 21 in case chars is more
 translate([-slot/2,inner/2,4*spacing])
 hull()
 {
  translate([slot*0.1,0,spacing/3])
  cube([slot*.8,1,spacing/3]);
  translate([0,outer/2-inner/2,spacing*0.1])
  cube([slot,1,spacing*.8]);
 }
 for(a=[0:chars-1])
 rotate([0,0,360*(a+0.5)/chars-2])
 translate([-gap/2,inner/2,5*spacing])
 hull()
 {
  translate([gap/3,0,spacing/3])
  cube([gap/3,1,spacing/3]);
  translate([0,ratchet/2-inner/2,0.01])
  cube([gap,1,spacing-0.02]);
 }
 for(a=[0:chars-1])
 for(aa=[0:3])
 rotate([0,0,360*(a+aa/4)/chars])
 for(b=[0:10])
 if(!bit(a,b)&&(!aa||!bit(a+1,b)))
 translate([0,dip/2,spacing*b])
 hull()
 {
    translate([-gap/2+gap/3,0,spacing/3])
    cube([gap/3,1,spacing/3]);
    translate([-dipsize,outer/2-dip/2,spacing*0.1])
    cube([dipsize*2,1,spacing*0.8]);
 }
 // rod up the middle
 translate([0,0,-spacing*2])cylinder(h=spacing*14,d=rod);
}