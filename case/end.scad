
module end(holes = true)
{
    scale([1.02, 1.02, 1])
    difference()
    {
        union()
        {
            translate([0,-0.2,5])
                cube([35.2, 20.2, 10], center = true);
            if(holes)
            {
                translate([(35-(2*4.1))/2, 2.3, -10])
                    cylinder(r=2.8/2, h=30, $fn=16);
                translate([-(35-(2*4.1))/2, 2.3, -10])
                    cylinder(r=2.8/2, h=30, $fn=16);
            }
            else
            {
                translate([-35.2/2, -11, 0])
                    cube([35.2, 1, 10]);
            }
            //translate([-10, -10-10, 0])
            //    cube([20, 0.01, 10]);
            
            difference()
            {
                translate([0, 0, 0])
                    cylinder(r=18.5, h=10);
                translate([-20, -10, 0])
                    cube([40, 50, 10]);
            }
        }
        
        translate([16/2+.8, 10, 0])
            rotate(a=-45, v=[0, 0, 1])
                cube([20, 10, 10]);
        mirror([1,0,0])
        {
            translate([16/2+.8, 10, 0])
                rotate(a=-45, v=[0, 0, 1])
                    cube([20, 10, 10]);

        }
    }
}

module cap(esp=true)
{
    difference()
    {
        scale([1.095, 1.11, 1])
            translate([0,4,0])
                end(false);

        if(esp)
        {
            translate([0,4,0.75])
                end();
            translate([-18.5/2, 0,0.75])
                cube([18.5, 20, 2]);
        }
        else
        {
            translate([0,4,0.75 + 4-3])
                end();
            translate([-3,-10,1])
                cube([6, 24, 10]);
        }
        
        translate([-30, -30, 2.75+4])
            cube([60, 60, 30]);
    }
    
    if(esp == false)
    {
        difference()
        {
            translate([-11,-1,0])
                cube([22,15,4]);
            translate([-3,-10,1])
                cube([6, 24, 10]);

        }
    }
}

//2D for pcb
module proj()
{
    projection()
    difference()
    {
        end();
        translate([(35-(2*4.1))/2, 2.3, -10])
            cylinder(r=2.8/2, h=30, $fn=16);
        translate([-(35-(2*4.1))/2, 2.3, -10])
            cylinder(r=2.8/2, h=30, $fn=16);
    }
}

//3D
//cap(true);
cap(false);