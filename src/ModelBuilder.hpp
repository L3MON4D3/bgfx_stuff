#ifndef MODEL_BUILDER_H_
#define MODEL_BUILDER_H_

#include "Util.hpp"

#include "FastNoise.h"
#include "bgfx/bgfx.h"

namespace worldWp {

struct ModelSpecs {
	int x_dim,
	    y_dim,
	    res;
};

class ModelBuilder {
public:
    ModelBuilder(ModelSpecs ms, FastNoise fn, worldWp::util::NoiseMods nm);
    bgfx::VertexBufferHandle getVBufferHandle();
    bgfx::IndexBufferHandle getIBufferHandle();
private:
	ModelSpecs ms;
	worldWp::util::NoiseMods nm;
	worldWp::util::PosNormalColorVertex *plane_verts;
    uint32_t *plane_indz;

	void add_normals();
};

};

#endif
