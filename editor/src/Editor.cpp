#include <iostream>

#include <magma_engine/Window.h>
#include <magma_engine/ServiceLocater.h>
#include <magma_engine/core/renderer/Renderer.h>

#include "gui/GuiContext.h"
#include "gui/GuiRenderer.h"
#include "gui/panes/ViewportPane.h"
#include "gui/panes/PropertiesPane.h"
#include "gui/panes/SceneHierarchyPane.h"
#include "gui/panes/MenuBarPane.h"

int main()
{
	Magma::ServiceLocator::Register(std::make_shared<Magma::Window>());
	Magma::ServiceLocator::Get<Magma::Window>()->OpenWindow({
		"Magma Editor",
		1920,
		1080
	});

	Magma::ServiceLocator::Register(std::make_shared<Magma::Renderer>());
	Magma::ServiceLocator::Get<Magma::Renderer>()->Init();

	auto window = Magma::ServiceLocator::Get<Magma::Window>();
	auto renderer = Magma::ServiceLocator::Get<Magma::Renderer>();

	Magma::GuiContext guiContext(window, renderer);

	Magma::GuiRenderer guiRenderer;
	guiRenderer.AddPane(std::make_unique<Magma::MenuBarPane>());
	guiRenderer.AddPane(std::make_unique<Magma::ViewportPane>(renderer));
	guiRenderer.AddPane(std::make_unique<Magma::PropertiesPane>());
	guiRenderer.AddPane(std::make_unique<Magma::SceneHierarchyPane>());

	while (!window->ShouldClose())
	{
		window->Update();

		renderer->BeginFrame();
		renderer->RenderScene();

		guiContext.BeginFrame();
		guiRenderer.RenderAllPanes();
		guiContext.EndFrame();

		renderer->CopyToSwapchain();
		renderer->BeginUIRenderPass();
		guiContext.RenderToCommandBuffer(renderer->GetCurrentCommandBuffer());
		renderer->EndFrame();
		renderer->Present();
	}

	guiContext.Cleanup();
	Magma::ServiceLocator::ShutdownServices();

	return 0;
}