#include <magma_engine/Engine.h>
#include <magma_engine/ServiceLocater.h>
#include <magma_engine/Window.h>
#include <magma_engine/core/renderer/Renderer.h>

void Magma::Engine::Init()
{
	ServiceLocator::Register(std::make_shared<Window>());
	ServiceLocator::Get<Window>()->OpenWindow({
		"Magma",
		1920,
		1080
	});

	ServiceLocator::Register(std::make_shared<Renderer>());
	ServiceLocator::Get<Renderer>()->Init();
}

void Magma::Engine::Run()
{
	while(!ServiceLocator::Get<Window>()->ShouldClose())
	{
		ServiceLocator::Get<Window>()->Update();
	}

	// Gracefully Exit
	Cleanup();
}

void Magma::Engine::Cleanup()
{
	ServiceLocator::ShutdownServices();
}
