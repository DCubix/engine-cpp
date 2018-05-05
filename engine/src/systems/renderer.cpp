#include "renderer.h"

NS_BEGIN

Mat4 Camera::getProjection() {
	if (type == CameraType::Orthographic) {
		return Mat4::ortho(-orthoScale, orthoScale, -orthoScale, orthoScale, zNear, zFar);
	}
	
	int vp[4];
	glGetIntegerv(GL_VIEWPORT, vp);
	
	return Mat4::perspective(FOV, float(vp[2]) / float(vp[3]), zNear, zFar);
}

const String RendererSystem::POST_FX_VS = 
#include "../shaders/lightingV.glsl"
;

RendererSystem::RendererSystem() {
	m_activeCamera = nullptr;
	m_activeCameraTransform = nullptr;
	
	int vp[4];
	glGetIntegerv(GL_VIEWPORT, vp);
	
	m_gbuffer = Builder<FrameBuffer>::build()
			.setSize(vp[2], vp[3])
			.addRenderBuffer(TextureFormat::Depth, Attachment::DepthAttachment)
			.addColorAttachment(TextureFormat::RGf) // Normals
			.addColorAttachment(TextureFormat::RGB) // Albedo
			.addColorAttachment(TextureFormat::RGBf) // RME
			.addDepthAttachment();
	
	m_finalBuffer = Builder<FrameBuffer>::build()
			.setSize(vp[2], vp[3])
			.addColorAttachment(TextureFormat::RGBf)
			.addDepthAttachment();
	
	m_pingPongBuffer = Builder<FrameBuffer>::build()
			.setSize(vp[2], vp[3])
			.addColorAttachment(TextureFormat::RGBf)
			.addColorAttachment(TextureFormat::RGBf);
	
	m_irradianceCaptureBuffer = Builder<FrameBuffer>::build()
			.setSize(32, 32)
			.addRenderBuffer(TextureFormat::Depth, Attachment::DepthAttachment);
	
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
	
	m_lightingShader = Builder<ShaderProgram>::build()
			.add(lVS, ShaderType::VertexShader)
			.add(lFS, ShaderType::FragmentShader);
	m_lightingShader.link();
	
	String fVS =
#include "../shaders/lightingV.glsl"
			;
	String fFS =
#include "../shaders/screenF.glsl"
			;
	
	fVS = Util::replace(fVS, "#include common", common);
	fFS = Util::replace(fFS, "#include common", common);
	fFS = Util::replace(fFS, "#include brdf", brdf);
	
	m_finalShader = Builder<ShaderProgram>::build()
			.add(fVS, ShaderType::VertexShader)
			.add(fFS, ShaderType::FragmentShader);
	m_finalShader.link();
	
	String cmVS = 
#include "../shaders/cmV.glsl"
			;
	String cmFS = 
#include "../shaders/cmF.glsl"
			;
	m_cubeMapShader = Builder<ShaderProgram>::build()
			.add(cmVS, ShaderType::VertexShader)
			.add(cmFS, ShaderType::FragmentShader);
	m_cubeMapShader.link();
	
	String cmiFS = 
#include "../shaders/cmIrradianceF.glsl"
			;
	m_irradianceShader = Builder<ShaderProgram>::build()
			.add(cmVS, ShaderType::VertexShader)
			.add(cmiFS, ShaderType::FragmentShader);
	m_irradianceShader.link();
	
	m_plane = Builder<Mesh>::build();
	m_plane.addVertex(Vertex(Vec3(0, 0, 0)))
		.addVertex(Vertex(Vec3(1, 0, 0)))
		.addVertex(Vertex(Vec3(1, 1, 0)))
		.addVertex(Vertex(Vec3(0, 1, 0)))
		.addTriangle(0, 1, 2)
		.addTriangle(2, 3, 0)
		.flush();
	
	m_cube = Builder<Mesh>::build();
	m_cube.addPlane(Axis::X, 1.0f, Vec3(-1, 0, 0));
	m_cube.addPlane(Axis::X, 1.0f, Vec3(1, 0, 0));
	m_cube.addPlane(Axis::Y, 1.0f, Vec3(0, -1, 0));
	m_cube.addPlane(Axis::Y, 1.0f, Vec3(0, 1, 0));
	m_cube.addPlane(Axis::Z, 1.0f, Vec3(0, 0, -1));
	m_cube.addPlane(Axis::Z, 1.0f, Vec3(0, 0, 1));
	m_cube.flush();
	
	m_screenMipSampler = Builder<Sampler>::build()
			.setFilter(TextureFilter::LinearMipLinear, TextureFilter::Linear)
			.setWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);
	
	m_screenTextureSampler = Builder<Sampler>::build()
			.setFilter(TextureFilter::Linear, TextureFilter::Linear)
			.setWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);
	
	m_screenDepthSampler = Builder<Sampler>::build()
			.setFilter(TextureFilter::Nearest, TextureFilter::Nearest)
			.setWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);
	
	m_cubeMapSampler = Builder<Sampler>::build()
			.setFilter(TextureFilter::LinearMipLinear, TextureFilter::Linear)
			.setWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);
	
	m_cubeMapSamplerNoMip = Builder<Sampler>::build()
			.setFilter(TextureFilter::Linear, TextureFilter::Linear)
			.setWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);
	
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
}

void RendererSystem::update(float dt) {
	m_time += dt;
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
	
	if (m_irradianceEnvMap.id() == 0) {
		computeIrradiance();
	}
	
	glEnable(GL_DEPTH_TEST);
	
	/// Fill GBuffer
	m_gbuffer.bind();
	glClearColor(0, 0, 0, 1);
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
		m_gbufferShader.get("material.albedo").set(D.material.albedo.xyz);
		
		int sloti = 0;
		for (TextureSlot slot : D.material.textures) {
			if (!slot.enabled) continue;
			
			String tname = "";
			switch (slot.type) {
				case TextureSlotType::NormalMap: tname = "tNormalMap"; break;
				case TextureSlotType::RougnessMetallicEmission: tname = "tRMEMap"; break;
				case TextureSlotType::Albedo0: tname = "tAlbedo0"; break;
				case TextureSlotType::Albedo1: tname = "tAlbedo1"; break;
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
	
	// Lights
	m_finalBuffer.bind();
	
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	
	m_lightingShader.bind();
	m_lightingShader.get("mProjection").set(projMat);
	m_lightingShader.get("mView").set(viewMat);
	
	m_gbuffer.getColorAttachment(0).bind(m_screenTextureSampler, 0); // Normals
	m_gbuffer.getColorAttachment(1).bind(m_screenTextureSampler, 1); // Albedo	
	m_gbuffer.getColorAttachment(2).bind(m_screenTextureSampler, 2); // RME
	m_gbuffer.getDepthAttachment().bind(m_screenDepthSampler, 3); // Depth
	
	m_lightingShader.get("tNormals").set(0);
	m_lightingShader.get("tAlbedo").set(1);
	m_lightingShader.get("tRME").set(2);
	m_lightingShader.get("tDepth").set(3);
	
	if (m_activeCameraTransform) {
		m_lightingShader.get("uEye").set(m_activeCameraTransform->worldPosition());
	}
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	
	m_plane.bind();
	
	// Directional Lights
	world.each([&](Entity& ent, Transform& T, DirectionalLight& L) {
		m_lightingShader.get("uLight.type").set(L.getType());
		m_lightingShader.get("uLight.color").set(L.color);
		m_lightingShader.get("uLight.intensity").set(L.intensity);
		
		m_lightingShader.get("uLight.direction").set(T.worldRotation().forward());
		
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
	});
	
	// Point Lights
	world.each([&](Entity& ent, Transform& T, PointLight& L) {
		m_lightingShader.get("uLight.type").set(L.getType());
		m_lightingShader.get("uLight.color").set(L.color);
		m_lightingShader.get("uLight.intensity").set(L.intensity);
		
		m_lightingShader.get("uLight.position").set(T.worldPosition());
		m_lightingShader.get("uLight.radius").set(L.radius);
		m_lightingShader.get("uLight.lightCutoff").set(L.lightCutOff);
		
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
	});
	
	// Spot Lights
	world.each([&](Entity& ent, Transform& T, SpotLight& L) {
		m_lightingShader.get("uLight.type").set(L.getType());
		m_lightingShader.get("uLight.color").set(L.color);
		m_lightingShader.get("uLight.intensity").set(L.intensity);
		
		m_lightingShader.get("uLight.position").set(T.worldPosition());
		m_lightingShader.get("uLight.direction").set(T.worldRotation().forward());
		m_lightingShader.get("uLight.radius").set(L.radius);
		m_lightingShader.get("uLight.lightCutoff").set(L.lightCutOff);
		m_lightingShader.get("uLight.spotCutoff").set(L.spotCutOff);
		
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
	});
	
	m_plane.unbind();
	m_lightingShader.unbind();
	
	if (m_irradianceEnvMap.id() != 0) {	
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glDepthFunc(GL_LEQUAL);
		glClear(GL_DEPTH_BUFFER_BIT);
		
		m_gbuffer.bind(FrameBufferTarget::ReadFramebuffer);
		m_finalBuffer.blit(
				0, 0, m_gbuffer.width(), m_gbuffer.height(),
				0, 0, m_gbuffer.width(), m_gbuffer.height(),
				ClearBufferMask::DepthBuffer,
				TextureFilter::Nearest
		);
		
		m_cube.bind();
		m_cubeMapShader.bind();
		m_cubeMapShader.get("mProjection").set(projMat);
		m_cubeMapShader.get("mView").set(viewMat);
		
		m_irradianceEnvMap.bind(m_cubeMapSampler, 0);
		m_cubeMapShader.get("mCubeMap").set(0);
		
		glDrawElements(GL_TRIANGLES, m_cube.indexCount(), GL_UNSIGNED_INT, nullptr);
		
		m_gbuffer.unbind();
		m_irradianceEnvMap.unbind();
		m_cube.unbind();
		m_cubeMapShader.unbind();
		
		glDepthFunc(GL_LESS);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
	}
	
	m_finalBuffer.unbind();
	
	m_finalBuffer.getColorAttachment(0).bind(m_screenMipSampler, 0);
	m_finalBuffer.getColorAttachment(0).generateMipmaps();
	m_finalBuffer.getColorAttachment(0).unbind();
	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	/// Final render
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	
	m_plane.bind();
	
	/// Post processing
	if (m_postEffects.empty()) {
		m_finalShader.bind();

		m_finalBuffer.getColorAttachment(0).bind(m_screenMipSampler, 0);
		m_finalShader.get("tTex").set(0);

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

		m_finalShader.unbind();
	} else {
		int currActive = 0;
		bool first = true;
		
		m_pingPongBuffer.bind();
		for (ShaderProgram& shd : m_postEffects) {
			int src = currActive;
			int dest = 1 - currActive;
			
			m_pingPongBuffer.setDrawBuffer(dest);
			
			shd.bind();
			
			if (first) {
				m_finalBuffer.getColorAttachment(0).bind(m_screenMipSampler, 0);
			} else {
				m_pingPongBuffer.getColorAttachment(src).generateMipmaps();
				m_pingPongBuffer.getColorAttachment(src).bind(m_screenMipSampler, 0);
			}
			
			shd.get("tScreen").set(0);
			shd.get("tTime").set(m_time);
			shd.get("tResolution").set(Vec2(m_finalBuffer.width(), m_finalBuffer.height()));
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
			
			if (first) {
				m_finalBuffer.getColorAttachment(0).unbind();
				first = false;
			} else {
				m_pingPongBuffer.getColorAttachment(src).unbind();
			}
			
			shd.unbind();
			
			currActive = 1 - currActive;
		}
		m_pingPongBuffer.unbind();
		
		m_finalShader.bind();

		m_pingPongBuffer.getColorAttachment(currActive).bind(m_screenTextureSampler, 0);
		m_finalShader.get("tTex").set(0);

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

		m_finalShader.unbind();
	}
	
	m_plane.unbind();
}

void RendererSystem::computeIrradiance() {
	if (m_envMap.id() == 0) return;
	
	const Mat4 capProj = Mat4::perspective(radians(45.0f), 1.0f, 0.1f, 10.0f);
	const Mat4 capViews[6] = {
		Mat4::lookAt(Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, -1, 0)),
		Mat4::lookAt(Vec3(0, 0, 0), Vec3(-1, 0, 0), Vec3(0, -1, 0)),
		Mat4::lookAt(Vec3(0, 0, 0), Vec3(0, 1, 0), Vec3(0, 0, 1)),
		Mat4::lookAt(Vec3(0, 0, 0), Vec3(0, -1, 0), Vec3(0, 0, -1)),
		Mat4::lookAt(Vec3(0, 0, 0), Vec3(0, 0, 1), Vec3(0, -1, 0)),
		Mat4::lookAt(Vec3(0, 0, 0), Vec3(0, 0, -1), Vec3(0, -1, 0))
	};
	
	if (m_irradianceEnvMap.id() != 0) {
		Builder<Texture>::dispose(m_irradianceEnvMap);
	}
	
	m_irradianceEnvMap = Builder<Texture>::build()
			.bind(TextureTarget::CubeMap)
			.setCubemapNull(32, 32, TextureFormat::RGBf);
	
	m_irradianceShader.bind();
	m_irradianceShader.get("mProjection").set(capProj);
	
	m_envMap.bind(m_cubeMapSampler, 0);
	m_irradianceShader.get("tCubeMap").set(0);
	
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	
	m_irradianceCaptureBuffer.bind();
	m_cube.bind();
	for (u32 i = 0; i < 6; i++) {
		m_irradianceShader.get("mView").set(capViews[i]);
		m_irradianceCaptureBuffer.setColorAttachment(0, (TextureTarget)(TextureTarget::CubeMapPX + i), m_irradianceEnvMap);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDrawElements(GL_TRIANGLES, m_cube.indexCount(), GL_UNSIGNED_INT, nullptr);
	}
	m_cube.unbind();
	m_irradianceCaptureBuffer.unbind();
	m_irradianceEnvMap.generateMipmaps();
	
	m_irradianceShader.unbind();
	
	glEnable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
}

RendererSystem& RendererSystem::addPostEffect(ShaderProgram effect) {
	m_postEffects.push_back(effect);
	return *this;
}

RendererSystem& RendererSystem::removePostEffect(u32 index) {
	m_postEffects.erase(m_postEffects.begin() + index);
	return *this;
}

RendererSystem& RendererSystem::setEnvironmentMap(const Texture& tex) {
	if (tex.target() != TextureTarget::CubeMap) {
		LogError("Texture is not a Cube Map.");
		return *this;
	}
	m_envMap = tex;
	return *this;
}

NS_END