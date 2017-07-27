## @file
#
##

[Defines]
  PLATFORM_NAME                  = Uefibutt
  PLATFORM_GUID                  = 2cd1be40-6c85-4e32-afca-48449a3d36e3
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 1.27
  OUTPUT_DIRECTORY               = build/Uefibutt
  SUPPORTED_ARCHITECTURES        = X64
  BUILD_TARGETS                  = DEBUG
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = DuetPkg/DuetPkg.fdf
!if $(TOOL_CHAIN_TAG) == GCC47 || $(TOOL_CHAIN_TAG) == GCC48 || $(TOOL_CHAIN_TAG) == GCC49 || $(TOOL_CHAIN_TAG) == GCC5
  POSTBUILD                      = Uefibutt/PostBuild.sh
!else
  POSTBUILD                      = DuetPkg/PostBuild.bat
!endif

[LibraryClasses]
  # Entry point
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf

  # Basic
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  PciLib|MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  PciCf8Lib|MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  PciExpressLib|MdePkg/Library/BasePciExpressLib/BasePciExpressLib.inf
  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf

  # UEFI & PI
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiDecompressLib|MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf
  UefiCpuLib|UefiCpuPkg/Library/BaseUefiCpuLib/BaseUefiCpuLib.inf

  # Misc
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  SerialPortLib|MdeModulePkg/Library/BaseSerialPortLib16550/BaseSerialPortLib16550.inf
  LocalApicLib|UefiCpuPkg/Library/BaseXApicLib/BaseXApicLib.inf
  MpInitLib|UefiCpuPkg/Library/MpInitLib/DxeMpInitLib.inf

[PcdsFixedAtBuild]

[PcdsFeatureFlag]

[Components]
  Uefibutt/Uefibutt.inf

[BuildOptions]

