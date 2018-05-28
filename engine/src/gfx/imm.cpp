#include "imm.h"
#include "../math/consts.h"

NS_BEGIN

Vector<ImmDrawable> Imm::g_drawables;
Vector<ImmBatch> Imm::g_batches;
Vector<ImmVertex> Imm::g_vertices;
Vector<u32> Imm::g_indices;
PrimitiveType Imm::g_beginPrimitive = PrimitiveType::Triangles;

VertexArray Imm::g_vao;
VertexBuffer Imm::g_vbo;
VertexBuffer Imm::g_ibo;
ShaderProgram Imm::g_shader;

Mat4 Imm::g_modelMatrix = Mat4(1.0f);
bool Imm::g_noDepth = false;

static const String ImmVS = R"(#version 330 core
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec4 vColor;

uniform mat4 mProjection;
uniform mat4 mView;

out vec4 oColor;

void main() {
	gl_Position = mProjection * mView * vec4(vPosition, 1.0);
	oColor = vColor;
}
)";

static const String ImmFS = R"(#version 330 core
out vec4 fragColor;

in vec4 oColor;

void main() {
	fragColor = oColor;
}
)";

void Imm::initialize() {
	g_vao = Builder<VertexArray>::build();
	g_vbo = Builder<VertexBuffer>::build();
	g_ibo = Builder<VertexBuffer>::build();

	g_vao.bind();
	g_vbo.bind(BufferType::ArrayBuffer);

	g_vbo.addVertexAttrib(0, 3, DataType::Float, false, sizeof(ImmVertex),  0);
	g_vbo.addVertexAttrib(1, 4, DataType::Float,  true, sizeof(ImmVertex), 12);

	g_ibo.bind(BufferType::IndexBuffer);

	g_vao.unbind();

	g_shader = Builder<ShaderProgram>::build();
	g_shader.add(ImmVS, ShaderType::VertexShader);
	g_shader.add(ImmFS, ShaderType::FragmentShader);
	g_shader.link();

	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(UINT_MAX);
}

void Imm::render(const Mat4& view, const Mat4& projection) {
	generateBatches();

	g_vao.bind();

	g_shader.bind();
	g_shader.get("mProjection").set(projection);
	g_shader.get("mView").set(view);

	for (ImmBatch b : g_batches) {
		if (b.noDepth) glDisable(GL_DEPTH_TEST);
		glDrawElements(b.primitiveType, b.indexCount, GL_UNSIGNED_INT, (void*)(4 * b.offset));
		if (b.noDepth) glEnable(GL_DEPTH_TEST);
	}

	g_shader.unbind();

	g_vao.unbind();

	g_drawables.clear();
	g_batches.clear();
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
		dw.vertices.push_back(nv);
	}
	dw.indices.insert(dw.indices.end(), g_indices.begin(), g_indices.end());
	dw.primitiveType = g_beginPrimitive;
	dw.noDepth = g_noDepth;

	g_vertices.clear();
	g_indices.clear();

	g_drawables.push_back(dw);

	g_modelMatrix = Mat4(1.0f);
	g_noDepth = false;
}

void Imm::vertex(const Vec3 &pos, const Vec4 &col, bool index) {
	ImmVertex vert;
	vert.position = pos;
	vert.color = col;
	g_vertices.push_back(vert);
	if (index) g_indices.push_back(g_indices.size());
}

void Imm::vertex(const Vec3& pos, bool index) {
	vertex(pos, Vec4(1.0f), index);
}

void Imm::reuse(u32 index) {
	g_indices.push_back(index);
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
			curr.noDepth != prev.noDepth) {
			off += g_batches.back().indexCount;
			ImmBatch b;
			b.primitiveType = curr.primitiveType;
			b.noDepth = curr.noDepth;
			b.indexCount = curr.indices.size();
			b.offset = off;
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

void Imm::sphere(const Vec3 &pos, float radius, const Vec4 &color) {
	const i32 lats = 16;
	const i32 longs = 8;

	u32 indicator = 0;

	for(i32 i = 0; i <= lats; i++) {
		float lat0 = Pi * (-0.5f + float(i - 1) / lats);
		float z0 = sinf(lat0);
		float zr0 = cosf(lat0);

		float lat1 = Pi * (-0.5 + float(i) / lats);
		float z1 = sinf(lat1);
		float zr1 = cosf(lat1);

		for(i32 j = 0; j <= longs; j++) {
			float lng = 2.0f * Pi * float(j - 1) / longs;
			float x = cosf(lng);
			float y = sinf(lng);

			vertex(Vec3(x * zr0, y * zr0, z0), false);
			reuse(indicator);
			indicator++;

			vertex(Vec3(x * zr1, y * zr1, z1), false);
			reuse(indicator);
			indicator++;
		}
		reuse(UINT_MAX);
	}
}

NS_END
