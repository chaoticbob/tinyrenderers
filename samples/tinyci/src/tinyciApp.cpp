#include "cinder/app/App.h"
using namespace ci;
using namespace ci::app;
using namespace std;

#define TINY_RENDERER_VK
#include "tinyci.h"

class tinyciApp : public App {
public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;

private:
	tr_renderer*	mRenderer;
	tr_cmd*			mCmd;
};


VKAPI_ATTR VkBool32 VKAPI_CALL proxyDebugReportCallback(
    VkDebugReportFlagsEXT      flags,
    VkDebugReportObjectTypeEXT objectType,
    uint64_t                   object,
    size_t                     location,
    int32_t                    messageCode,
    const char*                pLayerPrefix,
    const char*                pMessage,
    void*                      pUserData
)
{
	return false;
}

void tinyciApp::setup()
{
	//mRenderer = reinterpret_cast<TinyRenderer*>(this->getRenderer().get())->getRenderer();
	//mRenderer->settings.vk_debug_fn = proxyDebugReportCallback;

	auto renderer = tr_get_renderer();
	tr_create_cmd(renderer, renderer->graphics_queue->vk_queue_family_index, 0, &mCmd);
}

void tinyciApp::mouseDown( MouseEvent event )
{
}

void tinyciApp::update()
{
}

void tinyciApp::draw()
{
	auto renderer = tr_get_renderer();

	uint32_t frameIdx = getElapsedFrames() % renderer->settings.swapchain.image_count;

	tr_acquire_next_image(renderer, renderer->image_acquired_semaphores[frameIdx], renderer->image_acquired_fences[frameIdx]);

	tr_begin_cmd(mCmd);
	//tr_cmd_begin_render(mCmd, renderer->swapchain_render_targets[renderer->swapchain_image_index]);
	//tr_cmd_end_render(mCmd);
	tr_end_cmd(mCmd);

	tr_queue_submit(renderer->graphics_queue, 1, &mCmd, 1, &(renderer->image_acquired_semaphores[frameIdx]), 1, &(renderer->render_complete_semaphores[frameIdx]));
	tr_queue_present(renderer->present_queue, 1, &(renderer->render_complete_semaphores[frameIdx]));

	tr_queue_wait_idle(renderer->graphics_queue);
}

CINDER_APP( tinyciApp, TinyRenderer )
