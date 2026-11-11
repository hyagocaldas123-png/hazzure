// C# / Control.cs / .NET Framework 4.5+ x64 / local module control
using System;
using System.Diagnostics;
using System.IO;
using System.Globalization;
using System.IO.MemoryMappedFiles;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

static class Control {
    [DllImport("kernel32.dll",SetLastError=true)] static extern IntPtr OpenProcess(uint rights,bool inherit,int pid);
    [DllImport("kernel32.dll",SetLastError=true)] static extern IntPtr VirtualAllocEx(IntPtr process,IntPtr address,UIntPtr size,uint type,uint protection);
    [DllImport("kernel32.dll",SetLastError=true)] static extern bool VirtualFreeEx(IntPtr process,IntPtr address,UIntPtr size,uint type);
    [DllImport("kernel32.dll",SetLastError=true)] static extern bool WriteProcessMemory(IntPtr process,IntPtr address,byte[] bytes,UIntPtr size,out UIntPtr written);
    [DllImport("kernel32.dll",SetLastError=true)] static extern bool ReadProcessMemory(IntPtr process,IntPtr address,byte[] bytes,UIntPtr size,out UIntPtr read);
    [DllImport("kernel32.dll",SetLastError=true)] static extern IntPtr CreateRemoteThread(IntPtr process,IntPtr attributes,UIntPtr stack,IntPtr start,IntPtr parameter,uint flags,out uint tid);
    [DllImport("kernel32.dll")] static extern uint WaitForSingleObject(IntPtr handle,uint ms);
    [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr handle);
    [DllImport("kernel32.dll",CharSet=CharSet.Unicode)] static extern IntPtr GetModuleHandle(string name);
    [DllImport("kernel32.dll",CharSet=CharSet.Ansi)] static extern IntPtr GetProcAddress(IntPtr module,string name);
    static void Guard(int pid) {
        using(var process=Process.GetProcessById(pid))if(process.ProcessName!="cs2")throw new Exception("Target must be cs2.exe");
    }
    static void Load(int pid,string path) {
        Guard(pid);path=Path.GetFullPath(path);if(!File.Exists(path))throw new FileNotFoundException(path);
        using(var game=Process.GetProcessById(pid))foreach(ProcessModule m in game.Modules)
            if(string.Equals(m.FileName,path,StringComparison.OrdinalIgnoreCase)) { Console.WriteLine("Module already loaded");return; }
        IntPtr load=GetProcAddress(GetModuleHandle("kernel32.dll"),"LoadLibraryW");
        long delta=0;string owner=null;
        using(var here=Process.GetCurrentProcess())foreach(ProcessModule m in here.Modules) {
            long offset=load.ToInt64()-m.BaseAddress.ToInt64();
            if(offset>=0&&offset<m.ModuleMemorySize) { delta=offset;owner=m.ModuleName;break; }
        }
        IntPtr remoteLoad=IntPtr.Zero;
        using(var game=Process.GetProcessById(pid))foreach(ProcessModule m in game.Modules)
            if(string.Equals(m.ModuleName,owner,StringComparison.OrdinalIgnoreCase))remoteLoad=new IntPtr(m.BaseAddress.ToInt64()+delta);
        if(remoteLoad==IntPtr.Zero)throw new Exception("Loader function owner not found");
        IntPtr process=OpenProcess(0x2|0x8|0x10|0x20|0x400|0x100000,false,pid),memory=IntPtr.Zero,thread=IntPtr.Zero;
        bool finished=true;
        try {
            if(process==IntPtr.Zero)throw new System.ComponentModel.Win32Exception();
            byte[] bytes=Encoding.Unicode.GetBytes(path+"\0");
            memory=VirtualAllocEx(process,IntPtr.Zero,(UIntPtr)bytes.Length,0x3000,4);
            UIntPtr count;if(memory==IntPtr.Zero||!WriteProcessMemory(process,memory,bytes,(UIntPtr)bytes.Length,out count)||count.ToUInt64()!=(ulong)bytes.Length)throw new System.ComponentModel.Win32Exception();
            uint tid;thread=CreateRemoteThread(process,IntPtr.Zero,UIntPtr.Zero,remoteLoad,memory,0,out tid);
            if(thread==IntPtr.Zero)throw new System.ComponentModel.Win32Exception();
            finished=false;
            uint wait=WaitForSingleObject(thread,10000);
            finished=wait==0;
            if(!finished)throw new Exception("Load not confirmed; remote argument retained until game exits");
            bool found=false;
            using(var game=Process.GetProcessById(pid))foreach(ProcessModule m in game.Modules)
                if(string.Equals(m.FileName,path,StringComparison.OrdinalIgnoreCase))found=true;
            if(!found)throw new Exception("LoadLibrary did not load module");
            Console.WriteLine("Loaded: "+path);
        }finally {
            if(memory!=IntPtr.Zero&&finished)VirtualFreeEx(process,memory,UIntPtr.Zero,0x8000);
            if(thread!=IntPtr.Zero)CloseHandle(thread);
            if(process!=IntPtr.Zero)CloseHandle(process);
        }
    }
    static int Status(int pid,int? flags) {
        using(var map=MemoryMappedFile.OpenExisting("Local\\HazzureInternal."+pid,MemoryMappedFileRights.ReadWrite))
        using(var view=map.CreateViewAccessor(0,4096,MemoryMappedFileAccess.ReadWrite)) {
            if(view.ReadUInt32(0)!=0x485a4931||view.ReadUInt32(4)!=2)throw new Exception("IPC version mismatch");
            if(flags.HasValue) { Guard(pid);if(flags.Value<0||flags.Value>7)throw new Exception("Flags must be 0..7");view.Write(8,flags.Value); }
            byte[] text=new byte[216];view.ReadArray(40,text,0,text.Length);
            Console.WriteLine("status={0} flags={1} calls={2} changed={3} faults={4} message={5}",view.ReadInt32(12),view.ReadInt32(8),view.ReadInt64(16),view.ReadInt64(24),view.ReadInt64(32),ReadString(view,40,216));
            Console.WriteLine("skins={0} revision={1}/{2} frames={3} stages=0x{4:X} thread={5} applied={6} message={7}",view.ReadInt32(268),view.ReadInt32(260),view.ReadInt32(256),view.ReadInt64(272),view.ReadInt32(288),view.ReadInt32(292),view.ReadInt64(280),ReadString(view,312,200));
            Console.WriteLine("audioProtocol={0} localHeadshots={1}",view.ReadInt32(2576),view.ReadInt64(2584));
            return view.ReadInt32(12)<0?1:0;
        }
    }
    static string ReadString(MemoryMappedViewAccessor view,int offset,int capacity) {
        byte[] bytes=new byte[capacity];view.ReadArray(offset,bytes,0,capacity);
        int end=Array.IndexOf(bytes,(byte)0);return Encoding.ASCII.GetString(bytes,0,end<0?capacity:end);
    }
    static byte[] Read(IntPtr process,long address,int size) {
        byte[] result=new byte[size];UIntPtr count;
        if(address==0||!ReadProcessMemory(process,new IntPtr(address),result,(UIntPtr)size,out count)||count.ToUInt64()!=(ulong)size)throw new Exception("Diagnostic read failed");
        return result;
    }
    static long Pointer(IntPtr process,long address){return BitConverter.ToInt64(Read(process,address,8),0);}
    static uint Number(IntPtr process,long address){return BitConverter.ToUInt32(Read(process,address,4),0);}
    static long Resolve(IntPtr process,long client,uint handle) {
        if(handle==0xffffffff)return 0;
        long list=Pointer(process,client+0x271A908),chunk=Pointer(process,list+0x10+8*((handle&0x7fff)>>9));
        long slot=chunk+0x70*(handle&0x1ff);
        return Number(process,slot+0x10)==handle?Pointer(process,slot):0;
    }
    static void Weapon(int pid) {
        Guard(pid);long client=0;
        using(var game=Process.GetProcessById(pid))foreach(ProcessModule module in game.Modules)if(module.ModuleName=="client.dll")client=module.BaseAddress.ToInt64();
        IntPtr process=OpenProcess(0x410,false,pid);
        try {
            long pawn=Pointer(process,client+0x2565718),services=Pointer(process,pawn+0x12F0);
            uint handle=Number(process,services+0x60);long weapon=Resolve(process,client,handle);
            if(weapon==0)throw new Exception("No active weapon");
            long scene=Pointer(process,weapon+0x330),name=Pointer(process,scene+0x1e8);
            byte[] model=Read(process,name,240);int end=Array.IndexOf(model,(byte)0);
            Console.WriteLine("handle=0x{0:X} def={1} high=0x{2:X} low=0x{3:X} fallback={4}/{5}/{6} attributes={7} mask={8} model={9}",handle,BitConverter.ToUInt16(Read(process,weapon+0x149a,2),0),Number(process,weapon+0x14b0),Number(process,weapon+0x14b4),Number(process,weapon+0x18a8),Number(process,weapon+0x18ac),BitConverter.ToSingle(Read(process,weapon+0x18b0,4),0),Number(process,weapon+0x1568),Pointer(process,scene+0x348),Encoding.ASCII.GetString(model,0,end<0?model.Length:end));
            int count=(int)Number(process,weapon+0x1568);long entries=Pointer(process,weapon+0x1570);
            if(count<0||count>128)throw new Exception("Attribute count invalid");
            for(int i=0;i<count;i++)Console.WriteLine("attribute={0} value={1}",BitConverter.ToUInt16(Read(process,entries+0x48*i+0x30,2),0),BitConverter.ToSingle(Read(process,entries+0x48*i+0x34,4),0));
        }finally{if(process!=IntPtr.Zero)CloseHandle(process);}
    }
    static void Configure(int pid,string[] args) {
        Guard(pid);
        using(var map=MemoryMappedFile.OpenExisting("Local\\HazzureInternal."+pid,MemoryMappedFileRights.ReadWrite))
        using(var view=map.CreateViewAccessor(0,4096,MemoryMappedFileAccess.ReadWrite))
        using(var mutex=new Mutex(false,"Local\\HazzureInternal.Config."+pid)) {
            if(!mutex.WaitOne(2000))throw new Exception("Configuration writer busy");
            try {
                if(view.ReadUInt32(4)!=2)throw new Exception("IPC version mismatch");
                int revision=view.ReadInt32(256);if((revision&1)!=0)throw new Exception("Incomplete prior configuration write");
                byte[] payload=new byte[64*32];view.ReadArray(512,payload,0,payload.Length);
                int enabled=view.ReadInt32(264);
                if(args[0]=="--skins-off")enabled=0;
                else {
                    int def=int.Parse(args[2]),paint=int.Parse(args[3]),seed=int.Parse(args[4]);
                    float wear=float.Parse(args[5],CultureInfo.InvariantCulture);
                    int model=args.Length>6?int.Parse(args[6]):0,legacy=args.Length>7?int.Parse(args[7]):0;
                    if(def<1||def>526||paint<0||paint>20000||seed<0||seed>1000||float.IsNaN(wear)||wear<0||wear>1||model<0||model>526||(legacy!=0&&legacy!=1))throw new Exception("Invalid skin values");
                    int slot=-1;
                    for(int i=0;i<64;i++)if(BitConverter.ToInt32(payload,i*32)==def){slot=i;break;}
                    if(slot<0)for(int i=0;i<64;i++)if(BitConverter.ToInt32(payload,i*32+24)==0){slot=i;break;}
                    if(slot<0)throw new Exception("Skin profile limit reached");
                    int offset=slot*32;
                    Buffer.BlockCopy(BitConverter.GetBytes(def),0,payload,offset,4);
                    Buffer.BlockCopy(BitConverter.GetBytes(paint),0,payload,offset+4,4);
                    Buffer.BlockCopy(BitConverter.GetBytes(seed),0,payload,offset+8,4);
                    Buffer.BlockCopy(BitConverter.GetBytes(model),0,payload,offset+12,4);
                    Buffer.BlockCopy(BitConverter.GetBytes(wear),0,payload,offset+16,4);
                    Buffer.BlockCopy(BitConverter.GetBytes(legacy),0,payload,offset+20,4);
                    Buffer.BlockCopy(BitConverter.GetBytes(1),0,payload,offset+24,4);Buffer.BlockCopy(BitConverter.GetBytes(1),0,payload,offset+28,4);enabled=1;
                }
                view.Write(256,revision+1);Thread.MemoryBarrier();
                view.WriteArray(512,payload,0,payload.Length);view.Write(264,enabled);
                Thread.MemoryBarrier();view.Write(256,revision+2);
            }finally{mutex.ReleaseMutex();}
        }
    }
    static int Main(string[] args) {
        try {
            if(args.Length<2)throw new Exception("Usage: Hazzure.Internal.Control.exe --load PID DLL | --status PID | --flags PID 0..7 | --skin PID DEF KIT SEED WEAR [MODEL] [LEGACY] | --skins-off PID");
            int pid=int.Parse(args[1]);
            if(args[0]=="--weapon"&&args.Length==2){Weapon(pid);return 0;}
            if(args[0]=="--load"&&args.Length==3) {
                Load(pid,args[2]);for(int i=0;i<50;i++) {
                    try { return Status(pid,null); }catch(FileNotFoundException) { Thread.Sleep(100); }
                }
                throw new Exception("Module status unavailable");
            }
            if((args[0]=="--skin"&&args.Length>=6&&args.Length<=8)||(args[0]=="--skins-off"&&args.Length==2)) { Configure(pid,args);return Status(pid,null); }
            if(args[0]=="--status"&&args.Length==2)return Status(pid,null);
            if(args[0]=="--flags"&&args.Length==3)return Status(pid,int.Parse(args[2]));
            throw new Exception("Invalid arguments");
        }catch(Exception ex) { Console.Error.WriteLine(ex.Message);return 1; }
    }
}
