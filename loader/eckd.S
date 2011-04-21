# During the system IPL, 24 bytes are read from the device.
#
# NOTE: zArch IPLs in ESA/390 mode.
#

#
# Bytes 0-7 contain PSW to be loaded after IO operation completes
#
	.byte	0x00
		#   bits  value   name                        desc
		#      0      0   <zero>
		#      1      0   PER Mask (R)                disabled
		#    2-4      0   <zero>
		#      5      0   DAT Mode (T)                disabled
		#      6      0   I/O Mask (IO)               disabled
		#      7      0   External Mask (EX)          disabled

	.byte	0x08
		#   bits  value   name                        desc
		#   8-11      0   Key
		#     12      1   <one>
		#     13      0   Machine-Check Mask (M)      disabled
		#     14      0   Wait State (W)              executing
		#     15      0   Problem State (P)           supervisor state
		
	.byte	0x00
		#   bits  value   name                        desc
		#  16-17      0   Address-Space Control (AS)  disabled
		#  18-19      0   Condition Code (CC)
		#  20-23      0   Program Mask                exceptions disabled

	.byte	0x00
		#   bits  value   name                        desc
		#  24-30      0   <zero>
		#     31      0   Extended Addressing (EA)    ! 64 mode

	.byte	0x80	# bits 32-39
	.byte	0x80	# bits 40-47
	.byte	0x00	# bits 48-55
	.byte	0x00	# bits 56-63
		#   bits  value   name                        desc
		#     32      1   Basic Addressing (BA)       BA = 31, !BA = 24
		#  33-63   addr   Instruction Address         Address to exec

#
# The remaining 16 bytes should contain CCW to read data from device
#

# CCW format-0:
#   bits  name
#    0-7  Cmd Code
#   8-31  Data Address
#     32  Chain-Data (CD)
#     33  Chain-Command (CC)
#     34  Sup.-Len.-Inditcation (SLI)
#     35  Skip (SKP)
#     36  Prog.-Contr.-Inter. (PCI)
#     37  Indir.-Data-Addr. (IDA)
#     38  Suspend (S)
#     39  Modified I.D.A. (MIDA)
#  40-47  <ignored>
#  48-63  number of bytes to read

#
# CCW 1 (bytes 8-15): format-0
#
	# READ DATA 4 kB to 0x800000
	.byte	0x86, 0x80, 0x00, 0x00
	.byte	0x00, 0x00, 0x10, 0x00

#
# CCW 2 (bytes 16-23): format-0
#
	# invalid (unused)
	.byte	0x00, 0x00, 0x00, 0x00
	.byte	0x00, 0x00, 0x00, 0x00
