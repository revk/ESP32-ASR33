// Generated case design for ASR33/ASR33.kicad_pcb
// By https://github.com/revk/PCBCase
// Generated 2025-01-14 14:59:03
// title:	ASR33 driver
// rev:	1
// comment:	www.me.uk
// comment:	@TheRealRevK
//

// Globals
margin=0.200000;
lip=2.000000;
casebottom=2.000000;
casetop=7.000000;
casewall=3.000000;
fit=0.000000;
edge=2.000000;
pcbthickness=1.200000;
nohull=false;
hullcap=1.000000;
hulledge=1.000000;
useredge=false;

module outline(h=pcbthickness,r=0){linear_extrude(height=h)offset(r=r)polygon(points=[[39.100000,24.500000],[0.000000,24.500000],[0.000000,0.000000],[44.500000,0.000000],[44.500000,8.400000],[39.600000,8.400000],[39.408658,8.438061],[39.246447,8.546447],[39.138061,8.708658],[39.100000,8.900000]],paths=[[0,1,2,3,4,5,6,7,8,9]]);}

module pcb(h=pcbthickness,r=0){linear_extrude(height=h)offset(r=r)polygon(points=[[39.100000,24.500000],[0.000000,24.500000],[0.000000,0.000000],[44.500000,0.000000],[44.500000,8.400000],[39.600000,8.400000],[39.408658,8.438061],[39.246447,8.546447],[39.138061,8.708658],[39.100000,8.900000]],paths=[[0,1,2,3,4,5,6,7,8,9]]);}
spacing=60.500000;
pcbwidth=44.500000;
pcblength=24.500000;
// Parts to go on PCB (top)
module parts_top(part=false,hole=false,block=false){
// Missing model U4.1 SOT457
translate([12.000000,21.500000,1.200000])m0(part,hole,block,casetop); // RevK:R_0402_ R_0402_1005Metric (back)
translate([12.000000,17.600000,1.200000])m1(part,hole,block,casetop); // RevK:C_0603_ C_0603_1608Metric (back)
translate([34.700000,8.100000,1.200000])rotate([0,0,180.000000])m2(part,hole,block,casetop); // RevK:C_0402 C_0402_1005Metric (back)
translate([17.200000,16.900000,1.200000])rotate([0,0,90.000000])m3(part,hole,block,casetop); // U5 (back)
translate([12.200000,7.100000,1.200000])m4(part,hole,block,casetop,3); // J5 (back)
// Missing model U3.1 SOT457
translate([5.430000,13.400000,1.200000])rotate([0,0,-90.000000])translate([0.000000,-2.400000,0.000000])rotate([90.000000,-0.000000,-0.000000])m5(part,hole,block,casetop); // RevK:USB-C-Socket-H CSP-USC16-TR (back)
translate([8.600000,11.300000,1.200000])rotate([0,0,180.000000])m0(part,hole,block,casetop); // RevK:R_0402_ R_0402_1005Metric (back)
translate([6.450000,19.400000,1.200000])rotate([0,0,180.000000])m6(part,hole,block,casetop); // D1 (back)
translate([22.600000,18.200000,1.200000])rotate([0,0,180.000000])m7(part,hole,block,casetop); // D7 (back)
translate([18.500000,23.000000,1.200000])rotate([0,0,90.000000])m2(part,hole,block,casetop); // RevK:C_0402 C_0402_1005Metric (back)
translate([8.600000,15.200000,1.200000])m0(part,hole,block,casetop); // RevK:R_0402_ R_0402_1005Metric (back)
translate([13.800000,21.500000,1.200000])m0(part,hole,block,casetop); // RevK:R_0402_ R_0402_1005Metric (back)
translate([12.000000,19.600000,1.200000])m8(part,hole,block,casetop); // RevK:SOT-23-6-MD8942 SOT-23-6 (back)
translate([31.350000,16.500000,1.200000])rotate([0,0,-90.000000])m9(part,hole,block,casetop); // U8 (back)
translate([12.000000,15.000000,1.200000])rotate([-0.000000,-0.000000,-90.000000])m10(part,hole,block,casetop); // RevK:L_4x4_ TYA4020 (back)
translate([10.200000,21.500000,1.200000])m0(part,hole,block,casetop); // RevK:R_0402_ R_0402_1005Metric (back)
translate([11.850000,10.537500,1.200000])rotate([0,0,90.000000])m11(part,hole,block,casetop); // Q1 (back)
translate([22.625000,17.350000,1.200000])rotate([0,0,180.000000])m7(part,hole,block,casetop); // D7 (back)
translate([22.200000,7.100000,1.200000])m4(part,hole,block,casetop,3); // J5 (back)
translate([22.600000,13.300000,1.200000])rotate([0,0,180.000000])m7(part,hole,block,casetop); // D7 (back)
translate([3.450000,7.100000,1.200000])m4(part,hole,block,casetop,2); // J5 (back)
translate([27.200000,8.100000,1.200000])m0(part,hole,block,casetop); // RevK:R_0402_ R_0402_1005Metric (back)
translate([19.400000,23.000000,1.200000])rotate([0,0,90.000000])m2(part,hole,block,casetop); // RevK:C_0402 C_0402_1005Metric (back)
translate([14.500000,18.300000,1.200000])rotate([0,0,90.000000])m1(part,hole,block,casetop); // RevK:C_0603_ C_0603_1608Metric (back)
translate([21.950000,22.450000,1.200000])rotate([0,0,45.000000])m12(part,hole,block,casetop); // RevK:SMD1010+ SMD1010 (back)
translate([4.300000,22.400000,1.200000])m13(part,hole,block,casetop); // SW1 (back)
translate([39.700000,7.100000,1.200000])m4(part,hole,block,casetop,3); // J5 (back)
translate([22.600000,14.800000,1.200000])rotate([0,0,180.000000])m7(part,hole,block,casetop); // D7 (back)
translate([30.950000,7.100000,1.200000])m4(part,hole,block,casetop,2); // J5 (back)
translate([9.500000,18.300000,1.200000])rotate([0,0,90.000000])m1(part,hole,block,casetop); // RevK:C_0603_ C_0603_1608Metric (back)
}

parts_top=14;
// Parts to go on PCB (bottom)
module parts_bottom(part=false,hole=false,block=false){
}

parts_bottom=0;
module b(cx,cy,z,w,l,h){translate([cx-w/2,cy-l/2,z])cube([w,l,h]);}
module m0(part=false,hole=false,block=false,height)
{ // RevK:R_0402_ R_0402_1005Metric
// 0402 Resistor
if(part)
{
	b(0,0,0,1.5,0.65,0.2); // Pad size
	b(0,0,0,1.0,0.5,0.5); // Chip
}
}

module m1(part=false,hole=false,block=false,height)
{ // RevK:C_0603_ C_0603_1608Metric
// 0603 Capacitor
if(part)
{
	b(0,0,0,1.6,0.8,1); // Chip
	b(0,0,0,1.6,0.95,0.2); // Pad size
}
}

module m2(part=false,hole=false,block=false,height)
{ // RevK:C_0402 C_0402_1005Metric
// 0402 Capacitor
if(part)
{
	b(0,0,0,1.0,0.5,1); // Chip
	b(0,0,0,1.5,0.65,0.2); // Pad size
}
}

module m3(part=false,hole=false,block=false,height)
{ // U5
// SO-4_4.4x3.6mm_P2.54mm 
if(part)
{
	b(0,0,0,4.55,3.75,2.1); // Part
	b(0,0,0,7.0,3,1.5); // Pins
}
}

module m4(part=false,hole=false,block=false,height,N=0)
{ // J5
// PTSM socket
if(part)
{
	hull()
	{
		b(0,-7.5/2+0.3,0,1.7+N*2.5,7.5,4);
		b(0,-7.5/2+0.3,0,1.7+N*2.5-2,7.5,5);
	}
	// Pins
	for(p=[0:N-1])translate([-2.5*(N-1)/2+p*2.5,0,-2.1])cylinder(r1=0.3,r2=1,h=2.1);
}
if(hole)
{
	b(0,-10.5/2-7.5+0.3,0,1.1+N*2.5,10.5,5);
}
}

module m5(part=false,hole=false,block=false,height)
{ // RevK:USB-C-Socket-H CSP-USC16-TR
// USB connector
rotate([-90,0,0])translate([-4.47,-3.84,0])
{
	if(part)
	{
		b(4.47,7,0,7,2,0.2);	// Pads
		translate([1.63,-0.2,1.63])
		rotate([-90,0,0])
		hull()
		{
			cylinder(d=3.26,h=7.55,$fn=24);
			translate([5.68,0,0])
			cylinder(d=3.26,h=7.55,$fn=24);
		}
		translate([0,6.2501,0])cube([8.94,1.1,1.6301]);
		translate([0,1.7,0])cube([8.94,1.6,1.6301]);
	}
	if(hole)
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

module m6(part=false,hole=false,block=false,height)
{ // D1
// SOD-123 Diode
if(part)
{
	b(0,0,0,2.85,1.8,1.35); // part
	b(0,0,0,4.2,1.2,0.7); // pads
}
}

module m7(part=false,hole=false,block=false,height)
{ // D7
// DFN1006-2L
if(part)
{
	b(0,0,0,1.0,0.6,0.45); // Chip
}
}

module m8(part=false,hole=false,block=false,height)
{ // RevK:SOT-23-6-MD8942 SOT-23-6
// SOT-23-6
if(part)
{
	b(0,0,0,1.726,3.026,1.2); // Part
	b(0,0,0,3.6,2.5,0.5); // Pins
}
}

module m9(part=false,hole=false,block=false,height)
{ // U8
// ESP32-S3-MINI-1
translate([-15.4/2,-15.45/2,0])
{
	if(part)
	{
		cube([15.4,20.5,0.8]);
		translate([0.7,0.5,0])cube([14,13.55,2.4]);
	}
	if(hole)
	{
		cube([15.4,20.5,0.8]);
	}
}
}

module m10(part=false,hole=false,block=false,height)
{ // RevK:L_4x4_ TYA4020
// 4x4 Inductor
if(part)
{
	b(0,0,0,4,4,3);
}
}

module m11(part=false,hole=false,block=false,height)
{ // Q1
// SOT-23
if(part)
{
	b(0,0,0,1.4,3.0,1.1); // Body
	b(-0.9375,-0.95,0,1.475,0.6,0.5); // Pad
	b(-0.9375,0.95,0,1.475,0.6,0.5); // Pad
	b(0.9375,0,0,1.475,0.6,0.5); // Pad
}
}

module m12(part=false,hole=false,block=false,height)
{ // RevK:SMD1010+ SMD1010
// 1x1mm LED
if(part)
{
        b(0,0,0,1.2,1.2,.8);
}
if(hole)
{
        hull()
        {
                b(0,0,.8,1.2,1.2,1);
                translate([0,0,height])cylinder(d=2,h=1,$fn=16);
        }
}
if(block)
{
        hull()
        {
                b(0,0,0,2.4,2.4,1);
                translate([0,0,height])cylinder(d=4,h=1,$fn=16);
        }
}
}

module m13(part=false,hole=false,block=false,height)
{ // SW1
if(part)
{
	b(0,0,0,6.1,3.6,1.8);
	b(0,0,0,7.9,0.6,0.5);
}
if(hole)
{
	b(0,0,0,2.65,1.25,height+1);
}
}

// Generate PCB casework

height=casebottom+pcbthickness+casetop;
$fn=48;

module pyramid()
{ // A pyramid
 polyhedron(points=[[0,0,0],[-height,-height,height],[-height,height,height],[height,height,height],[height,-height,height]],faces=[[0,1,2],[0,2,3],[0,3,4],[0,4,1],[4,3,2,1]]);
}


module pcb_hulled(h=pcbthickness,r=0)
{ // PCB shape for case
	if(useredge)outline(h,r);
	else hull()outline(h,r);
}

module solid_case(d=0)
{ // The case wall
	hull()
        {
                translate([0,0,-casebottom])pcb_hulled(height,casewall-edge);
                translate([0,0,edge-casebottom])pcb_hulled(height-edge*2,casewall);
        }
}

module preview()
{
	pcb();
	color("#0f0")parts_top(part=true);
	color("#0f0")parts_bottom(part=true);
	color("#f00")parts_top(hole=true);
	color("#f00")parts_bottom(hole=true);
	color("#00f8")parts_top(block=true);
	color("#00f8")parts_bottom(block=true);
}

module top_half(step=false)
{
	difference()
	{
		translate([-casebottom-100,-casewall-100,pcbthickness-lip/2+0.01]) cube([pcbwidth+casewall*2+200,pcblength+casewall*2+200,height]);
		if(step)translate([0,0,pcbthickness-lip/2-0.01])pcb_hulled(lip,casewall/2+fit);
	}
}

module bottom_half(step=false)
{
	translate([-casebottom-100,-casewall-100,pcbthickness+lip/2-height-0.01]) cube([pcbwidth+casewall*2+200,pcblength+casewall*2+200,height]);
	if(step)translate([0,0,pcbthickness-lip/2])pcb_hulled(lip,casewall/2-fit);
}

module case_wall()
{
	difference()
	{
		solid_case();
		translate([0,0,-height])pcb_hulled(height*2);
	}
}

module top_side_hole()
{
	intersection()
	{
		parts_top(hole=true);
		case_wall();
	}
}

module bottom_side_hole()
{
	intersection()
	{
		parts_bottom(hole=true);
		case_wall();
	}
}

module parts_space()
{
	minkowski()
	{
		union()
		{
			parts_top(part=true,hole=true);
			parts_bottom(part=true,hole=true);
		}
		sphere(r=margin,$fn=6);
	}
}

module top_cut()
{
	difference()
	{
		top_half(true);
		if(parts_top)difference()
		{
			minkowski()
			{ // Penetrating side holes
				top_side_hole();
				rotate([180,0,0])
				pyramid();
			}
			minkowski()
			{
				top_side_hole();
				rotate([0,0,45])cylinder(r=margin,h=height,$fn=4);
			}
		}
	}
	if(parts_bottom)difference()
	{
		minkowski()
		{ // Penetrating side holes
			bottom_side_hole();
			pyramid();
		}
			minkowski()
			{
				bottom_side_hole();
				rotate([0,0,45])translate([0,0,-height])cylinder(r=margin,h=height,$fn=4);
			}
	}
}

module bottom_cut()
{
	difference()
	{
		 translate([-casebottom-50,-casewall-50,-height]) cube([pcbwidth+casewall*2+100,pcblength+casewall*2+100,height*2]);
		 top_cut();
	}
}

module top_body()
{
	difference()
	{
		intersection()
		{
			solid_case();
			pcb_hulled(height);
			top_half();
		}
		if(parts_top)minkowski()
		{
			if(nohull)parts_top(part=true);
			else hull()parts_top(part=true);
			translate([0,0,margin-height])cylinder(r=margin,h=height,$fn=8);
		}
	}
	intersection()
	{
		solid_case();
		parts_top(block=true);
	}
}

module top_edge()
{
	intersection()
	{
		case_wall();
		top_cut();
	}
}

module top()
{
	translate([casewall,casewall+pcblength,pcbthickness+casetop])rotate([180,0,0])difference()
	{
		union()
		{
			top_body();
			top_edge();
		}
		parts_space();
		translate([0,0,pcbthickness-height])pcb(height,r=margin);
	}
}

module bottom_body()
{
	difference()
	{
		intersection()
		{
			solid_case();
			translate([0,0,-height])pcb_hulled(height);
			bottom_half();
		}
		if(parts_bottom)minkowski()
		{
			if(nohull)parts_bottom(part=true);
			else hull()parts_bottom(part=true);
			translate([0,0,-margin])cylinder(r=margin,h=height,$fn=8);
		}
	}
	intersection()
	{
		solid_case();
		parts_bottom(block=true);
	}
}

module bottom_edge()
{
	intersection()
	{
		case_wall();
		bottom_cut();
	}
}

module bottom()
{
	translate([casewall,casewall,casebottom])difference()
	{
		union()
		{
        		bottom_body();
        		bottom_edge();
		}
		parts_space();
		pcb(height,r=margin);
	}
}
bottom(); translate([spacing,0,0])top();
