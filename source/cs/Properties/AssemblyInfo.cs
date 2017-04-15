using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

// General Information about an assembly is controlled through the following 
// set of attributes. Change these attribute values to modify the information
// associated with an assembly.
[assembly: AssemblyTitle("III.VC.SA.CoordsManager")]
[assembly: AssemblyDescription("III.VC.SA.CoordsManager")]
[assembly: AssemblyConfiguration("")]
[assembly: AssemblyCompany("ThirteenAG")]
[assembly: AssemblyProduct("III.VC.SA.CoordsManager")]
[assembly: AssemblyCopyright("MIT License")]
[assembly: AssemblyTrademark("")]
[assembly: AssemblyCulture("")]

// Setting ComVisible to false makes the types in this assembly not visible 
// to COM components.  If you need to access a type in this assembly from 
// COM, set the ComVisible attribute to true on that type.
[assembly: ComVisible(false)]

// The following GUID is for the ID of the typelib if this project is exposed to COM
[assembly: Guid("41e898bd-c4b7-46ec-a0c4-746b2c144de8")]

// Version information for an assembly consists of the following four values:
//
//      Major Version
//      Minor Version 
//      Build Number
//      Revision
//
// You can specify all the values or you can default the Build and Revision Numbers 
// by using the '*' as shown below:
// [assembly: AssemblyVersion("1.0.*")]
[assembly: AssemblyVersion("1.0.0.0")]
[assembly: AssemblyFileVersion("1.0.0.0")]
[assembly: UpdateUrlAttribute("UpdateUrl https://github.com/ThirteenAG/III.VC.SA.WindowedMode")]

[System.AttributeUsage(System.AttributeTargets.Assembly)]
public class UpdateUrlAttribute : System.Attribute
{
    string url;
    public UpdateUrlAttribute() : this(string.Empty) { }
    public UpdateUrlAttribute(string txt) { url = txt; }
}

