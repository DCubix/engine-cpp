#include "imm.h"
#include "../math/consts.h"

#include "../core/logging/log.h"

#include <iterator>

NS_BEGIN

Vector<ImmDrawable> Imm::g_drawables;
Vector<ImmBatch> Imm::g_batches;
Vector<ImmVertex> Imm::g_vertices;
Vector<u32> Imm::g_indices;
PrimitiveType Imm::g_beginPrimitive = PrimitiveType::Triangles;
float Imm::g_lineWidth = 1.0f;
Texture Imm::g_texture;

VertexArray Imm::g_vao;
VertexBuffer Imm::g_vbo;
VertexBuffer Imm::g_ibo;
ShaderProgram Imm::g_shader;

Mat4 Imm::g_modelMatrix = Mat4(1.0f);
Mat4 Imm::g_viewMatrix = Mat4(1.0f);
bool Imm::g_noDepth = false;
bool Imm::g_wire = false;
bool Imm::g_cullFace = true;

static const String ImmVS = R"(#version 330 core
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec2 vUV;
layout (location = 2) in vec4 vColor;

uniform mat4 mProjection;
uniform mat4 mView;

out vec4 oColor;
out vec2 oUV;

void main() {
	gl_Position = mProjection * mView * vec4(vPosition, 1.0);
	oColor = vColor;
	oUV = vUV;
}
)";

static const String ImmFS = R"(#version 330 core
out vec4 fragColor;

in vec4 oColor;
in vec2 oUV;

uniform sampler2D tTex;
uniform bool tEnable = false;

void main() {
	vec4 col = oColor;
	if (tEnable) {
		col *= texture(tTex, oUV);
	}
	fragColor = col;
}
)";

void Imm::initialize() {
	g_vao = Builder<VertexArray>::build();
	g_vbo = Builder<VertexBuffer>::build();
	g_ibo = Builder<VertexBuffer>::build();

	g_vao.bind();
	g_vbo.bind(BufferType::ArrayBuffer);

	g_vbo.addVertexAttrib(0, 3, DataType::Float, false, sizeof(ImmVertex),  0);
	g_vbo.addVertexAttrib(1, 2, DataType::Float, false, sizeof(ImmVertex), 12);
	g_vbo.addVertexAttrib(2, 4, DataType::Float, false, sizeof(ImmVertex), 20);

	g_ibo.bind(BufferType::IndexBuffer);

	g_vao.unbind();

	g_shader = Builder<ShaderProgram>::build();
	g_shader.add(ImmVS, ShaderType::VertexShader);
	g_shader.add(ImmFS, ShaderType::FragmentShader);
	g_shader.link();
}

void Imm::render(const Mat4& view, const Mat4& projection) {
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	g_viewMatrix = view;

	generateBatches();

	g_vao.bind();

	g_shader.bind();
	g_shader.get("mProjection").set(projection);
	g_shader.get("mView").set(view);

	bool cullFaceEnabled = glIsEnabled(GL_CULL_FACE);

	for (ImmBatch b : g_batches) {
		if (b.noDepth) glDisable(GL_DEPTH_TEST);
		if (b.wire) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		if (!b.cullFace && cullFaceEnabled) glDisable(GL_CULL_FACE);
		else if (b.cullFace && !cullFaceEnabled) glEnable(GL_CULL_FACE);

		if (b.texture.id() != 0) {
			b.texture.bind(Texture::DEFAULT_SAMPLER, 0);
			g_shader.get("tTex").set(0);
			g_shader.get("tEnable").set(true);
		}

		glLineWidth(b.lineWidth <= 0.0f ? 1.0f : b.lineWidth);
		glDrawElements(b.primitiveType, b.indexCount, GL_UNSIGNED_INT, (void*)(4 * b.offset));

		if (!b.cullFace && cullFaceEnabled) glEnable(GL_CULL_FACE);
		else if (b.cullFace && !cullFaceEnabled) glDisable(GL_CULL_FACE);

		if (b.noDepth) glEnable(GL_DEPTH_TEST);
		if (b.wire) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		if (b.texture.id() != 0) {
			g_shader.get("tEnable").set(false);
			b.texture.unbind();
		}
	}

	g_shader.unbind();

	g_vao.unbind();

	g_drawables.clear();
	g_batches.clear();
}

void Imm::lineWidth(float value) {
	g_lineWidth = value;
}

void Imm::wire(bool enable) {
	g_wire = enable;
}

void Imm::texture(const Texture& value) {
	g_texture = value;
}

void Imm::cullFace(bool enable) {
	g_cullFace = enable;
}

void Imm::setModel(const Mat4& m) {
	g_modelMatrix = m;
}

void Imm::disableDepth() {
	g_noDepth = true;
}

void Imm::begin(PrimitiveType primitive) {
	assert(g_vertices.empty());
	g_beginPrimitive = primitive;
}

void Imm::end() {
	ImmDrawable dw;
	for (ImmVertex v : g_vertices) {
		ImmVertex nv;
		nv.color = v.color;
		nv.position = Vec3(g_modelMatrix * Vec4(v.position, 1.0f));
		nv.uv = v.uv;
		dw.vertices.push_back(nv);
	}
	dw.indices.insert(dw.indices.end(), g_indices.begin(), g_indices.end());
	dw.primitiveType = g_beginPrimitive;
	dw.noDepth = g_noDepth;
	dw.lineWidth = g_lineWidth;
	dw.wire = g_wire;
	dw.texture = g_texture;
	dw.cullFace = g_cullFace;

	g_vertices.clear();
	g_indices.clear();

	g_drawables.push_back(dw);

	g_modelMatrix = Mat4(1.0f);
	g_noDepth = false;
	g_wire = false;
	g_cullFace = true;
	g_lineWidth = 1.0f;
	g_texture.invalidate();
}

void Imm::vertex(const Vec3 &pos, const Vec4 &col, bool index, const Vec2& uv) {
	ImmVertex vert;
	vert.position = pos;
	vert.color = col;
	vert.uv = uv;

	if (index)
		g_indices.push_back(g_vertices.size());
	g_vertices.push_back(vert);
}

void Imm::vertex(const Vec3& pos, bool index, const Vec2& uv) {
	vertex(pos, Vec4(1.0f), index);
}

void Imm::addIndex(u32 index) {
	g_indices.push_back(g_vertices.size() + index);
}

void Imm::addIndices(const Vector<u32>& indices) {
	for (u32 i : indices) addIndex(i);
}

void Imm::line(const Vec3& a, const Vec3& b, const Vec4 color) {
	vertex(a, color);
	vertex(b, color);
}

void Imm::cube(const Vec3& halfExtents, const Vec4 color, const Vec3& origin) {
	const u32 indices[] = {
		// front
		0, 1, 2,
		2, 3, 0,
		// right
		1, 5, 6,
		6, 2, 1,
		// back
		7, 6, 5,
		5, 4, 7,
		// left
		4, 0, 3,
		3, 7, 4,
		// bottom
		4, 5, 1,
		1, 0, 4,
		// top
		3, 2, 6,
		6, 7, 3
	};

	Vector<u32> indices_v(std::begin(indices), std::end(indices));
	addIndices(indices_v);

	float x = halfExtents.x;
	float y = halfExtents.y;
	float z = halfExtents.z;

	vertex(origin + Vec3(-x, -y, z), color, false);
	vertex(origin + Vec3(x, -y, z), color, false);
	vertex(origin + Vec3(x, y, z), color, false);
	vertex(origin + Vec3(-x, y, z), color, false);
	vertex(origin + Vec3(-x, -y, -z), color, false);
	vertex(origin + Vec3(x, -y, -z), color, false);
	vertex(origin + Vec3(x, y, -z), color, false);
	vertex(origin + Vec3(-x, y, -z), color, false);
}

void Imm::sphere(float radius, const Vec4 &color, u32 stacks, u32 slices) {
	// Calc The Index Positions
	for (u32 i = 0; i < stacks; ++i) {
		for (u32 j = 0; j < slices; ++j) {
			addIndex(i * slices + j);
			addIndex(i * slices + (j+1));
			addIndex((i+1) * slices + (j+1));
			addIndex((i+1) * slices + (j+1));
			addIndex((i+1) * slices + j);
			addIndex(i * slices + j);
		}
	}

	// Calc The Vertices
	for (u32 i = 0; i <= stacks; ++i) {
		float V   = float(i) / float(stacks);
		float phi = V * Pi;

		// Loop Through Slices
		for (u32 j = 0; j <= slices; ++j) {
			float U = float(j) / float(slices);
			float theta = U * (Pi * 2.0f);

			// Calc The Vertex Positions
			float x = std::cos(theta) * std::sin(phi);
			float y = std::cos(phi);
			float z = std::sin(theta) * std::sin(phi);

			// Push Back Vertex Data
			vertex(Vec3(x, y, z) * radius, color, false);
		}
	}
}

void Imm::cone(float base, float height, const Vec4& color, const Vec3& origin, u32 slices, bool invert) {
	u32 i = 0;
	for (i = 1; i < slices; i++) {
		addIndex(0); addIndex(i); addIndex(i+1);
	}
	addIndex(0); addIndex(i); addIndex(1);
	for (i = 1; i < slices; i++) {
		addIndex(i+1); addIndex(i); addIndex(slices);
	}
	addIndex(slices); addIndex(i); addIndex(1);

	float sign = invert ? -1 : 1;
	vertex(origin + Vec3(0, 0, -height * sign), color, false);

	for (u32 i = 0; i < slices; i++) {
		float V   = float(i) / float(slices);
		float phi = V * TwoPi;
		float x = std::cos(phi);
		float z = std::sin(phi);
		Vec3 ax = Vec3(x, z, 0);
		vertex(origin + ax * base, color, false);
	}

	vertex(origin + Vec3(0, 0, 0), color, false);
}

void Imm::arrow(float len, const Vec4& color, float thickness) {
	cube(Vec3(thickness*0.4f, thickness*0.4f, len), color, Vec3(0, 0, -len));
	cone(thickness, 0.4f, color, Vec3(0, 0, -len*2), 12);
}

void Imm::axes(float len) {
	addIndex(0); addIndex(1);
	addIndex(2); addIndex(3);
	addIndex(4); addIndex(5);
	vertex(Vec3(0.0f), Vec4(1.0f, 0.0f, 0.0f, 1.0f));
	vertex(Vec3(1.0f, 0.0f, 0.0f), Vec4(1.0f, 0.0f, 0.0f, 1.0f));

	vertex(Vec3(0.0f), Vec4(0.0f, 1.0f, 0.0f, 1.0f));
	vertex(Vec3(0.0f, 1.0f, 0.0f), Vec4(0.0f, 1.0f, 0.0f, 1.0f));

	vertex(Vec3(0.0f), Vec4(0.0f, 0.0f, 1.0f, 1.0f));
	vertex(Vec3(0.0f, 0.0f, 1.0f), Vec4(0.0f, 0.0f, 1.0f, 1.0f));
}

static Vec3 btransform(const Mat4& view, const Vec3& pos, const Vec3& sqr, float sz) {
	Mat4 mat = glm::inverse(view);
	float scale = sz * glm::length(pos - Vec3(view[3])) / 2.0f;

	Vec4 wpos = mat * Vec4(sqr, 0.0f) * scale;
	return Vec3(wpos) + pos;
}

void Imm::billboard(const Vec3& pos, float size, const Vec4& color) {
	addIndex(0); addIndex(1); addIndex(2);
	addIndex(2); addIndex(3); addIndex(0);

	vertex(btransform(g_viewMatrix, pos, Vec3(-size, -size, 0.0f), size), color, false, Vec2(0.0f, 1.0f));
	vertex(btransform(g_viewMatrix, pos, Vec3( size, -size, 0.0f), size), color, false, Vec2(1.0f, 1.0f));
	vertex(btransform(g_viewMatrix, pos, Vec3( size,  size, 0.0f), size), color, false, Vec2(1.0f, 0.0f));
	vertex(btransform(g_viewMatrix, pos, Vec3(-size,  size, 0.0f), size), color, false, Vec2(0.0f, 0.0f));
}

void Imm::billboardAtlas(const Vec3& pos, u32 wcount, u32 hcount, u32 index, float size, const Vec4& color) {
	addIndex(0); addIndex(1); addIndex(2);
	addIndex(2); addIndex(3); addIndex(0);

	float uw = 1.0f / wcount;
	float uh = 1.0f / hcount;
	float ux = float(index % wcount) * uw;
	float uy = float(std::floor(index / wcount)) * uh;

	vertex(btransform(g_viewMatrix, pos, Vec3(-size, -size, 0.0f), size), color, false, Vec2(ux, uy+uh));
	vertex(btransform(g_viewMatrix, pos, Vec3( size, -size, 0.0f), size), color, false, Vec2(ux+uw, uy+uh));
	vertex(btransform(g_viewMatrix, pos, Vec3( size,  size, 0.0f), size), color, false, Vec2(ux+uw, uy));
	vertex(btransform(g_viewMatrix, pos, Vec3(-size,  size, 0.0f), size), color, false, Vec2(ux, uy));
}

void Imm::generateBatches() {
	if (g_drawables.empty()) return;

	Vector<ImmVertex> vertices;
	Vector<u32> indices;

	ImmDrawable first = g_drawables[0];
	vertices.insert(vertices.end(), first.vertices.begin(), first.vertices.end());
	indices.insert(indices.end(), first.indices.begin(), first.indices.end());

	ImmBatch fb;
	fb.primitiveType = first.primitiveType;
	fb.noDepth = first.noDepth;
	fb.indexCount = first.indices.size();
	fb.offset = 0;
	g_batches.push_back(fb);

	u32 ioff = first.vertices.size();
	u32 off = 0;
	for (u32 i = 1; i < g_drawables.size(); i++) {
		ImmDrawable curr = g_drawables[i];
		ImmDrawable prev = g_drawables[i - 1];
		if (curr.primitiveType != prev.primitiveType ||
			curr.noDepth != prev.noDepth ||
			curr.cullFace != prev.cullFace ||
			curr.lineWidth != prev.lineWidth ||
			curr.wire != prev.wire ||
			curr.texture.id() != prev.texture.id()) {
			off += g_batches.back().indexCount;
			ImmBatch b;
			b.primitiveType = curr.primitiveType;
			b.noDepth = curr.noDepth;
			b.cullFace = curr.cullFace;
			b.indexCount = curr.indices.size();
			b.offset = off;
			b.lineWidth = curr.lineWidth;
			b.wire = curr.wire;
			b.texture = curr.texture;
			g_batches.push_back(b);
		} else {
			g_batches.back().indexCount += curr.indices.size();
		}

		vertices.insert(vertices.end(), curr.vertices.begin(), curr.vertices.end());
		for (u32 i : curr.indices) indices.push_back(i + ioff);

		ioff += curr.vertices.size();
	}

	g_vbo.bind();
	g_vbo.setData<ImmVertex>(vertices.size(), vertices.data(), BufferUsage::Dynamic);
	g_vbo.unbind();

	g_ibo.bind();
	g_ibo.setData<u32>(indices.size(), indices.data(), BufferUsage::Dynamic);
	g_ibo.unbind();
}

NS_END
