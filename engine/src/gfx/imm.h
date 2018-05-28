#ifndef IMM_H
#define IMM_H

#include "../core/types.h"
#include "../math/vec.h"
#include "../math/mat.h"
#include "api.h"
#include "shader.h"

#include <climits>

using namespace api;

NS_BEGIN

struct ImmVertex {
	Vec3 position;
	Vec4 color;
};

struct ImmDrawable {
	Vector<ImmVertex> vertices;
	Vector<u32> indices;
	PrimitiveType primitiveType;
	bool noDepth;
};

struct ImmBatch {
	u32 offset, indexCount;
	PrimitiveType primitiveType;
	bool noDepth;
};

class Imm {
public:
	static void begin(PrimitiveType primitive = PrimitiveType::Triangles);
	static void end();

	static void vertex(const Vec3& pos, bool index = true);
	static void vertex(const Vec3& pos, const Vec4& col, bool index = true);
	static void reuse(u32 index);

	static void sphere(const Vec3& pos, float radius, const Vec4& color = Vec4(1.0f));

	static void setModel(const Mat4& m);
	static void disableDepth();

	static void initialize();

	static void render(const Mat4& view, const Mat4& projection);

private:
	static VertexArray g_vao;
	static VertexBuffer g_vbo, g_ibo;
	static ShaderProgram g_shader;

	static Vector<ImmDrawable> g_drawables;
	static Vector<ImmBatch> g_batches;
	static PrimitiveType g_beginPrimitive;

	static Vector<ImmVertex> g_vertices;
	static Vector<u32> g_indices;

	static Mat4 g_modelMatrix;
	static bool g_noDepth;

	static void generateBatches();
};

NS_END

#endif // IMM_H