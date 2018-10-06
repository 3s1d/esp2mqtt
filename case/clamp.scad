module clamp()
{
    difference()
    {
        union()
        {
            translate([-5, -4, 0])
                cube([20, 8, 4]);
            translate([-5, -4.9/2, 4])
                cube([20, 4.9, 1.5]);
        }
        
        translate([0,0,1.2])
            cylinder(r=3.2/2, h=10, $fn=16);
        cylinder(r=6.8/2, h=2.5, $fn=6);
        
        translate([10,0,0])
            cylinder(r=3/2, h=10, $fn=16);
        translate([10,-3/2,0])
            cube([10, 3, 2.7]);
        
    }
}


clamp();