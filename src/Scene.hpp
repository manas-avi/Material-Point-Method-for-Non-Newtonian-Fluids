#ifndef NO_GRAPHICS_ 

#ifndef SCENE_HPP
#define SCENE_HPP

#ifdef WIN32
#include <GL/glew.h>

#else
#define GL3_PROTOTYPES 1
#include <GLES3/gl3.h>

#endif

#ifndef GL_BGR
#define GL_BGR 0x80E0
#endif


#include <SDL2/SDL.h>
#include <iostream>
#include <string>
#include <vector>
#include <list>
#include "Input.hpp"
#include "Texture.hpp"
#include "Shader.hpp"
#include "CameraObject.hpp"
#include "Object.hpp"
#include "Simulation.hpp"
#include <FreeImage.h>
//#include <SDL2/SDL_ttf.h>

class Scene {
public:
	static Scene* SCENE;
	static void animateScene();
	
	Scene();
	Scene(std::string titreFenetre, int largeurFenetre, int hauteurFenetre);
	~Scene();

	bool initialiserFenetre();
	bool initGL();
	void animationLoop();
	void bouclePrincipale();

	void init();
	void animate();
	void draw();

	void getProjView(glm::mat4 &vp);
	Shader* getShader(uint i);
	Texture* getTexture(uint i);
	glm::vec3 getCameraPosition() const;

	void setLoad(std::string);
	void setExport(std::string);
	void setImage(std::string);
	void setImport(std::string);
	void setScene(std::string);

	void setRun(bool run);
	void setStop(uint t_end);

	FLOAT getTime();
	bool running;
private:
	uint step_by_step;
	uint t;
	FLOAT sim_time;
	FLOAT frame_step;
	FLOAT frame_step_cur;
	bool end_;
	bool re_init;
	int back;
	uint stop;
	
	std::string m_titreFenetre;
	int m_largeurFenetre;
	int m_hauteurFenetre;

	SDL_Window* m_window;
	SDL_GLContext m_contexteOpenGL;
	SDL_Renderer* m_renderer;
	//  TTF_Font *font;
	
	//  SDL_Event m_evenements;
	std::string image_path;

	Input m_input;
	
	CameraObject m_camera;
	glm::mat4 m_projection;
	glm::mat4 m_view;
	glm::mat4 m_vp;

	std::vector<Shader*> l_shaders;
	std::vector<Texture*> l_textures;
	//std::vector<glm::mat4>  l_model_views;

	std::list<Object*> l_objects; // stores just the simulation pointer
	Simulation *sim;

	unsigned int m_frameRate;
	Uint32 m_debutBoucle, m_finBoucle, m_tempsEcoule;
	unsigned int m_input_rate;
	Uint32 m_input_loop0, m_input_loop1, m_input_loop_time;
	bool saveScreenshotPNG(std::string filepath);
};

#endif

#endif
