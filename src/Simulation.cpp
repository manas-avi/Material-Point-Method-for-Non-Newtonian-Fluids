#include "Simulation.hpp"
#include "Scene.hpp"
#include "error.hpp"
#include "PlaneObstacle.hpp"
#include "SphereObstacle.hpp"
#include "CylinderObstacle.hpp"
#include "BoxObstacle.hpp"
#include "OpenBoxObstacle.hpp"
#include "BallObstacle.hpp"
#include "Times.hpp"
#include "mpm_conf.hpp"
#include "utils.hpp"

#define POISSON_PROGRESS_INDICATOR 1
#include "PoissonGenerator.hpp"

Simulation::Simulation(int shader) : Object(shader) {
	import_ = false;
	export_ = false;
	load_conf_ = false;

  // conf_file = "test.conf";
  // export_path = "object/test";
  // import_path = "object/test";
	if (load_conf_) {
		mpm_conf::loadConf(conf_file);
	}
	scene_path = "first_scene.sc";
}

Simulation::~Simulation() {
	clear();
}

FLOAT min(FLOAT x,FLOAT y) {
	if (x<y)
		return x;
	else 
		return y;
}

FLOAT max(FLOAT x,FLOAT y) {
	if (x>y)
		return x;
	else 
		return y;
}

void Simulation::init() {
	nb_file_i = 0;
	nb_file_e = 0;

	if (load_conf_) {
		mpm_conf::loadConf(conf_file);
	}

	if (!import_) {

		grid = Grid(mpm_conf::size_grid_(0), mpm_conf::size_grid_(1), mpm_conf::size_grid_(2), mpm_conf::grid_spacing_, 2);
		loadScene();

		grid.init(particules);
		grid.initCollision(obstacles);

		INFO(1, "Lame Parameters : lambda = "<<mpm_conf::lambda_<<"    mu = "<<mpm_conf::mu_<<"\n");
		INFO(1, "dt = "<<mpm_conf::dt_<<"\n");
		INFO(3, "obstacles = "<<obstacles.size()<<"\n");
		if (export_) {
			exportSim();
		}
	} else {
		importSim();
		loadScene();
	}
	t = 0;
}

void Simulation::clearParticules() {
	for (auto& p : particules) {
    // INFO(3,"delete p"<<" "<<p);
		delete p;
	}
	particules.clear();
}

void Simulation::clear() {
	INFO(1, "Clear Simulation");
	clearParticules();
	for (auto& ob : obstacles) {
		delete ob;
	}
	obstacles.clear();
}

void Simulation::animate() {
	++t;
	 INFO(1, "Simulation step : "<<t);
	if (!import_) {
		oneStep();
		if (export_)
			exportSim();

		for (auto &ob : obstacles)
			ob->animate();

	} else {
		importSim();
		INFO(3, particules.front()->getVolume());
		for (uint s = 0; s < mpm_conf::replay_speed_; ++s) {
			for (auto &ob : obstacles) {
				ob->animate();
			}
		} 
	}
}

#ifndef NO_GRAPHICS_ 
void Simulation::draw(glm::mat4 m, int s) {

	Times::TIMES->tick(Times::display_time_);
	uint cur_shader = m_shader;
	if (m_shader == -1) {
		cur_shader = s;
	}
	glm::mat4 cur_model = m * m_model_view;

	grid.draw(cur_model, cur_shader);

	enableShader();
	for (auto& p : particules) {
    // p->modelView() =  m_model_view * p->modelView();
		p->draw(cur_model, cur_shader);
    //    INFO(3, p->getPosition());
	}
	for (auto& ob : obstacles) {
    //ob->modelView() =  m_model_view * ob->modelView();
		ob->draw(cur_model, cur_shader);
	}

	disableShader();
	Times::TIMES->tock(Times::display_time_);
}
#endif

void Simulation::oneStep() {
	Times::TIMES->tick(Times::simu_time_);
	grid.nextStep(); //resets the grid for this step.
	// if (mpm_conf::implicit_) {
	// 	grid.particulesToGridImplicite(particules);
	// } else {
	// 	grid.particulesToGrid(particules);
	// 	// it performs force addition there itself
	// }
	grid.particulesToGrid(particules);

	// debug function to check on particles
	// grid.checkParticles(particules);
	// not needed to smooth the velocities
	if (mpm_conf::smooth_vel_) {
		for (uint i = 0; i < 1; ++i) {
			grid.smoothVelocity();
		}
		INFO(3, "SMOOTH");
	}
	// grid based collisions
	grid.collision(obstacles);
	// sending information from grid to particles
	grid.gridToParticules(particules);
	grid.initCollision(obstacles);
	Times::TIMES->tock(Times::simu_time_);
}

void Simulation::importParticules(std::ifstream & file) {
	particules.clear();
	std::string line;
	uint ir = 0, ia = 0;
	while (getline(file, line)) {
		if (line.substr(0,2) == "v ") {
			std::istringstream s(line.substr(2));
			VEC3 v;
			s >> v(0); 
			s >> v(1); 
			s >> v(2);
			Particule *p = new Particule(1, 0.000005, v);
			p->setColor(1, 0.5, 0.5);
			particules.push_back(p);
			// INFO(3, p->getPosition());
		} else if (line.substr(0,2) == "r ") {
			std::istringstream s(line.substr(2));
			//INFO(3, line);
			MAT3 r;
			for (uint i = 0; i < 9; ++i) {
				s >> r(i);
			}
			particules[ir]->setAnisotropyRotation(r);
			//setAnisotropyAxes(VEC3(r(0), r(3), r(6)), VEC3(r(1), r(4), r(7)), VEC3(r(2), r(5), r(8)));
			++ir;
			//INFO(3, r);
		} else if (line.substr(0,2) == "a ") {
			std::istringstream s(line.substr(2));
			//INFO(3, line);
			FLOAT vx, vy, vz;
			s >> vx >> vy >> vz;
			particules[ia]->setAnisotropyValues(vx, vy, vz);
			++ia;
		} else if (line[0] == '#') {
			//INFO(3, "COMMENT "<<line);
		} else {
			WARNING(false, "Imported file possibly corrupted", line);
		}
	}
}

void Simulation::exportParticules(std::ofstream & file) const {
	file << "# particules \n";
	for (auto &p : particules) {
		VEC3 v = p->getPosition();
		file<<"v "<<v(0)<<" "<<v(1)<<" "<<v(2)<<"\n";
	}
	for (auto &p : particules) {
		MAT3 r = p->getRotation();
		file<<"r ";
		for (uint i = 0; i < 9; ++i) {
			file<<r(i)<<" ";
		}
		file<<"\n";
	}
	for (auto &p : particules) {
		VEC3 a = p->getAnisotropy();
		file<<"a "<<a(0)<<" "<<a(1)<<" "<<a(2)<<"\n";
	}

}

void Simulation::importSim() {
	std::stringstream ss;
	ss <<import_path<<nb_file_i<<".obj";
	std::string str(ss.str());
	std::ifstream file(str.c_str());
	INFO(1, "Import file \""<<str<<"\"  "<<mpm_conf::replay_speed_);
	if (file.good()) {
		clearParticules();
		importParticules(file);
		if ((int)nb_file_i >= -mpm_conf::replay_speed_) {
			nb_file_i += mpm_conf::replay_speed_;
    } // else {
    //   mpm_conf::replay_speed_ = 1;
    // }
    } else {
    	if ((int)nb_file_i >= 1 && mpm_conf::replay_speed_ < 0 && (int)nb_file_i >= -mpm_conf::replay_speed_) {
    		nb_file_i += mpm_conf::replay_speed_;
    }//  else{
    //   mpm_conf::replay_speed_ = 1;
    // }
    }
    file.close();
}

void Simulation::exportSim() const {
	if (nb_file_e % mpm_conf::export_step_ == 0) {
		std::stringstream ss;
		ss <<export_path<<nb_file_e/mpm_conf::export_step_<<".obj";
		std::string str(ss.str());
		std::ofstream file(str.c_str());
		ERROR(file.good(), "cannot open file \""<<str<<"\"", "");
		INFO(1, "Export file \""<<str<<"\"");
		exportParticules(file);
		file.close();
	}
	++nb_file_e;
}

void Simulation::setLoad(std::string s) {
	conf_file = s;
	load_conf_ = true;
}

void Simulation::setExport(std::string s) {
	export_path = s;
	export_ = true;
}

void Simulation::setImport(std::string s) {
	import_path = s;
	import_ = true;
}

void Simulation::setScene(std::string s) {
	scene_path = s;
}

void Simulation::backward(uint n) {
	if (nb_file_i >= 1) {
		nb_file_i -= 1;
	} else {
		nb_file_i = 0;
	}
}

void randomRotation(MAT3 & R) {
	VEC3 x((FLOAT)rand()/(FLOAT)RAND_MAX, (FLOAT)rand()/(FLOAT)RAND_MAX, (FLOAT)rand()/(FLOAT)RAND_MAX);
	VEC3 y((FLOAT)rand()/(FLOAT)RAND_MAX, (FLOAT)rand()/(FLOAT)RAND_MAX, (FLOAT)rand()/(FLOAT)RAND_MAX);
	VEC3 z((FLOAT)rand()/(FLOAT)RAND_MAX, (FLOAT)rand()/(FLOAT)RAND_MAX, (FLOAT)rand()/(FLOAT)RAND_MAX);
	z = x.cross(y);
	y = z.cross(x);
	x.normalize();
	y.normalize();
	z.normalize();
	R.col(0) = x;
	R.col(1) = y;
	R.col(2) = z;
}

void Simulation::loadScene() {
	std::ifstream file(scene_path.c_str());
	std::string line;
	INFO(3, "load SCENE");

	while (getline(file, line)) {

		if (line.substr(0,11) == "<obstacles>") {
			getline(file, line);
			std::list<Motion> motions;
			while (line.substr(0,12) != "</obstacles>") {
			  	// INFO(3, "Obsctacle "<<line);
				if (line.substr(0,9) == " <motion>") {
					getline(file, line);
					Motion m;
					while (line.substr(0,10) != " </motion>") {
						if (line.substr(0,9) == "  <begin>") {
							std::istringstream s(line.substr(9));
							s >> m.begin_time;
						} else if (line.substr(0,7) == "  <end>") {
							std::istringstream s(line.substr(7));
							s >> m.end_time;
						} else if (line.substr(0,9) == "  <scale>") {
							std::istringstream s(line.substr(9));
							s >> m.scale;
							m.scale *= mpm_conf::dt_;
						} else if (line.substr(0,15) == "  <translation>") {
							std::istringstream s(line.substr(15));
							for (uint i = 0; i < 3; ++i) {
								s >> m.translation(i);
							}
							m.translation *= mpm_conf::dt_;
						} else if (line.substr(0,12) == "  <rotation>") {
							getline(file, line);
							MAT3 rotation = MAT3::Identity();
							FLOAT angle = 0;
							VEC3 axe;
							while (line.substr(0,13) != "  </rotation>") {
								if (line.substr(0,8) == "   <axe>") {
									std::istringstream s(line.substr(8));
									for (uint i = 0; i < 3; ++i) {
										s >> axe(i);
									}
									INFO(3, "AXE \n"<<axe);
								} else  if (line.substr(0,10) == "   <angle>") {
									std::istringstream s(line.substr(10));
									s >> angle;
								} else if (line.substr(0,11) == "   <center>") {
									std::istringstream s(line.substr(11));
									for (uint i = 0; i < 3; ++i) {
										s >> m.center(i);
									}
									m.rotation_center_def = true;
								} else {
									std::cerr<<"Line not recognized in file \""<<scene_path<<"\": "<<line<<std::endl;
									exit(-1);
								}
								getline(file, line);
							}
							rotation = utils::rotation(angle*mpm_conf::dt_, axe);
							m.rotation = rotation;
						} else {
							std::cerr<<"Line not recognized in file \""<<scene_path<<"\": "<<line<<std::endl;
							exit(-1);
						}
						getline(file, line);
					}
					motions.push_back(m);
				} else if (line.substr(0,9) == " <sphere>") {
					VEC3 center(0, 0, 0);
					FLOAT ray = 0;
					FLOAT hr = 0.0;
					VEC3 hn = VEC3(0, 0, 0);
					FLOAT fric = mpm_conf::friction_coef_;
					getline(file, line);
					while (line.substr(0,10) != " </sphere>") {
						INFO(3, "sphere "<<line);
						if (line.substr(0,10) == "  <center>") {
							std::istringstream s(line.substr(10));
							for (uint i = 0; i < 3; ++i) {
								s >> center(i);
							}
						} else if (line.substr(0,7) == "  <ray>") {
							std::istringstream s(line.substr(7));
							s >> ray;
						} else if (line.substr(0,15) == "  <hole normal>") {
							std::istringstream s(line.substr(15));
							for (uint i = 0; i < 3; ++i) {
								s >> hn(i);
							}
						} else if (line.substr(0,12) == "  <hole ray>") {
							std::istringstream s(line.substr(12));
							s >> hr;
						} else if (line.substr(0,12) == "  <friction>") {
							std::istringstream s(line.substr(12));
							s >> fric;
						} else {
							std::cerr<<"Line not recognized in file \""<<scene_path<<"\": "<<line<<std::endl;
							exit(-1);
						}
						getline(file, line);
					}
					SphereObstacle *o = new SphereObstacle(center, ray, hr, hn);
					obstacles.push_back(o);
					o->setFriction(fric);
					o->setMotions(motions);
				    //INFO(3, "obstacles = "<<center<<"\n"<<ray);
				    // end sphere
				} else if (line.substr(0,6) == " <box>") {
					VEC3 min(0, 0, 0);
					VEC3 max(0, 0, 0);
					FLOAT fric = mpm_conf::friction_coef_;
					getline(file, line);
					while (line.substr(0,7) != " </box>") {
						if (line.substr(0,11) == "  <min pos>") {
							std::istringstream s(line.substr(11));
							for (uint i = 0; i < 3; ++i) {
								s >> min(i);
							}
						} else if (line.substr(0,11) == "  <max pos>") {
							std::istringstream s(line.substr(11));
							for (uint i = 0; i < 3; ++i) {
								s >> max(i);
							}
						} else if (line.substr(0,12) == "  <friction>") {
							std::istringstream s(line.substr(12));
							s >> fric;
						} else {
							std::cerr<<"Line not recognized in file \""<<scene_path<<"\": "<<line<<std::endl;
							exit(-1);
						}
						getline(file, line);
					}
					BoxObstacle *o = new BoxObstacle(min, max);
					obstacles.push_back(o);
					o->setFriction(fric);
					o->setMotions(motions);
				    //INFO(3, "obstacles = "<<min<<"\n"<<max);
				  	// end box
				} else if (line.substr(0,11) == " <open box>") {
					VEC3 min(0, 0, 0);
					VEC3 max(0, 0, 0);
					FLOAT fric = mpm_conf::friction_coef_;
					getline(file, line);
					while (line.substr(0,12) != " </open box>") {
						if (line.substr(0,11) == "  <min pos>") {
							std::istringstream s(line.substr(11));
							for (uint i = 0; i < 3; ++i) {
								s >> min(i);
							}
						} else if (line.substr(0,11) == "  <max pos>") {
							std::istringstream s(line.substr(11));
							for (uint i = 0; i < 3; ++i) {
								s >> max(i);
							}
						} else if (line.substr(0,12) == "  <friction>") {
							std::istringstream s(line.substr(12));
							s >> fric;
						} else {
							std::cerr<<"Line not recognized in file \""<<scene_path<<"\": "<<line<<std::endl;
							exit(-1);
						}
						getline(file, line);
					}
					OpenBoxObstacle *o = new OpenBoxObstacle(min, max);
					obstacles.push_back(o);
					o->setFriction(fric);
					o->setMotions(motions);
				    //INFO(3, "obstacles = "<<min<<"\n"<<max);
				  	// end box
				}  else if (line.substr(0,8) == " <plane>") {
					VEC3 pos(0.5, 0.5, 0);
					VEC3 n(0, 0, 0);
					FLOAT l = 0, w = 0;
					FLOAT fric = mpm_conf::friction_coef_;
					getline(file, line);
					while (line.substr(0,9) != " </plane>") {
						if (line.substr(0,7) == "  <pos>") {
							std::istringstream s(line.substr(7));
							s >> pos(2);
						} else if (line.substr(0,12) == "  <position>") {
							std::istringstream s(line.substr(12));
							for (uint i = 0; i < 3; ++i) {
								s >> pos(i);
							}
						} else if (line.substr(0,10) == "  <normal>") {
							std::istringstream s(line.substr(10));
							for (uint i = 0; i < 3; ++i) {
								s >> n(i);
							}
						} else if (line.substr(0,12) == "  <friction>") {
							std::istringstream s(line.substr(12));
							s >> fric;
						} else if (line.substr(0,10) == "  <length>") {
							std::istringstream s(line.substr(10));
							s >> l;
						} else if (line.substr(0,9) == "  <width>") {
							std::istringstream s(line.substr(9));
							s >> w;
						} else {
							std::cerr<<"Line not recognized in file \""<<scene_path<<"\": "<<line<<std::endl;
							exit(-1);
						}
						getline(file, line);
					}
					PlaneObstacle *o = new PlaneObstacle(pos, n, l, w);
					obstacles.push_back(o);
					o->setFriction(fric);
					o->setMotions(motions);
				    //INFO(3, "obstacles = "<<pos<<"\n"<<n);
				    // end plane
				} else if (line.substr(0,11) == " <cylinder>") {
					VEC3 pos(0, 0, 0);
					VEC3 dir(0, 0, 0);
					FLOAT r = 0;
					FLOAT fric = mpm_conf::friction_coef_;
					getline(file, line);
					while (line.substr(0,12) != " </cylinder>") {
						if (line.substr(0,7) == "  <pos>") {
							std::istringstream s(line.substr(7));
							for (uint i = 0; i < 3; ++i) {
								s >> pos(i);
							}
						} else if (line.substr(0,13) == "  <direction>") {
							std::istringstream s(line.substr(13));
							for (uint i = 0; i < 3; ++i) {
								s >> dir(i);
							}
						} else if (line.substr(0,7) == "  <ray>") {
							std::istringstream s(line.substr(7));
							s >> r;
						} else if (line.substr(0,12) == "  <friction>") {
							std::istringstream s(line.substr(12));
							s >> fric;
						} else {
							std::cerr<<"Line not recognized in file WWWW\""<<scene_path<<"\": "<<line<<std::endl;
							exit(-1);
						}
						getline(file, line);
					}
					CylinderObstacle *o = new CylinderObstacle(pos, dir, r, 0);
					obstacles.push_back(o);
					o->setFriction(fric);
					o->setMotions(motions);
				    //INFO(3, "obstacles = "<<pos<<"\n"<<n);
				    // end cylinder
				} else if (line.substr(0,7) == " <ball>") {
					VEC3 pos(0, 0, 0);
					FLOAT r = 0;
					FLOAT fric = mpm_conf::friction_coef_;
					getline(file, line);
					while (line.substr(0,8) != " </ball>") {
						if (line.substr(0,7) == "  <pos>") {
							std::istringstream s(line.substr(7));
							for (uint i = 0; i < 3; ++i) {
								s >> pos(i);
							}
						} else if (line.substr(0,7) == "  <ray>") {
							std::istringstream s(line.substr(7));
							s >> r;
						} else if (line.substr(0,12) == "  <friction>") {
							std::istringstream s(line.substr(12));
							s >> fric;
						} else {
							std::cerr<<"Line not recognized in file WWWW\""<<scene_path<<"\": "<<line<<std::endl;
							exit(-1);
						}
						getline(file, line);
					}
					BallObstacle *o = new BallObstacle(pos, r, 0);
					obstacles.push_back(o);
					o->setFriction(fric);
					o->setMotions(motions);
				    //INFO(3, "obstacles = "<<pos<<"\n"<<n);
				    // end cylinder
				} else {
					std::cerr<<"Line not recognized in file FERWE\""<<scene_path<<"\": "<<line<<std::endl;
					exit(-1);
				}
				getline(file, line);
			}
       	// end obstacles
		} else if (line.substr(0,12) == "<particules>") {
			MAT3 rotation = MAT3::Identity();
			bool random = false;
			getline(file, line);	
			while (line.substr(0,13) != "</particules>") {
				if (!import_) {
					if (line.substr(0,11) == " <rotation>") {
						getline(file, line);
						FLOAT angle = 0;
						VEC3 axe;
						while (line.substr(0,12) != " </rotation>") {
							if (line.substr(0,7) == "  <axe>") {
								std::istringstream s(line.substr(7));
								for (uint i = 0; i < 3; ++i) {
									s >> axe(i);
								}
							} else  if (line.substr(0,9) == "  <angle>") {
								std::istringstream s(line.substr(9));
								s >> angle;
							} else  if (line.substr(0,10) == "  <random>") {
								random = true;
							} else {
								std::cerr<<"Line not recognized in file \""<<scene_path<<"\": "<<line<<std::endl;
								exit(-1);
							}
							getline(file, line);
						}
						rotation = utils::rotation(angle, axe)*rotation;
						// rotation << 1, 0, 0,
					    // 	0, sqrt(2.0)*0.5, sqrt(2.0)*0.5,
					    // 	0, -sqrt(2.0)*0.5, sqrt(2.0)*0.5;
					    //  INFO(3, "rot \n"<<rotation);
				    //cuboid
					} else if (line.substr(0,9) == " <cuboid>") {
						FLOAT xmin = 0, xmax = 0, ymin = 0, ymax = 0, zmin = 0, zmax = 0;
						uint nb_part;
						VEC3 vel(0, 0, 0);
						getline(file, line);
						while (line.substr(0,10) != " </cuboid>") {
							if (line.substr(0,5) == "  <x>") {
								std::istringstream s(line.substr(5));
								s >> xmin;
								s >> xmax;
							} else  if (line.substr(0,5) == "  <y>") {
								std::istringstream s(line.substr(5));
								s >> ymin;
								s >> ymax;
							} else if (line.substr(0,5) == "  <z>") {
								std::istringstream s(line.substr(5));
								s >> zmin;
								s >> zmax;
							} else if (line.substr(0,17) == "  <nb particules>") {
								std::istringstream s(line.substr(17));
								s >> nb_part;
							} else if (line.substr(0,12) == "  <velocity>") {
								std::istringstream s(line.substr(12));
								for (uint i = 0; i < 3; ++i) {
									s >> vel(i);
								}
							} else if (line[0] == '#') {
								getline(file, line);
								continue;
							} else {
								std::cerr<<"Line not recognized in file FERWE\""<<scene_path<<"\": "<<line<<std::endl;
								exit(-1);
							}
							getline(file, line);
						}
						FLOAT h = zmax - zmin, w = xmax - xmin, l = ymax - ymin;
						FLOAT volume = h*l*w;

						PoissonGenerator::PRNG prng;
						std::list<VEC3> points = PoissonGenerator::GeneratePoissonPointsR(nb_part, prng, 30, VEC3(w, l, h));

						nb_part = points.size();
						for (auto &v: points) {
						    Particule *p = new Particule(volume*mpm_conf::density_/(FLOAT)nb_part, 
						    volume/(FLOAT)nb_part, v + VEC3(xmin, ymin, zmin), VEC3(0, 0, 1), vel);
							particules.push_back(p);
							if (random) {
								randomRotation(rotation);
							}
							p->setAnisotropyValues(1, 1, 1);
							p->setAnisotropyRotation(rotation);
						}

				    //sphere
					} else if (line.substr(0,9) == " <sphere>") {
						VEC3 center(0, 0, 0);
						FLOAT ray = 1;
						uint nb_part;
						FLOAT radius = 1;
						VEC3 vel(0, 0, 0);
						FLOAT force_val = 0;
						getline(file, line);
						while (line.substr(0,10) != " </sphere>") {
							if (line.substr(0,10) == "  <center>") {
								std::istringstream s(line.substr(10));
								for (uint i = 0; i < 3; ++i) {
									s >> center(i);
								}
							} else  if (line.substr(0,7) == "  <ray>") {
								std::istringstream s(line.substr(7));
								s >> ray;
							} else  if (line.substr(0,10) == "  <radius>") {
								std::istringstream s(line.substr(10));
								s >> radius;
							} else  if (line.substr(0,9) == "  <force>") {
								std::istringstream s(line.substr(9));
								s >> force_val;
							} else if (line.substr(0,17) == "  <nb particules>") {
								std::istringstream s(line.substr(17));
								s >> nb_part;
							} else if (line.substr(0,12) == "  <velocity>") {
								std::istringstream s(line.substr(12));
								for (uint i = 0; i < 3; ++i) {
									s >> vel(i);
								}
							} else {
								std::cerr<<"Line not recognized in file FERWE\""<<scene_path<<"\": "<<line<<std::endl;
								exit(-1);
							}
							getline(file, line);
						}
						FLOAT volume = 4.0/3.0*M_PI*pow(radius, 3);
						PoissonGenerator::PRNG prng;
						std::list<VEC3> points = PoissonGenerator::GeneratePoissonPointsC(nb_part, prng,
						 30, radius);
						nb_part = points.size();
						std::cout << "Number of particles : " << nb_part << std::endl; 
						for (auto &v: points) {
							float mag = vel.norm();
							VEC3 radius_vec = VEC3(radius, radius, radius);
							VEC3 norm_direc = (v - radius_vec).normalized();
							VEC3 tang_velo = norm_direc.cross(vel);

							VEC3 p_velocity = mag * norm_direc;
							Particule *p = new Particule(volume*mpm_conf::density_/(FLOAT)nb_part,
							volume/(FLOAT)nb_part, v - radius_vec + center, norm_direc, vel);
							// volume/(FLOAT)nb_part, v - radius_vec + center, norm_direc, tang_velo);
							// volume/(FLOAT)nb_part, v - radius_vec + center, norm_direc, p_velocity);
							particules.push_back(p);
							if (random) {
								randomRotation(rotation);
							}
							if(mpm_conf::anisotropy_on)
							{
								p->setAnisotropyValues(1, 1, 1);
								p->setAnisotropyRotation(rotation);
							}
							// depending on force type add force values on sphere
							// p->setForce(VEC3(0,0,force_val));
							// if (force_val > 0) {
							// 	p->setForce(VEC3(0,0,-10)*force_val);
							// }
							// else if (force_val < 0) {
							// 	p->setForce(norm_direc*force_val);
							// }
							// else {
							// 	INFO(2, "FORCE TYPE NOT FOUND");
							// 	exit(1);
							// }

						}

					} else if (line.substr(0,9) == " <obj>") {
						VEC3 center(0, 0, 0);
						uint nb_part;
						FLOAT scale = 1;
						FLOAT xangle = 0;
						FLOAT yangle = 0;
						std::string obj_filename;
						VEC3 vel(0, 0, 0);
						getline(file, line);
						while (line.substr(0,10) != " </obj>") {
							if (line.substr(0,10) == "  <center>") {
								std::istringstream s(line.substr(10));
								for (uint i = 0; i < 3; ++i) {
									s >> center(i);
								}
							} else  if (line.substr(0,9) == "  <scale>") {
								std::istringstream s(line.substr(9));
								s >> scale;
							} else  if (line.substr(0,10) == "  <xangle>") {
								std::istringstream s(line.substr(10));
								s >> xangle;
								xangle = xangle * M_PI/180;
							} else  if (line.substr(0,10) == "  <yangle>") {
								std::istringstream s(line.substr(10));
								s >> yangle;
								yangle = yangle * M_PI/180;
							} else if (line.substr(0,8) == "  <file>") {
								std::istringstream s(line.substr(8));
								s >> obj_filename;
							} else if (line.substr(0,12) == "  <velocity>") {
								std::istringstream s(line.substr(12));
								for (uint i = 0; i < 3; ++i) {
									s >> vel(i);
								}
							} else {
								std::cerr<<"Line not recognized in file FERWE\""<<scene_path<<"\": "<<line<<std::endl;
								exit(-1);
							}
							getline(file, line);
						}
						std::list<VEC3> points;
					    std::ifstream obj_inputfile;
						obj_inputfile.open(obj_filename);
						if(!obj_inputfile){
						    std::cerr << "Unable to open " << obj_filename << "!" << std::endl;
						    exit(1);
						}
						std::string line;
						while(std::getline(obj_inputfile, line)){
						    std::stringstream ss(line);
						    VEC3 thisXp;
						    if (line[0] == 'v'){
						        ss.ignore();
						        for (int i = 0; i < 3; i++){
						            ss >> thisXp(i);
						            //TODO assert data
						            //assert(thisXp(i) > 0 && thisXp(i) < 1);
						        }
							    points.push_back(thisXp);
						    }
						}
						// FLOAT volume = 0;
						FLOAT x_min=1000, x_max=0, y_min=1000, y_max=0, z_min=1000, z_max=0;
						nb_part = points.size();
						std::cout << "Number of particles : " << nb_part << std::endl; 
						VEC3 com = VEC3(0,0,0);
						for (auto &v: points) {
							com = com + v/ nb_part;
						}
						for (auto &v: points) {
							Eigen::AngleAxisd rot_mat1(xangle, Eigen::Vector3d::UnitX());
							Eigen::AngleAxisd rot_mat2(yangle, Eigen::Vector3d::UnitY());
							MAT3 rotationMatrix = rot_mat1.matrix() * rot_mat2.matrix();
							v = rotationMatrix * (v-com);
							v = v*scale  + center;
							x_min=min(x_min, v(0)); x_max=max(x_max, v(0));
							y_min=min(y_min, v(1)); y_max=max(y_max, v(1));
							z_min=min(z_min, v(2)); z_max=max(z_max, v(2));
						}
						// corrected volume of the bounding box of the object.
						// FLOAT volume = 0.001*pow(scale,3);
						FLOAT volume = (x_max-x_min)*(y_max-y_min)*(z_max-z_min);
						for (auto &v: points) {
							float mag = vel.norm();
							VEC3 norm_direc = VEC3(0, 0, 1);
							// since things are a bit twisted about x-y axis so we have to rotate the object
							Particule *p = new Particule(volume*mpm_conf::density_/(FLOAT)nb_part,
							volume/(FLOAT)nb_part, v, norm_direc, vel);
							particules.push_back(p);
							if (random) {
								randomRotation(rotation);
							}
							if(mpm_conf::anisotropy_on)
							{
								p->setAnisotropyValues(1, 1, 1);
								p->setAnisotropyRotation(rotation);
							}
						}


					} else {
						std::cerr<<"Line not recognized in file \""<<scene_path<<"\": "<<line<<std::endl;
						exit(-1);
					}
				}
				getline(file, line);	
			}
		//end particules
		} else {
			std::cerr<<"Line not recognized in file \""<<scene_path<<"\": "<<line<<std::endl;
			exit(-1);
		}

    } //end main loop
    
} // end loadScene


// void Simulation::saveState(std::string save_file) {
//     std::ofstream file(save_file);
//     ERROR(file.good(), "cannot open file \""<<save_file<<"\"", "");
//     INFO(1, "Save simulation state in file \""<<str<<"\"");
//     exportParticules(file);
//     file.close();
//  }
