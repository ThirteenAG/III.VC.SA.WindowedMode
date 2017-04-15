using System;
using System.Runtime.InteropServices;

namespace III.VC.SA.CoordsManager
{
    public static class GameVersion
    {
        static char game, region, major, minor, majorRevision, minorRevision, cracker, steam;

        public static bool DetectGameVersion()
        {
            try
            {
                UInt32 baseAddr = (UInt32)minjector.GetProcess().MainModule.BaseAddress;
                var dos_e_lfanew = minjector.ReadMemory<UInt32>(baseAddr + (UInt32)Marshal.OffsetOf(typeof(IMAGE_DOS_HEADER), "e_lfanew"));
                UInt32 nt = baseAddr + dos_e_lfanew;
                var AddressOfEntryPoint = minjector.ReadMemory<UInt32>(nt + (UInt32)Marshal.OffsetOf(typeof(IMAGE_NT_HEADERS), "OptionalHeader") + (UInt32)Marshal.OffsetOf(typeof(IMAGE_OPTIONAL_HEADER), "AddressOfEntryPoint"));

                switch (AddressOfEntryPoint + 0x400000)
                {
                    case 0x5C1E70:  // GTA III 1.0
                        game = '3'; major = (char)1; minor = (char)0; region = (char)0; steam = (char)0;
                        return true;

                    case 0x5C2130:  // GTA III 1.1
                        game = '3'; major = (char)1; minor = (char)1; region = (char)0; steam = (char)0;
                        return true;

                    case 0x5C6FD0:  // GTA III 1.1 (Cracked Steam Version)
                    case 0x9912ED:  // GTA III 1.1 (Encrypted Steam Version)
                        game = '3'; major = (char)1; minor = (char)1; region = (char)0; steam = (char)1;
                        return true;

                    case 0x667BF0:  // GTA VC 1.0
                        game = 'V'; major = (char)1; minor = (char)0; region = (char)0; steam = (char)0;
                        return true;

                    case 0x667C40:  // GTA VC 1.1
                        game = 'V'; major = (char)1; minor = (char)1; region = (char)0; steam = (char)0;
                        return true;

                    case 0x666BA0:  // GTA VC 1.1 (Cracked Steam Version)
                    case 0xA402ED:  // GTA VC 1.1 (Encrypted Steam Version)
                        game = 'V'; major = (char)1; minor = (char)1; region = (char)0; steam = (char)1;
                        return true;

                    case 0x82457C:  // GTA SA 1.0 US Cracked
                    case 0x824570:  // GTA SA 1.0 US Compact
                        game = 'S'; major = (char)1; minor = (char)0; region = 'U'; steam = (char)0;
                        return true;

                    case 0x8245BC:  // GTA SA 1.0 EU Cracked (??????)
                    case 0x8245B0:  // GTA SA 1.0 EU Cracked
                        game = 'S'; major = (char)1; minor = (char)0; region = 'E'; steam = (char)0;
                        return true;

                    case 0x8252FC:  // GTA SA 1.1 US Cracked
                        game = 'S'; major = (char)1; minor = (char)1; region = 'U'; steam = (char)0;
                        return true;

                    case 0x82533C:  // GTA SA 1.1 EU Cracked
                        game = 'S'; major = (char)1; minor = (char)1; region = 'E'; steam = (char)0;
                        return true;

                    case 0x85EC4A:  // GTA SA 3.0 (Cracked Steam Version)
                    case 0xD3C3DB:  // GTA SA 3.0 (Encrypted Steam Version)
                        game = 'S'; major = (char)3; minor = (char)0; region = (char)0; steam = (char)1;
                        return true;

                    default:
                        return false;
                }
            }
            catch
            {
                return false;
            }
        }

        // Checks if I don't know the game we are attached to
        public static bool IsUnknown() { return game == 0; }
        // Checks if this is the steam version
        public static bool IsSteam() { return steam != 0; }
        // Gets the game we are attached to (0, '3', 'V', 'S', 'I', 'E')
        public static char GetGame() { return game; }
        // Gets the region from the game we are attached to (0, 'U', 'E');
        public static char GetRegion() { return region; }
        // Get major and minor version of the game (e.g. [major = 1, minor = 0] = 1.0)
        public static int GetMajorVersion() { return major; }
        public static int GetMinorVersion() { return minor; }
        public static int GetMajorRevisionVersion() { return majorRevision; }
        public static int GetMinorRevisionVersion() { return minorRevision; }

        public static bool IsHoodlum() { return cracker == 'H'; }

        // Region conditions
        public static bool IsUS() { return region == 'U'; }
        public static bool IsEU() { return region == 'E'; }

        // Game Conditions
        public static bool IsIII() { return game == '3'; }
        public static bool IsVC() { return game == 'V'; }
        public static bool IsSA() { return game == 'S'; }
        public static bool IsIV() { return game == 'I'; }
        public static bool IsEFLC() { return game == 'E'; }

        [DllImport("kernel32.dll")]
        private static extern IntPtr GetModuleHandle(string lpModuleName);

        [StructLayout(LayoutKind.Sequential)]
        public struct IMAGE_DOS_HEADER
        {
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 2)]
            public char[] e_magic;
            public UInt16 e_cblp;
            public UInt16 e_cp;
            public UInt16 e_crlc;
            public UInt16 e_cparhdr;
            public UInt16 e_minalloc;
            public UInt16 e_maxalloc;
            public UInt16 e_ss;
            public UInt16 e_sp;
            public UInt16 e_csum;
            public UInt16 e_ip;
            public UInt16 e_cs;
            public UInt16 e_lfarlc;
            public UInt16 e_ovno;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
            public UInt16[] e_res1;
            public UInt16 e_oemid;
            public UInt16 e_oeminfo;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 10)]
            public UInt16[] e_res2;
            public Int32 e_lfanew;

            private string _e_magic
            {
                get
                {
                    return new string(e_magic);
                }
            }

            public bool isValid
            {
                get
                {
                    return _e_magic == "MZ";
                }
            }
        }

        [StructLayout(LayoutKind.Sequential, Pack = 4)]
        public struct IMAGE_FILE_HEADER
        {
            public UInt16 Machine;
            public UInt16 NumberOfSections;
            public UInt32 TimeDateStamp;
            public UInt32 PointerToSymbolTable;
            public UInt32 NumberOfSymbols;
            public UInt16 SizeOfOptionalHeader;
            public UInt16 Characteristics;
        }

        public const Int32 IMAGE_NUMBEROF_DIRECTORY_ENTRIES = 16;
        [StructLayout(LayoutKind.Sequential, Pack = 4)]
        public struct IMAGE_OPTIONAL_HEADER
        {
            public UInt16 Magic;
            public Byte MajorLinkerVersion;
            public Byte MinorLinkerVersion;
            public UInt32 SizeOfCode;
            public UInt32 SizeOfInitializedData;
            public UInt32 SizeOfUninitializedData;
            public UInt32 AddressOfEntryPoint;
            public UInt32 BaseOfCode;
            public UInt32 BaseOfData;
            public UInt32 ImageBase;
            public UInt32 SectionAlignment;
            public UInt32 FileAlignment;
            public UInt16 MajorOperatingSystemVersion;
            public UInt16 MinorOperatingSystemVersion;
            public UInt16 MajorImageVersion;
            public UInt16 MinorImageVersion;
            public UInt16 MajorSubsystemVersion;
            public UInt16 MinorSubsystemVersion;
            public UInt32 Win32VersionValue;
            public UInt32 SizeOfImage;
            public UInt32 SizeOfHeaders;
            public UInt32 CheckSum;
            public UInt16 Subsystem;
            public UInt16 DllCharacteristics;
            public UInt32 SizeOfStackReserve;
            public UInt32 SizeOfStackCommit;
            public UInt32 SizeOfHeapReserve;
            public UInt32 SizeOfHeapCommit;
            public UInt32 LoaderFlags;
            public UInt32 NumberOfRvaAndSizes;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = IMAGE_NUMBEROF_DIRECTORY_ENTRIES)]
            public IMAGE_DATA_DIRECTORY[] DataDirectory;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 4)]
        public struct IMAGE_DATA_DIRECTORY
        {
            public UInt32 VirtualAddress;
            public UInt32 Size;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct IMAGE_NT_HEADERS
        {
            public UInt32 Signature;
            public IMAGE_FILE_HEADER FileHeader;
            public IMAGE_OPTIONAL_HEADER OptionalHeader;
        }
    }
}
