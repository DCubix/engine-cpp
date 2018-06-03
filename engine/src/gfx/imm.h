#ifndef IMM_H
#define IMM_H

#include "../core/types.h"
#include "../math/vec.h"
#include "../math/mat.h"
#include "api.h"
#include "shader.h"
#include "texture.h"

#include <climits>

using namespace api;

NS_BEGIN

struct ImmVertex {
	Vec3 position;
	Vec2 uv;
	Vec4 color;
};

struct ImmDrawable {
	Vector<ImmVertex> vertices;
	Vector<u32> indices;
	PrimitiveType primitiveType;
	float lineWidth;
	bool noDepth, wire, cullFace;
	Texture texture;
};

struct ImmBatch {
	u32 offset, indexCount;
	PrimitiveType primitiveType;
	float lineWidth;
	bool noDepth, wire, cullFace;
	Texture texture;
};

class Imm {
public:
	static void begin(PrimitiveType primitive = PrimitiveType::Triangles);
	static void end();

	static void vertex(const Vec3& pos, bool index = true, const Vec2& uv = Vec2(0.0f));
	static void vertex(const Vec3& pos, const Vec4& col, bool index = true, const Vec2& uv = Vec2(0.0f));
	static void addIndex(u32 index);
	static void addIndices(const Vector<u32>& indices);

	static void line(const Vec3& a, const Vec3& b, const Vec4 color = Vec4(1.0f));
	static void cube(const Vec3& halfExtents, const Vec4 color = Vec4(1.0f), const Vec3& origin = Vec3(0.0f));
	static void sphere(float radius, const Vec4& color = Vec4(1.0f), u32 stacks = 16, u32 slices = 8);
	static void cone(float base, float height, const Vec4& color = Vec4(1.0f), const Vec3& origin = Vec3(0.0f), u32 slices = 4, bool invert = false);
	static void arrow(float len, const Vec4& color = Vec4(1.0f), float thickness = 1.0f);
	static void axes(float len);

	static void billboard(const Vec3& pos, float size = 1.0f, const Vec4& color = Vec4(1.0f));
	static void billboardAtlas(const Vec3& pos, u32 wcount, u32 hcount, u32 index, float size = 1.0f, const Vec4& color = Vec4(1.0f));

	static void setModel(const Mat4& m);
	static void disableDepth();

	static void initialize();

	static void render(const Mat4& view, const Mat4& projection);

	static void lineWidth(float value);
	static void wire(bool enable);
	static void texture(const Texture& value);
	static void cullFace(bool enable);

private:
	static VertexArray g_vao;
	static VertexBuffer g_vbo, g_ibo;
	static ShaderProgram g_shader;

	static Vector<ImmDrawable> g_drawables;
	static Vector<ImmBatch> g_batches;
	static PrimitiveType g_beginPrimitive;
	static float g_lineWidth;

	static Vector<ImmVertex> g_vertices;
	static Vector<u32> g_indices;

	static Mat4 g_modelMatrix, g_viewMatrix;
	static bool g_noDepth, g_wire, g_cullFace;
	static Texture g_texture;

	static void generateBatches();
};

NS_END

#endif // IMM_H
