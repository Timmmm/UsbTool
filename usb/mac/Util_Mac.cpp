#if defined(__APPLE__)

#include "Util_Mac.h"

#include <IOKit/IOReturn.h>
#include <IOKit/usb/USB.h>

using std::to_string;

std::string IOKit_Common_Error(kern_return_t kr)
{
	switch (err_get_code(kr))
	{
	case err_get_code(kIOReturnError): return "general error";
	case err_get_code(kIOReturnNoMemory): return "can't allocate memory";
	case err_get_code(kIOReturnNoResources): return "resource shortage";
	case err_get_code(kIOReturnIPCError): return "error during IPC";
	case err_get_code(kIOReturnNoDevice): return "no such device";
	case err_get_code(kIOReturnNotPrivileged): return "privilege violation";
	case err_get_code(kIOReturnBadArgument): return "invalid argument";
	case err_get_code(kIOReturnLockedRead): return "device read locked";
	case err_get_code(kIOReturnLockedWrite): return "device write locked";
	case err_get_code(kIOReturnExclusiveAccess): return "exclusive access and device already open";
	case err_get_code(kIOReturnBadMessageID): return "sent/received messages had different msg_id";
	case err_get_code(kIOReturnUnsupported): return "unsupported function";
	case err_get_code(kIOReturnVMError): return "misc. VM failure";
	case err_get_code(kIOReturnInternalError): return "internal error";
	case err_get_code(kIOReturnIOError): return "General I/O error";
	case err_get_code(kIOReturnCannotLock): return "can't acquire lock";
	case err_get_code(kIOReturnNotOpen): return "device not open";
	case err_get_code(kIOReturnNotReadable): return "read not supported";
	case err_get_code(kIOReturnNotWritable): return "write not supported";
	case err_get_code(kIOReturnNotAligned): return "alignment error";
	case err_get_code(kIOReturnBadMedia): return "Media Error";
	case err_get_code(kIOReturnStillOpen): return "device(s) still open";
	case err_get_code(kIOReturnRLDError): return "rld failure";
	case err_get_code(kIOReturnDMAError): return "DMA failure";
	case err_get_code(kIOReturnBusy): return "Device Busy";
	case err_get_code(kIOReturnTimeout): return "I/O Timeout";
	case err_get_code(kIOReturnOffline): return "device offline";
	case err_get_code(kIOReturnNotReady): return "not ready";
	case err_get_code(kIOReturnNotAttached): return "device not attached";
	case err_get_code(kIOReturnNoChannels): return "no DMA channels left";
	case err_get_code(kIOReturnNoSpace): return "no space for data";
	case err_get_code(kIOReturnPortExists): return "port already exists";
	case err_get_code(kIOReturnCannotWire): return "can't wire down physical memory";
	case err_get_code(kIOReturnNoInterrupt): return "no interrupt attached";
	case err_get_code(kIOReturnNoFrames): return "no DMA frames enqueued";
	case err_get_code(kIOReturnMessageTooLarge): return "oversized msg received on interrupt port";
	case err_get_code(kIOReturnNotPermitted): return "not permitted";
	case err_get_code(kIOReturnNoPower): return "no power to device";
	case err_get_code(kIOReturnNoMedia): return "media not present";
	case err_get_code(kIOReturnUnformattedMedia): return "media not formatted";
	case err_get_code(kIOReturnUnsupportedMode): return "no such mode";
	case err_get_code(kIOReturnUnderrun): return "data underrun";
	case err_get_code(kIOReturnOverrun): return "data overrun";
	case err_get_code(kIOReturnDeviceError): return "the device is not working properly!";
	case err_get_code(kIOReturnNoCompletion): return "a completion routine is required";
	case err_get_code(kIOReturnAborted): return "operation aborted";
	case err_get_code(kIOReturnNoBandwidth): return "bus bandwidth would be exceeded";
	case err_get_code(kIOReturnNotResponding): return "device not responding";
	case err_get_code(kIOReturnIsoTooOld): return "isochronous I/O request too old";
	case err_get_code(kIOReturnIsoTooNew): return "isochronous I/O request too new";
	case err_get_code(kIOReturnNotFound): return "data was not found";
	}
	return "";
}

std::string IOKit_USB_Error(kern_return_t kr)
{
	switch (err_get_code(kr))
	{
	case err_get_code(kIOUSBUnknownPipeErr): return "Pipe ref not recognized";
	case err_get_code(kIOUSBTooManyPipesErr): return "Too many pipes";
	case err_get_code(kIOUSBNoAsyncPortErr): return "no async port";
	case err_get_code(kIOUSBNotEnoughPipesErr): return "not enough pipes in interface";
	case err_get_code(kIOUSBNotEnoughPowerErr): return "not enough power for selected configuration";
	case err_get_code(kIOUSBEndpointNotFound): return "Endpoint Not found";
	case err_get_code(kIOUSBConfigNotFound): return "Configuration Not found";
	case err_get_code(kIOUSBTransactionTimeout): return "Transaction timed out";
	case err_get_code(kIOUSBTransactionReturned): return "The transaction has been returned to the caller";
	case err_get_code(kIOUSBPipeStalled): return "Pipe has stalled, error needs to be cleared";
	case err_get_code(kIOUSBInterfaceNotFound): return "Interface ref not recognized";
	case err_get_code(kIOUSBLowLatencyBufferNotPreviouslyAllocated): return "Attempted to use user land low latency isoc calls w/out calling PrepareBuffer (on the data buffer) first ";
	case err_get_code(kIOUSBLowLatencyFrameListNotPreviouslyAllocated): return "Attempted to use user land low latency isoc calls w/out calling PrepareBuffer (on the frame list) first";
	case err_get_code(kIOUSBHighSpeedSplitError): return "Error to hub on high speed bus trying to do split transaction";
	case err_get_code(kIOUSBSyncRequestOnWLThread): return "A synchronous USB request was made on the workloop thread (from a callback?).  Only async requests are permitted in that case";
	case err_get_code(kIOUSBDeviceTransferredToCompanion): return "The device has been tranferred to another controller for enumeration";
	case err_get_code(kIOUSBClearPipeStallNotRecursive): return "IOUSBPipe::ClearPipeStall should not be called recursively";
	case err_get_code(kIOUSBDevicePortWasNotSuspended): return "Port was not suspended";
	case err_get_code(kIOUSBEndpointCountExceeded): return "The endpoint was not created because the controller cannot support more endpoints";
	case err_get_code(kIOUSBDeviceCountExceeded): return "The device cannot be enumerated because the controller cannot support more devices";
	case err_get_code(kIOUSBStreamsNotSupported): return "The request cannot be completed because the XHCI controller does not support streams";
	case err_get_code(kIOUSBInvalidSSEndpoint): return "An endpoint found in a SuperSpeed device is invalid (usually because there is no Endpoint Companion Descriptor)";
	case err_get_code(kIOUSBTooManyTransactionsPending): return "The transaction cannot be submitted because it would exceed the allowed number of pending transactions";
	case err_get_code(kIOUSBLinkErr): return "USB link error";
	case err_get_code(kIOUSBNotSent2Err): return "Transaction not sent";
	case err_get_code(kIOUSBNotSent1Err): return "Transaction not sent";
	case err_get_code(kIOUSBBufferUnderrunErr): return "Buffer Underrun (Host hardware failure on data out, PCI busy?)";
	case err_get_code(kIOUSBBufferOverrunErr): return "Buffer Overrun (Host hardware failure on data out, PCI busy?)";
	case err_get_code(kIOUSBReserved2Err): return "Reserved";
	case err_get_code(kIOUSBReserved1Err): return "Reserved";
	case err_get_code(kIOUSBWrongPIDErr): return "Pipe stall, Bad or wrong PID";
	case err_get_code(kIOUSBPIDCheckErr): return "Pipe stall, PID CRC error";
	case err_get_code(kIOUSBDataToggleErr): return "Pipe stall, Bad data toggle";
	case err_get_code(kIOUSBBitstufErr): return "Pipe stall, bitstuffing";
	case err_get_code(kIOUSBCRCErr): return "Pipe stall, bad CRC";
	}
	return "";
}

std::string KernReturnToString(kern_return_t kr)
{
	std::string msg;
	
	std::string r = "System: ";
	switch (err_get_system(kr))
	{
	case err_get_system(err_kern):
		r += "kernel";
		break;
	case err_get_system(err_us):
		r += "user-space library";
		break;
	case err_get_system(err_server):
		r += "user-space servers";
		break;
	case err_get_system(err_ipc):
		r += "old-ipc";
		break;
	case err_get_system(err_mach_ipc):
		r += "mach-ipc";
		break;
	case err_get_system(err_dipc):
		r += "distributed ipc";
		break;
	case err_get_system(err_local):
		r += "user defined";
		break;
	case err_get_system(err_ipc_compat):
		r += "mach-ipc (compat)";
		break;
	case err_get_system(sys_iokit):
		r += "IOKit";
		break;
	default:
		r += to_string(err_get_system(kr));
		break;
	}

	r += "; Sub: ";

	if (err_get_system(kr) == err_get_system(sys_iokit))
	{
		switch (err_get_sub(kr))
		{
		case err_get_sub(sub_iokit_common):
			r += "common";
			msg = IOKit_Common_Error(kr);
			break;
		case err_get_sub(sub_iokit_usb):
			r += "usb";
			msg = IOKit_USB_Error(kr);
			break;
		case err_get_sub(sub_iokit_firewire):
			r += "firewire";
			break;
		case err_get_sub(sub_iokit_block_storage):
			r += "block_storage";
			break;
		case err_get_sub(sub_iokit_graphics):
			r += "graphics";
			break;
		case err_get_sub(sub_iokit_networking):
			r += "networking";
			break;
		case err_get_sub(sub_iokit_bluetooth):
			r += "bluetooth";
			break;
		case err_get_sub(sub_iokit_pmu):
			r += "pmu";
			break;
		case err_get_sub(sub_iokit_acpi):
			r += "acpi";
			break;
		case err_get_sub(sub_iokit_smbus):
			r += "smbus";
			break;
		case err_get_sub(sub_iokit_ahci):
			r += "ahci";
			break;
		case err_get_sub(sub_iokit_powermanagement):
			r += "powermanagement";
			break;
		case err_get_sub(sub_iokit_hidsystem):
			r += "hidsystem";
			break;
		case err_get_sub(sub_iokit_scsi):
			r += "scsi";
			break;
		case err_get_sub(sub_iokit_usbaudio):
			r += "usbaudio";
			break;
		case err_get_sub(err_sub(21)):
			r += "pccard";
			break;
		case err_get_sub(sub_iokit_thunderbolt):
			r += "thunderbolt";
			break;
		case err_get_sub(sub_iokit_platform):
			r += "platform";
			break;
		case err_get_sub(sub_iokit_audio_video):
			r += "audio_video";
			break;
		case err_get_sub(sub_iokit_baseband):
			r += "baseband";
			break;
		case err_get_sub(sub_iokit_HDA):
			r += "HDA";
			break;
		case err_get_sub(sub_iokit_hsic):
			r += "hsic";
			break;
		case err_get_sub(sub_iokit_sdio):
			r += "sdio";
			break;
		case err_get_sub(sub_iokit_wlan):
			r += "wlan";
			break;
		default:
			r += to_string(err_get_sub(kr));
			break;
		}
	}
	else
	{
		r += to_string(err_get_sub(kr));
	}

	r += "; Code: ";

	if (msg.empty())
	{
		switch (err_get_code(kr))
		{
		case err_get_code(KERN_SUCCESS): r += "SUCCESS"; break;
		case err_get_code(KERN_INVALID_ADDRESS): r += "INVALID_ADDRESS"; break;
		case err_get_code(KERN_PROTECTION_FAILURE): r += "PROTECTION_FAILURE"; break;
		case err_get_code(KERN_NO_SPACE): r += "NO_SPACE"; break;
		case err_get_code(KERN_INVALID_ARGUMENT): r += "INVALID_ARGUMENT"; break;
		case err_get_code(KERN_FAILURE): r += "FAILURE"; break;
		case err_get_code(KERN_RESOURCE_SHORTAGE): r += "RESOURCE_SHORTAGE"; break;
		case err_get_code(KERN_NOT_RECEIVER): r += "NOT_RECEIVER"; break;
		case err_get_code(KERN_NO_ACCESS): r += "NO_ACCESS"; break;
		case err_get_code(KERN_MEMORY_FAILURE): r += "MEMORY_FAILURE"; break;
		case err_get_code(KERN_MEMORY_ERROR): r += "MEMORY_ERROR"; break;
		case err_get_code(KERN_ALREADY_IN_SET): r += "ALREADY_IN_SET"; break;
		case err_get_code(KERN_NOT_IN_SET): r += "NOT_IN_SET"; break;
		case err_get_code(KERN_NAME_EXISTS): r += "NAME_EXISTS"; break;
		case err_get_code(KERN_ABORTED): r += "ABORTED"; break;
		case err_get_code(KERN_INVALID_NAME): r += "INVALID_NAME"; break;
		case err_get_code(KERN_INVALID_TASK): r += "INVALID_TASK"; break;
		case err_get_code(KERN_INVALID_RIGHT): r += "INVALID_RIGHT"; break;
		case err_get_code(KERN_INVALID_VALUE): r += "INVALID_VALUE"; break;
		case err_get_code(KERN_UREFS_OVERFLOW): r += "UREFS_OVERFLOW"; break;
		case err_get_code(KERN_INVALID_CAPABILITY): r += "INVALID_CAPABILITY"; break;
		case err_get_code(KERN_RIGHT_EXISTS): r += "RIGHT_EXISTS"; break;
		case err_get_code(KERN_INVALID_HOST): r += "INVALID_HOST"; break;
		case err_get_code(KERN_MEMORY_PRESENT): r += "MEMORY_PRESENT"; break;
		case err_get_code(KERN_MEMORY_DATA_MOVED): r += "MEMORY_DATA_MOVED"; break;
		case err_get_code(KERN_MEMORY_RESTART_COPY): r += "MEMORY_RESTART_COPY"; break;
		case err_get_code(KERN_INVALID_PROCESSOR_SET): r += "INVALID_PROCESSOR_SET"; break;
		case err_get_code(KERN_POLICY_LIMIT): r += "POLICY_LIMIT"; break;
		case err_get_code(KERN_INVALID_POLICY): r += "INVALID_POLICY"; break;
		case err_get_code(KERN_INVALID_OBJECT): r += "INVALID_OBJECT"; break;
		case err_get_code(KERN_ALREADY_WAITING): r += "ALREADY_WAITING"; break;
		case err_get_code(KERN_DEFAULT_SET): r += "DEFAULT_SET"; break;
		case err_get_code(KERN_EXCEPTION_PROTECTED): r += "EXCEPTION_PROTECTED"; break;
		case err_get_code(KERN_INVALID_LEDGER): r += "INVALID_LEDGER"; break;
		case err_get_code(KERN_INVALID_MEMORY_CONTROL): r += "INVALID_MEMORY_CONTROL"; break;
		case err_get_code(KERN_INVALID_SECURITY): r += "INVALID_SECURITY"; break;
		case err_get_code(KERN_NOT_DEPRESSED): r += "NOT_DEPRESSED"; break;
		case err_get_code(KERN_TERMINATED): r += "TERMINATED"; break;
		case err_get_code(KERN_LOCK_SET_DESTROYED): r += "LOCK_SET_DESTROYED"; break;
		case err_get_code(KERN_LOCK_UNSTABLE): r += "LOCK_UNSTABLE"; break;
		case err_get_code(KERN_LOCK_OWNED): r += "LOCK_OWNED"; break;
		case err_get_code(KERN_LOCK_OWNED_SELF): r += "LOCK_OWNED_SELF"; break;
		case err_get_code(KERN_SEMAPHORE_DESTROYED): r += "SEMAPHORE_DESTROYED"; break;
		case err_get_code(KERN_RPC_SERVER_TERMINATED): r += "RPC_SERVER_TERMINATED"; break;
		case err_get_code(KERN_RPC_TERMINATE_ORPHAN): r += "RPC_TERMINATE_ORPHAN"; break;
		case err_get_code(KERN_RPC_CONTINUE_ORPHAN): r += "RPC_CONTINUE_ORPHAN"; break;
		case err_get_code(KERN_NOT_SUPPORTED): r += "NOT_SUPPORTED"; break;
		case err_get_code(KERN_NODE_DOWN): r += "NODE_DOWN"; break;
		case err_get_code(KERN_NOT_WAITING): r += "NOT_WAITING"; break;
		case err_get_code(KERN_OPERATION_TIMED_OUT): r += "OPERATION_TIMED_OUT"; break;
		case err_get_code(KERN_CODESIGN_ERROR): r += "CODESIGN_ERROR"; break;
		case err_get_code(KERN_POLICY_STATIC): r += "POLICY_STATIC"; break;
		case err_get_code(KERN_INSUFFICIENT_BUFFER_SIZE): r += "INSUFFICIENT_BUFFER_SIZE"; break;
		default: r += to_string(err_get_code(kr)); break;
		}
	}
	else
	{
		r += msg;
	}

	return r;
}

#endif
