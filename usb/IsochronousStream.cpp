#include "IsochronousStream.h"

#include <iostream>

using std::cerr;
using std::endl;
using std::cout;

IsochronousStream::IsochronousStream(Device& dev, int iface, uint8_t pipe, int bytesPerFrame)
	: mDev(dev), mIface(iface), mPipe(pipe), mBytesPerFrame(bytesPerFrame)
{
	// Start the submit transfers thread.
	mSubmitTransfersThread = std::thread([&] {
		SubmitTransfersFunc();
	});
}

IsochronousStream::~IsochronousStream()
{
	cout << "Stopping isochronous stream" << endl;
	mSubmitTransfersQuit = true;
	mSubmitTransfersThread.join();
}

uint64_t IsochronousStream::CurrentFrameNumber() const
{
	return mDev.getBusFrameNumber();
}

bool IsochronousStream::WriteFrame(uint64_t usbFrame, const uint8_t* data)
{
//	for (int i = 0; i < NUM_TRANSFERS; ++i)
//	{
//		uint64_t sf = mTransfers[i].startFrame;
//		if (sf <= usbFrame && usbFrame < sf + FRAMES_PER_TRANSFER)
//		{
//			memcpy(mTransfers[i].writeBuffer.data(), data, mBytesPerFrame);
//			return true;
//		}
//	}
	return false;
}

void IsochronousStream::SubmitTransfersFunc()
{
//	cout << "Starting iso submission thread" << endl;
//	// We'll just use one buffer for each transfer and loop them.
//	for (int i = 0; i < NUM_TRANSFERS; ++i)
//	{
//		auto&& res = mDev.createIsochWriteBuffer(mIface, mPipe, FRAMES_PER_TRANSFER, mBytesPerFrame);
//		if (!res)
//		{
//			cerr << "Error creating isoch buffer" << res.unwrap_err() << endl;
//			return;
//		}
//		mTransfers[i].writeBuffer = res.unwrap();
//		mTransfers[i].startFrame = 0;
//	}
	
//	// TODO: Latency measurement. I need to make this work at all first.
	
//	// 16 ms in the future should be plenty.
//	uint64_t submissionFrame = mDev.getBusFrameNumber() + 16;
	
//	std::vector<bool> transferSubmitted(NUM_TRANSFERS, false);
	
//	// Loop until we are told to quit.
//	for (int i = 0; !mSubmitTransfersQuit; i = (i + 1) % NUM_TRANSFERS)
//	{
//		// Wait for the second oldest transfer to finish. We don't want for the oldest one
//		// because it seems to complete before it has actually finished! I know right wtf!
//		int w = (i + 1) % NUM_TRANSFERS;
//		if (transferSubmitted[w])
//		{
//			cerr << "Loop: Wait for transfer: " << w << " (" << mTransfers[w].startFrame << "- )" << endl;
//			// Wait for it to complete.
//			auto&& res = mTransfers[w].transferHandle.result();
//			if (!res)
//			{
//				cerr << "Error completing transfer " << w << ": " << res.unwrap_err() << endl;
//				return;
//			}
//			cerr << "Loop: Transfer complete: " << w << " (- " << mTransfers[w].startFrame + FRAMES_PER_TRANSFER << ") @ " << mDev.getBusFrameNumber() << endl;
//		}

//		// Ok let's submit a transfer.
		
//		// Zero the buffer. This is interpretted as the device as padding/underflow.
//		memset(mTransfers[i].writeBuffer.data(), 0, mTransfers[i].writeBuffer.size());
		
//		// Set the start frame.
//		mTransfers[i].startFrame = submissionFrame;
		
//		cout << "Loop: Submitting transfer: " << i << " (" << submissionFrame << ")" << endl;
//		// Submit it at the appropriate place.
//		auto&& res = mDev.submitIsoOutTransfer(mTransfers[i].writeBuffer, submissionFrame);
//		if (!res)
//		{
//			cerr << "Error submitting transfer " << i << ": " << res.unwrap_err() << endl;
//			return;
//		}
		
//		// Record the new transfer handle.
//		mTransfers[i].transferHandle = res.unwrap();
		
//		// This transfer has been submitted.
//		transferSubmitted[i] = true;
		
//		// When to submit the next frame...
//		submissionFrame += FRAMES_PER_TRANSFER;
//	}
//	cout << "Iso submission thread exit..." << endl;
	
//	// Wait for all the transfers to finish.
//	for (int i = 0; i < NUM_TRANSFERS; ++i)
//	{
//		if (transferSubmitted[i])
//		{
//			auto&& res = mTransfers[i].transferHandle.result();
//			if (!res)
//			{
//				cerr << "Error completing transfer " << i << ": " << res.unwrap_err() << endl;
//				return;
//			}
//		}
//	}
}
