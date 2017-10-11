#pragma once

#include <thread>
#include <array>
#include <atomic>
#include <stdint.h>

#include "Device.h"

// This class tracks the current haptic frame number, and where to write frames.
class IsochronousStream
{
public:
	IsochronousStream(Device& dev, int iface, uint8_t pipe, int bytesPerFrame);
	virtual ~IsochronousStream();
	
	// Get the current USB frame number.
	uint64_t CurrentFrameNumber() const;
	
	// Write a usbFrame. Returns false if it was way in the past or future.
	// Even if it returns true, it may have only just been in the past.
	// `data` must point to `bytesPerFrame` bytes.
	bool WriteFrame(uint64_t usbFrame, const uint8_t* data);
private:
	
	// This function loops, submitting 64-ms isochronous transfers.
	void SubmitTransfersFunc();
	
	struct TransferInfo
	{
		IsochWriteBuffer writeBuffer;
		UsbIsochTransferHandle transferHandle;
		
		// The first USB frame for this transfer.
		std::atomic<uint64_t> startFrame;
	};
	
	// Number of in-flight transfers to have. We need at least two. 3 or 4 is probably reasonable
	// to be safe.
	static const int NUM_TRANSFERS = 4;
	// How long should each transfer be. This should be reasonably high so that we aren't submitting
	// transfers all the time.
	static const int FRAMES_PER_TRANSFER = 64;
	
	// Array of transfers.
	std::array<TransferInfo, NUM_TRANSFERS> mTransfers;
	
	// We have a thread that loops just submitting transfers.
	std::thread mSubmitTransfersThread;
	// Bool to indicate that the submit thread should exit.
	std::atomic_bool mSubmitTransfersQuit{false};
	
	Device& mDev;
	int mIface = 0;
	uint8_t mPipe = 0;
	int mBytesPerFrame = 0;
};
