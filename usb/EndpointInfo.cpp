#include "EndpointInfo.h"

std::string to_string(const EndpointInfo &val)
{
	std::string s = "#" + std::to_string(val.number);
	
	switch (val.type)
	{
	case EndpointInfo::Type::Control:
		s += " Control";
		break;
	case EndpointInfo::Type::Isochronous:
		s += " Isochronous";
		break;
	case EndpointInfo::Type::Bulk:
		s += " Bulk";
		break;
	case EndpointInfo::Type::Interrupt:
		s += " Interrupt";
		break;
	default:
		s += " Unknown";
		break;
	}
	
	switch (val.direction)
	{
	case EndpointInfo::Direction::In:
		s += " IN";
		break;
	case EndpointInfo::Direction::Out:
		s += " OUT";
		break;
	}
	
	s += " Max Packet Size: " + std::to_string(val.maxPacketSize);
	s += " Interval: " + std::to_string(val.interval);
	
	if (val.type == EndpointInfo::Type::Isochronous)
	{
		switch (val.synchronisation)
		{
		case EndpointInfo::Synchronisation::None:
			s += " No Sync";
			break;
		case EndpointInfo::Synchronisation::Asyncronous:
			s += " Async";
			break;
		case EndpointInfo::Synchronisation::Adaptive:
			s += " Adaptive";
			break;
		case EndpointInfo::Synchronisation::Synchronous:
			s += " Sync";
			break;
		default:
			s += " Unknown Sync";
			break;
		}
		
		switch (val.usage)
		{
		case EndpointInfo::Usage::Data:
			s += ", Data";
			break;
		case EndpointInfo::Usage::Feedback:
			s += ", Feedback";
			break;
		case EndpointInfo::Usage::ImplicitFeedback:
			s += ", Implicit Feedback";
			break;
		case EndpointInfo::Usage::Reserved:
			s += ", Reserved";
			break;
		default:
			s += " Unknown Usage";
			break;
		}
	}
	return s;
}
