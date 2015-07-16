//===-- usb.c - avrlit USB CDC Device Implementation ----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "usb.h"

const USB_Descriptor_Device_t PROGMEM DeviceDescriptor = {
	.Header = {
        .Size = sizeof(USB_Descriptor_Device_t),
        .Type = DTYPE_Device
    },
	.USBSpecification       = VERSION_BCD(1,1,0),
	.Class                  = USB_CSCP_IADDeviceClass,
	.SubClass               = USB_CSCP_IADDeviceSubclass,
	.Protocol               = USB_CSCP_IADDeviceProtocol,

	.Endpoint0Size          = FIXED_CONTROL_ENDPOINT_SIZE,

	.VendorID               = 0x03EB,
	.ProductID              = 0x204E,
	.ReleaseNumber          = VERSION_BCD(0,0,1),

	.ManufacturerStrIndex   = STRING_ID_Manufacturer,
	.ProductStrIndex        = STRING_ID_Product,
	.SerialNumStrIndex      = USE_INTERNAL_SERIAL,

	.NumberOfConfigurations = FIXED_NUM_CONFIGURATIONS
};

const USB_Descriptor_Configuration_t PROGMEM ConfigurationDescriptor = {
  .Config = {
    .Header = {
      .Size = sizeof(USB_Descriptor_Configuration_Header_t),
      .Type = DTYPE_Configuration
    },
    .TotalConfigurationSize = sizeof(USB_Descriptor_Configuration_t),
    .TotalInterfaces        = 4,

    .ConfigurationNumber    = 1,
    .ConfigurationStrIndex  = NO_DESCRIPTOR,

    .ConfigAttributes       = (USB_CONFIG_ATTR_RESERVED | USB_CONFIG_ATTR_SELFPOWERED),

    .MaxPowerConsumption    = USB_CONFIG_POWER_MA(100)
  },

  .CDC_IAD = {
    .Header = {
      .Size = sizeof(USB_Descriptor_Interface_Association_t),
      .Type = DTYPE_InterfaceAssociation
    },
    .FirstInterfaceIndex = INTERFACE_ID_CDC_CCI,
    .TotalInterfaces     = 2,

    .Class               = CDC_CSCP_CDCClass,
    .SubClass            = CDC_CSCP_ACMSubclass,
    .Protocol            = CDC_CSCP_ATCommandProtocol,

    .IADStrIndex         = NO_DESCRIPTOR
  },

  .CDC_CCI_Interface = {
    .Header = {
      .Size = sizeof(USB_Descriptor_Interface_t),
      .Type = DTYPE_Interface
    },
    .InterfaceNumber   = INTERFACE_ID_CDC_CCI,
    .AlternateSetting  = 0,

    .TotalEndpoints    = 1,

    .Class             = CDC_CSCP_CDCClass,
    .SubClass          = CDC_CSCP_ACMSubclass,
    .Protocol          = CDC_CSCP_ATCommandProtocol,

    .InterfaceStrIndex = NO_DESCRIPTOR
  },

  .CDC_Functional_Header = {
    .Header = {
      .Size = sizeof(USB_CDC_Descriptor_FunctionalHeader_t),
      .Type = DTYPE_CSInterface
    },
    .Subtype          = CDC_DSUBTYPE_CSInterface_Header,

    .CDCSpecification = VERSION_BCD(1,1,0),
  },

  .CDC_Functional_ACM = {
    .Header = {
      .Size = sizeof(USB_CDC_Descriptor_FunctionalACM_t),
      .Type = DTYPE_CSInterface
    },
    .Subtype      = CDC_DSUBTYPE_CSInterface_ACM,

    .Capabilities = 0x06,
  },

  .CDC_Functional_Union = {
    .Header = {
      .Size = sizeof(USB_CDC_Descriptor_FunctionalUnion_t),
      .Type = DTYPE_CSInterface
    },
    .Subtype               = CDC_DSUBTYPE_CSInterface_Union,

    .MasterInterfaceNumber = INTERFACE_ID_CDC_CCI,
    .SlaveInterfaceNumber  = INTERFACE_ID_CDC_DCI,
  },

  .CDC_ManagementEndpoint = {
    .Header = {
      .Size = sizeof(USB_Descriptor_Endpoint_t),
      .Type = DTYPE_Endpoint
    },
    .EndpointAddress   = CDC_NOTIFICATION_EPADDR,
    .Attributes        = (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
    .EndpointSize      = CDC_NOTIFICATION_EPSIZE,
    .PollingIntervalMS = 0xFF
  },

  .CDC_DCI_Interface = {
    .Header = {
      .Size = sizeof(USB_Descriptor_Interface_t),
      .Type = DTYPE_Interface
    },
    .InterfaceNumber   = INTERFACE_ID_CDC_DCI,
    .AlternateSetting  = 0,

    .TotalEndpoints    = 2,

    .Class             = CDC_CSCP_CDCDataClass,
    .SubClass          = CDC_CSCP_NoDataSubclass,
    .Protocol          = CDC_CSCP_NoDataProtocol,

    .InterfaceStrIndex = NO_DESCRIPTOR
  },

  .CDC_DataOutEndpoint = {
    .Header = {
      .Size = sizeof(USB_Descriptor_Endpoint_t),
      .Type = DTYPE_Endpoint
    },
    .EndpointAddress   = CDC_RX_EPADDR,
    .Attributes        = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
    .EndpointSize      = CDC_TXRX_EPSIZE,
    .PollingIntervalMS = 0x05
  },

  .CDC_DataInEndpoint = {
    .Header = {
      .Size = sizeof(USB_Descriptor_Endpoint_t),
      .Type = DTYPE_Endpoint
    },
    .EndpointAddress   = CDC_TX_EPADDR,
    .Attributes        = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
    .EndpointSize      = CDC_TXRX_EPSIZE,
    .PollingIntervalMS = 0x05
  },
};

const USB_Descriptor_String_t PROGMEM LanguageString = USB_STRING_DESCRIPTOR_ARRAY(LANGUAGE_ID_ENG);
const USB_Descriptor_String_t PROGMEM ManufacturerString = USB_STRING_DESCRIPTOR(L"Dean Camera");
const USB_Descriptor_String_t PROGMEM ProductString = USB_STRING_DESCRIPTOR(L"LUFA Dual CDC Demo");

USB_ClassInfo_CDC_Device_t cdc = {
  .Config = {
    .ControlInterfaceNumber = INTERFACE_ID_CDC_CCI,
    .DataINEndpoint = {
      .Address = CDC_TX_EPADDR,
      .Size    = CDC_TXRX_EPSIZE,
      .Banks   = 1,
    },
    .DataOUTEndpoint = {
      .Address = CDC_RX_EPADDR,
      .Size    = CDC_TXRX_EPSIZE,
      .Banks   = 1,
    },
    .NotificationEndpoint = {
      .Address = CDC_NOTIFICATION_EPADDR,
      .Size    = CDC_NOTIFICATION_EPSIZE,
      .Banks   = 1,
    }
  }
};

uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint8_t wIndex,
                                    const void** const DescriptorAddress)
{
  const uint8_t  DescriptorType   = (wValue >> 8);
  const uint8_t  DescriptorNumber = (wValue & 0xFF);

  const void* Address = NULL;
  uint16_t    Size    = NO_DESCRIPTOR;

  switch (DescriptorType) {
    case DTYPE_Device:
      Address = &DeviceDescriptor;
      Size    = sizeof(USB_Descriptor_Device_t);
      break;
    case DTYPE_Configuration:
      Address = &ConfigurationDescriptor;
      Size    = sizeof(USB_Descriptor_Configuration_t);
      break;
    case DTYPE_String:
      switch (DescriptorNumber) {
        case STRING_ID_Language:
          Address = &LanguageString;
          Size    = pgm_read_byte(&LanguageString.Header.Size);
          break;
        case STRING_ID_Manufacturer:
          Address = &ManufacturerString;
          Size    = pgm_read_byte(&ManufacturerString.Header.Size);
          break;
        case STRING_ID_Product:
          Address = &ProductString;
          Size    = pgm_read_byte(&ProductString.Header.Size);
          break;
      }
      break;
  }

	*DescriptorAddress = Address;
	return Size;
}

