#ifndef SIMULATION_HPP
#define SIMULATION_HPP

#include <list>
#include <fstream>

#include "Grid.hpp"
#include "Particule.hpp"
#include "Obstacle.hpp"
#include <SDL2/SDL.h>
// #include "Scene.hpp"


class Simulation : public Object {

private :
  std::string conf_file;
  std::string import_path;
  std::string export_path;
  std::string scene_path;
  bool import_;
  bool export_;
  bool load_conf_;
  uint nb_file_i;
  mutable uint nb_file_e;
  uint t;
  FLOAT time;
  FLOAT frame_sample;
  FLOAT frame_sample_cur;
  
  Grid grid;
  std::vector<Particule*> particules;
  std::vector<Particule*> particules_buf; // additional particules for adaptive time stepping
  std::vector<Obstacle*> obstacles;
  std::vector<Obstacle*> obstacles_buf; // additional particules for adaptive time stepping

  inline FLOAT weight(FLOAT x);
public :
  Simulation(int shader);
  ~Simulation();

  void init();
  void clear();
  void clearParticules();
  
  void animate();
#ifndef NO_GRAPHICS_ 
  void draw(glm::mat4 m = glm::mat4(1.0f), int s = -1);
#endif

  void oneStep();

  void importParticules(std::ifstream & file);
  void exportParticules(std::ofstream & file) const;
  void importSim();
  void exportSim() ;


  void setLoad(std::string s);
  void setExport(std::string s);
  void setImport(std::string s);
  void setImage(std::string s);

  void setScene(std::string s);
  
  void backward(uint n);
  void loadScene();
  bool oneStepTry();
  void Vectorcopy(std::vector<Particule*> & l, std::vector<Particule*> r);
  // void Obstaclecopy(std::vector<Obstacle*> & l, std::vector<Obstacle*> r);

};



#endif
