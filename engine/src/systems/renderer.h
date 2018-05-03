#ifndef RENDERER_H
#define RENDERER_H

#include "../core/ecs.h"
#include "../components/transform.h"
#include "../gfx/mesher.h"
#include "../gfx/shader.h"
#include "../gfx/framebuffer.h"

NS_BEGIN

struct Drawable3D : public Component {
	Drawable3D() = default;
	Drawable3D(Mesh* mesh, ShaderProgram* shader) : mesh(mesh), shader(shader) {}
	
	Mesh* mesh;
	ShaderProgram* shader;
};

enum class CameraType {
	Orthographic = 0,
	Perspective
};

struct Camera : public Component {
	Camera() = default;
	Camera(float n, float f, float fov, CameraType t = CameraType::Perspective, float oscale = 1.0f)
			: zNear(n), zFar(f), FOV(fov), orthoScale(oscale), type(t)
	{}
	
	float zNear, zFar, FOV, orthoScale;
	CameraType type;
	
	Mat4 getProjection();
};

class RendererSystem : public EntitySystem {
public:
	RendererSystem();
	
	void render(EntityWorld& world);

private:
	Camera *m_activeCamera;
	Transform *m_activeCameraTransform;
	
	// Buffers
	FrameBuffer m_gbuffer, m_lightBuffer;
	
	// Shaders
	ShaderProgram m_gbufferShader;
};

NS_END

#endif /* RENDERER_H */

