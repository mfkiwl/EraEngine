#pragma once

#include "core/math.h"
#include "core/camera.h"
#include "render_pass.h"
#include "light_source.h"
#include "pbr.h"
#include "core/input.h"
#include "raytracer.h"
#include "render_algorithms.h"
#include "render_resources.h"
#include "render_utils.h"
#include "path_tracing.h"
#include "light_probe.h"
#include "pbr_environment.h"
#include "light_source.hlsli"
#include "camera.hlsli"
#include <dlss/dlss.h>

#define MAX_NUM_SUN_LIGHT_SHADOW_PASSES 16
#define MAX_NUM_SPOT_LIGHT_SHADOW_PASSES 16
#define MAX_NUM_POINT_LIGHT_SHADOW_PASSES 16

struct renderer_settings
{
	tonemap_settings tonemapSettings;
	tonemap_settings defaultTonemapSettings;

	bool enableAO = true;
	hbao_settings aoSettings;

	bool enableSSS = true;
	sss_settings sssSettings;

	bool enableSSR = true;
	ssr_settings ssrSettings;

	bool enableTAA = true;
	taa_settings taaSettings;

	bool enableDLSS = false;

	bool cacheShadowMap = false;

	bool enableBloom = true;
	bloom_settings bloomSettings;

	bool enableSharpen = true;
	sharpen_settings sharpenSettings;
};
REFLECT_STRUCT(renderer_settings,
	(tonemapSettings, "Tonemap"),
	(enableAO, "Enable AO"),
	(aoSettings, "AO"),
	(enableSSS, "Enable SSS"),
	(sssSettings, "SSS"),
	(enableSSR, "Enable SSR"),
	(ssrSettings, "SSR"),
	(enableTAA, "Enable TAA"),
	(taaSettings, "TAA"),
	(enableBloom, "Enable Bloom"),
	(bloomSettings, "Bloom"),
	(enableSharpen, "Enable sharpen"),
	(sharpenSettings, "Sharpen")
);

enum aspect_ratio_mode
{
	aspect_ratio_free,
	aspect_ratio_fix_16_9,
	aspect_ratio_fix_16_10,

	aspect_ratio_mode_count,
};

static const char* aspectRatioNames[] =
{
	"Free",
	"16:9",
	"16:10",
};

enum renderer_mode
{
	renderer_mode_rasterized,
	renderer_mode_visualize_sun_shadow_cascades,
	renderer_mode_pathtraced,

	renderer_mode_count,
};

static const char* rendererModeNames[] =
{
	"Rasterized",
	"Visualize sun shadow cascades",
	"Path-traced",
};

struct renderer_spec
{
	bool allowObjectPicking = true;
	bool allowAO = true;
	bool allowSSS = true;
	bool allowSSR = true;
	bool allowTAA = true;
	bool allowBloom = true;
	bool allowDLSS = true;
};

struct main_renderer
{
	main_renderer() {}
	void initialize(color_depth colorDepth, uint32 windowWidth, uint32 windowHeight, renderer_spec spec, float initDLSS = false);

	void beginFrame(uint32 windowWidth, uint32 windowHeight);
	void endFrame(const user_input* input, float dt);

	void setCamera(const render_camera& camera);

	void dlssPass(float dt);

	void submitRenderPass(opaque_render_pass* renderPass) {	ASSERT(!opaqueRenderPass); opaqueRenderPass = renderPass; }
	void submitRenderPass(transparent_render_pass* renderPass) { ASSERT(!transparentRenderPass); transparentRenderPass = renderPass; }
	void submitRenderPass(ldr_render_pass* renderPass) { ASSERT(!ldrRenderPass); ldrRenderPass = renderPass; }
	void submitComputePass(compute_pass* computePass) { ASSERT(!this->computePass); this->computePass = computePass; }

	void submitShadowRenderPass(sun_shadow_render_pass* renderPass) { ASSERT(numSunLightShadowRenderPasses < MAX_NUM_SUN_LIGHT_SHADOW_PASSES); sunShadowRenderPasses[numSunLightShadowRenderPasses++] = renderPass; }
	void submitShadowRenderPass(spot_shadow_render_pass* renderPass) { ASSERT(numSpotLightShadowRenderPasses < MAX_NUM_SPOT_LIGHT_SHADOW_PASSES);	spotLightShadowRenderPasses[numSpotLightShadowRenderPasses++] = renderPass; }
	void submitShadowRenderPass(point_shadow_render_pass* renderPass) { ASSERT(numPointLightShadowRenderPasses < MAX_NUM_POINT_LIGHT_SHADOW_PASSES); pointLightShadowRenderPasses[numPointLightShadowRenderPasses++] = renderPass; }

	void setRaytracingScene(raytracing_tlas* tlas) { this->tlas = tlas; }

	void setEnvironment(const pbr_environment& environment);
	void setSun(const directional_light& light);

	void setPointLights(const ref<dx_buffer>& lights, uint32 numLights, const ref<dx_buffer>& shadowInfoBuffer);
	void setSpotLights(const ref<dx_buffer>& lights, uint32 numLights, const ref<dx_buffer>& shadowInfoBuffer);
	void setDecals(const ref<dx_buffer>& decals, uint32 numDecals, const ref<dx_texture>& textureAtlas);

	// Settings.
	renderer_mode mode = renderer_mode_rasterized;
	aspect_ratio_mode aspectRatioMode = aspect_ratio_free;

	renderer_settings settings{};

	uint32 renderWidth{};
	uint32 renderHeight{};
	ref<dx_texture> frameResult;

	uint32 hoveredObjectID = -1;

	const renderer_spec spec;

	path_tracer pathTracer{};

	NODISCARD const ref<dx_texture>& getAOResult() const { return aoTextures[aoHistoryIndex]; }
	NODISCARD const ref<dx_texture>& getSSSResult() const { return sssTextures[sssHistoryIndex]; }
	NODISCARD const ref<dx_texture>& getSSRResult() const { return ssrResolveTexture; }
	NODISCARD const ref<dx_texture>& getBloomResult() const { return bloomTexture; }
	NODISCARD const ref<dx_texture>& getScreenVelocities() const { return screenVelocitiesTexture; }

private:
	template <typename func_t = std::nullptr_t>
	uint64 executeComputeTasks(compute_pass_event eventTime, const func_t& additionalTasksCallback = nullptr);

	NODISCARD dx_command_list* renderThread0(const common_render_data& commonRenderData);
	NODISCARD dx_command_list* renderThread1(const common_render_data& commonRenderData, bool aspectRatioModeChanged);
	NODISCARD dx_command_list* renderThread2(const common_render_data& commonRenderData, const user_input* input);
	NODISCARD dx_command_list* renderThread3(common_render_data commonRenderData, dx_dynamic_constant_buffer unjitteredCameraCBV);

	const sun_shadow_render_pass* sunShadowRenderPasses[MAX_NUM_SUN_LIGHT_SHADOW_PASSES];
	const spot_shadow_render_pass* spotLightShadowRenderPasses[MAX_NUM_SPOT_LIGHT_SHADOW_PASSES];
	const point_shadow_render_pass* pointLightShadowRenderPasses[MAX_NUM_POINT_LIGHT_SHADOW_PASSES];
	uint32 numSunLightShadowRenderPasses{};
	uint32 numSpotLightShadowRenderPasses{};
	uint32 numPointLightShadowRenderPasses{};

	raytracing_tlas* tlas;

	const pbr_environment* environment;
	directional_light_cb sun{};

	ref<dx_buffer> pointLights;
	ref<dx_buffer> spotLights;
	ref<dx_buffer> pointLightShadowInfoBuffer;
	ref<dx_buffer> spotLightShadowInfoBuffer;
	ref<dx_buffer> decals;
	uint32 numPointLights{};
	uint32 numSpotLights{};
	uint32 numDecals{};
	
	ref<dx_texture> decalTextureAtlas;

	const opaque_render_pass* opaqueRenderPass;
	const transparent_render_pass* transparentRenderPass;
	const ldr_render_pass* ldrRenderPass;
	const compute_pass* computePass;

	uint32 windowWidth{};
	uint32 windowHeight{};
	int32 windowXOffset{};
	int32 windowYOffset{};

	bool windowHovered = false;

	ref<dx_texture> hdrColorTexture;
	ref<dx_texture> worldNormalsRoughnessTexture;
	ref<dx_texture> screenVelocitiesTexture;
	ref<dx_texture> objectIDsTexture;
	ref<dx_texture> depthStencilBuffer;
	ref<dx_texture> linearDepthBuffer;
	ref<dx_texture> opaqueDepthBuffer; // The depth-stencil buffer gets copied to this texture after the opaque pass.

	ref<dx_texture> aoCalculationTexture;
	ref<dx_texture> aoBlurTempTexture;
	ref<dx_texture> aoTextures[2]; // These get flip-flopped from frame to frame.
	volatile uint32 aoHistoryIndex = 0;
	bool aoWasOnLastFrame = false;

	ref<dx_texture> sssCalculationTexture;
	ref<dx_texture> sssBlurTempTexture;
	ref<dx_texture> sssTextures[2]; // These get flip-flopped from frame to frame.
	uint32 sssHistoryIndex = 0;
	bool sssWasOnLastFrame = false;

	ref<dx_texture> ssrRaycastTexture;
	ref<dx_texture> ssrResolveTexture;
	ref<dx_texture> ssrTemporalTextures[2]; // These get flip-flopped from frame to frame.
	uint32 ssrHistoryIndex = 0;

	ref<dx_texture> prevFrameHDRColorTexture; // This is downsampled by a factor of 2 and contains up to 8 mip levels.
	ref<dx_texture> prevFrameHDRColorTempTexture;

	ref<dx_texture> hdrPostProcessingTexture;
	ref<dx_texture> ldrPostProcessingTexture;
	ref<dx_texture> taaTextures[2]; // These get flip-flopped from frame to frame.
	uint32 taaHistoryIndex = 0;

	ref<dx_texture> bloomTexture;
	ref<dx_texture> bloomTempTexture;

	ref<dx_buffer> hoveredObjectIDReadbackBuffer;

	camera_cb jitteredCamera{};
	camera_cb unjitteredCamera{};

	light_culling culling{};

	dlss_feature_adapter dlss_adapter{};
	bool dlssInited = false;

	aspect_ratio_mode oldAspectRatioMode = aspect_ratio_free;
	renderer_mode oldMode = renderer_mode_rasterized;

	void recalculateViewport(bool resizeTextures);

	friend dlss_feature_adapter;
	friend struct eeditor;
};