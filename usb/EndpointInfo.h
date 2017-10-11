#pragma once

#include <string>

#include "Descriptors.h"
#include "util/EnumCasts.h"

struct EndpointInfo
{
	static EndpointInfo from(EndpointDescriptor desc)
	{
		EndpointInfo ep;
		ep.direction = (desc.bEndpointAddress & 0x80) == 0 ? Direction::Out : Direction::In;
		ep.number = desc.bEndpointAddress & 0x0F;
		ep.type = from_integral<Type>(desc.bmAttributes & 0x03);
		ep.maxPacketSize = desc.wMaxPacketSize;
		ep.interval = desc.bInterval;
		ep.synchronisation = from_integral<Synchronisation>((desc.bmAttributes >> 2) & 0x03);
		ep.usage = from_integral<Usage>((desc.bmAttributes >> 4) & 0x03);
		return ep;
	}
	
	// The direction from the host's perspective. Out is from the host to device.
	// In is from the device to the host.
	enum class Direction {
		Out = 0,
		In = 1,
	} direction = Direction::Out;
	
	// The endpoint number of the address (0-15), not including the direction bit.
	int number = 0;
	
	// The type of endpoint.
	enum class Type {
		Control = 0,
		Isochronous = 1,
		Bulk = 2,
		Interrupt = 3,
	} type = Type::Control;
	
	// Maximum size of a single packet.
	int maxPacketSize = 0;
	
	// bInterval is ignored for bulk and control endpoints.
	//
	// For low speed endpoints this is 1-255 and specifies the number of frames (ms)
	// for which the endpoint is serviced. E.g. if it is 1 then the endpoint is serviced
	// every frame. If it is 5 it is serviced every 5th frame (every 5 ms).
	//
	// For high speed endpoints it is 1-16 and specifies the number of microframes (125 us)
	// for which the endpoint is serviced, but as 2^(bInterval-1).
	int interval = 0;
	
	// The following values only apply to isochronous endpoints.
	enum class Synchronisation {
		None = 0,
		Asyncronous = 1,
		Adaptive = 2,
		Synchronous = 3,
	} synchronisation = Synchronisation::None;
	
	enum class Usage {
		Data = 0,
		Feedback = 1,
		ImplicitFeedback = 2,
		Reserved = 3,
	} usage = Usage::Data;
};

std::string to_string(const EndpointInfo& val);
