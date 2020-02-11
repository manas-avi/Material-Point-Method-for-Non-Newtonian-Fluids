#ifndef NO_GRAPHICS_ 

#include "Scene.hpp"
#include "Cube.hpp"
#include "Sphere.hpp"
#include "CylinderObstacle.hpp"
#include "Cylinder.hpp"
#include "Simulation.hpp"
#include "error.hpp"
#include "Times.hpp"
#include "mpm_conf.hpp"
#include "Skybox.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <thread>

#define MAX_STRING_LEN 4
#include "common.h"

using namespace glm;

Scene* Scene::SCENE(new Scene("test", 2400, 1800));

void Scene::animateScene() {
	SCENE->animationLoop();
}

Scene::Scene() {
}

Scene::Scene(std::string titreFenetre, int largeurFenetre, int hauteurFenetre) :
m_titreFenetre(titreFenetre), m_largeurFenetre(largeurFenetre),
m_hauteurFenetre(hauteurFenetre), m_window(0), m_contexteOpenGL(0), m_input() {
}

Scene::~Scene() {
	std::vector<Object*>::iterator it;
	for (auto& o : l_objects) {
		delete o;
	}
	std::vector<Shader*>::iterator its;
	for (its = l_shaders.begin(); its != l_shaders.end(); ++its) {
		delete (*its);
	}
	std::vector<Texture*>::iterator itt;
	for (itt = l_textures.begin(); itt != l_textures.end(); ++itt) {
		delete (*itt);
	}
	l_objects.clear();
	l_shaders.clear();
	l_textures.clear();

	SDL_GL_DeleteContext(m_contexteOpenGL);
	SDL_DestroyWindow(m_window);
	SDL_Quit();
}

bool Scene::initialiserFenetre() {
	// Initialisation de la SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cout << "Erreur lors de l'initialisation de la SDL : " << SDL_GetError() << std::endl;
		SDL_Quit();
		return false;
	}

		// Version d'OpenGL
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

		// Double Buffer
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	std::cout<<"init buffer"<< std::endl;
	
		 // Création de la fenêtre
	m_window = SDL_CreateWindow(m_titreFenetre.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, m_largeurFenetre, m_hauteurFenetre, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
	m_renderer = SDL_CreateRenderer( m_window, -1, SDL_RENDERER_ACCELERATED);

	if (m_window == 0) {
		std::cout << "Erreur lors de la creation de la fenetre : " << SDL_GetError() << std::endl;
		SDL_Quit();
		return false;
	}

		 // Création du contexte OpenGL
	m_contexteOpenGL = SDL_GL_CreateContext(m_window);
	if (m_contexteOpenGL == 0) {
		std::cout << SDL_GetError() << std::endl;
		SDL_DestroyWindow(m_window);
		SDL_Quit();
		return false;
	}
	return true;
}

bool Scene::initGL() {
	srand (time(NULL));
#ifdef WIN32
	GLenum initialisationGLEW( glewInit() );
	if(initialisationGLEW != GLEW_OK) {
		std::cout << "Erreur d'initialisation de GLEW : " << glewGetErrorString(initialisationGLEW) << std::endl;
		SDL_GL_DeleteContext(m_contexteOpenGL);
		SDL_DestroyWindow(m_window);
		SDL_Quit();
		return false;
	}
#endif
	glEnable (GL_BLEND);
	// glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable( GL_DEPTH_TEST );
	glDepthFunc(GL_LESS);
	return true;
}

void Scene::init() {
	Sphere::create_array();
	Cylinder::create_array();
	
	l_shaders = std::vector<Shader*>(4);
	l_shaders[0] = new Shader("shaders/simple.vert", "shaders/simple.frag");
	l_shaders[1] = new Shader("shaders/texture.vert", "shaders/texture.frag");
	l_shaders[2] = new Shader("shaders/point.vert", "shaders/point.frag");
	l_shaders[3] = new Shader("shaders/toon.vert", "shaders/toon.frag");

	l_textures = std::vector<Texture*>(6);
	l_textures[0] = new Texture("Textures/boulet.jpg");
	l_textures[0]->charger();
	l_textures[1] = new Texture("Textures/leaf.png");
	l_textures[1]->charger();
	l_textures[2] = new Texture("Textures/square.png");
	l_textures[2]->charger();
	l_textures[3] = new Texture("Textures/full_square.png");
	l_textures[3]->charger();
	l_textures[4] = new Texture("Textures/skybox4.png");
	l_textures[4]->charger();
	l_textures[5] = new Texture("Textures/skybox.png");
	l_textures[5]->charger();
		// // Camera matrix
		// glm::mat4 view = glm::lookAt(glm::vec3(-10, -10, 0), // Camera is at (4,3,3), in World Space
		// 				 glm::vec3(0, 0, 0), // and looks at the origin
		// 				 glm::vec3(0, 0, 1)  // Head is up (set to 0,-1,0 to look upside-down)
	//);
	m_camera = CameraObject(vec3(-1.5, 0.5, 0.13), vec3(0.4, 0.5, 0.0), vec3(0, 0, 1), 0.005, 0.005); 
	//curent eye followed by where am I looking followed by the up vector and then some parameters..
	// m_camera = CameraObject(vec3(-1, 0.5, 0.3), vec3(0.4, 0.4, 0.2), vec3(0, 0, 1), 0.005, 0.005);
	
	m_projection = glm::perspective(glm::radians(45.0f), (float) m_largeurFenetre/ (float) m_hauteurFenetre, 0.1f, 100.0f);

	m_frameRate = 1000 / 50;
	m_debutBoucle = 0;
	m_finBoucle = 0;
	m_tempsEcoule = 0;
	m_input_rate = 1000 / 10;
	m_input_loop0 = 0;
	m_input_loop1 = 0;
	m_input_loop_time = 0;

	running = false;
	step_by_step = 0;
	end_ = false;
	re_init = false;
	back = 0;

	t = 0;
	stop = 1000000;
	sim_time = 0;
	frame_step = 0.0;
	frame_step_cur = mpm_conf::frame_rate_;
	
	Times::TIMES->init();

	sim = new Simulation(0);
	l_objects.push_back(sim);
}

void Scene::animate() {
	m_camera.lookAt(m_view);
	m_vp = m_projection * m_view;

	if (running || step_by_step > 0) {
		t += mpm_conf::replay_speed_;
		std::vector<Object*>::iterator it;
		for (auto& o : l_objects) {
			o->animate();
		}
		// for (it = l_objects.begin(); it != l_objects.end(); ++it) {
			// (*it)->animate();
		// }
	}
	if (step_by_step > 0) {
		--step_by_step;
	}
}

void Scene::draw() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

	std::list<Object*>::iterator it;
	for (it = l_objects.begin(); it != l_objects.end(); ++it) {
		(*it)->draw();
	}
}


void Scene::animationLoop() {
	// keyboard and mouse input events are handled here
	while(!end_) {
		m_input_loop0 = SDL_GetTicks();
		m_input.updateEvenements(); //update current input state

		if(m_input.getTouche(SDL_SCANCODE_ESCAPE) || m_input.terminer()) {
			end_ = true;
			break;
		}
		if(m_input.getTouche(SDL_SCANCODE_RETURN)) {
			if (running) {
				INFO(1, "STOP ANIMATION");
			} else {
				INFO(1, "RUN ANIMATION");
			}
			running = !running;
		}
		if(m_input.getTouche(SDL_SCANCODE_SPACE)) {
			--back;
			++step_by_step;
		}
		if(m_input.getTouche(SDL_SCANCODE_BACKSPACE)) {
			re_init = true;
		}
		if(m_input.getTouche(SDL_SCANCODE_B) ) {
			++step_by_step;
			++back;
		}
		if(m_input.getTouche(SDL_SCANCODE_S) ) {
			mpm_conf::display_sphere_ = !mpm_conf::display_sphere_;
			if (mpm_conf::display_sphere_) {
				INFO(1, "Display sphere");
			} else {
				INFO(1, "Do not display sphere");
			}
		}
		if(m_input.getTouche(SDL_SCANCODE_EQUALS) ) {
			if (mpm_conf::replay_speed_ >= 1) {
				mpm_conf::replay_speed_ *= 2;
			} else if (mpm_conf::replay_speed_ < -1) {
				mpm_conf::replay_speed_ /= 2;
			} else {
				assert( mpm_conf::replay_speed_ == -1);
				mpm_conf::replay_speed_ = 1;
				INFO(1, "Speed x"<<mpm_conf::replay_speed_);
			}
			INFO(1, "Speed x"<<mpm_conf::replay_speed_);
		}
		if(m_input.getTouche(SDL_SCANCODE_MINUS) ) {
			if (mpm_conf::replay_speed_ <= -1) {
				mpm_conf::replay_speed_ *= 2;
			} else if (mpm_conf::replay_speed_ > 1) {
				mpm_conf::replay_speed_ /= 2;
			} else {
				assert( mpm_conf::replay_speed_ == 1);
				mpm_conf::replay_speed_ = -1;
			}
			INFO(1, "Speed x"<<mpm_conf::replay_speed_);
		}
		m_camera.deplacer(m_input); // move the camera based on the input 

		m_input_loop1 = SDL_GetTicks();
		m_input_loop_time = m_input_loop1 - m_input_loop0;
		if(m_input_loop_time < m_input_rate) {
			SDL_Delay(m_input_rate - m_input_loop_time);
		}
	}
}

void Scene::bouclePrincipale() { // main loop
	std::thread t1(animateScene);
	sim->init();
	FLOAT spacing =  mpm_conf::grid_spacing_;

	FLOAT time_elapsed = 0.0;
	
	while(!end_) {
		m_debutBoucle = SDL_GetTicks();
		if (re_init) {
			sim->clear();
			sim->init();
			Times::TIMES->init();
			re_init = false;
		}
		if (back) {
			sim->backward(back+mpm_conf::replay_speed_);
			back = 0;
		}
		Times::TIMES->tick(Times::total_time_);
		animate();
		draw();

		if (t > stop) {
			end_ = true;
		}

		Times::TIMES->tock(Times::total_time_);
		if (running) {
			INFO(2, "   total_time_ "<< time_elapsed);
			// INFO(2, "Times : simu "<< Times::TIMES->getTime(Times::simu_time_)
			// 	<<"   display "<< Times::TIMES->getTime(Times::display_time_)
			// 	<<"   total "<< Timem_windows::TIMES->getTime(Times::total_time_));
			time_elapsed += (FLOAT)Times::TIMES->getTime(Times::total_time_);
			Times::TIMES->next_loop();
		}
		INFO(2, "WALL CLOCK TIME IS ---------" << time_elapsed);
		INFO(2, "SIM TIME IS ---------" << time_elapsed);

		sim_time += t*mpm_conf::dt_; // have to save dt_ as well too much work 
		// if (sim_time > frame_step and mpm_conf::save_img)
		if (t%mpm_conf::export_step_ == 0 and mpm_conf::save_img)
		{
			frame_step += frame_step_cur;
			std::stringstream ss;
		    ss << std::setw(4) << std::setfill('0') << t/mpm_conf::export_step_;
		    std::string fnum = ss.str();
			std::string str(image_path + ss.str() + ".png");
			std::ofstream file(str.c_str());
			ERROR(file.good(), "cannot open file \""<<str<<"\"", "");
			INFO(1, "Exporting img \""<<str<<"\"");
			std::cout << "the value is: " << saveScreenshotPNG(str) << std::endl;	\
		}

		// SDL_RenderPresent(m_renderer);
		SDL_GL_SwapWindow(m_window);
		m_finBoucle = SDL_GetTicks();
		m_tempsEcoule = m_finBoucle - m_debutBoucle;
		if(m_tempsEcoule < m_frameRate) {
			SDL_Delay(m_frameRate - m_tempsEcoule);
		}
	}
	t1.join();
}

void Scene::getProjView(glm::mat4 &vp) {
	vp = m_vp;
}

Shader* Scene::getShader(uint i) {
	return l_shaders[i];
}

Texture* Scene::getTexture(uint i) {
	return l_textures[i];
}

glm::vec3 Scene::getCameraPosition() const {
	return m_camera.getPosition();
}

void Scene::setLoad(std::string s) {
	sim->setLoad(s);
}

void Scene::setExport(std::string s) {
	sim->setExport(s);
}

void Scene::setImage(std::string s) {
	image_path = s;
	mpm_conf::save_img = true;
}

void Scene::setImport(std::string s) {
	sim->setImport(s);
}

void Scene::setScene(std::string s) {
	sim->setScene(s);
}

void Scene::setRun(bool run) {
	if (running) {
		INFO(1, "STOP ANIMATION");
	} else {
		INFO(1, "RUN ANIMATION");
	}
	running = run;
}

void Scene::setStop(uint t_end) {
	stop = t_end;
}

FLOAT Scene::getTime() {
	return t*mpm_conf::dt_;
}

bool Scene::saveScreenshotPNG(std::string filepath) {
	// some hack to display the current results
	// unsigned char* image = (unsigned char*)malloc(sizeof(unsigned char) * 3 * m_largeurFenetre * m_hauteurFenetre);
	BYTE* image = new  BYTE[m_largeurFenetre * m_hauteurFenetre/2 * 3];
	glReadPixels(0, m_hauteurFenetre/2, m_largeurFenetre, m_hauteurFenetre/2, GL_BGR, GL_UNSIGNED_BYTE, image);
	// Convert to FreeImage format & save to file
	FIBITMAP* img = FreeImage_ConvertFromRawBits(image, m_largeurFenetre, m_hauteurFenetre/2, 3 * m_largeurFenetre, 24, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, false);
	// FIBITMAP* img = FreeImage_ConvertFromRawBits(image, m_largeurFenetre, m_hauteurFenetre, 3 , 8, 0xFF0000, 0x00FF00, 0x0000FF, false);
	FreeImage_Save(FIF_PNG, img, filepath.c_str(), 0);
	// Free resources
	FreeImage_Unload(img);
	delete [] image;
	return true;
}
#endif
