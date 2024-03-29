// Generated case design for PCB/ASR33.kicad_pcb
// By https://github.com/revk/PCBCase
// Generated 2023-03-27 16:36:13
// title:	ASR33 driver
// date:	${DATE}
// rev:	1
// comment:	www.me.uk
// comment:	@TheRealRevK
//

// Globals
margin=0.500000;
overlap=2.000000;
lip=0.000000;
casebase=2.000000;
casetop=7.000000;
casewall=3.000000;
fit=0.000000;
edge=2.000000;
pcbthickness=0.800000;
nohull=false;
hullcap=1.000000;
hulledge=1.000000;
useredge=false;

module outline(h=pcbthickness,r=0){linear_extrude(height=h)offset(r=r)polygon(points=[[38.750000,8.500000],[38.750000,27.000000],[0.000000,27.000000],[0.000000,0.000000],[45.000000,0.000000],[45.000000,8.500000]],paths=[[0,1,2,3,4,5]]);}

module pcb(h=pcbthickness,r=0){linear_extrude(height=h)offset(r=r)polygon(points=[[38.750000,8.500000],[38.750000,27.000000],[0.000000,27.000000],[0.000000,0.000000],[45.000000,0.000000],[45.000000,8.500000]],paths=[[0,1,2,3,4,5]]);}
spacing=61.000000;
pcbwidth=45.000000;
pcblength=27.000000;
// Populated PCB
module board(pushed=false,hulled=false){
translate([6.580000,20.330000,0.800000])m1(pushed,hulled); // RevK:D_1206_ D_1206_3216Metric (back)
translate([4.400000,13.500000,0.800000])rotate([0,0,-90.000000])translate([0.000000,-1.050000,0.000000])rotate([90.000000,-0.000000,-0.000000])m3(pushed,hulled); // RevK:USC16-TR CSP-USC16-TR (back)
translate([25.000000,10.250000,0.800000])translate([2.100000,1.400000,-1.300000])rotate([90.000000,-0.000000,-90.000000])m6(pushed,hulled); // RevK:SOT-457 SOT457 (back)
translate([25.000000,20.750000,0.800000])m8(pushed,hulled); // RevK:R_0603 R_0603_1608Metric (back)
translate([22.000000,25.250000,0.800000])rotate([0,0,90.000000])m8(pushed,hulled); // RevK:R_0603 R_0603_1608Metric (back)
translate([18.000000,24.000000,0.800000])rotate([0,0,-90.000000])m9(pushed,hulled); // RevK:QFN-20-1EP_4x4mm_P0.5mm_EP2.5x2.5mm QFN-20-1EP_4x4mm_P0.5mm_EP2.5x2.5mm (back)
translate([21.000000,13.000000,0.800000])rotate([0,0,90.000000])m10(pushed,hulled); // Package_SO:SO-4_4.4x3.6mm_P2.54mm SO-4_4.4x3.6mm_P2.54mm (back)
translate([10.250000,17.500000,0.800000])m12(pushed,hulled); // RevK:C_0603 C_0603_1608Metric (back)
translate([0.750000,22.500000,0.800000])rotate([0,0,90.000000])m8(pushed,hulled); // RevK:R_0603 R_0603_1608Metric (back)
translate([2.200000,23.250000,0.800000])rotate([0,0,90.000000])m12(pushed,hulled); // RevK:C_0603 C_0603_1608Metric (back)
translate([12.500000,7.200000,0.800000])m16(pushed,hulled,3); // RevK:PTSM-HH1-3-RA PTSM-HH1-3-RA (back)
translate([14.000000,25.750000,0.800000])m12(pushed,hulled); // RevK:C_0603 C_0603_1608Metric (back)
translate([15.250000,10.250000,0.800000])translate([2.100000,1.400000,-1.300000])rotate([90.000000,-0.000000,-90.000000])m6(pushed,hulled); // RevK:SOT-457 SOT457 (back)
translate([8.250000,11.000000,0.800000])rotate([0,0,90.000000])m8(pushed,hulled); // RevK:R_0603 R_0603_1608Metric (back)
translate([9.700000,19.950000,0.800000])rotate([0,0,90.000000])m18(pushed,hulled); // RevK:C_0805_ C_0805_2012Metric (back)
translate([0.750000,25.500000,0.800000])rotate([0,0,-90.000000])m8(pushed,hulled); // RevK:R_0603 R_0603_1608Metric (back)
translate([8.250000,16.000000,0.800000])rotate([0,0,90.000000])m8(pushed,hulled); // RevK:R_0603 R_0603_1608Metric (back)
translate([11.250000,19.500000,0.800000])rotate([0,0,90.000000])m8(pushed,hulled); // RevK:R_0603 R_0603_1608Metric (back)
translate([33.056750,20.250000,0.800000])rotate([0,0,-90.000000])m19(pushed,hulled); // RevK:ESP32-PICO-MINI-02 ESP32-PICO-MINI-02 (back)
translate([35.750000,10.250000,0.800000])m20(pushed,hulled); // Package_TO_SOT_SMD:SOT-363_SC-70-6 SOT-363_SC-70-6 (back)
translate([14.000000,22.000000,0.800000])m12(pushed,hulled); // RevK:C_0603 C_0603_1608Metric (back)
translate([25.000000,23.250000,0.800000])m8(pushed,hulled); // RevK:R_0603 R_0603_1608Metric (back)
translate([25.000000,25.000000,0.800000])translate([0.000000,-0.100000,0.400000])rotate([-90.000000,-0.000000,-0.000000])m22(pushed,hulled); // RevK:MHS190RGBCT LED_0603_1608Metric (back)
translate([10.750000,14.750000,0.800000])rotate([0,0,90.000000])m1(pushed,hulled); // RevK:D_1206_ D_1206_3216Metric (back)
translate([14.000000,23.250000,0.800000])m8(pushed,hulled); // RevK:R_0603 R_0603_1608Metric (back)
translate([11.250000,22.250000,0.800000])rotate([0,0,90.000000])m8(pushed,hulled); // RevK:R_0603 R_0603_1608Metric (back)
translate([11.750000,10.500000,0.800000])rotate([0,0,90.000000])m24(pushed,hulled); // Package_TO_SOT_SMD:SOT-23 SOT-23 (back)
translate([4.300000,23.500000,0.800000])rotate([-0.000000,-0.000000,-90.000000])m26(pushed,hulled); // RevK:SOT-23-Thin-6-Reg SOT-23-6 (back)
translate([22.500000,7.200000,0.800000])m16(pushed,hulled,3); // RevK:PTSM-HH1-3-RA PTSM-HH1-3-RA (back)
translate([25.000000,22.000000,0.800000])m8(pushed,hulled); // RevK:R_0603 R_0603_1608Metric (back)
translate([14.000000,24.500000,0.800000])rotate([0,0,180.000000])m8(pushed,hulled); // RevK:R_0603 R_0603_1608Metric (back)
translate([3.750000,7.200000,0.800000])m16(pushed,hulled,2); // RevK:PTSM-HH1-3-RA PTSM-HH1-3-RA (back)
translate([8.200000,23.500000,0.800000])rotate([-0.000000,-0.000000,-90.000000])m29(pushed,hulled); // RevK:L_4x4_ TYA4020 (back)
translate([3.450000,19.950000,0.800000])rotate([0,0,90.000000])m18(pushed,hulled); // RevK:C_0805_ C_0805_2012Metric (back)
translate([15.500000,16.750000,0.800000])rotate([0,0,90.000000])translate([-3.250000,2.250000,0.000000])m31(pushed,hulled); // RevK:SW_PUSH_6mm_PTH SW_PUSH_6mm (back)
translate([40.000000,7.200000,0.800000])m16(pushed,hulled,3); // RevK:PTSM-HH1-3-RA PTSM-HH1-3-RA (back)
translate([31.250000,10.250000,0.800000])m20(pushed,hulled); // Package_TO_SOT_SMD:SOT-363_SC-70-6 SOT-363_SC-70-6 (back)
translate([31.250000,7.200000,0.800000])m16(pushed,hulled,2); // RevK:PTSM-HH1-3-RA PTSM-HH1-3-RA (back)
translate([22.000000,21.750000,0.800000])rotate([0,0,-90.000000])m12(pushed,hulled); // RevK:C_0603 C_0603_1608Metric (back)
}

module b(cx,cy,z,w,l,h){translate([cx-w/2,cy-l/2,z])cube([w,l,h]);}
module m1(pushed=false,hulled=false)
{ // RevK:D_1206_ D_1206_3216Metric
b(0,0,0,4.4,1.75,1.2); // Pad size
}

module m3(pushed=false,hulled=false)
{ // RevK:USC16-TR CSP-USC16-TR
rotate([-90,0,0])translate([-4.47,-3.84,0])
{
	translate([1.63,0,1.63])
	rotate([-90,0,0])
	hull()
	{
		cylinder(d=3.26,h=7.75,$fn=24);
		translate([5.68,0,0])
		cylinder(d=3.26,h=7.75,$fn=24);
	}
	translate([0,6.65,0])cube([8.94,1.1,1.63]);
	translate([0,2.2,0])cube([8.94,1.6,1.63]);
	if(!hulled)
	{
		// Plug
		translate([1.63,-20,1.63])
		rotate([-90,0,0])
		hull()
		{
			cylinder(d=2.5,h=21,$fn=24);
			translate([5.68,0,0])
			cylinder(d=2.5,h=21,$fn=24);
		}
		translate([1.63,-22.5,1.63])
		rotate([-90,0,0])
		hull()
		{
			cylinder(d=7,h=21,$fn=24);
			translate([5.68,0,0])
			cylinder(d=7,h=21,$fn=24);
		}
	}
}

}

module m6(pushed=false,hulled=false)
{ // RevK:SOT-457 SOT457
b(0,0,0,1.15,2.0,1.1);
b(0,0,0,2.1,2.0,0.6);
}

module m8(pushed=false,hulled=false)
{ // RevK:R_0603 R_0603_1608Metric
b(0,0,0,2.8,0.95,0.5); // Pad size
}

module m9(pushed=false,hulled=false)
{ // RevK:QFN-20-1EP_4x4mm_P0.5mm_EP2.5x2.5mm QFN-20-1EP_4x4mm_P0.5mm_EP2.5x2.5mm
cube([4,4,1],center=true);
}

module m10(pushed=false,hulled=false)
{ // Package_SO:SO-4_4.4x3.6mm_P2.54mm SO-4_4.4x3.6mm_P2.54mm
b(0,0,0,4.55+0.25,3.7+0.25,2.1+0.2);
b(0,0,0,7.0+0.44,3,1.5);
}

module m12(pushed=false,hulled=false)
{ // RevK:C_0603 C_0603_1608Metric
b(0,0,0,1.6,0.95,0.2); // Pad size
b(0,0,0,1.6,0.8,1); // Chip
}

module m16(pushed=false,hulled=false,n=0)
{ // RevK:PTSM-HH1-3-RA PTSM-HH1-3-RA
hull()
{ // Socket
	b(0,-7.5/2+0.3,0,1.7+n*2.5,7.5,4);
	b(0,-7.5/2+0.3,0,1.7+n*2.5-2,7.5,5);
}
// Plug
b(0,-10.5/2-7.5+0.3,0,1.1+n*2,10.5,5);
}

module m18(pushed=false,hulled=false)
{ // RevK:C_0805_ C_0805_2012Metric
b(0,0,0,2,1.45,0.2); // Pad size
b(0,0,0,2,1.2,1); // Chip
}

module m19(pushed=false,hulled=false)
{ // RevK:ESP32-PICO-MINI-02 ESP32-PICO-MINI-02
translate([-13.2/2,-16.6/2+2.7,0])
{
	if(!hulled)cube([13.2,16.6,0.8]);
	cube([13.2,11.2,2.4]);
}
}

module m20(pushed=false,hulled=false)
{ // Package_TO_SOT_SMD:SOT-363_SC-70-6 SOT-363_SC-70-6
b(0,0,0,1.15,2.0,1.1);
b(0,0,0,2.1,2.0,0.6);
}

module m22(pushed=false,hulled=false)
{ // RevK:MHS190RGBCT LED_0603_1608Metric
b(0,0,0,1.6,0.8,0.25);
b(0,0,0,1.2,0.8,0.55);
b(0,0,0,0.8,0.8,0.95);
if(!hulled&&pushed)b(0,0,0,1,1,20);
}

module m24(pushed=false,hulled=false)
{ // Package_TO_SOT_SMD:SOT-23 SOT-23
b(0,0,0,2.92,1.3,1.2);
b(0,0,0,2.27,2.4,0.6);
}

module m26(pushed=false,hulled=false)
{ // RevK:SOT-23-Thin-6-Reg SOT-23-6
b(0,0,0,3.05,3.05,0.5);
b(0,0,0,1.45,3.05,1.1);
}

module m29(pushed=false,hulled=false)
{ // RevK:L_4x4_ TYA4020
b(0,0,0,4,4,2.1);
}

module m31(pushed=false,hulled=false)
{ // RevK:SW_PUSH_6mm_PTH SW_PUSH_6mm
translate([3.25,-2.25,0])
{
	b(0,0,0,6,6,4);
	if(!hulled&&pushed)cylinder(d=4,h=100);
	for(x=[-3.25,3.25])for(y=[-2.25,2.25])translate([x,y,-2])cylinder(d=2,h=4);
}
}

height=casebase+pcbthickness+casetop;
$fn=48;

module boardh(pushed=false)
{ // Board with hulled parts
	union()
	{
		if(!nohull)intersection()
		{
			translate([0,0,hullcap-casebase])outline(casebase+pcbthickness+casetop-hullcap*2,-hulledge);
			hull()board(pushed,true);
		}
		board(pushed,false);
		pcb();
	}
}

module boardf()
{ // This is the board, but stretched up to make a push out in from the front
	render()
	{
		intersection()
		{
			translate([-casewall-1,-casewall-1,-casebase-1]) cube([pcbwidth+casewall*2+2,pcblength+casewall*2+2,height+2]);
			union()
			{
				minkowski()
				{
					boardh(true);
					cylinder(h=height+100,d=margin,$fn=8);
				}
				board(false,false);
			}
		}
	}
}

module boardb()
{ // This is the board, but stretched down to make a push out in from the back
	render()
	{
		intersection()
		{
			translate([-casewall-1,-casewall-1,-casebase-1]) cube([pcbwidth+casewall*2+2,pcblength+casewall*2+2,height+2]);
			union()
			{
				minkowski()
				{
					boardh(true);
					translate([0,0,-height-100])
					cylinder(h=height+100,d=margin,$fn=8);
				}
				board(false,false);
			}
		}
	}
}

module boardm()
{
	render()
	{
 		minkowski()
 		{
			translate([0,0,-margin/2])cylinder(d=margin,h=margin,$fn=8);
 			boardh(false);
		}
		//intersection()
    		//{
        		//translate([0,0,-(casebase-hullcap)])pcb(pcbthickness+(casebase-hullcap)+(casetop-hullcap));
        		//translate([0,0,-(casebase-hullcap)])outline(pcbthickness+(casebase-hullcap)+(casetop-hullcap));
			boardh(false);
    		//}
 	}
}

module pcbh(h=pcbthickness,r=0)
{ // PCB shape for case
	if(useredge)outline(h,r);
	else hull()outline(h,r);
}

module pyramid()
{ // A pyramid
 polyhedron(points=[[0,0,0],[-height,-height,height],[-height,height,height],[height,height,height],[height,-height,height]],faces=[[0,1,2],[0,2,3],[0,3,4],[0,4,1],[4,3,2,1]]);
}

module wall(d=0)
{ // The case wall
	translate([0,0,-casebase-d])
	{
		if(useredge)
			intersection()
			{
				pcb(height+d*2,margin/2+d);
				pcbh(height+d*2,margin/2+d);
			}
		else pcbh(height+d*2,margin/2+d);
	}
}

module cutf()
{ // This cut up from base in the wall
	intersection()
	{
		boardf();
		difference()
		{
			translate([-casewall+0.01,-casewall+0.01,-casebase+0.01])cube([pcbwidth+casewall*2-0.02,pcblength+casewall*2-0.02,casebase+overlap+lip]);
			wall();
			boardb();
		}
	}
}

module cutb()
{ // The cut down from top in the wall
	intersection()
	{
		boardb();
		difference()
		{
			translate([-casewall+0.01,-casewall+0.01,0.01])cube([pcbwidth+casewall*2-0.02,pcblength+casewall*2-0.02,casetop+pcbthickness]);
			wall();
			boardf();
		}
	}
}

module cutpf()
{ // the push up but pyramid
	render()
	intersection()
	{
		minkowski()
		{
			pyramid();
			cutf();
		}
		difference()
		{
			translate([-casewall-0.01,-casewall-0.01,-casebase-0.01])cube([pcbwidth+casewall*2+0.02,pcblength+casewall*2+0.02,casebase+overlap+lip+0.02]);
			wall();
			boardh(true);
		}
		translate([-casewall,-casewall,-casebase])case();
	}
}

module cutpb()
{ // the push down but pyramid
	render()
	intersection()
	{
		minkowski()
		{
			scale([1,1,-1])pyramid();
			cutb();
		}
		difference()
		{
			translate([-casewall-0.01,-casewall-0.01,-0.01])cube([pcbwidth+casewall*2+0.02,pcblength+casewall*2+0.02,casetop+pcbthickness+0.02]);
			wall();
			boardh(true);
		}
		translate([-casewall,-casewall,-casebase])case();
	}
}

module case()
{ // The basic case
	hull()
	{
		translate([casewall,casewall,0])pcbh(height,casewall-edge);
		translate([casewall,casewall,edge])pcbh(height-edge*2,casewall);
	}
}

module cut(d=0)
{ // The cut point in the wall
	translate([casewall,casewall,casebase+lip])pcbh(casetop+pcbthickness-lip+1,casewall/2+d/2+margin/4);
}

module base()
{ // The base
	difference()
	{
		case();
		difference()
		{
			union()
			{
				translate([-1,-1,casebase+overlap+lip])cube([pcbwidth+casewall*2+2,pcblength+casewall*2+2,casetop+1]);
				cut(fit);
			}
		}
		translate([casewall,casewall,casebase])boardf();
		translate([casewall,casewall,casebase])boardm();
		translate([casewall,casewall,casebase])cutpf();
	}
	translate([casewall,casewall,casebase])cutpb();
}

module top()
{
	translate([0,pcblength+casewall*2,height])rotate([180,0,0])
	{
		difference()
		{
			case();
			difference()
			{
				translate([-1,-1,-1])cube([pcbwidth+casewall*2+2,pcblength+casewall*2+2,casebase+overlap+lip-margin+1]);
				cut(-fit);
			}
			translate([casewall,casewall,casebase])boardb();
			translate([casewall,casewall,casebase])boardm();
			translate([casewall,casewall,casebase])cutpb();
		}
		translate([casewall,casewall,casebase])cutpf();
	}
}

module test()
{
	translate([0*spacing,0,0])base();
	translate([1*spacing,0,0])top();
	translate([2*spacing,0,0])pcb();
	translate([3*spacing,0,0])outline();
	translate([4*spacing,0,0])wall();
	translate([5*spacing,0,0])board();
	translate([6*spacing,0,0])board(false,true);
	translate([7*spacing,0,0])board(true);
	translate([8*spacing,0,0])boardh();
	translate([9*spacing,0,0])boardf();
	translate([10*spacing,0,0])boardb();
	translate([11*spacing,0,0])cutpf();
	translate([12*spacing,0,0])cutpb();
	translate([13*spacing,0,0])cutf();
	translate([14*spacing,0,0])cutb();
	translate([15*spacing,0,0])case();
}

module parts()
{
	base();
	translate([spacing,0,0])top();
}
difference(){top();translate([3,34,-1])cube([45,30,10]);}
translate([spacing,0,0])base();
