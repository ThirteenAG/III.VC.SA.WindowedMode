using System;
using System.Diagnostics;
using System.Runtime.InteropServices;

class minjector
{
    public static minjector Instance { get; private set; }

    static Process curProcess;
    static IntPtr hProc;

    static minjector()
    {
        Instance = new minjector();
    }

    ~minjector()
    {
        if (hProc != null)
            CloseHandle(hProc);
    }

    public static void SetProcess(Process p)
    {
        if (hProc != null)
            CloseHandle(hProc);

        curProcess = p;
        hProc = OpenProcess(ProcessAccessFlags.All, false, curProcess.Id);
    }

    public static Process GetProcess()
    {
        try
        {
            if (curProcess.MainModule == null)
            {
                curProcess = null;
            }
        }
        catch
        {
            curProcess = null;
        }
        return curProcess;
    }

    public static bool ProtectMemory(object addr, Int32 size, UInt32 protection)
    {
        return VirtualProtectEx(hProc, ConvertTo<IntPtr>(addr), size, protection, out protection) != false;
    }

    public static bool UnprotectMemory(object addr, Int32 size, ref UInt32 out_oldprotect)
    {
        const UInt32 PAGE_EXECUTE_READWRITE = 0x40;
        return VirtualProtectEx(hProc, ConvertTo<IntPtr>(addr), size, PAGE_EXECUTE_READWRITE, out out_oldprotect) != false;
    }

    public static UInt32 WriteMemory<T>(object addr, T value, bool vp = false)
    {
        var valSize = Marshal.SizeOf(value);
        UInt32 oldprotect = 0;
        UInt32 bytesWritten = 0;
        if (vp)
            UnprotectMemory(addr, valSize, ref oldprotect);

        WriteProcessMemory(hProc, ConvertTo<IntPtr>(addr), ToByteArray(value, valSize), valSize, out bytesWritten);

        if (vp)
            ProtectMemory(addr, valSize, oldprotect);

        return bytesWritten;
    }

    public static T ReadMemory<T>(object addr, bool vp = false)
    {
        T result = default(T);
        var valSize = Marshal.SizeOf(result);
        UInt32 bytesRead = 0;
        UInt32 oldprotect = 0;

        if (vp)
            UnprotectMemory(addr, valSize, ref oldprotect);

        var buf = ToByteArray(result, valSize);
        ReadProcessMemory(hProc, ConvertTo<IntPtr>(addr), buf, valSize, out bytesRead);
        result = FromByteArray<T>(buf);

        if (vp)
            ProtectMemory(addr, valSize, oldprotect);

        return result;
    }

    public static T FromByteArray<T>(byte[] rawValue)
    {
        GCHandle handle = GCHandle.Alloc(rawValue, GCHandleType.Pinned);
        T structure = (T)Marshal.PtrToStructure(handle.AddrOfPinnedObject(), typeof(T));
        handle.Free();
        return structure;
    }

    public static byte[] ToByteArray(object value, int maxLength)
    {
        Int32 rawsize = Marshal.SizeOf(value);
        byte[] rawdata = new byte[rawsize];
        GCHandle handle = GCHandle.Alloc(rawdata, GCHandleType.Pinned);
        Marshal.StructureToPtr(value, handle.AddrOfPinnedObject(), false);
        handle.Free();
        if (maxLength < rawdata.Length)
        {
            byte[] temp = new byte[maxLength];
            Array.Copy(rawdata, temp, maxLength);
            return temp;
        }
        else
        {
            return rawdata;
        }
    }

    public static T ConvertTo<T>(object obj)
    {
        return FromByteArray<T>(ToByteArray(obj, Marshal.SizeOf(obj)));
    }

    [DllImport("kernel32.dll")]
    static extern IntPtr OpenProcess(ProcessAccessFlags dwDesiredAccess, [MarshalAs(UnmanagedType.Bool)] bool bInheritHandle, Int32 dwProcessId);

    [DllImport("kernel32.dll")]
    public static extern bool ReadProcessMemory(IntPtr hProcess, IntPtr lpBaseAddress, byte[] lpBuffer, Int32 nSize, out UInt32 lpNumberOfBytesRead);

    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool WriteProcessMemory(IntPtr hProcess, IntPtr lpBaseAddress, byte[] lpBuffer, Int32 nSize, out UInt32 lpNumberOfBytesWritten);

    [DllImport("kernel32.dll")]
    public static extern Int32 CloseHandle(IntPtr hProcess);

    [DllImport("kernel32.dll")]
    public static extern bool VirtualProtectEx(IntPtr hProcess, IntPtr lpAddress, Int32 dwSize, UInt32 flNewProtect, out UInt32 lpflOldProtect);
}

[Flags]
public enum ProcessAccessFlags : UInt32
{
    All = 0x001F0FFF,
    Terminate = 0x00000001,
    CreateThread = 0x00000002,
    VMOperation = 0x00000008,
    VMRead = 0x00000010,
    VMWrite = 0x00000020,
    DupHandle = 0x00000040,
    SetInformation = 0x00000200,
    QueryInformation = 0x00000400,
    Synchronize = 0x00100000
}