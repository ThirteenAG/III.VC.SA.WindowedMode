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

    public static bool ProtectMemory(IntPtr addr, UIntPtr size, uint protection)
    {
        return VirtualProtectEx(hProc, addr, size, protection, out protection) != false;
    }

    public static bool UnprotectMemory(IntPtr addr, UIntPtr size, ref uint out_oldprotect)
    {
        const uint PAGE_EXECUTE_READWRITE = 0x40;
        return VirtualProtectEx(hProc, addr, size, PAGE_EXECUTE_READWRITE, out out_oldprotect) != false;
    }

    public static int WriteMemory<T>(uint addr, T value, bool vp = false)
    {
        UIntPtr valSize = (UIntPtr)Marshal.SizeOf(value);
        uint oldprotect = 0;
        int bytesWritten = 0;
        if (vp)
            UnprotectMemory((IntPtr)addr, valSize, ref oldprotect);

        WriteProcessMemory(hProc, (IntPtr)addr, ToByteArray(value, (int)valSize), (uint)valSize, out bytesWritten);

        if (vp)
            ProtectMemory((IntPtr)addr, valSize, oldprotect);

        return bytesWritten;
    }

    public static T ReadMemory<T>(uint addr, bool vp = false)
    {
        T result = default(T);
        var valSize = Marshal.SizeOf(result);
        int bytesRead = 0;
        uint oldprotect = 0;

        if (vp)
            UnprotectMemory((IntPtr)addr, (UIntPtr)valSize, ref oldprotect);

        var buf = ToByteArray(result, valSize);
        ReadProcessMemory(hProc, (IntPtr)addr, buf, (uint)valSize, out bytesRead);
        result = FromByteArray<T>(buf);

        if (vp)
            ProtectMemory((IntPtr)addr, (UIntPtr)valSize, oldprotect);

        return result;
    }

    public static uint ReadMemory(uint addr, bool vp = false)
    {
        return ReadMemory<uint>(addr, vp);
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
        int rawsize = Marshal.SizeOf(value);
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

    [DllImport("kernel32.dll")]
    static extern IntPtr OpenProcess(ProcessAccessFlags dwDesiredAccess, [MarshalAs(UnmanagedType.Bool)] bool bInheritHandle, int dwProcessId);

    [DllImport("kernel32.dll")]
    public static extern bool ReadProcessMemory(IntPtr hProcess, IntPtr lpBaseAddress, byte[] lpBuffer, uint nSize, out int lpNumberOfBytesRead);

    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool WriteProcessMemory(IntPtr hProcess, IntPtr lpBaseAddress, byte[] lpBuffer, uint nSize, out int lpNumberOfBytesWritten);

    [DllImport("kernel32.dll")]
    public static extern Int32 CloseHandle(IntPtr hProcess);

    [DllImport("kernel32.dll")]
    public static extern bool VirtualProtectEx(IntPtr hProcess, IntPtr lpAddress, UIntPtr dwSize, uint flNewProtect, out uint lpflOldProtect);
}

[Flags]
public enum ProcessAccessFlags : uint
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