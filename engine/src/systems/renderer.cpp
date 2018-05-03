#include "renderer.h"

NS_BEGIN

Mat4 Camera::getProjection() {
	if (type == CameraType::Orthographic) {
		return Mat4::ortho(-orthoScale, orthoScale, -orthoScale, orthoScale, zNear, zFar);
	}
	
	int vp[4];
	glGetIntegerv(GL_VIEWPORT, vp);
	
	return Mat4::perspective(radians(FOV), float(vp[2]) / float(vp[3]), zNear, zFar);
}

RendererSystem::RendererSystem() {
	m_activeCamera = nullptr;
	m_activeCameraTransform = nullptr;
	
	int vp[4];
	glGetIntegerv(GL_VIEWPORT, vp);
	
	m_gbuffer = Builder<FrameBuffer>::build()
			.setSize(vp[2], vp[3])
			.addRenderBuffer(TextureFormat::Depth, Attachment::DepthAttachment)
			.addColorAttachment(TextureFormat::RGBf)
			.addColorAttachment(TextureFormat::RGBAf);
	
	String common =
#include "../shaders/common.glsl"
			;
	
	String gVS =
#include "../shaders/gbufferV.glsl"
			;
	String gFS =
#include "../shaders/gbufferF.glsl"
			;
	
	gVS = Util::replace(gVS, "#include common", common);
	gFS = Util::replace(gFS, "#include common", common);
	
	m_gbufferShader = Builder<ShaderProgram>::build()
			.add(gVS, ShaderType::VertexShader)
			.add(gFS, ShaderType::FragmentShader);
	m_gbufferShader.link();
	
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
}

void RendererSystem::render(EntityWorld& world) {
	if (m_activeCamera == nullptr || m_activeCameraTransform == nullptr) {
		Entity* cameraEnt = world.find<Camera>();
		if (cameraEnt) {
			m_activeCamera = cameraEnt->get<Camera>();
			m_activeCameraTransform = cameraEnt->get<Transform>();
		}
	}

	Mat4 projMat = Mat4::ident();
	if (m_activeCamera) {
		projMat = m_activeCamera->getProjection();
	}
	
	Mat4 viewMat = Mat4::ident();
	if (m_activeCameraTransform) {
		Mat4 rot = m_activeCameraTransform->worldRotation().conjugated().toMat4();
		Mat4 loc = Mat4::translation(m_activeCameraTransform->worldPosition() * -1.0f);
		viewMat = rot * loc;
	}
	
	m_gbuffer.bind();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	m_gbufferShader.bind();
	m_gbufferShader.get("mProjection").value().set(projMat);
	m_gbufferShader.get("mView").value().set(viewMat);
	world.each([&](Entity& ent, Transform& T, Drawable3D& D) {
		Mat4 modelMat = T.localToWorldMatrix();
		m_gbufferShader.get("mModel").value().set(modelMat);
		
		D.mesh->bind();
		glDrawElements(GL_TRIANGLES, D.mesh->indexCount(), GL_UNSIGNED_INT, nullptr);
	});
	m_gbufferShader.unbind();
	m_gbuffer.unbind();
}
		
NS_END