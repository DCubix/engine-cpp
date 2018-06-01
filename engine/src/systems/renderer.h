#ifndef RENDERER_H
#define RENDERER_H

#include "../core/ecs.h"
#include "../components/transform.h"
#include "../gfx/mesher.h"
#include "../gfx/shader.h"
#include "../gfx/filter.h"
#include "../gfx/framebuffer.h"
#include "../gfx/material.h"
#include "../components/light.h"

NS_BEGIN

struct Drawable3D : public Component {
	Drawable3D() = default;
	Drawable3D(Mesh mesh, Material material)
		: mesh(mesh), material(material)
	{}

	Mesh mesh;
	Material material;
};

enum class CameraType {
	Orthographic = 0,
	Perspective
};

struct MeshInstance {
	Vector<Mat4> models;
	Mesh mesh;
};

struct RenderMesh {
	Mesh mesh;
	Material material;
	Mat4 modelMatrix;
};

struct Camera : public Component {
	Camera() = default;
	Camera(float n, float f, float fov, CameraType t = CameraType::Perspective, float oscale = 1.0f)
			: zNear(n), zFar(f), FOV(fov), orthoScale(oscale), type(t)
	{}

	float zNear, zFar, FOV, orthoScale;
	CameraType type;

	Mat4 getProjection(u32 width, u32 height);
};

using RenderCondition = Fn<bool(RenderMesh&)>;

class RendererSystem : public EntitySystem {
public:
	RendererSystem();
	RendererSystem(u32 width, u32 height);

	void update(EntityWorld& world, float dt);
	void render(EntityWorld& world, FrameBuffer* target);
	void messageReceived(EntityWorld& world, const Message& msg);

	void resizeBuffers(u32 width, u32 height);

	static const String POST_FX_VS;

	RendererSystem& addPostEffect(Filter effect);
	RendererSystem& removePostEffect(u32 index);

	RendererSystem& setEnvironmentMap(const Texture& tex);

	void clear(i32 mask, float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f);

	FrameBuffer& GBuffer() { return m_gbuffer; }
	FrameBuffer& finalBuffer() { return m_finalBuffer; }

	float time() const { return m_time; }

	void renderScreenQuad();

	u32 renderHeight() const;
	u32 renderWidth() const;

private:
	Camera *m_activeCamera;
	Transform *m_activeCameraTransform;

	// Buffers
	FrameBuffer m_gbuffer, m_finalBuffer, m_pingPongBuffer,
			m_captureBuffer, m_pickingBuffer, m_shadowBuffer,
			m_screenBuffer;

	bool m_IBLGenerated;

	// Shaders
	ShaderProgram m_gbufferShader, m_lightingShader,
					m_finalShader, m_cubeMapShader,
					m_irradianceShader,
					m_preFilterShader,
					m_brdfLUTShader,
					m_pickingShader,
					m_gbufferInstancedShader,
					m_shadowShader,
					m_shadowInstancedShader;

	// EnvMap
	Texture m_envMap, m_irradiance, m_radiance, m_brdf;

	// Misc
	Mesh m_plane, m_cube;
	Sampler m_screenTextureSampler, m_cubeMapSampler,
			m_screenDepthSampler, m_screenMipSampler,
			m_cubeMapSamplerNoMip;

	VertexBuffer m_instanceBuffer;

	// PostFX
	Vector<Filter> m_postEffects;
	float m_time;

	void computeIrradiance();
	void computeRadiance();
	void computeBRDF();
	void computeIBL();

	void pickingPass(EntityWorld& world, const Mat4& projection, const Mat4& view, const Vector<RenderMesh>& renderables);
	void gbufferPass(const Mat4& projection, const Mat4& view, const Vector<RenderMesh>& renderables);
	void lightingPass(EntityWorld& world, const Mat4& projection, const Mat4& view, const Vector<RenderMesh>& renderables);
	void finalPass(const Mat4& projection, const Mat4& view, const Vector<RenderMesh>& renderables, FrameBuffer* target);

	void render(ShaderProgram& shader, const Vector<RenderMesh>& renderables,
				bool textures = true, const RenderCondition& cond = nullptr);
	void renderInstanced(ShaderProgram& shader, const Vector<RenderMesh>& renderables,
						bool textures = true, const RenderCondition& cond = nullptr);

	u32 m_renderWidth, m_renderHeight;
};

NS_END

#endif /* RENDERER_H */

