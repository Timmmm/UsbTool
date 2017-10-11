#pragma once

#if defined(__APPLE__)

#include <thread>
#include <mutex>
#include <condition_variable>

#include <CoreFoundation/CoreFoundation.h>

// For asynchronous events on OSX, you have to manually create
// a thread and call their run loop function. Then other threads can add
// event sources to it.
class RunLoop
{
public:
	RunLoop();
	~RunLoop();
	
	// Get the run loop. Returns null if for some reason its creation failed
	// (although currently in that case RunLoop() will freeze forever).
	CFRunLoopRef loop() const;
	
private:
	RunLoop(const RunLoop&) = delete;
	RunLoop& operator=(const RunLoop&) = delete;
	
	// This function runs on another thread.
	void run();
	
	// If this is null it means the run loop is not running. Access is protected
	// via the runLoopMutex, and the runLoopWait condition variable is used in the constructor
	// so that the constructor only returns once runLoop has been set.
	CFRunLoopRef runLoop = nullptr;
	
	// Protects runLoop and is used by runLoopWait.
	mutable std::mutex runLoopMutex;
	
	// Used to wait for runLoop to be set in the constructor.
	std::condition_variable runLoopWait;
	
	// This is used to shut down the run loop. It is implicitly protected by runLoopMutex.
	CFRunLoopSourceRef shutdownSource;
	
	// The actual run loop thread.
	std::thread thread;
};

#endif
