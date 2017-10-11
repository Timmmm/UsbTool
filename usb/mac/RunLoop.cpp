#if defined(__APPLE__)

#include "RunLoop.h"

#include <memory>
#include <cstring>

RunLoop::RunLoop() : thread(std::bind(&RunLoop::run, this))
{
	// Wait for runLoop to be set.
	std::unique_lock<std::mutex> lock(runLoopMutex);
	while (runLoop == nullptr)
		runLoopWait.wait(lock);
}

RunLoop::~RunLoop()
{
	// Send the shutdown event and wait for it to finish.
	{
		std::unique_lock<std::mutex> lock(runLoopMutex);
		
		if (runLoop != nullptr)
		{
			CFRunLoopSourceSignal(shutdownSource);
			CFRunLoopWakeUp(runLoop);
		}
	}
	
	thread.join();
}

CFRunLoopRef RunLoop::loop() const
{
	std::unique_lock<std::mutex> lock(runLoopMutex);
	return runLoop;
}

void RunLoop::run()
{
	// Lock the mutex.
	{
		std::unique_lock<std::mutex> lock(runLoopMutex);
		
		// Get the run loop reference.
		runLoop = CFRunLoopGetCurrent();

		// Create a run loop event. When it is triggered it will pass
		// `runLoop` as the first parameter to `CFRunLoopStop()`.
		CFRunLoopSourceContext shutdownEvent;
		std::memset(&shutdownEvent, 0, sizeof(shutdownEvent));
		
		shutdownEvent.info = runLoop;
		shutdownEvent.perform = (void (*)(void*))CFRunLoopStop;
		
		// Create a source for the above event.
		shutdownSource = CFRunLoopSourceCreate(kCFAllocatorDefault, 0, &shutdownEvent);
		
		// Add it to the run loop.
		CFRunLoopAddSource(runLoop, shutdownSource, kCFRunLoopDefaultMode);
		
		// Allow the constructor to return.
		runLoopWait.notify_one();
	}
	
	// Start the loop!
	CFRunLoopRun();
}

#endif
