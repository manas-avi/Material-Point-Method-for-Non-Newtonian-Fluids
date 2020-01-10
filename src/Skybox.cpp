#ifndef NO_GRAPHICS_ 

#include "Skybox.hpp"
#include "Scene.hpp"
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/glm.hpp>

const float Skybox::vertices[108] = {0, 0, 0,   0, 1, 0,   0, 0, 1,     // Face 1
				     0, 0, 1,   0, 1, 0,   0, 1, 1,// Face 1

				     0, 1, 1,   1, 0, 1,   0, 0, 1,        // Face 2
				     1, 0, 1,   0, 1, 1,   1, 1, 1,     // Face 2

				     0, 1, 0,   0, 0, 0,   1, 1, 0,    // Face 3
				     1, 1, 0,   0, 0, 0,   1, 0, 0,   // Face 3

				     0, 1, 0,   1, 1, 0,   1, 1, 1,       // Face 4
				     0, 1, 0,   1, 1, 1,   0, 1, 1, // Face 4

				     1, 1, 0,   1, 0, 0,   1, 0, 1,// Face 5
				     1, 1, 0,   1, 0, 1,   1, 1, 1, // Face 5

				     1, 0, 0,   0, 0, 0,   0, 0, 1, // Face 6
                                     1, 0, 0,   0, 0, 1,   1, 0, 1};

const float Skybox::normals[108] =  {1, 0, 0,   1, 0, 0,   1, 0, 0,           // Face 1
				     1, 0, 0,   1, 0, 0,   1, 0, 0,         // Face 1

				   0, 0.0, -1.0,   0, 0.0, -1.0,   0, 0.0, -1.0,           // Face 2
				   0, 0.0, -1.0,   0, 0.0, -1.0,   0, 0.0, -1.0,           // Face 2

				   0.0, 0.0, 1.0,   0.0, 0.0, 1.0,   0.0, 0.0, 1.0,           // Face 3
				   0.0, 0.0, 1.0,   0.0, 0.0, 1.0,   0.0, 0.0, 1.0,           // Face 3

				   0.0, -1.0, 0.0,   0.0, -1.0, 0,   0.0, -1.0, 0.0,           // Face 4
				   0.0, -1.0, 0.0,   0.0, -1.0, 0,   0.0, -1.0, 0.0,           // Face 4

				     -1, 0, 0,   -1, 0, 0,   -1, 0, 0,           // Face 5
				     -1, 0, 0,   -1, 0, 0,   -1, 0, 0,         // Face 5

				   0.0, 1.0, 0.0,   0.0, 1.0, 0.0,   0.0, 1.0, 0.0,           // Face 6
				   0.0, 1.0, 0.0,   0.0, 1.0, 0.0,   0.0, 1.0, 0.0};

const  float Skybox::coordTexture[72] =  {1, 0.333,   0.75, 0.333,    1, 0.666,  //Face 1
					  1,  0.666,   0.75, 0.333,   0.75, 0.666, 
					  
					  0.75, 0.666,   1, 1,   1, 0.666,  // Face 2
					  1, 1,   0.75, 0.666,   0.75, 1,

					  0.75, 0.333,   1, 0.333,   0.75, 0,  // Face 3 
 					  0.75, 0,   1, 0.333,   1, 0,

					  0.75, 0.333,   0.5, 0.333,   0.5, 0.666,   //Face 4
					  0.75, 0.333,   0.5, 0.666,   0.75, 0.666,

					  0.5, 0.333,   0.25, 0.333,   0.25, 0.666,  // Face 5
					  0.5, 0.333,   0.25, 0.666,   0.5, 0.666,

					  0.25, 0.333,   0, 0.333,   0, 0.666,  // Face 6
					  0.25, 0.333,   0, 0.666,   0.25, 0.666  };

Skybox::Skybox(float s, int shader, int texture): Object(shader, texture) {
    size = s;
    glm::vec3 axe(0.0, 0.0, 1.0);
    glm::mat4 rot = glm::rotate((float)-1.57, axe)*translate(glm::vec3(-1, 0, 0));
    m_model_view = translate(glm::vec3(-0.5*s + 0.5, -0.5*s + 0.5, -0.3*s + 0.5))*scale(glm::vec3(s, s, s))*rot;

    }

Skybox::~Skybox() {}

  void Skybox::animate() {
    //    m_model_view = rotate(m_model_view, (float)0.02, glm::vec3(0, 1, 0));
  // m_model_view = translate(m_model_view, glm::vec3(0.0, 5.0, 0.0));
  }
  
  void Skybox::draw(glm::mat4 m, int s) {
    float couleurs[108] = {1.0, 1.0, 1.0,   1.0, 1.0, 1.0,   1.0, 1.0, 1.0,           // Face 1
		1.0, 1.0, 1.0,   1.0, 1.0, 1.0,   1.0, 1.0, 1.0,           // Face 1

		1.0, 1.0, 1.0,   1.0, 1.0, 1.0,   1.0, 1.0, 1.0,           // Face 2
		1.0, 1.0, 1.0,   1.0, 1.0, 1.0,   1.0, 1.0, 1.0,           // Face 2

		1.0, 1.0, 1.0,   1.0, 1.0, 1.0,   1.0, 1.0, 1.0,           // Face 3
		1.0, 1.0, 1.0,   1.0, 1.0, 1.0,   1.0, 1.0, 1.0,           // Face 3

		1.0, 1.0, 1.0,   1.0, 1.0, 1.0,   1.0, 1.0, 1.0,           // Face 4
		1.0, 1.0, 1.0,   1.0, 1.0, 1.0,   1.0, 1.0, 1.0,           // Face 4

		1.0, 1.0, 1.0,   1.0, 1.0, 1.0,   1.0, 1.0, 1.0,           // Face 5
		1.0, 1.0, 1.0,   1.0, 1.0, 1.0,   1.0, 1.0, 1.0,           // Face 5

		1.0, 1.0, 1.0,   1.0, 1.0, 1.0,   1.0, 1.0, 1.0,           // Face 6
		1.0, 1.0, 1.0,   1.0, 1.0, 1.0,   1.0, 1.0, 1.0};

    m_shader = 1;
    m_texture = 5;
    
    enableShader();
    setMVP(m, s);
    // glm::mat4 vp;
    // Scene::SCENE->getProjView(vp);
    // glm::mat4 mvp = vp * m_model_view;
    //   glUniformMatrix4fv(glGetUniformLocation(m_shader->getProgramID(), "mvp"), 1, GL_FALSE, value_ptr(mvp));

    
      
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vertices);
    glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, couleurs);
  glEnableVertexAttribArray(1);

  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, normals);
  glEnableVertexAttribArray(2);

  glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, coordTexture);
  glEnableVertexAttribArray(3);

  glDrawArrays(GL_TRIANGLES, 0, 36);

  glDisableVertexAttribArray(3);
  glDisableVertexAttribArray(2);
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(0);

  disableShader();
  }

#endif
