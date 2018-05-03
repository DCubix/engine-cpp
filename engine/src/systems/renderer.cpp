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
			.addColorAttachment(TextureFormat::RGB)
			.addColorAttachment(TextureFormat::RGBAf);
	
	m_lightBuffer = Builder<FrameBuffer>::build()
			.setSize(vp[2], vp[3])
			.addColorAttachment(TextureFormat::RGB) // Diffuse BRDF
			.addColorAttachment(TextureFormat::RGBf); // Specular BRDF
	
	String common =
#include "../shaders/common.glsl"
			;
	String brdf =
#include "../shaders/brdf.glsl"
			;
	
	String gVS =
#include "../shaders/gbufferV.glsl"
			;
	String gFS =
#include "../shaders/gbufferF.glsl"
			;
	
	gVS = Util::replace(gVS, "#include common", common);
	gFS = Util::replace(gFS, "#include common", common);
	gFS = Util::replace(gFS, "#include brdf", brdf);
	
	m_gbufferShader = Builder<ShaderProgram>::build()
			.add(gVS, ShaderType::VertexShader)
			.add(gFS, ShaderType::FragmentShader);
	m_gbufferShader.link();
	
	String lVS =
#include "../shaders/lightingV.glsl"
			;
	String lFS =
#include "../shaders/lightingF.glsl"
			;
	
	lVS = Util::replace(lVS, "#include common", common);
	lFS = Util::replace(lFS, "#include common", common);
	lFS = Util::replace(lFS, "#include brdf", brdf);
	
	m_lightBufferShader = Builder<ShaderProgram>::build()
			.add(lVS, ShaderType::VertexShader)
			.add(lFS, ShaderType::FragmentShader);
	m_lightBufferShader.link();
	
	String fVS =
#include "../shaders/gbufferV.glsl"
			;
	String fFS =
#include "../shaders/finalF.glsl"
			;
	
	fVS = Util::replace(fVS, "#include common", common);
	fFS = Util::replace(fFS, "#include common", common);
	fFS = Util::replace(fFS, "#include brdf", brdf);
	
	m_finalShader = Builder<ShaderProgram>::build()
			.add(fVS, ShaderType::VertexShader)
			.add(fFS, ShaderType::FragmentShader);
	m_finalShader.link();
	
	m_plane = Builder<Mesh>::build();
	m_plane.addVertex(Vertex(Vec3(0, 0, 0)))
		.addVertex(Vertex(Vec3(1, 0, 0)))
		.addVertex(Vertex(Vec3(1, 1, 0)))
		.addVertex(Vertex(Vec3(0, 1, 0)))
		.addTriangle(0, 1, 2)
		.addTriangle(2, 3, 0)
		.flush();
	
	m_screenTextureSampler = Builder<Sampler>::build()
			.setFilter(TextureFilter::Linear, TextureFilter::Linear)
			.setWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);
	
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
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
	
	glEnable(GL_DEPTH_TEST);
	
	/// Fill GBuffer
	m_gbuffer.bind();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	m_gbufferShader.bind();
	m_gbufferShader.get("mProjection").set(projMat);
	m_gbufferShader.get("mView").set(viewMat);
	
	world.each([&](Entity& ent, Transform& T, Drawable3D& D) {
		Mat4 modelMat = T.localToWorldMatrix();
		m_gbufferShader.get("mModel").set(modelMat);
		
		m_gbufferShader.get("material.roughness").set(D.material.roughness);
		m_gbufferShader.get("material.metallic").set(D.material.metallic);
		m_gbufferShader.get("material.emission").set(D.material.emission);
		
		int sloti = 0;
		for (TextureSlot slot : D.material.textures) {
			if (!slot.enabled) continue;
			
			String tname = "";
			switch (slot.type) {
				case TextureSlotType::NormalMap: tname = "tNormalMap"; break;
				case TextureSlotType::RougnessMetallicEmission: tname = "tMaterialMap"; break;
				default: break;
			}
			
			if (!tname.empty()) {
				slot.texture.bind(slot.sampler, sloti);
				m_gbufferShader.get(tname + String(".img")).set(sloti);
				m_gbufferShader.get(tname + String(".opt.enabled")).set(true);
				m_gbufferShader.get(tname + String(".opt.uv_transform")).set(slot.uvTransform);
				sloti++;
			}
		}
		
		D.mesh.bind();
		glDrawElements(GL_TRIANGLES, D.mesh.indexCount(), GL_UNSIGNED_INT, nullptr);
	});
	
	m_gbufferShader.unbind();
	m_gbuffer.unbind();
	
	glDisable(GL_DEPTH_TEST);
	
	/// Fill Light Buffer
	m_lightBuffer.bind();
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	
	m_lightBufferShader.bind();
	m_lightBufferShader.get("mProjection").set(projMat);
	m_lightBufferShader.get("mView").set(viewMat);
	
	m_gbuffer.getColorAttachment(0).bind(m_screenTextureSampler, 0); // Material
	m_gbuffer.getColorAttachment(1).bind(m_screenTextureSampler, 1); // Normal+Depth
	
	m_lightBufferShader.get("tMaterial").set(0);
	m_lightBufferShader.get("tNormalDepth").set(1);
	
	if (m_activeCameraTransform) {
		m_lightBufferShader.get("uEye").set(m_activeCameraTransform->worldPosition());
	}
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	
	m_plane.bind();
	
	// Directional Lights
	world.each([&](Entity& ent, Transform& T, DirectionalLight& L) {
		m_lightBufferShader.get("uLight.type").set(L.getType());
		m_lightBufferShader.get("uLight.color").set(L.color);
		m_lightBufferShader.get("uLight.intensity").set(L.intensity);
		
		m_lightBufferShader.get("uLight.direction").set(T.worldRotation().forward());
		
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
	});
	
	// Point Lights
	world.each([&](Entity& ent, Transform& T, PointLight& L) {
		m_lightBufferShader.get("uLight.type").set(L.getType());
		m_lightBufferShader.get("uLight.color").set(L.color);
		m_lightBufferShader.get("uLight.intensity").set(L.intensity);
		
		m_lightBufferShader.get("uLight.position").set(T.worldPosition());
		m_lightBufferShader.get("uLight.radius").set(L.radius);
		m_lightBufferShader.get("uLight.lightCutoff").set(L.lightCutOff);
		
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
	});
	
	// Spot Lights
	world.each([&](Entity& ent, Transform& T, SpotLight& L) {
		m_lightBufferShader.get("uLight.type").set(L.getType());
		m_lightBufferShader.get("uLight.color").set(L.color);
		m_lightBufferShader.get("uLight.intensity").set(L.intensity);
		
		m_lightBufferShader.get("uLight.position").set(T.worldPosition());
		m_lightBufferShader.get("uLight.direction").set(T.worldRotation().forward());
		m_lightBufferShader.get("uLight.radius").set(L.radius);
		m_lightBufferShader.get("uLight.lightCutoff").set(L.lightCutOff);
		m_lightBufferShader.get("uLight.spotCutoff").set(L.spotCutOff);
		
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
	});
	
	m_plane.unbind();
	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	m_lightBufferShader.unbind();
	m_lightBuffer.unbind();
	
	glEnable(GL_DEPTH_TEST);
	
	/// Final render
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	m_finalShader.bind();
	m_finalShader.get("mProjection").set(projMat);
	m_finalShader.get("mView").set(viewMat);
	
	m_lightBuffer.getColorAttachment(0).bind(m_screenTextureSampler, 0);
	m_lightBuffer.getColorAttachment(1).bind(m_screenTextureSampler, 1);
	m_gbuffer.getColorAttachment(0).bind(m_screenTextureSampler, 2);
	
	m_finalShader.get("tLightingDiffuse").set(0);
	m_finalShader.get("tLightingSpecular").set(1);
	m_finalShader.get("tMaterial").set(2);
	
	m_finalShader.get("uResolution").set(Vec2(m_lightBuffer.width(), m_lightBuffer.height()));
		
	world.each([&](Entity& ent, Transform& T, Drawable3D& D) {
		Mat4 modelMat = T.localToWorldMatrix();
		m_finalShader.get("mModel").set(modelMat);
		
		m_finalShader.get("uAlbedoColor").set(D.material.albedo);
		
		int sloti = 3;
		for (TextureSlot slot : D.material.textures) {
			if (!slot.enabled) continue;
			
			String tname = "";
			switch (slot.type) {
				case TextureSlotType::Albedo0: tname = "tAlbedo0"; break;
				case TextureSlotType::Albedo1: tname = "tAlbedo1"; break;
				default: break;
			}
			
			if (!tname.empty()) {
				slot.texture.bind(slot.sampler, sloti);
				m_finalShader.get(tname + String(".img")).set(sloti);
				m_finalShader.get(tname + String(".opt.enabled")).set(true);
				m_finalShader.get(tname + String(".opt.uv_transform")).set(slot.uvTransform);
				sloti++;
			}
		}
		
		D.mesh.bind();
		glDrawElements(GL_TRIANGLES, D.mesh.indexCount(), GL_UNSIGNED_INT, nullptr);
	});
	
	m_finalShader.unbind();
}
		
NS_END