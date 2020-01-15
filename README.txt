/*********************************************************/
/***  MPM Piles Of StuFF  *******************************/
/********************************************************/

// To generate video in the images folder
ffmpeg -r 5 -i frame%04d.png -c:v libx264 -vf "fps=25,format=yuv420p" out.mp4 

///  COMPILATION:  ////

* graphic mode
>qmake poff.pro
>make

* no graphic mode
>qmake poff_no_graph.pro
>make

Note: the macro -D__MODE_DEBUG=3 to switch between debug modes (3 lots of debugging information, 0 no debug). (see error.hpp for more details).




///  SIMULATION:   ////

Synopsis: 
     .\poff <options>
     .\poff_no_graph <options>

Options:
     -l, -load <file>: load configuration file
     -s, -scene <file>: load scene from file
     -e, -export <name>: export animation in a set of files <name><frame number>.obj
     -i, -import <name>: import animation from a set of files <name><frame number>.obj
     -stop <t>: stop animation and exit at time t
     -es, -export_step <n>: export every n frames
     -r, -run: run directly the animation
     -h, -help: print help


Note: results generated with .\poff_no_graph can be vizualized with .\poff



////  OTHERS   //////

* An example of command line can be found in the file example_script
* There are examples of configuration files in the "material" repository. These files are read using "src/mpm_conf.hpp"
* There are examples of scenes files in the "scenes" repository. These files are read using the function  loadScene from "src/Simulation.hpp"
* The implicit integration does not work properly yet.



///// CONTACT  //////

For questions, bug report or other comments, you can contact me:

mail: camille.schreck@ist.ac.at
Office: I21.O2.106  West Office Building


///// TROUBLESHOOTING //////

Compilation:
in poff*.pro, I used the paths "/usr/local/lib" and "/usr/local/include". You may need to remove the "local":
"/usr/lib" and "/usr/include"

The lib TTF need to be installed. (Note: the functionality using it are not finish yet, so you can also for now remove the function "render_text" of the class Simulation and the attribut TTF_Font *font.)

I used the version 3.30 core of glsl. If not supported, you can use the version 3.00 es (replace "#version 330 core" by "#version 300 es" in the first line of each shader.). In which case you need also to add the line "precision mediump float;" after the version line.

