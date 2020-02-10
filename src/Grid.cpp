#include "Grid.hpp"

//#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <omp.h>
#include "Eigen/IterativeLinearSolvers"

#include "Scene.hpp"
#include "Simulation.hpp"
#include "Sphere.hpp"
#include "error.hpp"
#include "mpm_conf.hpp"
#include <list>

inline uint Grid::index(uint i, uint j, uint k) const {
	return i*(j_max+1)*(k_max+1) + j*(k_max+1) + k;
}

Vector3i Grid::nodeFromIndex(int ind) const {
	int i, j, k;
	i = (int) ind / ((j_max+1)*(k_max+1));
	int aux = ind - i*(j_max+1)*(k_max+1);
	j = (int) aux / (k_max+1);
	k = aux - j*(k_max+1);
  //TEST(ind == index(i, j, k));
  //  INFO(3, "nide from index "<<ind<<" "<<aux<<" " << i<<" "<<j<<" "<<k);
  // TEST(index(i, j, k) == ind);
	return Vector3i(i, j, k);
}

Grid::Grid() : Object(), x_max(0),  y_max(0), z_max(0),  i_max(0),  j_max(0),  k_max(0) {
}

Grid::Grid(FLOAT width, FLOAT depth, FLOAT height, FLOAT space_step, int shader): Object(shader) {
  // assert(m_shader != -1);
	x_max = width;
	y_max = depth;
	z_max = height;
	spacing = space_step;

	i_max = x_max/mpm_conf::grid_spacing_ + 1;
	j_max = y_max/mpm_conf::grid_spacing_ + 1;
	k_max = z_max/mpm_conf::grid_spacing_ + 1;

	nb_lines = ((i_max+1)*(j_max+1) + (k_max+1)*(j_max+1) +(k_max+1)*(i_max+1));
	nb_nodes = (i_max+1)*(j_max+1)*(k_max+1);
	nb_cells = (i_max)*(j_max)*(k_max);
	kernel_size = 2;
	std::cout<<"i max "<< i_max<<" "<<j_max<<" "<<k_max<<std::endl;
  // std::cout<<"x max "<< x_max<<" "<<y_max<<" "<<z_max<<std::endl;

	masses = std::vector<FLOAT>(nb_nodes);
	active_nodes = std::vector<bool>(nb_nodes);
	velocities = std::vector<VEC3>(nb_nodes);
	inter_velocities = std::vector<VEC3>(nb_nodes);
	prev_velocities = std::vector<VEC3>(nb_nodes);
	positions = std::vector<VEC3>(nb_nodes);
	new_positions = std::vector<VEC3>(nb_nodes);
	cells = std::vector<std::list<Particule*> >(nb_cells);
	distance_collision = std::vector<FLOAT>(nb_nodes);
	// second_der = std::vector<std::vector<MAT3> >(nb_nodes);

	// for (uint i = 0; i <= nb_nodes; ++i) {
	//   second_der[i] = std::vector<MAT3>(nb_nodes);
	// }
	for (uint i = 0; i <= i_max; ++i) {
		for (uint j = 0; j <= j_max; ++j) {
			for (uint k = 0; k <= k_max; ++k) {
				//std::cout<<index(i, j, k)<<std::endl;
				positions[index(i, j, k)] = VEC3(i*mpm_conf::grid_spacing_, j*mpm_conf::grid_spacing_, 
					k*mpm_conf::grid_spacing_);
				velocities[index(i, j, k)] = VEC3(0, 0, 0);
			}
		}
	}

#ifndef NO_GRAPHICS_ 
	// creation of staggered grid...
	vertices = new GLfloat[nb_lines*6];
	colors = new GLfloat[nb_lines*6];

	// std::cout<<"size "<<((i_max+1)*(j_max+1) + (k_max+1)*(j_max+1) +(k_max+1)*(i_max+1))*6<<std::endl;
	uint h = 0;
	for (uint i = 0; i <= i_max; ++i) {
		for (uint j = 0; j <= j_max; ++j) {
			VEC3 p1 = positions[index(i, j, 0)];
			VEC3 p2 = positions[index(i, j, k_max)];
			for (uint l = 0; l < 3; ++l) {
				vertices[h+l] = p1(l);
				vertices[h+l+3] = p2(l);
			}
			h += 6;
		}
	}

	for (uint i = 0; i <= i_max; ++i) {
		for (uint k = 0; k <= k_max; ++k) {
			VEC3 p1 = positions[index(i, 0, k)];
			VEC3 p2 = positions[index(i, j_max, k)];
			for (uint l = 0; l < 3; ++l) {
				vertices[h+l] = p1(l);
				vertices[h+l+3] = p2(l);
			}
			h += 6;
		}
	}
	for (uint k = 0; k <= k_max; ++k) {
		for (uint j = 0; j <= j_max; ++j) {
			VEC3 p1 = positions[index(0, j, k)];
			VEC3 p2 = positions[index(i_max, j, k)];
			// std::cout<<p1<<"\n"<<std::endl;
			// std::cout<<p2<<"\n\n\n"<<std::endl;
			for (uint l = 0; l < 3; ++l) {
				vertices[h+l] = p1(l);
				vertices[h+l+3] = p2(l);
			}
			h += 6;
		}
	}

	for (uint i = 0; i < nb_lines*6; i+=3) {
		colors[i] = 1;
		colors[i+1] = 0;
		colors[i+2] = 0;
	}
 #endif
	nextStep();
}


void Grid::animate() {

}

#ifndef NO_GRAPHICS_ 
void Grid::draw(glm::mat4 m, int s) {
	if (!positions.empty()) {
		GLfloat ext_vert[72];
		for (uint l = 0; l < 3; ++l) {
			ext_vert[l] = positions[index(0, 0, 0)](l);
			ext_vert[l+3] = positions[index(i_max, 0, 0)](l);

			ext_vert[l+6] = positions[index(0, 0, 0)](l);
			ext_vert[l+9] = positions[index(0, j_max, 0)](l);

			ext_vert[l+12] = positions[index(0, 0, 0)](l);
			ext_vert[l+15] = positions[index(0, 0, k_max)](l);

			ext_vert[l+18] = positions[index(i_max, j_max, k_max)](l);
			ext_vert[l+21] = positions[index(i_max, j_max, 0)](l);

			ext_vert[l+24] = positions[index(i_max, j_max, k_max)](l);
			ext_vert[l+27] = positions[index(i_max, 0, k_max)](l);

			ext_vert[l+30] = positions[index(i_max, j_max, k_max)](l);
			ext_vert[l+33] = positions[index(0, j_max, k_max)](l);

			ext_vert[l+36] = positions[index(i_max, 0, 0)](l);
			ext_vert[l+39] = positions[index(i_max, j_max, 0)](l);

			ext_vert[l+42] = positions[index(i_max, 0, 0)](l);
			ext_vert[l+45] = positions[index(i_max, 0, k_max)](l);

			ext_vert[l+48] = positions[index(0, j_max, 0)](l);
			ext_vert[l+51] = positions[index(i_max, j_max, 0)](l);

			ext_vert[l+54] = positions[index(0, j_max, 0)](l);
			ext_vert[l+57] = positions[index(0, j_max, k_max)](l);

			ext_vert[l+60] = positions[index(0, 0, k_max)](l);
			ext_vert[l+63] = positions[index(i_max, 0, k_max)](l);

			ext_vert[l+66] = positions[index(0, 0, k_max)](l);
			ext_vert[l+69] = positions[index(0, j_max, k_max)](l);
		}
		GLfloat ext_col[72];
		for (uint l = 0; l < 72; l+=3) {
			ext_col[l] = 1.0f;
			ext_col[l+1] = 0;
			ext_col[l+2] = 0;
		}

		glLineWidth(1.0f);

		enableShader();
		setMVP(m, s);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, ext_vert);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, ext_col);
		glEnableVertexAttribArray(1);

		glDrawArrays(GL_LINES, 0, 24);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);

		disableShader();

		glLineWidth(1.0f);

		enableShader();
		setMVP(m, s);

		if (mpm_conf::enable_debugging) {

			glLineWidth(1.0f);
			for (uint i = 0; i <= i_max; ++i) {
				for (uint j = 0; j <= j_max; ++j) {
					for (uint k = 0; k <= k_max; ++k) {
						uint ind = index(i, j, k);
						if (active_nodes[ind]) {

							VEC3 pos = positions[ind];
							VEC3 vel = mpm_conf::dt_ * velocities[ind]+pos; 
							GLfloat vel_line[6] = {pos(0),  pos(1), pos(2), vel(0), vel(1), vel(2)};
							GLfloat vel_color[6] = {0, 0, 1, 0, 0, 1};
							GLfloat vel_color2[6] = {1, 0, 0, 1, 0, 0};

							enableShader();
							setMVP(m, s);

							glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vel_line);
							glEnableVertexAttribArray(0);

							if (velocities[ind].norm() < 2) {
								glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, vel_color);
							} else {
								glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, vel_color2);
							}
							glEnableVertexAttribArray(1);

							glDrawArrays(GL_LINES, 0, 2);

							glDisableVertexAttribArray(0);
							glDisableVertexAttribArray(1);

							disableShader();
						}
					}
				}
			}
		}

	}
}
#endif

VEC3 Grid::position(uint i, uint j, uint k) const {
	assert(i <= i_max);
	assert(j <= j_max);
	assert(k <= k_max);
	return positions[index(i, j, k)];
}

VEC3 Grid::velocity(uint i, uint j, uint k) const {
	assert(i <= i_max);
	assert(j <= j_max);
	assert(k <= k_max);
	return velocities[index(i, j, k)];
}

VEC3 & Grid::velocity(uint i, uint j, uint k) {
	assert(i <= i_max);
	assert(j <= j_max);
	assert(k <= k_max);
	return velocities[index(i, j, k)];
}

void Grid::nextStep() {
#pragma omp parallel for
	for (uint i = 0; i < nb_nodes; ++i) {
		masses[i] = 0;
		active_nodes[i] = false;
		inter_velocities[i] = VEC3(0, 0, 0);
		prev_velocities[i] = velocities[i]; 
		velocities[i] = VEC3(0, 0, 0);
	}
#pragma omp parallel for
	for (uint i = 0; i < nb_cells; ++i) {
		cells[i].clear();
	}
}

void Grid::smoothVelocity() {
	std::vector<FLOAT> laplacien = {
		0, 0, 0,
		0, 1.0/6.0, 0,
		0, 0, 0,

		0, 1.0/6.0, 0,
		1.0/6.0, -1, 1.0/6.0,
		0, 1.0/6.0, 0,

		0, 0, 0,
		0, 1.0/6.0, 0,
		0, 0, 0
	};

	#pragma omp parallel for
	for (int i = 0; i <= (int)i_max; ++i) {
		for (int j = 0; j <= (int)j_max; ++j) {
			for (int k = 0; k <= (int)k_max; ++k) {
				uint ind = index(i, j, k);
				if (active_nodes[ind]) 
				{
					VEC3 sv(0, 0, 0);

					for (int l = 0; l < 3; ++l) 
					{
						if (i-1+l >= 0 && i-1+l < (int)i_max) 
						{
							for (int m = 0; m < 3; ++m) 
							{
								if (j-1+m >= 0 && j-1+m < (int)j_max) 
								{
									for (int n = 0; n < 3; ++n) 
									{
										if (k-1+n >= 0 && k-1+n < (int)k_max) 
										{
											uint ind_neigh = index(i-1+l, j-1+m, k-1+n);
											if (active_nodes[ind_neigh]) 
											{
												sv += inter_velocities[ind_neigh]*laplacien[l*9 + m*3 + n];
											} else 
											{
												sv += inter_velocities[ind]*laplacien[l*9 + m*3 + n];
											}
										      // if (active_nodes[ind]) {
										      // 	if (inter_velocities[ind_neigh](0) != 0) {
										      // 	  INFO(3,"laplace coef\n "<<inter_velocities[ind_neigh]);
										      // 	}
										      // }
										}
									}
								}
							}
						}
					}
					velocities[ind] += 1*sv;
				  // if (active_nodes[ind]) {
				  //   INFO(3, "lapl vel\n"<<sv);
				  // }
				}

			}
		}
	}
}

void Grid::removeEscapedParticles(std::vector<Particule*> & particules) {

	std::vector<Particule*> new_particules;
	for (uint ip = 0; ip < particules.size(); ++ip) {
		Particule *p = particules[ip];
		Vector3i cell = p->getCell();

		// check for this assertion otherwise the code gets stuck in an infinite loop
		int i_curr = cell(0);
		int j_curr = cell(1);
		int k_curr = cell(2);

		if (i_curr < 0 or i_curr > i_max or j_curr < 0 or j_curr > j_max or k_curr < 0 or k_curr > k_max) {
			// do nothing
		}
		else {
			new_particules.push_back(p);
		}

	}
	particules = new_particules;
}

void Grid::particulesToGrid(std::vector<Particule*> & particules) {

	removeEscapedParticles(particules);
	checkParticles(particules);
	for (auto &p : particules) {
	    Vector3i c = p->getCell(); // tells which cell is it in
	    uint ind = c(0)*j_max*k_max + c(1)*(k_max) + c(2);
	    if (ind < nb_cells) {
	    	cells[ind].push_back(p);
	    }   
	}

	uint nb_ac = 0;
	for (uint i = 0; i < cells.size(); ++i) {
		if (cells[i].size() != 0) {
			++nb_ac;
		}
	}
	// INFO(2, "Part 2 Grid");
	// INFO(3, particules.front()->getVelocity());
	FLOAT s2 = mpm_conf::grid_spacing_*mpm_conf::grid_spacing_; 
	// grid spacing square used in D matrix for apic based calculations

	#pragma omp parallel for
	for (int i = 0; i <= (int)i_max; ++i) {
		for (int j = 0; j <= (int)j_max; ++j) {
			for (int k = 0; k <= (int)k_max; ++k) {
				uint ind = index(i, j, k);
				VEC3 f(0, 0, 0);
				for (int l = i - kernel_size; l < i + kernel_size; ++l) {
					if (l >= 0 && l < (int)i_max) {
						for (int m = j - kernel_size; m < j + kernel_size; ++m) {
							if (m >= 0 && m < (int)j_max) {
								for (int n = k - kernel_size; n < k + kernel_size; ++n) {
									if (n >= 0 && n < (int)k_max) {
										uint indc = l*j_max*k_max + m*(k_max) + n;
										for (auto& p : cells[indc]) {
											FLOAT w = p->weight(Vector3i(i, j, k));
											if (w > 0) {
												active_nodes[ind] = true; // make the nodes active
																		 // that are recieving particle data
												
												// Project velocities and mass on the grid --->
												masses[ind] += w*p->getMass();
												if (mpm_conf::method_ == mpm_conf::apic_) {
													MAT3 C = 3.0/s2*p->getB(); // ~ Dp matrix in apic paper
													MAT3 C_skew = 0.5*(C - C.transpose());
													MAT3 C_sym = 0.5*(C + C.transpose());
													// FLOAT v = 1;
													// FLOAT v = 0;
													// INFO(3, "particle vel at index " << ind << "is " << p->getVelocity());
													// velocities[ind] += w*p->getMass()*(p->getVelocity() +
													//  (C_skew + (1-v)*C_sym)*(positions[ind] - p->getPosition()));
													velocities[ind] += w*p->getMass()*(p->getVelocity() +
													 C*(positions[ind] - p->getPosition()));
													// TODO experiment with this fishy "v" parameter
												} else {
													velocities[ind] += w*p->getMass()*p->getVelocity();
												}
												// compute different forces on the grid ----->
												
												// TODO remove this commented line later 
												IS_DEF(velocities[ind](0));
												VEC3 incrf = p->getForceIncrement()*p->gradWeight(Vector3i(i, j, k));
												VEC3 extrf = p->getForce();
												if (!std::isnan(incrf(0)) && !std::isinf(incrf(0))) {
													f += incrf;
													f += extrf;
												}
												IS_DEF(f(0));
												
											}
										}
									}
								}
							}
						}
					}
				}
				if (active_nodes[ind]) {
					IS_DEF(velocities[ind](0));
					// INFO(3, "vel at index " << ind << "is " << velocities[ind]);
					if (masses[ind] > /*1e-8*mpm_conf::dt_*/0) { 
						// don't comapte with threshold can lead to oscillations
					    //  IS_DEF(f(0));
						if (std::isnan(f(0)) || std::isinf(f(0))) {
							f = VEC3(0, 0, 0);
						}
						// Update the velocities on the grid based on the forces that are applied --->
						// TODO remove the below comment
						velocities[ind] -= mpm_conf::dt_*f + mpm_conf::dt_*mpm_conf::damping_*velocities[ind];
						velocities[ind] /= masses[ind]; // kind of centre of mass velocities of neighbours

					} else { // if mass of this node is zero => it does not contribute
						velocities[ind] = VEC3(0, 0, 0);
					}
					// Apply the gravitational force
					IS_DEF(velocities[ind](0));
					velocities[ind] += mpm_conf::dt_*mpm_conf::gravity_;
					// INFO(3, "vel at index " << ind << "is " << velocities[ind]);

					// redundant information
					inter_velocities[ind] = velocities[ind];//particules.front()->gradWeight(Vector3i(i, j, k));
					new_positions[ind] = positions[ind] + mpm_conf::dt_*velocities[ind];
					IS_DEF(velocities[ind](0));

					// Add other electro-magnetic forces here ---->
					/* **************************************** */ 
				}
			}
		}
	}
// exit(1);
	//INFO(2, "END Part 2 Grid");
}

// debug function to check on particles
void Grid::checkParticles(std::vector<Particule*> & particules) {
	for (uint ip = 0; ip < particules.size(); ++ip) {
		Particule *p = particules[ip];
		Vector3i cell = p->getCell();

		// check for this assertion otherwise the code gets stuck in an infinite loop
		int i_curr = cell(0);
		int j_curr = cell(1);
		int k_curr = cell(2);

		assert(i_curr <= i_max);
		assert(i_curr >= 0);
		assert(j_curr <= j_max);
		assert(j_curr >= 0);
		assert(k_curr <= k_max);
		assert(k_curr >= 0);
	}		
}

void Grid::gridToParticules(std::vector<Particule*> & particules) {
	// MAT3 orientation = particules[1000]->getOrientation();
	// EigenSolver<MatrixXd> es(orientation);
	// MatrixXd D = es.pseudoEigenvalueMatrix();
	// MatrixXd V = es.pseudoEigenvectors();
	// INFO(3, "ORIENTATION\n"<<V<<"\n\n"<<D);
	// INFO(2, "Grid 2 Part");

	removeEscapedParticles(particules);
	checkParticles(particules);

	FLOAT s3 = pow(mpm_conf::grid_spacing_, 3);
	//for (auto& p : particules) {

	#pragma omp parallel for
	for (uint ip = 0; ip < particules.size(); ++ip) {
		Particule *p = particules[ip];
		Vector3i cell = p->getCell();
		VEC3 vel(0, 0, 0);
		VEC3 vel_flip = p->getVelocity();
    // INFO(3, "vel FLIP \n"<<vel_flip);
		MAT3 B = MAT3::Zero(); // apic matrix
		MAT3 T = MAT3::Zero();
		VEC3 pos(0, 0, 0);
		VEC3 prev_pos = p->getPosition();
		FLOAT density_max = 0;
		FLOAT density_av = 0;

		for (int i = cell(0) - kernel_size; i <= cell(0) + kernel_size; ++i) 
		{
			if (i >= 0 && i <= (int)i_max) 
			{
				for (int j = cell(1) - kernel_size; j <= cell(1) + kernel_size; ++j) 
				{
					if (j >= 0 && j <= (int)j_max) 
					{
						for (int k = cell(2) - kernel_size; k <= cell(2) + kernel_size; ++k) 
						{
							if (k >= 0 && k <= (int)k_max) 
							{
								uint ind = index(i, j, k);
								if (active_nodes[ind]) 
								{
									FLOAT w = p->weight(Vector3i(i, j, k));
									IS_DEF(w);
									IS_DEF(velocities[ind](0));
									// these are particle based velocity
									vel += w*velocities[ind]; // this is pic based update

									vel_flip += w*(velocities[ind] - prev_velocities[ind]); // this is flip based update
									B += w*velocities[ind]*(positions[ind] - p->getPosition()).transpose();
									if (mpm_conf::method_ == mpm_conf::apic_ || mpm_conf::method_ == mpm_conf::pic_)
									{
										pos += w*(positions[ind] + mpm_conf::dt_*velocities[ind]); //mult by w twice ?
										// in other methods position is updated using particle velocity informaiton
									}
									T += mpm_conf::cheat_damping_ *
										velocities[ind]*p->gradWeight(Vector3i(i, j, k)).transpose();
									// INFO(3, "vel g2p \n"<<prev_velocities[ind]);
									IS_DEF(velocities[ind](0));
									IS_DEF(w);
									IS_DEF(pos(0));
									if (density_max < masses[ind]) 
										density_max = masses[ind];
									density_av += masses[ind]*w;
								}
							}
						}
					}
				}
			}
		}
		density_max /= s3;
		density_av /= s3;

		p->setDensity(density_max);
		if (mpm_conf::method_ == mpm_conf::apic_ || mpm_conf::method_ == mpm_conf::pic_) {
			p->update(pos, vel, B, T);
		} else if (mpm_conf::method_ == mpm_conf::flip_) {
			p->update(pos, vel_flip, B, T);
		} else if  (mpm_conf::method_ == mpm_conf::mix_) {
			FLOAT alpha = 0.95;
			VEC3 v = alpha*vel_flip + (1-alpha)*vel;
			p->update(pos, v, B, T);
		}

	    /* Rotation */
		if (mpm_conf::anisotropy_on) {
	 		MAT3 A = MAT3::Zero(3, 3);
			FLOAT sum = 0;
			for (int i = cell(0) - 2; i <= cell(0) + 2; ++i) {
				if (i >= 0 && i <= (int)i_max) {
					for (int j = cell(1) - 2; j <= cell(1) + 2; ++j) {
						if (j >= 0 && j <= (int)j_max) {
							for (int k = cell(2) - 2; k <= cell(2) + 2; ++k) {
								if (k >= 0 && k <= (int)k_max) {
									uint ind = index(i, j, k);
									if (active_nodes[ind]) {
										FLOAT w = p->weight(Vector3i(i, j, k));
										sum += w;
										A += w * (new_positions[ind] - p->getPosition())*
										(positions[ind] - prev_pos).transpose();
									}
								}
							}
						}
					}
				}
			}
			A /= sum;
			JacobiSVD<MAT3> svd(A, ComputeFullU | ComputeFullV);
			MAT3 rot = svd.matrixU()*svd.matrixV().transpose();
			p->rotate(rot);

		}
	}
  // INFO(2, "END Grid 2 Part");
}

void Grid::init(std::vector<Particule*> & particules) {
	for (auto &p : particules) {
		Vector3i c = p->getCell();
		uint ind = c(0)*j_max*k_max + c(1)*(k_max) + c(2);
		if (ind < nb_cells) {
			cells[ind].push_back(p);
		}
	}

	// intialize the grid quantities
	FLOAT s2 = mpm_conf::grid_spacing_*mpm_conf::grid_spacing_;
	#pragma omp parallel for
	for (int i = 0; i <= (int)i_max; ++i) {
		for (int j = 0; j <= (int)j_max; ++j) {
			for (int k = 0; k <= (int)k_max; ++k) {
				uint ind = index(i, j, k);
				VEC3 f(0, 0, 0);
				for (int l = i - 2; l < i + 2; ++l) {
					if (l >= 0 && l < (int)i_max) {
						for (int m = j - 2; m < j + 2; ++m) {
							if (m >= 0 && m < (int)j_max) {
								for (int n = k - 2; n < k + 2; ++n) {
									if (n >= 0 && n < (int)k_max) {
										uint indc = l*j_max*k_max + m*(k_max) + n;
										for (auto& p : cells[indc]) {
											FLOAT w = p->weight(Vector3i(i, j, k));
											if (w > 0) {
												active_nodes[ind] = true;
												masses[ind] += w*p->getMass();
												velocities[ind] += w*p->getMass()*p->getVelocity();
											}
										}
									}
								}
							}
						}
					}
				}
				if (active_nodes[ind]) {
					if (masses[ind] > 0/*1e-8*mpm_conf::dt_*/) {
						velocities[ind] /= masses[ind];
					} else {
						velocities[ind] = VEC3(0, 0, 0);
					}
				}
			}
		}
	}
	// define the grid volume
	// for (auto &p : particules) { 
	// cannot do above as openMp requires for loop to be in the canonical form
	FLOAT s3 = pow(mpm_conf::grid_spacing_, 3);
	#pragma omp parallel for
	for (uint ip = 0; ip < particules.size(); ++ip) {
		Particule *p = particules[ip];

		/* rather use grid interpolated densities */
	    FLOAT density = 0;//mpm_conf::density_;;
	    Vector3i cell = p->getCell();
	    for (int i = cell(0) - 2; i <= cell(0) + 2; ++i) {
			if (i >= 0 && i <= (int)i_max) {
	    		for (int j = cell(1) - 2; j <= cell(1) + 2; ++j) {
	    	  		if (j >= 0 && j <= (int)j_max) {
	    	    		for (int k = cell(2) - 2; k <= cell(2) + 2; ++k) {
	    	      			if (k >= 0 && k <= (int)k_max) {
					    		uint ind = index(i, j, k);
					    		FLOAT w = p->weight(Vector3i(i, j, k));
					    		density += masses[ind]*w;
				    	    }
			    	    }
			    	}
		    	}
		    }
	    }
	    density /= s3;
		/* should not use constant density rather use grid based density*/
		// density = mpm_conf::density_;
	    p->initVolume(density);
	    //INFO(3,"density "<<mpm_conf::density_<<"   denstity local "<<density);
	}
}


// void Grid::updatebuf() {
// 	#pragma omp parallel for
// 	for (uint i = 0; i < nb_nodes; ++i) {
// 		bool val = active_nodes[i];
// 		active_nodes_buf[i] = val;
// 	}
// }

// void Grid::revertbuf() {
// 	#pragma omp parallel for
// 	for (uint i = 0; i < nb_nodes; ++i) {
// 		bool val = active_nodes_buf[i];
// 		active_nodes[i] = val;
// 	}
// }

void Grid::initCollision(std::vector<Obstacle*> obstacles) {
	#pragma omp parallel for
	for (uint i = 0; i < nb_nodes; ++i) {
		distance_collision[i] = 100; // some big distance
		for (auto & ob : obstacles) {

			FLOAT d = ob->distance(positions[i]);
			if (fabs(d) < fabs( distance_collision[i])) {
				distance_collision[i] = d; // distance between particule and nearest object before collision
			}
		}
	}
	printf("I am here\n");
}

void Grid::collision(std::vector<Obstacle*> obstacles) {
	// INFO(2, "Collision");
	// TODO try to improve how collision is handeled
	#pragma omp parallel for
	for (uint i = 0; i < nb_nodes; ++i) {
		if (active_nodes[i]) {
			
			VEC3 n(0, 0, 1);// = ob->getNormal(pos);

			FLOAT d = 10;// = ob->distance(pos);
			// FLOAT d_prev;
			FLOAT friction = mpm_conf::friction_coef_;
			VEC3 pos = positions[i] + mpm_conf::dt_*velocities[i];
			for (auto & ob : obstacles) {
				VEC3 n_cur;
				FLOAT d_cur;
				ob->getCollisionValues(pos, d_cur, n_cur);
				if (fabs(d_cur) < fabs(d)) {
					d = d_cur;
					n = n_cur; // update the normal vector
					friction = ob->getFriction();
				}
			}
			// distance_collision[i] stores the prev_distance
			// while d is with updated positions

			// TODO there is a problem with this method if the obstacle is very thin and it may get out of the other boundary
			FLOAT dcomp = d - std::min(distance_collision[i],(FLOAT)0.0);
  		    if (fabs(d) <= 1*mpm_conf::grid_spacing_ and dcomp < 0) { // there is collision and it is not seprating
				// IF IT IS LESS THEN A CERTAIN DISTANCE THEN adjust the velocities
				// so that it can come back to the surface
		      	FLOAT dv = -dcomp/mpm_conf::dt_;
				velocities[i] += dv*n*(1 + mpm_conf::rest_coeff_);
				// //friction
				VEC3 vt = velocities[i] - velocities[i].dot(n)*n;
				FLOAT nvt = vt.norm();

				if (nvt > friction*dv) {
				  	velocities[i] -= friction*dv*vt/nvt;
				} else {
				  	velocities[i] -= vt;
				} 
			}
		}
	}
}

bool Grid::checkParticlesAdapt(std::vector<Particule*> & particules_buf, FLOAT max_j, FLOAT min_j) {
	// TODO FILL THIS WITH USEFUL CRITERION
	// // first criterion with velocity was not so useful
	// bool fail = false;
	// FLOAT vel_mag_buf = 0;
	// FLOAT vel_min_buf = 10000;

	// for (uint ip = 0; ip < particules_buf.size(); ++ip) {
	// 	Particule *p = particules_buf[ip];
	// 	VEC3 p_vel = p->getVelocity();
	// 	vel_mag_buf = std::max(p_vel.norm(), vel_mag_buf);
	// 	vel_min_buf = std::min(p_vel.norm(), vel_min_buf);
	// }
	// printf("vel mag buf is %f\n", fabs(vel_mag_buf));
	// printf("vel mag is %f\n", fabs(vel_mag));
	// printf("abs vel diff is %f\n", fabs(vel_mag - vel_mag_buf));


	// printf("vel min buf is %f\n", fabs(vel_min_buf));
	// printf("vel min is %f\n", fabs(vel_min));
	// printf("abs vel diff is %f\n", fabs(vel_min - vel_min_buf));

	// // vel_mag is previous high velocity
	// // vel_mag_buf is the current high velocity
	// // if (vel_mag_buf - vel_mag > 1)
	// if (vel_mag_buf > vel_mag * 2)
	// 	return true;
	// if (vel_min_buf < vel_min * 0.9)
	// 	return true;

	// // second critertion with stress value
	FLOAT max_j_buf = 0;
	FLOAT min_j_buf = 10000;
	for (uint ip = 0; ip < particules_buf.size(); ++ip) {
		Particule *p = particules_buf[ip];
		FLOAT J = (p->getDeformation()).determinant();
		max_j_buf = std::max(max_j_buf, J);
		min_j_buf = std::min(min_j_buf, J);
	}
	printf("the value of max_j_buf is %f\n", max_j_buf);
	printf("the value of max_j is %f\n", max_j);
	printf("abs max diff is %f\n", fabs(max_j - max_j_buf));
	if (max_j_buf > max_j * 1.01)
		return true;

	printf("the value of min_j_buf is %f\n", min_j_buf);
	printf("the value of min_j is %f\n", min_j);
	printf("abs min diff is %f\n", fabs(min_j - min_j_buf));
	if (max_j_buf > max_j * 1.1)
		return true;
	if (min_j_buf < min_j * 0.9)
		return true;

	return false;

}

bool Grid::checkGridAdapt(FLOAT &max_vel, FLOAT &min_vel) {
	// TODO FILL THIS WITH USEFUL CRITERION
	// #pragma omp parallel for
	// uint co = 0;
	for (uint i = 0; i < nb_nodes; ++i) {
		if (active_nodes[i]) {
			max_vel = std::max(velocities[i].norm(), max_vel);
			min_vel = std::min(velocities[i].norm(), min_vel);
			// if (min_vel < 1e-4){
			// 	// co++;
			// 	printf("something is wrong\n");
			// }
		}
	}
	// if (co >= 1)
	// {
	// 	printf("something was wrong %d times\n", co);
	// 	exit(1);
	// }
	// printf("GRID max val is %f\n", max_vel);
	// printf("GRID min val is %f\n", min_vel);
	return false;
}


MAT3 Grid::secondDer(uint i, uint j, std::vector<Particule*> & particules) {
	MAT3 second_der = MAT3::Zero();
	Vector3i indi = nodeFromIndex(i);
	Vector3i indj = nodeFromIndex(j);


	// INFO(3, "second der indice \n"<<indi<<"\n\n "<<indj);
	// INFO(3, i_max<<" "<<j_max<<" "<<k_max);
	for (int l = indi(0) - 2; l <= indi(0) + 2; ++l) {
		if (l >= 0 && l < (int)i_max) {
			for (int m = indi(1) - 2; m <= indi(1) + 2; ++m) {
				if (m >= 0 && m < (int)j_max) {
					for (int n = indi(2) - 2; n <= indi(2) + 2; ++n) {
						if (n >= 0 && n < (int)k_max) {
							uint indc = l*j_max*k_max + m*(k_max) + n;
							for (auto& p : cells[indc]) {
								//for (auto& p : particules) {
								//second deriative energy for implicite scheme
								VEC3 wip = p->gradWeight(indi);
								VEC3 wjp = p->gradWeight(indj);
								MAT3 F = p->getDeformationElastic();
								FLOAT det = F.determinant()*p->getVolume();
								//	F = 0.5*(F + F.transpose());
								VEC3 wipF = wip;
								VEC3 wjpF = (wjp.transpose()*F).transpose();
								const Tensor T = p->getSecondEnergyDer();
								for (uint alpha = 0; alpha < 3; ++alpha) {
									for (uint beta = 0; beta < 3; ++beta) {
									    // INFO(3, "wipF\n"<<wipF);
									    // INFO(3, "wjpF\n"<<wjpF);
										FLOAT der_ab = 0;
										for (uint u = 0; u < 3; ++u) {
											for (uint v = 0; v < 3; ++v) {
												der_ab += T(alpha, u, beta, v)*wjpF(v)*wipF(u);
												// 	IS_DEF(T(alpha, u, beta, v));
												// 	IS_DEF(second_der(alpha, beta));
												//TEST(std::fabs(T(alpha, u, beta, v) - T(beta, v, alpha, u)) < 0.0001);
											}
										}
										second_der(alpha, beta) += det*der_ab;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return second_der;
}