//nozzel 0.4

/* rail */
$width = 20;
$height = 15;
$thickness = 2;
$tol = 0.4;
$wallth = 4*0.4;
$wireout = 24;

module rail(filled = false)
{
    translate([0,$height/2-$thickness-$tol,25])
    difference()
    {
        cube([$width+2*$tol, $height+$tol, 50], center = true);
        if(filled == false)
            translate([0, $thickness/2+$tol/2, 0])
                cube([$width-2*$thickness-2*$tol, $height-$thickness-$tol+0.1, 50], center = true);  
    }
}

module inner()
{
    difference()
    {
        union()
        {
    //        translate([0,-$wallth/2+$height/2-$thickness-$tol,1.5])
    //            cube([$width+2*$tol + 2*$wallth, $height+$tol+$wallth, 3], center=true);

            translate([-$width/2+$thickness+$tol,-$tol/2,0])
                cube([$width-2*$thickness-2*$tol, $height-$thickness, 20]);
            
            translate([-$width/2,-$tol/2,0])
                cube([$width, $height/2-$thickness, 30]);
        }
        
        translate([0,0, 0])
            rail();

            //ceiling
        translate([0,-0.1,10])
            rotate(a=90, v=[-1,0,0])
            {
                translate([0,0,2])
                    cylinder(r1=3, r2=6, h=8.5, $fn=32);
                translate([0,0,10.5])
                    cylinder(r=6, h=10, $fn=32);
                cylinder(r=3, h=19, $fn=32);
            }
        
        //wire
        translate([0,-1,$wireout])
            rotate(a=90, v=[-1,0,0])
                cylinder(r=3/2, h=20, $fn=16);
        translate([0,$height/5,$wireout])
            rotate(a=90, v=[-1,0,0])
                cylinder(r1=3/2, r2=3, h=$height/6, $fn=16);
        translate([$width/4, 3.8, 0])
            cylinder(r=2.5/2, h=8.5, $fn=16);
        translate([-$width/4, 3.8, 0])
            cylinder(r=2.5/2, h=8.5, $fn=16);
        translate([-$width/2+$thickness+2,-$tol,0])
            rotate(a=45+90, v=[0,0,1])
                cube([4,4,50]);
        translate([$width/2-$thickness-2,-$tol,0])
            rotate(a=-45, v=[0,0,1])
                cube([4,4,50]);
    }

    //nose
    difference()
    {
        translate([0,-4,$wireout])
            rotate(a=90, v=[-1,0,0])
                cylinder(r=4, h=5, $fn=16);
        translate([0,-5,$wireout])
            rotate(a=90, v=[-1,0,0])
                cylinder(r=3/2, h=20, $fn=16);
    }
}

module outer()
{
    difference()
    {
        hull()
        {
            translate([0,-$wallth/2+$height/2-$thickness-$tol-3/2,1.5-1.5])
                cube([$width+2*$tol + 2*$wallth, $height+$tol+$wallth+3, 3], center=true);
            translate([-($width+2*$tol)/2 - $wallth,-2.6 -2*$wallth,28])
                cube([$width+2*$tol + 2*$wallth, 3+$wallth, 3]);

        }
        
        translate([0,0, 0])
            rail(true);
        
        translate([-$width, $height-$thickness-1, -3])
            cube([$width*2, 10, 20]);

            //fix
        translate([$width/4, 4, -5])
            cylinder(r=3/2, h=10, $fn=16);
        translate([-$width/4, 4, -5])
            cylinder(r=3/2, h=10, $fn=16);

            //nose
        translate([0,-4.5,$wireout+0.4])
            rotate(a=92.75, v=[-1,0,0])
                cylinder(r=5, h=5, $fn=16);
        translate([0,-10,$wireout+0.4])
            rotate(a=90, v=[-1,0,0])
                cylinder(r=3.5/2, h=10, $fn=16);
        
        //ceiling
        translate([0,-$thickness-3-$wallth/2-0.21,10])
            rotate(a=92.75, v=[-1,0,0])
                    cylinder(r=8, h=20, $fn=16);
                
        translate([-$width/2-$thickness+4,-$tol-7.5,-4])
            rotate(a=45+90, v=[0,0,1])
                                                            cube([6,6,50]);
        translate([$width/2+$thickness-4,-$tol-7.5,-4])
            rotate(a=-45, v=[0,0,1])
                cube([6,6,50]);
    }
}

inner();
//outer();

//#rail();