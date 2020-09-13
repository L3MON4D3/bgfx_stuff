#include "PlaneBuilder.hpp"
#include "Util.hpp"
#include "bx/math.h"

#include <cmath>
#include <iostream>
#include <ostream>

#define VBUF_CNT (ms.x_dim*ms.z_dim*2 + 12*2)
#define IBUF_CNT ((ms.x_dim-1)*(ms.z_dim-1)*12 + frame_indzs_count*2)

const float frame_width = 1;

//eight triangles, both side, 3 verts per triangle.
const int frame_indzs_count = 8*3*2;

namespace worldWp {

//offsets from an xy coordinate to construct a frame.
const float frame_verts[12][2] {
	{0          , 0          },
	{frame_width, 0          },
	{frame_width, frame_width},

	{frame_width, 0          },
	{frame_width, frame_width},
	{0          , frame_width},

	{frame_width, frame_width},
	{0          , frame_width},
	{0          ,           0},

	{0          , frame_width},
	{0          ,           0},
	{frame_width,           0}
};

const uint32_t frame_indzs[frame_indzs_count] {
	 1,  2,  4,
	 1,  4,  2,
	 1,  4,  3,
	 1,  3,  4,

	 4,  5,  7,
	 4,  7,  5,
	 4,  7,  6,
	 4,  6,  7,

	 7,  8, 10,
	 7, 10,  8,
	 7, 10,  9,
	 7,  9, 10,

	10, 11,  1,
	10,  1, 11,
	10,  1,  0,
	10,  0,  1
};

PlaneBuilder::PlaneBuilder(
  const ModelSpecs& ms,
  const FastNoise& fn,
  const worldWp::util::NoiseMods& nm
)
	: ms{ ms },
	  nm{ nm },
	  //double space, store duplicate indizes ith different normals.
	  plane_verts{ new worldWp::util::PosNormalColorVertex[VBUF_CNT] },
	  plane_indz{ new uint32_t[IBUF_CNT] } {

	add_plane_vertices(fn);
	add_frame_vertices();
	add_normals();

	//fill plane_indz.
	int offset{ ms.x_dim*ms.z_dim };
	{
		int plane_x_dim{ ms.x_dim-1 },
		    plane_z_dim{ ms.z_dim-1 };
		for(int i = 0; i != plane_x_dim; ++i)
			for(int j = 0; j != plane_z_dim; ++j) {
				int vert_start_indx {i*ms.x_dim + j};
				
				//init vertices for triangles.
				int v1{ vert_start_indx },
				    v2{ v1+1 },
				    v3{ vert_start_indx+ms.z_dim },
				    v4{ v3+1 };
				
				int tri_start_indx {(i*plane_x_dim + j) * 12};
				//first Triangle of "square".
				plane_indz[tri_start_indx   ] = v3;
				plane_indz[tri_start_indx+ 1] = v2;
				plane_indz[tri_start_indx+ 2] = v1;
				
				plane_indz[tri_start_indx+ 3] = v2;
				plane_indz[tri_start_indx+ 4] = v3;
				plane_indz[tri_start_indx+ 5] = v1;
				
				//second Triangle of "square".
				plane_indz[tri_start_indx+ 6] = v3+offset;
				plane_indz[tri_start_indx+ 7] = v4+offset;
				plane_indz[tri_start_indx+ 8] = v2+offset;
				
				plane_indz[tri_start_indx+ 9] = v4+offset;
				plane_indz[tri_start_indx+10] = v3+offset;
				plane_indz[tri_start_indx+11] = v2+offset;
			}
	}
}

void PlaneBuilder::add_normals() {
	for(int i {0}; i != (ms.x_dim-1)*ms.z_dim; ++i) {
		worldWp::util::add_normal(&plane_verts[i],
			//pass pos of each Vertex
			(float*) &plane_verts[i+1],
			(float*) &plane_verts[i+ms.z_dim]);

		//add normals to "downward-pointing" triangle.
		int i_offset = ms.x_dim*ms.z_dim+i+1;
		worldWp::util::add_normal(&plane_verts[i_offset],
			(float*) &plane_verts[i_offset+ms.z_dim],
			(float*) &plane_verts[i_offset+ms.z_dim-1]);
	}
}

void PlaneBuilder::add_plane_vertices(const FastNoise& fn) {
	//fill plane_verts with values from fn.
	//indx = i*j at any point in loop.
	int indx {0};
	int offset {ms.x_dim*ms.z_dim};
	for(int i {0}; i != ms.x_dim*ms.res; i+=ms.res)
		for(int j {0}; j != ms.z_dim*ms.res; j+=ms.res, ++indx)
			plane_verts[indx+offset] =
			plane_verts[indx       ] = { (float) i-ms.x_dim*ms.res/2,
			                             util::get_noise_mdfd(i, j, fn, nm),
			                             (float) j-ms.z_dim*ms.res/2, 
			                             0, 0, 0,
			                             0xff666666 };
}

void PlaneBuilder::add_frame_vertices_2d(
  Dimension dim,
  bx::Vec3 pos, float dim1_sz, float dim2_sz,
  int start_pos
) {
	int dim0{ static_cast<int>(dim) },
	    dim1{ (dim0+1)%3 },
	    dim2{ (dim1+1)%3 };

	//initialize all verices here, assign correct position later.
	for(int i{0}; i != 12; ++i)
		plane_verts[start_pos+i] = {0,0,0, 0,0,0, 0xffffffff};

	float* pos_f = ((float*)&pos);
	float corners[4][2] {
		{pos_f[dim1]        , pos_f[dim2]        },
		{pos_f[dim1]+dim1_sz, pos_f[dim2]        },
		{pos_f[dim1]+dim1_sz, pos_f[dim2]+dim2_sz},
		{pos_f[dim1]        , pos_f[dim2]+dim2_sz}
	};

	int indx{0};
	for(int i{0}; i != 4; ++i) {
		for(int j{0}; j != 3; ++j, ++indx) {
			util::PosNormalColorVertex& v{ plane_verts[start_pos+indx] };
			v.pos[dim0] = pos_f[dim0];
			v.pos[dim1] = corners[i][0]+frame_verts[indx][0];
			v.pos[dim2] = corners[i][1]+frame_verts[indx][1];
		}
	}
}

void PlaneBuilder::add_frame_indzs(int start_indx, int vertex_offset) {
	for(int i{0}; i != frame_indzs_count; ++i)
		plane_indz[start_indx+i] = frame_indzs[i]+vertex_offset;
}

void PlaneBuilder::add_frame_vertices() {
	int vert_offset{ms.x_dim*ms.z_dim*2},
	    indx_offset{(ms.x_dim-1)*(ms.z_dim-1)*12};

	add_frame_vertices_2d(Dimension::Z, {0,0,0}, 10, 10, vert_offset);
	add_frame_indzs(indx_offset, vert_offset);
	vert_offset += 12, indx_offset += frame_indzs_count;

	add_frame_vertices_2d(Dimension::Y, {0,0,0}, 10, 10, vert_offset);
	add_frame_indzs(indx_offset, vert_offset);
}

float* PlaneBuilder::get_raw_noise(const FastNoise& fn) {
	float* ns {new float[ms.x_dim*ms.z_dim]};

	int indx {0};
	for(int i {0}; i != ms.x_dim*ms.res; i+=ms.res)
		for(int j {0}; j != ms.z_dim*ms.res; j+=ms.res, ++indx)
			ns[indx] = util::get_noise_mdfd(i, j, fn, nm);
	return ns;
}

bgfx::IndexBufferHandle PlaneBuilder::getIBufferHandle() {
	std::cout << plane_indz[9] << std::endl;
	return bgfx::createIndexBuffer(bgfx::makeRef(plane_indz,
		IBUF_CNT*sizeof(uint32_t)),
		BGFX_BUFFER_INDEX32);
}

bgfx::VertexBufferHandle PlaneBuilder::getVBufferHandle() {
	std::cout << plane_verts[9].pos[0] << std::endl;
	return bgfx::createVertexBuffer(
		bgfx::makeRef(plane_verts,
			VBUF_CNT*sizeof(util::PosNormalColorVertex)),
			util::PosNormalColorVertex::layout);
}

void PlaneBuilder::for_each_vertex(
  const std::function<void(util::PosNormalColorVertex&, int indx)>& fn
) {
	for(int i{0}; i != ms.x_dim*ms.z_dim; ++i) {
		//apply function to both vertices (each vertex exists twice for normals).
		fn(plane_verts[i], i);
		fn(plane_verts[i+ms.x_dim*ms.z_dim], i);
	}
}

};
